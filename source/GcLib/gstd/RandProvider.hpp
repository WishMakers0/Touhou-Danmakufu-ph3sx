#ifndef __GSTD_RANDPROVIDER__
#define __GSTD_RANDPROVIDER__

#include"GstdConstant.hpp"

namespace gstd {
	/*
	class RandProvider {
		enum {
			MT_W = 32,
			MT_N = 624,
			MT_M = 397,
			MT_R = 31,

			MT_A = 0x9908B0DF,

			MT_U = 11,
			MT_D = 0xFFFFFFFF,

			MT_S = 7,
			MT_B = 0x9D2C5680,

			MT_T = 15,
			MT_C = 0xEFC60000,
			MT_L = 18,

			MT_F = 0x6C078965,

			LOWER_MASK = 0x7FFFFFFF,
			UPPER_MASK = 0x80000000,
		};

		uint32_t mtStates_[MT_N];
		int mtIndex_;

		int seed_;
		uint32_t _GenrandInt32();
	public:
		RandProvider();
		RandProvider(unsigned long s);
		virtual ~RandProvider() {}
		void Initialize(unsigned long s);

		int GetSeed() { return seed_; }
		long GetInt();
		long GetInt(long min, long max);
		int64_t GetInt64();
		int64_t GetInt64(int64_t min, int64_t max);
		float GetReal();
		float GetReal(float min, float max);
	};
	*/

	class RandProvider {
	private:
		uint32_t states_[4];

		int seed_;
		uint32_t _GenrandInt32();

		inline uint32_t rotl(uint32_t u, int x);
		inline uint32_t rotr(uint32_t u, int x);
	public:
		RandProvider();
		RandProvider(unsigned long s);
		virtual ~RandProvider() {}
		void Initialize(unsigned long s);

		int GetSeed() { return seed_; }
		long GetInt();
		long GetInt(long min, long max);
		int64_t GetInt64();
		int64_t GetInt64(int64_t min, int64_t max);
		double GetReal();
		double GetReal(double min, double max);
	};
}

#endif
