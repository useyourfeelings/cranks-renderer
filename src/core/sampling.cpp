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