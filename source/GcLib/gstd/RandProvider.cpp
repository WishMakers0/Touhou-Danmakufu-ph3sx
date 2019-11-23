#include "RandProvider.hpp"

using namespace gstd;

inline uint32_t RandProvider::rotl(uint32_t u, int x) {
	return (u << x) | (u >> (32 - x));
}
inline uint32_t RandProvider::rotr(uint32_t u, int x) {
	return (u >> x) | (u << (32 - x));
}

RandProvider::RandProvider() {
	this->Initialize(0);
}
RandProvider::RandProvider(unsigned long s) {
	this->Initialize(s);
}

/*
uint32_t RandProvider::_GenrandInt32() {
	if (mtIndex_ >= MT_N) {
		for (int i = 0; i < MT_N; ++i) {
			int x = (mtStates_[i] & UPPER_MASK) + (mtStates_[(i + 1) % MT_N] & LOWER_MASK);
			int y = x >> 1;

			if ((x & 0x00000001) == 1)
				y = y ^ MT_A;
			mtStates_[i] = mtStates_[(i + MT_M) % MT_N] ^ y;
		}
		mtIndex_ = 0;
	}

	uint32_t result = mtStates_[mtIndex_++];

	result ^= (result >> MT_U);
	result ^= (result << MT_S) & MT_B;
	result ^= (result << MT_T) & MT_C;
	result ^= (result >> MT_L);

	return (result & MT_D);
}

void RandProvider::Initialize(unsigned long s) {
	mtIndex_ = MT_N;
	mtStates_[0] = s;

	for (int i = 1; i < MT_N; ++i) {
		mtStates_[i] = (MT_F * mtStates_[i - 1] ^ (mtStates_[i - 1] >> (MT_W - 2)) + i) & MT_D;
	}
	seed_ = s;
}
*/
void RandProvider::Initialize(unsigned long s) {
	static const uint32_t JUMP[] = { 0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b };

	states_[0] = s ^ JUMP[0];

	for (int i = 1; i < 4; ++i) {
		uint32_t s = 0x6c078965 * states_[i - 1] ^ (states_[i - 1] >> 30) + i;
		s ^= JUMP[i];
		states_[i] = s & 0xffffffff;
	}
}
uint32_t RandProvider::_GenrandInt32() {
	uint32_t result = rotl(states_[0] * 5, 7) * 9;

	uint32_t t = states_[1] << 9;

	states_[2] ^= states_[0];
	states_[3] ^= states_[1];
	states_[1] ^= states_[2];
	states_[0] ^= states_[3];

	states_[2] ^= t;

	states_[3] = rotl(states_[3], 11);

	return result;
}

long RandProvider::GetInt() {
	return (long)(_GenrandInt32() >> 1);
}
long RandProvider::GetInt(long min, long max) {
	return (int)this->GetReal(min, max);
}
int64_t RandProvider::GetInt64() {
	return (int64_t)this->GetReal();
}
int64_t RandProvider::GetInt64(int64_t min, int64_t max) {
	return (int64_t)this->GetReal(min, max);
}
double RandProvider::GetReal() {
	return (double)(_GenrandInt32() * (1.0 / (double)UINT32_MAX));
}
double RandProvider::GetReal(double a, double b) {
	if (a == b) return a;
	return (a + GetReal() * (b - a));
}