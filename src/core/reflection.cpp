#include "reflection.h"
#include "spectrum.h"
#include "sampler.h"
#include "sampling.h"

// BxDF基类pdf
float BxDF::Pdf(const Vector3f& wo, const Vector3f& wi) const {
    return SameHemisphere(wo, wi) ? AbsCosTheta(wi) * InvPi : 0;
}

Spectrum BxDF::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u, float* pdf, BxDFType* sampledType) const {
    //return Spectrum(0.f);

    //pbrt 13.5 13.6设计较多数学推导
    
    // Cosine-sample the hemisphere, flipping the direction if necessary
    *wi = CosineSampleHemisphere(u);
    if (wo.z < 0) wi->z *= -1;
    *pdf = Pdf(wo, *wi);
    return f(wo, *wi);
}

Spectrum LambertianReflection::f(const Vector3f& wo, const Vector3f& wi) const {
    // 为什么InvPi。待整理。
    return R * InvPi;
}

Spectrum LambertianTransmission::f(const Vector3f& wo, const Vector3f& wi) const {
    return T * InvPi;
}

float LambertianTransmission::Pdf(const Vector3f& wo, const Vector3f& wi) const {
    return !SameHemisphere(wo, wi) ? AbsCosTheta(wi) * InvPi : 0;
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

// pbrt page 832
// 
// 给定wo。随机找一个匹配类型的bxdf。bxdf进行采样。得到wi。
// 
Spectrum BSDF::Sample_f(const Vector3f& woWorld, Vector3f* wiWorld, const Point2f& u, float* pdf, BxDFType type, BxDFType* sampledType) const {

    // Choose which _BxDF_ to sample
    int matchingComps = NumComponents(type);
    if (matchingComps == 0) {
        *pdf = 0;
        if (sampledType) *sampledType = BxDFType(0);
        return Spectrum(0);
    }

    // 这里代码有点恶心。其实就是要从匹配的bxdf中按平均分布，随机选一个。

    // [0, matchingComps - 1]中的某个整数
    int comp = std::min((int)std::floor(u.x * matchingComps), matchingComps - 1); // min([0, matchingComps), matchingComps-1)

    // Get _BxDF_ pointer for chosen component
    //BxDF* bxdf = nullptr;
    std::shared_ptr<BxDF> bxdf = nullptr;
    int count = comp;
    for (int i = 0; i < nBxDFs; ++i)
        if (bxdfs[i]->MatchesFlags(type) && count-- == 0) {
            bxdf = bxdfs[i];
            break;
        }
    //CHECK(bxdf != nullptr);
    //VLOG(2) << "BSDF::Sample_f chose comp = " << comp << " / matching = " << matchingComps << ", bxdf: " << bxdf->ToString();


    // Remap _BxDF_ sample _u_ to $[0,1)^2$
    // OneMinusEpsilon是1缺一点点。比如0.99999999999999
    Point2f uRemapped(std::min(u.x * matchingComps - comp, OneMinusEpsilon), u.y);

    // Sample chosen _BxDF_
    Vector3f wi;
    Vector3f wo = WorldToLocal(woWorld); // 转到微小面本地nts

    if (wo.z == 0)
        return 0.; // normal方向为0

    *pdf = 0;
    if (sampledType)
        *sampledType = bxdf->type;

    // bxdf采样。已知wo，采样一个wi，算出光能，并拿到pdf。
    // 例如对于镜面反射，就是完美的对称的wi，pdf为1。光能根据fresnel算。
    // 例如对于折射，就算折射方向和。光能根据fresnel算。
    // 对于Lambertian？反射平均分布在半球。随机一个方向？
    // 坐标必须先转到local。z轴为normal。
    Spectrum f = bxdf->Sample_f(wo, &wi, uRemapped, pdf, sampledType);

    //VLOG(2) << "For wo = " << wo << ", sampled f = " << f << ", pdf = " 
    //    << *pdf << ", ratio = " << ((*pdf > 0) ? (f / *pdf) : Spectrum(0.))
    //    << ", wi = " << wi;
    if (*pdf == 0) {
        if (sampledType) *sampledType = BxDFType(0);
        return 0;
    }
    *wiWorld = LocalToWorld(wi);

    // Compute overall PDF with all matching _BxDF_s
    if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1)
        for (int i = 0; i < nBxDFs; ++i)
            if (bxdfs[i] != bxdf && bxdfs[i]->MatchesFlags(type))
                *pdf += bxdfs[i]->Pdf(wo, wi);
    if (matchingComps > 1) *pdf /= matchingComps;

    // Compute value of BSDF for sampled direction
    if (!(bxdf->type & BSDF_SPECULAR)) {
        bool reflect = Dot(*wiWorld, ng) * Dot(woWorld, ng) > 0;
        f = 0.;
        for (int i = 0; i < nBxDFs; ++i)
            if (bxdfs[i]->MatchesFlags(type) &&
                ((reflect && (bxdfs[i]->type & BSDF_REFLECTION)) ||
                    (!reflect && (bxdfs[i]->type & BSDF_TRANSMISSION))))
                f += bxdfs[i]->f(wo, wi);
    }
    //VLOG(2) << "Overall f = " << f << ", pdf = " << *pdf << ", ratio = " << ((*pdf > 0) ? (f / *pdf) : Spectrum(0.));


    return f;
}

// 算所有bxdf的平均值
float BSDF::Pdf(const Vector3f& woWorld, const Vector3f& wiWorld,
    BxDFType flags) const {
    //ProfilePhase pp(Prof::BSDFPdf);
    if (nBxDFs == 0.f) return 0.f;
    Vector3f wo = WorldToLocal(woWorld), wi = WorldToLocal(wiWorld);
    if (wo.z == 0) return 0.;
    float pdf = 0.f;
    int matchingComps = 0;
    for (int i = 0; i < nBxDFs; ++i)
        if (bxdfs[i]->MatchesFlags(flags)) {
            ++matchingComps;
            pdf += bxdfs[i]->Pdf(wo, wi);
        }
    float v = matchingComps > 0 ? pdf / matchingComps : 0.f;
    return v;
}

/////////////////////////////////

float FrDielectric(float cosThetaI, float etaI, float etaT) {
    cosThetaI = Clamp(cosThetaI, -1, 1);
    // Potentially swap indices of refraction

    bool entering = cosThetaI > 0.f;
    if (!entering) {
        std::swap(etaI, etaT);
        cosThetaI = std::abs(cosThetaI);
    }

    float sinThetaI = std::sqrt(std::max((float)0, 1 - cosThetaI * cosThetaI)); // sin2(a) + cos2(b) = 1

    // Snell's law
    float sinThetaT = etaI / etaT * sinThetaI;

    // Handle total internal reflection
    if (sinThetaT >= 1) return 1;

    float cosThetaT = std::sqrt(std::max((float)0, 1 - sinThetaT * sinThetaT)); // sin2(a) + cos2(b) = 1

    float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) /
        ((etaT * cosThetaI) + (etaI * cosThetaT));
    float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) /
        ((etaI * cosThetaI) + (etaT * cosThetaT));
    return (Rparl * Rparl + Rperp * Rperp) / 2;
}


Spectrum FresnelDielectric::Evaluate(float cosThetaI) const {
    return FrDielectric(cosThetaI, etaI, etaT);
}

// 镜面的反射部分
Spectrum SpecularReflection::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample, float* pdf, BxDFType* sampledType) const {
    // Compute perfect specular reflection direction
    //std::cout << "SpecularReflection::Sample_f" << std::endl;
    *wi = Vector3f(-wo.x, -wo.y, wo.z); // 以原点镜面反射。此时一定是本地坐标，z轴为normal？
    *pdf = 1;

    // pbrt page 524
    return fresnel->Evaluate(CosTheta(*wi)) * R / AbsCosTheta(*wi);
}

// 镜面的折射部分
Spectrum SpecularTransmission::Sample_f(const Vector3f& wo, Vector3f* wi,
    const Point2f& sample, float* pdf,
    BxDFType* sampledType) const {
    //std::cout << "SpecularTransmission::Sample_f" << std::endl;

    // Figure out which $\eta$ is incident and which is transmitted
    bool entering = CosTheta(wo) > 0;
    float etaI = entering ? etaA : etaB;
    float etaT = entering ? etaB : etaA;

    // Compute ray direction for specular transmission
    if (!Refract(wo, Faceforward(Vector3f(0, 0, 1), wo), etaI / etaT, wi))
        return 0;
    *pdf = 1;
    Spectrum ft = T * (Spectrum(1.) - fresnel.Evaluate(CosTheta(*wi)));
    // Account for non-symmetry with transmission to different medium
    if (mode == TransportMode::Radiance)
        ft *= (etaI * etaI) / (etaT * etaT);

    return ft / AbsCosTheta(*wi);
}