#include "material.h"
#include "texture.h"
#include "reflection.h"
#include "pbr.h"

void MatteMaterial::ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
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

    Spectrum r = Kd->Evaluate(*si).Clamp();
    float sig = Clamp(sigma->Evaluate(*si), 0, 90);

    si->bsdf = std::make_shared<BSDF>(*si);

    if (sig)
        ;

    si->bsdf->Add(std::make_shared<LambertianReflection>(r));

}


//////////////////////////////////

void GlassMaterial::ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    float eta = index->Evaluate(*si);
    float urough = uRoughness->Evaluate(*si);
    float vrough = vRoughness->Evaluate(*si);
    Spectrum R = Kr->Evaluate(*si).Clamp();
    Spectrum T = Kt->Evaluate(*si).Clamp();
    // Initialize _bsdf_ for smooth or rough dielectric
    si->bsdf = std::make_shared<BSDF>(*si);

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
                si->bsdf->Add(std::make_shared<SpecularReflection>(R, fresnel));
            }
            //else
            //    si->bsdf->Add(ARENA_ALLOC(arena, MicrofacetReflection)(R, distrib, fresnel));
        }

        if (!T.IsBlack()) {
            if (isSpecular) {
                si->bsdf->Add(std::make_shared<SpecularTransmission>(T, 1.f, eta, mode));
            }
                
            
            //else
            //    si->bsdf->Add(ARENA_ALLOC(arena, MicrofacetTransmission)(T, distrib, 1.f, eta, mode));
        }
    }
}

//////////////////////////////

void MirrorMaterial::ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    //si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);

    si->bsdf = std::make_shared<BSDF>(*si);

    Spectrum R = Kr->Evaluate(*si).Clamp();
    if (!R.IsBlack())
        //si->bsdf->Add(ARENA_ALLOC(arena, SpecularReflection)(R, ARENA_ALLOC(arena, FresnelNoOp)()));
        si->bsdf->Add(std::make_shared<SpecularReflection>(R, std::make_shared<FresnelNoOp>()));
}

//////////////////////////////

void Material::Bump(const std::shared_ptr<Texture<float>>& d,
    SurfaceInteraction* si) {
}


void PlasticMaterial::ComputeScatteringFunctions(
    SurfaceInteraction* si, TransportMode mode,
    bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap)
        Bump(bumpMap, si);

    si->bsdf = std::make_shared<BSDF>(*si);
    // Initialize diffuse component of plastic material
    Spectrum kd = Kd->Evaluate(*si).Clamp();
    if (!kd.IsBlack())
        si->bsdf->Add(std::make_shared<LambertianReflection>(kd));

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
        si->bsdf->Add(std::make_shared<MicrofacetReflection>(ks, distrib, fresnel));
    }
}

//////////////////////////////

void MetalMaterial::ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap)
        Bump(bumpMap, si);

    si->bsdf = std::make_shared<BSDF>(*si);

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
    
    si->bsdf->Add(std::make_shared<MicrofacetReflection>(1, distrib, fresnel));
}