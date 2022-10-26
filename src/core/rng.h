#ifndef CORE_RNG_H
#define CORE_RNG_H

#include <random>

static const float FloatOneMinusEpsilon = 0x1.fffffep-1;     // 1缺一点点
static const float OneMinusEpsilon = FloatOneMinusEpsilon;

// https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution

class RNG {
public:
	RNG() :dis(0, 1) { // 范围为 [0, 1)
		std::random_device rd;
		gen = std::mt19937(rd());
	}

	float UniformFloat() {
		return float(dis(gen));
	}

	std::mt19937 gen;
	std::uniform_real_distribution<> dis; // rage 0 - 1
};


#endif