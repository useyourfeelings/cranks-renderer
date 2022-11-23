#include "ppm.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"
#include "../base/events.h"
#include <random>

PPMIntegrator::PPMIntegrator(std::shared_ptr<Camera> camera,
    std::shared_ptr<Sampler> sampler
)
    : camera(camera),
    sampler(sampler),
    filter(1),
    emitPhotons(5000),
    energyScale(10000),
    renderDirect(1),
    currentIteration(0),
    maxIterations(10),
    alpha(0.5),
    initalRadius(20)
    {
    }

void PPMIntegrator::SetOptions(const json& data) {
    sampler->SetSamplesPerPixel(data["ray_sample_no"]);
    SetRayBounceNo(data["ray_bounce_no"]);
    SetRenderThreadsCount(data["render_threads_no"]);

    emitPhotons = data["emitPhotons"];
    filter = data["filter"];
    energyScale = data["energyScale"];

    renderDirect = data["renderDirect"];
    renderCaustic = data["renderCaustic"];
    renderDiffuse = data["renderDiffuse"];
    renderGlobal = data["renderGlobal"];

    // ppm
    maxIterations = data["maxIterations"];
    alpha = data["alpha"];
    initalRadius = data["initalRadius"];
}

json PPMIntegrator::GetRenderStatus() {
    json status;
    status["currentIteration"] = currentIteration;

    if (render_status == 1)
        render_duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - render_start_time).count());

    status["render_duration"] = render_duration;
    
    return status;
}

void PPMIntegrator::TraceRay(Scene& scene, std::vector<MemoryBlock> &mbs) {
    std::cout << "PMIntegrator::TraceRay()\n" << std::endl;

    //Log("Render");
    render_status = 1;
    has_new_photo = 0;
    totalPhotons = 0;

    hitpoints.clear();

    BBox2i sampleBounds = BBox2i(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));
    // sampleBounds 类似600 400

    this->render_progress_now.resize(render_threads_no);
    this->render_progress_total.resize(render_threads_no);
    for (int i = 0; i < render_threads_no; ++i) {
        render_progress_now[i] = 0;
    }

    MultiTaskCtx tast_args(render_threads_no);
    tast_args.x_start = sampleBounds.pMin.x;
    tast_args.y_start = sampleBounds.pMin.y;
    tast_args.x_end = sampleBounds.pMax.x - 1;
    tast_args.y_end = sampleBounds.pMax.y - 1;

    // https://stackoverflow.com/questions/4324763/can-we-have-functions-inside-functions-in-c
    // https://en.cppreference.com/w/cpp/language/lambda

    // 改为竖着分像素。一般会更均匀。
    /*auto manager_func = [&](int task_index, const json& args) {
        int x_interval = (int(args["x_end"]) - args["x_start"] + 1) / args["task_count"];
        if ((int(args["x_end"]) - args["x_start"] + 1) % args["task_count"] != 0)
            x_interval++;

        int x_start = args["x_start"] + x_interval * task_index;
        int x_end = std::min(int(args["x_end"]), int(x_start + x_interval - 1));

        return json({
            { "task_index", task_index },
            { "task_progress_total", (x_end - x_start + 1) * (int(args["y_end"]) - args["y_start"] + 1)},
            { "y_start", args["y_start"]},
            { "x_start", x_start },
            { "y_end", args["y_end"] },
            { "x_end", x_end} });
    };*/

    auto manager_func = [](int task_index, MultiTaskCtx args) {
        int x_interval = (args.x_end - args.x_start + 1) / args.task_count;
        if ((args.x_end - args.x_start + 1) % args.task_count != 0)
            x_interval++;

        int x_start = args.x_start + x_interval * task_index;
        int x_end = std::min(args.x_end, (x_start + x_interval - 1));

        args.task_index = task_index;
        args.task_progress_total = (x_end - x_start + 1) * (args.y_end - args.y_start + 1);
        args.y_start = args.y_start;
        args.x_start = x_start;
        args.y_end = args.y_end;
        args.x_end = x_end;

        return args;
    };

    auto render_task = [&](MultiTaskCtx args) {
        int i = args.x_start;
        int j = args.y_start;
        int task_index = args.task_index;

        std::cout << "--- render_task " << args.task_index << " " << i << " " << j << std::endl;

        this->render_progress_total[task_index] = args.task_progress_total;
        int progress_interval = args.task_progress_total / 40;
        int progress = 0;
        int next_progress_to_report = progress_interval;

        for (int i = args.x_start; i <= args.x_end; ++i) {
            for (int j = args.y_start; j <= args.y_end; ++j) {

                if (render_status == 0)
                    break;

                progress++;

                //std::cout << std::format("render_task {} {} {}, {} {}\n", task_index, i, j, args.x_end, args.y_end);
                Point2i pixel(i, j);

                sampler->StartPixel(pixel);

                if (progress > next_progress_to_report) {
                    this->render_progress_now[task_index] = progress;
                    next_progress_to_report += progress_interval;
                }

                //do {
                    CameraSample cameraSample = sampler->GetCameraSample(pixel);

                    RayDifferential ray;
                    float rayWeight = camera->GenerateRayDifferential(cameraSample, &ray);
                    //ray.ScaleDifferentials(1 / std::sqrt((float)local_sampler->samplesPerPixel));

                    if (rayWeight > 0)
                        TraceARay(mbs[task_index], ray, scene, Point2i(i, j), Point2i(camera->resolutionX, camera->resolutionY), 0, Spectrum(1.f), task_index);

                //} while (sampler->StartNextSample());
            }

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
                        auto help_x_start = (i + args.x_end) / 2 + 1;
                        auto help_x_end = args.x_end;
                        int result = MultiTaskCallHelp(args, help_x_start, help_x_end);
                        if (result == 1) {
                            args.x_end = help_x_start - 1;

                            args.task_progress_total = (args.x_end - args.x_start + 1) * (args.y_end - args.y_start + 1);
                            this->render_progress_total[task_index] = args.task_progress_total;
                            progress_interval = args.task_progress_total / 40;
                        }
                    }
                }
            }
        }

        this->render_progress_now[task_index] = args.task_progress_total;

        std::cout << std::format("render_task {} over over over over over over over over over over\n", task_index);
    };

    StartMultiTask2(render_task, manager_func, tast_args);

    // Save final image after rendering
    //camera->film->WriteImage();
    //render_status = 0;
    //has_new_photo = 1;
}

void PPMIntegrator::TraceARay(MemoryBlock& mb, const RayDifferential& ray, Scene& scene, Point2i pixel, Point2i resolution, int depth, Spectrum energyWeight, int pool_id) {
    //std::cout << "PPMIntegrator::TraceARay() energyWeight " << energyWeight.c[0] << " " << energyWeight.c[1] << " " << energyWeight.c[2]
    //    << std::endl << depth
    //    << std::endl;
    
    auto isect = std::make_shared<SurfaceInteraction>();
    float tHit;
    bool hitScene = scene.Intersect(ray, &tHit, isect.get(), pool_id);

    if (hitScene) {
        isect->ComputeScatteringFunctions(mb, ray);

        // diffuse
        if (isect->bsdf->NumComponents(BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY))) > 0) {

            std::lock_guard<std::mutex> lock(hitpointsMutex);
            Hitpoint hitpoint;

            //std::cout << "PPMIntegrator::TraceARay() wtf1" << std::endl;

            hitpoint.pos = isect->p;
            hitpoint.dir = ray.d;
            hitpoint.normal = isect->shading.n;
            hitpoint.energyWeight = energyWeight; // weight to pixel
            hitpoint.energy[0] = hitpoint.energy[1] = hitpoint.energy[2] = Spectrum(0);
            hitpoint.photonCount[0] = hitpoint.photonCount[1] = hitpoint.photonCount[2] = 0;
            hitpoint.r[0] = hitpoint.r[1] = hitpoint.r[2] = initalRadius;
            hitpoint.x = pixel.x;
            hitpoint.y = pixel.y;
            hitpoint.isect = isect;

            hitpoints.push_back(hitpoint);
        }

        // specular
        if (depth < 100) {
            if (isect->bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0 ||
                isect->bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0 ||
                isect->bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION)) > 0
                ) {

                Spectrum Ls(0.f);
                Vector3f wo = -ray.d, wi;
                float pdf;
                BxDFType flags;

                if (depth > 3) { // 当 depth > 3 做统一采样
                    Spectrum f = isect->bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL), &flags);

                    if (f.IsBlack() || pdf == 0.f) {
                    }
                    else {
                        Spectrum weight = f * AbsDot(wi, isect->shading.n) / pdf;
                        
                        // rr
                        float q = 1 - weight.MaxComponentValue();
                        if (q > 0.8) {
                            if (sampler->Get1D() < q) {
                                //std::cout << "PPMIntegrator::TraceARay() rr return" << std::endl;
                                return;
                            }

                            TraceARay(mb, isect->SpawnRay(wi), scene, pixel, resolution, depth + 1, weight / (1 - q), pool_id);
                        }
                    }
                }
                else { // 否则区分ray
                    if (isect->bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0) {
                        Spectrum f = isect->bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_SPECULAR | BSDF_REFLECTION), &flags);

                        if (f.IsBlack() || pdf == 0.f) {
                        }
                        else {
                            TraceARay(mb, isect->SpawnRay(wi), scene, pixel, resolution, depth + 1, f * AbsDot(wi, isect->shading.n) / pdf, pool_id);
                            //Ls = f * AbsDot(wi, isect.shading.n) * Li(isect.SpawnRay(wi), scene, depth + 1) / pdf;
                        }
                    }

                    if (isect->bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0) {
                        Spectrum f = isect->bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_GLOSSY | BSDF_REFLECTION), &flags);

                        if (f.IsBlack() || pdf == 0.f) {
                        }
                        else {
                            TraceARay(mb, isect->SpawnRay(wi), scene, pixel, resolution, depth + 1, f * AbsDot(wi, isect->shading.n) / pdf, pool_id);
                            //Ls = f * AbsDot(wi, isect.shading.n) * Li(isect.SpawnRay(wi), scene, depth + 1) / pdf;
                        }
                    }

                    if (isect->bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION)) > 0) {
                        Spectrum f = isect->bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION), &flags);

                        if (f.IsBlack() || pdf == 0.f) {
                        }
                        else {
                            TraceARay(mb, isect->SpawnRay(wi), scene, pixel, resolution, depth + 1, f * AbsDot(wi, isect->shading.n) / pdf, pool_id);
                            //Ls = f * AbsDot(wi, isect.shading.n) * Li(isect.SpawnRay(wi), scene, depth + 1) / pdf;
                        }
                    }
                }
            }
        }
    }
}

void PPMIntegrator::EmitPhoton(Scene& scene) {
    std::cout << "PPMIntegrator::EmitPhoton() emitPhotons = " << emitPhotons << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<Photon> globalPhotons;
    std::vector<Photon> causticPhotons;
    std::vector<Photon> diffusePhotons;

    MemoryBlock mb;

    // 每个灯发射光子
    for (auto light : scene.lights) {
        std::cout << "light Emit Photon " << std::endl;

        float x, y, z;

        for (int ne = 0; ne < emitPhotons; ++ne) {
            for (;;) {
                //x = (dis(e) - 0.5) * 2;
                x = (sampler->Get1D() - 0.5) * 2;
                y = (sampler->Get1D() - 0.5) * 2;
                z = (sampler->Get1D() - 0.5) * 2;

                if (x * x + y * y + z * z <= 1)
                    break;
            }

            Vector3f dir = Normalize(Vector3f(x, y, z));

            //std::cout << "photon # " << ne << std::endl;

            //std::cout << "dir = " << dir.x <<" "<< dir.y<<" "<< dir.z << std::endl;

            bool caustic = false;
            RayDifferential ray(light->pos, dir);

            //std::cout << "light pos = " << light->pos.x << " " << light->pos.y << " " << light->pos.z << std::endl;

            Spectrum photonEnergy = light->Le(ray);// / emitPhotons;

            // emit to scene
            for (int pathLen = 0; pathLen < 100; ++pathLen) {
                //std::cout << "pathLen " << pathLen << std::endl;

                SurfaceInteraction isect;
                float tHit;
                bool hitScene = scene.Intersect(ray, &tHit, &isect);

                if (!hitScene)
                    break;

                // 根据material添加bxdf
                isect.ComputeScatteringFunctions(mb, ray);

                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION)) > 0) { // BSDF_GLOSSY | BSDF_REFLECTION
                //if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR)) > 0 ||
                //    isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY)) > 0) {

                    // 一旦碰到specular，就开始一个caustic。
                    // 一整条路径中可包含多个caustic。目前这样实现。
                    caustic = true;
                }
                else {
                    // 碰到diffuse，形成一个子路径。检查是否为caustic。
                    if (caustic) {
                        causticPhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    }
                    else {
                        diffusePhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    }

                    globalPhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    //std::cout << "globalPhotons push " << photonEnergy.c[0] << " " << photonEnergy.c[1] << " " << photonEnergy.c[2] << std::endl;

                    caustic = false; // 重新开始
                }

                Vector3f wo = -ray.d, wi;
                float pdf;
                BxDFType flags;

                // 以wo采一束入射光，算出f。
                // 同时得到一个随机方向，作为下一层的path。
                Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                if (f.IsBlack() || pdf == 0.f)
                    break;

                // 更新光子能量
                photonEnergy *= (f * AbsDot(wo, isect.shading.n) / pdf);

                // rr
                float q = (1 - photonEnergy.MaxComponentValue() * emitPhotons);
                if (q > 0.8) {
                    if (sampler->Get1D() < q) {
                        //std::cout << "rr break q = " << q << " pathLen = " << pathLen << std::endl;
                        break;
                    }

                    photonEnergy /= 1 - q;
                }

                ray = isect.SpawnRay(wi);

                dir = Normalize(wi);

                mb.Reset();
            }
        }

        totalPhotons += emitPhotons;
    }

    std::cout << "globalPhotons size = " << globalPhotons.size() << std::endl;
    std::cout << "causticPhotons size = " << causticPhotons.size() << std::endl;
    std::cout << "diffusePhotons size = " << diffusePhotons.size() << std::endl;

    globalPhotonMap.Build(globalPhotons);
    causticPhotonMap.Build(causticPhotons);
    diffusePhotonMap.Build(diffusePhotons);

    float duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time).count());
    std::cout << std::format("----------- EmitPhoton duration = {}\n", duration);
}

void PPMIntegrator::RenderPhoton(Scene& scene) {
    std::cout << "PPMIntegrator::RenderPhoton() emitPhotons = " << emitPhotons << std::endl;

    this->render_progress_now.resize(render_threads_no);
    this->render_progress_total.resize(render_threads_no);
    for (int i = 0; i < render_threads_no; ++i) {
        render_progress_now[i] = 0;
    }

    MultiTaskCtx tast_args(render_threads_no);
    tast_args.x_start = 0;
    tast_args.y_start = 0;
    tast_args.x_end = int(hitpoints.size()) - 1;
    tast_args.y_end = 0;

    // https://stackoverflow.com/questions/4324763/can-we-have-functions-inside-functions-in-c
    // https://en.cppreference.com/w/cpp/language/lambda

    auto manager_func = [](int task_index, MultiTaskCtx args) {
        int interval = (args.x_end - args.x_start + 1) / args.task_count;
        if ((args.x_end - args.x_start + 1) % args.task_count != 0)
            interval++;

        int start = args.x_start + interval * task_index;
        int end = std::min(args.x_end, (start + interval - 1));

        args.task_index = task_index;
        args.task_progress_total = (end - start + 1);
        args.x_start = start;
        args.x_end = end;

        return args;
    };

    auto render_task = [&](MultiTaskCtx args) {
        int i = args.x_start;
        int task_index = args.task_index;

        std::cout << "--- render_task " << args.task_index << " " << i << std::endl;

        this->render_progress_total[task_index] = args.task_progress_total;

        int progress_interval = args.task_progress_total / 40;
        int progress = 0;
        int next_progress_to_report = progress_interval;

        for (; i <= args.x_end; ++i) {

            if (render_status == 0)
                break;

            UpdateHitpoint(scene, i, task_index);

            progress++;
            if (progress > next_progress_to_report) {
                this->render_progress_now[task_index] = progress;
                next_progress_to_report += progress_interval;
            }

            // call help
            if (args.task_count > 1) {
                if ((args.x_end - i > 4) && (float(i - args.x_start) / (args.x_end - args.x_start)) < 0.95) {
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

                            args.task_progress_total = (args.x_end - args.x_start + 1) * (args.y_end - args.y_start + 1);
                            this->render_progress_total[task_index] = args.task_progress_total;
                            progress_interval = args.task_progress_total / 40;
                        }
                    }
                }
            }
        }

        this->render_progress_now[task_index] = args.task_progress_total;
    };

    auto start_time = std::chrono::high_resolution_clock::now();

    StartMultiTask2(render_task, manager_func, tast_args);

    float duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time).count());
    std::cout << std::format("----------- RenderPhoton task_count = {} duration = {}\n", int(tast_args.task_count), duration);

    // Save final image after rendering
    camera->film->WriteImage();
    camera->films.push_back(camera->film);
    //render_status = 0;
    has_new_photo = 1;
}

void PPMIntegrator::UpdateHitpoint(Scene& scene, int i, int pool_id) {
    Spectrum L(0.f);

    Hitpoint & hitpoint = hitpoints[i];

    // direct
    if (renderDirect) {
        
        auto distrib = lightDistribution->Lookup(hitpoint.isect->p);
        Spectrum Ld = UniformSampleOneLight(*hitpoint.isect, scene, *(sampler->Clone()), false, distrib);
        L += Ld;

        L *= hitpoint.energyWeight;

        camera->AddSample(Point2f(hitpoint.x, hitpoint.y), L, 1, 1);
    }

    KNNResult<Photon> knnResult;
    knnResult.payloads.reserve(1000);

    // caustic
    if (renderCaustic) {
        Spectrum L_caustic(0.f);

        int near = emitPhotons; // take all

        causticPhotonMap.KNN(hitpoint.isect->p, near, hitpoint.r[0] * hitpoint.r[0], pool_id, knnResult); //, info);

        // for cone filter
        float r = hitpoint.r[0]; // std::sqrt(knnResult.resultMaxDistance2);
        float k = 1.1f;

        for (auto photon : knnResult.payloads) {
            Vector3f wo = -photon.dir, wi;
            float pdf;
            BxDFType flags;

            Spectrum f = hitpoint.isect->bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY)), &flags); // BSDF_ALL

            if (f.IsBlack() || pdf == 0.f)
                continue;

            auto photonEnergy = f * AbsDot(wi, hitpoint.isect->shading.n) * photon.energy * energyScale / pdf;

            if (filter == 1) {
                photonEnergy *= (1.f - Distance(photon.pos, hitpoint.isect->p) / (k * r));
            }

            L_caustic += photonEnergy;
        }

        if (filter == 0) {
            //L_caustic /= (Pi * knnResult.resultMaxDistance2);
            L_caustic /= (Pi * r * r);
        }
        else if (filter == 1) {
            //L_caustic /= (Pi * knnResult.resultMaxDistance2 * (1.f - 2.f / (k * 3)));
            L_caustic /= (Pi * r * r * (1.f - 2.f / (k * 3)));
        }

        // ppm

        size_t newPhotonNum = knnResult.payloads.size();

        Spectrum newEnergy = (hitpoint.energy[0] + L_caustic) *
            (hitpoint.photonCount[0] + newPhotonNum * alpha) /
            (hitpoint.photonCount[0] + newPhotonNum);

        camera->AddSample(Point2f(hitpoint.x, hitpoint.y), newEnergy * hitpoint.energyWeight / totalPhotons, 1, 1);

        hitpoint.energy[0] = newEnergy;
        hitpoint.r[0] *= std::sqrt(
            (hitpoint.photonCount[0] + newPhotonNum * alpha) /
            (hitpoint.photonCount[0] + newPhotonNum));
        hitpoint.photonCount[0] += newPhotonNum * alpha;

    }

    // diffuse
    if (renderDiffuse) {
        if (hitpoint.isect->bsdf->NumComponents(BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY))) > 0) {

            Spectrum L_caustic(0.f);

            int near = emitPhotons; // take all

            knnResult.payloads.clear();
            diffusePhotonMap.KNN(hitpoint.pos, near, hitpoint.r[1] * hitpoint.r[1], pool_id, knnResult);

            // for cone filter
            float r = hitpoint.r[1]; // std::sqrt(knnResult.resultMaxDistance2);
            float k = 1.1f;

            for (auto photon : knnResult.payloads) {
                Vector3f wo = -photon.dir, wi;
                float pdf;
                BxDFType flags;

                Spectrum f = hitpoint.isect->bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                if (f.IsBlack() || pdf == 0.f)
                    continue;

                auto photonEnergy = f * AbsDot(wi, hitpoint.isect->shading.n) * photon.energy * energyScale / pdf;

                if (filter == 1) {
                    photonEnergy *= (1.f - Distance(photon.pos, hitpoint.isect->p) / (k * r));
                }

                L_caustic += photonEnergy;
            }

            if (filter == 0) {
                //L_caustic /= (Pi * knnResult.resultMaxDistance2);
                L_caustic /= (Pi * r * r);
            }
            else if (filter == 1) {
                //L_caustic /= (Pi * knnResult.resultMaxDistance2 * (1.f - 2.f / (k * 3)));
                L_caustic /= (Pi * r * r * (1.f - 2.f / (k * 3)));
            }

            // ppm

            size_t newPhotonNum = knnResult.payloads.size();

            Spectrum newEnergy = (hitpoint.energy[1] + L_caustic) *
                (hitpoint.photonCount[1] + newPhotonNum * alpha) /
                (hitpoint.photonCount[1] + newPhotonNum);

            camera->AddSample(Point2f(hitpoint.x, hitpoint.y), newEnergy * hitpoint.energyWeight / totalPhotons, 1, 1);

            hitpoint.energy[1] = newEnergy;
            hitpoint.r[1] *= std::sqrt(
                (hitpoint.photonCount[1] + newPhotonNum * alpha) /
                (hitpoint.photonCount[1] + newPhotonNum));
            hitpoint.photonCount[1] += newPhotonNum * alpha;

        }
    }

    // global
    if (renderGlobal) {
        if (hitpoint.isect->bsdf->NumComponents(BxDFType(BSDF_ALL)) > 0) { //  & ~(BSDF_SPECULAR | BSDF_GLOSSY

            Spectrum L_caustic(0.f);

            int near = emitPhotons; // take all

            knnResult.payloads.clear();
            globalPhotonMap.KNN(hitpoint.isect->p, near, hitpoint.r[2] * hitpoint.r[2], pool_id, knnResult);

            // for cone filter
            float r = hitpoint.r[2]; // std::sqrt(knnResult.resultMaxDistance2);
            float k = 1.1f;

            //std::cout << knnResult.payloads.size() << " "<< knnResult.resultMaxDistance2<< std::endl;

            for (auto photon : knnResult.payloads) {
                Vector3f wo = -photon.dir, wi;
                float pdf;
                BxDFType flags;

                Spectrum f = hitpoint.isect->bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                if (f.IsBlack() || pdf == 0.f)
                    continue;

                auto photonEnergy = f * AbsDot(wi, hitpoint.isect->shading.n) * photon.energy * energyScale / pdf;

                if (filter == 1) {
                    photonEnergy *= (1.f - Distance(photon.pos, hitpoint.isect->p) / (k * r));
                }

                L_caustic += photonEnergy;
            }

            if (filter == 0) {
                //L_caustic /= (Pi * knnResult.resultMaxDistance2);
                L_caustic /= (Pi * r * r);
            }
            else if (filter == 1) {
                //L_caustic /= (Pi * knnResult.resultMaxDistance2 * (1.f - 2.f / (k * 3)));
                L_caustic /= (Pi * r * r * (1.f - 2.f / (k * 3)));
            }

            // ppm

            size_t newPhotonNum = knnResult.payloads.size();

            Spectrum newEnergy = (hitpoint.energy[2] + L_caustic) *
                (hitpoint.photonCount[2] + newPhotonNum * alpha) /
                (hitpoint.photonCount[2] + newPhotonNum);

            camera->AddSample(Point2f(hitpoint.x, hitpoint.y), newEnergy* hitpoint.energyWeight / totalPhotons, 1, 1);

            hitpoint.energy[2] = newEnergy;
            hitpoint.r[2] *= std::sqrt(
                (hitpoint.photonCount[2] + newPhotonNum * alpha) /
                (hitpoint.photonCount[2] + newPhotonNum));
            hitpoint.photonCount[2] += newPhotonNum * alpha;
        }
    }

    // update to film

    //L *= hitpoints[i].energyWeight;

    //camera->AddSample(Point2f(hitpoints[i].x, hitpoints[i].y), L, 1, 1);
}

void PPMIntegrator::Render(Scene& scene) {
    std::cout << "PPMIntegrator::Render() " << std::endl;

    render_start_time = std::chrono::high_resolution_clock::now();

    lightDistribution = CreateLightSampleDistribution("uniform", scene);

    std::vector<MemoryBlock> mbs(16); // hispoints在TraceRay中计算。永久保持。

    TraceRay(scene, mbs);

    for (currentIteration = 1; currentIteration <= maxIterations; ++currentIteration) {
        std::cout << "PPMIntegrator::Render() currentIteration = " << currentIteration << " totalPhotons = " << totalPhotons<<std::endl;
        camera->SetFilm();
        EmitPhoton(scene);
        RenderPhoton(scene);
        if (render_status == 0)
            break;
    }
    
    render_status = 0;
}


