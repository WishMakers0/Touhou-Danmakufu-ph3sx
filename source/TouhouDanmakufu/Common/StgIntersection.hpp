#ifndef __TOUHOUDANMAKUFU_DNHSTG_INTERSECTION__
#define __TOUHOUDANMAKUFU_DNHSTG_INTERSECTION__

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"

class StgIntersectionManager;
class StgIntersectionSpace;
class StgIntersectionCheckList;

class StgIntersectionObject;

/**********************************************************
//StgIntersectionTarget
**********************************************************/
class StgIntersectionTarget : public IStringInfo {
	friend StgIntersectionManager;
public:
	enum {
		SHAPE_CIRCLE = 0,
		SHAPE_LINE = 1,

		TYPE_PLAYER,
		TYPE_PLAYER_SHOT,
		TYPE_PLAYER_SPELL,
		TYPE_ENEMY,
		TYPE_ENEMY_SHOT,
	};
protected:
	//int mortonNo_;
	int typeTarget_;
	int shape_;
	weak_ptr<StgIntersectionObject> obj_;

	RECT intersectionSpace_;
public:
	using ptr = std::shared_ptr<StgIntersectionTarget>;

	StgIntersectionTarget();
	virtual ~StgIntersectionTarget() {}

	RECT& GetIntersectionSpaceRect() { return intersectionSpace_; }
	virtual void SetIntersectionSpace() = 0;

	int GetTargetType() { return typeTarget_; }
	void SetTargetType(int type) { typeTarget_ = type; }
	int GetShape() { return shape_; }
	weak_ptr<StgIntersectionObject> GetObject() { return obj_; }
	void SetObject(weak_ptr<StgIntersectionObject> obj) {
		if (!obj.expired()) obj_ = obj;
	}

	//int GetMortonNumber() { return mortonNo_; }
	//void SetMortonNumber(int no) { mortonNo_ = no; }
	void ClearObjectIntersectedIdList();

	virtual std::wstring GetInfoAsString();
};

class StgIntersectionTarget_Circle : public StgIntersectionTarget {
	friend StgIntersectionManager;
	DxCircle circle_;
public:
	using ptr = std::shared_ptr<StgIntersectionTarget_Circle>;

	StgIntersectionTarget_Circle() { shape_ = SHAPE_CIRCLE; }
	virtual ~StgIntersectionTarget_Circle() {}

	virtual void SetIntersectionSpace() {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		LONG screenWidth = graphics->GetScreenWidth();
		LONG screenHeight = graphics->GetScreenWidth();

		constexpr LONG margin = 16L;
		double x = circle_.GetX();
		double y = circle_.GetY();
		double r = circle_.GetR();

		intersectionSpace_ = { (int)(x - r), (int)(y - r), (int)(x + r), (int)(y + r) };
		intersectionSpace_.left = std::max(intersectionSpace_.left, -margin);
		intersectionSpace_.left = std::min(intersectionSpace_.left, screenWidth + margin);
		intersectionSpace_.top = std::max(intersectionSpace_.top, -margin);
		intersectionSpace_.top = std::min(intersectionSpace_.top, screenHeight + margin);

		intersectionSpace_.right = std::max(intersectionSpace_.right, -margin);
		intersectionSpace_.right = std::min(intersectionSpace_.right, screenWidth + margin);
		intersectionSpace_.bottom = std::max(intersectionSpace_.bottom, -margin);
		intersectionSpace_.bottom = std::min(intersectionSpace_.bottom, screenHeight + margin);
	}

	DxCircle& GetCircle() { return circle_; }
	void SetCircle(DxCircle& circle) { 
		circle_ = circle;
		SetIntersectionSpace();
	}
};

class StgIntersectionTarget_Line : public StgIntersectionTarget {
	friend StgIntersectionManager;
	DxWidthLine line_;
public:
	using ptr = std::shared_ptr<StgIntersectionTarget_Line>;

	StgIntersectionTarget_Line() { shape_ = SHAPE_LINE; }
	virtual ~StgIntersectionTarget_Line() {}

	virtual void SetIntersectionSpace() {
		double x1 = line_.GetX1();
		double y1 = line_.GetY1();
		double x2 = line_.GetX2();
		double y2 = line_.GetY2();
		double width = line_.GetWidth();
		if (x1 > x2) {
			double tx = x1;
			x1 = x2;
			x2 = tx;
		}
		if (y1 > y2) {
			double ty = y1;
			y1 = y2;
			y2 = ty;
		}

		x1 -= width;
		x2 += width;
		y1 -= width;
		y2 += width;

		DirectGraphics* graphics = DirectGraphics::GetBase();

		constexpr double margin = 16.0;
		double screenWidth = graphics->GetScreenWidth();
		double screenHeight = graphics->GetScreenWidth();

		x1 = std::min(x1, screenWidth + margin);
		x1 = std::max(x1, -margin);
		x2 = std::min(x2, screenWidth + margin);
		x2 = std::max(x2, -margin);

		y1 = std::min(y1, screenHeight + margin);
		y1 = std::max(y1, -margin);
		y2 = std::min(y2, screenHeight + margin);
		y2 = std::max(y2, -margin);

		//RECT rect = {x1 - width, y1 - width, x2 + width, y2 + width};
		intersectionSpace_ = { (int)x1, (int)y1, (int)x2, (int)y2 };
	}

	DxWidthLine& GetLine() { return line_; }
	void SetLine(DxWidthLine& line) { 
		line_ = line; 
		SetIntersectionSpace();
	}
};

class StgIntersectionTargetPoint;

/**********************************************************
//StgIntersectionManager
//下記を参考
//http://marupeke296.com/COL_2D_No8_QuadTree.html
**********************************************************/
class StgIntersectionManager {
private:
	enum {
		SPACE_PLAYER_ENEMY = 0,//自機-敵、敵弾
		SPACE_PLAYERSOHT_ENEMY,//自弾,スペル-敵
		SPACE_PLAYERSHOT_ENEMYSHOT,//自弾,スペル-敵弾
	};
	std::vector<StgIntersectionSpace*> listSpace_;
	std::vector<StgIntersectionTargetPoint> listEnemyTargetPoint_;
	std::vector<StgIntersectionTargetPoint> listEnemyTargetPointNext_;

	omp_lock_t lock_;
public:
	StgIntersectionManager();
	virtual ~StgIntersectionManager();
	void Work();

	void AddTarget(StgIntersectionTarget::ptr target);
	void AddEnemyTargetToShot(StgIntersectionTarget::ptr target);
	void AddEnemyTargetToPlayer(StgIntersectionTarget::ptr target);
	std::vector<StgIntersectionTargetPoint>* GetAllEnemyTargetPoint() { return &listEnemyTargetPoint_; }

	//void CheckDeletedObject(std::string funcName);

	static bool IsIntersected(StgIntersectionTarget::ptr& target1, StgIntersectionTarget::ptr& target2);

	omp_lock_t* GetLock() { return &lock_; }
};

/**********************************************************
//StgIntersectionSpace
//以下サイトを参考
//　○×（まるぺけ）つくろーどっとコム
//　http://marupeke296.com/
**********************************************************/
/*
class StgIntersectionSpace {
	enum {
		MAX_LEVEL = 9,
		TYPE_A = 0,
		TYPE_B = 1,
	};
protected:
	//Cell TARGETA/B listTarget
	std::vector<std::vector<std::vector<StgIntersectionTarget*>>> listCell_;
	int listCountLevel_[MAX_LEVEL + 1];	// 各レベルのセル数
	double spaceWidth_; // 領域のX軸幅
	double spaceHeight_; // 領域のY軸幅
	double spaceLeft_; // 領域の左側（X軸最小値）
	double spaceTop_; // 領域の上側（Y軸最小値）
	double unitWidth_; // 最小レベル空間の幅単位
	double unitHeight_; // 最小レベル空間の高単位
	int countCell_; // 空間の数
	int unitLevel_; // 最下位レベル
	StgIntersectionCheckList* listCheck_;

	unsigned int _GetMortonNumber(float left, float top, float right, float bottom);
	unsigned int  _BitSeparate32(unsigned int  n);
	unsigned short _Get2DMortonNumber(unsigned short x, unsigned short y);
	unsigned int  _GetPointElem(float pos_x, float pos_y);
	void _WriteIntersectionCheckList(int indexSpace, StgIntersectionCheckList*& listCheck, 
		std::vector<std::vector<StgIntersectionTarget*>> &listStack);
public:
	StgIntersectionSpace();
	virtual ~StgIntersectionSpace();
	bool Initialize(int level, int left, int top, int right, int bottom);
	bool RegistTarget(int type, StgIntersectionTarget*& target);
	bool RegistTargetA(StgIntersectionTarget*& target) { return RegistTarget(TYPE_A, target); }
	bool RegistTargetB(StgIntersectionTarget*& target) { return RegistTarget(TYPE_B, target); }
	void ClearTarget();
	StgIntersectionCheckList* CreateIntersectionCheckList();
};
*/

class StgIntersectionCheckList {
	size_t count_;
	//std::vector<StgIntersectionTarget*> listTargetA_;
	//std::vector<StgIntersectionTarget*> listTargetB_;
	std::vector<std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr>> listTargetPair_;
public:
	StgIntersectionCheckList() { count_ = 0; }
	virtual ~StgIntersectionCheckList() {}

	void Clear() { count_ = 0; }
	size_t GetCheckCount() { return count_; }
	void Add(StgIntersectionTarget::ptr& targetA, StgIntersectionTarget::ptr& targetB) {
		std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr> pair = { targetA, targetB };
		if (listTargetPair_.size() <= count_) {
			listTargetPair_.push_back(pair);
		}
		else {
			listTargetPair_[count_] = pair;
		}
		++count_;
	}
	StgIntersectionTarget::ptr GetTargetA(int index) {
		std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr> pair = listTargetPair_[index];
		listTargetPair_[index].first = nullptr;
		return pair.first; 
	}
	StgIntersectionTarget::ptr GetTargetB(int index) {
		std::pair<StgIntersectionTarget::ptr, StgIntersectionTarget::ptr> pair = listTargetPair_[index];
		listTargetPair_[index].second = nullptr;
		return pair.second;
	}
};

class StgIntersectionSpace {
	enum {
		TYPE_A = 0,
		TYPE_B = 1,
	};
protected:
	std::vector<std::vector<StgIntersectionTarget::ptr>> listCell_;

	double spaceWidth_; // 領域のX軸幅
	double spaceHeight_; // 領域のY軸幅
	double spaceLeft_; // 領域の左側（X軸最小値）
	double spaceTop_; // 領域の上側（Y軸最小値）

	StgIntersectionCheckList* listCheck_;

	size_t _WriteIntersectionCheckList(StgIntersectionManager* manager, StgIntersectionCheckList*& listCheck);
//		std::vector<std::vector<StgIntersectionTarget*>> &listStack);
public:
	StgIntersectionSpace();
	virtual ~StgIntersectionSpace();
	bool Initialize(int left, int top, int right, int bottom);
	bool RegistTarget(int type, StgIntersectionTarget::ptr& target);
	bool RegistTargetA(StgIntersectionTarget::ptr& target) { return RegistTarget(TYPE_A, target); }
	bool RegistTargetB(StgIntersectionTarget::ptr& target) { return RegistTarget(TYPE_B, target); }
	void ClearTarget() {
		listCell_[0].clear();
		listCell_[1].clear();
	}
	StgIntersectionCheckList* CreateIntersectionCheckList(StgIntersectionManager* manager, size_t& total) {
		StgIntersectionCheckList* res = listCheck_;
		res->Clear();

		total += _WriteIntersectionCheckList(manager, res);
		return res;
	}
};

class StgIntersectionObject {
protected:
	bool bIntersected_;//衝突判定
	size_t intersectedCount_;
	std::vector<StgIntersectionTarget::ptr> listRelativeTarget_;
	std::vector<DxCircle> listOrgCircle_;
	std::vector<DxWidthLine> listOrgLine_;
	std::vector<int> listIntersectedID_;

public:
	StgIntersectionObject() { bIntersected_ = false; intersectedCount_ = 0; }
	virtual ~StgIntersectionObject() {}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) = 0;
	void ClearIntersected() { bIntersected_ = false; intersectedCount_ = 0; }
	bool IsIntersected() { return bIntersected_; }
	void SetIntersected() { bIntersected_ = true; intersectedCount_++; }
	int GetIntersectedCount() { return intersectedCount_; }
	void ClearIntersectedIdList() { if (listIntersectedID_.size() > 0)listIntersectedID_.clear(); }
	void AddIntersectedId(int id) { listIntersectedID_.push_back(id); }
	std::vector<int>& GetIntersectedIdList() { return listIntersectedID_; }

	void ClearIntersectionRelativeTarget();
	void AddIntersectionRelativeTarget(StgIntersectionTarget::ptr target);
	StgIntersectionTarget::ptr GetIntersectionRelativeTarget(size_t index) { return listRelativeTarget_[index]; }

	void UpdateIntersectionRelativeTarget(int posX, int posY, double angle);
	void RegistIntersectionRelativeTarget(StgIntersectionManager* manager);
	int GetIntersectionRelativeTargetCount() { return listRelativeTarget_.size(); }
	int GetDxScriptObjectID();

	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList() { 
		return std::vector<StgIntersectionTarget::ptr>();
	}
};

inline void StgIntersectionTarget::ClearObjectIntersectedIdList() { 
	if (auto ptr = obj_.lock()) 
		ptr->ClearIntersectedIdList();
}

/**********************************************************
//StgIntersectionTargetPoint
**********************************************************/
class StgEnemyObject;
class StgIntersectionTargetPoint {
private:
	POINT pos_;
	weak_ptr<StgEnemyObject> ptrObject_;
public:
	POINT& GetPoint() { return pos_; }
	void SetPoint(POINT& pos) { pos_ = pos; }
	weak_ptr<StgEnemyObject> GetObjectRef() { return ptrObject_; }
	void SetObjectRef(weak_ptr<StgEnemyObject> id) { ptrObject_ = id; }
};

#endif

