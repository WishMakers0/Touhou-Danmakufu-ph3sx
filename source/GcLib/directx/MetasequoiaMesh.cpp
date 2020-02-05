#include "source/GcLib/pch.h"
#include "MetasequoiaMesh.hpp"
#include "DxUtility.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//MetasequoiaMesh
**********************************************************/

//MetasequoiaMeshData
MetasequoiaMeshData::MetasequoiaMeshData() {}
MetasequoiaMeshData::~MetasequoiaMeshData() {
	for (auto obj : renderList_) {
		if (obj) delete obj;
		obj = nullptr;
	}
	for (auto obj : materialList_) {
		if (obj) delete obj;
		obj = nullptr;
	}
}
bool MetasequoiaMeshData::CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader) {
	bool res = false;
	path_ = reader->GetOriginalPath();
	std::string text;
	int size = reader->GetFileSize();
	text.resize(size);
	reader->Read(&text[0], size);

	gstd::Scanner scanner(text);
	try {
		while (scanner.HasNext()) {
			gstd::Token& tok = scanner.Next();
			if (tok.GetElement() == L"Material") {
				_ReadMaterial(scanner);
			}
			else if (tok.GetElement() == L"Object") {
				_ReadObject(scanner);
			}
		}

		res = true;
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(StringUtility::Format(L"MetasequoiaMeshData読み込み失敗 %s %d", e.what(), scanner.GetCurrentLine()));
		res = false;
	}
	return res;
}
void MetasequoiaMeshData::_ReadMaterial(gstd::Scanner& scanner) {
	int countMaterial = scanner.Next().GetInteger();
	materialList_.resize(countMaterial);
	for (int iMat = 0; iMat < countMaterial; iMat++) {
		materialList_[iMat] = new Material();
	}
	scanner.CheckType(scanner.Next(), Token::TK_OPENC);
	scanner.CheckType(scanner.Next(), Token::TK_NEWLINE);

	int posMat = 0;
	Material* mat = materialList_[posMat];
	D3DCOLORVALUE color;
	ZeroMemory(&color, sizeof(D3DCOLORVALUE));
	while (true) {
		gstd::Token& tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC)break;

		if (tok.GetType() == Token::TK_NEWLINE) {
			posMat++;
			if (materialList_.size() <= posMat)
				break;
			mat = materialList_[posMat];
			ZeroMemory(&color, sizeof(D3DCOLORVALUE));
		}
		else if (tok.GetType() == Token::TK_STRING) {
			mat->name_ = tok.GetString();
		}
		else if (tok.GetElement() == L"col") {
			scanner.CheckType(scanner.Next(), Token::TK_OPENP);
			color.r = scanner.Next().GetReal();
			color.g = scanner.Next().GetReal();
			color.b = scanner.Next().GetReal();
			color.a = scanner.Next().GetReal();
			scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
		}
		else if (tok.GetElement() == L"dif" ||
			tok.GetElement() == L"amb" ||
			tok.GetElement() == L"emi" ||
			tok.GetElement() == L"spc") {
			D3DCOLORVALUE *value = NULL;
			if (tok.GetElement() == L"dif")value = &mat->mat_.Diffuse;
			else if (tok.GetElement() == L"amb")value = &mat->mat_.Ambient;
			else if (tok.GetElement() == L"emi")value = &mat->mat_.Emissive;
			else if (tok.GetElement() == L"spc")value = &mat->mat_.Specular;

			scanner.CheckType(scanner.Next(), Token::TK_OPENP);
			float num = scanner.Next().GetReal();
			if (value != NULL) {
				value->a = color.a;
				value->r = color.r * num;
				value->g = color.g * num;
				value->b = color.b * num;
			}
			scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
		}
		else if (tok.GetElement() == L"power") {
			scanner.CheckType(scanner.Next(), Token::TK_OPENP);
			mat->mat_.Power = scanner.Next().GetReal();
			scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
		}
		else if (tok.GetElement() == L"tex") {
			scanner.CheckType(scanner.Next(), Token::TK_OPENP);
			tok = scanner.Next();

			std::wstring wPathTexture = tok.GetString();
			std::wstring path = PathProperty::GetFileDirectory(path_) + wPathTexture;
			mat->texture_ = new Texture();
			mat->texture_->CreateFromFile(path, false, false);
			scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
		}
	}
}
void MetasequoiaMeshData::_ReadObject(gstd::Scanner& scanner) {
	Object obj;
	obj.name_ = scanner.Next().GetString();
	scanner.CheckType(scanner.Next(), Token::TK_OPENC);
	scanner.CheckType(scanner.Next(), Token::TK_NEWLINE);

	std::map<int, std::list<MetasequoiaMeshData::Object::Face*> > mapFace;

	while (true) {
		gstd::Token& tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC)break;

		if (tok.GetElement() == L"visible") {
			obj.bVisible_ = scanner.Next().GetInteger() == 15;
		}
		else if (tok.GetElement() == L"vertex") {
			int count = scanner.Next().GetInteger();
			obj.vertices_.resize(count);
			scanner.CheckType(scanner.Next(), Token::TK_OPENC);
			scanner.CheckType(scanner.Next(), Token::TK_NEWLINE);
			for (int iVert = 0; iVert < count; iVert++) {
				obj.vertices_[iVert].x = scanner.Next().GetReal();
				obj.vertices_[iVert].y = scanner.Next().GetReal();
				obj.vertices_[iVert].z = -scanner.Next().GetReal();
				scanner.CheckType(scanner.Next(), Token::TK_NEWLINE);
			}
			scanner.CheckType(scanner.Next(), Token::TK_CLOSEC);
			scanner.CheckType(scanner.Next(), Token::TK_NEWLINE);
		}
		else if (tok.GetElement() == L"face") {
			int countFace = scanner.Next().GetInteger();
			obj.faces_.resize(countFace);
			scanner.CheckType(scanner.Next(), Token::TK_OPENC);
			scanner.CheckType(scanner.Next(), Token::TK_NEWLINE);

			for (int iFace = 0; iFace < countFace; iFace++) {
				int countVert = scanner.Next().GetInteger();
				obj.faces_[iFace].vertices_.resize(countVert);
				MetasequoiaMeshData::Object::Face* face = &obj.faces_[iFace];
				while (true) {
					gstd::Token& tok = scanner.Next();
					if (tok.GetType() == Token::TK_NEWLINE)break;
					if (tok.GetElement() == L"V") {
						scanner.CheckType(scanner.Next(), Token::TK_OPENP);
						for (int iVert = 0; iVert < countVert; iVert++)
							face->vertices_[iVert].indexVertex_ = scanner.Next().GetInteger();
						scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
					}
					else if (tok.GetElement() == L"M") {
						scanner.CheckType(scanner.Next(), Token::TK_OPENP);
						face->indexMaterial_ = scanner.Next().GetInteger();
						scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
					}
					else if (tok.GetElement() == L"UV") {
						scanner.CheckType(scanner.Next(), Token::TK_OPENP);
						for (int iVert = 0; iVert < countVert; iVert++) {
							face->vertices_[iVert].tcoord_.x = scanner.Next().GetReal();
							face->vertices_[iVert].tcoord_.y = scanner.Next().GetReal();
						}
						scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
					}
				}

				mapFace[face->indexMaterial_].push_back(face);
			}

			scanner.CheckType(scanner.Next(), Token::TK_CLOSEC);
			scanner.CheckType(scanner.Next(), Token::TK_NEWLINE);
		}
		else if (tok.GetType() == Token::TK_ID) {
			while (scanner.GetToken().GetType() != Token::TK_OPENC &&
				scanner.GetToken().GetType() != Token::TK_NEWLINE) {
				scanner.Next();
			}
			if (scanner.GetToken().GetType() == Token::TK_OPENC) {
				while (scanner.GetToken().GetType() != Token::TK_CLOSEC)
					scanner.Next();
			}
		}
	}

	//不可視ならオブジェクトを作成しない
	if (!obj.bVisible_)return;

	//マテリアルごとに仕分けてオブジェクトを作成
	size_t iMapFace = 0U;
	std::map<int, std::list<MetasequoiaMeshData::Object::Face*> >::iterator itrMap;
	for (itrMap = mapFace.begin(); itrMap != mapFace.end(); itrMap++, iMapFace++) {
		int indexMaterial = itrMap->first;
		std::list<MetasequoiaMeshData::Object::Face*>& listFace = itrMap->second;

		MetasequoiaMeshData::RenderObject* render = new MetasequoiaMeshData::RenderObject();
		renderList_.push_back(render);
		if (indexMaterial >= 0)
			render->material_ = materialList_[indexMaterial];

		int countVert = 0;
		std::list<MetasequoiaMeshData::Object::Face*>::iterator itrFace;
		for (itrFace = listFace.begin(); itrFace != listFace.end(); itrFace++) {
			MetasequoiaMeshData::Object::Face* face = *itrFace;
			countVert += face->vertices_.size() == 3 ? 3 : 6;
		}

		std::vector<ref_count_ptr<NormalData> > listNormalData;
		listNormalData.resize(countVert);

		render->SetVertexCount(countVert);

		int posVert = 0;
		for (itrFace = listFace.begin(); itrFace != listFace.end(); itrFace++) {
			MetasequoiaMeshData::Object::Face* face = *itrFace;
			if (face->vertices_.size() == 3) {
				int indexVert[3] =
				{
					face->vertices_[0].indexVertex_,
					face->vertices_[1].indexVertex_,
					face->vertices_[2].indexVertex_,
				};
				D3DXVECTOR3 normal = DxMath::Normalize(
					DxMath::CrossProduct(
						obj.vertices_[indexVert[1]] - obj.vertices_[indexVert[0]],
						obj.vertices_[indexVert[2]] - obj.vertices_[indexVert[0]]
					)
				);

				for (int iVert = 0; iVert < 3; iVert++) {
					int mqoVertexIndex = indexVert[iVert];
					int nxVertexIndex = posVert + iVert;
					VERTEX_NX* vert = render->GetVertex(nxVertexIndex);
					vert->position = obj.vertices_[mqoVertexIndex];
					vert->texcoord = face->vertices_[iVert].tcoord_;
					vert->normal = normal;
				}
				posVert += 3;
			}
			else if (face->vertices_.size() == 4) {
				int indexVert[4] =
				{
					face->vertices_[0].indexVertex_,
					face->vertices_[1].indexVertex_,
					face->vertices_[2].indexVertex_,
					face->vertices_[3].indexVertex_,
				};
				D3DXVECTOR3 normals[2] = {
					DxMath::Normalize(
						DxMath::CrossProduct(
							obj.vertices_[indexVert[1]] - obj.vertices_[indexVert[0]],
							obj.vertices_[indexVert[2]] - obj.vertices_[indexVert[0]]
						)
					),
					DxMath::Normalize(
						DxMath::CrossProduct(
							obj.vertices_[indexVert[3]] - obj.vertices_[indexVert[2]],
							obj.vertices_[indexVert[0]] - obj.vertices_[indexVert[2]]
						)
					)
				};

				for (int iVert = 0; iVert < 6; iVert++) {
					int nxVertexIndex = posVert + iVert;
					VERTEX_NX* vert = render->GetVertex(nxVertexIndex);

					int indexFace = iVert;
					if (iVert == 3)indexFace = 2;
					else if (iVert == 4)indexFace = 3;
					else if (iVert == 5)indexFace = 0;

					int mqoVertexIndex = indexVert[indexFace];
					vert->position = obj.vertices_[mqoVertexIndex];
					vert->texcoord = face->vertices_[indexFace].tcoord_;
					vert->normal = iVert < 3 ? normals[0] : normals[1];
				}

				posVert += 6;
			}
		}

		int countNormalData = listNormalData.size();
		for (int iData = 0; iData < countNormalData; iData++) {
			ref_count_ptr<NormalData> normalData = listNormalData[iData];
			if (normalData != NULL) {
				int countVertexIndex = normalData->listIndex_.size();
				for (int iVert = 0; iVert < countVertexIndex; iVert++) {
					int nvVertexIndex = normalData->listIndex_[iVert];
					VERTEX_NX* vert = render->GetVertex(nvVertexIndex);
					vert->normal = normalData->normal_;
				}
			}
		}

		if (countVert > 0) {
			IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
			IDirect3DVertexBuffer9*& vertexBuf = render->pVertexBuffer_;

			void* pVoid;
			VERTEX_NX* pVertData = render->GetVertex(0);

			device->CreateVertexBuffer(countVert * sizeof(VERTEX_NX), 0, VERTEX_NX::fvf, D3DPOOL_MANAGED,
				&vertexBuf, nullptr);

			vertexBuf->Lock(0, countVert * sizeof(VERTEX_NX), &pVoid, D3DUSAGE_WRITEONLY);
			memcpy(pVoid, pVertData, countVert * sizeof(VERTEX_NX));
			vertexBuf->Unlock();
		}
	}
}

void MetasequoiaMeshData::RenderObject::Render() {
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	if (material_ != nullptr) {
		if (material_->texture_ != nullptr)
			SetTexture(material_->texture_);
		D3DMATERIAL9 tMaterial = ColorAccess::SetColor(material_->mat_, color_);
		device->SetMaterial(&tMaterial);
	}

	RenderObjectNX::Render();
}

//MetasequoiaMesh
bool MetasequoiaMesh::CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader) {
	bool res = false;
	{
		Lock lock(DxMeshManager::GetBase()->GetLock());
		if (data_ != NULL)Release();

		std::wstring name = reader->GetOriginalPath();

		data_ = _GetFromManager(name);
		if (data_ == NULL) {
			if (!reader->Open())throw gstd::wexception(L"ファイルが開けません");
			data_ = new MetasequoiaMeshData();
			data_->SetName(name);
			MetasequoiaMeshData* data = (MetasequoiaMeshData*)data_.GetPointer();
			res = data->CreateFromFileReader(reader);
			if (res) {
				Logger::WriteTop(StringUtility::Format(L"メッシュを読み込みました[%s]", name.c_str()));
				_AddManager(name, data_);
			}
			else {
				data_ = NULL;
			}
		}
		else
			res = true;
	}
	return res;
}
bool MetasequoiaMesh::CreateFromFileInLoadThread(std::wstring path) {
	return DxMesh::CreateFromFileInLoadThread(path, MESH_METASEQUOIA);
}
std::wstring MetasequoiaMesh::GetPath() {
	if (data_ == NULL)return L"";
	return ((MetasequoiaMeshData*)data_.GetPointer())->path_;
}

void MetasequoiaMesh::Render() {
	MetasequoiaMesh::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void MetasequoiaMesh::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	if (data_ == NULL)return;

	MetasequoiaMeshData* data = (MetasequoiaMeshData*)data_.GetPointer();

	while (!data->bLoad_) {
		::Sleep(1);
	}

	{
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();
		ref_count_ptr<DxCamera> camera = graphics->GetCamera();

		DWORD bFogEnable = FALSE;
		if (bCoordinate2D_) {
			device->SetTransform(D3DTS_VIEW, &camera->GetIdentity());
			device->GetRenderState(D3DRS_FOGENABLE, &bFogEnable);
			device->SetRenderState(D3DRS_FOGENABLE, FALSE);
			RenderObject::SetCoordinate2dDeviceMatrix();
		}

		D3DXMATRIX mat = RenderObject::CreateWorldMatrix(position_, scale_,
			angX, angY, angZ, &camera->GetIdentity(), bCoordinate2D_);
		device->SetTransform(D3DTS_WORLD, &mat);

		size_t i = 0;
		for (auto render : data->renderList_) {
			render->SetShader(shader_);
			render->SetColor(color_);
			render->Render();
		}

		if (bCoordinate2D_) {
			device->SetTransform(D3DTS_VIEW, &camera->GetViewProjectionMatrix());
			device->SetRenderState(D3DRS_FOGENABLE, bFogEnable);
		}
	}
}
