#ifndef __TOUHOUDANMAKUFU_DNHSTG_SHOT__
#define __TOUHOUDANMAKUFU_DNHSTG_SHOT__

#include "../../GcLib/pch.h"

#include "StgCommon.hpp"
#include "StgIntersection.hpp"

class StgShotDataList;
class StgShotData;
class StgShotRenderer;
class StgShotObject;
/**********************************************************
//StgShotManager
**********************************************************/
class StgShotManager {
	//uint16_t RTARGET_VERT_INDICES[6];
	//size_t vertRTw;
	//size_t vertRTh;
	friend class StgShotRenderer;
public:
	enum {
		DEL_TYPE_ALL,
		DEL_TYPE_SHOT,
		DEL_TYPE_CHILD,
		TO_TYPE_IMMEDIATE,
		TO_TYPE_FADE,
		TO_TYPE_ITEM,
	};

	enum {
		BIT_EV_DELETE_IMMEDIATE = 1,
		BIT_EV_DELETE_TO_ITEM,
		BIT_EV_DELETE_FADE,
		BIT_EV_DELETE_COUNT,
	};

	enum {
		SHOT_MAX = 8192,
	};
protected:
	StgStageController* stageController_;
	StgShotDataList* listPlayerShotData_;
	StgShotDataList* listEnemyShotData_;
	std::vector<ref_count_ptr<StgShotObject>::unsync> listObj_;

	std::bitset<BIT_EV_DELETE_COUNT> listDeleteEventEnable_;

	//IDirect3DTexture9* renderTexture_;
	//IDirect3DSurface9* renderSurface_;
	IDirect3DVertexBuffer9* vertexBuffer_;
	size_t vertexBufferSize_;

	ID3DXEffect* effectLayer_;
	D3DXHANDLE handleEffectWorld_;
public:
	StgShotManager(StgStageController* stageController);
	virtual ~StgShotManager();
	void Work();
	void Render(int targetPriority);
	void RegistIntersectionTarget();

	void AddShot(ref_count_ptr<StgShotObject>::unsync obj) { listObj_.push_back(obj); }

	StgShotDataList* GetPlayerShotDataList() { return listPlayerShotData_; }
	StgShotDataList* GetEnemyShotDataList() { return listEnemyShotData_; }

	bool LoadPlayerShotData(std::wstring path, bool bReload = false);
	bool LoadEnemyShotData(std::wstring path, bool bReload = false);

	RECT GetShotAutoDeleteClipRect();

	void DeleteInCircle(int typeDelete, int typeTo, int typeOnwer, int cx, int cy, double radius);
	std::vector<int> GetShotIdInCircle(int typeOnwer, int cx, int cy, int radius);
	size_t GetShotCount(int typeOnwer);
	size_t GetShotCountAll() { return listObj_.size(); }

	void GetValidRenderPriorityList(std::vector<PriListBool>& list);

	void SetDeleteEventEnableByType(int type, bool bEnable);
	bool IsDeleteEventEnable(int bit) {
		bool res = listDeleteEventEnable_[bit];
		return res;
	}

	size_t GetVertexBufferSize() { return vertexBufferSize_; }
	IDirect3DVertexBuffer9* GetVertexBuffer() { return vertexBuffer_; }
	void _SetVertexBuffer(size_t size);
};

/**********************************************************
//StgShotDataList
**********************************************************/
class StgShotDataList {
public:
	enum {
		RENDER_TYPE_COUNT = 8,
	};
private:
	std::set<std::wstring> listReadPath_;
	std::vector<ref_count_ptr<Texture>> listTexture_;
	std::vector<std::vector<StgShotRenderer*>> listRenderer_;
	std::vector<StgShotData*> listData_;

	D3DCOLOR defaultDelayColor_;

	void _ScanShot(std::vector<StgShotData*>& listData, Scanner& scanner);
	void _ScanAnimation(StgShotData*& shotData, Scanner& scanner);
	std::vector<std::wstring> _GetArgumentList(Scanner& scanner);
public:
	StgShotDataList();
	virtual ~StgShotDataList();

	int GetTextureCount() { return listTexture_.size(); }
	ref_count_ptr<Texture> GetTexture(int index) { return listTexture_[index]; }
	StgShotRenderer* GetRenderer(int index, int typeRender) { return listRenderer_[typeRender][index]; }
	std::vector<StgShotRenderer*>* GetRendererList(int typeRender) { return &listRenderer_[typeRender]; }

	StgShotData* GetData(int id) { return (id >= 0 && id < listData_.size()) ? listData_[id] : NULL; }

	bool AddShotDataList(std::wstring path, bool bReload);
};

class StgShotData {
	friend StgShotDataList;
public:
	struct AnimationData {
		RECT rcSrc_;
		RECT rcDst_;
		int frame_;
	};
private:
	StgShotDataList* listShotData_;
	int indexTexture_;
	int typeRender_;
	int typeDelayRender_;

	RECT rcDelay_;
	RECT rcDstDelay_;

	int alpha_;
	D3DCOLOR colorDelay_;
	DxCircle listCol_;

	std::vector<AnimationData> listAnime_;
	int totalAnimeFrame_;

	double angularVelocityMin_;
	double angularVelocityMax_;
	bool bFixedAngle_;

public:
	StgShotData(StgShotDataList* listShotData);
	virtual ~StgShotData();

	int GetTextureIndex() { return indexTexture_; }
	int GetRenderType() { return typeRender_; }
	int GetDelayRenderType() { return typeDelayRender_; }
	AnimationData* GetData(int frame);
	RECT& GetDelayRect() { return rcDelay_; }
	RECT& GetDelayDest() { return rcDstDelay_; }
	int GetAlpha() { return alpha_; }
	D3DCOLOR GetDelayColor() { return colorDelay_; }
	DxCircle* GetIntersectionCircleList() { return &listCol_; }
	double GetAngularVelocityMin() { return angularVelocityMin_; }
	double GetAngularVelocityMax() { return angularVelocityMax_; }
	bool IsFixedAngle() { return bFixedAngle_; }

	int GetFrameCount() { return listAnime_.size(); }

	ref_count_ptr<Texture> GetTexture();
	StgShotRenderer* GetRenderer();
	StgShotRenderer* GetRenderer(int type);

	static bool IsAlphaBlendValidType(int blendType);
};
#pragma region StgShotData_impl
inline ref_count_ptr<Texture> StgShotData::GetTexture() {
	return listShotData_->GetTexture(indexTexture_);
}
inline StgShotRenderer* StgShotData::GetRenderer() {
	return GetRenderer(typeRender_);
}
inline StgShotRenderer* StgShotData::GetRenderer(int blendType) {
	if (blendType < DirectGraphics::MODE_BLEND_ALPHA || blendType > DirectGraphics::MODE_BLEND_ALPHA_INV)
		return listShotData_->GetRenderer(indexTexture_, 0);
	return listShotData_->GetRenderer(indexTexture_, blendType - 1);
}
inline bool StgShotData::IsAlphaBlendValidType(int blendType) {
	switch (blendType) {
	case DirectGraphics::MODE_BLEND_ALPHA:
	case DirectGraphics::MODE_BLEND_ADD_ARGB:
	case DirectGraphics::MODE_BLEND_SUBTRACT:
		return true;
	default:
		return false;
	}
}
#pragma endregion StgShotData_impl

/**********************************************************
//StgShotRenderer
**********************************************************/
class StgShotRenderer : public RenderObjectTLX {
	friend class StgShotManager;

	size_t countMaxVertex_;
	size_t countRenderVertex_;
	void* tmp;
public:
	StgShotRenderer();
	~StgShotRenderer();

	virtual void Render(StgShotManager* manager);
	void AddVertex(VERTEX_TLX& vertex);
	void AddSquareVertex(VERTEX_TLX* listVertex) {
		AddVertex(listVertex[0]);
		AddVertex(listVertex[2]);
		AddVertex(listVertex[1]);
		AddVertex(listVertex[1]);
		AddVertex(listVertex[2]);
		AddVertex(listVertex[3]);
	}

	virtual size_t GetVertexCount() {
		return std::min(countRenderVertex_, vertex_.size() / strideVertexStreamZero_);
	}
	virtual void SetVertexCount(size_t count) {
		vertex_.SetSize(count * strideVertexStreamZero_);
	}
};

/**********************************************************
//StgShotObject
**********************************************************/
class StgShotObject : public DxScriptRenderObject, public StgMoveObject, public StgIntersectionObject {
public:
	enum {
		OWNER_PLAYER = 0,
		OWNER_ENEMY,
		OWNER_NULL,

		FRAME_FADEDELETE = 30,
		FRAME_FADEDELETE_LASER = 30,

		LIFE_SPELL_UNREGIST = 5,
		LIFE_SPELL_REGIST = 256 * 256 * 256,
	};
	class ReserveShotList;
	class ReserveShotListData;

protected:
	StgStageController* stageController_;

	int frameWork_;
	int idShotData_;
	int typeOwner_;

	double c_;
	double s_;
	float lastAngle_;

	float hitboxScaleX_;
	float hitboxScaleY_;

	//D3DCOLOR color_;
	int delay_;//遅延時間
	int typeSourceBrend_; //遅延時間ブレンド種別
	int frameGrazeInvalid_;//かすり無効フレーム
	int frameFadeDelete_;
	double damage_;
	double life_;//貫通力
	bool bAutoDelete_;
	bool bEraseShot_;//弾削除機能
	bool bSpellFactor_;//スペル付加
	int frameAutoDelete_;//自動削除フレーム
	ref_count_ptr<ReserveShotList>::unsync listReserveShot_;

	bool bUserIntersectionMode_;//ユーザ定義あたり判定モード
	bool bIntersectionEnable_;
	bool bChangeItemEnable_;

	StgShotData* _GetShotData();
	void _SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z = 1.0f, float w = 1.0f);
	void _SetVertexUV(VERTEX_TLX& vertex, float u, float v);
	void _SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color);
	virtual void _DeleteInLife();
	virtual void _DeleteInAutoClip();
	virtual void _DeleteInFadeDelete();
	virtual void _DeleteInAutoDeleteFrame();
	virtual void _Move();
	void _AddReservedShotWork();
	virtual void _AddReservedShot(ref_count_ptr<StgShotObject>::unsync obj, ReserveShotListData* data);
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision) {}
	virtual void _SendDeleteEvent(int bit);
public:
	StgShotObject(StgStageController* stageController);
	virtual ~StgShotObject();

	virtual void Work();
	virtual void Render() {}//一括で描画するためオブジェクト管理での描画はしない
	virtual void Activate() {}
	virtual void RenderOnShotManager() {}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);
	virtual void ClearShotObject() { ClearIntersectionRelativeTarget(); }
	virtual void RegistIntersectionTarget() = 0;

	virtual void SetX(double x) { posX_ = x; DxScriptRenderObject::SetX(x); }
	virtual void SetY(double y) { posY_ = y; DxScriptRenderObject::SetY(y); }
	virtual void SetColor(int r, int g, int b);
	virtual void SetAlpha(int alpha);
	virtual void SetRenderState() {}

	ref_count_ptr<StgShotObject>::unsync GetOwnObject();
	int GetShotDataID() { return idShotData_; }
	virtual void SetShotDataID(int id) { idShotData_ = id; }
	int GetOwnerType() { return typeOwner_; }
	void SetOwnerType(int type) { typeOwner_ = type; }

	bool IsValidGraze() { return frameGrazeInvalid_ <= 0; }
	int GetDelay() { return delay_; }
	void SetDelay(int delay) { delay_ = delay; }
	int GetSourceBlendType() { return typeSourceBrend_; }
	void SetSourceBlendType(int type) { typeSourceBrend_ = type; }
	double GetLife() { return life_; }
	void SetLife(double life) { life_ = life; }
	double GetDamage() { return damage_; }
	void SetDamage(double damage) { damage_ = damage; }
	virtual void SetFadeDelete() { if (frameFadeDelete_ < 0)frameFadeDelete_ = FRAME_FADEDELETE; }
	bool IsAutoDelete() { return bAutoDelete_; }
	void SetAutoDelete(bool b) { bAutoDelete_ = b; }
	void SetAutoDeleteFrame(int frame) { frameAutoDelete_ = frame; }
	bool IsEraseShot() { return bEraseShot_; }
	void SetEraseShot(bool bErase) { bEraseShot_ = bErase; }
	bool IsSpellFactor() { return bSpellFactor_; }
	void SetSpellFactor(bool bSpell) { bSpellFactor_ = bSpell; }
	void SetUserIntersectionMode(bool b) { bUserIntersectionMode_ = b; }
	void SetIntersectionEnable(bool b) { bIntersectionEnable_ = b; }
	bool IsIntersectionEnable() { return bIntersectionEnable_; }
	void SetItemChangeEnable(bool b) { bChangeItemEnable_ = b; }

	void SetHitboxScaleX(float x) { hitboxScaleX_ = x; }
	void SetHitboxScaleY(float y) { hitboxScaleY_ = y; }
	float GetHitboxScaleX() { return hitboxScaleX_; }
	float GetHitboxScaleY() { return hitboxScaleY_; }

	virtual void DeleteImmediate();
	virtual void ConvertToItem(bool flgPlayerCollision);
	void AddShot(int frame, int idShot, int radius, int angle);
};

class StgShotObject::ReserveShotListData {
	friend ReserveShotList;
	int idShot_;//対象ID
	double radius_;//出現位置距離
	double angle_;//出現位置角度
public:
	ReserveShotListData() { idShot_ = DxScript::ID_INVALID; radius_ = 0; angle_ = 0; }
	virtual ~ReserveShotListData() {}
	int GetShotID() { return idShot_; }
	double GetRadius() { return radius_; }
	double GetAngle() { return angle_; }
};

class StgShotObject::ReserveShotList {
public:
	class ListElement {
		std::list<ReserveShotListData> list_;
	public:
		ListElement() {}
		virtual ~ListElement() {}
		void Add(ReserveShotListData& data) { list_.push_back(data); }
		std::list<ReserveShotListData>* GetDataList() { return &list_; }
	};
private:
	int frame_;
	std::unordered_map<int, ref_count_ptr<ListElement>::unsync> mapData_;
public:
	ReserveShotList() { frame_ = 0; }
	virtual ~ReserveShotList() {}
	ref_count_ptr<ListElement>::unsync GetNextFrameData();
	void AddData(int frame, int idShot, int radius, int angle);
	void Clear(StgStageController* stageController);
};

/**********************************************************
//StgNormalShotObject
**********************************************************/
class StgNormalShotObject : public StgShotObject {
protected:
	double angularVelocity_;
	void _AddIntersectionRelativeTarget();
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgNormalShotObject(StgStageController* stageController);
	virtual ~StgNormalShotObject();
	virtual void Work();
	virtual void RenderOnShotManager();

	virtual void ClearShotObject() {
		ClearIntersectionRelativeTarget();
	}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);

	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}

	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();
	virtual void SetShotDataID(int id);
};

/**********************************************************
//StgLaserObject(レーザー基本部)
**********************************************************/
class StgLaserObject : public StgShotObject {
protected:
	int length_;
	int widthRender_;
	int widthIntersection_;
	float invalidLengthStart_;
	float invalidLengthEnd_;
	int frameGrazeInvalidStart_;
	double itemDistance_;
	void _AddIntersectionRelativeTarget();

public:
	StgLaserObject(StgStageController* stageController);
	virtual void ClearShotObject() {
		ClearIntersectionRelativeTarget();
	}
	virtual void Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget);

	int GetLength() { return length_; }
	void SetLength(int length) { length_ = length; }
	int GetRenderWidth() { return widthRender_; }
	void SetRenderWidth(int width) {
		widthRender_ = width;
		if (widthIntersection_ < 0)widthIntersection_ = width / 4;
	}
	int GetIntersectionWidth() { return widthIntersection_; }
	void SetIntersectionWidth(int width) { widthIntersection_ = width; }
	void SetInvalidLength(float start, float end) { invalidLengthStart_ = start; invalidLengthEnd_ = end; }
	void SetGrazeInvalidFrame(int frame) { frameGrazeInvalidStart_ = frame; }
	void SetItemDistance(double dist) { itemDistance_ = dist; }
};

/**********************************************************
//StgLooseLaserObject(射出型レーザー)
**********************************************************/
class StgLooseLaserObject : public StgLaserObject {
protected:
	double posXE_;//後方x
	double posYE_;//後方y

	virtual void _DeleteInAutoClip();
	virtual void _Move();
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgLooseLaserObject(StgStageController* stageController);
	virtual void Work();
	virtual void RenderOnShotManager();

	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}
	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();
	virtual void SetX(double x) { StgShotObject::SetX(x); posXE_ = x; }
	virtual void SetY(double y) { StgShotObject::SetY(y); posYE_ = y; }
};

/**********************************************************
//StgStraightLaserObject(設置型レーザー)
**********************************************************/
class StgStraightLaserObject : public StgLaserObject {
protected:
	double angLaser_;
	bool bUseSouce_;
	double scaleX_;

	bool bLaserExpand_;

	virtual void _DeleteInAutoClip();
	virtual void _DeleteInAutoDeleteFrame();
	virtual void _AddReservedShot(ref_count_ptr<StgShotObject>::unsync obj, ReserveShotListData* data);
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgStraightLaserObject(StgStageController* stageController);
	virtual void Work();
	virtual void RenderOnShotManager();
	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}
	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();

	double GetLaserAngle() { return angLaser_; }
	void SetLaserAngle(double angle) { angLaser_ = angle; }
	void SetFadeDelete() { if (frameFadeDelete_ < 0)frameFadeDelete_ = FRAME_FADEDELETE_LASER; }
	void SetSourceEnable(bool bEnable) { bUseSouce_ = bEnable; }

	void SetLaserExpand(bool b) { bLaserExpand_ = b; }
	bool GetLaserExpand() { return bLaserExpand_; }
};

/**********************************************************
//StgCurveLaserObject(曲がる型レーザー)
**********************************************************/
class StgCurveLaserObject : public StgLaserObject {
protected:
	struct Position {
		double x;
		double y;
	};
	struct LaserNode {
		Position pos;
		Position vertOff[2];
	};

	std::list<LaserNode> listPosition_;
	double tipDecrement_;

	virtual void _DeleteInAutoClip();
	virtual void _Move();
	virtual void _ConvertToItemAndSendEvent(bool flgPlayerCollision);
public:
	StgCurveLaserObject(StgStageController* stageController);
	virtual void Work();
	virtual void RenderOnShotManager();
	virtual void RegistIntersectionTarget() {
		if (!bUserIntersectionMode_) _AddIntersectionRelativeTarget();
	}
	virtual std::vector<StgIntersectionTarget::ptr> GetIntersectionTargetList();
	void SetTipDecrement(double dec) { tipDecrement_ = dec; }
};




inline bool StgShotManager::LoadPlayerShotData(std::wstring path, bool bReload) {
	return listPlayerShotData_->AddShotDataList(path, bReload);
}
inline bool StgShotManager::LoadEnemyShotData(std::wstring path, bool bReload) {
	return listEnemyShotData_->AddShotDataList(path, bReload);
}

#pragma region StgShotObject_impl
inline void StgShotObject::_SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z, float w) {
	constexpr float bias = -0.5f;
	vertex.position.x = x + bias;
	vertex.position.y = y + bias;
	vertex.position.z = z;
	vertex.position.w = w;
}
inline void StgShotObject::_SetVertexUV(VERTEX_TLX& vertex, float u, float v) {
	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)return;

	ref_count_ptr<Texture> texture = shotData->GetTexture();
	int width = texture->GetWidth();
	int height = texture->GetHeight();
	vertex.texcoord.x = u / width;
	vertex.texcoord.y = v / height;
}
inline void StgShotObject::_SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color) {
	vertex.diffuse_color = color;
}
inline void StgShotObject::SetAlpha(int alpha) {
	color_ = ColorAccess::SetColorA(color_, alpha);
}
inline void StgShotObject::SetColor(int r, int g, int b) {
	color_ = ColorAccess::SetColorR(color_, r);
	color_ = ColorAccess::SetColorG(color_, g);
	color_ = ColorAccess::SetColorB(color_, b);
}
#pragma endregion StgShotObject_impl

#endif