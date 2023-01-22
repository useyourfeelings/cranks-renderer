#ifndef CORE_PBR_H
#define CORE_PBR_H

#include <cmath>
#include <limits>
#include <utility>
#include <memory>
#include <format>
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
class Medium;
class MediumInteraction;

template <typename T> class Texture;

#define MaxFloat std::numeric_limits<float>::max()
#define MinFloat std::numeric_limits<float>::min()
#define Infinity std::numeric_limits<float>::infinity()
#define MachineEpsilon (std::numeric_limits<float>::epsilon() * 0.5) // ulp/2

// const
//static float Error1 = 1;// 2;// 1.5;//0.001;
static float ShadowEpsilon = 0.0001f;
static float Pi = 3.14159265358979323846;
static float InvPi = 0.31830988618379067154;
static float Inv2Pi = 0.15915494309189533577;
static float Inv4Pi = 0.07957747154594766788;
static float PiOver2 = 1.57079632679489661923;
static float PiOver4 = 0.78539816339744830961;

inline float Radians(float deg) { return deg * Pi / 180; }


// pbrt page 1064
template <typename T>
inline bool IsPowerOf2(T v) {
	return v && !(v & (v - 1));
}

// pbrt page 1064
inline int32_t RoundUpPow2(int32_t v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return v + 1;
}

inline int64_t RoundUpPow2(int64_t v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	return v + 1;
}

template <typename T, typename U, typename V>
inline T Clamp(T val, U low, V high) {
	if (val < low)
		return low;
	else if (val > high)
		return high;
	else
		return val;
}

template <typename T>
inline T Mod(T a, T b) {
	T result = a - (a / b) * b;
	return (T)((result < 0) ? result + b : result);
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