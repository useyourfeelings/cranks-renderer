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
    sampler(sampler),
    filter(1),
    emitPhotons(3000),
    gatherPhotons(30),
    gatherPhotonsR(0.1),
    gatherMethod(0),
    energyScale(10000),
    reemitPhotons(1),
    renderDirect(1)
    {
    }

void PMIntegrator::SetOptions(const json& data) {
    sampler->SetSamplesPerPixel(data["ray_sample_no"]);
    SetRayBounceNo(data["ray_bounce_no"]);
    SetRenderThreadsCount(data["render_threads_no"]);

    emitPhotons = data["emitPhotons"];
    gatherPhotons = data["gatherPhotons"];
    gatherPhotonsR = data["gatherPhotonsR"];
    gatherMethod = data["gatherMethod"];
    filter = data["filter"];
    energyScale = data["energyScale"];
    reemitPhotons = data["reemitPhotons"];

    renderDirect = data["renderDirect"];
    renderSpecular = data["renderSpecular"];
    renderCaustic = data["renderCaustic"];
    renderDiffuse = data["renderDiffuse"];
    renderGlobal = data["renderGlobal"];

    specularMethod = data["specularMethod"];
    specularRTSamples = data["specularRTSamples"];
}

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

    BSDFRESULT all_f_result[32]; // ��ĳ����������bxdf��sample_f�����
    float reflectivity[32]; // bxdf���ʡ��������ӵĲ�����

    // ÿ���Ʒ������
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

                // ����material���bxdf
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

                // ������ѡ��bxdf
                float random = sampler->Get1D() * bxdfCount;
                i = 0;
                for (; i < bxdfCount; ++i) {
                    if (random <= reflectivity[i])
                        break;
                }

                if (i >= bxdfCount) { // ����
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
                    // ����diffuse���γ�һ����·��������Ƿ�Ϊcaustic��
                    if (caustic) {
                        causticPhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    }
                    else {
                        diffusePhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    }

                    globalPhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    //std::cout << "globalPhotons push " << photonEnergy.c[0] << " " << photonEnergy.c[1] << " " << photonEnergy.c[2] << std::endl;

                    caustic = false; // ���¿�ʼ
                }

                //std::cout << "type = " << type << std::endl;

                // ���¹�������
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
    // sampleBounds ����600 400

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
                // ����ֱ�ӹ⡣
                // �����й�Դ������ѡ��һ����Դ������bsdf���õ����Ĺ��ܡ�

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
                            // ��wo��һ������⣬���f��
                            // ͬʱ�õ�һ�����������Ϊ��һ���path��
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
                        // ��wo��һ������⣬���f��
                        // ͬʱ�õ�һ�����������Ϊ��һ���path��
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
        if (renderCaustic) {
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
        if (renderDiffuse) {
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
        if (renderGlobal) {
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