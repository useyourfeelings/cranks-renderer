#ifndef CORE_INTERACTION_H
#define CORE_INTERACTION_H

#include "pbr.h"
#include "geometry.h"
#include "transform.h"
#include "medium.h"
#include "material.h"
#include<memory>
#include "../base/memory.h"

class Interaction {
public:
    Interaction() : time(0) {}

    Interaction(const Point3f& p, const Vector3f& shape_n, const Vector3f& n, const Vector3f& pError,
        const Vector3f& wo, float time)
        : p(p),
        time(time),
        pError(pError),
        wo(Normalize(wo)),
        n(Normalize(n)),
        shape_n(Normalize(shape_n)) 
    {
    }

    Interaction(const Point3f& p, const Vector3f& wo, float time, const MediumInterface& mediumInterface)
        : p(p), time(time), wo(wo), mediumInterface(mediumInterface) {}

    Interaction(const Point3f& p, float time, const MediumInterface& mediumInterface)
        : p(p), time(time), mediumInterface(mediumInterface) {}

    bool IsSurfaceInteraction() const {
        return n != Vector3f();
    }

    // w和normal同向就是打到了外壁，否则是打到了内壁。
    // 外壁我写死null，用ray当前的medium。
    std::shared_ptr<Medium> GetMedium(const Vector3f& w) const {
        return Dot(w, n) > 0 ? mediumInterface.outside.lock() : mediumInterface.inside.lock();
    }

    std::shared_ptr<Medium> GetMedium() const {
        //CHECK_EQ(mediumInterface.inside, mediumInterface.outside);
        return mediumInterface.inside.lock();
    }

    Ray SpawnRay(const Vector3f& d) const {
        Point3f o = OffsetRayOrigin(p, pError, n, d);
        return Ray(o, d, Infinity, time);
    }

    Ray SpawnRayTo(const Point3f& p2) const {
        Point3f origin = OffsetRayOrigin(p, pError, n, p2 - p);
        Vector3f d = p2 - p; // 这里是实际长度，没有normalize。
        return Ray(origin, d, 1 - ShadowEpsilon, time, GetMedium(d));
    }

    Ray SpawnRayTo(const Interaction& it) const {
        Point3f origin = OffsetRayOrigin(p, pError, n, it.p - p);
        Point3f target = OffsetRayOrigin(it.p, it.pError, it.n, origin - it.p);
        Vector3f d = target - origin;
        return Ray(origin, d, 1 - ShadowEpsilon, time, GetMedium(d));
    }

    Ray SpawnRayThrough(const Point3f& p2) const {
        Point3f origin = OffsetRayOrigin(p, pError, n, p2 - p);
        Vector3f d = p2 - p;
        return Ray(origin, d, Infinity, time);
    }

    Point3f p;
    float time;

    Vector3f pError;
    Vector3f wo;
    Vector3f n; 
    Vector3f shape_n; // 对于shape的normal

    MediumInterface mediumInterface;

};

class SurfaceInteraction : public Interaction {
public:
    SurfaceInteraction() {}
    SurfaceInteraction(const Point3f& p,
        const Vector3f& pError,
        const Vector3f& shape_n,
        const Point2f& uv, const Vector3f& wo,
        const Vector3f& dpdu, const Vector3f& dpdv,
        float time,
        const Shape* sh, bool from_outside);

    //Ray SpawnRay(const Vector3f& d) const {
    //	Point3f o = OffsetRayOrigin(p, pError, n, d);
    //	return Ray(o, d, Infinity, time);
    //}

    //Ray SpawnRayTo(const Point3f& p2) const {
    //	Point3f origin = OffsetRayOrigin(p, pError, n, p2 - p);
    //	Vector3f d = p2 - p; // 这里是实际长度，没有normalize。
    //	//return Ray(origin, d, 1 - ShadowEpsilon, time);
    //	return Ray(origin, d, 1, time); // tMax限定为1
    //}

    //Ray SpawnRayThrough(const Point3f& p2) const {
    //	Point3f origin = OffsetRayOrigin(p, pError, n, p2 - p);
    //	Vector3f d = p2 - p;
    //	return Ray(origin, d, Infinity, time);
    //}

    void ComputeDifferentials(const RayDifferential& r) const;

    void ComputeScatteringFunctions(
        MemoryBlock& mb,
        const RayDifferential& ray,
        TransportMode mode = TransportMode::Radiance);

    std::shared_ptr<Medium> GetMedium();

    int GetHitObjectID();
    std::string GetHitObjectName();

    Spectrum Le(const Vector3f& w) const;

    void LogSelf();

    Point2f uv;
    Vector3f dpdu, dpdv;

    struct {
        Vector3f n; // Normal3f n;
        Vector3f dpdu, dpdv;
        //Normal3f dndu, dndv;
    } shading;

    const Shape* shape = nullptr;
    const Primitive* primitive = nullptr;

    BSDF* bsdf = nullptr;
    //std::shared_ptr<BSDF> bsdf = nullptr;

    mutable Vector3f dpdx, dpdy;
    mutable float dudx = 0, dvdx = 0, dudy = 0, dvdy = 0;

    bool from_outside; // 从模型外面还是里面打过来
};


class MediumInteraction : public Interaction {
public:
    // MediumInteraction Public Methods
    MediumInteraction() : phase(nullptr) {}
    MediumInteraction(const Point3f& p, const Vector3f& wo, float time,
        std::shared_ptr<Medium> medium, const PhaseFunction* phase)
        : Interaction(p, wo, time, medium), phase(phase) {}
    bool IsValid() const { return phase != nullptr; }

    // MediumInteraction Public Data
    const PhaseFunction* phase;
};


#endif