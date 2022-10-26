#include "point.h"

// 已知光源，交点。算出wi，能量。
Spectrum PointLight::Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf) const {
    //ProfilePhase _(Prof::LightSample);
    //auto v = pLight - ref.p;
    //auto vn = Normalize(v);

    *wi = Normalize(pLight - ref.p);
    *pdf = 1.f;
    // *vis = VisibilityTester(ref, Interaction(pLight, ref.time, mediumInterface));

    // 指数衰减
    // return I;
    return I / DistanceSquared(pLight, ref.p) * 4000;
}

Spectrum PointLight::Le(const RayDifferential& ray) const {

    return I;
}