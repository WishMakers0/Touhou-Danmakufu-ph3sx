
#include"StgCommon.hpp"
#include"StgSystem.hpp"

/**********************************************************
//StgMoveObject
**********************************************************/
StgMoveObject::StgMoveObject(StgStageController* stageController) {
	posX_ = 0;
	posY_ = 0;
	framePattern_ = 0;
	stageController_ = stageController;

	pattern_ = nullptr;
}
StgMoveObject::~StgMoveObject() {
	for (auto& itr : mapPattern_) {
		StgMovePattern*& pattern = itr.second;
		if (pattern) delete pattern;
		pattern = nullptr;
	}
}
void StgMoveObject::_Move() {
	if (pattern_ == nullptr)return;

	if (mapPattern_.size() > 0) {
		std::map<int, StgMovePattern*>::iterator itr = mapPattern_.begin();
		int frame = itr->first;
		if (frame == framePattern_) {
			StgMovePattern* pattern = itr->second;
			_AttachReservedPattern(pattern);
			mapPattern_.erase(frame);
		}
	}

	pattern_->Move();
	++framePattern_;
}
void StgMoveObject::_AttachReservedPattern(StgMovePattern* pattern) {
	//‘¬“xŒp‘±‚È‚Ç
	if (pattern_ == nullptr)
		pattern_ = new StgMovePattern_Angle(this);

	int newMoveType = pattern->GetType();
	if (newMoveType == StgMovePattern::TYPE_ANGLE) {
		StgMovePattern_Angle* angPattern = dynamic_cast<StgMovePattern_Angle*>(pattern);
		if (angPattern->GetSpeed() == StgMovePattern::NO_CHANGE)
			angPattern->SetSpeed(pattern_->GetSpeed());
		if (angPattern->GetDirectionAngle() == StgMovePattern::NO_CHANGE)
			angPattern->SetDirectionAngle(pattern_->GetDirectionAngle());
	}
	else if (newMoveType == StgMovePattern::TYPE_XY) {
		StgMovePattern_XY* xyPattern = dynamic_cast<StgMovePattern_XY*>(pattern);

		double speed = pattern_->GetSpeed();
		double angle = pattern_->GetDirectionAngle();
		double speedX = speed * cos(angle);
		double speedY = speed * sin(angle);

		if (xyPattern->GetSpeedX() == StgMovePattern::NO_CHANGE)
			xyPattern->SetSpeedX(speedX);
		if (xyPattern->GetSpeedY() == StgMovePattern::NO_CHANGE)
			xyPattern->SetSpeedY(speedY);
	}

	//’u‚«Š·‚¦
	pattern_ = pattern;
}
double StgMoveObject::GetSpeed() {
	if (pattern_ == nullptr)return 0;
	double res = pattern_->GetSpeed();
	return res;
}
void StgMoveObject::SetSpeed(double speed) {
	if (pattern_ == NULL || pattern_->GetType() != StgMovePattern::TYPE_ANGLE) {
		pattern_ = new StgMovePattern_Angle(this);
	}
	StgMovePattern_Angle* pattern = dynamic_cast<StgMovePattern_Angle*>(pattern_);
	pattern->SetSpeed(speed);
}
double StgMoveObject::GetDirectionAngle() {
	if (pattern_ == nullptr)return 0;
	double res = pattern_->GetDirectionAngle();
	return res;
}
void StgMoveObject::SetDirectionAngle(double angle) {
	if (pattern_ == NULL || pattern_->GetType() != StgMovePattern::TYPE_ANGLE) {
		pattern_ = new StgMovePattern_Angle(this);
	}
	StgMovePattern_Angle* pattern = dynamic_cast<StgMovePattern_Angle*>(pattern_);
	pattern->SetDirectionAngle(angle);
}
void StgMoveObject::AddPattern(int frameDelay, StgMovePattern* pattern) {
	if (frameDelay == 0)
		_AttachReservedPattern(pattern);
	else {
		int frame = frameDelay + framePattern_;
		mapPattern_[frame] = pattern;
	}
}

/**********************************************************
//StgMovePattern
**********************************************************/
//StgMovePattern
StgMovePattern::StgMovePattern(StgMoveObject* target) {
	target_ = target;
	idShotData_ = NO_CHANGE;
	frameWork_ = 0;
	typeMove_ = TYPE_OTHER;
	c_ = 1;
	s_ = 0;
}
ref_count_ptr<StgMoveObject>::unsync StgMovePattern::_GetMoveObject(int id) {
	StgStageController* controller = _GetStageController();
	ref_count_ptr<DxScriptObjectBase>::unsync base = controller->GetMainRenderObject(id);
	if (base == NULL || base->IsDeleted())return NULL;

	return ref_count_ptr<StgMoveObject>::unsync::DownCast(base);
}

//StgMovePattern_Angle
StgMovePattern_Angle::StgMovePattern_Angle(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_ANGLE;
	speed_ = 0;
	angDirection_ = 0;
	acceleration_ = 0;
	maxSpeed_ = 0;
	angularVelocity_ = 0;
	idRalativeID_ = DxScript::ID_INVALID;
}
void StgMovePattern_Angle::Move() {
	if (frameWork_ == 0)
		_Activate();
	double angle = angDirection_;

	if (acceleration_ != 0) {
		speed_ += acceleration_;
		if (acceleration_ > 0)
			speed_ = min(speed_, maxSpeed_);
		if (acceleration_ < 0)
			speed_ = max(speed_, maxSpeed_);
	}
	if (angularVelocity_ != 0) {
		SetDirectionAngle(angle + angularVelocity_);
	}

	double sx = speed_ * c_;
	double sy = speed_ * s_;
	double px = target_->GetPositionX() + sx;
	double py = target_->GetPositionY() + sy;

	target_->SetPositionX(px);
	target_->SetPositionY(py);

	frameWork_++;
}
void StgMovePattern_Angle::_Activate() {
	if (idRalativeID_ != DxScript::ID_INVALID) {
		ref_count_ptr<StgMoveObject>::unsync obj = _GetMoveObject(idRalativeID_);
		if (obj != nullptr) {
			double px = target_->GetPositionX();
			double py = target_->GetPositionY();
			double tx = obj->GetPositionX();
			double ty = obj->GetPositionY();
			double angle = atan2(ty - py, tx - px);

			SetDirectionAngle(angle);
		}
	}
}
void StgMovePattern_Angle::SetDirectionAngle(double angle) {
	c_ = cos(angle);
	s_ = sin(angle);
	angDirection_ = angle;
}
double StgMovePattern_Angle::GetSpeedX() {
	return (speed_ * c_);
}
double StgMovePattern_Angle::GetSpeedY() {
	return (speed_ * s_);
}

//StgMovePattern_XY
StgMovePattern_XY::StgMovePattern_XY(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_XY;
	c_ = 0;
	s_ = 0;
	accelerationX_ = 0;
	accelerationY_ = 0;
	maxSpeedX_ = INT_MAX;
	maxSpeedY_ = INT_MAX;
}
void StgMovePattern_XY::Move() {
	if (frameWork_ == 0)
		_Activate();

	if (accelerationX_ != 0) {
		c_ += accelerationX_;
		if (accelerationX_ > 0)
			c_ = min(c_, maxSpeedX_);
		if (accelerationX_ < 0)
			c_ = max(c_, maxSpeedX_);
	}
	if (accelerationY_ != 0) {
		s_ += accelerationY_;
		if (accelerationY_ > 0)
			s_ = min(s_, maxSpeedY_);
		if (accelerationY_ < 0)
			s_ = max(s_, maxSpeedY_);
	}

	double px = target_->GetPositionX() + c_;
	double py = target_->GetPositionY() + s_;

	target_->SetPositionX(px);
	target_->SetPositionY(py);

	frameWork_++;
}
double StgMovePattern_XY::GetSpeed() {
	double res = sqrt(c_ * c_ + s_ * s_);
	return res;
}
double StgMovePattern_XY::GetDirectionAngle() {
	double res = atan2(s_, c_);
	return res;
}

//StgMovePattern_Line
StgMovePattern_Line::StgMovePattern_Line(StgMoveObject* target) : StgMovePattern(target) {
	typeMove_ = TYPE_NONE;
	speed_ = 0;
	angDirection_ = 0;
	weight_ = 0;
	maxSpeed_ = 0;
	frameStop_ = 0;
	c_ = 0;
	s_ = 0;
}
void StgMovePattern_Line::Move() {
	if (typeLine_ == TYPE_SPEED || typeLine_ == TYPE_FRAME) {
		double sx = speed_ * c_;
		double sy = speed_ * s_;
		double px = target_->GetPositionX() + sx;
		double py = target_->GetPositionY() + sy;

		target_->SetPositionX(px);
		target_->SetPositionY(py);
		frameStop_--;
		if (frameStop_ <= 0) {
			typeLine_ = TYPE_NONE;
			speed_ = 0;
		}
	}
	else if (typeLine_ == TYPE_WEIGHT) {
		double nx = target_->GetPositionX();
		double ny = target_->GetPositionY();
		if (dist_ < 1) {
			typeLine_ = TYPE_NONE;
			speed_ = 0;
		}
		else {
			speed_ = dist_ / weight_;
			if (speed_ > maxSpeed_)
				speed_ = maxSpeed_;
			double px = target_->GetPositionX() + speed_ * c_;
			double py = target_->GetPositionY() + speed_ * s_;
			target_->SetPositionX(px);
			target_->SetPositionY(py);

			dist_ -= speed_;
		}
	}
}
void StgMovePattern_Line::SetAtSpeed(double tx, double ty, double speed) {
	typeLine_ = TYPE_SPEED;
	toX_ = tx;
	toY_ = ty;
	double nx = tx - target_->GetPositionX();
	double ny = ty - target_->GetPositionY();
	dist_ = sqrt(nx * nx + ny * ny);
	speed_ = speed;
	angDirection_ = atan2(ny, nx);
	frameStop_ = dist_ / speed;

	c_ = cos(angDirection_);
	s_ = sin(angDirection_);
}
void StgMovePattern_Line::SetAtFrame(double tx, double ty, double frame) {
	typeLine_ = TYPE_FRAME;
	toX_ = tx;
	toY_ = ty;
	double nx = tx - target_->GetPositionX();
	double ny = ty - target_->GetPositionY();
	dist_ = sqrt(nx * nx + ny * ny);
	speed_ = dist_ / frame;
	angDirection_ = atan2(ny, nx);
	frameStop_ = frame;

	c_ = cos(angDirection_);
	s_ = sin(angDirection_);
}
void StgMovePattern_Line::SetAtWait(double tx, double ty, double weight, double maxSpeed) {
	typeLine_ = TYPE_WEIGHT;
	toX_ = tx;
	toY_ = ty;
	weight_ = weight;
	maxSpeed_ = maxSpeed;
	double nx = tx - target_->GetPositionX();
	double ny = ty - target_->GetPositionY();
	dist_ = sqrt(nx * nx + ny * ny);
	speed_ = maxSpeed_;
	angDirection_ = atan2(ny, nx);

	c_ = cos(angDirection_);
	s_ = sin(angDirection_);
}
