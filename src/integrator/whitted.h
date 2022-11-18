#ifndef CORE_INTEGRATOR_WHITTED_H
#define CORE_INTEGRATOR_WHITTED_H

#include "../core/geometry.h"
#include "../core/integrator.h"

class WhittedIntegrator : public SamplerIntegrator {
public:
    WhittedIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
        std::shared_ptr<Sampler> sampler,
        const BBox2i& pixelBounds)
        : SamplerIntegrator(camera, sampler, pixelBounds) {}

    Spectrum Li(MemoryBlock& mb, const RayDifferential& ray, Scene& scene, Sampler& sampler, int pool_id = 0, int depth = 0);

private:
};


#endif