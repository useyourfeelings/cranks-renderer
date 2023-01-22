#include "path.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"

PathIntegrator::PathIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
    std::shared_ptr<Sampler> sampler,
    const BBox2i& pixelBounds, float rrThreshold,
    const std::string& lightSampleStrategy)
    : SamplerIntegrator(camera, sampler, pixelBounds),
    rrThreshold(rrThreshold),
    lightSampleStrategy(lightSampleStrategy) {

    default_config = json({
        {"name", "path"},
        {"ray_sample_no", 1},
        {"ray_bounce_no", 10},
        {"render_threads_no", 12},
    });
    SetOptions(default_config);
}

void PathIntegrator::Preprocess(const Scene& scene, Sampler& sampler) {
    lightDistribution = CreateLightSampleDistribution(lightSampleStrategy, scene);
}

void PathIntegrator::SetOptions(const json& new_config) {
    std::cout << "PathIntegrator::SetOptions " << new_config;
    auto current_config = config;
    current_config.merge_patch(new_config);

    ray_sample_no = current_config["ray_sample_no"];
    sampler->SetSamplesPerPixel(current_config["ray_sample_no"]);
    SetRayBounceNo(current_config["ray_bounce_no"]);
    //SetRenderThreadsCount(current_config["render_threads_no"]);

    config = current_config;
}

//json PathIntegrator::GetConfig() {
//    json config;
//
//    config["ray_sample_no"] = ray_sample_no;
//    config["render_threads_no"] = render_threads_no;
//    config["ray_bounce_no"] = maxDepth;
//
//    return config;
//}

Spectrum PathIntegrator::Li(MemoryBlock& mb, const RayDifferential& r, Scene& scene, Sampler& sampler, int pool_id, int depth)  {
    Spectrum L(0.f), beta(1.f);
    RayDifferential ray(r);
    bool specularBounce = false;
    int bounces;

    for (bounces = 0;; ++bounces) {
        //std::cout << "bounce " << bounces << std::endl;
        SurfaceInteraction isect;
        float tHit;
        
        bool hitScene = scene.Intersect(ray, &tHit, &isect, pool_id);

        //break; // 多线程到此有损失 2线程1.85。4线程3.x。8线程6。
        
        //std::cout << "hitScene " << hitScene << std::endl;

        // Possibly add emitted light at intersection
        if (bounces == 0 || specularBounce || !hitScene) {
            // Add emitted light at path vertex or from the environment
            if (hitScene) {
                //L += beta * isect.Le(-ray.d);
                //VLOG(2) << "Added Le -> L = " << L;
            }
            else {
                // 对于specularBounce，上一次反射是镜面反射。
                // 镜面反射是不加直接光的。到这里如果没碰撞，就是打到环境了。就得返回环境。否则就是全黑。

                // bounces == 0
                // 特殊情况，眼睛直接看到环境。

                // 环境光
                if (0) {
                    auto le = GetFakeSky(ray.d.z);
                    L += beta * le;
                }
                else {
                    for (const auto& light : scene.infiniteLights) {
                        auto le = light->Le(ray);
                        L += beta * le;
                    }
                }
            }
        }

        if (!hitScene || bounces >= maxDepth) {
            //L += GetSky(ray.d.z) *0.5;
            break;
        }

        // 根据material添加bxdf
        isect.ComputeScatteringFunctions(mb, ray);

        auto distrib = lightDistribution->Lookup(isect.p);

        // Sample illumination from lights to find path contribution.
        // (But skip this for perfectly specular BSDFs.)
        if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) > 0) {
            //++totalPaths;

            // 计算直接光。
            // 对所有光源采样，选出一个光源，计算bsdf。得到最后的光能。
            Spectrum Ld = beta * UniformSampleOneLight(isect, scene, sampler, pool_id, false, distrib);

            L += Ld;
        }

        Vector3f wo = -ray.d, wi;
        float pdf;
        BxDFType flags;

        //std::cout << "Sample_f " << std::endl;

        // 以wo采一束入射光，算出能量。
        // 同时得到一个随机方向，作为下一层的path。
        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, BSDF_ALL, &flags);

        if (f.IsBlack() || pdf == 0.f)
            break;

        // 更新beta
        beta *= f * AbsDot(wi, isect.shading.n) / pdf;

        // rr
        float q = 1 - beta.MaxComponentValue();
        if (q > 0.8 and bounces >= 3) {
            if (sampler.Get1D() < q)
                break;

            beta /= 1 - q;
        }

        specularBounce = (flags & BSDF_SPECULAR) != 0;

        ray = isect.SpawnRay(wi);
    }

    return L;
}