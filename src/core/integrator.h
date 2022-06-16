#ifndef CORE_INTEGRATOR_H
#define CORE_INTEGRATOR_H

#include "geometry.h"
#include "interaction.h"
#include "scene.h"
#include "sampler.h"
#include "camera.h"

class Integrator {
public:
    // Integrator Interface
    virtual ~Integrator() {};
    virtual void Render(const Scene& scene) = 0;
};

class SamplerIntegrator : public Integrator {
public:
    // SamplerIntegrator Public Methods
    SamplerIntegrator(std::shared_ptr<Camera> camera, std::shared_ptr<Sampler> sampler, const BBox2i& pixelBounds)
        : camera(camera), sampler(sampler), pixelBounds(pixelBounds) {}

    virtual void Preprocess(const Scene& scene, Sampler& sampler) {}

    void Render(const Scene& scene);

    virtual Spectrum Li(const RayDifferential& ray, const Scene& scene, Sampler& sampler, int depth = 0) const = 0;

    Spectrum SpecularReflect(const RayDifferential& ray,
        const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const;

    /*Spectrum SpecularTransmit(const RayDifferential& ray,
        const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const;*/

protected:
    // SamplerIntegrator Protected Data
    std::shared_ptr<Camera> camera;

private:
    // SamplerIntegrator Private Data
    std::shared_ptr<Sampler> sampler;
    const BBox2i pixelBounds;
};

#endif