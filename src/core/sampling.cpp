#include "sampling.h"
#include "pbr.h"

Point2f ConcentricSampleDisk(const Point2f& u) {
    // Map uniform random numbers to $[-1,1]^2$
    Point2f uOffset = u * 2.f - Vector2f(1, 1);

    // Handle degeneracy at the origin
    if (uOffset.x == 0 && uOffset.y == 0) return Point2f(0, 0);

    // Apply concentric mapping to point
    float theta, r;
    if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
        r = uOffset.x;
        theta = PiOver4 * (uOffset.y / uOffset.x);
    }
    else {
        r = uOffset.y;
        theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
    }
    return Point2f(std::cos(theta), std::sin(theta)) * r;
}

Vector3f CosineSampleHemisphere(const Point2f& u) {
    Point2f d = ConcentricSampleDisk(u);
    float z = std::sqrt(std::max((float)0, 1 - d.x * d.x - d.y * d.y));
    return Vector3f(d.x, d.y, z);
}

// pConditionalV。一组Distribution1D。
// pMarginal。以每一组的平均值再生成一个Distribution1D。
Distribution2D::Distribution2D(const float* data, int nu, int nv) {
    pConditionalV.reserve(nv);
    for (int v = 0; v < nv; ++v) {
        // Compute conditional sampling distribution for $\tilde{v}$
        pConditionalV.emplace_back(new Distribution1D(&data[v * nu], nu));
    }
    // Compute marginal sampling distribution $p[\tilde{v}]$
    std::vector<float> marginalFunc;
    marginalFunc.reserve(nv);
    for (int v = 0; v < nv; ++v)
        marginalFunc.push_back(pConditionalV[v]->funcInt);
    pMarginal.reset(new Distribution1D(&marginalFunc[0], nv));
}