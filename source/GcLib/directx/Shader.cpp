#include "source/GcLib/pch.h"
#include "Shader.hpp"
#include "HLSL.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//ShaderData
**********************************************************/
ShaderData::ShaderData() {
	bLoad_ = false;
	effect_ = nullptr;
	bText_ = false;
}
ShaderData::~ShaderData() {

}

/**********************************************************
//Shader
**********************************************************/
gstd::CriticalSection Shader::lock_ = gstd::CriticalSection();
std::wstring Shader::lastError_ = L"";
Shader::Shader() {
	data_ = nullptr;
}
Shader::Shader(Shader* shader) {
	{
		Lock lock(lock_);
		data_ = shader->data_;
	}
}
Shader::~Shader() {
	Release();
}
void Shader::Release() {
	{
		Lock lock(lock_);

		if (data_ != nullptr) {
			if (data_->effect_)
				data_->effect_->Release();
			data_->effect_ = nullptr;

			delete data_;
			data_ = nullptr;
		}
	}
}

ID3DXEffect* Shader::GetEffect() {
	ID3DXEffect* res = nullptr;
	if (data_ != nullptr) res = data_->effect_;
	return res;
}

bool Shader::CreateFromFile(std::wstring path) {
	path = PathProperty::GetUnique(path);
	ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == NULL || !reader->Open()) {
		std::wstring log = StringUtility::Format(L"ShaderManager: Shader load failed (%s)\r\n\t[%s]",
			"Cannot open file.", path.c_str());
		lastError_ = log;
		Logger::WriteTop(log);
		return false;
	}

	int size = reader->GetFileSize();
	ByteBuffer buf;
	buf.SetSize(size);
	reader->Read(buf.GetPointer(), size);

	std::string source;
	source.resize(size);
	memcpy(&source[0], buf.GetPointer(), size);

	ShaderData* data = new ShaderData();
	data_ = data;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	ID3DXBuffer* pErr = nullptr;
	HRESULT hr = D3DXCreateEffect(
		graphics->GetDevice(),
		source.c_str(),
		source.size(),
		nullptr, nullptr,
		0,
		nullptr,
		&data->effect_,
		&pErr
	);

	if (FAILED(hr)) {
		std::wstring err = L"";
		if (pErr != nullptr) {
			char* cText = (char*)pErr->GetBufferPointer();
			err = StringUtility::ConvertMultiToWide(cText);
		}
		std::wstring log = StringUtility::Format(L"ShaderManager: Shader load failed (%s)\r\n\t[%s]",
			err.c_str(), path.c_str());
		lastError_ = log;
		Logger::WriteTop(log);

		return false;
	}
	else {
		std::wstring log = StringUtility::Format(L"ShaderManager: Shader loaded [%s]", path.c_str());
		lastError_ = log;
		Logger::WriteTop(log);

		data->name_ = path;

		return true;
	}
}
bool Shader::CreateFromText(std::string& source) {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	std::wstring id = StringUtility::ConvertMultiToWide(source);
	id = StringUtility::Slice(id, 64);

	ShaderData* data = new ShaderData();
	data_ = data;

	ID3DXBuffer* pErr = nullptr;
	HRESULT hr = D3DXCreateEffect(
		graphics->GetDevice(),
		source.c_str(),
		source.size(),
		nullptr, nullptr,
		0,
		nullptr,
		&data->effect_,
		&pErr
	);

	std::string tStr = StringUtility::Slice(source, 128);
	if (FAILED(hr)) {
		char* err = "";
		if (pErr != nullptr)err = (char*)pErr->GetBufferPointer();
		std::string log = StringUtility::Format("ShaderManager: Shader load failed: (%s)\r\n\t[%s]", err, tStr.c_str());
		lastError_ = StringUtility::ConvertMultiToWide(log);
		Logger::WriteTop(log);
		
		return false;
	}
	else {
		std::string log = StringUtility::Format("ShaderManager: Shader loaded [%s]", tStr.c_str());
		lastError_ = StringUtility::ConvertMultiToWide(log);
		Logger::WriteTop(log);

		data->name_ = id;
		data->bText_ = true;

		return true;
	}
}

bool Shader::SetTexture(std::string name, gstd::ref_count_ptr<Texture> texture) {
	//gstd::ref_count_ptr<ShaderParameter> param = _GetParameter(name, true);
	//param->SetTexture(texture);
	paramTexture_ = texture;
	data_->effect_->SetTexture(name.c_str(), texture->GetD3DTexture());

	return true;
}

RenderShaderManager* RenderShaderManager::thisBase_ = nullptr;
RenderShaderManager::RenderShaderManager() {
	effectSkinnedMesh_ = nullptr;
	effectRender2D_ = nullptr;

	declarationTLX_ = nullptr;
	declarationLX_ = nullptr;
	declarationNX_ = nullptr;

	Initialize();
}
RenderShaderManager::~RenderShaderManager() {
	//Release();
}

void RenderShaderManager::Initialize() {
	if (thisBase_ != nullptr) return;

	ID3DXBuffer* error = nullptr;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	HRESULT hr = D3DXCreateEffect(device, HLSL_DEFAULT_SKINNED_MESH.c_str(), HLSL_DEFAULT_SKINNED_MESH.size(),
		nullptr, nullptr, 0, nullptr, &effectSkinnedMesh_, &error);
	if (FAILED(hr)) {
		std::string err = StringUtility::Format("RenderShaderManager: Shader load failed. (HLSL_DEFAULT_SKINNED_MESH)\n[%s]",
			reinterpret_cast<const char*>(error->GetBufferPointer()));
		throw gstd::wexception(err);
	}
	
	hr = D3DXCreateEffect(device, HLSL_DEFAULT_RENDER2D.c_str(), HLSL_DEFAULT_RENDER2D.size(),
		nullptr, nullptr, 0, nullptr, &effectRender2D_, &error);
	if (FAILED(hr)) {
		std::string err = StringUtility::Format("RenderShaderManager: Shader load failed. (HLSL_DEFAULT_RENDER2D)\n[%s]",
			reinterpret_cast<const char*>(error->GetBufferPointer()));
		throw gstd::wexception(err);
	}
	effectRender2D_->SetTechnique("Render");

	/*
	hr = D3DXCreateEffect(device, HLSL_DEFAULT_RENDER3D.c_str(), HLSL_DEFAULT_RENDER3D.size(),
		nullptr, nullptr, 0, nullptr, &effectRender3D_, &error);
	if (FAILED(hr)) {
		std::string err = StringUtility::Format("RenderShaderManager: Shader load failed. (HLSL_DEFAULT_RENDER3D)\n[%s]",
			reinterpret_cast<const char*>(error->GetBufferPointer()));
		throw gstd::wexception(err);
	}

	hr = D3DXCreateEffect(device, HLSL_DEFAULT_RENDERTARGETLAYER.c_str(), HLSL_DEFAULT_RENDERTARGETLAYER.size(),
		nullptr, nullptr, 0, nullptr, &effectRenderTarget_, &error);
	if (FAILED(hr)) {
		std::string err = StringUtility::Format("RenderShaderManager: Shader load failed. (HLSL_DEFAULT_RENDERTARGETLAYER)\n[%s]",
			reinterpret_cast<const char*>(error->GetBufferPointer()));
		throw gstd::wexception(err);
	}
	*/

	hr = device->CreateVertexDeclaration(ELEMENTS_TLX, &declarationTLX_);
	if (FAILED(hr)) {
		std::wstring err = L"RenderShaderManager: CreateVertexDeclaration failed. (ELEMENTS_TLX)";
		throw gstd::wexception(err);
	}
	hr = device->CreateVertexDeclaration(ELEMENTS_LX, &declarationLX_);
	if (FAILED(hr)) {
		std::wstring err = L"RenderShaderManager: CreateVertexDeclaration failed. (ELEMENTS_LX)";
		throw gstd::wexception(err);
	}
	hr = device->CreateVertexDeclaration(ELEMENTS_NX, &declarationNX_);
	if (FAILED(hr)) {
		std::wstring err = L"RenderShaderManager: CreateVertexDeclaration failed. (ELEMENTS_NX)";
		throw gstd::wexception(err);
	}

	thisBase_ = this;
}
void RenderShaderManager::Release() {
	if (effectSkinnedMesh_ != nullptr) {
		effectSkinnedMesh_->Release();
		effectSkinnedMesh_ = nullptr;
	}
	if (effectRender2D_ != nullptr) {
		effectRender2D_->Release();
		effectRender2D_ = nullptr;
	}
	if (declarationTLX_ != nullptr) {
		declarationTLX_->Release();
		declarationTLX_ = nullptr;
	}
	if (declarationLX_ != nullptr) {
		declarationLX_->Release();
		declarationLX_ = nullptr;
	}
	if (declarationNX_ != nullptr) {
		declarationNX_->Release();
		declarationNX_ = nullptr;
	}
}