#include "vpath.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"

VolumePathIntegrator::VolumePathIntegrator(int maxDepth, std::shared_ptr<Camera> camera,
    std::shared_ptr<Sampler> sampler,
    const BBox2i& pixelBounds, float rrThreshold,
    const std::string& lightSampleStrategy)
    : SamplerIntegrator(camera, sampler, pixelBounds),
    rrThreshold(rrThreshold),
    lightSampleStrategy(lightSampleStrategy) {

    default_config = json({
        {"name", "vpath"},
        {"ray_sample_no", 1},
        {"ray_bounce_no", 10},
        {"render_threads_no", 12},
    });

    SetOptions(default_config);
}

void VolumePathIntegrator::Preprocess(const Scene& scene, Sampler& sampler) {
    lightDistribution = CreateLightSampleDistribution(lightSampleStrategy, scene);
}

void VolumePathIntegrator::SetOptions(const json& new_config) {
    std::cout << "VolumePathIntegrator::SetOptions " << new_config;
    auto current_config = config;
    current_config.merge_patch(new_config);

    ray_sample_no = current_config["ray_sample_no"];
    sampler->SetSamplesPerPixel(current_config["ray_sample_no"]);
    SetRayBounceNo(current_config["ray_bounce_no"]);
    //SetRenderThreadsCount(current_config["render_threads_no"]);

    config = current_config;
}


// 物体介质关系如何安排？物体存在叠加、包含等。
// 先给定初始位置，以及所处的介质。光打出去，如果碰不到，结束。
// 如果碰到一个背面(normal反向)，采用这个物体的内部介质。
// 如果碰到一个正面，采用当前的介质。
// 这样实际是不对的，表现会不一致。但好做。一般够用？

Spectrum VolumePathIntegrator::Li(MemoryBlock& mb, const RayDifferential& r, Scene& scene, Sampler& sampler, int pool_id, int depth) {
    Spectrum L(0.f), beta(1.f);
    RayDifferential ray(r);
    bool specularBounce = false;
    int bounces;

    for (bounces = 0;; ++bounces) {
        //std::cout << "bounce " << bounces << std::endl;
        SurfaceInteraction isect;
        float tHit;

        // 先算可能的交点。如果得到某个面上的点。ray.tMax置为两点间距离。
        bool hitScene = scene.Intersect(ray, &tHit, &isect, pool_id);

        // 对介质的transmittance采样一个距离，得到一个点，得到对应的衰减比例。
        // 采样的平均距离为1/sigma。
        MediumInteraction mi;
        if (auto medium = ray.medium.lock())
            beta *= medium->Sample(ray, sampler, mb, &mi, medium);

        if (beta.IsBlack())
            break;

        if (mi.IsValid()) {
            // 采样到了介质中间

            if (bounces >= maxDepth)
                break;

            // 采样直接光，算出光源对这一点的能量贡献。
            auto distrib = lightDistribution->Lookup(mi.p);
            L += beta * UniformSampleOneLight(mi, scene, sampler, pool_id, true, distrib);

            // 采样phase函数，得到一个方向，作为in-scattering光的来源代表。进行跟踪，计算下一个path。
            // 这里按phase函数的概率，是不需要scale能量的。
            // 大量随机操作下，最后的分布天然符合给定的phase函数。
            Vector3f wo = -ray.d, wi;
            mi.phase->Sample_p(wo, &wi, sampler.Get2D());
            ray = mi.SpawnRay(wi);
        }
        else {
            // 碰到了面

            //++surfaceInteractions;
            // Handle scattering at point on surface for volumetric path tracer

            // Possibly add emitted light at intersection
            if (bounces == 0 || specularBounce || !hitScene) {
                // Add emitted light at path vertex or from the environment
                if (hitScene) {
                    //L += beta * isect.Le(-ray.d);
                }
                else {
                    for (const auto& light : scene.infiniteLights)
                        L += beta * light->Le(ray);
                }
            }

            // Terminate path if ray escaped or _maxDepth_ was reached
            if (!hitScene || bounces >= maxDepth)
                break;

            // 根据material添加bxdf
            isect.ComputeScatteringFunctions(mb, ray);

            auto distrib = lightDistribution->Lookup(isect.p);

            
            //L += beta * UniformSampleOneLight(isect, scene, sampler, pool_id, true, lightDistrib);
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) > 0) {
                //++totalPaths;

                // 计算直接光。
                // 对所有光源采样，选出一个光源，计算bsdf。得到最后的光能。
                Spectrum Ld = beta * UniformSampleOneLight(isect, scene, sampler, pool_id, true, distrib);

                L += Ld;
            }

            // Sample BSDF to get new path direction
            Vector3f wo = -ray.d, wi;
            float pdf;
            BxDFType flags;
            Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, BSDF_ALL, &flags);
            if (f.IsBlack() || pdf == 0.f) 
                break;

            beta *= f * AbsDot(wi, isect.shading.n) / pdf;

            //DCHECK(std::isinf(beta.y()) == false);
            specularBounce = (flags & BSDF_SPECULAR) != 0;
            
            /*
            if ((flags & BSDF_SPECULAR) && (flags & BSDF_TRANSMISSION)) {
                float eta = isect.bsdf->eta;
                // Update the term that tracks radiance scaling for refraction
                // depending on whether the ray is entering or leaving the
                // medium.
                etaScale *= (Dot(wo, isect.n) > 0) ? (eta * eta) : 1 / (eta * eta);
            }*/


            ray = isect.SpawnRay(wi);

            /*
            // Account for attenuated subsurface scattering, if applicable
            if (isect.bssrdf && (flags & BSDF_TRANSMISSION)) {
                // Importance sample the BSSRDF
                SurfaceInteraction pi;
                Spectrum S = isect.bssrdf->Sample_S(
                    scene, sampler.Get1D(), sampler.Get2D(), arena, &pi, &pdf);
                DCHECK(std::isinf(beta.y()) == false);
                if (S.IsBlack() || pdf == 0) break;
                beta *= S / pdf;

                // Account for the attenuated direct subsurface scattering
                // component
                L += beta *
                    UniformSampleOneLight(pi, scene, arena, sampler, true,
                                          lightDistribution->Lookup(pi.p));

                // Account for the indirect subsurface scattering component
                Spectrum f = pi.bsdf->Sample_f(pi.wo, &wi, sampler.Get2D(),
                                               &pdf, BSDF_ALL, &flags);
                if (f.IsBlack() || pdf == 0) break;
                beta *= f * AbsDot(wi, pi.shading.n) / pdf;
                DCHECK(std::isinf(beta.y()) == false);
                specularBounce = (flags & BSDF_SPECULAR) != 0;
                ray = pi.SpawnRay(wi);
            }*/
        }

        // 更新beta
        //beta *= f * AbsDot(wi, isect.shading.n) / pdf;

        // rr
        float q = 1 - beta.MaxComponentValue();
        if (q > 0.8 and bounces >= 3) {
            if (sampler.Get1D() < q)
                break;

            beta /= 1 - q;
        }

        //ray = isect.SpawnRay(wi);
    }

    return L;
}