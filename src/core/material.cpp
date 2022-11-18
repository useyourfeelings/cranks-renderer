#include "material.h"
#include "texture.h"
#include "../texture/checkerboard.h"
#include "reflection.h"
#include "pbr.h"

std::shared_ptr<Material> GenMaterial(const json& material_config, int keep_id, int inc_last_id) {
    //std::cout << "PbrApp::GenMaterial()" << material_config.dump() << std::endl;

    std::shared_ptr<Material> material = nullptr;

    auto material_type = material_config["type"];

    if (material_type == "matte") {
        auto kd = material_config["kd"];
        std::shared_ptr<Texture<Spectrum>> kd_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kd[0], kd[1], kd[2]));

        std::shared_ptr<Texture<float>> sigma_tex = std::make_shared<ConstantTexture<float>>(material_config["sigma"]);

        std::shared_ptr<Texture<Spectrum>> checker_tex = nullptr; // CreateCheckerboardSpectrumTexture(Transform());

        material = std::make_shared<MatteMaterial>(material_config, keep_id, inc_last_id, kd_tex, sigma_tex, nullptr, checker_tex);
    }
    else if (material_type == "glass") {
        auto kr = material_config["kr"];
        auto kt = material_config["kt"];
        auto remaproughness = material_config["remaproughness"];
        std::shared_ptr<Texture<Spectrum>> kr_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kr[0], kr[1], kr[2]));
        std::shared_ptr<Texture<Spectrum>> kt_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kt[0], kt[1], kt[2]));

        std::shared_ptr<Texture<float>> eta = std::make_shared<ConstantTexture<float>>(material_config["eta"]);
        std::shared_ptr<Texture<float>> uroughness = std::make_shared<ConstantTexture<float>>(material_config["uroughness"]);
        std::shared_ptr<Texture<float>> vroughness = std::make_shared<ConstantTexture<float>>(material_config["vroughness"]);
        std::shared_ptr<Texture<float>> bumpmap = std::make_shared<ConstantTexture<float>>(material_config["bumpmap"]);

        material = std::make_shared<GlassMaterial>(material_config, keep_id, inc_last_id, kr_tex, kt_tex, uroughness, vroughness, eta, bumpmap, remaproughness);
    }
    else if (material_type == "mirror") {
        auto kr = material_config["kr"];
        std::shared_ptr<Texture<Spectrum>> kr_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kr[0], kr[1], kr[2]));

        std::shared_ptr<Texture<float>> bumpmap = std::make_shared<ConstantTexture<float>>(material_config["bumpmap"]);

        material = std::make_shared<MirrorMaterial>(material_config, keep_id, inc_last_id, kr_tex, bumpmap);
    }
    else if (material_type == "plastic") {
        auto kd = material_config["kd"];
        auto ks = material_config["ks"];
        auto remaproughness = material_config["remaproughness"];
        std::shared_ptr<Texture<Spectrum>> kd_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kd[0], kd[1], kd[2]));
        std::shared_ptr<Texture<Spectrum>> ks_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(ks[0], ks[1], ks[2]));

        std::shared_ptr<Texture<float>> roughness = std::make_shared<ConstantTexture<float>>(material_config["roughness"]);
        std::shared_ptr<Texture<float>> bumpmap = std::make_shared<ConstantTexture<float>>(material_config["bumpmap"]);

        material = std::make_shared<PlasticMaterial>(material_config, keep_id, inc_last_id, kd_tex, ks_tex, roughness, bumpmap, remaproughness);
    }
    else if (material_type == "metal") {
        auto k = material_config["k"];
        auto eta = material_config["eta"];
        auto remaproughness = material_config["remaproughness"];
        std::shared_ptr<Texture<Spectrum>> k_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(k[0], k[1], k[2]));
        std::shared_ptr<Texture<Spectrum>> eta_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(eta[0], eta[1], eta[2]));

        std::shared_ptr<Texture<float>> roughness = std::make_shared<ConstantTexture<float>>(material_config["roughness"]);
        std::shared_ptr<Texture<float>> bumpmap = std::make_shared<ConstantTexture<float>>(material_config["bumpmap"]);

        material = std::make_shared<MetalMaterial>(material_config, keep_id, inc_last_id, eta_tex, k_tex, roughness, bumpmap, remaproughness);
    }


    return material;
}

void MatteMaterial::ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    //if (bumpMap) Bump(bumpMap, si);

    //// Evaluate textures for _MatteMaterial_ material and allocate BRDF
    //si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);
    //Spectrum r = Kd->Evaluate(*si).Clamp();
    //Float sig = Clamp(sigma->Evaluate(*si), 0, 90);
    //if (!r.IsBlack()) {
    //    if (sig == 0)
    //        si->bsdf->Add(ARENA_ALLOC(arena, LambertianReflection)(r));
    //    else
    //        si->bsdf->Add(ARENA_ALLOC(arena, OrenNayar)(r, sig));
    //}

    Spectrum r = Kd->Evaluate(*si);

    if (checker != nullptr)
        r = r * checker->Evaluate(*si);

    r = r.Clamp();

    //auto checker_color = checker->Evaluate(*si);
    //std::cout << "checker_color " << checker_color.c[0] << " " << checker_color.c[1] << " " << checker_color.c[2] << std::endl;

    float sig = Clamp(sigma->Evaluate(*si), 0, 90);

    //si->bsdf = std::make_shared<BSDF>(*si);
    si->bsdf = MB_ALLOC(mb, BSDF)(*si);

    //if (sig)
    //    ; // OrenNayar

    //si->bsdf->Add(std::make_shared<LambertianReflection>(r));
    si->bsdf->Add(MB_ALLOC(mb, LambertianReflection)(r));

}

//////////////////////////////////

void GlassMaterial::ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    float eta = index->Evaluate(*si);
    float urough = uRoughness->Evaluate(*si);
    float vrough = vRoughness->Evaluate(*si);
    Spectrum R = Kr->Evaluate(*si).Clamp();
    Spectrum T = Kt->Evaluate(*si).Clamp();
    // Initialize _bsdf_ for smooth or rough dielectric
    //si->bsdf = std::make_shared<BSDF>(*si);
    si->bsdf = MB_ALLOC(mb, BSDF)(*si);

    if (R.IsBlack() && T.IsBlack()) return;

    bool isSpecular = urough == 0 && vrough == 0;

    // allowMultipleLobes 是否把多个bxdf捏成一个
    // pbrt page 577
    if (isSpecular && allowMultipleLobes) { 
        //si->bsdf->Add(ARENA_ALLOC(arena, FresnelSpecular)(R, T, 1.f, eta, mode));
    }
    else {
        if (remapRoughness) {
            //urough = TrowbridgeReitzDistribution::RoughnessToAlpha(urough);
            //vrough = TrowbridgeReitzDistribution::RoughnessToAlpha(vrough);
        }
        //MicrofacetDistribution* distrib = isSpecular ? nullptr : ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(urough, vrough);

        if (!R.IsBlack()) {
            //Fresnel* fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, eta);

            auto fresnel = std::make_shared<FresnelDielectric>(1.f, eta);

            if (isSpecular) {
                //si->bsdf->Add(ARENA_ALLOC(arena, SpecularReflection)(R, fresnel));
                //si->bsdf->Add(std::make_shared<SpecularReflection>(R, fresnel));
                si->bsdf->Add(MB_ALLOC(mb, SpecularReflection)(R, fresnel));
            }
            //else
            //    si->bsdf->Add(ARENA_ALLOC(arena, MicrofacetReflection)(R, distrib, fresnel));
        }

        if (!T.IsBlack()) {
            if (isSpecular) {
                //si->bsdf->Add(std::make_shared<SpecularTransmission>(T, 1.f, eta, mode));
                si->bsdf->Add(MB_ALLOC(mb, SpecularTransmission)(T, 1.f, eta, mode));
            }
                
            
            //else
            //    si->bsdf->Add(ARENA_ALLOC(arena, MicrofacetTransmission)(T, distrib, 1.f, eta, mode));
        }
    }
}

//////////////////////////////

void MirrorMaterial::ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    //si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);

    //si->bsdf = std::make_shared<BSDF>(*si);
    si->bsdf = MB_ALLOC(mb, BSDF)(*si);

    Spectrum R = Kr->Evaluate(*si).Clamp();
    if (!R.IsBlack())
        //si->bsdf->Add(ARENA_ALLOC(arena, SpecularReflection)(R, ARENA_ALLOC(arena, FresnelNoOp)()));
        //si->bsdf->Add(std::make_shared<SpecularReflection>(R, std::make_shared<FresnelNoOp>()));
        si->bsdf->Add(MB_ALLOC(mb, SpecularReflection)(R, std::make_shared<FresnelNoOp>()));
}

//////////////////////////////

void Material::Bump(const std::shared_ptr<Texture<float>>& d,
    SurfaceInteraction* si) {
}

const json& Material::GetJson() const {
    return config;
}

int Material::GetID() const {
    return config["id"];
}

void Material::Update(const json& new_config) { // update whole material
    std::cout << "Material::Update " << new_config << std::endl;
    std::string final_name = config["name"];

    if (new_config.contains("name")) {
        // delete old
        material_name_set.erase(std::string(config["name"]));

        // gen new
        std::string new_name = new_config["name"];
        final_name = new_name;

        int index = 2;
        while (material_name_set.contains(final_name)) {
            final_name = new_name + std::format("_{}", index++);
        }

        material_name_set.insert(final_name);
    }

    config = new_config;
    config["name"] = final_name;


    
}


void PlasticMaterial::ComputeScatteringFunctions(
    MemoryBlock& mb, 
    SurfaceInteraction* si, TransportMode mode,
    bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap)
        Bump(bumpMap, si);

    //si->bsdf = std::make_shared<BSDF>(*si);
    si->bsdf = MB_ALLOC(mb, BSDF)(*si);

    // Initialize diffuse component of plastic material
    Spectrum kd = Kd->Evaluate(*si).Clamp();
    if (!kd.IsBlack())
        //si->bsdf->Add(std::make_shared<LambertianReflection>(kd));
        si->bsdf->Add(MB_ALLOC(mb, LambertianReflection)(kd));

    // Initialize specular component of plastic material
    Spectrum ks = Ks->Evaluate(*si).Clamp();
    if (!ks.IsBlack()) {
        auto fresnel = std::make_shared<FresnelDielectric>(1.5f, 1.f);
        // Create microfacet distribution _distrib_ for plastic material
        float rough = roughness->Evaluate(*si);
        
        if (remapRoughness)
            rough = GGXDistribution::RoughnessToAlpha(rough);

        auto distrib = std::make_shared<GGXDistribution>(rough, rough);
        //auto spec = std::make_shared <MicrofacetReflection>(ks, distrib, fresnel);
        //si->bsdf->Add(std::make_shared<MicrofacetReflection>(ks, distrib, fresnel));
        si->bsdf->Add(MB_ALLOC(mb, MicrofacetReflection)(ks, distrib, fresnel));
    }
}

//////////////////////////////

void MetalMaterial::ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap)
        Bump(bumpMap, si);

    //si->bsdf = std::make_shared<BSDF>(*si);
    si->bsdf = MB_ALLOC(mb, BSDF)(*si);

    float rough = roughness->Evaluate(*si);

    /*float uRough =
        uRoughness ? uRoughness->Evaluate(*si) : roughness->Evaluate(*si);
    float vRough =
        vRoughness ? vRoughness->Evaluate(*si) : roughness->Evaluate(*si);
    if (remapRoughness) {
        uRough = GGXDistribution::RoughnessToAlpha(uRough);
        vRough = GGXDistribution::RoughnessToAlpha(vRough);
    }*/
    //Fresnel* frMf = ARENA_ALLOC(arena, FresnelConductor)(1., eta->Evaluate(*si), k->Evaluate(*si));
    auto fresnel = std::make_shared<FresnelConductor>(1., eta->Evaluate(*si), k->Evaluate(*si));

    auto distrib = std::make_shared<GGXDistribution>(rough, rough);
    
    //si->bsdf->Add(std::make_shared<MicrofacetReflection>(1, distrib, fresnel));
    si->bsdf->Add(MB_ALLOC(mb, MicrofacetReflection)(1, distrib, fresnel));
}