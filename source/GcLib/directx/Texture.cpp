#include "source/GcLib/pch.h"

#include "Texture.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//TextureData
**********************************************************/
TextureData::TextureData() {
	manager_ = nullptr;
	pTexture_ = nullptr;
	lpRenderSurface_ = nullptr;
	lpRenderZ_ = nullptr;
	bLoad_ = true;

	useMipMap_ = false;
	useNonPowerOfTwo_ = false;

	resourceSize_ = 0U;

	ZeroMemory(&infoImage_, sizeof(D3DXIMAGE_INFO));
	type_ = TYPE_TEXTURE;
}
TextureData::~TextureData() {
	ptr_release(pTexture_);
	ptr_release(lpRenderSurface_);
	ptr_release(lpRenderZ_);
}

/**********************************************************
//Texture
**********************************************************/
Texture::Texture() {}
Texture::Texture(Texture* texture) {
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		data_ = texture->data_;
	}
}
Texture::~Texture() {
	Release();
}
void Texture::Release() {
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) {
			TextureManager* manager = data_->manager_;
			if (manager != nullptr && manager->IsDataExists(data_->name_)) {
				//自身とTextureManager内の数だけになったら削除
				if (data_.use_count() == 2) {
					manager->_ReleaseTextureData(data_->name_);
				}
			}
			data_ = nullptr;
		}
	}
}
std::wstring Texture::GetName() {
	std::wstring res = L"";
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) res = data_->GetName();
	}
	return res;
}
bool Texture::CreateFromFile(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo) {
	path = PathProperty::GetUnique(path);

	bool res = false;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) Release();

		TextureManager* manager = TextureManager::GetBase();
		ref_count_ptr<Texture> texture = manager->CreateFromFile(path, genMipmap, flgNonPowerOfTwo);
		if (texture) data_ = texture->data_;
		res = data_ != nullptr;
	}

	return res;
}

bool Texture::CreateRenderTarget(std::wstring name, size_t width, size_t height) {
	bool res = false;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) Release();

		TextureManager* manager = TextureManager::GetBase();
		ref_count_ptr<Texture> texture = manager->CreateRenderTarget(name, width, height);
		if (texture) data_ = texture->data_;
		res = data_ != nullptr;
	}
	return res;
}
bool Texture::CreateFromFileInLoadThread(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo, bool bLoadImageInfo) {
	path = PathProperty::GetUnique(path);

	bool res = false;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) Release();

		TextureManager* manager = TextureManager::GetBase();
		ref_count_ptr<Texture> texture = manager->CreateFromFileInLoadThread(path, bLoadImageInfo, genMipmap, flgNonPowerOfTwo);
		if (texture) data_ = texture->data_;
		res = data_ != nullptr;
	}

	return res;
}
void Texture::SetTexture(IDirect3DTexture9 *pTexture) {
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) Release();

		TextureData* textureData = new TextureData();
		textureData->pTexture_ = pTexture;
		D3DSURFACE_DESC desc;
		pTexture->GetLevelDesc(0, &desc);

		D3DXIMAGE_INFO* infoImage = &textureData->infoImage_;
		infoImage->Width = desc.Width;
		infoImage->Height = desc.Height;
		infoImage->Format = desc.Format;
		infoImage->ImageFileFormat = D3DXIFF_BMP;
		infoImage->ResourceType = D3DRTYPE_TEXTURE;

		data_ = shared_ptr<TextureData>(textureData);
	}
}

IDirect3DTexture9* Texture::GetD3DTexture() {
	IDirect3DTexture9* res = nullptr;
	{
		bool bWait = true;
		int time = timeGetTime();
		while (bWait) {
			Lock lock(TextureManager::GetBase()->GetLock());
			if (data_ != nullptr) {
				bWait = !data_->bLoad_;
				if (!bWait)
					res = _GetTextureData()->pTexture_;

				if (bWait && abs((int)(timeGetTime() - time)) > 10000) {
					//一定時間たってもだめだったらロック？
					std::wstring path = data_->GetName();
					Logger::WriteTop(
						StringUtility::Format(L"Texture is possibly locked: %s", path.c_str()));
					data_->bLoad_ = true;
					break;
				}
			}
			else break;

			if (bWait)::Sleep(1);
		}

	}
	return res;
}
IDirect3DSurface9* Texture::GetD3DSurface() {
	IDirect3DSurface9* res = nullptr;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) res = _GetTextureData()->lpRenderSurface_;
	}
	return res;
}
IDirect3DSurface9* Texture::GetD3DZBuffer() {
	IDirect3DSurface9* res = nullptr;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		if (data_) res = _GetTextureData()->lpRenderZ_;
	}
	return res;
}
int Texture::GetWidth() {
	int res = 0;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		TextureData* data = _GetTextureData();
		if (data) res = data->infoImage_.Width;
	}
	return res;
}
int Texture::GetHeight() {
	int res = 0;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		TextureData* data = _GetTextureData();
		if (data) res = data->infoImage_.Height;
	}
	return res;
}
int Texture::GetType() {
	int res = TextureData::TYPE_TEXTURE;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(TextureManager::GetBase()->GetLock());
#endif
		TextureData* data = _GetTextureData();
		if (data) res = data->type_;
	}
	return res;
}
size_t Texture::GetFormatBPP(D3DFORMAT format) {
	switch (format) {
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A8R3G3B2:
	case D3DFMT_X4R4G4B4:
		return 2U;
	case D3DFMT_R8G8B8:
		return 3U;
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A2B10G10R10:
	case D3DFMT_A8B8G8R8:
	case D3DFMT_X8B8G8R8:
	case D3DFMT_G16R16:
	case D3DFMT_A2R10G10B10:
		return 4U;
	case D3DFMT_A16B16G16R16:
		return 8U;
	case D3DFMT_R3G3B2:
	case D3DFMT_A8:
	default:
		return 1U;
	}
}

/**********************************************************
//TextureManager
**********************************************************/
const std::wstring TextureManager::TARGET_TRANSITION = L"__RENDERTARGET_TRANSITION__";
TextureManager* TextureManager::thisBase_ = nullptr;
TextureManager::TextureManager() {

}
TextureManager::~TextureManager() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->RemoveDirectGraphicsListener(this);
	this->Clear();

	FileManager::GetBase()->RemoveLoadThreadListener(this);

	panelInfo_ = nullptr;
	thisBase_ = nullptr;
}
bool TextureManager::Initialize() {
	if (thisBase_ != nullptr)return false;

	thisBase_ = this;
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->AddDirectGraphicsListener(this);

	ref_count_ptr<Texture> texTransition = new Texture();
	bool res = texTransition->CreateRenderTarget(TARGET_TRANSITION);
	Add(TARGET_TRANSITION, texTransition);

	FileManager::GetBase()->AddLoadThreadListener(this);

	return res;
}
void TextureManager::Clear() {
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(lock_);
#endif
		mapTexture_.clear();
		mapTextureData_.clear();
	}
}
void TextureManager::_ReleaseTextureData(std::wstring name) {
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(lock_);
#endif

		auto itr = mapTextureData_.find(name);
		if (itr != mapTextureData_.end()) {
			itr->second->bLoad_ = true;		//読み込み完了扱い
			mapTextureData_.erase(itr);
			Logger::WriteTop(StringUtility::Format(L"TextureManager: Texture released. [%s]", name.c_str()));
		}
	}
}
void TextureManager::ReleaseDxResource() {
	std::map<std::wstring, shared_ptr<TextureData>>::iterator itrMap;
	{
		Lock lock(GetLock());

		for (itrMap = mapTextureData_.begin(); itrMap != mapTextureData_.end(); ++itrMap) {
			std::wstring name = itrMap->first;
			TextureData* data = (itrMap->second).get();

			if (data->type_ == TextureData::TYPE_RENDER_TARGET) {
				ptr_release(data->pTexture_);
				ptr_release(data->lpRenderSurface_);
				ptr_release(data->lpRenderZ_);
			}
		}
	}
}
void TextureManager::RestoreDxResource() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	std::map<std::wstring, shared_ptr<TextureData>>::iterator itrMap;
	{
		Lock lock(GetLock());

		for (itrMap = mapTextureData_.begin(); itrMap != mapTextureData_.end(); ++itrMap) {
			std::wstring name = itrMap->first;
			TextureData* data = (itrMap->second).get();

			if (data->type_ == TextureData::TYPE_RENDER_TARGET) {
				UINT width = data->infoImage_.Width;
				UINT height = data->infoImage_.Height;

				HRESULT hr;
				// Zバッファ生成
				hr = graphics->GetDevice()->CreateDepthStencilSurface(width, height, D3DFMT_D16, D3DMULTISAMPLE_NONE, 
					0, FALSE, &data->lpRenderZ_, nullptr);

				//テクスチャ作成
				D3DFORMAT fmt;
				//if (graphics->GetScreenMode() == DirectGraphics::SCREENMODE_FULLSCREEN) {
					fmt = graphics->GetConfigData().GetColorMode() == DirectGraphicsConfig::COLOR_MODE_32BIT ?
						D3DFMT_A8R8G8B8 : D3DFMT_A4R4G4B4;
				//}
				//else fmt = graphics->GetWindowPresentParameter().BackBufferFormat;

				hr = graphics->GetDevice()->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, fmt, 
					D3DPOOL_DEFAULT, &data->pTexture_, nullptr);
				data->pTexture_->GetSurfaceLevel(0, &data->lpRenderSurface_);
			}
		}
	}
}

bool TextureManager::_CreateFromFile(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo) {
	if (IsDataExists(path)) {
		return true;
	}

	DirectGraphics* graphics = DirectGraphics::GetBase();

	//まだ作成されていないなら、作成
	try {
		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr)throw gstd::wexception("File not found.");
		if (!reader->Open())throw gstd::wexception("Cannot open file for reading.");

		int size = reader->GetFileSize();
		ByteBuffer buf;
		buf.SetSize(size);
		reader->Read(buf.GetPointer(), size);

		//		D3DXIMAGE_INFO info;
		//		D3DXGetImageInfoFromFileInMemory(buf.GetPointer(), size, &info);

		D3DCOLOR colorKey = D3DCOLOR_ARGB(255, 0, 0, 0);
		if (path.find(L".bmp") == std::wstring::npos)//bmpのみカラーキー適応
			colorKey = 0;
		D3DFORMAT pixelFormat = graphics->GetConfigData().GetColorMode() == DirectGraphicsConfig::COLOR_MODE_32BIT ? 
			D3DFMT_A8R8G8B8 : D3DFMT_A4R4G4B4;

		shared_ptr<TextureData> data(new TextureData());
		data->useMipMap_ = genMipmap;
		data->useNonPowerOfTwo_ = flgNonPowerOfTwo;

		HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(DirectGraphics::GetBase()->GetDevice(),
			buf.GetPointer(), size,
			(data->useNonPowerOfTwo_) ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT,
			(data->useNonPowerOfTwo_) ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT,
			0, 0, pixelFormat, D3DPOOL_MANAGED, D3DX_FILTER_BOX, D3DX_DEFAULT, colorKey,
			nullptr, nullptr, &data->pTexture_);
		if (FAILED(hr)) {
			throw gstd::wexception("D3DXCreateTextureFromFileInMemoryEx failure.");
		}

		if (data->useMipMap_) data->pTexture_->GenerateMipSubLevels();

		mapTextureData_[path] = data;
		data->manager_ = this;
		data->name_ = path;
		D3DXGetImageInfoFromFileInMemory(buf.GetPointer(), size, &data->infoImage_);

		data->resourceSize_ = data->infoImage_.Width * data->infoImage_.Height;
		if (data->useMipMap_) {
			UINT wd = data->infoImage_.Width;
			UINT ht = data->infoImage_.Height;
			while (wd > 1U && ht > 1U) {
				wd /= 2U;
				ht /= 2U;
				data->resourceSize_ += wd * ht;
			}
		}
		data->resourceSize_ *= Texture::GetFormatBPP(data->infoImage_.Format);

		Logger::WriteTop(StringUtility::Format(L"TextureManager: Texture loaded. [%s]", path.c_str()));
	}
	catch (gstd::wexception& e) {
		std::wstring str = StringUtility::Format(L"TextureManager: Failed to load texture. [%s]\n\t%s", path.c_str(), e.what());
		Logger::WriteTop(str);
		return false;
	}

	return true;
}
bool TextureManager::_CreateRenderTarget(std::wstring name, size_t width, size_t height) {
	if (IsDataExists(name)) {
		return true;
	}

	bool res = true;
	try {
		shared_ptr<TextureData> data(new TextureData());
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		if (width == 0U) {
			size_t screenWidth = graphics->GetScreenWidth();
			width = 1U;
			while (width <= screenWidth) {
				width *= 2U;
			}
		}
		if (height == 0U) {
			size_t screenHeight = graphics->GetScreenHeight();
			height = 1U;
			while (height <= screenHeight) {
				height *= 2U;
			}
		}
		if (width > 4096U) width = 4096U;
		if (height > 4096U) height = 4096U;
		//Max size is 67,108,864 bytes (64 megabytes)

		HRESULT hr;
		hr = device->CreateDepthStencilSurface(width, height, D3DFMT_D16, D3DMULTISAMPLE_NONE,
			0, FALSE, &data->lpRenderZ_, nullptr);
		if (FAILED(hr))throw false;

		D3DFORMAT fmt = graphics->GetConfigData().GetColorMode() == DirectGraphicsConfig::COLOR_MODE_32BIT ?
			D3DFMT_A8R8G8B8 : D3DFMT_A4R4G4B4;
		hr = device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT,
			&data->pTexture_, nullptr);

		if (FAILED(hr)) {
			//テクスチャを正方形にする
			if (width > height)height = width;
			else if (height > width)width = height;

			hr = device->CreateDepthStencilSurface(width, height, D3DFMT_D16, D3DMULTISAMPLE_NONE,
				0, FALSE, &data->lpRenderZ_, nullptr);
			if (FAILED(hr))throw false;

			hr = device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT,
				&data->pTexture_, nullptr);
			if (FAILED(hr))throw false;
		}
		data->pTexture_->GetSurfaceLevel(0, &data->lpRenderSurface_);

		mapTextureData_[name] = data;
		data->manager_ = this;
		data->name_ = name;
		data->type_ = TextureData::TYPE_RENDER_TARGET;
		data->infoImage_.Width = width;
		data->infoImage_.Height = height;

		data->resourceSize_ = width * height * 4U;

		Logger::WriteTop(StringUtility::Format(L"TextureManager: Render target created. [%s]", name.c_str()));
	}
	catch (...) {
		Logger::WriteTop(StringUtility::Format(L"TextureManager: Failed to create render target. [%s]", name.c_str()));
		res = false;
	}

	return res;
}
gstd::ref_count_ptr<Texture> TextureManager::CreateFromFile(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo) {
	path = PathProperty::GetUnique(path);
	gstd::ref_count_ptr<Texture> res;
	{
		Lock lock(lock_);

		auto itr = mapTexture_.find(path);
		if (itr != mapTexture_.end()) {
			res = itr->second;
		}
		else {
			bool bSuccess = _CreateFromFile(path, genMipmap, flgNonPowerOfTwo);
			if (bSuccess) {
				res = new Texture();
				res->data_ = mapTextureData_[path];
			}
		}
	}
	return res;
}

gstd::ref_count_ptr<Texture> TextureManager::CreateRenderTarget(std::wstring name, size_t width, size_t height) {
	gstd::ref_count_ptr<Texture> res;
	{
		Lock lock(lock_);

		auto itr = mapTexture_.find(name);
		if (itr != mapTexture_.end()) {
			res = itr->second;
		}
		else {
			bool bSuccess = _CreateRenderTarget(name);
			if (bSuccess) {
				res = new Texture();
				res->data_ = mapTextureData_[name];
			}
		}
	}
	return res;
}
gstd::ref_count_ptr<Texture> TextureManager::CreateFromFileInLoadThread(std::wstring path, bool genMipmap, 
	bool flgNonPowerOfTwo, bool bLoadImageInfo) 
{
	path = PathProperty::GetUnique(path);
	gstd::ref_count_ptr<Texture> res;
	{
		Lock lock(lock_);

		auto itr = mapTexture_.find(path);
		if (itr != mapTexture_.end()) {
			res = itr->second;
		}
		else {
			bool bLoadTarget = true;
			res = new Texture();
			if (!IsDataExists(path)) {
				shared_ptr<TextureData> data(new TextureData());
				mapTextureData_[path] = data;
				data->manager_ = this;
				data->name_ = path;
				data->bLoad_ = false;
				data->useMipMap_ = genMipmap;
				data->useNonPowerOfTwo_ = flgNonPowerOfTwo;

				//画像情報だけ事前に読み込み
				if (bLoadImageInfo) {
					try {
						ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
						if (reader == nullptr)throw gstd::wexception("Texture released.");
						if (!reader->Open())throw gstd::wexception("File not found.");

						int size = reader->GetFileSize();
						ByteBuffer buf;
						buf.SetSize(size);
						reader->Read(buf.GetPointer(), size);

						D3DXIMAGE_INFO info;
						HRESULT hr = D3DXGetImageInfoFromFileInMemory(buf.GetPointer(), size, &info);
						if (FAILED(hr)) {
							throw gstd::wexception("D3DXGetImageInfoFromFileInMemory failure.");
						}

						data->resourceSize_ = data->infoImage_.Width * data->infoImage_.Height;
						if (data->useMipMap_) {
							UINT wd = data->infoImage_.Width;
							UINT ht = data->infoImage_.Height;
							while (wd > 1U && ht > 1U) {
								wd /= 2U;
								ht /= 2U;
								data->resourceSize_ += wd * ht;
							}
						}
						data->resourceSize_ *= Texture::GetFormatBPP(data->infoImage_.Format);

						data->infoImage_ = info;
					}
					catch (gstd::wexception& e) {
						std::wstring str = StringUtility::Format(L"TextureManager: Failed to load texture. [%s]\n\t%s", path.c_str(), e.what());
						Logger::WriteTop(str);
						data->bLoad_ = true;//読み込み完了扱い
						bLoadTarget = false;
					}
				}
			}
			else bLoadTarget = false;

			res->data_ = mapTextureData_[path];
			if (bLoadTarget) {
				ref_count_ptr<FileManager::LoadObject> source = res;
				ref_count_ptr<FileManager::LoadThreadEvent> event = new FileManager::LoadThreadEvent(this, path, res);
				FileManager::GetBase()->AddLoadThreadEvent(event);

			}
		}
	}
	return res;
}
void TextureManager::CallFromLoadThread(ref_count_ptr<FileManager::LoadThreadEvent> event) {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	std::wstring path = event->GetPath();
	{
		Lock lock(lock_);

		ref_count_ptr<Texture> texture = ref_count_ptr<Texture>::DownCast(event->GetSource());
		if (texture == nullptr)return;

		shared_ptr<TextureData> data = texture->data_;
		if (data == nullptr || data->bLoad_)return;

		int countRef = data.use_count();
		//自身とTextureManager内の数だけになったら読み込まない。
		if (countRef <= 2) {
			data->bLoad_ = true;//念のため読み込み完了扱い
			return;
		}

		try {
			ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
			if (reader == nullptr)throw gstd::wexception("Texture released.");
			if (!reader->Open())throw gstd::wexception("File not found.");

			int size = reader->GetFileSize();
			ByteBuffer buf;
			buf.SetSize(size);
			reader->Read(buf.GetPointer(), size);

			D3DCOLOR colorKey = D3DCOLOR_ARGB(255, 0, 0, 0);
			if (path.find(L".bmp") == std::wstring::npos)//bmpのみカラーキー適応
				colorKey = 0;

			D3DFORMAT pixelFormat = graphics->GetConfigData().GetColorMode() == DirectGraphicsConfig::COLOR_MODE_32BIT ?
				D3DFMT_A8R8G8B8 : D3DFMT_A4R4G4B4;

			HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(DirectGraphics::GetBase()->GetDevice(), 
				buf.GetPointer(), size,
				(data->useNonPowerOfTwo_) ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT, 
				(data->useNonPowerOfTwo_) ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT, 
				0, 0, pixelFormat, D3DPOOL_MANAGED, D3DX_FILTER_BOX, D3DX_DEFAULT, colorKey,
				nullptr, nullptr, &data->pTexture_);
			if (FAILED(hr)) {
				throw gstd::wexception("D3DXCreateTextureFromFileInMemoryEx failure.");
			}

			if (data->useMipMap_) data->pTexture_->GenerateMipSubLevels();
			D3DXGetImageInfoFromFileInMemory(buf.GetPointer(), size, &data->infoImage_);

			Logger::WriteTop(StringUtility::Format(L"TextureManager: Texture loaded. (Load Thread) [%s]", path.c_str()));
		}
		catch (gstd::wexception& e) {
			std::wstring str = StringUtility::Format(L"TextureManager: Failed to load texture. (Load Thread) [%s]\n\t%s", path.c_str(), e.what());
			Logger::WriteTop(str);
		}
		data->bLoad_ = true;
	}
}

shared_ptr<TextureData> TextureManager::GetTextureData(std::wstring name) {
	shared_ptr<TextureData> res = nullptr;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(lock_);
#endif

		auto itr = mapTextureData_.find(name);
		if (itr != mapTextureData_.end()) {
			res = itr->second;
		}
	}
	return res;
}

gstd::ref_count_ptr<Texture> TextureManager::GetTexture(std::wstring name) {
	gstd::ref_count_ptr<Texture> res;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(lock_);
#endif

		auto itr = mapTexture_.find(name);
		if (itr != mapTexture_.end()) {
			res = itr->second;
		}
	}
	return res;
}

void TextureManager::Add(std::wstring name, gstd::ref_count_ptr<Texture> texture) {
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(lock_);
#endif

		bool bExist = mapTexture_.find(name) != mapTexture_.end();
		if (!bExist) {
			mapTexture_[name] = texture;
		}
	}
}
void TextureManager::Release(std::wstring name) {
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(lock_);
#endif

		mapTexture_.erase(name);
	}
}
bool TextureManager::IsDataExists(std::wstring name) {
	bool res = false;
	{
#ifdef __L_TEXTURE_THREADSAFE
		Lock lock(lock_);
#endif

		res = mapTextureData_.find(name) != mapTextureData_.end();
	}
	return res;
}

/**********************************************************
//TextureInfoPanel
**********************************************************/
TextureInfoPanel::TextureInfoPanel() {
	timeUpdateInterval_ = 500;
}
TextureInfoPanel::~TextureInfoPanel() {
	Stop();
	Join(1000);
}
bool TextureInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);

	wndListView_.AddColumn(64, ROW_ADDRESS, L"Address");
	wndListView_.AddColumn(96, ROW_NAME, L"Name");
	wndListView_.AddColumn(64, ROW_FULLNAME, L"FullName");
	wndListView_.AddColumn(32, ROW_COUNT_REFFRENCE, L"Ref");
	wndListView_.AddColumn(48, ROW_WIDTH_IMAGE, L"Width");
	wndListView_.AddColumn(48, ROW_HEIGHT_IMAGE, L"Height");
	wndListView_.AddColumn(72, ROW_SIZE, L"Size");

	Start();

	return true;
}
void TextureInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	wndListView_.SetBounds(wx, wy, wWidth, wHeight);
}
void TextureInfoPanel::_Run() {
	while (GetStatus() == RUN) {
		TextureManager* manager = TextureManager::GetBase();
		if (manager != nullptr)
			Update(manager);
		Sleep(timeUpdateInterval_);
	}
}
void TextureInfoPanel::Update(TextureManager* manager) {
	if (!IsWindowVisible())return;
	std::set<std::wstring> setKey;
	std::map<std::wstring, shared_ptr<TextureData>>::iterator itrMap;
	{
		Lock lock(manager->GetLock());

		std::map<std::wstring, shared_ptr<TextureData>>& mapData = manager->mapTextureData_;
		for (itrMap = mapData.begin(); itrMap != mapData.end(); itrMap++) {
			std::wstring name = itrMap->first;
			TextureData* data = (itrMap->second).get();

			std::wstring key = StringUtility::Format(L"%08x", (int)data);
			int index = wndListView_.GetIndexInColumn(key, ROW_ADDRESS);
			if (index == -1) {
				index = wndListView_.GetRowCount();
				wndListView_.SetText(index, ROW_ADDRESS, key);
			}

			int countRef = (itrMap->second).use_count();
			D3DXIMAGE_INFO* infoImage = &data->infoImage_;

			wndListView_.SetText(index, ROW_NAME, PathProperty::GetFileName(name));
			wndListView_.SetText(index, ROW_FULLNAME, name);
			wndListView_.SetText(index, ROW_COUNT_REFFRENCE, StringUtility::Format(L"%d", countRef));
			wndListView_.SetText(index, ROW_WIDTH_IMAGE, StringUtility::Format(L"%d", infoImage->Width));
			wndListView_.SetText(index, ROW_HEIGHT_IMAGE, StringUtility::Format(L"%d", infoImage->Height));
			wndListView_.SetText(index, ROW_SIZE, StringUtility::Format(L"%d", data->GetResourceSize()));

			setKey.insert(key);
		}
	}

	for (int iRow = 0; iRow < wndListView_.GetRowCount();) {
		std::wstring key = wndListView_.GetText(iRow, ROW_ADDRESS);
		if (setKey.find(key) != setKey.end())iRow++;
		else wndListView_.DeleteRow(iRow);
	}
}
