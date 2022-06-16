#include "reflection.h"
#include "spectrum.h"

Spectrum LambertianReflection::f(const Vector3f& wo, const Vector3f& wi) const {
    return R * InvPi;
}

Spectrum LambertianTransmission::f(const Vector3f& wo,
    const Vector3f& wi) const {
    return T * InvPi;
}

Spectrum BSDF::f(const Vector3f& woW, const Vector3f& wiW, BxDFType flags) const {
    Spectrum f(0.f);
    return f;
}

Spectrum BSDF::Sample_f(const Vector3f& woWorld, Vector3f* wiWorld,
    const Point2f& u, float* pdf, BxDFType type,
    BxDFType* sampledType) const {
    Spectrum f(0.f);
    return f;
}
