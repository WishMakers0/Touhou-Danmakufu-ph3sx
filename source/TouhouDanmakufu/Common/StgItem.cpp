#include "source/GcLib/pch.h"

#include "StgItem.hpp"
#include "StgSystem.hpp"
#include "StgStageScript.hpp"
#include "StgPlayer.hpp"

/**********************************************************
//StgItemManager
**********************************************************/
StgItemManager::StgItemManager(StgStageController* stageController) {
	stageController_ = stageController;
	listItemData_ = new StgItemDataList();

	std::wstring dir = EPathProperty::GetSystemImageDirectory();

	listSpriteItem_ = new SpriteList2D();
	std::wstring pathItem = dir + L"System_Stg_Item.png";
	ref_count_ptr<Texture> textureItem = new Texture();
	textureItem->CreateFromFile(pathItem, false, false);
	listSpriteItem_->SetTexture(textureItem);

	listSpriteDigit_ = new SpriteList2D();
	std::wstring pathDigit = dir + L"System_Stg_Digit.png";
	ref_count_ptr<Texture> textureDigit = new Texture();
	textureDigit->CreateFromFile(pathDigit, false, false);
	listSpriteDigit_->SetTexture(textureDigit);

	bDefaultBonusItemEnable_ = true;
	bAllItemToPlayer_ = false;

	//listObj_.reserve(ITEM_MAX);

	vertexBuffer_ = nullptr;
	_SetVertexBuffer(256 * 256);

	{
		RenderShaderManager* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();
		effectLayer_ = shaderManager_->GetRender2DShader();
		handleEffectWorld_ = effectLayer_->GetParameterBySemantic(nullptr, "WORLD");
	}
}
StgItemManager::~StgItemManager() {
	ptr_release(vertexBuffer_);

	ptr_delete(listSpriteItem_);
	ptr_delete(listSpriteDigit_);
	ptr_delete(listItemData_);
}
void StgItemManager::Work() {
	shared_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
	double px = objPlayer->GetX();
	double py = objPlayer->GetY();
	double pr = objPlayer->GetItemIntersectionRadius();
	pr *= pr;
	int pAutoItemCollectY = objPlayer->GetAutoItemCollectY();

	std::list<shared_ptr<StgItemObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end();) {
		shared_ptr<StgItemObject> obj = *itr;

		if (obj->IsDeleted()) {
			//obj->Clear();
			itr = listObj_.erase(itr);
		}
		else {
			double ix = obj->GetPositionX();
			double iy = obj->GetPositionY();
			if (objPlayer->GetState() == StgPlayerObject::STATE_NORMAL) {
				bool bMoveToPlayer = false;

				double dx = px - ix;
				double dy = py - iy;

				//if (obj->GetItemType() == StgItemObject::ITEM_SCORE && obj->GetFrameWork() > 60 * 15)
				//	obj->Intersect(nullptr, nullptr);

				bool deleted = false;

				double radius = dx * dx + dy * dy;
				if (radius <= 16 * 16) {
					obj->Intersect(nullptr, nullptr);
					deleted = true;
				}
				else if (radius <= pr && obj->IsPermitMoveToPlayer()) {
					obj->SetMoveToPlayer(true);
					bMoveToPlayer = true;
				}

				if (!deleted) {
					if (bCancelToPlayer_) {
						//自動回収キャンセル
						obj->SetMoveToPlayer(false);
					}
					else if (obj->IsPermitMoveToPlayer() && !bMoveToPlayer) {
						if (pAutoItemCollectY >= 0) {
							//上部自動回収
							int typeMove = obj->GetMoveType();
							if (!obj->IsMoveToPlayer() && py <= pAutoItemCollectY)
								bMoveToPlayer = true;
						}

						if (listItemTypeToPlayer_.size() > 0) {
							//自機にアイテムを集める
							int typeItem = obj->GetItemType();
							bool bFind = listItemTypeToPlayer_.find(typeItem) != listItemTypeToPlayer_.end();
							if (bFind)
								bMoveToPlayer = true;
						}

						if (listCircleToPlayer_.size() > 0) {
							std::list<DxCircle>::iterator itr = listCircleToPlayer_.begin();
							for (; itr != listCircleToPlayer_.end(); itr++) {
								DxCircle circle = *itr;

								double cdx = ix - circle.GetX();
								double cdy = iy - circle.GetY();

								if ((cdx * cdx + cdy * cdy) <= circle.GetR() * circle.GetR()) {
									bMoveToPlayer = true;
									break;
								}
							}
						}

						if (bAllItemToPlayer_)
							bMoveToPlayer = true;

						if (bMoveToPlayer)
							obj->SetMoveToPlayer(true);
					}
				}
			}
			else {
				/*
				if (obj->GetItemType() == StgItemObject::ITEM_SCORE) {
					std::shared_ptr<StgMovePattern_Item> move =
						std::shared_ptr<StgMovePattern_Item>(new StgMovePattern_Item(obj.get()));
					move->SetItemMoveType(StgMovePattern_Item::MOVE_DOWN);
					obj->SetPattern(move);
				}
				*/
				obj->SetMoveToPlayer(false);
			}

			++itr;
		}
	}
	listItemTypeToPlayer_.clear();
	listCircleToPlayer_.clear();
	bAllItemToPlayer_ = false;
	bCancelToPlayer_ = false;

}
void StgItemManager::Render(int targetPriority) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetCullingMode(D3DCULL_NONE);
	graphics->SetLightingEnable(false);
	graphics->SetTextureFilter(DirectGraphics::MODE_TEXTURE_FILTER_LINEAR, 0);

	//フォグを解除する
	DWORD bEnableFog = FALSE;
	device->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();

	D3DXMATRIX& matCamera = camera2D->GetMatrix();

	std::list<shared_ptr<StgItemObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgItemObject> obj = *itr;
		if (obj->IsDeleted())continue;
		if (obj->GetRenderPriorityI() != targetPriority)continue;

		obj->RenderOnItemManager();
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

	RenderShaderManager* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();

	device->SetFVF(VERTEX_TLX::fvf);

	int countBlendType = StgItemDataList::RENDER_TYPE_COUNT;
	int blendMode[] = { 
		DirectGraphics::MODE_BLEND_ADD_ARGB, 
		DirectGraphics::MODE_BLEND_ADD_RGB, 
		DirectGraphics::MODE_BLEND_ALPHA 
	};

	{
		device->SetVertexDeclaration(nullptr);

		graphics->SetBlendMode(DirectGraphics::MODE_BLEND_ADD_ARGB);
		listSpriteDigit_->Render();
		listSpriteDigit_->ClearVertexCount();
		graphics->SetBlendMode(DirectGraphics::MODE_BLEND_ALPHA);
		listSpriteItem_->Render();
		listSpriteItem_->ClearVertexCount();

		device->SetVertexDeclaration(shaderManager_->GetVertexDeclarationTLX());

		for (size_t iBlend = 0; iBlend < countBlendType; iBlend++) {
			graphics->SetBlendMode(blendMode[iBlend]);

			std::vector<StgItemRenderer*>* listRenderer =
				listItemData_->GetRendererList(blendMode[iBlend] - 1);

			for (size_t iRender = 0; iRender < listRenderer->size(); iRender++)
				(*listRenderer)[iRender]->Render(this);
		}

		device->SetVertexDeclaration(nullptr);
	}

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void StgItemManager::_SetVertexBuffer(size_t size) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	ptr_release(vertexBuffer_);

	vertexBufferSize_ = size;

	device->CreateVertexBuffer(size * sizeof(VERTEX_TLX), D3DUSAGE_DYNAMIC, VERTEX_TLX::fvf, D3DPOOL_DEFAULT,
		&vertexBuffer_, nullptr);
}

void StgItemManager::GetValidRenderPriorityList(std::vector<PriListBool>& list) {
	auto objectManager = stageController_->GetMainObjectManager();
	list.clear();
	list.resize(objectManager->GetRenderBucketCapacity());

	std::list<shared_ptr<StgItemObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end(); ++itr) {
		shared_ptr<StgItemObject> obj = *itr;
		if (obj->IsDeleted()) continue;
		int pri = obj->GetRenderPriorityI();
		list[pri] = true;
	}
}

bool StgItemManager::LoadItemData(std::wstring path, bool bReload) {
	return listItemData_->AddItemDataList(path, bReload);
}
shared_ptr<StgItemObject> StgItemManager::CreateItem(int type) {
	shared_ptr<StgItemObject> res;
	switch (type) {
	case StgItemObject::ITEM_1UP:
	case StgItemObject::ITEM_1UP_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_1UP(stageController_));
		break;
	case StgItemObject::ITEM_SPELL:
	case StgItemObject::ITEM_SPELL_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_Bomb(stageController_));
		break;
	case StgItemObject::ITEM_POWER:
	case StgItemObject::ITEM_POWER_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_Power(stageController_));
		break;
	case StgItemObject::ITEM_POINT:
	case StgItemObject::ITEM_POINT_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_Point(stageController_));
		break;
	case StgItemObject::ITEM_USER:
		res = shared_ptr<StgItemObject>(new StgItemObject_User(stageController_));
		break;
	}
	res->SetItemType(type);

	return res;
}
void StgItemManager::CollectItemsAll() {
	bAllItemToPlayer_ = true;
}
void StgItemManager::CollectItemsByType(int type) {
	listItemTypeToPlayer_.insert(type);
}
void StgItemManager::CollectItemsInCircle(DxCircle circle) {
	listCircleToPlayer_.push_back(circle);
}
void StgItemManager::CancelCollectItems() {
	bCancelToPlayer_ = true;
}

/**********************************************************
//StgItemDataList
**********************************************************/
StgItemDataList::StgItemDataList() {
	listRenderer_.resize(RENDER_TYPE_COUNT);
}
StgItemDataList::~StgItemDataList() {
	for (std::vector<StgItemRenderer*>& renderList : listRenderer_) {
		for (StgItemRenderer* renderer : renderList)
			ptr_delete(renderer);
		renderList.clear();
	}
	listRenderer_.clear();

	for (StgItemData* itemData : listData_)
		ptr_delete(itemData);
	listData_.clear();
}
bool StgItemDataList::AddItemDataList(std::wstring path, bool bReload) {
	if (!bReload && listReadPath_.find(path) != listReadPath_.end())return true;

	ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr) throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	if (!reader->Open())throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	std::string source = reader->ReadAllString();

	bool res = false;
	Scanner scanner(source);
	try {
		std::vector<StgItemData*> listData;

		std::wstring pathImage = L"";
		RECT rcDelay = { -1, -1, -1, -1 };
		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::TK_EOF)//Eofの識別子が来たらファイルの調査終了
			{
				break;
			}
			else if (tok.GetType() == Token::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"ItemData") {
					_ScanItem(listData, scanner);
				}
				else if (element == L"item_image") {
					scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
					pathImage = scanner.Next().GetString();
				}

				if (scanner.HasNext())
					tok = scanner.Next();

			}
		}

		//テクスチャ読み込み
		if (pathImage.size() == 0)throw gstd::wexception("Item texture must be set.");
		std::wstring dir = PathProperty::GetFileDirectory(path);
		pathImage = StringUtility::Replace(pathImage, L"./", dir);

		ref_count_ptr<Texture> texture = new Texture();
		bool bTexture = texture->CreateFromFile(pathImage, false, false);
		if (!bTexture)throw gstd::wexception("The specified item texture cannot be found.");

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
			for (size_t iRender = 0; iRender < listRenderer_.size(); iRender++) {
				StgItemRenderer* render = new StgItemRenderer();
				render->SetTexture(texture);
				listRenderer_[iRender].push_back(render);
			}
		}

		if (listData_.size() < listData.size())
			listData_.resize(listData.size());
		for (size_t iData = 0; iData < listData.size(); iData++) {
			StgItemData* data = listData[iData];
			if (data == nullptr)continue;
			data->indexTexture_ = textureIndex;
			listData_[iData] = data;
		}

		listReadPath_.insert(path);
		Logger::WriteTop(StringUtility::Format(L"Loaded item data: %s", path.c_str()));
		res = true;
	}
	catch (gstd::wexception& e) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: [Line=%d] (%s)", scanner.GetCurrentLine(), e.what());
		Logger::WriteTop(log);
		res = false;
	}
	catch (...) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: [Line=%d] (Unknown error.)", scanner.GetCurrentLine());
		Logger::WriteTop(log);
		res = false;
	}

	return res;
}
void StgItemDataList::_ScanItem(std::vector<StgItemData*>& listData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
	scanner.CheckType(tok, Token::TK_OPENC);

	StgItemData* data = new StgItemData(this);
	int id = -1;
	int typeItem = -1;

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
			else if (element == L"type") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				typeItem = scanner.Next().GetInteger();
			}
			else if (element == L"rect") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);

				StgItemData::AnimationData anime;

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
				anime.rcDest_ = rcDest;

				data->listAnime_.resize(1);
				data->listAnime_[0] = anime;
				data->totalAnimeFrame_ = 1;
			}
			else if (element == L"out") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				RECT rect;
				rect.left = StringUtility::ToInteger(list[0]);
				rect.top = StringUtility::ToInteger(list[1]);
				rect.right = StringUtility::ToInteger(list[2]);
				rect.bottom = StringUtility::ToInteger(list[3]);
				data->rcOutSrc_ = rect;

				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
				if (width % 2 == 1) rcDest.right += 1;
				if (height % 2 == 1) rcDest.bottom += 1;
				data->rcOutDest_ = rcDest;
			}
			else if (element == L"render") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				std::wstring render = scanner.Next().GetElement();
				if (render == L"ADD" || render == L"ADD_RGB")
					data->typeRender_ = DirectGraphics::MODE_BLEND_ADD_RGB;
				else if (render == L"ADD_ARGB")
					data->typeRender_ = DirectGraphics::MODE_BLEND_ADD_ARGB;
			}
			else if (element == L"alpha") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				data->alpha_ = scanner.Next().GetInteger();
			}
			else if (element == L"AnimationData") {
				_ScanAnimation(data, scanner);
			}
		}
	}

	if (id >= 0) {
		if (listData.size() <= id)
			listData.resize(id + 1);

		if (typeItem < 0)
			typeItem = id;
		data->typeItem_ = typeItem;

		listData[id] = data;
	}
}
void StgItemDataList::_ScanAnimation(StgItemData* itemData, Scanner& scanner) {
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
					StgItemData::AnimationData anime;
					int frame = StringUtility::ToInteger(list[0]);
					RECT rcSrc = {
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]),
						StringUtility::ToInteger(list[3]),
						StringUtility::ToInteger(list[4]),
					};

					int width = rcSrc.right - rcSrc.left;
					int height = rcSrc.bottom - rcSrc.top;
					RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
					if (width % 2 == 1)rcDest.right += 1;
					if (height % 2 == 1)rcDest.bottom += 1;

					anime.frame_ = frame;
					anime.rcSrc_ = rcSrc;
					anime.rcDest_ = rcDest;

					itemData->listAnime_.push_back(anime);
					itemData->totalAnimeFrame_ += frame;
				}
			}
		}
	}
}
std::vector<std::wstring> StgItemDataList::_GetArgumentList(Scanner& scanner) {
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

//StgItemData
StgItemData::StgItemData(StgItemDataList* listItemData) {
	listItemData_ = listItemData;
	typeRender_ = DirectGraphics::MODE_BLEND_ALPHA;
	SetRect(&rcOutSrc_, 0, 0, 0, 0);
	SetRect(&rcOutDest_, 0, 0, 0, 0);
	alpha_ = 255;
	totalAnimeFrame_ = 0;
}
StgItemData::~StgItemData() {}
StgItemData::AnimationData* StgItemData::GetData(int frame) {
	if (totalAnimeFrame_ <= 1)
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
StgItemRenderer* StgItemData::GetRenderer(int type) {
	if (type < DirectGraphics::MODE_BLEND_ALPHA || type > DirectGraphics::MODE_BLEND_ADD_ARGB)
		return listItemData_->GetRenderer(indexTexture_, 0);
	return listItemData_->GetRenderer(indexTexture_, type - 1);
}

/**********************************************************
//StgItemRenderer
**********************************************************/
StgItemRenderer::StgItemRenderer() {
	countRenderVertex_ = 0;
	countMaxVertex_ = 256 * 256;
	SetVertexCount(countMaxVertex_);
}
StgItemRenderer::~StgItemRenderer() {

}
void StgItemRenderer::Render(StgItemManager* manager) {
	if (countRenderVertex_ < 3)
		return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	gstd::ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else
		device->SetTexture(0, nullptr);

	//device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (int)(countRenderVertex_ / 3), vertex_.get(), sizeof(VERTEX_TLX));

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
			device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, countRenderVertex_ / 3);
			effect->EndPass();
		}
		effect->End();
	}

	countRenderVertex_ = 0;
}
void StgItemRenderer::AddVertex(VERTEX_TLX& vertex) {
	if (countRenderVertex_ == countMaxVertex_ - 1) {
		countMaxVertex_ *= 2;
		SetVertexCount(countMaxVertex_);
	}

	SetVertex(countRenderVertex_, vertex);
	++countRenderVertex_;
}

/**********************************************************
//StgItemObject
**********************************************************/
StgItemObject::StgItemObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::OBJ_ITEM;

	pattern_ = std::shared_ptr<StgMovePattern_Item>(new StgMovePattern_Item(this));
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	score_ = 0;

	bMoveToPlayer_ = false;
	bPermitMoveToPlayer_ = true;
	bChangeItemScore_ = true;

	frameWork_ = 0;

	int priItemI = stageController_->GetStageInformation()->GetItemObjectPriority();
	double priItemD = (double)priItemI / (stageController_->GetMainObjectManager()->GetRenderBucketCapacity() - 1);
	SetRenderPriority(priItemD);
}
void StgItemObject::Work() {
	bool bDefaultMovePattern = std::dynamic_pointer_cast<StgMovePattern_Item>(GetPattern()) != nullptr;
	if (!bDefaultMovePattern && IsMoveToPlayer()) {
		double speed = 8;
		shared_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
		double angle = atan2(objPlayer->GetY() - GetPositionY(), objPlayer->GetX() - GetPositionX());
		double angDirection = angle;
		SetSpeed(speed);
		SetDirectionAngle(angDirection);
	}
	StgMoveObject::_Move();
	SetX(posX_);
	SetY(posY_);

	_DeleteInAutoClip();

	++frameWork_;
}
void StgItemObject::RenderOnItemManager() {
	StgItemManager* itemManager = stageController_->GetItemManager();
	SpriteList2D* renderer = typeItem_ == ITEM_SCORE ?
		itemManager->GetDigitRenderer() : itemManager->GetItemRenderer();

	if (typeItem_ != ITEM_SCORE) {
		double scale = 1.0;
		switch (typeItem_) {
		case ITEM_1UP:
		case ITEM_SPELL:
		case ITEM_POWER:
		case ITEM_POINT:
			scale = 1.0;
			break;

		case ITEM_1UP_S:
		case ITEM_SPELL_S:
		case ITEM_POWER_S:
		case ITEM_POINT_S:
		case ITEM_BONUS:
			scale = 0.75;
			break;
		}

		RECT rcSrc;
		switch (typeItem_) {
		case ITEM_1UP:
		case ITEM_1UP_S:
			SetRect(&rcSrc, 1, 1, 16, 16);
			break;
		case ITEM_SPELL:
		case ITEM_SPELL_S:
			SetRect(&rcSrc, 20, 1, 35, 16);
			break;
		case ITEM_POWER:
		case ITEM_POWER_S:
			SetRect(&rcSrc, 40, 1, 55, 16);
			break;
		case ITEM_POINT:
		case ITEM_POINT_S:
			SetRect(&rcSrc, 1, 20, 16, 35);
			break;
		case ITEM_BONUS:
			SetRect(&rcSrc, 20, 20, 35, 35);
			break;
		}

		//上にはみ出している
		double posY = posY_;
		D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255);
		if (posY_ <= 0) {
			D3DCOLOR colorOver = D3DCOLOR_ARGB(255, 255, 255, 255);
			switch (typeItem_) {
			case ITEM_1UP:
			case ITEM_1UP_S:
				colorOver = D3DCOLOR_ARGB(255, 236, 0, 236);
				break;
			case ITEM_SPELL:
			case ITEM_SPELL_S:
				colorOver = D3DCOLOR_ARGB(255, 0, 160, 0);
				break;
			case ITEM_POWER:
			case ITEM_POWER_S:
				colorOver = D3DCOLOR_ARGB(255, 209, 0, 0);
				break;
			case ITEM_POINT:
			case ITEM_POINT_S:
				colorOver = D3DCOLOR_ARGB(255, 0, 0, 160);
				break;
			}
			if (color != colorOver) {
				SetRect(&rcSrc, 113, 1, 126, 10);
				posY = 6;
			}
			color = colorOver;
		}

		RECT_D rcSrcD = GetRectD(rcSrc);
		renderer->SetColor(color);
		renderer->SetPosition(posX_, posY, 0);
		renderer->SetScaleXYZ(scale, scale, scale);
		renderer->SetSourceRect(rcSrcD);
		renderer->SetDestinationCenter();
		renderer->AddVertex();
	}
	else {
		renderer->SetScaleXYZ(1.0, 1.0, 1.0);
		renderer->SetColor(color_);
		renderer->SetPosition(0, 0, 0);

		int fontSize = 14;
		int64_t score = score_;
		std::vector<int> listNum;
		while (true) {
			int tnum = score % 10;
			score /= 10;
			listNum.push_back(tnum);
			if (score == 0)break;
		}
		for (int iNum = listNum.size() - 1; iNum >= 0; iNum--) {
			RECT_D rcSrc = { (double)(listNum[iNum] * 36), 0.,
				(double)((listNum[iNum] + 1) * 36 - 1), 31. };
			RECT_D rcDest = { (double)(posX_ + (listNum.size() - 1 - iNum)*fontSize / 2), (double)posY_,
				(double)(posX_ + (listNum.size() - iNum)*fontSize / 2), (double)(posY_ + fontSize) };
			renderer->SetSourceRect(rcSrc);
			renderer->SetDestinationRect(rcDest);
			renderer->AddVertex();
		}
	}
}
void StgItemObject::_DeleteInAutoClip() {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	RECT rcClip;
	ZeroMemory(&rcClip, sizeof(RECT));
	rcClip.left = -64;
	rcClip.right = graphics->GetScreenWidth() + 64;
	rcClip.bottom = graphics->GetScreenHeight() + 64;
	bool bDelete = (posX_ < rcClip.left || posX_ > rcClip.right || posY_ > rcClip.bottom);
	if (!bDelete)return;

	stageController_->GetMainObjectManager()->DeleteObject(this);
}
void StgItemObject::_CreateScoreItem() {
	auto objectManager = stageController_->GetMainObjectManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
		shared_ptr<StgItemObject_Score> obj = shared_ptr<StgItemObject_Score>(new StgItemObject_Score(stageController_));
		obj->SetX(posX_);
		obj->SetY(posY_);
		obj->SetScore(score_);
		objectManager->AddObject(obj);
		itemManager->AddItem(obj);
	}
}
void StgItemObject::_NotifyEventToPlayerScript(std::vector<float>& listValue) {
	//自機スクリプトへ通知
	shared_ptr<StgPlayerObject> player = stageController_->GetPlayerObject();
	StgStagePlayerScript* scriptPlayer = player->GetPlayerScript();
	std::vector<gstd::value> listScriptValue;
	for (size_t iVal = 0; iVal < listValue.size(); iVal++) {
		listScriptValue.push_back(scriptPlayer->CreateRealValue(listValue[iVal]));
	}

	scriptPlayer->RequestEvent(StgStageItemScript::EV_GET_ITEM, listScriptValue);
}
void StgItemObject::_NotifyEventToItemScript(std::vector<float>& listValue) {
	//アイテムスクリプトへ通知
	auto stageScriptManager = stageController_->GetScriptManager();
	int64_t idItemScript = stageScriptManager->GetItemScriptID();
	if (idItemScript != StgControlScriptManager::ID_INVALID) {
		ref_count_ptr<ManagedScript> scriptItem = stageScriptManager->GetScript(idItemScript);
		if (scriptItem) {
			std::vector<gstd::value> listScriptValue;
			for (size_t iVal = 0; iVal < listValue.size(); iVal++) {
				listScriptValue.push_back(scriptItem->CreateRealValue(listValue[iVal]));
			}
			scriptItem->RequestEvent(StgStageItemScript::EV_GET_ITEM, listScriptValue);
		}
	}
}
void StgItemObject::SetAlpha(int alpha) {
	color_ = ColorAccess::SetColorA(color_, alpha);
}
void StgItemObject::SetColor(int r, int g, int b) {
	color_ = ColorAccess::SetColorR(color_, r);
	color_ = ColorAccess::SetColorG(color_, g);
	color_ = ColorAccess::SetColorB(color_, b);
}
void StgItemObject::SetToPosition(POINT pos) {
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetToPosition(pos);
}
int StgItemObject::GetMoveType() {
	int res = StgMovePattern_Item::MOVE_NONE;

	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	if (move) res = move->GetItemMoveType();
	return res;
}
void StgItemObject::SetMoveType(int type) {
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	if (move) move->SetItemMoveType(type);
}


//StgItemObject_1UP
StgItemObject_1UP::StgItemObject_1UP(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_1UP;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_1UP::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bomb
StgItemObject_Bomb::StgItemObject_Bomb(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_SPELL;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_Bomb::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Power
StgItemObject_Power::StgItemObject_Power(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_POWER;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
	score_ = 10;
}
void StgItemObject_Power::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	if (bChangeItemScore_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Point
StgItemObject_Point::StgItemObject_Point(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_POINT;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_Point::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	if (bChangeItemScore_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bonus
StgItemObject_Bonus::StgItemObject_Bonus(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_BONUS;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPLAYER);

	int graze = stageController->GetStageInformation()->GetGraze();
	score_ = (int)(graze / 40) * 10 + 300;
}
void StgItemObject_Bonus::Work() {
	StgItemObject::Work();

	shared_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
	if (objPlayer->GetState() != StgPlayerObject::STATE_NORMAL) {
		_CreateScoreItem();
		stageController_->GetStageInformation()->AddScore(score_);

		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgItemObject_Bonus::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Score
StgItemObject_Score::StgItemObject_Score(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_SCORE;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_SCORE);

	bPermitMoveToPlayer_ = false;

	frameDelete_ = 0;
}
void StgItemObject_Score::Work() {
	StgItemObject::Work();
	int alpha = 255 - frameDelete_ * 8;
	color_ = D3DCOLOR_ARGB(alpha, alpha, alpha, alpha);

	if (frameDelete_ > 30) {
		stageController_->GetMainObjectManager()->DeleteObject(this);
		return;
	}
	frameDelete_++;
}
void StgItemObject_Score::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) { }

//StgItemObject_User
StgItemObject_User::StgItemObject_User(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_USER;
	idImage_ = -1;
	frameWork_ = 0;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_DOWN);

	bChangeItemScore_ = true;
}
void StgItemObject_User::SetImageID(int id) {
	idImage_ = id;
	StgItemData* data = _GetItemData();
	if (data) {
		typeItem_ = data->GetItemType();
	}
}
StgItemData* StgItemObject_User::_GetItemData() {
	StgItemData* res = nullptr;
	StgItemManager* itemManager = stageController_->GetItemManager();
	StgItemDataList* dataList = itemManager->GetItemDataList();

	if (dataList) {
		res = dataList->GetData(idImage_);
	}

	return res;
}
void StgItemObject_User::_SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z, float w) {
	constexpr float bias = -0.5f;
	vertex.position.x = x + bias;
	vertex.position.y = y + bias;
	vertex.position.z = z;
	vertex.position.w = w;
}
void StgItemObject_User::_SetVertexUV(VERTEX_TLX& vertex, float u, float v) {
	StgItemData* itemData = _GetItemData();
	if (itemData == nullptr) return;

	ref_count_ptr<Texture> texture = itemData->GetTexture();
	int width = texture->GetWidth();
	int height = texture->GetHeight();
	vertex.texcoord.x = u / width;
	vertex.texcoord.y = v / height;
}
void StgItemObject_User::_SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color) {
	vertex.diffuse_color = color;
}
void StgItemObject_User::Work() {
	StgItemObject::Work();
	frameWork_++;
}
void StgItemObject_User::RenderOnItemManager() {
	if (!IsVisible())return;

	StgItemData* itemData = _GetItemData();
	if (itemData == nullptr)return;

	StgItemRenderer* renderer = nullptr;

	int objBlendType = GetBlendType();
	if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
		objBlendType = itemData->GetRenderType();
	}
	
	renderer = itemData->GetRenderer(objBlendType);

	if (renderer == nullptr)return;

	double scaleX = scale_.x;
	double scaleY = scale_.y;
	double c = 1.0;
	double s = 0.0;
	double posy = position_.y;

	StgItemData::AnimationData* frameData = itemData->GetData(frameWork_);

	RECT* rcSrc = &frameData->rcSrc_;
	RECT* rcDst = &frameData->rcDest_;
	D3DCOLOR color;

	{
		bool bOutY = false;
		if (position_.y + (rcSrc->bottom - rcSrc->top) / 2 <= 0) {
			bOutY = true;
			rcSrc = itemData->GetOutSrc();
			rcDst = itemData->GetOutDest();
		}

		if (!bOutY) {
			c = cos(angle_.z);
			s = sin(angle_.z);
		}
		else {
			scaleX = 1.0;
			scaleY = 1.0;
			posy = (rcSrc->bottom - rcSrc->top) / 2;
		}

		bool bBlendAddRGB = (objBlendType == DirectGraphics::MODE_BLEND_ADD_RGB);

		color = color_;
		double alpha = itemData->GetAlpha() / 255.0;
		if (bBlendAddRGB)
			color = ColorAccess::ApplyAlpha(color, alpha);
		else {
			int colorA = ColorAccess::GetColorA(color);
			color = ColorAccess::SetColorA(color, alpha * colorA);
		}
	}

	//if(bIntersected_)color = D3DCOLOR_ARGB(255, 255, 0, 0);//接触テスト

	VERTEX_TLX verts[4];
	/*
	int srcX[] = { rcSrc.left, rcSrc.right, rcSrc.left, rcSrc.right };
	int srcY[] = { rcSrc.top, rcSrc.top, rcSrc.bottom, rcSrc.bottom };
	int destX[] = { rcDest.left, rcDest.right, rcDest.left, rcDest.right };
	int destY[] = { rcDest.top, rcDest.top, rcDest.bottom, rcDest.bottom };
	*/
	LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
	LONG* ptrDst = reinterpret_cast<LONG*>(rcDst);

	for (size_t iVert = 0; iVert < 4; iVert++) {
		VERTEX_TLX vt;

		_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1], ptrSrc[iVert | 0b1]);
		_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
		_SetVertexColorARGB(vt, color);

		double px = vt.position.x * scaleX;
		double py = vt.position.y * scaleY;
		vt.position.x = (px * c - py * s) + position_.x;
		vt.position.y = (px * s + py * c) + posy;
		vt.position.z = position_.z;

		//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
		verts[iVert] = vt;
	}

	renderer->AddSquareVertex(verts);
}
void StgItemObject_User::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	if (bChangeItemScore_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}


/**********************************************************
//StgMovePattern_Item
**********************************************************/
StgMovePattern_Item::StgMovePattern_Item(StgMoveObject* target) : StgMovePattern(target) {
	frame_ = 0;
	typeMove_ = MOVE_DOWN;
	speed_ = 3;
	angDirection_ = Math::DegreeToRadian(270);
	ZeroMemory(&posTo_, sizeof(POINT));
}
void StgMovePattern_Item::Move() {
	StgItemObject* itemObject = (StgItemObject*)target_;
	StgStageController* stageController = itemObject->GetStageController();

	double px = target_->GetPositionX();
	double py = target_->GetPositionY();
	if (typeMove_ == MOVE_TOPLAYER || itemObject->IsMoveToPlayer()) {
		if (frame_ == 0) speed_ = 6;
		speed_ += 0.025;
		shared_ptr<StgPlayerObject> objPlayer = stageController->GetPlayerObject();
		double angle = atan2(objPlayer->GetY() - py, objPlayer->GetX() - px);
		angDirection_ = angle;
	}
	else if (typeMove_ == MOVE_TOPOSITION_A) {
		double dx = posTo_.x - px;
		double dy = posTo_.y - py;
		speed_ = sqrt(dx * dx + dy * dy) / 16.0;

		double angle = atan2(dy, dx);
		angDirection_ = angle;
		if (frame_ == 60) {
			speed_ = 0;
			angDirection_ = Math::DegreeToRadian(90);
			typeMove_ = MOVE_DOWN;
		}
	}
	else if (typeMove_ == MOVE_DOWN) {
		speed_ += 3.0 / 60.0;
		if (speed_ > 2.5) speed_ = 2.5;
		angDirection_ = Math::DegreeToRadian(90);
	}
	else if (typeMove_ == MOVE_SCORE) {
		speed_ = 1;
		angDirection_ = Math::DegreeToRadian(270);
	}

	if (typeMove_ != MOVE_NONE) {
		c_ = cos(angDirection_);
		s_ = sin(angDirection_);
		double sx = speed_ * c_;
		double sy = speed_ * s_;
		px = target_->GetPositionX() + sx;
		py = target_->GetPositionY() + sy;
		target_->SetPositionX(px);
		target_->SetPositionY(py);
	}

	++frame_;
}

