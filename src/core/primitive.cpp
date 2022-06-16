#include "primitive.h"

bool GeometricPrimitive::Intersect(const Ray& r, float* tHit, SurfaceInteraction* isect) const {
    if (!shape->Intersect(r, tHit, isect))
        return false;

    Log("shape->Intersect");

    isect->primitive = this;
    return true;
}

bool GeometricPrimitive::Intersect(const Ray& r) const {
	return shape->Intersect(r);
}

void GeometricPrimitive::ComputeScatteringFunctions(
    SurfaceInteraction* isect, TransportMode mode) const {
    //ProfilePhase p(Prof::ComputeScatteringFuncs);
    if (material)
        material->ComputeScatteringFunctions(isect, mode);
}