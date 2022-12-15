#ifndef CORE_REFLECTION_H
#define CORE_REFLECTION_H

#include<string>
#include<memory>
#include<vector>
#include "transform.h"
#include "spectrum.h"
#include "interaction.h"
#include "microfacet.h"

enum BxDFType {
	BSDF_REFLECTION = 1 << 0,
	BSDF_TRANSMISSION = 1 << 1,
	BSDF_DIFFUSE = 1 << 2,
	BSDF_GLOSSY = 1 << 3,
	BSDF_SPECULAR = 1 << 4,
	BSDF_ALL = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR | BSDF_REFLECTION |
	BSDF_TRANSMISSION,
};

// pbrt page 510
// 本地w的各种计算
inline float CosTheta(const Vector3f& w) { return w.z; }
inline float Cos2Theta(const Vector3f& w) { return w.z * w.z; }
inline float AbsCosTheta(const Vector3f& w) { return std::abs(w.z); }
inline float Sin2Theta(const Vector3f& w) {
    return std::max((float)0, (float)1 - Cos2Theta(w));
}

inline float SinTheta(const Vector3f& w) { return std::sqrt(Sin2Theta(w)); }
inline float TanTheta(const Vector3f& w) { return SinTheta(w) / CosTheta(w); }
inline float Tan2Theta(const Vector3f& w) {
    return Sin2Theta(w) / Cos2Theta(w);
}

inline float CosPhi(const Vector3f& w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1 : Clamp(w.x / sinTheta, -1, 1);
}

inline float SinPhi(const Vector3f& w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 0 : Clamp(w.y / sinTheta, -1, 1);
}

inline float Cos2Phi(const Vector3f& w) { return CosPhi(w) * CosPhi(w); }
inline float Sin2Phi(const Vector3f& w) { return SinPhi(w) * SinPhi(w); }

// 已知入射方向，normal和eta比。求折射方向。
inline bool Refract(const Vector3f& wi, const Vector3f& n, float eta, Vector3f* wt) {
    // Compute $\cos \theta_\roman{t}$ using Snell's law
    float cosThetaI = Dot(n, wi);
    float sin2ThetaI = std::max(float(0), float(1 - cosThetaI * cosThetaI));
    float sin2ThetaT = eta * eta * sin2ThetaI;

    // Handle total internal reflection for transmission
    if (sin2ThetaT >= 1)
        return false;

    float cosThetaT = std::sqrt(1 - sin2ThetaT);

    *wt = eta * -wi + (eta * cosThetaI - cosThetaT) * n;
    return true;
}

// 已知normal求反射光线
inline Vector3f Reflect(const Vector3f& wo, const Vector3f& n) {
    return -wo + 2 * Dot(wo, n) * n;
}

// 夹角<90度。在同个半球。
inline bool SameHemisphere(const Vector3f& w, const Vector3f& wp) {
    return w.z * wp.z > 0;
}

// 球坐标
inline Vector3f SphericalDirection(float sinTheta, float cosTheta, float phi) {
    return Vector3f(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
}

float FrDielectric(float cosThetaI, float etaI, float etaT);

class BxDF {
public:
	BxDF(BxDFType type) : type(type) {}
	virtual ~BxDF() {}

    bool MatchesFlags(BxDFType t) const { 
        return (type & t) == type; // t包含自身
    }

	virtual Spectrum f(const Vector3f& wo, const Vector3f& wi) const = 0;
    virtual Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample, float* pdf, BxDFType* sampledType = nullptr) const;
	virtual float Pdf(const Vector3f& wo, const Vector3f& wi) const;

    virtual Spectrum rho(const Vector3f& wo, int nSamples, const Point2f* samples) const;

	const BxDFType type;
};

struct BSDFRESULT {
    float pdf;
    BxDFType type;
    Vector3f wi;
    Spectrum f;
};

class BSDF {
public:
    // BSDF Public Methods
    BSDF(const SurfaceInteraction& si, float eta = 1):
        nBxDFs(0),
        eta(eta),
        ns(si.shading.n),
        ng(si.n),
        ss(Normalize(si.shading.dpdu)),
        ts(Cross(ns, ss)) {}

    ~BSDF() {}

    void Add(BxDF* b) {
        //CHECK_LT(nBxDFs, MaxBxDFs);
        bxdfs[nBxDFs++] = b;
    }

    /*void Add(std::shared_ptr<BxDF> bxdf) {
        bxdfs[nBxDFs++] = bxdf;
    }*/

    int NumComponents(BxDFType flags = BSDF_ALL) const;

    // 计算ss/ts/ns上的分量。即转到微小面本地stn。
    Vector3f WorldToLocal(const Vector3f& v) const {
        return Vector3f(Dot(v, ss), Dot(v, ts), Dot(v, ns));
    }

    Vector3f LocalToWorld(const Vector3f& v) const {
        return Vector3f(ss.x * v.x + ts.x * v.y + ns.x * v.z,
            ss.y * v.x + ts.y * v.y + ns.y * v.z,
            ss.z * v.x + ts.z * v.y + ns.z * v.z);
    }

    Spectrum f(const Vector3f& woW, const Vector3f& wiW,
        BxDFType flags = BSDF_ALL) const;
    Spectrum rho(int nSamples, const Point2f* samples1, const Point2f* samples2,
        BxDFType flags = BSDF_ALL) const;
    Spectrum rho(const Vector3f& wo, int nSamples, const Point2f* samples,
        BxDFType flags = BSDF_ALL) const;
    Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
        float* pdf, BxDFType type = BSDF_ALL,
        BxDFType* sampledType = nullptr) const;
    float Pdf(const Vector3f& wo, const Vector3f& wi,
        BxDFType flags = BSDF_ALL) const;
    std::string ToString() const;

    int All_f(const Vector3f& woWorld, const Point2f& u, BSDFRESULT *result) const;

    // BSDF Public Data
    const float eta;

private:
    // BSDF Private Methods
    

    // BSDF Private Data
    const Vector3f ns, ng; //Normal3f ns, ng;
    const Vector3f ss, ts;
    int nBxDFs = 0;
    static const int MaxBxDFs = 8;
    BxDF* bxdfs[MaxBxDFs];
    //std::shared_ptr<BxDF> bxdfs[MaxBxDFs];

    //std::vector<std::shared_ptr<BxDF>> bxdfs;
    friend class MixMaterial;
};

inline int BSDF::NumComponents(BxDFType flags) const {
    int num = 0;
    for (int i = 0; i < nBxDFs; ++i){
        if (bxdfs[i]->MatchesFlags(flags))
            ++num;
    }
        
    return num;
}


class LambertianReflection : public BxDF {
public:
    // LambertianReflection Public Methods
    LambertianReflection(const Spectrum& R)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(R) {}
    Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
    Spectrum rho(const Vector3f&, int, const Point2f*) const { return R; } // 反射率为常数R
    //Spectrum rho(int, const Point2f*, const Point2f*) const { return R; }
    //std::string ToString() const;

private:
    // LambertianReflection Private Data
    const Spectrum R;
};

class LambertianTransmission : public BxDF {
public:
    // LambertianTransmission Public Methods
    LambertianTransmission(const Spectrum& T)
        : BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_DIFFUSE)), T(T) {}
    Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
    Spectrum rho(const Vector3f&, int, const Point2f*) const { return T; }
    Spectrum rho(int, const Point2f*, const Point2f*) const { return T; }
    Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u, float* pdf, BxDFType* sampledType) const;
    float Pdf(const Vector3f& wo, const Vector3f& wi) const;
    //std::string ToString() const;

private:
    // LambertianTransmission Private Data
    Spectrum T;
};



class Fresnel {
public:
    // Fresnel Interface
    virtual ~Fresnel() {};
    virtual Spectrum Evaluate(float cosI) const = 0;
    //virtual std::string ToString() const = 0;
};

// 两个绝缘体交界处的Fresnel
class FresnelDielectric : public Fresnel {
public:
    // FresnelDielectric Public Methods
    FresnelDielectric(float etaI, float etaT) : etaI(etaI), etaT(etaT) {}

    Spectrum Evaluate(float cosThetaI) const;
    //std::string ToString() const;

private:
    float etaI; // 材质1 eta
    float etaT; // 材质2 eta
};

// 导体
class FresnelConductor : public Fresnel {
public:
    // FresnelConductor Public Methods
    Spectrum Evaluate(float cosThetaI) const;
    FresnelConductor(const Spectrum& etaI, const Spectrum& etaT,
        const Spectrum& k)
        : etaI(etaI), etaT(etaT), k(k) {}
    //std::string ToString() const;

private:
    Spectrum etaI, etaT, k;
};

class FresnelNoOp : public Fresnel {
public:
    Spectrum Evaluate(float) const { return Spectrum(1.); }
    
    //std::string ToString() const { return "[ FresnelNoOp ]"; }
};


//////////////////////////////////////////

class SpecularReflection : public BxDF {
public:
    // SpecularReflection Public Methods
    SpecularReflection(const Spectrum& R, std::shared_ptr<Fresnel> fresnel)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)),
        R(R),
        fresnel(fresnel) {}
    Spectrum f(const Vector3f& wo, const Vector3f& wi) const {
        // see pbrt page 524
        // 对于镜面反射始终返回0
        return Spectrum(0.f);
    }
    Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample,
        float* pdf, BxDFType* sampledType) const;
    float Pdf(const Vector3f& wo, const Vector3f& wi) const { return 0; }
    //std::string ToString() const;

    Spectrum rho(const Vector3f& w, int nSamples, const Point2f* u) const;

private:
    // SpecularReflection Private Data
    const Spectrum R;
    //const Fresnel* fresnel;
    std::shared_ptr<Fresnel> fresnel;
};

class SpecularTransmission : public BxDF {
public:
    // SpecularTransmission Public Methods
    SpecularTransmission(const Spectrum& T, float etaA, float etaB, TransportMode mode)
        : BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)),
        T(T),
        etaA(etaA),
        etaB(etaB),
        fresnel(etaA, etaB),
        mode(mode) {}
    Spectrum f(const Vector3f& wo, const Vector3f& wi) const {
        return Spectrum(0.f);
    }
    Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample,
        float* pdf, BxDFType* sampledType) const;

    float Pdf(const Vector3f& wo, const Vector3f& wi) const { return 0; }

    //std::string ToString() const;

    Spectrum rho(const Vector3f& w, int nSamples, const Point2f* u) const;

private:
    // SpecularTransmission Private Data
    const Spectrum T;
    const float etaA, etaB;
    const FresnelDielectric fresnel;
    const TransportMode mode;
};

class MicrofacetReflection : public BxDF {
public:
    // MicrofacetReflection Public Methods
    MicrofacetReflection(const Spectrum& R, std::shared_ptr<MicrofacetDistribution> distribution, std::shared_ptr<Fresnel> fresnel)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
        R(R),
        distribution(distribution),
        fresnel(fresnel) {}
    Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
    Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
        float* pdf, BxDFType* sampledType) const;
    float Pdf(const Vector3f& wo, const Vector3f& wi) const;
    //std::string ToString() const;

private:
    // MicrofacetReflection Private Data
    const Spectrum R;
    std::shared_ptr<MicrofacetDistribution> distribution;
    std::shared_ptr<Fresnel> fresnel;
};



#endif