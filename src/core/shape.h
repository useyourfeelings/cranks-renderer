#ifndef CORE_SHAPE_H
#define CORE_SHAPE_H

#include "pbr.h"
#include "transform.h"
#include "interaction.h"

class Shape {
public:
	Shape(const Transform &ObjectToWorld, const Transform &WorldToObject);
	virtual ~Shape();

	virtual bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const = 0;
	virtual bool Intersect(const Ray& r) const = 0;

	//virtual float Area() const = 0;

	virtual float Pdf();
	virtual Interaction Sample();

	const Transform ObjectToWorld, WorldToObject;
};


#endif