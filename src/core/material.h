#ifndef CORE_MATERIAL_H
#define CORE_MATERIAL_H

#include "pbr.h"
#include "transform.h"
#include "spectrum.h"

enum class TransportMode { Radiance, Importance };

// Material Declarations
class Material {
public:
    // Material Interface
    virtual void ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes = false) const = 0;
    virtual ~Material() {};
    static void Bump(const std::shared_ptr<Texture<float>>& d, SurfaceInteraction* si);
};

/////////////////////////////////////////




class MatteMaterial : public Material {
public:
    // MatteMaterial Public Methods
    MatteMaterial() {}
    MatteMaterial(const std::shared_ptr<Texture<Spectrum>>& Kd,
        const std::shared_ptr<Texture<float>>& sigma,
        const std::shared_ptr<Texture<float>>& bumpMap)
        : Kd(Kd), sigma(sigma), bumpMap(bumpMap) {}
    void ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes = false) const;

private:
    // MatteMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kd;
    std::shared_ptr<Texture<float>> sigma, bumpMap;
};

/////////////////////////////////////////

class GlassMaterial : public Material {
public:
    // GlassMaterial Public Methods
    GlassMaterial(const std::shared_ptr<Texture<Spectrum>>& Kr,
        const std::shared_ptr<Texture<Spectrum>>& Kt,
        const std::shared_ptr<Texture<float>>& uRoughness,
        const std::shared_ptr<Texture<float>>& vRoughness,
        const std::shared_ptr<Texture<float>>& index,
        const std::shared_ptr<Texture<float>>& bumpMap,
        bool remapRoughness)
        : Kr(Kr),
        Kt(Kt),
        uRoughness(uRoughness),
        vRoughness(vRoughness),
        index(index),
        bumpMap(bumpMap),
        remapRoughness(remapRoughness) {}
    void ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes = false) const;

private:
    // GlassMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kr, Kt;
    std::shared_ptr<Texture<float>> uRoughness, vRoughness;
    std::shared_ptr<Texture<float>> index;
    std::shared_ptr<Texture<float>> bumpMap;
    bool remapRoughness;
};


///////////////////////

class MirrorMaterial : public Material {
public:
    // MirrorMaterial Public Methods
    MirrorMaterial(const std::shared_ptr<Texture<Spectrum>>& r,
        const std::shared_ptr<Texture<float>>& bump) {
        Kr = r;
        bumpMap = bump;
    }
    void ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const;

private:
    // MirrorMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kr;
    std::shared_ptr<Texture<float>> bumpMap;
};



#endif