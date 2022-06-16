#ifndef CORE_RNG_H
#define CORE_RNG_H

#include <random>

class RNG {
public:
	RNG() :dis(0, 1) {

	}


	float UniformFloat() {
		return dis(e);
	}

	// https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution

	std::default_random_engine e;
	std::uniform_real_distribution<> dis; // rage 0 - 1
};


#endif