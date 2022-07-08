#ifndef CORE_INTEGRATOR_WHITTED_H
#define CORE_INTEGRATOR_WHITTED_H

#include "../core/geometry.h"
#include "../core/integrator.h"

class WhittedIntegrator : public SamplerIntegrator {
public:
    // WhittedIntegrator Public Methods
    WhittedIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
        std::shared_ptr<Sampler> sampler,
        const BBox2i& pixelBounds)
        : SamplerIntegrator(camera, sampler, pixelBounds) {}

    Spectrum Li(const RayDifferential& ray, const Scene& scene, Sampler& sampler, int depth = 0) const;

private:
    // WhittedIntegrator Private Data
    //int maxDepth;
};


#endif