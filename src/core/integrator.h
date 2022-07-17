#ifndef CORE_INTEGRATOR_H
#define CORE_INTEGRATOR_H

#include<vector>
#include "geometry.h"
#include "interaction.h"
#include "scene.h"
#include "sampler.h"
#include "camera.h"
#include "../base/json.h"

class Integrator {
public:
    // Integrator Interface
    Integrator() :
        render_threads_count(3),
        render_progress_now(render_threads_count),
        render_progress_total(render_threads_count),
        render_status(0),
        has_new_photo(0),
        maxDepth(1) {};
    virtual ~Integrator() {};
    virtual void Render(const Scene& scene) = 0;

    void SetRayBounceNo(int n) {
        maxDepth = n + 1;
    }

    void SetRenderThreadsCount(int c) {
        render_threads_count = c;

        render_progress_now.resize(c);
        render_progress_total.resize(c);
    }

    int GetRenderProgress() {
        int progress = 0;
        for (auto p : render_progress_now) {
            progress += p;
        }

        return progress;
    }

    std::vector<int> render_progress_now, render_progress_total;
    int render_status;// , render_progress_now, render_progress_total;
    int has_new_photo;
    int maxDepth;

    int render_threads_count;


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

    Spectrum SpecularTransmit(const RayDifferential& ray,
        const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const;

protected:
    // SamplerIntegrator Protected Data
    std::shared_ptr<Camera> camera;

private:
    // SamplerIntegrator Private Data
    std::shared_ptr<Sampler> sampler;
    const BBox2i pixelBounds;
};

#endif