#include <iostream>
#include "integrator.h"
#include "reflection.h"
#include "film.h"
#include "../tool/logger.h"
#include "../base/json.h"
#include "../base/events.h"

void SamplerIntegrator::Render(const Scene& scene) {
    Log("Render");
    render_status = 1;
    has_new_photo = 0;
    Preprocess(scene, *sampler);
    // Render image tiles in parallel

    // Compute number of tiles, _nTiles_, to use for parallel rendering
    //BBox2i sampleBounds = this->pixelBounds;// camera->film->GetSampleBounds();
    BBox2i sampleBounds = BBox2i(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));
    // sampleBounds 类似600 400

    //this->render_progress_total = camera->resolutionX * camera->resolutionY;
    //this->render_progress_now = 0;
    this->render_progress_now.resize(render_threads_count);
    this->render_progress_total.resize(render_threads_count);
    for (int i = 0; i < render_threads_count; ++ i) {
        render_progress_now[i] = 0;
    }

    json tast_args({ 
        { "task_count", render_threads_count },
        { "x_start",sampleBounds.pMin.x },
        { "y_start",sampleBounds.pMin.y },
        { "x_end",sampleBounds.pMax.x - 1 },
        { "y_end",sampleBounds.pMax.y - 1 }
    });

    //json manager_func(int task_index, const json & args) {

    // https://stackoverflow.com/questions/4324763/can-we-have-functions-inside-functions-in-c
    // https://en.cppreference.com/w/cpp/language/lambda

    auto manager_func = [&](int task_index, const json& args) {
        int y_interval = (int(args["y_end"]) - args["y_start"] + 1) / args["task_count"];
        if((int(args["y_end"]) - args["y_start"] + 1) % args["task_count"] != 0)
            y_interval++;

        int y_start = args["y_start"] + y_interval * task_index;
        int y_end = std::min(int(args["y_end"]), int(y_start + y_interval - 1));

        return json({ 
            { "task_index", task_index },
            { "task_progress_total", (y_end - y_start + 1) * (int(args["x_end"]) - args["x_start"] + 1)},
            { "x_start", args["x_start"]},
            { "y_start", y_start },
            { "x_end", args["x_end"] },
            { "y_end", y_end} });
    };

    auto render_task = [&](const json& args) {
        int i = args["x_start"];
        int j = args["y_start"];
        int task_index = args["task_index"];

        std::cout << "--- render_task "<< args["task_index"] << " " << i << " " << j << std::endl;
        std::cout << args << std::endl;

        std::unique_ptr<Sampler> local_sampler = sampler->Clone();

        this->render_progress_total[task_index] = args["task_progress_total"];

        // Loop over pixels in tile to render them
        //while (i < sampleBounds.pMax.x && j < sampleBounds.pMax.y) {
        for (int j = args["y_start"]; j <= args["y_end"]; ++j) {
            for (int i = args["x_start"]; i <= args["x_end"]; ++i) {

                if (render_status == 0)
                    break;

                Log("Render %d %d", i, j);
                //std::cout << "Render " << i << " " << j << std::endl;
                Point2i pixel(i, j);

                if (i == 384 && j == 40) {
                    static int gg = 0;
                    gg++;

                    //break;
                }

                //this->sampler->StartPixel(pixel);
                local_sampler->StartPixel(pixel);

                this->render_progress_now[task_index]++;

                // Do this check after the StartPixel() call; this keeps
                // the usage of RNG values from (most) Samplers that use
                // RNGs consistent, which improves reproducability /
                // debugging.
                //if (!InsideExclusive(pixel, pixelBounds))
                //    continue;

                do {
                    // Initialize _CameraSample_ for current sample
                    CameraSample cameraSample = local_sampler->GetCameraSample(pixel);

                    // Generate camera ray for current sample
                    RayDifferential ray;
                    float rayWeight = camera->GenerateRayDifferential(cameraSample, &ray);
                    ray.ScaleDifferentials(1 / std::sqrt((float)local_sampler->samplesPerPixel));

                    //++nCameraRays;

                    // Evaluate radiance along camera ray
                    Spectrum L(0.f);
                    if (rayWeight > 0)
                        L = Li(ray, scene, *local_sampler);

                    // Issue warning if unexpected radiance value returned
                    /* if (L.HasNaNs()) {
                        LOG(ERROR) << StringPrintf(
                            "Not-a-number radiance value returned "
                            "for pixel (%d, %d), sample %d. Setting to black.",
                            pixel.x, pixel.y,
                            (int)tileSampler->CurrentSampleNumber());
                        L = Spectrum(0.f);
                    }
                    else if (L.y() < -1e-5) {
                        LOG(ERROR) << StringPrintf(
                            "Negative luminance value, %f, returned "
                            "for pixel (%d, %d), sample %d. Setting to black.",
                            L.y(), pixel.x, pixel.y,
                            (int)tileSampler->CurrentSampleNumber());
                        L = Spectrum(0.f);
                    }
                    else if (std::isinf(L.y())) {
                        LOG(ERROR) << StringPrintf(
                            "Infinite luminance value returned "
                            "for pixel (%d, %d), sample %d. Setting to black.",
                            pixel.x, pixel.y,
                            (int)tileSampler->CurrentSampleNumber());
                        L = Spectrum(0.f);
                    }
                    VLOG(1) << "Camera sample: " << cameraSample << " -> ray: " <<
                        ray << " -> L = " << L;
                    */

                    // Add camera ray's contribution to image
                    //filmTile->AddSample(cameraSample.pFilm, L, rayWeight);

                    camera->AddSample(Point2f(pixel.x, pixel.y), L, rayWeight, local_sampler->samplesPerPixel);

                    // Free _MemoryArena_ memory from computing image sample
                    // value
                    //arena.Reset();
                } while (local_sampler->StartNextSample());
            }
        }
    };

    StartMultiTask(render_task, manager_func, tast_args);

    // Save final image after rendering
    camera->film->WriteImage();
    render_status = 0;
    has_new_photo = 1;
}

Spectrum SamplerIntegrator::SpecularReflect(
    const RayDifferential& ray, const SurfaceInteraction& isect,
    const Scene& scene, Sampler& sampler, int depth) const {

    Log("SpecularReflect");

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
        Log("SpecularReflect continue");

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
        return f * Li(rd, scene, sampler, depth + 1) * std::abs(Dot(wi, ns)) / pdf;
    }
    else
        return Spectrum(0.f);
}

Spectrum SamplerIntegrator::SpecularTransmit(
    const RayDifferential& ray, const SurfaceInteraction& isect,
    const Scene& scene, Sampler& sampler, int depth) const {
    Vector3f wo = isect.wo, wi;
    float pdf;
    const Point3f& p = isect.p;
    const BSDF& bsdf = *isect.bsdf;
    Spectrum f = bsdf.Sample_f(wo, &wi, sampler.Get2D(), &pdf,
        BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
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
        L = f * Li(rd, scene, sampler, depth + 1) * AbsDot(wi, ns) / pdf;
    }
    return L;
}


////////////////////

// 对全局的光源进行平均采样。选出一个光源。
Spectrum UniformSampleOneLight(const Interaction& it, const Scene& scene,
    Sampler& sampler,
    bool handleMedia, const Distribution1D* lightDistrib) {

    int nLights = scene.lights.size();

    if (nLights == 0)
        return Spectrum(0.f);

    int lightIndex;
    float lightPdf;

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
    return EstimateDirect(it, uScattering, *light, uLight, scene, sampler, handleMedia) / lightPdf;
}

// 计算一个直接光源作用于交点，产生的光能。
Spectrum EstimateDirect(const Interaction& it, const Point2f& uScattering,
    const Light& light, const Point2f& uLight,
    const Scene& scene, Sampler& sampler,
    bool handleMedia, bool specular) {

    BxDFType bsdfFlags = specular ? BSDF_ALL : BxDFType(BSDF_ALL & ~BSDF_SPECULAR);
    Spectrum Ld(0.f);

    // Sample light source with multiple importance sampling
    Vector3f wi;
    float lightPdf = 0, scatteringPdf = 0;
    //VisibilityTester visibility;

    // 先得到直接光的初始值。有可能被遮挡。后续再判断。
    // 光打到交点，算出wi。
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
        }

        if (!f.IsBlack()) {
            // Compute effect of visibility for light source sample
            if (handleMedia) {
                //Li *= visibility.Tr(scene, sampler);
                //VLOG(2) << "  after Tr, Li: " << Li;
            }
            else {
                // 如果遮挡
                Ray temp_ray = isect.SpawnRayTo(light.pos);

                SurfaceInteraction temp_isect;
                float temp_t;
                auto ires = scene.Intersect(temp_ray, &temp_t, &temp_isect);
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
                    //Ld += f * Li * weight / lightPdf;
                }
            }
        }


    }

    if (!light.IsDeltaLight()) {
        // todo
    }


    return Ld;

}

