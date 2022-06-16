#include "interaction.h"
#include "primitive.h"

SurfaceInteraction::SurfaceInteraction(
    const Point3f& p, const Vector3f& n, const Point2f& uv,
    const Vector3f& wo, float time, const Shape* shape)
    : Interaction(p, n, wo, time),
    uv(uv),

    shape(shape){

}

void SurfaceInteraction::ComputeDifferentials( const RayDifferential& ray) const {
    //if (ray.hasDifferentials) {
    //    // Estimate screen space change in $\pt{}$ and $(u,v)$

    //    // Compute auxiliary intersection points with plane
    //    Float d = Dot(n, Vector3f(p.x, p.y, p.z));
    //    Float tx =
    //        -(Dot(n, Vector3f(ray.rxOrigin)) - d) / Dot(n, ray.rxDirection);
    //    if (std::isinf(tx) || std::isnan(tx)) goto fail;
    //    Point3f px = ray.rxOrigin + tx * ray.rxDirection;
    //    Float ty =
    //        -(Dot(n, Vector3f(ray.ryOrigin)) - d) / Dot(n, ray.ryDirection);
    //    if (std::isinf(ty) || std::isnan(ty)) goto fail;
    //    Point3f py = ray.ryOrigin + ty * ray.ryDirection;
    //    dpdx = px - p;
    //    dpdy = py - p;

    //    // Compute $(u,v)$ offsets at auxiliary points

    //    // Choose two dimensions to use for ray offset computation
    //    int dim[2];
    //    if (std::abs(n.x) > std::abs(n.y) && std::abs(n.x) > std::abs(n.z)) {
    //        dim[0] = 1;
    //        dim[1] = 2;
    //    }
    //    else if (std::abs(n.y) > std::abs(n.z)) {
    //        dim[0] = 0;
    //        dim[1] = 2;
    //    }
    //    else {
    //        dim[0] = 0;
    //        dim[1] = 1;
    //    }

    //    // Initialize _A_, _Bx_, and _By_ matrices for offset computation
    //    Float A[2][2] = { {dpdu[dim[0]], dpdv[dim[0]]},
    //                     {dpdu[dim[1]], dpdv[dim[1]]} };
    //    Float Bx[2] = { px[dim[0]] - p[dim[0]], px[dim[1]] - p[dim[1]] };
    //    Float By[2] = { py[dim[0]] - p[dim[0]], py[dim[1]] - p[dim[1]] };
    //    if (!SolveLinearSystem2x2(A, Bx, &dudx, &dvdx)) dudx = dvdx = 0;
    //    if (!SolveLinearSystem2x2(A, By, &dudy, &dvdy)) dudy = dvdy = 0;
    //}
    //else {
    //fail:
    //    dudx = dvdx = 0;
    //    dudy = dvdy = 0;
    //    dpdx = dpdy = Vector3f(0, 0, 0);
    //}
}

void SurfaceInteraction::ComputeScatteringFunctions(const RayDifferential& ray, TransportMode mode) {
    ComputeDifferentials(ray);
    primitive->ComputeScatteringFunctions(this, mode);
}

