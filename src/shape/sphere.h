#ifndef CORE_SPHERE_H
#define CORE_SPHERE_H

#include "../core/shape.h"

class Sphere:public Shape {
public:
	Sphere(const Transform &ObjectToWorld, const Transform &WorldToObject, float radius, float zMin,
		float zMax, float phiMax):
		Shape(ObjectToWorld, WorldToObject),
		radius(radius),
		zMin(Clamp(std::min(zMin, zMax), -radius, radius)),
		zMax(Clamp(std::max(zMin, zMax), -radius, radius)),
		thetaMin(std::acos(Clamp(std::min(zMin, zMax) / radius, -1, 1))),
		thetaMax(std::acos(Clamp(std::max(zMin, zMax) / radius, -1, 1))),
		phiMax(Radians(Clamp(phiMax, 0, 360))) {}
	
	//~Sphere();

	bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const;
	bool Intersect(const Ray& ray) const;

	//float Area() const = 0;

	float Pdf();
	Interaction Sample();

	// radius default = 1
	// zmin default -radius
	// zmax default radius
	// thetaMin default -1
	// thetaMax default 1
	// phimax default = 360      Radians(360) 2pi

	float radius;
	const float zMin, zMax;
	const float thetaMin, thetaMax, phiMax;

};

#endif