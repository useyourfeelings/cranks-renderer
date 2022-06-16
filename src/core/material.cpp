#include "material.h"

void MatteMaterial::ComputeScatteringFunctions(SurfaceInteraction* si, TransportMode mode) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);

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
}

void Material::Bump(const std::shared_ptr<Texture<float>>& d,
    SurfaceInteraction* si) {
}
