#ifndef CORE_INTEGRATOR_H
#define CORE_INTEGRATOR_H

#include<vector>
#include "geometry.h"
#include "interaction.h"
#include "scene.h"
#include "sampler.h"
#include "camera.h"
#include"../tool/json.h"
#include"../base/memory.h"

struct MultiTaskArg;

class Integrator {
public:
    // Integrator Interface
    Integrator() :
        render_threads_no(3),
        render_progress_now(render_threads_no, 0),
        render_progress_total(render_threads_no, 0),
        render_status(0),
        render_duration(0),
        has_new_photo(0),
        maxDepth(1) {
    };

    virtual ~Integrator() {};
    virtual void Render(Scene& scene) = 0;

    virtual void SetOptions(const json& data) {

    }

    void SetRayBounceNo(int n) {
        maxDepth = n + 1;
    }

    void SetRenderThreadsCount(int c) {
        render_threads_no = c;

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

    virtual json GetRenderStatus() {
        json status;
        if (render_status == 1)
            render_duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - render_start_time).count());
        
        status["render_duration"] = render_duration;

        return status;
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

    int render_threads_no; // order is important
    std::vector<int> render_progress_now, render_progress_total;
    int render_status;// , render_progress_now, render_progress_total;
    int has_new_photo;
    int maxDepth;
    std::chrono::steady_clock::time_point render_start_time;
    float render_duration; // s
};

class SamplerIntegrator : public Integrator {
public:
    // SamplerIntegrator Public Methods
    SamplerIntegrator(std::shared_ptr<Camera> camera, std::shared_ptr<Sampler> sampler, const BBox2i& pixelBounds)
        : camera(camera), sampler(sampler), pixelBounds(pixelBounds) {}

    virtual void Preprocess(const Scene& scene, Sampler& sampler) {}

    void Render(Scene& scene);

    virtual Spectrum Li(MemoryBlock &mb, const RayDifferential& ray, Scene& scene, Sampler& sampler, int pool_id = 0, int depth = 0) = 0;

    Spectrum SpecularReflect(MemoryBlock& mb, const RayDifferential& ray,
        const SurfaceInteraction& isect,
        Scene& scene, Sampler& sampler, int pool_id, int depth);

    Spectrum SpecularTransmit(MemoryBlock& mb, const RayDifferential& ray,
        const SurfaceInteraction& isect,
        Scene& scene, Sampler& sampler, int pool_id, int depth);

protected:
    // SamplerIntegrator Protected Data
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Sampler> sampler;

private:
    // SamplerIntegrator Private Data
    
    const BBox2i pixelBounds;
};


//////////////////////////////

Spectrum UniformSampleOneLight(const Interaction& it, Scene& scene,
    Sampler& sampler, int pool_id = 0,
    bool handleMedia = false,
    const Distribution1D* lightDistrib = nullptr);

Spectrum EstimateDirect(const Interaction& it, const Point2f& uShading,
    const Light& light, const Point2f& uLight,
    Scene& scene, Sampler& sampler, int pool_id = 0,
    bool handleMedia = false,
    bool specular = false);

#endif