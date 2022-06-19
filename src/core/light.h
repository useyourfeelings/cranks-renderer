#ifndef CORE_LIGHT_H
#define CORE_LIGHT_H

#include "interaction.h"
#include "transform.h"
#include "spectrum.h"

enum class LightFlags : int {
	DeltaPosition = 1,
	DeltaDirection = 2,
	Area = 4,
	Infinite = 8
};

class Light {
public:
	Light(int flags, const Transform& LightToWorld, int nSamples = 1);
	virtual ~Light();

	virtual Spectrum Sample_Li(const Interaction& ref, Vector3f* wi, float* pdf) const = 0;
	virtual Spectrum Le(const RayDifferential& r) const;

	const int flags;
	const int nSamples;

	Point3f pos;

	const Transform LightToWorld, WorldToLight;
};


#endif