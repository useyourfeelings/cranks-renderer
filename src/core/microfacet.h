#ifndef CORE_MICROFACET_H
#define CORE_MICROFACET_H

#include "geometry.h"

class MicrofacetDistribution {
public:
    // MicrofacetDistribution Public Methods
    virtual ~MicrofacetDistribution();
    virtual float D(const Vector3f& wh) const = 0;
#if 0
    virtual float Lambda(const Vector3f& w) const = 0;
    float G1(const Vector3f& w) const {
        //    if (Dot(w, wh) * CosTheta(w) < 0.) return 0.;
        return 1 / (1 + Lambda(w));
    }
    virtual float G(const Vector3f& wo, const Vector3f& wi) const {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }
#endif
    virtual Vector3f Sample_wh(const Vector3f& wo, const Point2f& u) const = 0;
    float Pdf(const Vector3f& wo, const Vector3f& wh) const;
    //virtual std::string ToString() const = 0;

protected:
    // MicrofacetDistribution Protected Methods
    MicrofacetDistribution(bool sampleVisibleArea)
        : sampleVisibleArea(sampleVisibleArea) {}

    // MicrofacetDistribution Protected Data
    const bool sampleVisibleArea;
};

// GGX
class GGXDistribution : public MicrofacetDistribution {
public:
    // TrowbridgeReitzDistribution Public Methods
    static inline float RoughnessToAlpha(float roughness) {
        roughness = std::max(roughness, (float)1e-3);
        float x = std::log(roughness);
        return 1.62142f + 0.819955f * x + 0.1734f * x * x + 0.0171201f * x * x * x +
            0.000640711f * x * x * x * x;
    }

    GGXDistribution(float alphax, float alphay,
        bool samplevis = false)
        : MicrofacetDistribution(samplevis),
        alphax(std::max(float(0.001), alphax)),
        alphay(std::max(float(0.001), alphay)) {}
    float D(const Vector3f& wh) const;
    Vector3f Sample_wh(const Vector3f& wo, const Point2f& u) const;
    //std::string ToString() const;

private:
    // GGXDistribution Private Methods
    //float Lambda(const Vector3f& w) const;

    // GGXDistribution Private Data
    const float alphax, alphay;
};


#endif