#include "source/GcLib/pch.h"

#include "StgPlayer.hpp"
#include "StgSystem.hpp"

/**********************************************************
//StgPlayerObject
**********************************************************/
StgPlayerObject::StgPlayerObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::OBJ_PLAYER;

	infoPlayer_ = new StgPlayerInformation();
	RECT* rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	int stgWidth = rcStgFrame->right - rcStgFrame->left;
	int stgHeight = rcStgFrame->bottom - rcStgFrame->top;

	SetRenderPriorityI(30);
	speedFast_ = 4;
	speedSlow_ = 1.6;
	SetRect(&rcClip_, 0, 0, stgWidth, stgHeight);

	state_ = STATE_NORMAL;
	frameStateDown_ = 120;
	frameRebirthMax_ = 15;
	infoPlayer_->frameRebirth_ = frameRebirthMax_;
	frameRebirthDiff_ = 3;

	infoPlayer_->life_ = 3;
	infoPlayer_->countBomb_ = 3;
	infoPlayer_->power_ = 1.0;

	itemCircle_ = 24;
	frameInvincibility_ = 0;
	bForbidShot_ = false;
	bForbidSpell_ = false;
	yAutoItemCollect_ = -256 * 256;

	enableStateEnd_ = true;
	enableShootdownEvent_ = true;

	rebirthX_ = REBIRTH_DEFAULT;
	rebirthY_ = REBIRTH_DEFAULT;

	hitObjectID_ = DxScript::ID_INVALID;

	_InitializeRebirth();
}
StgPlayerObject::~StgPlayerObject() {}
void StgPlayerObject::_InitializeRebirth() {
	RECT* rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();

	SetX((rebirthX_ == REBIRTH_DEFAULT) ? (rcStgFrame->right - rcStgFrame->left) / 2.0 : rebirthX_);
	SetY((rebirthY_ == REBIRTH_DEFAULT) ? rcStgFrame->bottom - 48 : rebirthY_);
}
void StgPlayerObject::Work() {
	EDirectInput* input = EDirectInput::GetInstance();
	auto scriptManager = stageController_->GetScriptManager();
	StgEnemyManager* enemyManager = stageController_->GetEnemyManager();

	//当たり判定クリア
	ClearIntersected();

	if (state_ == STATE_NORMAL) {
		//通常時
		if (hitObjectID_ != DxScript::ID_INVALID) {
			if (frameInvincibility_ <= 0) {
				state_ = STATE_HIT;
				frameState_ = infoPlayer_->frameRebirth_;

				gstd::value valueHitObjectID = script_->CreateRealValue(hitObjectID_);
				std::vector<gstd::value> listScriptValue;
				listScriptValue.push_back(valueHitObjectID);
				script_->RequestEvent(StgStagePlayerScript::EV_HIT, listScriptValue);
			}
		}
		else {
			if (listGrazedShot_.size() > 0) {
				std::vector<value> listValPos;
				std::vector<double> listShotID;

				int grazedShotCount = listGrazedShot_.size();
				for (size_t iObj = 0; iObj < grazedShotCount; iObj++) {
					if (auto ptrObj = listGrazedShot_[iObj].lock()) {
						shared_ptr<StgShotObject> objShot = std::dynamic_pointer_cast<StgShotObject>(ptrObj);

						if (objShot != nullptr) {
							double id = (double)objShot->GetObjectID();
							listShotID.push_back(id);

							std::vector<double> listPos;
							listPos.push_back(objShot->GetPositionX());
							listPos.push_back(objShot->GetPositionY());
							listValPos.push_back(script_->CreateRealArrayValue(listPos));
						}
					}
				}

				std::vector<gstd::value> listScriptValue;
				listScriptValue.push_back(script_->CreateRealValue(grazedShotCount));
				listScriptValue.push_back(script_->CreateRealArrayValue(listShotID));
				listScriptValue.push_back(script_->CreateValueArrayValue(listValPos));
				script_->RequestEvent(StgStagePlayerScript::EV_GRAZE, listScriptValue);

				auto stageScriptManager = stageController_->GetScriptManager();
				ref_count_ptr<ManagedScript> itemScript = stageScriptManager->GetItemScript();
				if (itemScript != nullptr) {
					itemScript->RequestEvent(StgStagePlayerScript::EV_GRAZE, listScriptValue);
				}
			}
			//_Move();
			if (input->GetVirtualKeyState(EDirectInput::KEY_BOMB) == KEY_PUSH)
				CallSpell();

			_AddIntersection();
		}
	}
	if (state_ == STATE_HIT) {
		//くらいボム待機
		if (input->GetVirtualKeyState(EDirectInput::KEY_BOMB) == KEY_PUSH)
			CallSpell();

		if (state_ == STATE_HIT) {
			//くらいボム有効フレーム減少
			frameState_--;
			if (frameState_ < 0) {
				//自機ダウン
				bool bEnemyLastSpell = false;

				shared_ptr<StgEnemyBossSceneObject> objBossScene = enemyManager->GetBossSceneObject();
				if (objBossScene) {
					objBossScene->AddPlayerShootDownCount();
					if (objBossScene->GetActiveData()->IsLastSpell())
						bEnemyLastSpell = true;
				}
				if (!bEnemyLastSpell)
					infoPlayer_->life_--;

				if (enableShootdownEvent_)
					scriptManager->RequestEventAll(StgStagePlayerScript::EV_PLAYER_SHOOTDOWN);
				
				if (infoPlayer_->life_ >= 0 || !enableStateEnd_) {
					bVisible_ = false;
					state_ = STATE_DOWN;
					frameState_ = frameStateDown_;
				}
				else {
					state_ = STATE_END;
				}

				//Also prevents STATE_END and STATE_DOWN
				if (!enableShootdownEvent_) {
					frameState_ = 0;
					bVisible_ = true;
					_InitializeRebirth();
					state_ = STATE_NORMAL;
				}
			}
		}
	}
	if (state_ == STATE_DOWN) {
		//ダウン
		frameState_--;
		if (frameState_ <= 0) {
			bVisible_ = true;
			_InitializeRebirth();
			state_ = STATE_NORMAL;
			scriptManager->RequestEventAll(StgStageScript::EV_PLAYER_REBIRTH);
		}
	}
	if (state_ == STATE_END) {
		bVisible_ = false;
	}

	--frameInvincibility_;
	frameInvincibility_ = std::max(frameInvincibility_, 0);
	listGrazedShot_.clear();
	hitObjectID_ = DxScript::ID_INVALID;
}
void StgPlayerObject::Move() {
	if (state_ == STATE_NORMAL) {
		//通常時
		if (hitObjectID_ == DxScript::ID_INVALID) {
			_Move();
		}
	}
}
void StgPlayerObject::_Move() {
	double sx = 0;
	double sy = 0;
	EDirectInput* input = EDirectInput::GetInstance();
	int keyLeft = input->GetVirtualKeyState(EDirectInput::KEY_LEFT);
	int keyRight = input->GetVirtualKeyState(EDirectInput::KEY_RIGHT);
	int keyUp = input->GetVirtualKeyState(EDirectInput::KEY_UP);
	int keyDown = input->GetVirtualKeyState(EDirectInput::KEY_DOWN);
	int keySlow = input->GetVirtualKeyState(EDirectInput::KEY_SLOWMOVE);

	double speed = speedFast_;
	if (keySlow == KEY_PUSH || keySlow == KEY_HOLD)speed = speedSlow_;

	bool bKetLeft = keyLeft == KEY_PUSH || keyLeft == KEY_HOLD;
	bool bKeyRight = keyRight == KEY_PUSH || keyRight == KEY_HOLD;
	bool bKeyUp = keyUp == KEY_PUSH || keyUp == KEY_HOLD;
	bool bKeyDown = keyDown == KEY_PUSH || keyDown == KEY_HOLD;

	if (bKetLeft && !bKeyRight)sx += -speed;
	if (!bKetLeft && bKeyRight)sx += speed;
	if (bKeyUp && !bKeyDown)sy += -speed;
	if (!bKeyUp && bKeyDown)sy += speed;

	constexpr double diagFactor = 1.0 / gstd::GM_SQRT2;
	if (sx != 0 && sy != 0) {
		sx *= diagFactor;
		sy *= diagFactor;
	}

	double px = posX_ + sx;
	double py = posY_ + sy;

	//はみ出たときの処理
	px = std::max(px, (double)rcClip_.left);
	px = std::min(px, (double)rcClip_.right);
	py = std::max(py, (double)rcClip_.top);
	py = std::min(py, (double)rcClip_.bottom);

	SetX(px);
	SetY(py);
}
void StgPlayerObject::_AddIntersection() {
	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	UpdateIntersectionRelativeTarget(posX_, posY_, 0);
	RegistIntersectionRelativeTarget(intersectionManager);
}
bool StgPlayerObject::_IsValidSpell() {
	bool res = true;
	res &= (state_ == STATE_NORMAL || (state_ == STATE_HIT && frameState_ > 0));
	res &= (objSpell_ == nullptr || objSpell_->IsDeleted());
	return res;
}
void StgPlayerObject::CallSpell() {
	if (!_IsValidSpell())return;
	if (!IsPermitSpell())return;

	auto objectManager = stageController_->GetMainObjectManager();
	objSpell_ = shared_ptr<StgPlayerSpellManageObject>(new StgPlayerSpellManageObject());
	int idSpell = objectManager->AddObject(objSpell_);

	gstd::value vUse = script_->RequestEvent(StgStagePlayerScript::EV_REQUEST_SPELL);
	if (!script_->IsBooleanValue(vUse))
		throw gstd::wexception(L"@Event(EV_REQUEST_SPELL) must return a boolean value.");
	bool bUse = vUse.as_boolean();
	if (!bUse) {
		objectManager->DeleteObject(objSpell_);
		objSpell_ = nullptr;
		return;
	}

	if (state_ == STATE_HIT) {
		state_ = STATE_NORMAL;
		infoPlayer_->frameRebirth_ -= frameRebirthDiff_;
		infoPlayer_->frameRebirth_ = std::max(infoPlayer_->frameRebirth_, 0);
	}

	StgEnemyManager* enemyManager = stageController_->GetEnemyManager();
	shared_ptr<StgEnemyBossSceneObject> objBossScene = enemyManager->GetBossSceneObject();
	if (objBossScene != nullptr) {
		objBossScene->AddPlayerSpellCount();
	}

	auto scriptManager = stageController_->GetScriptManager();
	scriptManager->RequestEventAll(StgStageScript::EV_PLAYER_SPELL);
}

void StgPlayerObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	StgIntersectionTarget_Player::ptr own = std::dynamic_pointer_cast<StgIntersectionTarget_Player>(ownTarget);
	if (own == nullptr) return;

	if (auto ptrObj = otherTarget->GetObject().lock()) {
		int otherType = otherTarget->GetTargetType();
		switch (otherType) {
		case StgIntersectionTarget::TYPE_ENEMY_SHOT:
		{
			StgShotObject* objShot = (StgShotObject*)ptrObj.get();
			if (objShot != nullptr) {
				//敵弾
				if (!own->IsGraze()) {
					hitObjectID_ = objShot->GetObjectID();

					if (objShot->GetLife() != StgShotObject::LIFE_SPELL_REGIST &&
						objShot->GetObjectType() == TypeObject::OBJ_SHOT)
						objShot->ConvertToItem(true);
				}
				else if (objShot->IsValidGraze()) {
					listGrazedShot_.push_back(otherTarget->GetObject());
					stageController_->GetStageInformation()->AddGraze(1);
				}
			}
		}
		break;
		case StgIntersectionTarget::TYPE_ENEMY:
		{
			//敵
			if (!own->IsGraze()) {
				shared_ptr<StgEnemyObject> objEnemy = std::dynamic_pointer_cast<StgEnemyObject>(ptrObj);
				if (objEnemy)
					hitObjectID_ = objEnemy->GetObjectID();
			}
		}
		break;
		}
	}
}
shared_ptr<StgPlayerObject> StgPlayerObject::GetOwnObject() {
	return std::dynamic_pointer_cast<StgPlayerObject>(stageController_->GetMainRenderObject(idObject_));
}
bool StgPlayerObject::IsPermitShot() {
	//以下のとき不可
	//・会話中
	return !bForbidShot_;
}
bool StgPlayerObject::IsPermitSpell() {
	//以下のとき不可
	//・会話中
	//・ラストスペル中
	StgEnemyManager* enemyManager = stageController_->GetEnemyManager();
	bool bEnemyLastSpell = false;
	shared_ptr<StgEnemyBossSceneObject> objBossScene = enemyManager->GetBossSceneObject();
	if (objBossScene) {
		shared_ptr<StgEnemyBossSceneData> data = objBossScene->GetActiveData();
		if (data != nullptr && data->IsLastSpell())
			bEnemyLastSpell = true;
	}

	return !bEnemyLastSpell && !bForbidSpell_;
}
bool StgPlayerObject::IsWaitLastSpell() {
	bool res = IsPermitSpell() && state_ == STATE_HIT;
	return res;
}

/**********************************************************
//StgPlayerSpellObject
**********************************************************/
StgPlayerSpellObject::StgPlayerSpellObject(StgStageController* stageController) {
	stageController_ = stageController;
	damage_ = 0;
	bEraseShot_ = true;
	life_ = 256 * 256 * 256;
}
void StgPlayerSpellObject::Work() {
	if (IsDeleted())return;
	if (life_ <= 0) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgPlayerSpellObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	double damage = 0;
	int otherType = otherTarget->GetTargetType();
	switch (otherType) {
	case StgIntersectionTarget::TYPE_ENEMY:
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		damage = 1;
		break;
	}
	}
	life_ -= damage;
	life_ = std::max(life_, 0.0);
}


