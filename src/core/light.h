#ifndef CORE_LIGHT_H
#define CORE_LIGHT_H

#include "object.h"
#include "interaction.h"
#include "transform.h"
#include "spectrum.h"
#include "sampling.h"

class Scene;

enum class LightFlags : int {
	DeltaPosition = 1,
	DeltaDirection = 2,
	Area = 4,
	Infinite = 8
};

class Light: public Object {
public:
	Light(const std::string& name, int flags, const Transform& LightToWorld, int nSamples = 1);
	virtual ~Light();

	virtual Spectrum Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf) const = 0;
	virtual Spectrum Le(const RayDifferential& r) const;

	inline bool IsDeltaLight() const {
		return flags & (int)LightFlags::DeltaPosition || flags & (int)LightFlags::DeltaDirection;
	}

	const int flags;
	const int nSamples;

	Point3f pos;

	const Transform LightToWorld, WorldToLight;
};

////////////////////////


class LightDistribution {
public:
	virtual ~LightDistribution() {};

	// Given a point |p| in space, this method returns a (hopefully
	// effective) sampling distribution for light sources at that point.
	virtual const Distribution1D* Lookup(const Point3f& p) const = 0;
};

class UniformLightDistribution : public LightDistribution {
public:
	UniformLightDistribution(const Scene& scene);
	Distribution1D* Lookup(const Point3f& p) const;

private:
	std::unique_ptr<Distribution1D> distrib;
};


std::unique_ptr<LightDistribution> CreateLightSampleDistribution(
	const std::string& name, const Scene& scene);


#endif