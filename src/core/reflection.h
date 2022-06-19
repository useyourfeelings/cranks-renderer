#ifndef CORE_REFLECTION_H
#define CORE_REFLECTION_H

#include<string>
#include<memory>
#include<vector>
#include "transform.h"
#include "spectrum.h"
#include "interaction.h"

enum BxDFType {
	BSDF_REFLECTION = 1 << 0,
	BSDF_TRANSMISSION = 1 << 1,
	BSDF_DIFFUSE = 1 << 2,
	BSDF_GLOSSY = 1 << 3,
	BSDF_SPECULAR = 1 << 4,
	BSDF_ALL = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR | BSDF_REFLECTION |
	BSDF_TRANSMISSION,
};

inline float AbsCosTheta(const Vector3f& w) { return std::abs(w.z); }

inline bool SameHemisphere(const Vector3f& w, const Vector3f& wp) {
    return w.z * wp.z > 0;
}

class BxDF {
public:
	BxDF(BxDFType type) : type(type) {}
	virtual ~BxDF() {}

	virtual Spectrum f(const Vector3f& wo, const Vector3f& wi) const = 0;
	virtual float Pdf(const Vector3f& wo, const Vector3f& wi) const;

	const BxDFType type;
};

class BSDF {
public:
    // BSDF Public Methods
    BSDF(const SurfaceInteraction& si, float eta = 1)
        : eta(eta),
        ns(si.shading.n),
        ng(si.n),
        ss(Normalize(si.shading.dpdu)),
        ts(Cross(ns, ss)) {}

    ~BSDF() {}

    //void Add(BxDF* b) {
    //    //CHECK_LT(nBxDFs, MaxBxDFs);
    //    bxdfs[nBxDFs++] = b;
    //}

    void Add(std::shared_ptr<BxDF> bxdf) {
        bxdfs.push_back(bxdf);
    }

    int NumComponents(BxDFType flags = BSDF_ALL) const;
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

    // BSDF Public Data
    const float eta;

private:
    // BSDF Private Methods
    

    // BSDF Private Data
    const Vector3f ns, ng; //Normal3f ns, ng;
    const Vector3f ss, ts;
    int nBxDFs = 0;
    static const int MaxBxDFs = 8;
    //BxDF* bxdfs[MaxBxDFs];

    std::vector<std::shared_ptr<BxDF>> bxdfs;
    friend class MixMaterial;
};


class LambertianReflection : public BxDF {
public:
    // LambertianReflection Public Methods
    LambertianReflection(const Spectrum& R)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(R) {}
    Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
    Spectrum rho(const Vector3f&, int, const Point2f*) const { return R; }
    Spectrum rho(int, const Point2f*, const Point2f*) const { return R; }
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





#endif