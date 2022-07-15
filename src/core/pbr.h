#ifndef CORE_PBR_H
#define CORE_PBR_H

#include <cmath>
#include <limits>
#include <utility>
#include <memory>
#include "error.h"
#include "../tool/logger.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4305)  // double constant assigned to float
#pragma warning(disable : 4244)
#endif

class Transform;
class Interaction;
class SurfaceInteraction;
class Shape;
class Primitive;
class BSDF;
//class CoefficientSpectrum;
//class RGBSpectrum;
//class Spectrum;
class Film;

template <typename T> class Texture;

#define MaxFloat std::numeric_limits<float>::max()
#define Infinity std::numeric_limits<float>::infinity()
#define MachineEpsilon (std::numeric_limits<float>::epsilon() * 0.5) // ulp/2

// const
//static float Error1 = 1;// 2;// 1.5;//0.001;
static float Pi = 3.14159265358979323846;
static float InvPi = 0.31830988618379067154;

inline float Radians(float deg) { return deg * Pi / 180; }

template <typename T, typename U, typename V>
inline T Clamp(T val, U low, V high) {
	if (val < low)
		return low;
	else if (val > high)
		return high;
	else
		return val;
}

inline int Quadratic(float a, float b, float c, float *r1, float *r2) {
	float discriminant = b * b - a * c * 4;

	if (discriminant < 0)
		return 0;

	if (discriminant == 0) {
		*r1 = *r2 = (-b) / (2 * a);
		return 1;
	}

	float sqrtd = std::sqrt(discriminant);

	
	float q;
	
	//*r1 = (-b + sqrtd) / (2 * a);
	//*r2 = (-b - sqrtd) / (2 * a);
	// 避免趋近于0

	// <<Accuracy and Stability of Numerical Algorithms>> 1.7 1.8
	// 避免可能的相减为0

	//*r1 = (b - sqrtd) / (-2 * a);
	//*r2 = (b + sqrtd) / (-2 * a);

	// 如果b<0就挑减法，如果大于0就挑加法。算出一个误差小的解，再算另一个。
	if (b < 0)
		q = (b - sqrtd) * -0.5;
	else
		q = (b + sqrtd) * -0.5;

	// r1 * r2 = c/a
	*r1 = q / a;
	*r2 = c / q;

	if(*r1 > *r2)
		std::swap(*r1, *r2);

	return 2;
}


#endif