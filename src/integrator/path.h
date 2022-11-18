#ifndef CORE_INTEGRATOR_PATH_H
#define CORE_INTEGRATOR_PATH_H

#include "../core/geometry.h"
#include "../core/integrator.h"
#include "../core/light.h"

class PathIntegrator : public SamplerIntegrator {
public:
    PathIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
        std::shared_ptr<Sampler> sampler,
        const BBox2i& pixelBounds, float rrThreshold = 1,
        const std::string& lightSampleStrategy = "uniform");

    void Preprocess(const Scene& scene, Sampler& sampler);

    Spectrum Li(MemoryBlock& mb, const RayDifferential& ray, Scene& scene, Sampler& sampler, int pool_id = 0, int depth = 0) ;

private:
    const float rrThreshold;
    const std::string lightSampleStrategy;
    std::unique_ptr<LightDistribution> lightDistribution;
};


#endif