#include "point.h"

Spectrum PointLight::Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf) const {
    //ProfilePhase _(Prof::LightSample);
    //auto v = pLight - ref.p;
    //auto vn = Normalize(v);

    *wi = Normalize(pLight - ref.p);
    *pdf = 1.f;
    // *vis = VisibilityTester(ref, Interaction(pLight, ref.time, mediumInterface));

    // Ö¸ÊýË¥¼õ
    // return I;
    return I / DistanceSquared(pLight, ref.p) * 4000;
}