#include "reflection.h"
#include "spectrum.h"
#include "sampler.h"
#include "sampling.h"

// BxDF����pdf
float BxDF::Pdf(const Vector3f& wo, const Vector3f& wi) const {
    return SameHemisphere(wo, wi) ? AbsCosTheta(wi) * InvPi : 0;
}

Spectrum BxDF::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u, float* pdf, BxDFType* sampledType) const {
    // Ĭ�϶԰����������
    // pbrt 13.5 13.6��ƽ϶���ѧ�Ƶ�
    
    // Cosine-sample the hemisphere, flipping the direction if necessary
    *wi = CosineSampleHemisphere(u);
    if (wo.z < 0) wi->z *= -1;
    *pdf = Pdf(wo, *wi);
    return f(wo, *wi);
}

Spectrum LambertianReflection::f(const Vector3f& wo, const Vector3f& wi) const {
    // ΪʲôInvPi��������
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

    /*for (auto bxdf : bxdfs) {
        f += bxdf->f(wo, wi);
    }*/

    for (int i = 0; i < nBxDFs; ++i) {
        f += bxdfs[i]->f(wo, wi);
    }


    return f;
}

// pbrt page 832
// 
// ����wo�������һ��ƥ�����͵�bxdf��bxdf���в������õ�wi��
// 
Spectrum BSDF::Sample_f(const Vector3f& woWorld, Vector3f* wiWorld, const Point2f& u, float* pdf, BxDFType type, BxDFType* sampledType) const {

    // Choose which _BxDF_ to sample
    int matchingComps = NumComponents(type);
    if (matchingComps == 0) {
        *pdf = 0;
        if (sampledType) 
            *sampledType = BxDFType(0);

        return Spectrum(0);
    }

    // ��������е���ġ���ʵ����Ҫ��ƥ���bxdf�а�ƽ���ֲ������ѡһ����

    // [0, matchingComps - 1]�е�ĳ������
    int comp = std::min((int)std::floor(u.x * matchingComps), matchingComps - 1); // min([0, matchingComps), matchingComps-1)

    // Get _BxDF_ pointer for chosen component
    BxDF* bxdf = nullptr;
    //std::shared_ptr<BxDF> bxdf = nullptr;
    int count = comp;
    for (int i = 0; i < nBxDFs; ++i)
        if (bxdfs[i]->MatchesFlags(type) && count-- == 0) {
            bxdf = bxdfs[i];
            break;
        }
    //CHECK(bxdf != nullptr);
    //VLOG(2) << "BSDF::Sample_f chose comp = " << comp << " / matching = " << matchingComps << ", bxdf: " << bxdf->ToString();


    // Remap _BxDF_ sample _u_ to $[0,1)^2$
    // OneMinusEpsilon��1ȱһ��㡣����0.99999999999999
    Point2f uRemapped(std::min(u.x * matchingComps - comp, OneMinusEpsilon), u.y);

    // Sample chosen _BxDF_
    Vector3f wi;
    Vector3f wo = WorldToLocal(woWorld); // ת��΢С�汾��nts

    if (wo.z == 0)
        return 0.; // normal����Ϊ0

    *pdf = 0;
    if (sampledType)
        *sampledType = bxdf->type;

    // bxdf��������֪wo������һ��wi��������ܣ����õ�pdf��
    // ������ھ��淴�䣬���������ĶԳƵ�wi��pdfΪ1�����ܸ���fresnel�㡣
    // ����������䣬�������䷽��͡����ܸ���fresnel�㡣
    // ����Lambertian������ƽ���ֲ��ڰ������һ������
    // ���������ת��local��z��Ϊnormal��
    Spectrum f = bxdf->Sample_f(wo, &wi, uRemapped, pdf, sampledType);

    //VLOG(2) << "For wo = " << wo << ", sampled f = " << f << ", pdf = " 
    //    << *pdf << ", ratio = " << ((*pdf > 0) ? (f / *pdf) : Spectrum(0.))
    //    << ", wi = " << wi;
    if (*pdf == 0) {
        if (sampledType)
            *sampledType = BxDFType(0);

        return 0;
    }
    *wiWorld = LocalToWorld(wi);

    // Compute overall PDF with all matching _BxDF_s
    // ���ѡ���Ĳ���specular����������match��bxdf��������bxdf��pdf�����ϡ�
    if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1)
        for (int i = 0; i < nBxDFs; ++i)
            if (bxdfs[i] != bxdf && bxdfs[i]->MatchesFlags(type))
                *pdf += bxdfs[i]->Pdf(wo, wi);

    // ƽ��pdf
    if (matchingComps > 1)
        *pdf /= matchingComps;

    // Compute value of BSDF for sampled direction
    if (!(bxdf->type & BSDF_SPECULAR)) {
        bool reflect = Dot(*wiWorld, ng) * Dot(woWorld, ng) > 0;
        f = 0.;
        for (int i = 0; i < nBxDFs; ++i) {
            if (bxdfs[i]->MatchesFlags(type)) {
                if ((reflect && (bxdfs[i]->type & BSDF_REFLECTION)) ||
                    (!reflect && (bxdfs[i]->type & BSDF_TRANSMISSION)))
                    f += bxdfs[i]->f(wo, wi);
            }
        }
            
    }
    //VLOG(2) << "Overall f = " << f << ", pdf = " << *pdf << ", ratio = " << ((*pdf > 0) ? (f / *pdf) : Spectrum(0.));


    return f;
}

// ������bxdf��ƽ��ֵ
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

// �õ�fresnel�ٷֱ�
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

// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
Spectrum FrConductor(float cosThetaI, const Spectrum& etai,
    const Spectrum& etat, const Spectrum& k) {
    cosThetaI = Clamp(cosThetaI, -1, 1);
    Spectrum eta = etat / etai;
    Spectrum etak = k / etai;

    float cosThetaI2 = cosThetaI * cosThetaI;
    float sinThetaI2 = 1. - cosThetaI2;
    Spectrum eta2 = eta * eta;
    Spectrum etak2 = etak * etak;

    Spectrum t0 = eta2 - etak2 - sinThetaI2;
    Spectrum a2plusb2 = Sqrt(t0 * t0 + 4 * eta2 * etak2);
    Spectrum t1 = a2plusb2 + cosThetaI2;
    Spectrum a = Sqrt(0.5f * (a2plusb2 + t0));
    Spectrum t2 = (float)2 * cosThetaI * a;
    Spectrum Rs = (t1 - t2) / (t1 + t2);

    Spectrum t3 = cosThetaI2 * a2plusb2 + sinThetaI2 * sinThetaI2;
    Spectrum t4 = t2 * sinThetaI2;
    Spectrum Rp = Rs * (t3 - t4) / (t3 + t4);

    return 0.5 * (Rp + Rs);
}

Spectrum FresnelConductor::Evaluate(float cosThetaI) const {
    return FrConductor(std::abs(cosThetaI), etaI, etaT, k);
}

/////////////////////////////////

// ����ķ��䲿��
Spectrum SpecularReflection::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample, float* pdf, BxDFType* sampledType) const {
    // Compute perfect specular reflection direction
    //std::cout << "SpecularReflection::Sample_f" << std::endl;
    *wi = Vector3f(-wo.x, -wo.y, wo.z); // ��ԭ�㾵�淴�䡣��ʱһ���Ǳ������꣬z��Ϊnormal��
    *pdf = 1;

    // pbrt page 524
    return fresnel->Evaluate(CosTheta(*wi)) * R / AbsCosTheta(*wi);
}

// ��������䲿��
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

//////////////////////////

float MicrofacetReflection::Pdf(const Vector3f& wo, const Vector3f& wi) const {
    if (!SameHemisphere(wo, wi)) return 0;
    Vector3f wh = Normalize(wo + wi);
    return distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
}

Spectrum MicrofacetReflection::f(const Vector3f& wo, const Vector3f& wi) const {
    float cosThetaO = AbsCosTheta(wo), cosThetaI = AbsCosTheta(wi);
    Vector3f wh = wi + wo;
    // Handle degenerate cases for microfacet reflection
    if (cosThetaI == 0 || cosThetaO == 0) return Spectrum(0.);
    if (wh.x == 0 && wh.y == 0 && wh.z == 0) return Spectrum(0.);
    wh = Normalize(wh);
    // For the Fresnel call, make sure that wh is in the same hemisphere
    // as the surface normal, so that TIR is handled correctly.
    Spectrum F = fresnel->Evaluate(Dot(wi, Faceforward(wh, Vector3f(0, 0, 1))));
    //return R * distribution->D(wh) * distribution->G(wo, wi) * F / (4 * cosThetaI * cosThetaO);

    return R * distribution->D(wh) * F / (4 * cosThetaI * cosThetaO);
}

// ΢С��ķ���
// woΪ�������ꡣz��Ϊnormal��
Spectrum MicrofacetReflection::Sample_f(const Vector3f& wo, Vector3f* wi,
    const Point2f& u, float* pdf,
    BxDFType* sampledType) const {

    // Sample microfacet orientation $\wh$ and reflected direction $\wi$
    if (wo.z == 0)
        return 0.;


    Vector3f wh = distribution->Sample_wh(wo, u);
    if (Dot(wo, wh) < 0) 
        return 0.;   // Should be rare

    // �õ��ԳƵ��������
    *wi = Reflect(wo, wh);

    if (!SameHemisphere(wo, *wi))
        return Spectrum(0.f);

    // Compute PDF of _wi_ for microfacet reflection
    *pdf = distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
    return f(wo, *wi);
}

