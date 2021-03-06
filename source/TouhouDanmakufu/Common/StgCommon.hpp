#pragma once

#include "../../GcLib/pch.h"

#include "DnhCommon.hpp"
#include "DnhGcLibImpl.hpp"
#include "DnhReplay.hpp"
#include "DnhScript.hpp"

class StgSystemController;
class StgSystemInformation;
class StgStageController;
class StgPackageController;
class StgStageInformation;
class StgSystemInformation;
class StgMovePattern;

/**********************************************************
//StgMoveObject
**********************************************************/
class StgMoveObject {
	friend StgMovePattern;
private:
	StgStageController* stageController_;
protected:
	double posX_;
	double posY_;
	std::shared_ptr<StgMovePattern> pattern_;

	int framePattern_;
	std::map<int, std::shared_ptr<StgMovePattern>> mapPattern_;
	virtual void _Move();
	void _AttachReservedPattern(std::shared_ptr<StgMovePattern> pattern);

public:
	StgMoveObject(StgStageController* stageController);
	virtual ~StgMoveObject();

	double GetPositionX() { return posX_; }
	void SetPositionX(double pos) { posX_ = pos; }
	double GetPositionY() { return posY_; }
	void SetPositionY(double pos) { posY_ = pos; }

	double GetSpeed();
	void SetSpeed(double speed);
	double GetDirectionAngle();
	void SetDirectionAngle(double angle);

	void SetSpeedX(double speedX);
	void SetSpeedY(double speedY);

	std::shared_ptr<StgMovePattern> GetPattern() { return pattern_; }
	void SetPattern(std::shared_ptr<StgMovePattern> pattern) {
		pattern_ = pattern;
	}
	void AddPattern(int frameDelay, std::shared_ptr<StgMovePattern> pattern);
};

/**********************************************************
//StgMovePattern
**********************************************************/
class StgMovePattern {
	friend StgMoveObject;
public:
	enum {
		TYPE_OTHER,
		TYPE_ANGLE,
		TYPE_XY,
		TYPE_LINE,

		NO_CHANGE = -256 * 256 * 256,
	};

protected:
	int typeMove_;
	StgMoveObject* target_;

	int frameWork_;//アクティブになるフレーム。
	int idShotData_;//弾画像ID(弾オブジェクト専用)

	double c_;
	double s_;
	double dist_;

	StgStageController* _GetStageController() { return target_->stageController_; }
	shared_ptr<StgMoveObject> _GetMoveObject(int id);
	virtual void _Activate() {}
public:
	StgMovePattern(StgMoveObject* target);
	virtual ~StgMovePattern() {}
	virtual void Move() = 0;

	int GetType() { return typeMove_; }

	virtual inline double GetSpeed() = 0;
	virtual inline double GetDirectionAngle() = 0;
	int GetShotDataID() { return idShotData_; }
	void SetShotDataID(int id) { idShotData_ = id; }

	virtual double GetSpeedX() { return c_; }
	virtual double GetSpeedY() { return s_; }
};

class StgMovePattern_Angle : public StgMovePattern {
protected:
	double speed_;
	double angDirection_;
	double acceleration_;
	double maxSpeed_;
	double angularVelocity_;
	int idRalativeID_;

	virtual void _Activate();
public:
	StgMovePattern_Angle(StgMoveObject* target);
	virtual void Move();

	virtual inline double GetSpeed() { return speed_; }
	virtual inline double GetDirectionAngle() { return angDirection_; }

	void SetSpeed(double speed) { speed_ = speed; }
	void SetDirectionAngle(double angle);
	void SetAcceleration(double accel) { acceleration_ = accel; }
	void SetMaxSpeed(double max) { maxSpeed_ = max; }
	void SetAngularVelocity(double av) { angularVelocity_ = av; }
	void SetRelativeObjectID(int id) { idRalativeID_ = id; }

	virtual inline double GetSpeedX() {
		return (speed_ * c_);
	}
	virtual inline double GetSpeedY() {
		return (speed_ * s_);
	}
};

class StgMovePattern_XY : public StgMovePattern {
protected:
	double accelerationX_;
	double accelerationY_;
	double maxSpeedX_;
	double maxSpeedY_;

public:
	StgMovePattern_XY(StgMoveObject* target);
	virtual void Move();

	virtual inline double GetSpeed() { return sqrt(c_ * c_ + s_ * s_); }
	virtual inline double GetDirectionAngle() { return atan2(s_, c_); }

	virtual double GetSpeedX() { return c_; }
	virtual double GetSpeedY() { return s_; }
	void SetSpeedX(double value) { c_ = value; }
	void SetSpeedY(double value) { s_ = value; }
	void SetAccelerationX(double value) { accelerationX_ = value; }
	void SetAccelerationY(double value) { accelerationY_ = value; }
	void SetMaxSpeedX(double value) { maxSpeedX_ = value; }
	void SetMaxSpeedY(double value) { maxSpeedY_ = value; }
};

class StgMovePattern_Line : public StgMovePattern {
protected:
	enum {
		TYPE_SPEED,
		TYPE_FRAME,
		TYPE_WEIGHT,
		TYPE_NONE,
	};

	int typeLine_;
	double speed_;
	double angDirection_;
	double weight_;
	double maxSpeed_;
	int frameStop_;
	double toX_;
	double toY_;
public:
	StgMovePattern_Line(StgMoveObject* target);
	virtual void Move();
	virtual inline double GetSpeed() { return speed_; }
	virtual inline double GetDirectionAngle() { return angDirection_; }

	void SetAtSpeed(double tx, double ty, double speed);
	void SetAtFrame(double tx, double ty, double frame);
	void SetAtWait(double tx, double ty, double weight, double maxSpeed);

	virtual double GetSpeedX() { return c_; }
	virtual double GetSpeedY() { return s_; }
};
