#include "texture.h"
#include "interaction.h"

float Lanczos(float x, float tau) {
    x = std::abs(x);
    if (x < 1e-5f) return 1;
    if (x > 1.f) return 0;
    x *= Pi;
    float s = std::sin(x * tau) / (x * tau);
    float lanczos = std::sin(x) / x;
    return s * lanczos;
}


UVMapping2D::UVMapping2D(float su, float sv, float du, float dv)
    : su(su), sv(sv), du(du), dv(dv) {}

Point2f UVMapping2D::Map(const SurfaceInteraction& si, Vector2f* dstdx,
    Vector2f* dstdy) const {
    // Compute texture differentials for 2D identity mapping
    *dstdx = Vector2f(su * si.dudx, sv * si.dvdx);
    *dstdy = Vector2f(su * si.dudy, sv * si.dvdy);
    return Point2f(su * si.uv[0] + du, sv * si.uv[1] + dv);
}