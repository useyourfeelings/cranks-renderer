#ifndef CORE_INTERACTION_H
#define CORE_INTERACTION_H

#include "geometry.h"
#include "transform.h"

#include "material.h"
#include<memory>

class Interaction {
public:
	Interaction() : time(0) {}

	Interaction(const Point3f& p, const Vector3f& shape_n, const Vector3f& n,
		const Vector3f& wo, float time)
		: p(p),
		time(time),
		//pError(pError),
		wo(Normalize(wo)),
		n(Normalize(n)),
		shape_n(Normalize(shape_n)) 
	{
	}

	Ray SpawnRay(const Vector3f& d) const {
		// Point3f o = OffsetRayOrigin(p, pError, n, d);
		return Ray(p, d, Infinity, time);
	}

	Point3f p;
	float time;

	//Vector3f pError;
	Vector3f wo; // 射出方向
	Vector3f n; 
	Vector3f shape_n; // 对于shape的normal
};

class SurfaceInteraction : public Interaction {
public:
	SurfaceInteraction() {}
	SurfaceInteraction(const Point3f& p,
		const Vector3f& shape_n,
		const Point2f& uv, const Vector3f& wo,
		const Vector3f& dpdu, const Vector3f& dpdv,
		float time,
		const Shape* sh);

	void ComputeDifferentials(const RayDifferential& r) const;

	void ComputeScatteringFunctions(
		const RayDifferential& ray,
		TransportMode mode = TransportMode::Radiance);

	//Spectrum Le(const Vector3f& w) const;

	void LogSelf();

	Point2f uv;
	Vector3f dpdu, dpdv;

	struct {
		Vector3f n; // Normal3f n;
		Vector3f dpdu, dpdv;
		//Normal3f dndu, dndv;
	} shading;

	const Shape* shape = nullptr;
	const Primitive* primitive = nullptr;

	//BSDF* bsdf = nullptr;
	std::shared_ptr<BSDF> bsdf = nullptr;

};


#endif