#include "microfacet.h"
#include "reflection.h"
#include "transform.h"

MicrofacetDistribution::~MicrofacetDistribution() {}

Vector3f GGXDistribution::Sample_wh(const Vector3f& wo, const Point2f& u) const {
    Vector3f wh;

    if (!sampleVisibleArea) {

        float cosTheta = 0, phi = (2 * Pi) * u[1];
        if (alphax == alphay) { // 各向同性
            /*float tanTheta2 = alphax * alphax * u[0] / (1.0f - u[0]);
            cosTheta = 1 / std::sqrt(1 + tanTheta2);*/

            float cosTheta2 = (1.0f - u[0]) / (alphax * alphax * u[0] - u[0] + 1);

            float sinTheta = std::sqrt(std::max((float)0., (float)1. - cosTheta2));
            wh = SphericalDirection(sinTheta, std::sqrt(cosTheta2), phi);
            if (!SameHemisphere(wo, wh))
                wh = -wh;

            return wh;
        }
#if 0
        else {
            phi =
                std::atan(alphay / alphax * std::tan(2 * Pi * u[1] + .5f * Pi));
            if (u[1] > .5f) phi += Pi;
            Float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
            const Float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
            const Float alpha2 =
                1 / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
            Float tanTheta2 = alpha2 * u[0] / (1 - u[0]);
            cosTheta = 1 / std::sqrt(1 + tanTheta2);
        }

        float sinTheta = std::sqrt(std::max((float)0., (float)1. - cosTheta * cosTheta));
        wh = SphericalDirection(sinTheta, cosTheta, phi);
        if (!SameHemisphere(wo, wh))
            wh = -wh;
#endif
    }


#if 0
    else {
        bool flip = wo.z < 0;
        wh = TrowbridgeReitzSample(flip ? -wo : wo, alphax, alphay, u[0], u[1]);
        if (flip) 
            wh = -wh;
    }
#endif 
    return wh;
}

// ndf
float GGXDistribution::D(const Vector3f& wh) const {
    float tan2Theta = Tan2Theta(wh);
    if (std::isinf(tan2Theta)) return 0.;
    const float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);
    float e = (Cos2Phi(wh) / (alphax * alphax) + Sin2Phi(wh) / (alphay * alphay)) * tan2Theta;
    return 1 / (Pi * alphax * alphay * cos4Theta * (1 + e) * (1 + e));
}


float MicrofacetDistribution::Pdf(const Vector3f& wo,
    const Vector3f& wh) const {
#if 0
    if (sampleVisibleArea)
        return D(wh) * G1(wo) * AbsDot(wo, wh) / AbsCosTheta(wo);
    else
#endif
        return D(wh) * AbsCosTheta(wh);
}