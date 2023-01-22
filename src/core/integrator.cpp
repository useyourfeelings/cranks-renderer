#include <iostream>
#include "integrator.h"
#include "reflection.h"
#include "film.h"
#include "../tool/logger.h"
#include "../tool/json.h"
#include "../base/events.h"
#include "../base/memory.h"

void SamplerIntegrator::Render(Scene& scene) {
    std::cout << std::format("SamplerIntegrator::Render\n");

    Log("Render");
    render_status = 1;
    render_start_time = std::chrono::high_resolution_clock::now();
    has_new_photo = 0;
    Preprocess(scene, *sampler);
    // Render image tiles in parallel

    // Compute number of tiles, _nTiles_, to use for parallel rendering
    //BBox2i sampleBounds = this->pixelBounds;// camera->film->GetSampleBounds();
    BBox2i sampleBounds = BBox2i(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));
    // sampleBounds 类似600 400

    //this->render_progress_total = camera->resolutionX * camera->resolutionY;
    //this->render_progress_now = 0;
    this->render_progress_now.resize(render_threads_no);
    this->render_progress_total.resize(render_threads_no);
    for (int i = 0; i < render_threads_no; ++ i) {
        render_progress_now[i] = 0;
    }

    MultiTaskCtx tast_args(render_threads_no);
    tast_args.x_start = sampleBounds.pMin.x;
    tast_args.y_start = sampleBounds.pMin.y;
    tast_args.x_end = sampleBounds.pMax.x - 1;
    tast_args.y_end = sampleBounds.pMax.y - 1;

    // https://stackoverflow.com/questions/4324763/can-we-have-functions-inside-functions-in-c
    // https://en.cppreference.com/w/cpp/language/lambda

    auto manager_func = [](int task_index, MultiTaskCtx args) {
        int x_interval = (args.x_end - args.x_start + 1) / args.task_count;
        if ((args.x_end - args.x_start + 1) % args.task_count != 0)
            x_interval++;

        int x_start = args.x_start + x_interval * task_index;
        int x_end = std::min(args.x_end, (x_start + x_interval - 1));

        args.task_index = task_index;
        args.y_start = args.y_start;
        args.x_start = x_start;
        args.y_end = args.y_end;
        args.x_end = x_end;
        args.task_progress_total = (x_end - x_start + 1) * (args.y_end - args.y_start + 1);

        return args;
    };

    auto render_task_lambda = [&](MultiTaskCtx args) {
        int i = args.x_start;
        int j = args.y_start;
        int task_index = args.task_index;

        std::cout << std::format("--- render_task {} i {} j {}\n", task_index, i, j);

        MemoryBlock mb;

        // 之后的计算存在多处内存分配(计算bsdf等环节)。非常卡。
        // 可以做共用的memory block来复用，避免频繁申请/释放内存。
        // 同一个线程中流程是顺序执行的，可以不考虑同步问题。
        // 一个线程只申请一组block。比如一次Li()有10个操作，那么第一次会实际申请10次。
        // 第一次Li()完成后，10个block中的数据就没用了。所有block标记为可用。
        // 进行第二次Li()，如果第二次的10个操作中需要的内存可被之前申请的block满足，那么可以复用，否则申请新的block。依此类推。
        // 如果各个操作需要的内存大小处在一个较小范围内。就能高效复用。

        std::unique_ptr<Sampler> local_sampler = sampler->Clone();

        this->render_progress_total[task_index] = args.task_progress_total;

        int progress_interval = args.task_progress_total / 40; // 分xx级
        int progress = 0;
        int next_progress_to_report = progress_interval;

        // 一列列算
        for (int i = args.x_start; i <= args.x_end; ++i) {
            for (int j = args.y_start; j <= args.y_end; ++j) {

                if (render_status == 0)
                    break;

                progress++;

                if (progress > next_progress_to_report) {
                    this->render_progress_now[task_index] = progress;
                    next_progress_to_report += progress_interval;
                }

                Point2i pixel(i, j);

                local_sampler->StartPixel(pixel);

                do {
                    CameraSample cameraSample = local_sampler->GetCameraSample(pixel);

                    RayDifferential ray;
                    float rayWeight = camera->GenerateRayDifferential(cameraSample, &ray);
                    ray.ScaleDifferentials(1 / std::sqrt((float)local_sampler->samplesPerPixel));

                    Spectrum L(0.f);
                    if (rayWeight > 0)
                        L = Li(mb, ray, scene, *local_sampler, task_index);

                    camera->AddSample(Point2f(pixel.x, pixel.y), L, rayWeight, local_sampler->samplesPerPixel);

                    mb.Reset();
                } while (local_sampler->StartNextSample());
            }

            // 每完成一列。如果自己还剩有一定的任务，尝试请已经结束的线程帮忙算。
            
            // 检查其他线程进度，如果有已完成的，发event给控制线程请求帮助。
            // 阻塞等待结果。控制线程判断确实可以分任务，创建新线程执行，此处更新x_end即可。
            if (args.task_count > 1) {
                if ((args.x_end - i > 10)){// && (float(i - args.x_start) / (args.x_end - args.x_start)) < 0.95) {
                    int t = 0;
                    for (; t < args.task_count; ++t) {
                        if (t == task_index) // skip self
                            continue;

                        if (this->render_progress_total[t] == this->render_progress_now[t])
                            break;
                    }

                    if (t < args.task_count) { // possible helper
                        //std::cout << "MultiTaskCallHelp 1\n";
                        auto help_x_start = (i + args.x_end) / 2 + 1;
                        auto help_x_end = args.x_end;
                        int result = MultiTaskCallHelp(args, help_x_start, help_x_end);
                        if (result == 1) {
                            args.x_end = help_x_start - 1;

                            this->render_progress_total[task_index] = (args.x_end - args.x_start + 1) * (args.y_end - args.y_start + 1);
                        }
                    }
                }
            }
        }

        this->render_progress_now[task_index] = args.task_progress_total;
    };

    auto start_time = std::chrono::high_resolution_clock::now();

    //StartMultiTask(render_task_lambda, manager_func, tast_args);

    StartMultiTask2(render_task_lambda, manager_func, tast_args);

    float duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time).count());
    std::cout << std::format("----------- task_count = {} duration = {}\n", int(tast_args.task_count), duration);

    // Save final image after rendering
    camera->film->WriteImage();
    camera->films.push_back(camera->film);
    camera->film.reset();
    render_status = 0;
    has_new_photo = 1;
}

Spectrum SamplerIntegrator::SpecularReflect(
    MemoryBlock& mb,
    const RayDifferential& ray, const SurfaceInteraction& isect,
    Scene& scene, Sampler& sampler, int pool_id, int depth) {

    //Log("SpecularReflect");

    // Compute specular reflection direction _wi_ and BSDF value
    Vector3f wo = isect.wo;
    Vector3f wi;
    float pdf;
    BxDFType type = BxDFType(BSDF_REFLECTION | BSDF_SPECULAR);

    // 采样一束光。只取镜面反射的bxdf。对于镜面反射bxdf，最后走的就是fresnel。
    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, type);

    // Return contribution of specular reflection
    const Vector3f& ns = isect.shading.n;
    if (pdf > 0.f && !f.IsBlack() && std::abs(Dot(wi, ns)) != 0.f) {

        // Compute ray differential _rd_ for specular reflection
        RayDifferential rd = isect.SpawnRay(wi);
        //if (ray.hasDifferentials) {
        //    rd.hasDifferentials = true;
        //    rd.rxOrigin = isect.p + isect.dpdx;
        //    rd.ryOrigin = isect.p + isect.dpdy;
        //    // Compute differential reflected directions
        //    Vector3f dndx = isect.shading.dndu * isect.dudx +
        //        isect.shading.dndv * isect.dvdx;
        //    Vector3f dndy = isect.shading.dndu * isect.dudy +
        //        isect.shading.dndv * isect.dvdy;
        //    Vector3f dwodx = -ray.rxDirection - wo,
        //        dwody = -ray.ryDirection - wo;
        //    float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
        //    float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);
        //    rd.rxDirection =
        //        wi - dwodx + 2.f * Vector3f(Dot(wo, ns) * dndx + dDNdx * ns);
        //    rd.ryDirection =
        //        wi - dwody + 2.f * Vector3f(Dot(wo, ns) * dndy + dDNdy * ns);
        //}

        // 下一轮Li
        return f * Li(mb, rd, scene, sampler, pool_id, depth + 1) * std::abs(Dot(wi, ns)) / pdf;
    }
    else
        return Spectrum(0.f);
}

Spectrum SamplerIntegrator::SpecularTransmit(
    MemoryBlock& mb, 
    const RayDifferential& ray, const SurfaceInteraction& isect,
    Scene& scene, Sampler& sampler, int pool_id, int depth)  {

    Vector3f wo = isect.wo, wi;
    float pdf;
    const Point3f& p = isect.p;
    const BSDF& bsdf = *isect.bsdf;
    Spectrum f = bsdf.Sample_f(wo, &wi, sampler.Get2D(), &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
    Spectrum L = Spectrum(0.f);
    Vector3f ns = isect.shading.n;
    if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f) {
        // Compute ray differential _rd_ for specular transmission
        RayDifferential rd = isect.SpawnRay(wi);

#if 0
        if (ray.hasDifferentials) {
            rd.hasDifferentials = true;
            rd.rxOrigin = p + isect.dpdx;
            rd.ryOrigin = p + isect.dpdy;

            Normal3f dndx = isect.shading.dndu * isect.dudx +
                isect.shading.dndv * isect.dvdx;
            Normal3f dndy = isect.shading.dndu * isect.dudy +
                isect.shading.dndv * isect.dvdy;

            // The BSDF stores the IOR of the interior of the object being
            // intersected.  Compute the relative IOR by first out by
            // assuming that the ray is entering the object.
            Float eta = 1 / bsdf.eta;
            if (Dot(wo, ns) < 0) {
                // If the ray isn't entering, then we need to invert the
                // relative IOR and negate the normal and its derivatives.
                eta = 1 / eta;
                ns = -ns;
                dndx = -dndx;
                dndy = -dndy;
            }

            /*
              Notes on the derivation:
              - pbrt computes the refracted ray as: \wi = -\eta \omega_o + [ \eta (\wo \cdot \N) - \cos \theta_t ] \N
                It flips the normal to lie in the same hemisphere as \wo, and then \eta is the relative IOR from
                \wo's medium to \wi's medium.
              - If we denote the term in brackets by \mu, then we have: \wi = -\eta \omega_o + \mu \N
              - Now let's take the partial derivative. (We'll use "d" for \partial in the following for brevity.)
                We get: -\eta d\omega_o / dx + \mu dN/dx + d\mu/dx N.
              - We have the values of all of these except for d\mu/dx (using bits from the derivation of specularly
                reflected ray deifferentials).
              - The first term of d\mu/dx is easy: \eta d(\wo \cdot N)/dx. We already have d(\wo \cdot N)/dx.
              - The second term takes a little more work. We have:
                 \cos \theta_i = \sqrt{1 - \eta^2 (1 - (\wo \cdot N)^2)}.
                 Starting from (\wo \cdot N)^2 and reading outward, we have \cos^2 \theta_o, then \sin^2 \theta_o,
                 then \sin^2 \theta_i (via Snell's law), then \cos^2 \theta_i and then \cos \theta_i.
              - Let's take the partial derivative of the sqrt expression. We get:
                1 / 2 * 1 / \cos \theta_i * d/dx (1 - \eta^2 (1 - (\wo \cdot N)^2)).
              - That partial derivatve is equal to:
                d/dx \eta^2 (\wo \cdot N)^2 = 2 \eta^2 (\wo \cdot N) d/dx (\wo \cdot N).
              - Plugging it in, we have d\mu/dx =
                \eta d(\wo \cdot N)/dx - (\eta^2 (\wo \cdot N) d/dx (\wo \cdot N))/(-\wi \cdot N).
             */
            Vector3f dwodx = -ray.rxDirection - wo,
                dwody = -ray.ryDirection - wo;
            Float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
            Float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);

            Float mu = eta * Dot(wo, ns) - AbsDot(wi, ns);
            Float dmudx =
                (eta - (eta * eta * Dot(wo, ns)) / AbsDot(wi, ns)) * dDNdx;
            Float dmudy =
                (eta - (eta * eta * Dot(wo, ns)) / AbsDot(wi, ns)) * dDNdy;

            rd.rxDirection =
                wi - eta * dwodx + Vector3f(mu * dndx + dmudx * ns);
            rd.ryDirection =
                wi - eta * dwody + Vector3f(mu * dndy + dmudy * ns);
        }
#endif
        L = f * Li(mb, rd, scene, sampler, pool_id, depth + 1) * AbsDot(wi, ns) / pdf;
    }
    return L;
}


////////////////////

// 对全局的光源进行平均采样。选出一个光源。
Spectrum UniformSampleOneLight(const Interaction& it, Scene& scene,
    Sampler& sampler, int pool_id,
    bool handleMedia, const Distribution1D* lightDistrib) {

    int nLights = int(scene.lights.size());

    if (nLights == 0)
        return Spectrum(0.f);

    int lightIndex = 0;
    float lightPdf = 1;

    // 随机采样一个灯
    if (lightDistrib) {
        lightIndex = lightDistrib->SampleDiscrete(sampler.Get1D(), &lightPdf);
        if (lightPdf == 0)
            return Spectrum(0.f);
    }
    else {
        //lightNum = std::min((int)(sampler.Get1D() * nLights), nLights - 1);
        //lightPdf = Float(1) / nLights;
    }

    const std::shared_ptr<Light>& light = scene.lights[lightIndex];
    Point2f uLight = sampler.Get2D();
    Point2f uScattering = sampler.Get2D();
    return EstimateDirect(it, uScattering, *light, uLight, scene, sampler, pool_id, handleMedia) / lightPdf;
}

// 计算光在两点之间的衰减。
// 如果中间有物体，直接返回0。
// 这里也是简化的。如果中间的物体也是透明介质呢？
Spectrum RayTr(const Interaction& p0, const Interaction& p1, Scene& scene, Sampler& sampler, int pool_id) {
    Ray ray(p0.SpawnRayTo(p1));

    //if(!ray.medium.lock())


    Spectrum Tr(1.f);
    while (true) {
        SurfaceInteraction isect;
        float tHit;

        bool hitSurface = scene.Intersect(ray, &tHit, &isect, pool_id);
        
        // 碰到物体返回0
        if (hitSurface)// && isect.primitive->GetMaterial() != nullptr)
            return Spectrum(0.0f);

        // 算剩余能量
        if (auto m = ray.medium.lock()) {
            Tr *= m->Tr(ray, sampler);
        }
            

        // Generate next ray segment or return final transmittance
        if (!hitSurface)
            break;

        //ray = isect.SpawnRayTo(p1);
    }
    return Tr;
}

// 计算一个直接光源作用于交点，产生的光能。
Spectrum EstimateDirect(const Interaction& it, const Point2f& uScattering,
    const Light& light, const Point2f& uLight,
    Scene& scene, Sampler& sampler, int pool_id,
    bool handleMedia, bool specular) {

    BxDFType bsdfFlags = specular ? BSDF_ALL : BxDFType(BSDF_ALL & ~BSDF_SPECULAR);
    Spectrum Ld(0.f);

    // Sample light source with multiple importance sampling
    Vector3f wi;
    float lightPdf = 0, scatteringPdf = 0;
    //VisibilityTester visibility;

    // 先得到直接光的初始值。有可能被遮挡。后续再判断。
    // 光打到交点。算出wi，采样出光能，对应pdf。
    Spectrum Li = light.Sample_Li(it, uLight, &wi, &lightPdf);

    const SurfaceInteraction& isect = (const SurfaceInteraction&)it;

    if (lightPdf > 0 && !Li.IsBlack()) {

        Spectrum f;
        if (it.IsSurfaceInteraction()) {
            // Evaluate BSDF for light sampling strategy
            
            // 计算所有bxdf在(wo, wi)方向贡献的光
            f = isect.bsdf->f(isect.wo, wi, bsdfFlags) * AbsDot(wi, isect.shading.n);

            // 所有bxdf的pdf平均值
            scatteringPdf = isect.bsdf->Pdf(isect.wo, wi, bsdfFlags);
            //VLOG(2) << "  surf f*dot :" << f << ", scatteringPdf: " << scatteringPdf;

        }
        else {
            // medium

            // 已知光源入射方向和介质中所求光的传播方向。算phase值
            const MediumInteraction& mi = (const MediumInteraction&)it;
            float p = mi.phase->p(mi.wo, wi);
            f = Spectrum(p);
            scatteringPdf = p;
        }

        if (!f.IsBlack()) {
            // Compute effect of visibility for light source sample
            if (handleMedia) {
                //Li *= visibility.Tr(scene, sampler);
                //VLOG(2) << "  after Tr, Li: " << Li;

                // 对于介质。计算衰减。

                Li *= RayTr(it, Interaction(light.pos, it.time, it.mediumInterface), scene, sampler, pool_id);
            }
            else {
                // 如果遮挡
                Ray temp_ray = isect.SpawnRayTo(light.pos);

                SurfaceInteraction temp_isect;
                float temp_t;
                auto ires = scene.Intersect(temp_ray, &temp_t, &temp_isect, pool_id);
                if (ires) {
                    Li = Spectrum(0.f);
                }
            }

            // Add light's contribution to reflected radiance
            if (!Li.IsBlack()) {
                if (light.IsDeltaLight())
                    Ld += f * Li / lightPdf;
                else {
                    float weight = PowerHeuristic(1, lightPdf, 1, scatteringPdf); // todo??
                    auto add = f * Li * weight / lightPdf;
                    Ld += add ;
                }
            }
        }


    }

    if (!light.IsDeltaLight()) {
        // todo
    }


    return Ld;

}

