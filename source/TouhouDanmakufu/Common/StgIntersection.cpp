#include "source/GcLib/pch.h"
#include "StgIntersection.hpp"
#include "StgShot.hpp"
#include "StgPlayer.hpp"
#include "StgEnemy.hpp"

/**********************************************************
//StgIntersectionManager
**********************************************************/
StgIntersectionManager::StgIntersectionManager() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int screenWidth = graphics->GetScreenWidth();
	int screenHeight = graphics->GetScreenWidth();

	//_CreatePool(2);
	listSpace_.resize(3);
	for (int iSpace = 0; iSpace < listSpace_.size(); iSpace++) {
		StgIntersectionSpace* space = new StgIntersectionSpace();
		space->Initialize(-100, -100, screenWidth + 100, screenHeight + 100);
		listSpace_[iSpace] = space;
	}
}
StgIntersectionManager::~StgIntersectionManager() {
	for (auto& itr : listSpace_) {
		if (itr) {
			delete itr;
			itr = nullptr;
		}
	}
	listSpace_.clear();
}
void StgIntersectionManager::Work() {
	listEnemyTargetPoint_ = listEnemyTargetPointNext_;
	listEnemyTargetPointNext_.clear();

	//Prevent issues where some small hitboxes would randomly get skipped by forcing a thread lock.
	lock_.Enter();

	int totalCheck = 0;
	int totalTarget = 0;
	std::vector<StgIntersectionSpace*>::iterator itr = listSpace_.begin();
	for (; itr != listSpace_.end(); itr++) {
		StgIntersectionSpace* space = *itr;
		StgIntersectionCheckList* listCheck = space->CreateIntersectionCheckList(totalTarget);
		int countCheck = listCheck->GetCheckCount();
		for (int iCheck = 0; iCheck < countCheck; iCheck++) {
			//Getは1回しか使用できません
			StgIntersectionTarget::ptr targetA = listCheck->GetTargetA(iCheck);
			StgIntersectionTarget::ptr targetB = listCheck->GetTargetB(iCheck);

			bool bIntersected = IsIntersected(targetA, targetB);
			if (!bIntersected) continue;

			//Grazeの関係で、先に自機の当たり判定をする必要がある。
			ref_count_weak_ptr<StgIntersectionObject>::unsync objA = targetA->GetObject();
			ref_count_weak_ptr<StgIntersectionObject>::unsync objB = targetB->GetObject();

			if (objA != NULL) {
				objA->Intersect(targetA, targetB);
				objA->SetIntersected();
				if (objB != NULL) objA->AddIntersectedId(objB->GetDxScriptObjectID());
			}
			if (objB != NULL) {
				objB->Intersect(targetB, targetA);
				objB->SetIntersected();
				if (objA != NULL) objB->AddIntersectedId(objA->GetDxScriptObjectID());
			}
		}

		totalCheck += countCheck;
		space->ClearTarget();
	}

	lock_.Leave();

	//_ArrangePool();

	ELogger* logger = ELogger::GetInstance();
	if (logger->IsWindowVisible()) {
		/*
		int countUsed = GetUsedPoolObjectCount();
		int countCache = GetCachePoolObjectCount();
		logger->SetInfo(9, L"Intersection count",
			StringUtility::Format(L"Used=%4d, Cached=%4d, Total=%4d, Check=%4d", countUsed, countCache, countUsed + countCache, totalCheck));
		*/
		logger->SetInfo(9, L"Intersection count",
			StringUtility::Format(L"Total=%4d, Check=%4d", totalTarget, totalCheck));
	}
}
void StgIntersectionManager::AddTarget(StgIntersectionTarget::ptr target) {
	//SPACE_PLAYER_ENEMY = 0,//自機-敵、敵弾
	//SPACE_PLAYERSOHT_ENEMY,//自弾,スペル-敵
	//SPACE_PLAYERSHOT_ENEMYSHOT,//自弾,スペル-敵弾

	//target->SetMortonNumber(-1);
	//target->ClearObjectIntersectedIdList();

	int type = target->GetTargetType();
	switch (type) {
	case StgIntersectionTarget::TYPE_PLAYER:
	{
		listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetA(target);
		break;
	}

	case StgIntersectionTarget::TYPE_PLAYER_SHOT:
	case StgIntersectionTarget::TYPE_PLAYER_SPELL:
	{
		listSpace_[SPACE_PLAYERSOHT_ENEMY]->RegistTargetA(target);

		//弾消し能力付加なら
		bool bEraseShot = false;
		if (type == StgIntersectionTarget::TYPE_PLAYER_SHOT) {
			StgShotObject* shot = (StgShotObject*)target->GetObject().GetPointer();
			if (shot != NULL)
				bEraseShot = shot->IsEraseShot();
		}
		else if (type == StgIntersectionTarget::TYPE_PLAYER_SPELL) {
			StgPlayerSpellObject* spell = (StgPlayerSpellObject*)target->GetObject().GetPointer();
			if (spell != NULL)
				bEraseShot = spell->IsEraseShot();
		}

		if (bEraseShot) {
			listSpace_[SPACE_PLAYERSHOT_ENEMYSHOT]->RegistTargetA(target);
		}
		break;
	}

	case StgIntersectionTarget::TYPE_ENEMY:
	{
		listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetB(target);
		listSpace_[SPACE_PLAYERSOHT_ENEMY]->RegistTargetB(target);

		StgIntersectionTarget_Circle::ptr circle = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
		if (circle != NULL) {
			ref_count_weak_ptr<StgEnemyObject>::unsync objEnemy =
				ref_count_weak_ptr<StgEnemyObject>::unsync::DownCast(target->GetObject());
			if (objEnemy != NULL) {
				int idObject = objEnemy->GetObjectID();
				POINT pos = { (int)circle->GetCircle().GetX(), (int)circle->GetCircle().GetY() };
				StgIntersectionTargetPoint tp;
				tp.SetObjectID(idObject);
				tp.SetPoint(pos);
				listEnemyTargetPointNext_.push_back(tp);
			}
		}

		break;
	}

	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetB(target);
		listSpace_[SPACE_PLAYERSHOT_ENEMYSHOT]->RegistTargetB(target);
		break;
	}
	}
}
void StgIntersectionManager::AddEnemyTargetToShot(StgIntersectionTarget::ptr target) {
	//target->SetMortonNumber(-1);
	//target->ClearObjectIntersectedIdList();

	int type = target->GetTargetType();
	switch (type) {
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		listSpace_[SPACE_PLAYERSOHT_ENEMY]->RegistTargetB(target);

		StgIntersectionTarget_Circle::ptr circle = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
		if (circle != NULL) {
			ref_count_weak_ptr<StgEnemyObject>::unsync objEnemy =
				ref_count_weak_ptr<StgEnemyObject>::unsync::DownCast(target->GetObject());
			if (objEnemy != NULL) {
				int idObject = objEnemy->GetObjectID();
				POINT pos = { (int)circle->GetCircle().GetX(), (int)circle->GetCircle().GetY() };
				StgIntersectionTargetPoint tp;
				tp.SetObjectID(idObject);
				tp.SetPoint(pos);
				listEnemyTargetPointNext_.push_back(tp);
			}
		}

		break;
	}
	}
}
void StgIntersectionManager::AddEnemyTargetToPlayer(StgIntersectionTarget::ptr target) {
	//target->SetMortonNumber(-1);
	//target->ClearObjectIntersectedIdList();

	int type = target->GetTargetType();
	switch (type) {
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		listSpace_[SPACE_PLAYER_ENEMY]->RegistTargetB(target);
		break;
	}
	}
}

bool StgIntersectionManager::IsIntersected(StgIntersectionTarget::ptr& target1, StgIntersectionTarget::ptr& target2) {
	bool res = false;
	int shape1 = target1->GetShape();
	int shape2 = target2->GetShape();
	StgIntersectionTarget* p1 = target1.get();
	StgIntersectionTarget* p2 = target2.get();
	if (p1 == nullptr || p2 == nullptr) return false;
	if (shape1 == StgIntersectionTarget::SHAPE_CIRCLE && shape2 == StgIntersectionTarget::SHAPE_CIRCLE) {
		StgIntersectionTarget_Circle* c1 = dynamic_cast<StgIntersectionTarget_Circle*>(p1);
		StgIntersectionTarget_Circle* c2 = dynamic_cast<StgIntersectionTarget_Circle*>(p2);
		res = DxMath::IsIntersected(c1->GetCircle(), c2->GetCircle());
	}
	else if ((shape1 == StgIntersectionTarget::SHAPE_CIRCLE && shape2 == StgIntersectionTarget::SHAPE_LINE) ||
		(shape1 == StgIntersectionTarget::SHAPE_LINE && shape2 == StgIntersectionTarget::SHAPE_CIRCLE)) {
		StgIntersectionTarget_Circle* c = nullptr;
		StgIntersectionTarget_Line* l = nullptr;
		if (shape1 == StgIntersectionTarget::SHAPE_CIRCLE && shape2 == StgIntersectionTarget::SHAPE_LINE) {
			c = dynamic_cast<StgIntersectionTarget_Circle*>(p1);
			l = dynamic_cast<StgIntersectionTarget_Line*>(p2);
		}
		else {
			c = dynamic_cast<StgIntersectionTarget_Circle*>(p2);
			l = dynamic_cast<StgIntersectionTarget_Line*>(p1);
		}

		res = DxMath::IsIntersected(c->GetCircle(), l->GetLine());
	}
	else if (shape1 == StgIntersectionTarget::SHAPE_LINE && shape2 == StgIntersectionTarget::SHAPE_LINE) {
		StgIntersectionTarget_Line* l1 = dynamic_cast<StgIntersectionTarget_Line*>(p1);
		StgIntersectionTarget_Line* l2 = dynamic_cast<StgIntersectionTarget_Line*>(p2);
		res = DxMath::IsIntersected(l1->GetLine(), l2->GetLine());
	}
	return res;
}

/*
void StgIntersectionManager::_ResetPoolObject(StgIntersectionTarget::ptr& obj) {
	//	ELogger::WriteTop(StringUtility::Format("_ResetPoolObject:start:%s)", obj->GetInfoAsString().c_str()));
	obj->obj_ = NULL;
	//	ELogger::WriteTop("_ResetPoolObject:end");
}
ref_count_ptr<StgIntersectionTarget>::unsync StgIntersectionManager::_CreatePoolObject(int type) {
	StgIntersectionTarget::ptr res = nullptr;
	switch (type) {
	case StgIntersectionTarget::SHAPE_CIRCLE:
		res = new StgIntersectionTarget_Circle();
		break;
	case StgIntersectionTarget::SHAPE_LINE:
		res = new StgIntersectionTarget_Line();
		break;
	}
	return res;
}

void StgIntersectionManager::CheckDeletedObject(std::string funcName) {
	int countType = listUsedPool_.size();
	for (int iType = 0; iType < countType; iType++) {
		std::list<StgIntersectionTarget::ptr>* listUsed = &listUsedPool_[iType];
		std::vector<StgIntersectionTarget::ptr>* listCache = &listCachePool_[iType];

		std::list<StgIntersectionTarget::ptr>::iterator itr = listUsed->begin();
		for (; itr != listUsed->end(); itr++) {
			StgIntersectionTarget::ptr& target = (*itr);
			ref_count_weak_ptr<DxScriptObjectBase>::unsync dxObj =
				ref_count_weak_ptr<DxScriptObjectBase>::unsync::DownCast(target->GetObject());
			if (dxObj != NULL && dxObj->IsDeleted()) {
				ELogger::WriteTop(StringUtility::Format(L"%s(deleted):%s", funcName.c_str(), target->GetInfoAsString().c_str()));
			}
		}
	}
}
*/

/**********************************************************
//StgIntersectionSpace
**********************************************************/
StgIntersectionSpace::StgIntersectionSpace() {
	spaceLeft_ = 0;
	spaceTop_ = 0;
	spaceWidth_ = 0;
	spaceHeight_ = 0;

	listCheck_ = new StgIntersectionCheckList();
}
StgIntersectionSpace::~StgIntersectionSpace() {
	if (listCheck_) {
		delete listCheck_;
		listCheck_ = nullptr;
	}
}
bool StgIntersectionSpace::Initialize(int left, int top, int right, int bottom) {
	listCell_.resize(2);

	spaceLeft_ = left;
	spaceTop_ = top;
	spaceWidth_ = right - left;
	spaceHeight_ = bottom - top;

	return true;
}
bool StgIntersectionSpace::RegistTarget(int type, StgIntersectionTarget::ptr& target) {
	RECT& rect = target->GetIntersectionSpaceRect();
	if (rect.right < spaceLeft_ || rect.bottom < spaceTop_ ||
		rect.left >(spaceLeft_ + spaceWidth_) || rect.top >(spaceTop_ + spaceHeight_))
		return false;

	listCell_[type].push_back(target);
	return true;
}

void StgIntersectionSpace::ClearTarget() {
	/*
	for (int iType = 0; iType < listCell_.size(); ++iType) {
		listCell_[iType].clear();
	}
	*/

	listCell_[0].clear();
	listCell_[1].clear();
}
StgIntersectionCheckList* StgIntersectionSpace::CreateIntersectionCheckList(int& total) {
	StgIntersectionCheckList* res = listCheck_;
	res->Clear();

	total += _WriteIntersectionCheckList(res);

	return res;
}
int StgIntersectionSpace::_WriteIntersectionCheckList(StgIntersectionCheckList*& listCheck)
//	std::vector<std::vector<StgIntersectionTarget*>> &listStack) 
{
	int count = 0;

	std::vector<StgIntersectionTarget::ptr>& listTargetA = listCell_[0];
	std::vector<StgIntersectionTarget::ptr>& listTargetB = listCell_[1];

	for (StgIntersectionTarget::ptr& targetA : listTargetA) {
		for (StgIntersectionTarget::ptr& targetB : listTargetB) {
			RECT& rc1 = targetA->GetIntersectionSpaceRect();
			RECT& rc2 = targetB->GetIntersectionSpaceRect();

			++count;

			bool bIntersectX = (rc1.left >= rc2.right && rc2.left >= rc1.right)
				|| (rc1.left <= rc2.right && rc2.left <= rc1.right);
			if (!bIntersectX) continue;
			bool bIntersectY = (rc1.bottom >= rc2.top && rc2.bottom >= rc1.top) 
				|| (rc1.bottom <= rc2.top && rc2.bottom <= rc1.top);
			if (!bIntersectY) continue;

			listCheck->Add(targetA, targetB);
		}
	}

	return count;

	//I have no idea what all these below were meant to do, but removing them resulted in 
	//both no unfortunate issues and faster performance.

	/*
	std::vector<std::vector<StgIntersectionTarget*>>& listCell = listCell_[indexSpace];
	int typeCount = listCell.size();
	for (int iType1 = 0; iType1 < typeCount; ++iType1) {
		std::vector<StgIntersectionTarget*>& list1 = listCell[iType1];
		int iType2 = 0;
		for (iType2 = iType1 + 1; iType2 < typeCount; ++iType2) {
			std::vector<StgIntersectionTarget*>& list2 = listCell[iType2];

			// ① 空間内のオブジェクト同士の衝突リスト作成
			std::vector<StgIntersectionTarget*>::iterator itr1 = list1.begin();
			for (; itr1 != list1.end(); ++itr1) {
				std::vector<StgIntersectionTarget*>::iterator itr2 = list2.begin();
				for (; itr2 != list2.end(); ++itr2) {
					StgIntersectionTarget*& target1 = (*itr1);
					StgIntersectionTarget*& target2 = (*itr2);
					listCheck->Add(target1, target2);
				}
			}

		}

		std::vector<StgIntersectionTarget*>& stack = listStack[iType1];
		for (iType2 = 0; iType2 < typeCount; ++iType2) {
			if (iType1 == iType2)continue;
			std::vector<StgIntersectionTarget*>& list2 = listCell[iType2];

			// ② 衝突スタックとの衝突リスト作成
			std::vector<StgIntersectionTarget*>::iterator itrStack = stack.begin();
			for (; itrStack != stack.end(); ++itrStack) {
				std::vector<StgIntersectionTarget*>::iterator itr2 = list2.begin();
				for (; itr2 != list2.end(); ++itr2) {
					StgIntersectionTarget*& target2 = (*itr2);
					StgIntersectionTarget*& targetStack = (*itrStack);
					if (iType1 < iType2)
						listCheck->Add(targetStack, target2);
					else
						listCheck->Add(target2, targetStack);
				}
			}
		}
	}

	//空間内のオブジェクトをスタックに追加
	int iType = 0;
	for (iType = 0; iType < typeCount; ++iType) {
		std::vector<StgIntersectionTarget*>& list = listCell[iType];
		std::vector<StgIntersectionTarget*>& stack = listStack[iType];
		std::vector<StgIntersectionTarget*>::iterator itr = list.begin();
		for (; itr != list.end(); ++itr) {
			StgIntersectionTarget* target = (*itr);
			stack.push_back(target);
		}
	}

	//スタックから解除
	for (iType = 0; iType < typeCount; ++iType) {
		std::vector<StgIntersectionTarget*>& list = listCell[iType];
		std::vector<StgIntersectionTarget*>& stack = listStack[iType];
		int count = list.size();
		for (int iCount = 0; iCount < count; ++iCount) {
			stack.pop_back();
		}
	}
	*/
}

/*
unsigned int StgIntersectionSpace::_GetMortonNumber(float left, float top, float right, float bottom) {
	// 座標から空間番号を算出
	// 最小レベルにおける各軸位置を算出
	unsigned int  LT = _GetPointElem(left, top);
	unsigned int  RB = _GetPointElem(right, bottom);

	// 空間番号の排他的論理和から
	// 所属レベルを算出
	unsigned int def = RB ^ LT;
	unsigned int hiLevel = 0;
	for (int iLevel = 0; iLevel < unitLevel_; ++iLevel) {
		DWORD Check = (def >> (iLevel * 2)) & 0x3;
		if (Check != 0)
			hiLevel = iLevel + 1;
	}
	DWORD spaceIndex = RB >> (hiLevel * 2);
	DWORD addIndex = (listCountLevel_[unitLevel_ - hiLevel] - 1) / 3;
	spaceIndex += addIndex;

	if (spaceIndex > countCell_)
		return 0xffffffff;

	return spaceIndex;
}
unsigned int StgIntersectionSpace::_BitSeparate32(unsigned int n) {
	// ビット分割関数
	n = (n | (n << 8)) & 0x00ff00ff;
	n = (n | (n << 4)) & 0x0f0f0f0f;
	n = (n | (n << 2)) & 0x33333333;
	return (n | (n << 1)) & 0x55555555;
}
unsigned short StgIntersectionSpace::_Get2DMortonNumber(unsigned short x, unsigned short y) {
	// 2Dモートン空間番号算出関数
	return (unsigned short)(_BitSeparate32(x) | (_BitSeparate32(y) << 1));
}
unsigned int  StgIntersectionSpace::_GetPointElem(float pos_x, float pos_y) {
	// 座標→線形4分木要素番号変換関数
	float val1 = max(pos_x - spaceLeft_, 0);
	float val2 = max(pos_y - spaceTop_, 0);
	return _Get2DMortonNumber(
		(unsigned short)(val1 / unitWidth_), (unsigned short)(val2 / unitHeight_));
}
*/

//StgIntersectionObject
void StgIntersectionObject::ClearIntersectionRelativeTarget() {
	for (int iTarget = 0; iTarget < listRelativeTarget_.size(); ++iTarget) {
		StgIntersectionTarget::ptr target = listRelativeTarget_[iTarget];
		target->SetObject(nullptr);
	}
	listRelativeTarget_.clear();
}
void StgIntersectionObject::AddIntersectionRelativeTarget(StgIntersectionTarget::ptr target) {
	listRelativeTarget_.push_back(target);
	int shape = target->GetShape();
	if (shape == StgIntersectionTarget::SHAPE_CIRCLE) {
		StgIntersectionTarget_Circle::ptr tTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
		if (tTarget != nullptr)
			listOrgCircle_.push_back(tTarget->GetCircle());
	}
	else if (shape == StgIntersectionTarget::SHAPE_LINE) {
		StgIntersectionTarget_Line::ptr tTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Line>(target);
		if (tTarget != nullptr)
			listOrgLine_.push_back(tTarget->GetLine());
	}
}
void StgIntersectionObject::UpdateIntersectionRelativeTarget(int posX, int posY, double angle) {
	int iCircle = 0;
	int iLine = 0;
	for (int iTarget = 0; iTarget < listRelativeTarget_.size(); ++iTarget) {
		StgIntersectionTarget::ptr target = listRelativeTarget_[iTarget];
		int shape = target->GetShape();
		if (shape == StgIntersectionTarget::SHAPE_CIRCLE) {
			StgIntersectionTarget_Circle::ptr tTarget = std::dynamic_pointer_cast<StgIntersectionTarget_Circle>(target);
			if (tTarget != nullptr) {
				DxCircle org = listOrgCircle_[iCircle];
				int px = org.GetX() + posX;
				int py = org.GetY() + posY;

				DxCircle circle = tTarget->GetCircle();
				circle.SetX(px);
				circle.SetY(py);
				tTarget->SetCircle(circle);
			}
			++iCircle;
		}
		else if (shape == StgIntersectionTarget::SHAPE_LINE) {
			//StgIntersectionTarget_Line::ptr tTarget = StgIntersectionTarget_Line::ptr::DownCast(target);
			++iLine;
		}
	}
}
void StgIntersectionObject::RegistIntersectionRelativeTarget(StgIntersectionManager* manager) {
	for (int iTarget = 0; iTarget < listRelativeTarget_.size(); ++iTarget) {
		StgIntersectionTarget::ptr target = listRelativeTarget_[iTarget];
		manager->AddTarget(target);
	}
}
int StgIntersectionObject::GetDxScriptObjectID() {
	int res = DxScript::ID_INVALID;
	StgEnemyObject* objEnemy = dynamic_cast<StgEnemyObject*>(this);
	if (objEnemy != NULL) {
		res = objEnemy->GetObjectID();
	}

	return res;
}

/**********************************************************
//StgIntersectionTarget
**********************************************************/
StgIntersectionTarget::StgIntersectionTarget() {
	//mortonNo_ = -1;
	ZeroMemory(&intersectionSpace_, sizeof(RECT));
}
void StgIntersectionTarget::ClearObjectIntersectedIdList() {
	if (obj_ != NULL) {
		obj_->ClearIntersectedIdList();
	}
}
std::wstring StgIntersectionTarget::GetInfoAsString() {
	std::wstring res;
	res += L"type[";
	switch (typeTarget_) {
	case TYPE_PLAYER:res += L"PLAYER"; break;
	case TYPE_PLAYER_SHOT:res += L"PLAYER_SHOT"; break;
	case TYPE_PLAYER_SPELL:res += L"PLAYER_SPELL"; break;
	case TYPE_ENEMY:res += L"ENEMY"; break;
	case TYPE_ENEMY_SHOT:res += L"ENEMY_SHOT"; break;
	}
	res += L"] ";

	res += L"shape[";
	switch (shape_) {
	case SHAPE_CIRCLE:res += L"CIRCLE"; break;
	case SHAPE_LINE:res += L"LINE"; break;
	}
	res += L"] ";

	res += StringUtility::Format(L"address[%08x] ", (int)this);

	res += L"obj[";
	if (obj_ == nullptr) {
		res += L"NULL";
	}
	else {
		ref_count_weak_ptr<DxScriptObjectBase>::unsync dxObj =
			ref_count_weak_ptr<DxScriptObjectBase>::unsync::DownCast(obj_);
		if (dxObj == nullptr)
			res += L"UNKNOWN";
		else {
			int address = (int)dxObj.GetPointer();
			char* className = (char*)typeid(*this).name();
			res += StringUtility::Format(L"ref=%d, " L"delete=%s, active=%s, class=%s[%08x]",
				dxObj.GetReferenceCount(),
				dxObj->IsDeleted() ? L"true" : L"false",
				dxObj->IsActive() ? L"true" : L"false",
				className, address);
		}
	}
	res += L"] ";

	return res;
}

