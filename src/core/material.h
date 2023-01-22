#ifndef CORE_MATERIAL_H
#define CORE_MATERIAL_H

#include <unordered_set>
#include "pbr.h"
#include "transform.h"
#include "spectrum.h"
#include "../tool/json.h"
#include "../base/memory.h"
#include "object.h"

inline std::unordered_set<std::string> material_name_set;
inline int latest_material_id = 0;

enum class TransportMode { Radiance, Importance };

// Material Declarations
//class Material {
//public:
//    Material(const json& new_config, int keep_id, int inc_last_id) {
//        config = new_config;
//        std::string name = config["name"];
//
//        int index = 2;
//        while (material_name_set.contains(name)) {
//            name = std::string(new_config["name"]) + std::format("_{}", index++);
//        }
//
//        config["name"] = name;
//
//        if (keep_id == 0) {
//            config["id"] = ++latest_material_id;
//        }
//        else {
//            //config["id"] = keep_id;
//            if (inc_last_id)
//                latest_material_id = config["id"];// keep_id;
//        }
//
//        material_name_set.insert(name);
//    }
//
//    virtual ~Material() {
//        material_name_set.erase(std::string(config["name"]));
//    };
//
//    std::string Rename(const std::string new_name) {
//        // delete current
//        material_name_set.erase(std::string(config["name"]));
//
//        // add new
//        std::string name = new_name;
//
//        int index = 2;
//        while (material_name_set.contains(name)) {
//            name = new_name + std::format("_{}", index++);
//        }
//
//        config["name"] = name;
//        material_name_set.insert(name);
//
//        return name;
//    }
//
//    // Material Interface
//    virtual void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes = false) const = 0;
//    
//    static void Bump(const std::shared_ptr<Texture<float>>& d, SurfaceInteraction* si);
//    const json& GetJson() const;
//    int GetID() const;
//
//    void Update(const json& new_config);
//
//    
//
//    json config;
//};

class Material : public Object {
public:
    Material(const json& new_config):
        Object(new_config, ObjectTypeMaterial) {
    }

    virtual ~Material() {};


    // Material Interface
    virtual void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes = false) const = 0;

    static void Bump(const std::shared_ptr<Texture<float>>& d, SurfaceInteraction* si);
};

/////////////////////////////////////////

std::shared_ptr<Material> GenMaterial(const json& material_config);


class MatteMaterial : public Material {
public:
    // MatteMaterial Public Methods
    //MatteMaterial() {}
    MatteMaterial(const json& new_config,
        const std::shared_ptr<Texture<Spectrum>>& Kd,
        const std::shared_ptr<Texture<float>>& sigma,
        const std::shared_ptr<Texture<float>>& bumpMap,
        const std::shared_ptr<Texture<Spectrum>>& checker
    )
        : Material(new_config),
        Kd(Kd), sigma(sigma), bumpMap(bumpMap), checker(checker) {

        config["type"] = "matte";
    }
    void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes = false) const;

private:
    // MatteMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kd;
    std::shared_ptr<Texture<float>> sigma, bumpMap;
    std::shared_ptr<Texture<Spectrum>> checker;
};

/////////////////////////////////////////

class GlassMaterial : public Material {
public:
    // GlassMaterial Public Methods
    GlassMaterial(const json& new_config, 
        const std::shared_ptr<Texture<Spectrum>>& Kr,
        const std::shared_ptr<Texture<Spectrum>>& Kt,
        const std::shared_ptr<Texture<float>>& uRoughness,
        const std::shared_ptr<Texture<float>>& vRoughness,
        const std::shared_ptr<Texture<float>>& index,
        const std::shared_ptr<Texture<float>>& bumpMap,
        bool remapRoughness)
        : Material(new_config),
        Kr(Kr),
        Kt(Kt),
        uRoughness(uRoughness),
        vRoughness(vRoughness),
        index(index),
        bumpMap(bumpMap),
        remapRoughness(remapRoughness) {
        config["type"] = "glass";
    }
    void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes = false) const;

private:
    // GlassMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kr, Kt;
    std::shared_ptr<Texture<float>> uRoughness, vRoughness;
    std::shared_ptr<Texture<float>> index;
    std::shared_ptr<Texture<float>> bumpMap;
    bool remapRoughness;
};


///////////////////////

class MirrorMaterial : public Material {
public:
    // MirrorMaterial Public Methods
    MirrorMaterial(const json& new_config, 
        const std::shared_ptr<Texture<Spectrum>>& r,
        const std::shared_ptr<Texture<float>>& bump):
        Material(new_config) {
        Kr = r;
        bumpMap = bump;

        config["type"] = "mirror";
    }
    void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const;

private:
    // MirrorMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kr;
    std::shared_ptr<Texture<float>> bumpMap;
};

///////////////////////

class PlasticMaterial : public Material {
public:
    // PlasticMaterial Public Methods
    PlasticMaterial(const json& new_config, 
        const std::shared_ptr<Texture<Spectrum>>& Kd,
        const std::shared_ptr<Texture<Spectrum>>& Ks,
        const std::shared_ptr<Texture<float>>& roughness,
        const std::shared_ptr<Texture<float>>& bumpMap,
        bool remapRoughness)
        : Material(new_config),
        Kd(Kd),
        Ks(Ks),
        roughness(roughness),
        bumpMap(bumpMap),
        remapRoughness(remapRoughness) {
        config["type"] = "plastic";
    }
    void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const;

private:
    // PlasticMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kd, Ks;
    std::shared_ptr<Texture<float>> roughness, bumpMap;
    const bool remapRoughness;
};

///////////////////////

class MetalMaterial : public Material {
public:
    // MetalMaterial Public Methods
    MetalMaterial(const json& new_config, 
        const std::shared_ptr<Texture<Spectrum>>& eta,
        const std::shared_ptr<Texture<Spectrum>>& k,
        const std::shared_ptr<Texture<float>>& rough,
        //const std::shared_ptr<Texture<float>>& urough,
        //const std::shared_ptr<Texture<float>>& vrough,
        const std::shared_ptr<Texture<float>>& bump,
        bool remapRoughness)
        : Material(new_config),
        eta(eta),
        k(k),
        roughness(rough),
        //uRoughness(uRoughness),
        //vRoughness(vRoughness),
        bumpMap(bumpMap),
        remapRoughness(remapRoughness) {
        config["type"] = "metal";
    }

    void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* si, TransportMode mode, bool allowMultipleLobes) const;

private:
    // MetalMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> eta, k;
    std::shared_ptr<Texture<float>> roughness;// , uRoughness, vRoughness;
    std::shared_ptr<Texture<float>> bumpMap;
    bool remapRoughness;
};

#endif