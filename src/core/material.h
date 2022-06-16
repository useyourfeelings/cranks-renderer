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
    virtual void ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode) const = 0;
    virtual ~Material() {};
    static void Bump(const std::shared_ptr<Texture<float>>& d, SurfaceInteraction* si);
};

/////////////////////////////////////////




class MatteMaterial : public Material {
public:
    // MatteMaterial Public Methods
    MatteMaterial(const std::shared_ptr<Texture<Spectrum>>& Kd,
        const std::shared_ptr<Texture<float>>& sigma,
        const std::shared_ptr<Texture<float>>& bumpMap)
        : Kd(Kd), sigma(sigma), bumpMap(bumpMap) {}
    void ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode) const;

private:
    // MatteMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kd;
    std::shared_ptr<Texture<float>> sigma, bumpMap;
};









#endif