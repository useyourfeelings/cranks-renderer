#include "interaction.h"
#include "primitive.h"

SurfaceInteraction::SurfaceInteraction(
    const Point3f& p, const Vector3f& shape_n, const Point2f& uv,
    const Vector3f& wo, const Vector3f& dpdu, const Vector3f& dpdv,
    float time, const Shape* shape)
    : Interaction(p, shape_n, Normalize(Cross(dpdu, dpdv)), wo, time),
    uv(uv),
    dpdu(dpdu),
    dpdv(dpdv),
    shape(shape){

    shading.n = n;
    shading.dpdu = dpdu;
    shading.dpdv = dpdv;


}

void SurfaceInteraction::LogSelf() {
    Log("interaction primitive = %s", primitive->name.c_str());
    Log("p");
    p.LogSelf();
    Log("n");
    n.LogSelf();
    Log("dpdu");
    dpdu.LogSelf();
    Log("dpdv");
    dpdv.LogSelf();
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

