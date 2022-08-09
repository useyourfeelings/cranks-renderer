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
    lightSampleStrategy(lightSampleStrategy) {}

void PathIntegrator::Preprocess(const Scene& scene, Sampler& sampler) {
    lightDistribution = CreateLightSampleDistribution(lightSampleStrategy, scene);
}

Spectrum PathIntegrator::Li(const RayDifferential& r, const Scene& scene, Sampler& sampler, int depth) const {
    Spectrum L(0.f), beta(1.f);
    RayDifferential ray(r);
    bool specularBounce = false;
    int bounces;

    for (bounces = 0;; ++bounces) {
        //std::cout << "bounce " << bounces << std::endl;
        SurfaceInteraction isect;
        float tHit;
        bool hitScene = scene.Intersect(ray, &tHit, &isect);

        //std::cout << "hitScene " << hitScene << std::endl;

        // Possibly add emitted light at intersection
        if (bounces == 0 || specularBounce) {
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
                //L += GetFakeSky(ray.d.z);
                for (const auto& light : scene.infiniteLights) {
                    auto le = light->Le(ray);
                    L += beta * le;
                }
                    
            }
        }

        if (!hitScene || bounces >= maxDepth) {
            //L += GetSky(ray.d.z) *0.5;
            break;
        }

        isect.ComputeScatteringFunctions(ray);

        auto distrib = lightDistribution->Lookup(isect.p);

        // Sample illumination from lights to find path contribution.
        // (But skip this for perfectly specular BSDFs.)
        if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) > 0) {
            //++totalPaths;

            // 计算直接光。
            // 对所有光源采样，选出一个光源，计算bsdf。得到最后的光能。
            Spectrum Ld = beta * UniformSampleOneLight(isect, scene, sampler, false, distrib);

            //VLOG(2) << "Sampled direct lighting Ld = " << Ld;
            //if (Ld.IsBlack()) ++zeroRadiancePaths;
            //CHECK_GE(Ld.y(), 0.f);
            L += Ld;
        }

        Vector3f wo = -ray.d, wi;
        float pdf;
        BxDFType flags;

        //std::cout << "Sample_f " << std::endl;

        // 以wo采一束光，算出能量。得到一个随机方向，作为下一层的path。
        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, BSDF_ALL, &flags);

        if (f.IsBlack() || pdf == 0.f)
            break;

        // 更新beta
        beta *= f * AbsDot(wi, isect.shading.n) / pdf;

        specularBounce = (flags & BSDF_SPECULAR) != 0;

        ray = isect.SpawnRay(wi);
    }

    return L;
}