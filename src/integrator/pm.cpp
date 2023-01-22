#include "pm.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"
#include "../base/events.h"
#include <random>

PMIntegrator::PMIntegrator(std::shared_ptr<Camera> camera,
    std::shared_ptr<Sampler> sampler
)
    : camera(camera),
    sampler(sampler) {

    default_config = json({
        {"name", "pm"},
        {"ray_sample_no", 1},
        {"ray_bounce_no", 10},
        {"render_threads_no", 12},
        {"emit_photons", 5000},
        {"gather_photons", 10},
        {"gather_photons_r", 0.0f},
        {"gather_method", 1},
        {"filter", 1},
        {"energy_scale", 10000.f},
        {"reemit_photons", true},
        {"render_direct", true},
        {"render_specular", true},
        {"render_caustic", true},
        {"render_diffuse", true},
        {"render_global", false},
        {"specular_method", 1},
        {"specular_rt_samples", 1}
    });

    SetOptions(default_config);
}

void PMIntegrator::SetOptions(const json& new_config) {
    std::cout << "PMIntegrator::SetOptions " << new_config;
    auto current_config = config;
    current_config.merge_patch(new_config);

    ray_sample_no = current_config["ray_sample_no"];
    sampler->SetSamplesPerPixel(current_config["ray_sample_no"]);
    SetRayBounceNo(current_config["ray_bounce_no"]);
    //SetRenderThreadsCount(current_config["render_threads_no"]);

    emitPhotons = current_config["emit_photons"];
    gatherPhotons = current_config["gather_photons"];
    gatherPhotonsR = current_config["gather_photons_r"];
    gatherMethod = current_config["gather_method"];
    filter = current_config["filter"];
    energyScale = current_config["energy_scale"];
    reemitPhotons = current_config["reemit_photons"];

    renderDirect = current_config["render_direct"];
    renderSpecular = current_config["render_specular"];
    renderCaustic = current_config["render_caustic"];
    renderDiffuse = current_config["render_diffuse"];
    renderGlobal = current_config["render_global"];

    specularMethod = current_config["specular_method"];
    specularRTSamples = current_config["specular_rt_samples"];

    config = current_config;
}

//json PMIntegrator::GetConfig() {
//    json config;
//
//    config["ray_sample_no"] = ray_sample_no;
//    config["render_threads_no"] = render_threads_no;
//    config["ray_bounce_no"] = maxDepth;
//
//    config["emit_photons"] = emitPhotons;
//    config["gather_photons"] = gatherPhotons;
//    config["gather_photons_r"] = gatherPhotonsR;
//    config["gather_method"] = gatherMethod;
//
//    config["reemit_photons"] = reemitPhotons;
//
//    config["filter"] = filter;
//    config["energy_scale"] = energyScale;
//    config["render_direct"] = renderDirect;
//    config["render_specular"] = renderSpecular;
//    config["render_caustic"] = renderCaustic;
//    config["render_diffuse"] = renderDiffuse;
//    config["render_global"] = renderGlobal;
//
//    config["specular_method"] = specularMethod;
//    config["specular_rt_samples"] = specularRTSamples;
//
//    return config;
//}

json PMIntegrator::GetRenderStatus() {
    json status;

    if (render_status == 1)
        render_duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - render_start_time).count());

    status["render_duration"] = render_duration;

    return status;
}

void PMIntegrator::EmitPhoton(Scene& scene) {
    std::cout << "PMIntegrator::EmitPhoton() emitPhotons = "<< emitPhotons <<" gatherPhotons = "<< gatherPhotons << std::endl;
    
    std::vector<Photon> globalPhotons;
    std::vector<Photon> causticPhotons;
    std::vector<Photon> diffusePhotons;

    MemoryBlock mb;

    BSDFRESULT all_f_result[32]; // 存某个点上所有bxdf的sample_f结果。
    float reflectivity[32]; // bxdf概率。决定光子的操作。

    // 每个灯发射光子
    for (auto light: scene.lights) {
        std::cout << "light Emit Photon " << std::endl;

        float x, y, z;

        for (int ne = 0; ne < emitPhotons; ++ ne) {
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

            Spectrum photonEnergy = light->Le(ray) / emitPhotons;

            //std::cout << std::format("photonEnergy = {} {} {}\n", photonEnergy.c[0], photonEnergy.c[1], photonEnergy.c[2]);

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

                int bxdfCount = isect.bsdf->All_f(-ray.d, sampler->Get2D(), all_f_result);

                int i = 0;
                for (; i < bxdfCount; ++i) {
                    reflectivity[i] = all_f_result[i].f.Avg();
                    if (i > 0) {
                        reflectivity[i] += reflectivity[i - 1];
                    }

                    //std::cout << std::format("reflectivity {}\n", reflectivity[i]);
                }

                // 按概率选定bxdf
                float random = sampler->Get1D() * bxdfCount;
                i = 0;
                for (; i < bxdfCount; ++i) {
                    if (random <= reflectivity[i])
                        break;
                }

                if (i >= bxdfCount) { // 吸收
                    //std::cout << "absorb\n";
                    break;
                }

                auto type = all_f_result[i].type;
                if (type == BxDFType(BSDF_SPECULAR | BSDF_REFLECTION) ||
                    type == BxDFType(BSDF_GLOSSY | BSDF_REFLECTION) ||
                    type == BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION)) {
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

                //std::cout << "type = " << type << std::endl;

                // 更新光子能量
                //photonEnergy = photonEnergy * all_f_result[i].f * AbsDot(all_f_result[i].wi, isect.shading.n) / all_f_result[i].f.Avg() / all_f_result[i].pdf;
                photonEnergy = photonEnergy * all_f_result[i].f / all_f_result[i].f.Avg();

                /*if (type == 5) {
                    std::cout << std::format("f = {} {} {}, pdf = {}, avg = {}, after = {} {} {}\n",
                        all_f_result[i].f.c[0], all_f_result[i].f.c[1], all_f_result[i].f.c[2], all_f_result[i].pdf, all_f_result[i].f.Avg(),
                        photonEnergy.c[0], photonEnergy.c[1], photonEnergy.c[2]);
                }*/

                ray = isect.SpawnRay(all_f_result[i].wi);

                dir = Normalize(all_f_result[i].wi);

                mb.Reset();
            }
        }
    }

    std::cout << "globalPhotons size = "<< globalPhotons.size() << std::endl;
    std::cout << "causticPhotons size = " << causticPhotons.size() << std::endl;
    std::cout << "diffusePhotons size = " << diffusePhotons.size() << std::endl;

    globalPhotonMap.Build(globalPhotons);
    causticPhotonMap.Build(causticPhotons);
    diffusePhotonMap.Build(diffusePhotons);
}

void PMIntegrator::RenderPhoton(Scene& scene) {
    std::cout << "PMIntegrator::RenderPhoton() emitPhotons = " << emitPhotons << " gatherPhotons = " << gatherPhotons << std::endl;

    render_status = 1;
    has_new_photo = 0;

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

        std::cout << "--- render_task " << task_index << " " << i << " " << j << std::endl;

        MemoryBlock mb;

        this->render_progress_total[task_index] = args.task_progress_total;
        int progress_interval = args.task_progress_total / 40;
        int progress = 0;
        int next_progress_to_report = progress_interval;

        for (int i = args.x_start; i <= args.x_end; ++i) {
            for (int j = args.y_start; j <= args.y_end; ++j) {

                if (render_status == 0)
                    break;

                progress++;

                Point2i pixel(i, j);

                sampler->StartPixel(pixel);

                if (progress > next_progress_to_report) {
                    this->render_progress_now[task_index] = progress;
                    next_progress_to_report += progress_interval;
                }

                do {
                    CameraSample cameraSample = sampler->GetCameraSample(pixel);

                    RayDifferential ray;
                    float rayWeight = camera->GenerateRayDifferential(cameraSample, &ray);
                    //ray.ScaleDifferentials(1 / std::sqrt((float)local_sampler->samplesPerPixel));

                    Spectrum L(0.f);
                    if (rayWeight > 0)
                        L = Li(mb, ray, scene, task_index, 0);

                    camera->AddSample(Point2f(pixel.x, pixel.y), L, rayWeight, sampler->samplesPerPixel);

                    mb.Reset();
                } while (sampler->StartNextSample());
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
    };

    auto start_time = std::chrono::high_resolution_clock::now();

    StartMultiTask2(render_task, manager_func, tast_args);

    float duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time).count());
    std::cout << std::format("----------- task_count = {} duration = {}\n", tast_args.task_count, duration);

    camera->film->WriteImage();
    camera->films.push_back(camera->film);
    render_status = 0;
    has_new_photo = 1;
}


Spectrum PMIntegrator::Li(MemoryBlock &mb, const RayDifferential& ray, Scene& scene, int pool_id, int depth, int samplingFlag) {
    Spectrum L(0.f);

    SurfaceInteraction isect;
    float tHit;
    bool hitScene = scene.Intersect(ray, &tHit, &isect, pool_id);
    
    if (hitScene) {
        isect.ComputeScatteringFunctions(mb, ray);

        // direct
        if (renderDirect) {
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY))) > 0) {
                // 计算直接光。
                // 对所有光源采样，选出一个光源，计算bsdf。得到最后的光能。

                auto distrib = lightDistribution->Lookup(isect.p);
                Spectrum Ld = UniformSampleOneLight(isect, scene, *(sampler->Clone()), false, distrib);
                L += Ld;
            }
        }

        // specular
        if (renderSpecular && depth < maxDepth) {
            if (specularMethod == 0 && depth >= 2) {
                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION))
                    ) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    if (!samplingFlag) {
                        for (int i = 0; i < specularRTSamples; ++i) {
                            // 以wo采一束入射光，算出f。
                            // 同时得到一个随机方向，作为下一层的path。
                            Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL), &flags);

                            if (f.IsBlack() || pdf == 0.f) {
                            }
                            else {
                                Ls += f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, pool_id, depth + 1, 1) / pdf;
                            }
                        }

                        L += (Ls / specularRTSamples);
                    }
                    else {
                        // 以wo采一束入射光，算出f。
                        // 同时得到一个随机方向，作为下一层的path。
                        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL), &flags);

                        if (f.IsBlack() || pdf == 0.f) {
                        }
                        else {
                            Ls += f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, pool_id, depth + 1, 1) / pdf;
                        }

                        L += Ls;
                    }
                    
                }
            }
            else {
                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_SPECULAR | BSDF_REFLECTION), &flags);

                    if (f.IsBlack() || pdf == 0.f) {
                        //throw("1");
                    }
                    else {
                        Ls = f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, pool_id, depth + 1, samplingFlag) / pdf;
                    }

                    L += Ls;
                }


                if (isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_GLOSSY | BSDF_REFLECTION), &flags);

                    if (f.IsBlack() || pdf == 0.f) {
                    }
                    else {
                        Ls = f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, pool_id, depth + 1, samplingFlag) / pdf;
                    }

                    L += Ls;
                }

                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION)) > 0) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION), &flags);

                    if (f.IsBlack() || pdf == 0.f) {
                    }
                    else {
                        Ls = f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, pool_id, depth + 1, samplingFlag) / pdf;
                    }

                    L += Ls;
                }
            }
        }


        KNNResult<Photon> knnResult;
        knnResult.payloads.reserve(1000);

        // caustic
        if (renderCaustic && causticPhotonMap.Size() != 0) {
            Spectrum L_caustic(0.f);

            int near = gatherPhotons;
            if (gatherMethod == 1)
                near = int(float(near) * causticPhotonMap.Size() / 1000);

            knnResult.payloads.clear();
            causticPhotonMap.KNN(isect.p, near, gatherPhotonsR * gatherPhotonsR, pool_id, knnResult);

            if (!knnResult.payloads.empty()) {
                // for cone filter
                float r = std::sqrt(knnResult.maxDistance2);
                float k = 1.1f;

                for (auto photon : knnResult.payloads) {
                    Vector3f wo = -photon.dir, wi;
                    float pdf;
                    BxDFType flags;

                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY)), &flags); // BSDF_ALL

                    if (f.IsBlack() || pdf == 0.f)
                        continue;

                    auto photonEnergy = f * AbsDot(wi, isect.shading.n) * photon.energy * energyScale / pdf;

                    if (filter == 1) {
                        photonEnergy *= (1.f - Distance(photon.pos, isect.p) / (k * r));
                    }

                    L_caustic += photonEnergy;
                }

                if (filter == 0) {
                    L_caustic /= (Pi * knnResult.maxDistance2);
                }
                else if (filter == 1) {
                    L_caustic /= (Pi * knnResult.maxDistance2 * (1.f - 2.f / (k * 3)));
                }

                L += L_caustic;
            }
        }

        // diffuse
        if (renderDiffuse && diffusePhotonMap.Size() != 0) {
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY))) > 0) {

                Spectrum L_caustic(0.f);

                int near = gatherPhotons;
                if (gatherMethod == 1)
                    near = int(float(near) * diffusePhotonMap.Size() / 1000);

                knnResult.payloads.clear();
                diffusePhotonMap.KNN(isect.p, near, gatherPhotonsR * gatherPhotonsR, pool_id, knnResult);

                if (!knnResult.payloads.empty()) {
                    // for cone filter
                    float r = std::sqrt(knnResult.maxDistance2);
                    float k = 1.1f;

                    for (auto photon : knnResult.payloads) {
                        Vector3f wo = -photon.dir, wi;
                        float pdf;
                        BxDFType flags;

                        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                        if (f.IsBlack() || pdf == 0.f)
                            continue;

                        auto photonEnergy = f * AbsDot(wi, isect.shading.n) * photon.energy * energyScale / pdf;

                        if (filter == 1) {
                            photonEnergy *= (1.f - Distance(photon.pos, isect.p) / (k * r));
                        }

                        L_caustic += photonEnergy;
                    }

                    if (filter == 0) {
                        L_caustic /= (Pi * knnResult.maxDistance2);
                    }
                    else if (filter == 1) {
                        L_caustic /= (Pi * knnResult.maxDistance2 * (1.f - 2.f / (k * 3)));
                    }

                    L += L_caustic;
                }
            }
        }

        // global
        if (renderGlobal && globalPhotonMap.Size() != 0) {
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL)) > 0) { //  & ~(BSDF_SPECULAR | BSDF_GLOSSY

                Spectrum L_caustic(0.f);

                int near = gatherPhotons;
                if (gatherMethod == 1)
                    near = int(float(near) * globalPhotonMap.Size() / 1000);

                knnResult.payloads.clear();
                globalPhotonMap.KNN(isect.p, near, gatherPhotonsR * gatherPhotonsR, pool_id, knnResult);

                if (!knnResult.payloads.empty()) {

                    // for cone filter
                    float r = std::sqrt(knnResult.maxDistance2);
                    float k = 1.1f;

                    //std::cout << knnResult.payloads.size() << " "<< knnResult.resultMaxDistance2<< std::endl;

                    for (auto photon : knnResult.payloads) {
                        Vector3f wo = -photon.dir, wi;
                        float pdf;
                        BxDFType flags;

                        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                        if (f.IsBlack() || pdf == 0.f)
                            continue;

                        auto photonEnergy = f * AbsDot(wi, isect.shading.n) * photon.energy * energyScale / pdf;

                        if (filter == 1) {
                            photonEnergy *= (1.f - Distance(photon.pos, isect.p) / (k * r));
                        }

                        L_caustic += photonEnergy;
                    }

                    if (filter == 0) {
                        L_caustic /= (Pi * knnResult.maxDistance2);
                    }
                    else if (filter == 1) {
                        L_caustic /= (Pi * knnResult.maxDistance2 * (1.f - 2.f / (k * 3)));
                    }

                    L += L_caustic;
                }
            }
        }

        //std::cout << i << " " << j << " " << L.c[0] <<" "<< L.c[1] << " " << L.c[2] << std::endl;
    }
    else {
        //throw("wtfffff");
    }

    if(L.IsBlack()) {
        //throw("wtfffff");
    }

    return L;
}

void PMIntegrator::Render(Scene& scene) {
    std::cout << "PMIntegrator::Render() " << std::endl;

    render_start_time = std::chrono::high_resolution_clock::now();

    lightDistribution = CreateLightSampleDistribution("uniform", scene);

    if (reemitPhotons) {
        EmitPhoton(scene);
    }
    
    RenderPhoton(scene);
}