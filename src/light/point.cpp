#include "point.h"

Spectrum PointLight::Sample_Li(const Interaction& ref, Vector3f* wi, float* pdf) const {
    //ProfilePhase _(Prof::LightSample);
    *wi = Normalize(pLight - ref.p);
    *pdf = 1.f;
    // *vis = VisibilityTester(ref, Interaction(pLight, ref.time, mediumInterface));
    return I / DistanceSquared(pLight, ref.p);
}