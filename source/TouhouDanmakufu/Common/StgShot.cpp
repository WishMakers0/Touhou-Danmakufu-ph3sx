#include "source/GcLib/pch.h"

#include "StgShot.hpp"
#include "StgSystem.hpp"
#include "StgIntersection.hpp"
#include "StgItem.hpp"
#include "../../GcLib/directx/HLSL.hpp"

/**********************************************************
//StgShotManager
**********************************************************/
StgShotManager::StgShotManager(StgStageController* stageController) {
	stageController_ = stageController;

	listPlayerShotData_ = new StgShotDataList();
	listEnemyShotData_ = new StgShotDataList();

	//listObj_.reserve(SHOT_MAX);

	vertexBuffer_ = nullptr;
	_SetVertexBuffer(256 * 256);

	/*
	{
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();
		RECT rcStg = stageController->GetStageInformation()->GetStgFrameRect();
		size_t stgWidth = rcStg.right - rcStg.left;
		size_t stgHeight = rcStg.bottom - rcStg.top;
		size_t width = 2U;
		size_t height = 2U;
		while (width <= stgWidth) {
			width = width << 1;
		}
		while (height <= stgHeight) {
			height = height << 1;
		}
		HRESULT hr = device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
			&renderTexture_, nullptr);
		if (FAILED(hr)) {
			size_t sqSize = std::max(width, height);
			width = sqSize;
			height = sqSize;
			hr = device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
				&renderTexture_, nullptr);
			if (FAILED(hr)) {
				char str[1024];
				sprintf_s(str, "Error %08x: Unable to create the shot layer render target.", hr);
				throw gstd::wexception(std::string(str));
			}
		}
		renderTexture_->GetSurfaceLevel(0, &renderSurface_);
		uint16_t tmpIndex[] = { 0, 1, 2, 2, 1, 3 };
		memcpy_s(const_cast<uint16_t*>(RTARGET_VERT_INDICES), sizeof(tmpIndex), tmpIndex, sizeof(tmpIndex));

		vertRTw = width;
		vertRTh = height;
	}
	*/

	RenderShaderManager* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();
	effectLayer_ = shaderManager_->GetRender2DShader();
	handleEffectWorld_ = effectLayer_->GetParameterBySemantic(nullptr, "WORLD");
}
StgShotManager::~StgShotManager() {
	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); itr++) {
		shared_ptr<StgShotObject> obj = (*itr);
		if (obj != nullptr) {
			obj->ClearShotObject();
		}
	}

	ptr_release(vertexBuffer_);
	ptr_delete(listPlayerShotData_);
	ptr_delete(listEnemyShotData_);
	/*
	if (renderTexture_) {
		renderTexture_->Release();
		renderTexture_ = nullptr;
	}
	if (renderSurface_) {
		renderSurface_->Release();
		renderSurface_ = nullptr;
	}
	*/
}
void StgShotManager::Work() {
	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ) {
		shared_ptr<StgShotObject> obj = (*itr);
		if (obj->IsDeleted()) {
			obj->ClearShotObject();
			itr = listObj_.erase(itr);
		}
		else if (!obj->IsActive()) {
			itr = listObj_.erase(itr);
		}
		else ++itr;
	}
}
void StgShotManager::Render(int targetPriority) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetCullingMode(D3DCULL_NONE);
	graphics->SetLightingEnable(false);
	graphics->SetTextureFilter(DirectGraphics::MODE_TEXTURE_FILTER_LINEAR, 0);

	//graphics->SetTextureFilter(DirectGraphics::MODE_TEXTURE_FILTER_POINT);
	//			MODE_TEXTURE_FILTER_POINT,//補間なし
	//			MODE_TEXTURE_FILTER_LINEAR,//線形補間
	//フォグを解除する
	DWORD bEnableFog = FALSE;
	device->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();

	D3DXMATRIX& matCamera = camera2D->GetMatrix();

	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = (*itr);
		if (obj->IsDeleted())continue;
		if (!obj->IsActive())continue;
		if (obj->GetRenderPriorityI() != targetPriority)continue;
		obj->RenderOnShotManager();
	}

	{
		D3DVIEWPORT9 viewPort;
		device->GetViewport(&viewPort);

		D3DXMATRIX matProj = camera3D->GetIdentity();
		matProj._11 = 2.0f / viewPort.Width;
		matProj._22 = -2.0f / viewPort.Height;
		matProj._41 = -(float)(viewPort.Width + viewPort.X * 2.0f) / viewPort.Width;
		matProj._42 = (float)(viewPort.Height + viewPort.Y * 2.0f) / viewPort.Height;

		matProj = matCamera * matProj;
		effectLayer_->SetMatrix(handleEffectWorld_, &matProj);
	}

	//描画
	int countBlendType = StgShotDataList::RENDER_TYPE_COUNT;
	int blendMode[] =
	{
		DirectGraphics::MODE_BLEND_ADD_ARGB,
		DirectGraphics::MODE_BLEND_ADD_RGB,
		DirectGraphics::MODE_BLEND_SHADOW,
		DirectGraphics::MODE_BLEND_MULTIPLY,
		DirectGraphics::MODE_BLEND_SUBTRACT,
		DirectGraphics::MODE_BLEND_INV_DESTRGB,
		DirectGraphics::MODE_BLEND_ALPHA_INV,
		DirectGraphics::MODE_BLEND_ALPHA
	};

	RenderShaderManager* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();

	device->SetFVF(VERTEX_TLX::fvf);
	device->SetVertexDeclaration(shaderManager_->GetVertexDeclarationTLX());

	{
		//Always render enemy shots above player shots, completely obliterates TAΣ's wet dream.

		for (size_t iBlend = 0; iBlend < countBlendType; iBlend++) {
			graphics->SetBlendMode(blendMode[iBlend]);

			std::vector<StgShotRenderer*>* listPlayer =
				listPlayerShotData_->GetRendererList(blendMode[iBlend] - 1);

			for (size_t iRender = 0U; iRender < listPlayer->size(); iRender++)
				(*listPlayer)[iRender]->Render(this);
		}
		for (size_t iBlend = 0; iBlend < countBlendType; iBlend++) {
			graphics->SetBlendMode(blendMode[iBlend]);

			std::vector<StgShotRenderer*>* listEnemy =
				listEnemyShotData_->GetRendererList(blendMode[iBlend] - 1);

			for (size_t iRender = 0U; iRender < listEnemy->size(); iRender++)
				(*listEnemy)[iRender]->Render(this);
		}
	}

	device->SetVertexDeclaration(nullptr);

	/*
	{
		device->SetRenderTarget(0, tmpSurface);
		graphics->SetTextureFilter(DirectGraphics::MODE_TEXTURE_FILTER_POINT, 0);
		device->SetTexture(0, renderTexture_);
		VERTEX_TLX tmpVert[] = {
			VERTEX_TLX(D3DXVECTOR4(0, 0, 1, 1), (D3DCOLOR)0xffffffff, D3DXVECTOR2(0, 0)),
			VERTEX_TLX(D3DXVECTOR4(vertRTw, 0, 1, 1), (D3DCOLOR)0xffffffff, D3DXVECTOR2(1, 0)),
			VERTEX_TLX(D3DXVECTOR4(0, vertRTh, 1, 1), (D3DCOLOR)0xffffffff, D3DXVECTOR2(0, 1)),
			VERTEX_TLX(D3DXVECTOR4(vertRTw, vertRTh, 1, 1), (D3DCOLOR)0xffffffff, D3DXVECTOR2(1, 1)),
		};
		D3DXMATRIX matCamera = graphics->GetCamera2D()->GetMatrix();
		for (int i = 0; i < 4; ++i) {
			D3DXVECTOR3* vec = (D3DXVECTOR3*)&tmpVert[i].position;
			D3DXVec3TransformCoord(vec, vec, &matCamera);
		}
		device->SetFVF(VERTEX_TLX::fvf);
		device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, RTARGET_VERT_INDICES,
			D3DFMT_INDEX16, tmpVert, sizeof(VERTEX_TLX));
		device->SetTexture(0, nullptr);
	}
	*/

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void StgShotManager::_SetVertexBuffer(size_t size) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	ptr_release(vertexBuffer_);

	vertexBufferSize_ = size;

	device->CreateVertexBuffer(size * sizeof(VERTEX_TLX), D3DUSAGE_DYNAMIC, VERTEX_TLX::fvf, D3DPOOL_DEFAULT,
		&vertexBuffer_, nullptr);
}

void StgShotManager::RegistIntersectionTarget() {
	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = *itr;

		if (!obj->IsDeleted() && obj->IsActive()) {
			obj->ClearIntersectedIdList();
			obj->RegistIntersectionTarget();
		}
	}
	/*
	ParallelAscent(GetShotCountAll(), [&](size_t iObj) {
		ref_count_ptr<StgShotObject>::unsync obj = *(itr + iObj);
		if (!obj->IsDeleted() && obj->IsActive()) {
			obj->ClearIntersectedIdList();
			obj->RegistIntersectionTarget();
		}
	});
	*/
}

RECT StgShotManager::GetShotAutoDeleteClipRect() {
	ref_count_ptr<StgStageInformation> stageInfo = stageController_->GetStageInformation();
	RECT rcStgFrame = stageInfo->GetStgFrameRect();
	RECT rcClip = stageInfo->GetShotAutoDeleteClip();

	int width = rcStgFrame.right - rcStgFrame.left;
	int height = rcStgFrame.bottom - rcStgFrame.top;
	rcClip.right += width;
	rcClip.bottom += height;
	return rcClip;
}

void StgShotManager::DeleteInCircle(int typeDelete, int typeTo, int typeOwner, int cx, int cy, double radius) {
	double rd = radius * radius;

	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = *itr;

		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		if (typeDelete == DEL_TYPE_SHOT && (obj->GetLife() == StgShotObject::LIFE_SPELL_REGIST)) continue;

		double sx = cx - obj->GetPositionX();
		double sy = cy - obj->GetPositionY();

		double tr = sx * sx + sy * sy;

		if (tr <= rd) {
			if (typeTo == TO_TYPE_IMMEDIATE) {
				obj->DeleteImmediate();
			}
			else if (typeTo == TO_TYPE_FADE) {
				obj->SetFadeDelete();
			}
			else if (typeTo == TO_TYPE_ITEM) {
				obj->ConvertToItem(false);
			}
		}
	}
	/*
	ParallelAscent(GetShotCountAll(), [&](size_t iObj) {
		ref_count_ptr<StgShotObject>::unsync obj = *(itr + iObj);
		if (obj->IsDeleted()) return;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) return;
		if (typeDelete == DEL_TYPE_SHOT && (obj->GetLife() == StgShotObject::LIFE_SPELL_REGIST)) return;
		double sx = cx - obj->GetPositionX();
		double sy = cy - obj->GetPositionY();
		double tr = sx * sx + sy * sy;
		if (tr <= rd) {
			if (typeTo == TO_TYPE_IMMEDIATE) {
				obj->DeleteImmediate();
			}
			else if (typeTo == TO_TYPE_FADE) {
				obj->SetFadeDelete();
			}
			else if (typeTo == TO_TYPE_ITEM) {
				obj->ConvertToItem(false);
			}
		}
	});
	*/
}

std::vector<int> StgShotManager::GetShotIdInCircle(int typeOwner, int cx, int cy, int radius) {
	std::vector<int> res;

	double rd = radius * radius;

	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = *itr;

		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;

		double sx = cx - obj->GetPositionX();
		double sy = cy - obj->GetPositionY();

		double tr = sx * sx + sy * sy;

		if (tr <= rd) {
			int id = obj->GetObjectID();

			res.push_back(id);
		}
	}

	return res;
}
size_t StgShotManager::GetShotCount(int typeOwner) {
	size_t res = 0;

	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = *itr;
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		++res;
	}

	return res;
}

void StgShotManager::GetValidRenderPriorityList(std::vector<PriListBool>& list) {
	auto objectManager = stageController_->GetMainObjectManager();
	list.clear();
	list.resize(objectManager->GetRenderBucketCapacity());

	std::list<shared_ptr<StgShotObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = *itr;
		if (obj->IsDeleted()) continue;
		int pri = obj->GetRenderPriorityI();
		list[pri] = true;
	}
}
void StgShotManager::SetDeleteEventEnableByType(int type, bool bEnable) {
	int bit = 0;
	switch (type) {
	case StgStageShotScript::EV_DELETE_SHOT_IMMEDIATE:
		bit = StgShotManager::BIT_EV_DELETE_IMMEDIATE;
		break;
	case StgStageShotScript::EV_DELETE_SHOT_TO_ITEM:
		bit = StgShotManager::BIT_EV_DELETE_TO_ITEM;
		break;
	case StgStageShotScript::EV_DELETE_SHOT_FADE:
		bit = StgShotManager::BIT_EV_DELETE_FADE;
		break;
	}

	if (bEnable) {
		listDeleteEventEnable_.set(bit);
	}
	else {
		listDeleteEventEnable_.reset(bit);
	}
}

/**********************************************************
//StgShotDataList
**********************************************************/
StgShotDataList::StgShotDataList() {
	listRenderer_.resize(RENDER_TYPE_COUNT);
	defaultDelayColor_ = D3DCOLOR_ARGB(255, 128, 128, 128);
}
StgShotDataList::~StgShotDataList() {
	for (std::vector<StgShotRenderer*>& renderList : listRenderer_) {
		for (StgShotRenderer* renderer : renderList)
			ptr_delete(renderer);
		renderList.clear();
	}
	listRenderer_.clear();

	for (StgShotData* shotData : listData_)
		ptr_delete(shotData);
	listData_.clear();
}
bool StgShotDataList::AddShotDataList(std::wstring path, bool bReload) {
	if (!bReload && listReadPath_.find(path) != listReadPath_.end())return true;

	ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr) throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	if (!reader->Open())throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	std::string source = reader->ReadAllString();

	bool res = false;
	Scanner scanner(source);
	try {
		std::vector<StgShotData*> listData;
		std::wstring pathImage = L"";
		RECT rcDelay = { -1, -1, -1, -1 };
		RECT rcDelayDest = { -1, -1, -1, -1 };

		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::TK_EOF)//Eofの識別子が来たらファイルの調査終了
			{
				break;
			}
			else if (tok.GetType() == Token::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"ShotData") {
					_ScanShot(listData, scanner);
				}
				else if (element == L"shot_image") {
					scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
					pathImage = scanner.Next().GetString();
				}
				else if (element == L"delay_color") {
					std::vector<std::wstring> list = _GetArgumentList(scanner);
					defaultDelayColor_ = D3DCOLOR_ARGB(255,
						StringUtility::ToInteger(list[0]),
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]));
				}
				else if (element == L"delay_rect") {
					std::vector<std::wstring> list = _GetArgumentList(scanner);

					RECT rect;
					rect.left = StringUtility::ToInteger(list[0]);
					rect.top = StringUtility::ToInteger(list[1]);
					rect.right = StringUtility::ToInteger(list[2]);
					rect.bottom = StringUtility::ToInteger(list[3]);
					rcDelay = rect;

					int width = rect.right - rect.left;
					int height = rect.bottom - rect.top;
					RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
					if (width % 2 == 1) rcDest.right += 1;
					if (height % 2 == 1) rcDest.bottom += 1;
					rcDelayDest = rcDest;
				}
				if (scanner.HasNext())
					tok = scanner.Next();

			}
		}

		//テクスチャ読み込み
		if (pathImage.size() == 0) throw gstd::wexception("Shot texture must be set.");
		std::wstring dir = PathProperty::GetFileDirectory(path);
		pathImage = StringUtility::Replace(pathImage, L"./", dir);

		ref_count_ptr<Texture> texture = new Texture();
		bool bTexture = texture->CreateFromFile(pathImage, false, false);
		if (!bTexture)throw gstd::wexception("The specified shot texture cannot be found.");

		int textureIndex = -1;
		for (int iTexture = 0; iTexture < listTexture_.size(); iTexture++) {
			ref_count_ptr<Texture> tSearch = listTexture_[iTexture];
			if (tSearch->GetName() == texture->GetName()) {
				textureIndex = iTexture;
				break;
			}
		}
		if (textureIndex < 0) {
			textureIndex = listTexture_.size();
			listTexture_.push_back(texture);
			for (int iRender = 0; iRender < listRenderer_.size(); iRender++) {
				StgShotRenderer* render = new StgShotRenderer();
				render->SetTexture(texture);
				listRenderer_[iRender].push_back(render);
			}
		}

		if (listData_.size() < listData.size())
			listData_.resize(listData.size());
		for (size_t iData = 0; iData < listData.size(); iData++) {
			StgShotData* data = listData[iData];
			if (data == nullptr)continue;
			data->indexTexture_ = textureIndex;
			if (data->rcDelay_.left < 0) {
				data->rcDelay_ = rcDelay;
				data->rcDstDelay_ = rcDelayDest;
			}
			listData_[iData] = data;
		}

		listReadPath_.insert(path);
		Logger::WriteTop(StringUtility::Format(L"Loaded shot data: %s", path.c_str()));
		res = true;
	}
	catch (gstd::wexception & e) {
		std::wstring log = StringUtility::Format(L"Failed to load shot data: [Line=%d] (%s)", scanner.GetCurrentLine(), e.what());
		Logger::WriteTop(log);
		res = false;
	}
	catch (...) {
		std::wstring log = StringUtility::Format(L"Failed to load shot data: [Line=%d] (Unknown error.)", scanner.GetCurrentLine());
		Logger::WriteTop(log);
		res = false;
	}

	return res;
}
void StgShotDataList::_ScanShot(std::vector<StgShotData*>& listData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
	scanner.CheckType(tok, Token::TK_OPENC);

	StgShotData* data = new StgShotData(this);
	data->colorDelay_ = defaultDelayColor_;
	int id = -1;

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::TK_ID) {
			std::wstring element = tok.GetElement();

			if (element == L"id") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				id = scanner.Next().GetInteger();
			}
			else if (element == L"rect") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);

				StgShotData::AnimationData anime;

				RECT rect;
				rect.left = StringUtility::ToInteger(list[0]);
				rect.top = StringUtility::ToInteger(list[1]);
				rect.right = StringUtility::ToInteger(list[2]);
				rect.bottom = StringUtility::ToInteger(list[3]);
				anime.rcSrc_ = rect;

				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
				if (width % 2 == 1) rcDest.right += 1;
				if (height % 2 == 1) rcDest.bottom += 1;
				anime.rcDst_ = rcDest;

				data->listAnime_.resize(1);
				data->listAnime_[0] = anime;
				data->totalAnimeFrame_ = 1;
			}
			else if (element == L"delay_color") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				data->colorDelay_ = D3DCOLOR_ARGB(255,
					StringUtility::ToInteger(list[0]),
					StringUtility::ToInteger(list[1]),
					StringUtility::ToInteger(list[2]));
			}
			else if (element == L"delay_rect") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);

				RECT rect;
				rect.left = StringUtility::ToInteger(list[0]);
				rect.top = StringUtility::ToInteger(list[1]);
				rect.right = StringUtility::ToInteger(list[2]);
				rect.bottom = StringUtility::ToInteger(list[3]);
				data->rcDelay_ = rect;

				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
				if (width % 2 == 1) rcDest.right += 1;
				if (height % 2 == 1) rcDest.bottom += 1;
				data->rcDstDelay_ = rcDest;
			}
			else if (element == L"collision") {
				DxCircle circle;
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				if (list.size() == 1) {
					circle.SetR(StringUtility::ToInteger(list[0]));
				}
				else if (list.size() == 3) {
					circle.SetR(StringUtility::ToInteger(list[0]));
					circle.SetX(StringUtility::ToInteger(list[1]));
					circle.SetY(StringUtility::ToInteger(list[2]));
				}

				data->listCol_ = circle;
			}
			else if (element == L"render" || element == L"delay_render") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				std::wstring strRender = scanner.Next().GetElement();
				int typeRender = DirectGraphics::MODE_BLEND_ALPHA;

				if (strRender == L"ADD" || strRender == L"ADD_RGB")
					typeRender = DirectGraphics::MODE_BLEND_ADD_RGB;
				else if (strRender == L"ADD_ARGB")
					typeRender = DirectGraphics::MODE_BLEND_ADD_ARGB;
				else if (strRender == L"MULTIPLY")
					typeRender = DirectGraphics::MODE_BLEND_MULTIPLY;
				else if (strRender == L"SUBTRACT")
					typeRender = DirectGraphics::MODE_BLEND_SUBTRACT;
				else if (strRender == L"INV_DESTRGB")
					typeRender = DirectGraphics::MODE_BLEND_INV_DESTRGB;

				if (element.size() == 6 /*element == L"render"*/)data->typeRender_ = typeRender;
				else if (element.size() == 12 /*element == L"delay_render"*/)data->typeDelayRender_ = typeRender;
			}
			else if (element == L"alpha") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				data->alpha_ = scanner.Next().GetInteger();
			}
			else if (element == L"angular_velocity") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				tok = scanner.Next();
				if (tok.GetElement() == L"rand") {
					scanner.CheckType(scanner.Next(), Token::TK_OPENP);
					data->angularVelocityMin_ = Math::DegreeToRadian(scanner.Next().GetReal());
					scanner.CheckType(scanner.Next(), Token::TK_COMMA);
					data->angularVelocityMax_ = Math::DegreeToRadian(scanner.Next().GetReal());
					scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
				}
				else {
					data->angularVelocityMin_ = Math::DegreeToRadian(tok.GetReal());
					data->angularVelocityMax_ = data->angularVelocityMin_;
				}
			}
			else if (element == L"fixed_angle") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				tok = scanner.Next();
				data->bFixedAngle_ = tok.GetElement() == L"true";
			}
			else if (element == L"AnimationData") {
				_ScanAnimation(data, scanner);
			}
		}
	}

	if (id >= 0) {
		if (data->listCol_.GetR() <= 0) {
			int r = 0;
			if (data->listAnime_.size() > 0) {
				RECT& rect = data->listAnime_[0].rcSrc_;
				int rx = abs(rect.right - rect.left);
				int ry = abs(rect.bottom - rect.top);
				int r = std::min(rx, ry);
				r = r / 3 - 3;
			}
			DxCircle circle(0, 0, std::max(r, 2));
			data->listCol_ = circle;
		}
		if (listData.size() <= id)
			listData.resize(id + 1);

		listData[id] = data;
	}
}
void StgShotDataList::_ScanAnimation(StgShotData*& shotData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
	scanner.CheckType(tok, Token::TK_OPENC);

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::TK_ID) {
			std::wstring element = tok.GetElement();

			if (element == L"animation_data") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				if (list.size() == 5) {
					StgShotData::AnimationData anime;
					int frame = StringUtility::ToInteger(list[0]);
					RECT rcSrc = {
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]),
						StringUtility::ToInteger(list[3]),
						StringUtility::ToInteger(list[4]),
					};

					anime.frame_ = frame;
					anime.rcSrc_ = rcSrc;

					int width = rcSrc.right - rcSrc.left;
					int height = rcSrc.bottom - rcSrc.top;
					RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
					if (width % 2 == 1) rcDest.right += 1;
					if (height % 2 == 1) rcDest.bottom += 1;
					anime.rcDst_ = rcDest;

					shotData->listAnime_.push_back(anime);
					shotData->totalAnimeFrame_ += frame;
				}
			}
		}
	}
}
std::vector<std::wstring> StgShotDataList::_GetArgumentList(Scanner& scanner) {
	std::vector<std::wstring> res;
	scanner.CheckType(scanner.Next(), Token::TK_EQUAL);

	Token& tok = scanner.Next();

	if (tok.GetType() == Token::TK_OPENP) {
		while (true) {
			tok = scanner.Next();
			int type = tok.GetType();
			if (type == Token::TK_CLOSEP)break;
			else if (type != Token::TK_COMMA) {
				std::wstring str = tok.GetElement();
				res.push_back(str);
			}
		}
	}
	else {
		res.push_back(tok.GetElement());
	}
	return res;
}

//StgShotData
StgShotData::StgShotData(StgShotDataList* listShotData) {
	listShotData_ = listShotData;
	typeRender_ = DirectGraphics::MODE_BLEND_ALPHA;
	typeDelayRender_ = DirectGraphics::MODE_BLEND_ADD_ARGB;
	colorDelay_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	SetRect(&rcDelay_, -1, -1, -1, -1);
	alpha_ = 255;
	totalAnimeFrame_ = 0;
	angularVelocityMin_ = 0;
	angularVelocityMax_ = 0;
	bFixedAngle_ = false;
}
StgShotData::~StgShotData() {}

StgShotData::AnimationData* StgShotData::GetData(int frame) {
	if (totalAnimeFrame_ == 1)
		return &listAnime_[0];

	frame = frame % totalAnimeFrame_;
	int total = 0;

	std::vector<AnimationData>::iterator itr = listAnime_.begin();
	for (; itr != listAnime_.end(); itr++) {
		//AnimationData* anime = itr;
		total += itr->frame_;
		if (total >= frame)
			return &(*itr);
	}
	return &listAnime_[0];
}

/**********************************************************
//StgShotRenderer
**********************************************************/
StgShotRenderer::StgShotRenderer() {
	countRenderVertex_ = 0;
	countMaxVertex_ = 256 * 256;
	SetVertexCount(countMaxVertex_);
}
StgShotRenderer::~StgShotRenderer() {

}
void StgShotRenderer::Render(StgShotManager* manager) {
	if (countRenderVertex_ < 3)
		return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	gstd::ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else
		device->SetTexture(0, nullptr);

	//device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (int)(countRenderVertex_ / 3), vertex_.GetPointer(), sizeof(VERTEX_TLX));

	while (countMaxVertex_ > manager->GetVertexBufferSize()) {
		manager->_SetVertexBuffer(manager->GetVertexBufferSize() * 2);
	}
	IDirect3DVertexBuffer9* vBuffer = manager->GetVertexBuffer();

	vBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
	memcpy(tmp, vertex_.data(), countRenderVertex_ * sizeof(VERTEX_TLX));
	vBuffer->Unlock();

	device->SetStreamSource(0, vBuffer, 0, sizeof(VERTEX_TLX));
	{
		ID3DXEffect*& effect = manager->effectLayer_;
		effect->SetTechnique("Render");

		UINT cPass = 1;
		effect->Begin(&cPass, 0);
		for (UINT iPass = 0; iPass < cPass; ++iPass) {
			effect->BeginPass(iPass);
			device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, countRenderVertex_ / 3U);
			effect->EndPass();
		}
		effect->End();
	}

	countRenderVertex_ = 0;
}
void StgShotRenderer::AddVertex(VERTEX_TLX& vertex) {
	if (countRenderVertex_ == countMaxVertex_ - 1) {
		countMaxVertex_ *= 2;
		SetVertexCount(countMaxVertex_);
	}

	SetVertex(countRenderVertex_, vertex);
	countRenderVertex_++;
}

/**********************************************************
//StgShotObject
**********************************************************/
StgShotObject::StgShotObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;

	frameWork_ = 0;
	posX_ = 0;
	posY_ = 0;
	idShotData_ = 0;
	typeBlend_ = DirectGraphics::MODE_BLEND_NONE;
	typeSourceBrend_ = DirectGraphics::MODE_BLEND_NONE;

	damage_ = 1;
	life_ = LIFE_SPELL_UNREGIST;
	bAutoDelete_ = true;
	bEraseShot_ = false;
	bSpellFactor_ = false;

	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	delay_ = 0;
	frameGrazeInvalid_ = 0;
	frameFadeDelete_ = -1;
	frameAutoDelete_ = INT_MAX;

	typeOwner_ = OWNER_ENEMY;
	bUserIntersectionMode_ = false;
	bIntersectionEnable_ = true;
	bChangeItemEnable_ = true;

	hitboxScaleX_ = 1.0f;
	int priShotI = stageController_->GetStageInformation()->GetShotObjectPriority();
	hitboxScaleY_ = 1.0f;

	double priShotD = (double)priShotI / (stageController_->GetMainObjectManager()->GetRenderBucketCapacity() - 1);
	SetRenderPriority(priShotD);
}
StgShotObject::~StgShotObject() {
	if (listReserveShot_ != nullptr) {
		listReserveShot_->Clear(stageController_);
	}
}
shared_ptr<StgShotObject> StgShotObject::GetOwnObject() {
	return std::dynamic_pointer_cast<StgShotObject>(stageController_->GetMainRenderObject(idObject_));
}
void StgShotObject::Work() {}
void StgShotObject::_Move() {
	if (delay_ == 0)
		StgMoveObject::_Move();
	SetX(posX_);
	SetY(posY_);

	//弾画像置き換え処理
	if (pattern_ != nullptr) {
		int idShot = pattern_->GetShotDataID();
		if (idShot != StgMovePattern::NO_CHANGE) {
			SetShotDataID(idShot);
		}
	}
}
void StgShotObject::_DeleteInLife() {
	if (IsDeleted())return;
	if (life_ <= 0) {
		_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_IMMEDIATE);

		auto objectManager = stageController_->GetMainObjectManager();

		if (typeOwner_ == StgShotObject::OWNER_PLAYER) {
			auto scriptManager = stageController_->GetScriptManager();
			ref_count_ptr<ManagedScript> scriptPlayer = scriptManager->GetPlayerScript();

			double posX = GetPositionX();
			double posY = GetPositionY();
			if (scriptManager != nullptr && scriptPlayer != nullptr) {
				std::vector<double> listPos;
				listPos.push_back(posX);
				listPos.push_back(posY);

				std::vector<value> listScriptValue;
				listScriptValue.push_back(scriptPlayer->CreateRealValue(idObject_));
				listScriptValue.push_back(scriptPlayer->CreateRealArrayValue(listPos));
				listScriptValue.push_back(scriptPlayer->CreateRealValue(GetShotDataID()));
				scriptPlayer->RequestEvent(StgStagePlayerScript::EV_DELETE_SHOT_PLAYER, listScriptValue);
			}
		}

		objectManager->DeleteObject(this);
	}
}
void StgShotObject::_DeleteInAutoClip() {
	if (IsDeleted())return;
	if (!IsAutoDelete())return;
	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();
	if (posX_ < rect.left || posX_ > rect.right || posY_ < rect.top || posY_ > rect.bottom) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgShotObject::_DeleteInFadeDelete() {
	if (IsDeleted())return;
	if (frameFadeDelete_ == 0) {
		_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_FADE);
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgShotObject::_DeleteInAutoDeleteFrame() {
	if (IsDeleted())return;
	if (delay_ > 0)return;

	if (frameAutoDelete_ <= 0) {
		_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_IMMEDIATE);
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
		return;
	}
	frameAutoDelete_ = std::max(0, frameAutoDelete_ - 1);
}
void StgShotObject::_SendDeleteEvent(int bit) {
	if (typeOwner_ != OWNER_ENEMY)return;

	auto stageScriptManager = stageController_->GetScriptManager();
	ref_count_ptr<ManagedScript> scriptShot = stageScriptManager->GetShotScript();
	if (scriptShot == nullptr)return;

	StgShotManager* shotManager = stageController_->GetShotManager();
	bool bSendEnable = shotManager->IsDeleteEventEnable(bit);
	if (!bSendEnable)return;

	double posX = GetPositionX();
	double posY = GetPositionY();

	std::vector<double> listPos;
	listPos.push_back(posX);
	listPos.push_back(posY);

	int typeEvent = 0;
	switch (bit) {
	case StgShotManager::BIT_EV_DELETE_IMMEDIATE:
		typeEvent = StgStageShotScript::EV_DELETE_SHOT_IMMEDIATE;
		break;
	case StgShotManager::BIT_EV_DELETE_TO_ITEM:
		typeEvent = StgStageShotScript::EV_DELETE_SHOT_TO_ITEM;
		break;
	case StgShotManager::BIT_EV_DELETE_FADE:
		typeEvent = StgStageShotScript::EV_DELETE_SHOT_FADE;
		break;
	}

	std::vector<gstd::value> listScriptValue;
	listScriptValue.push_back(scriptShot->CreateRealValue(idObject_));
	listScriptValue.push_back(scriptShot->CreateRealArrayValue(listPos));
	listScriptValue.push_back(scriptShot->CreateBooleanValue(false));
	listScriptValue.push_back(scriptShot->CreateRealValue(GetShotDataID()));
	scriptShot->RequestEvent(typeEvent, listScriptValue);
}
void StgShotObject::_AddReservedShotWork() {
	if (IsDeleted())return;
	if (listReserveShot_ == nullptr)return;
	ref_count_ptr<ReserveShotList::ListElement>::unsync listData = listReserveShot_->GetNextFrameData();
	if (listData == nullptr)return;

	auto objectManager = stageController_->GetMainObjectManager();
	std::list<ReserveShotListData>* list = listData->GetDataList();
	std::list<ReserveShotListData>::iterator itr = list->begin();
	for (; itr != list->end(); itr++) {
		StgShotObject::ReserveShotListData& data = (*itr);
		int idShot = data.GetShotID();
		shared_ptr<StgShotObject> obj = std::dynamic_pointer_cast<StgShotObject>(objectManager->GetObject(idShot));
		if (obj == nullptr || obj->IsDeleted())continue;

		_AddReservedShot(obj, &data);
	}

}

void StgShotObject::_AddReservedShot(shared_ptr<StgShotObject> obj, StgShotObject::ReserveShotListData* data) {
	auto objectManager = stageController_->GetMainObjectManager();

	double ownAngle = GetDirectionAngle();
	double ox = GetPositionX();
	double oy = GetPositionY();

	double dRadius = data->GetRadius();
	double dAngle = data->GetAngle();
	double sx = obj->GetPositionX();
	double sy = obj->GetPositionY();
	double sAngle = obj->GetDirectionAngle();
	double angle = ownAngle + dAngle;

	double tx = ox + sx + dRadius * cos(angle);
	double ty = oy + sy + dRadius * sin(angle);
	obj->SetX(tx);
	obj->SetY(ty);

	StgShotManager* shotManager = stageController_->GetShotManager();
	if (shotManager->GetShotCountAll() < StgShotManager::SHOT_MAX) {
		shotManager->AddShot(obj);
		obj->Activate();
		objectManager->ActivateObject(obj->GetObjectID(), true);
	}
	else
		objectManager->DeleteObject(obj);
}

void StgShotObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {

}
StgShotData* StgShotObject::_GetShotData() {
	StgShotData* res = nullptr;
	StgShotManager* shotManager = stageController_->GetShotManager();
	StgShotDataList* dataList = (typeOwner_ == OWNER_PLAYER) ?
		shotManager->GetPlayerShotDataList() : shotManager->GetEnemyShotDataList();

	if (dataList != nullptr) {
		res = dataList->GetData(idShotData_);
	}

	return res;
}

void StgShotObject::AddShot(int frame, int idShot, int radius, int angle) {
	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->ActivateObject(idShot, false);

	if (listReserveShot_ == nullptr)
		listReserveShot_ = new ReserveShotList();
	listReserveShot_->AddData(frame, idShot, radius, angle);
}
void StgShotObject::ConvertToItem(bool flgPlayerCollision) {
	if (IsDeleted())return;

	if (bChangeItemEnable_) {
		_ConvertToItemAndSendEvent(flgPlayerCollision);
		_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_TO_ITEM);
	}

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}
void StgShotObject::DeleteImmediate() {
	if (IsDeleted())return;

	_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_IMMEDIATE);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgShotObject::ReserveShotList
ref_count_ptr<StgShotObject::ReserveShotList::ListElement>::unsync StgShotObject::ReserveShotList::GetNextFrameData() {
	ref_count_ptr<ListElement>::unsync res = nullptr;

	auto itr = mapData_.find(frame_);
	if (itr != mapData_.end()) {
		res = itr->second;
		mapData_.erase(itr);
	}

	++frame_;
	return res;
}
void StgShotObject::ReserveShotList::AddData(int frame, int idShot, int radius, int angle) {
	ref_count_ptr<ListElement>::unsync list;

	auto itr = mapData_.find(frame);
	if (itr == mapData_.end()) {
		list = new ListElement();
		mapData_[frame] = list;
	}
	else {
		list = itr->second;
	}

	ReserveShotListData data;
	data.idShot_ = idShot;
	data.radius_ = radius;
	data.angle_ = angle;
	list->Add(data);
}
void StgShotObject::ReserveShotList::Clear(StgStageController* stageController) {
	auto objectManager = stageController->GetMainObjectManager();
	if (objectManager == nullptr)return;

	auto itrMap = mapData_.begin();
	for (; itrMap != mapData_.end(); ++itrMap) {
		ref_count_ptr<ListElement>::unsync listElement = itrMap->second;
		std::list<ReserveShotListData>* list = listElement->GetDataList();
		std::list<ReserveShotListData>::iterator itr = list->begin();
		for (; itr != list->end(); ++itr) {
			StgShotObject::ReserveShotListData& data = (*itr);
			int idShot = data.GetShotID();
			shared_ptr<StgShotObject> objShot = std::dynamic_pointer_cast<StgShotObject>(objectManager->GetObject(idShot));
			if (objShot != nullptr)objShot->ClearShotObject();
			objectManager->DeleteObject(objShot);
		}
	}
}

/**********************************************************
//StgNormalShotObject
**********************************************************/
StgNormalShotObject::StgNormalShotObject(StgStageController* stageController) : StgShotObject(stageController) {
	typeObject_ = TypeObject::OBJ_SHOT;
	angularVelocity_ = 0;

	c_ = 1;
	s_ = 0;
	lastAngle_ = 0;
}
StgNormalShotObject::~StgNormalShotObject() {

}
void StgNormalShotObject::Work() {
	_Move();
	if (delay_ == 0) {
		_AddReservedShotWork();
	}

	delay_ = std::max(delay_ - 1, 0);
	frameWork_++;

	angle_.z += angularVelocity_;

	if (frameFadeDelete_ >= 0) {
		--frameFadeDelete_;
	}

	{
		StgShotData* shotData = _GetShotData();

		double angleZ = angle_.z;
		if (shotData) {
			if (!shotData->IsFixedAngle()) angleZ += GetDirectionAngle() + Math::DegreeToRadian(90);
		}

		if (angleZ != lastAngle_) {
			c_ = cos(angleZ);
			s_ = sin(angleZ);
			lastAngle_ = angleZ;
		}
	}

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
}

void StgNormalShotObject::_AddIntersectionRelativeTarget() {
	if (delay_ > 0)return;
	if (frameFadeDelete_ >= 0)return;
	ClearIntersected();
	if (IsDeleted())return;
	if (bUserIntersectionMode_)return;//ユーザ定義あたり判定モード
	if (!bIntersectionEnable_)return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	DxCircle* listCircle = shotData->GetIntersectionCircleList();
	if (GetIntersectionRelativeTargetCount() != 1) {
		ClearIntersectionRelativeTarget();

		shared_ptr<StgShotObject> obj = GetOwnObject();
		if (obj == nullptr)return;
		weak_ptr<StgShotObject> wObj = obj;

		StgIntersectionTarget_Circle::ptr target =
			std::shared_ptr<StgIntersectionTarget_Circle>(new StgIntersectionTarget_Circle);
		if (target != nullptr) {
			if (typeOwner_ == OWNER_PLAYER)
				target->SetTargetType(StgIntersectionTarget::TYPE_PLAYER_SHOT);
			else
				target->SetTargetType(StgIntersectionTarget::TYPE_ENEMY_SHOT);
			target->SetObject(wObj);
			AddIntersectionRelativeTarget(target);
		}
	}

	bool bInvalid = true;
	{
		StgIntersectionTarget::ptr target = GetIntersectionRelativeTarget(0);
		StgIntersectionTarget_Circle::ptr cTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);

		if (cTarget != nullptr) {
			DxCircle circle = *listCircle;
			if (circle.GetX() != 0 || circle.GetY() != 0) {
				double px = circle.GetX() * c_ - (-circle.GetY()) * s_;
				double py = circle.GetX() * s_ + (-circle.GetY()) * c_;
				circle.SetX(px + posX_);
				circle.SetY(py + posY_);
			}
			else {
				circle.SetX(circle.GetX() + posX_);
				circle.SetY(circle.GetY() + posY_);
			}
			circle.SetR(circle.GetR() * ((hitboxScaleX_ + hitboxScaleY_) / 2));

			cTarget->SetCircle(circle);

			RECT rect = cTarget->GetIntersectionSpaceRect();
			if (rect.left != rect.right && rect.top != rect.bottom)
				bInvalid = false;
		}
	}

	if (typeOwner_ == OWNER_PLAYER) {
		//自弾の場合は登録
		bInvalid = false;
	}
	else {
		//自機の移動範囲が負の値が可能であれば敵弾でも登録
		shared_ptr<StgPlayerObject> player = stageController_->GetPlayerObject();
		if (player != nullptr) {
			RECT rcClip = player->GetClip();
			if (rcClip.left < 0 || rcClip.top < 0)
				bInvalid = false;
		}
	}

	if (!bInvalid)
		RegistIntersectionRelativeTarget(intersectionManager);
}
std::vector<StgIntersectionTarget::ptr> StgNormalShotObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;

	if (delay_ > 0)return res;
	if (frameFadeDelete_ >= 0)return res;
	if (IsDeleted())return res;
	if (bUserIntersectionMode_)return res;//ユーザ定義あたり判定モード
	if (!bIntersectionEnable_)return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return res;

	DxCircle* listCircle = shotData->GetIntersectionCircleList();
	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	{
		StgIntersectionTarget::ptr target =
			std::shared_ptr<StgIntersectionTarget_Circle>(new StgIntersectionTarget_Circle);
		StgIntersectionTarget_Circle::ptr cTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);

		if (cTarget != nullptr) {
			DxCircle circle = *listCircle;
			if (circle.GetX() != 0 || circle.GetY() != 0) {
				double px = circle.GetX() * c_ - (-circle.GetY()) * s_;
				double py = circle.GetX() * s_ + (-circle.GetY()) * c_;
				circle.SetX(px + posX_);
				circle.SetY(py + posY_);
			}
			else {
				circle.SetX(circle.GetX() + posX_);
				circle.SetY(circle.GetY() + posY_);
			}
			circle.SetR(circle.GetR() * ((hitboxScaleX_ + hitboxScaleY_) / 2));

			cTarget->SetCircle(circle);

			res.push_back(target);
		}
	}

	return res;
}

void StgNormalShotObject::RenderOnShotManager() {
	if (!IsVisible())return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return;

	StgShotRenderer* renderer = nullptr;

	int shotBlendType = DirectGraphics::MODE_BLEND_ALPHA;
	if (delay_ > 0) {
		//遅延時間
		int objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == DirectGraphics::MODE_BLEND_NONE) {
			shotBlendType = shotData->GetDelayRenderType();
			renderer = shotData->GetRenderer(shotBlendType);
		}
		else {
			renderer = shotData->GetRenderer(objDelayBlendType);
		}
	}
	else {
		int objBlendType = GetBlendType();
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer();
			shotBlendType = shotData->GetRenderType();
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
	}

	if (renderer == nullptr)return;

	double scaleX = 1.0;
	double scaleY = 1.0;

	RECT* rcSrc;
	RECT* rcDest;
	D3DCOLOR color;

	if (delay_ > 0) {
		double expa = 0.5f + (double)delay_ / 30.0f * 2;
		if (expa > 2)expa = 2;

		scaleX = expa;
		scaleY = expa;

		rcSrc = shotData->GetDelayRect();
		rcDest = shotData->GetDelayDest();

		color = shotData->GetDelayColor();
	}
	else {
		scaleX = scale_.x;
		scaleY = scale_.y;

		StgShotData::AnimationData* anime = shotData->GetData(frameWork_);
		rcSrc = anime->GetSource();
		rcDest = anime->GetDest();

		color = color_;
		double alpha = shotData->GetAlpha() / 255.0;
		if (frameFadeDelete_ >= 0)
			alpha = (double)frameFadeDelete_ / (double)FRAME_FADEDELETE;

		bool bValidAlpha = StgShotData::IsAlphaBlendValidType(shotBlendType);
		if (bValidAlpha) {
			int colorA = ColorAccess::GetColorA(color);
			color = ColorAccess::SetColorA(color, alpha * colorA);
		}
		else {
			color = ColorAccess::ApplyAlpha(color, alpha);
		}
	}

	//if(bIntersected_)color = D3DCOLOR_ARGB(255, 255, 0, 0);//接触テスト

	VERTEX_TLX verts[4];
	/*
	int srcX[] = { rcSrc->left, rcSrc->right, rcSrc->left, rcSrc->right };
	int srcY[] = { rcSrc->top, rcSrc->top, rcSrc->bottom, rcSrc->bottom };
	int destX[] = { rcDest->left, rcDest->right, rcDest->left, rcDest->right };
	int destY[] = { rcDest->top, rcDest->top, rcDest->bottom, rcDest->bottom };
	*/
	LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
	LONG* ptrDst = reinterpret_cast<LONG*>(rcDest);

//#pragma omp parallel for
	for (size_t iVert = 0; iVert < 4; iVert++) {
		VERTEX_TLX vt;

		_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1], ptrSrc[iVert | 0b1]);
		_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
		_SetVertexColorARGB(vt, color);

		double px = vt.position.x * scaleX;
		double py = vt.position.y * scaleY;
		vt.position.x = (px * c_ - py * s_) + position_.x;
		vt.position.y = (px * s_ + py * c_) + position_.y;
		vt.position.z = position_.z;

		//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
		verts[iVert] = vt;
	}

	renderer->AddSquareVertex(verts);
}

void StgNormalShotObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	shared_ptr<StgIntersectionObject> ptrObj = otherTarget->GetObject().lock();

	double damage = 0;
	int otherType = otherTarget->GetTargetType();
	switch (otherType) {
	case StgIntersectionTarget::TYPE_PLAYER:
	{
		//自機
		frameGrazeInvalid_ = INT_MAX;
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SHOT:
	{
		if (ptrObj) {
			StgShotObject* shot = (StgShotObject*)ptrObj.get();
			if (shot != nullptr) {
				bool bEraseShot = shot->IsEraseShot();
				if (bEraseShot && life_ != LIFE_SPELL_REGIST)
					ConvertToItem(false);
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SPELL:
	{
		//自機スペル
		if (ptrObj) {
			StgPlayerSpellObject* spell = (StgPlayerSpellObject*)ptrObj.get();
			if (spell != nullptr) {
				bool bEraseShot = spell->IsEraseShot();
				if (bEraseShot && life_ != LIFE_SPELL_REGIST)
					ConvertToItem(false);
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_ENEMY:
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		damage = 1;
		break;
	}
	}

	if (life_ != LIFE_SPELL_REGIST)
		life_ = std::max(life_ - damage, 0.0);
}
void StgNormalShotObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();
	ref_count_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	double posX = GetPositionX();
	double posY = GetPositionY();
	if (scriptItem != nullptr) {
		std::vector<double> listPos;
		listPos.push_back(posX);
		listPos.push_back(posY);

		std::vector<gstd::value> listScriptValue;
		listScriptValue.push_back(scriptItem->CreateRealValue(idObject_));
		listScriptValue.push_back(scriptItem->CreateRealArrayValue(listPos));
		listScriptValue.push_back(scriptItem->CreateBooleanValue(flgPlayerCollision));
		listScriptValue.push_back(scriptItem->CreateRealValue(GetShotDataID()));
		scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue);
	}

	if (itemManager->IsDefaultBonusItemEnable() && !flgPlayerCollision) {
		if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
			shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
			auto objectManager = stageController_->GetMainObjectManager();
			int id = objectManager->AddObject(obj);
			if (id != DxScript::ID_INVALID) {
				//弾の座標にアイテムを作成する
				itemManager->AddItem(obj);
				obj->SetPositionX(posX);
				obj->SetPositionY(posY);
			}
		}
	}
}
void StgNormalShotObject::SetShotDataID(int id) {
	StgShotData* oldData = _GetShotData();
	StgShotObject::SetShotDataID(id);

	//角速度更新
	StgShotData* shotData = _GetShotData();
	if (shotData != nullptr && oldData != shotData) {
		if (angularVelocity_ != 0) {
			angularVelocity_ = 0;
			angle_.z = 0;
		}

		double avMin = shotData->GetAngularVelocityMin();
		double avMax = shotData->GetAngularVelocityMax();
		if (avMin != 0 || avMax != 0) {
			ref_count_ptr<StgStageInformation> stageInfo = stageController_->GetStageInformation();
			RandProvider* rand = stageInfo->GetRandProvider();
			angularVelocity_ = rand->GetReal(avMin, avMax);
		}

	}

}

/**********************************************************
//StgLaserObject(レーザー基本部)
**********************************************************/
StgLaserObject::StgLaserObject(StgStageController* stageController) : StgShotObject(stageController) {
	life_ = LIFE_SPELL_REGIST;
	length_ = 0;
	widthRender_ = 0;
	widthIntersection_ = 0;
	invalidLengthStart_ = 0.1f;
	invalidLengthEnd_ = 0.1f;
	frameGrazeInvalidStart_ = 20;
	itemDistance_ = 24;

	c_ = 1;
	s_ = 0;
	lastAngle_ = 0;
}
void StgLaserObject::_AddIntersectionRelativeTarget() {
	if (delay_ > 0)return;
	if (frameFadeDelete_ >= 0)return;
	ClearIntersected();

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	std::vector<StgIntersectionTarget::ptr> listTarget = GetIntersectionTargetList();
	for (int iTarget = 0; iTarget < listTarget.size(); iTarget++)
		intersectionManager->AddTarget(listTarget[iTarget]);
}
void StgLaserObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	shared_ptr<StgIntersectionObject> ptrObj = otherTarget->GetObject().lock();

	double damage = 0;
	int otherType = otherTarget->GetTargetType();
	switch (otherType) {
	case StgIntersectionTarget::TYPE_PLAYER:
	{
		//自機
		if (frameGrazeInvalid_ <= 0) {
			frameGrazeInvalid_ = frameGrazeInvalidStart_ > 0 ? frameGrazeInvalidStart_ : INT_MAX;
		}
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SHOT:
	{
		//自機弾弾
		if (ptrObj) {
			StgShotObject* shot = (StgShotObject*)ptrObj.get();
			if (shot != nullptr) {
				bool bEraseShot = shot->IsEraseShot();
				if (bEraseShot && life_ != LIFE_SPELL_REGIST) {
					damage = shot->GetDamage();
					ConvertToItem(false);
				}
			}
		}

		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SPELL:
	{
		//自機スペル
		StgPlayerSpellObject* spell = (StgPlayerSpellObject*)ptrObj.get();
		if (spell != nullptr) {
			bool bEraseShot = spell->IsEraseShot();
			if (bEraseShot && life_ != LIFE_SPELL_REGIST) {
				damage = spell->GetDamage();
				ConvertToItem(false);
			}
		}
		break;
	}
	}
	if (life_ != LIFE_SPELL_REGIST) {
		life_ -= damage;
		life_ = std::max(life_, 0.0);
	}
}


/**********************************************************
//StgLooseLaserObject(射出型レーザー)
**********************************************************/
StgLooseLaserObject::StgLooseLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::OBJ_LOOSE_LASER;
}
void StgLooseLaserObject::Work() {
	//1フレーム目は移動しない
	if (frameWork_ == 0) {
		posXE_ = posX_;
		posYE_ = posY_;
	}

	_Move();
	if (delay_ == 0) {
		_AddReservedShotWork();
	}

	--delay_;
	delay_ = std::max(delay_, 0);
	frameWork_++;

	if (frameFadeDelete_ >= 0) {
		--frameFadeDelete_;
	}

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
	//	_AddIntersectionRelativeTarget();
	--frameGrazeInvalid_;
}
void StgLooseLaserObject::_Move() {
	if (delay_ == 0)
		StgMoveObject::_Move();
	DxScriptRenderObject::SetX(posX_);
	DxScriptRenderObject::SetY(posY_);

	if (delay_ > 0)return;

	double dx = posXE_ - posX_;
	double dy = posYE_ - posY_;

	if ((dx * dx + dy * dy) > (length_ * length_)) {
		double speed = GetSpeed();
		posXE_ += speed * c_;
		posYE_ += speed * s_;
	}

	if (lastAngle_ != GetDirectionAngle()) {
		lastAngle_ = GetDirectionAngle();
		c_ = cos(lastAngle_);
		s_ = sin(lastAngle_);
	}
}
void StgLooseLaserObject::_DeleteInAutoClip() {
	if (IsDeleted())return;
	if (!IsAutoDelete())return;
	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();
	if ((posX_ < rect.left && posXE_ < rect.left) || (posX_ > rect.right&& posXE_ > rect.right) ||
		(posY_ < rect.top && posYE_ < rect.top) || (posY_ > rect.bottom&& posYE_ > rect.bottom)) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}

std::vector<StgIntersectionTarget::ptr> StgLooseLaserObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;

	if (delay_ > 0)return res;
	if (frameFadeDelete_ >= 0)return res;
	if (IsDeleted())return res;
	if (bUserIntersectionMode_)return res;//ユーザ定義あたり判定モード
	if (!bIntersectionEnable_)return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return res;

	shared_ptr<StgShotObject> obj = GetOwnObject();
	if (obj == nullptr)return res;

	float dx = posXE_ - posX_;
	float dy = posYE_ - posY_;

	int posXS = posX_ + dx * invalidLengthStart_;
	int posYS = posY_ + dy * invalidLengthStart_;
	int posXE = posXE_ - dx * invalidLengthEnd_;
	int posYE = posYE_ - dy * invalidLengthEnd_;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	DxWidthLine line(posXS, posYS, posXE, posYE, widthIntersection_);

	weak_ptr<StgShotObject> wObj = obj;
	StgIntersectionTarget_Line::ptr target =
		std::shared_ptr<StgIntersectionTarget_Line>(new StgIntersectionTarget_Line);
	if (target != nullptr) {
		if (typeOwner_ == OWNER_PLAYER)
			target->SetTargetType(StgIntersectionTarget::TYPE_PLAYER_SHOT);
		else
			target->SetTargetType(StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(wObj);
		target->SetLine(line);

		res.push_back(target);
	}
	return res;
}

void StgLooseLaserObject::RenderOnShotManager() {
	if (!IsVisible())return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return;

	int shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
	StgShotRenderer* renderer = nullptr;
	if (delay_ > 0) {
		//遅延時間
		int objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
		}
		else {
			renderer = shotData->GetRenderer(objDelayBlendType);
		}
	}
	else {
		int objBlendType = GetBlendType();
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
			shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
	}

	if (renderer == nullptr)return;

	double scaleX = 1.0;
	double scaleY = 1.0;
	double renderC = 1.0;
	double renderS = 0.0;

	RECT* rcSrc;
	RECT rcDest;

	D3DCOLOR color;

	if (delay_ > 0) {
		double expa = 0.5f + (double)delay_ / 30.0f * 2;
		if (expa > 3.5)expa = 3.5;

		scaleX = expa;
		scaleY = expa;

		renderC = c_;
		renderS = s_;

		rcSrc = shotData->GetDelayRect();
		rcDest = *shotData->GetDelayDest();

		color = shotData->GetDelayColor();
	}
	else {
		scaleX = scale_.x;
		scaleY = scale_.y;

		double dx = posXE_ - posX_;
		double dy = posYE_ - posY_;
		double radius = sqrt(dx * dx + dy * dy);

		renderC = dx / radius;
		renderS = dy / radius;

		StgShotData::AnimationData* anime = shotData->GetData(frameWork_);
		rcSrc = anime->GetSource();

		color = color_;
		double alpha = shotData->GetAlpha() / 255.0;
		if (frameFadeDelete_ >= 0)
			alpha = (double)frameFadeDelete_ / (double)FRAME_FADEDELETE;

		bool bValidAlpha = StgShotData::IsAlphaBlendValidType(shotBlendType);
		if (bValidAlpha) {
			//α有効
			int colorA = ColorAccess::GetColorA(color);
			color = ColorAccess::SetColorA(color, alpha * colorA);
		}
		else {
			//α無効
			color = ColorAccess::ApplyAlpha(color, alpha);
		}

		//color = ColorAccess::ApplyAlpha(color, alpha);
		SetRect(&rcDest, -widthRender_ / 2, 0, widthRender_ / 2, radius);
	}


	VERTEX_TLX verts[4];
	/*
	int srcX[] = { rcSrc->left, rcSrc->right, rcSrc->left, rcSrc->right };
	int srcY[] = { rcSrc->top, rcSrc->top, rcSrc->bottom, rcSrc->bottom };
	int destY[] = { rcDest->left, rcDest->right, rcDest->left, rcDest->right };
	int destX[] = { rcDest->top, rcDest->top, rcDest->bottom, rcDest->bottom };
	*/
	LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
	LONG* ptrDst = reinterpret_cast<LONG*>(&rcDest);

	for (int iVert = 0; iVert < 4; iVert++) {
		VERTEX_TLX vt;

		_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1], ptrSrc[iVert | 0b1]);
		_SetVertexPosition(vt, ptrDst[iVert | 0b1], ptrDst[(iVert & 0b1) << 1]);
		_SetVertexColorARGB(vt, color);

		double px = vt.position.x * scaleX;
		double py = vt.position.y * scaleY;
		vt.position.x = (px * renderC - py * renderS) + position_.x;
		vt.position.y = (px * renderS + py * renderC) + position_.y;
		vt.position.z = position_.z;

		//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
		verts[iVert] = vt;
	}

	renderer->AddSquareVertex(verts);
}
void StgLooseLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();
	ref_count_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	int ex = GetPositionX();
	int ey = GetPositionY();

	float dx = posXE_ - posX_;
	float dy = posYE_ - posY_;
	double length = (double)sqrt(dx * dx + dy * dy);

	std::vector<double> listPos;
	listPos.resize(2);
	std::vector<gstd::value> listScriptValue;
	listScriptValue.resize(4);

	for (double itemPos = 0; itemPos < length; itemPos += itemDistance_) {
		double posX = ex - itemPos * c_;
		double posY = ey - itemPos * s_;

		if (scriptItem != nullptr) {
			listPos[0] = posX;
			listPos[1] = posY;

			listScriptValue[0] = scriptItem->CreateRealValue(idObject_);
			listScriptValue[1] = scriptItem->CreateRealArrayValue(listPos);
			listScriptValue[2] = scriptItem->CreateBooleanValue(flgPlayerCollision);
			listScriptValue[3] = scriptItem->CreateRealValue(GetShotDataID());
			scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue);
		}

		if (itemManager->IsDefaultBonusItemEnable() && delay_ == 0 && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
				int id = stageController_->GetMainObjectManager()->AddObject(obj);
				if (id != DxScript::ID_INVALID) {
					//弾の座標にアイテムを作成する
					itemManager->AddItem(obj);
					obj->SetPositionX(posX);
					obj->SetPositionY(posY);
				}
			}
		}
	}
}

/**********************************************************
//StgStraightLaserObject(設置型レーザー)
**********************************************************/
StgStraightLaserObject::StgStraightLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::OBJ_STRAIGHT_LASER;
	angLaser_ = 0;
	frameFadeDelete_ = -1;
	bUseSouce_ = true;
	scaleX_ = 0.05;

	bLaserExpand_ = true;

	c_ = 1;
	s_ = 0;
}
void StgStraightLaserObject::Work() {
	_Move();
	if (delay_ == 0) {
		_AddReservedShotWork();
	}

	--delay_;
	delay_ = std::max(delay_, 0);
	if (delay_ <= 0 && bLaserExpand_)
		scaleX_ = std::min(1.0, scaleX_ + 0.1);

	frameWork_++;

	if (frameFadeDelete_ >= 0) {
		--frameFadeDelete_;
	}

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
	--frameGrazeInvalid_;

	if (lastAngle_ != angLaser_) {
		lastAngle_ = angLaser_;
		c_ = cos(angLaser_);
		s_ = sin(angLaser_);
	}
}

void StgStraightLaserObject::_DeleteInAutoClip() {
	if (IsDeleted())return;
	if (!IsAutoDelete())return;
	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();

	int posXE = posX_ + (int)(length_ * c_);
	int posYE = posY_ + (int)(length_ * s_);

	if ((posX_ < rect.left && posXE < rect.left) || (posX_ > rect.right&& posXE > rect.right) ||
		(posY_ < rect.top && posYE < rect.top) || (posY_ > rect.bottom&& posYE > rect.bottom)) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgStraightLaserObject::_DeleteInAutoDeleteFrame() {
	if (IsDeleted())return;
	if (delay_ > 0)return;

	if (frameAutoDelete_ <= 0) {
		SetFadeDelete();
	}
	if (frameAutoDelete_ > 0) --frameAutoDelete_;
}
std::vector<StgIntersectionTarget::ptr> StgStraightLaserObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;
	if (delay_ > 0)return res;
	if (frameFadeDelete_ >= 0)return res;
	if (IsDeleted())return res;
	if (bUserIntersectionMode_)return res;//ユーザ定義あたり判定モード
	if (!bIntersectionEnable_)return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return res;
	if (scaleX_ < 1.0 && typeOwner_ != OWNER_PLAYER)return res;

	float _posXE = posX_ + (length_ * c_);
	float _posYE = posY_ + (length_ * s_);
	float posXS = posX_ + (_posXE - posX_) * invalidLengthStart_;
	float posYS = posY_ + (_posYE - posY_) * invalidLengthStart_;
	float posXE = _posXE + (posX_ - _posXE) * invalidLengthEnd_;
	float posYE = _posYE + (posY_ - _posYE) * invalidLengthEnd_;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	DxWidthLine line(posXS, posYS, posXE, posYE, widthIntersection_);
	shared_ptr<StgShotObject> obj = GetOwnObject();
	if (obj == nullptr)return res;

	weak_ptr<StgShotObject> wObj = obj;
	StgIntersectionTarget_Line::ptr target =
		std::shared_ptr<StgIntersectionTarget_Line>(new StgIntersectionTarget_Line);
	if (target != nullptr) {
		if (typeOwner_ == OWNER_PLAYER)
			target->SetTargetType(StgIntersectionTarget::TYPE_PLAYER_SHOT);
		else
			target->SetTargetType(StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(wObj);
		target->SetLine(line);

		res.push_back(target);
	}
	return res;
}
void StgStraightLaserObject::_AddReservedShot(shared_ptr<StgShotObject> obj, StgShotObject::ReserveShotListData* data) {
	auto objectManager = stageController_->GetMainObjectManager();

	double ownAngle = GetDirectionAngle();
	double ox = GetPositionX();
	double oy = GetPositionY();

	double dRadius = data->GetRadius();
	double dAngle = data->GetAngle();
	double sx = obj->GetPositionX();
	double sy = obj->GetPositionY();
	double objAngle = obj->GetDirectionAngle();
	double sAngle = angLaser_;
	double angle = sAngle + dAngle;

	double tx = ox + sx + dRadius * cos(angle);
	double ty = oy + sy + dRadius * sin(angle);
	obj->SetPositionX(tx);
	obj->SetPositionY(ty);
	obj->SetDirectionAngle(angle + objAngle);

	StgShotManager* shotManager = stageController_->GetShotManager();
	shotManager->AddShot(obj);
	obj->Activate();
	objectManager->ActivateObject(obj->GetObjectID(), true);
}
void StgStraightLaserObject::RenderOnShotManager() {
	if (!IsVisible())return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return;

	int objBlendType = GetBlendType();
	int shotBlendType = objBlendType;
	{
		StgShotRenderer* renderer = nullptr;
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
			shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
		if (renderer == nullptr)return;

		RECT* rcSrc;
		RECT rcDest;
		D3DCOLOR color;

		StgShotData::AnimationData* anime = shotData->GetData(frameWork_);
		rcSrc = anime->GetSource();
		//rcDest = anime->rcDst_;

		color = color_;
		double alpha = shotData->GetAlpha() / 255.0;
		if (frameFadeDelete_ >= 0)
			alpha = (double)frameFadeDelete_ / (double)FRAME_FADEDELETE_LASER;

		bool bValidAlpha = StgShotData::IsAlphaBlendValidType(shotBlendType);
		if (bValidAlpha) {
			//α有効
			int colorA = ColorAccess::GetColorA(color);
			color = ColorAccess::SetColorA(color, alpha * colorA);
		}
		else {
			//α無効
			color = ColorAccess::ApplyAlpha(color, alpha);
		}

		//		if(delay_ > 0)
		//			SetRect(&rcDest, -16, length_, 16, 0);
		//		else
		SetRect(&rcDest, -widthRender_ / 2, length_, widthRender_ / 2, 0);

		VERTEX_TLX verts[4];
		/*
		int srcX[] = { rcSrc->left, rcSrc->right, rcSrc->left, rcSrc->right };
		int srcY[] = { rcSrc->top, rcSrc->top, rcSrc->bottom, rcSrc->bottom };
		int destX[] = { rcDest->left, rcDest->right, rcDest->left, rcDest->right };
		int destY[] = { rcDest->top, rcDest->top, rcDest->bottom, rcDest->bottom };
		*/
		LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
		LONG* ptrDst = reinterpret_cast<LONG*>(&rcDest);

		for (size_t iVert = 0; iVert < 4; iVert++) {
			VERTEX_TLX vt;

			_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1], ptrSrc[iVert | 0b1]);
			_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
			_SetVertexColorARGB(vt, color);

			double px = vt.position.x * scaleX_ * scale_.x;
			double py = vt.position.y * scale_.y;

			vt.position.x = (px * s_ + py * c_) + position_.x;
			vt.position.y = (-px * c_ + py * s_) + position_.y;
			vt.position.z = position_.z;

			//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
			verts[iVert] = vt;
		}

		renderer->AddSquareVertex(verts);
	}

	if (bUseSouce_ && frameFadeDelete_ < 0) {	//Delay cloud
		StgShotRenderer* renderer = nullptr;
		int objSourceBlendType = GetSourceBlendType();
		int sourceBlendType = shotBlendType;
		if (objSourceBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(sourceBlendType);
		}
		else {
			renderer = shotData->GetRenderer(objSourceBlendType);
			sourceBlendType = objSourceBlendType;
		}
		if (renderer == nullptr)return;

		RECT* rcSrc;
		RECT rcDest;
		D3DCOLOR color;

		rcSrc = shotData->GetDelayRect();
		color = shotData->GetDelayColor();

		int sourceWidth = widthRender_ * 2 / 3;
		SetRect(&rcDest, -sourceWidth, -sourceWidth, sourceWidth, sourceWidth);

		VERTEX_TLX verts[4];
		/*
		int srcX[] = { rcSrc->left, rcSrc->right, rcSrc->left, rcSrc->right };
		int srcY[] = { rcSrc->top, rcSrc->top, rcSrc->bottom, rcSrc->bottom };
		int destX[] = { rcDest->left, rcDest->right, rcDest->left, rcDest->right };
		int destY[] = { rcDest->top, rcDest->top, rcDest->bottom, rcDest->bottom };
		*/
		LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
		LONG* ptrDst = reinterpret_cast<LONG*>(&rcDest);

		for (size_t iVert = 0; iVert < 4; iVert++) {
			VERTEX_TLX vt;

			_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1], ptrSrc[iVert | 0b1]);
			_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
			_SetVertexColorARGB(vt, color);

			double px = vt.position.x;
			double py = vt.position.y;
			vt.position.x = (px * s_ + py * c_) + position_.x;
			vt.position.y = (-px * c_ + py * s_) + position_.y;
			vt.position.z = position_.z;

			//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
			verts[iVert] = vt;
		}

		renderer->AddSquareVertex(verts);
	}

}
void StgStraightLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();
	ref_count_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	double ex = posX_;
	double ey = posY_;

	std::vector<double> listPos;
	listPos.resize(2);
	std::vector<gstd::value> listScriptValue;
	listScriptValue.resize(4);

	for (double itemPos = 0; itemPos < (double)length_; itemPos += itemDistance_) {
		double posX = ex + itemPos * c_;
		double posY = ey + itemPos * s_;

		if (scriptItem != nullptr) {
			listPos[0] = posX;
			listPos[1] = posY;

			listScriptValue[0] = scriptItem->CreateRealValue(idObject_);
			listScriptValue[1] = scriptItem->CreateRealArrayValue(listPos);
			listScriptValue[2] = scriptItem->CreateBooleanValue(flgPlayerCollision);
			listScriptValue[3] = scriptItem->CreateRealValue(GetShotDataID());
			scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue);
		}

		if (itemManager->IsDefaultBonusItemEnable() && delay_ == 0 && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
				int id = stageController_->GetMainObjectManager()->AddObject(obj);
				if (id != DxScript::ID_INVALID) {
					//弾の座標にアイテムを作成する
					itemManager->AddItem(obj);
					obj->SetPositionX(posX);
					obj->SetPositionY(posY);
				}
			}
		}
	}
}

/**********************************************************
//StgCurveLaserObject(曲がる型レーザー)
**********************************************************/
StgCurveLaserObject::StgCurveLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::OBJ_CURVE_LASER;
	tipDecrement_ = 0.0;

	invalidLengthStart_ = 0.0f;
	invalidLengthEnd_ = 0.0f;
}
void StgCurveLaserObject::Work() {
	_Move();
	if (delay_ == 0) {
		_AddReservedShotWork();
	}

	--delay_;
	delay_ = std::max(delay_, 0);
	frameWork_++;

	if (frameFadeDelete_ >= 0) {
		--frameFadeDelete_;
	}

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
	//	_AddIntersectionRelativeTarget();
	--frameGrazeInvalid_;

	if (lastAngle_ != GetDirectionAngle()) {
		lastAngle_ = GetDirectionAngle();
		c_ = cos(lastAngle_);
		s_ = sin(lastAngle_);
	}
}
void StgCurveLaserObject::_Move() {
	StgMoveObject::_Move();
	DxScriptRenderObject::SetX(posX_);
	DxScriptRenderObject::SetY(posY_);

	LaserNode pos;
	pos.pos.x = posX_;
	pos.pos.y = posY_;
	{
		double wRender = widthRender_ / 2.0;

		double nx = -wRender * s_;
		double ny = wRender * c_;

		pos.vertOff[0] = { posX_ + nx, posY_ + ny };
		pos.vertOff[1] = { posX_ - nx, posY_ - ny };
	}

	listPosition_.push_front(pos);
	if (listPosition_.size() > length_) {
		listPosition_.pop_back();
	}
}
void StgCurveLaserObject::_DeleteInAutoClip() {
	if (IsDeleted())return;
	if (!IsAutoDelete())return;
	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();

	bool bDelete = listPosition_.size() > 0;
	std::list<LaserNode>::iterator itr = listPosition_.begin();

	size_t i = 0;
	for (; itr != listPosition_.end(); ++itr, ++i) {
		if (i % 2 == 1) continue;	//lmao

		LaserNode& pos = (*itr);
		bool bXOut = pos.pos.x < rect.left || pos.pos.x > rect.right;
		bool bYOut = pos.pos.y < rect.top || pos.pos.y > rect.bottom;
		if (!bXOut && !bYOut) {
			bDelete = false;
			break;
		}
	}

	if (bDelete) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
std::vector<StgIntersectionTarget::ptr> StgCurveLaserObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;
	if (delay_ > 0)return res;
	if (frameFadeDelete_ >= 0)return res;
	if (IsDeleted())return res;
	if (bUserIntersectionMode_)return res;//ユーザ定義あたり判定モード
	if (!bIntersectionEnable_)return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return res;

	shared_ptr<StgShotObject> obj = GetOwnObject();
	if (obj == nullptr)return res;
	weak_ptr<StgShotObject> wObj = obj;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	size_t countPos = listPosition_.size();
	std::list<LaserNode>::iterator itr = listPosition_.begin();

	float iLengthS = invalidLengthStart_ * 0.5f;
	float iLengthE = 0.5f + (1.0f - invalidLengthStart_) * 0.5f;

	int posInvalidS = (int)(countPos * iLengthS);
	int posInvalidE = (int)(countPos * iLengthE);

	for (size_t iPos = 0; iPos < countPos - 1; ++iPos) {
		if (iPos < posInvalidS || iPos > posInvalidE) {
			++itr;
			continue;
		}

		std::list<LaserNode>::iterator itrNext = std::next(itr);

		double posXS = (*itr).pos.x;
		double posYS = (*itr).pos.y;
		double posXE = (*itrNext).pos.x;
		double posYE = (*itrNext).pos.y;
		++itr;
		/*
				if(iPos == 0)
				{
					double ang = atan2(posYE - posYS, posXE - posXS);
					int length = min(0 , invalidLengthStart_);
					posXS = posXS + length * cos(ang);
					posYS = posYS + length * sin(ang);
				}
				if(iPos == countPos - 2)
				{
					double ang = atan2(posYE - posYS, posXE - posXS);
					int length = max(invalidLengthEnd_, 0);
					posXE = posXE - length * cos(ang);
					posYE = posYE - length * sin(ang);
				}
		*/

		DxWidthLine line(posXS, posYS, posXE, posYE, widthIntersection_);
		StgIntersectionTarget_Line::ptr target =
			std::shared_ptr<StgIntersectionTarget_Line>(new StgIntersectionTarget_Line);
		if (target != nullptr) {
			if (typeOwner_ == OWNER_PLAYER)
				target->SetTargetType(StgIntersectionTarget::TYPE_PLAYER_SHOT);
			else
				target->SetTargetType(StgIntersectionTarget::TYPE_ENEMY_SHOT);
			target->SetObject(wObj);
			target->SetLine(line);

			res.push_back(target);
		}
	}
	return res;
}

void StgCurveLaserObject::RenderOnShotManager() {
	if (!IsVisible())return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return;

	int shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
	StgShotRenderer* renderer = nullptr;

	if ((delay_ > 0) /*&& (listPosition_.size() <= length_)*/) {
		int objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
			shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = shotData->GetRenderer(objDelayBlendType);
		}
		if (renderer == nullptr)return;

		RECT* rcSrc = shotData->GetDelayRect();
		RECT* rcDest = shotData->GetDelayDest();

		double expa = 0.5f + (double)delay_ / 30.0f * 2;
		if (expa > 3.5)expa = 3.5;

		double sX = listPosition_.back().pos.x;
		double sY = listPosition_.back().pos.y;

		D3DCOLOR color = shotData->GetDelayColor();

		VERTEX_TLX verts[4];
		/*
		int srcX[] = { rcSrc->left, rcSrc->right, rcSrc->left, rcSrc->right };
		int srcY[] = { rcSrc->top, rcSrc->top, rcSrc->bottom, rcSrc->bottom };
		int destX[] = { rcDest->left, rcDest->right, rcDest->left, rcDest->right };
		int destY[] = { rcDest->top, rcDest->top, rcDest->bottom, rcDest->bottom };
		*/
		LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
		LONG* ptrDst = reinterpret_cast<LONG*>(rcDest);

		for (size_t iVert = 0; iVert < 4; iVert++) {
			VERTEX_TLX vt;

			_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1], ptrSrc[iVert | 0b1]);
			_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
			_SetVertexColorARGB(vt, color);

			double px = vt.position.x * expa;
			double py = vt.position.y * expa;
			vt.position.x = (px * c_ - py * s_) + sX;
			vt.position.y = (px * s_ + py * c_) + sY;
			vt.position.z = position_.z;

			//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
			verts[iVert] = vt;
		}

		renderer->AddSquareVertex(verts);
	}
	{
		int objBlendType = GetBlendType();
		int shotBlendType = objBlendType;
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
			shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
		if (renderer == nullptr)return;

		//---------------------------------------------------

		size_t countPos = listPosition_.size();
		size_t countRect = countPos - 1U;

		double baseAlpha = shotData->GetAlpha() / 255.0;
		if (frameFadeDelete_ >= 0)
			baseAlpha = (double)frameFadeDelete_ / (double)FRAME_FADEDELETE;

		double tAlpha = ColorAccess::GetColorA(color_);
		double dAlpha = tAlpha / (double)countRect * 2.0 * tipDecrement_;
		bool bValidAlpha = StgShotData::IsAlphaBlendValidType(shotBlendType);
		int baseColorA = ColorAccess::GetColorA(color_);

		RECT* rcSrcOrg = shotData->GetData(frameWork_)->GetSource();
		double rcInc = (double)(rcSrcOrg->bottom - rcSrcOrg->top) / countRect;
		double rectV = rcSrcOrg->top;

		double srcX[] = { (double)rcSrcOrg->left, (double)rcSrcOrg->right };

		VERTEX_TLX oldVerts[2];
		std::list<LaserNode>::iterator itr = listPosition_.begin();
		for (size_t iPos = 0; iPos < countRect; ++iPos) {
			std::list<LaserNode>::iterator itrNext = std::next(itr);

			double thisAlpha = tAlpha;
			if (iPos > countRect / 2)
				thisAlpha -= dAlpha;
			else if (iPos < countRect / 2)
				thisAlpha = iPos * 256 / (countRect / 2) + (255 - tipDecrement_ * 255.);
			thisAlpha = std::max(0.0, thisAlpha);

			D3DCOLOR thisColor = color_;
			if (bValidAlpha) {
				thisColor = ColorAccess::SetColorA(thisColor, thisAlpha * baseColorA * baseAlpha);
			}
			else {
				thisColor = ColorAccess::ApplyAlpha(thisColor, thisAlpha * baseAlpha / 255.0);
			}

			LaserNode& posCurr = *itr;
			LaserNode& posNext = *itrNext;

			VERTEX_TLX verts[4];
			Position* destPos[] = { 
				&posCurr.vertOff[0], &posCurr.vertOff[1], 
				&posNext.vertOff[0], &posNext.vertOff[1]
			};
			for (size_t iVert = (iPos > 0U ? 2U : 0U); iVert < 4U; ++iVert) {
				VERTEX_TLX vt;

				_SetVertexUV(vt, srcX[iVert % 2U], rectV + iVert / 2U * rcInc);
				_SetVertexPosition(vt, destPos[iVert]->x, destPos[iVert]->y, position_.z);
				_SetVertexColorARGB(vt, thisColor);

				verts[iVert] = vt;
			}

			if (iPos > 0U)
				memcpy(verts, oldVerts, sizeof(VERTEX_TLX) * 2U);
			memcpy(oldVerts, &verts[2], sizeof(VERTEX_TLX) * 2U);

			renderer->AddSquareVertex(verts);

			rectV += rcInc;
			++itr;
		}
	}
}
void StgCurveLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();
	ref_count_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	std::vector<double> listPos;
	listPos.resize(2);
	std::vector<gstd::value> listScriptValue;
	listScriptValue.resize(4);

	std::list<LaserNode>::iterator itr = listPosition_.begin();
	for (; itr != listPosition_.end(); itr++) {
		LaserNode& pos = (*itr);

		if (scriptItem != nullptr) {
			listPos[0] = pos.pos.x;
			listPos[1] = pos.pos.y;

			listScriptValue[0] = scriptItem->CreateRealValue(idObject_);
			listScriptValue[1] = scriptItem->CreateRealArrayValue(listPos);
			listScriptValue[2] = scriptItem->CreateBooleanValue(flgPlayerCollision);
			listScriptValue[3] = scriptItem->CreateRealValue(GetShotDataID());
			scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue);
		}

		if (itemManager->IsDefaultBonusItemEnable() && delay_ == 0 && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
				int id = stageController_->GetMainObjectManager()->AddObject(obj);
				if (id != DxScript::ID_INVALID) {
					//弾の座標にアイテムを作成する
					itemManager->AddItem(obj);
					obj->SetPositionX(pos.pos.x);
					obj->SetPositionY(pos.pos.y);
				}
			}
		}
	}
}