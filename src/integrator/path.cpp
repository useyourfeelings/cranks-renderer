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

        //break; // ���̵߳�������ʧ 2�߳�1.85��4�߳�3.x��8�߳�6��
        
        //std::cout << "hitScene " << hitScene << std::endl;

        // Possibly add emitted light at intersection
        if (bounces == 0 || specularBounce || !hitScene) {
            // Add emitted light at path vertex or from the environment
            if (hitScene) {
                //L += beta * isect.Le(-ray.d);
                //VLOG(2) << "Added Le -> L = " << L;
            }
            else {
                // ����specularBounce����һ�η����Ǿ��淴�䡣
                // ���淴���ǲ���ֱ�ӹ�ġ����������û��ײ�����Ǵ򵽻����ˡ��͵÷��ػ������������ȫ�ڡ�

                // bounces == 0
                // ����������۾�ֱ�ӿ���������

                // ������
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

        // ����material���bxdf
        isect.ComputeScatteringFunctions(mb, ray);

        auto distrib = lightDistribution->Lookup(isect.p);

        // Sample illumination from lights to find path contribution.
        // (But skip this for perfectly specular BSDFs.)
        if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) > 0) {
            //++totalPaths;

            // ����ֱ�ӹ⡣
            // �����й�Դ������ѡ��һ����Դ������bsdf���õ����Ĺ��ܡ�
            Spectrum Ld = beta * UniformSampleOneLight(isect, scene, sampler, pool_id, false, distrib);

            L += Ld;
        }

        Vector3f wo = -ray.d, wi;
        float pdf;
        BxDFType flags;

        //std::cout << "Sample_f " << std::endl;

        // ��wo��һ������⣬���������
        // ͬʱ�õ�һ�����������Ϊ��һ���path��
        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, BSDF_ALL, &flags);

        if (f.IsBlack() || pdf == 0.f)
            break;

        // ����beta
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