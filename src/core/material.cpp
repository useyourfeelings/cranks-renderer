#include "material.h"
#include "reflection.h"

void MatteMaterial::ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode) const {
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

    Spectrum r;
    r.c[0] = 0.8;
    r.c[1] = 0.2;    
    r.c[2] = 0.4;

    si->bsdf = std::make_shared<BSDF>(*si);

    si->bsdf->Add(std::make_shared<LambertianReflection>(r));

}

void Material::Bump(const std::shared_ptr<Texture<float>>& d,
    SurfaceInteraction* si) {
}
