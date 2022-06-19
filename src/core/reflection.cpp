#include "reflection.h"
#include "spectrum.h"

float BxDF::Pdf(const Vector3f& wo, const Vector3f& wi) const {
    return SameHemisphere(wo, wi) ? AbsCosTheta(wi) * InvPi : 0;
}

Spectrum LambertianReflection::f(const Vector3f& wo, const Vector3f& wi) const {
    return R * InvPi;
}

Spectrum LambertianTransmission::f(const Vector3f& wo, const Vector3f& wi) const {
    return T * InvPi;
}

Spectrum BSDF::f(const Vector3f& woW, const Vector3f& wiW, BxDFType flags) const {
    
    Vector3f wi = WorldToLocal(wiW), wo = WorldToLocal(woW);

    if (wo.z == 0) 
        return 0.;

    //bool reflect = Dot(wiW, ng) * Dot(woW, ng) > 0;
    
    Spectrum f(0.f);

    for (auto bxdf : bxdfs) {
        f += bxdf->f(wo, wi);
    }


    return f;
}

Spectrum BSDF::Sample_f(const Vector3f& woWorld, Vector3f* wiWorld,
    const Point2f& u, float* pdf, BxDFType type,
    BxDFType* sampledType) const {
    Spectrum f(0.f);
    return f;
}
