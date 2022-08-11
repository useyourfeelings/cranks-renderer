#ifndef CORE_INTEGRATOR_H
#define CORE_INTEGRATOR_H

#include<vector>
#include "geometry.h"
#include "interaction.h"
#include "scene.h"
#include "sampler.h"
#include "camera.h"
#include"../tool/json.h"

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

    Spectrum GetFakeSky(float dz) const {
        Spectrum sky_spectrum;
        /*sky_spectrum.c[0] = 0.6;
        sky_spectrum.c[1] = 0.7;
        sky_spectrum.c[2] = 0.9;*/

        // https://raytracing.github.io/books/RayTracingInOneWeekend.html
        // gradient sky

        float t = (dz + 1) * 0.5; // map to [0, 1]
        /*sky_spectrum.c[0] = 1 - t + t * 0.5;
        sky_spectrum.c[1] = 1 - t + t * 0.7;
        sky_spectrum.c[2] = 1 - t + t * 1;*/

        sky_spectrum.c[0] = 1 - t * 0.5;
        sky_spectrum.c[1] = 1 - t * 0.3;
        sky_spectrum.c[2] = 1;

        return sky_spectrum;
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


//////////////////////////////

Spectrum UniformSampleOneLight(const Interaction& it, const Scene& scene,
    Sampler& sampler,
    bool handleMedia = false,
    const Distribution1D* lightDistrib = nullptr);

Spectrum EstimateDirect(const Interaction& it, const Point2f& uShading,
    const Light& light, const Point2f& uLight,
    const Scene& scene, Sampler& sampler,
    bool handleMedia = false,
    bool specular = false);

#endif