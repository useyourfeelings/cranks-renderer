#ifndef CORE_MEDIUM_H
#define CORE_MEDIUM_H

#include"pbr.h"
#include"spectrum.h"
#include"sampler.h"
#include"../base/memory.h"
#include <unordered_set>
#include "object.h"

inline float PhaseHG(float cosTheta, float g) {
    float denom = 1 + g * g + 2 * g * cosTheta;
    return Inv4Pi * (1 - g * g) / (denom * std::sqrt(denom));
}

class PhaseFunction {
public:
    virtual ~PhaseFunction() {};
    virtual float p(const Vector3f& wo, const Vector3f& wi) const = 0;
    virtual float Sample_p(const Vector3f& wo, Vector3f* wi,
        const Point2f& u) const = 0;
};

class HenyeyGreenstein : public PhaseFunction {
public:
    HenyeyGreenstein(float g) : g(g) {}
    float p(const Vector3f& wo, const Vector3f& wi) const;
    float Sample_p(const Vector3f& wo, Vector3f* wi,
        const Point2f& sample) const;

private:
    const float g;
};

inline std::unordered_set<std::string> medium_name_set;
inline int latest_medium_id = 0;

class Medium : public Object {
public:
    //virtual ~Medium() {}
    virtual Spectrum Tr(const Ray& ray, Sampler& sampler) const = 0;
    virtual Spectrum Sample(const Ray& ray, Sampler& sampler, MemoryBlock& mb, MediumInteraction* mi, std::shared_ptr<Medium> this_medium) = 0;

    //

    //Medium() {}

    Medium(const json& new_config):
        Object(new_config, ObjectTypeMedium) {
    }

    virtual ~Medium() {
    };

};

// 均匀介质
class HomogeneousMedium : public Medium {//; , public std::enable_shared_from_this<Medium> {
public:
    HomogeneousMedium(const json& new_config, const Spectrum& sigma_a, const Spectrum& sigma_s, float g)
        : sigma_a(sigma_a),
        sigma_s(sigma_s),
        sigma_t(sigma_s + sigma_a),
        g(g),
        Medium(new_config) {
    }
    Spectrum Tr(const Ray& ray, Sampler& sampler) const;
    Spectrum Sample(const Ray& ray, Sampler& sampler, MemoryBlock& mb, MediumInteraction* mi, std::shared_ptr<Medium> this_medium);

private:
    const Spectrum sigma_a, sigma_s, sigma_t;
    const float g;
};

struct MediumInterface {
    // 这里我只启用inside
    //MediumInterface() : inside(), outside() {}
    /*MediumInterface(const Medium* medium) : inside(medium), outside(medium) {}
    MediumInterface(const Medium* inside, const Medium* outside)*/

    //MediumInterface(std::shared_ptr<Medium> medium = nullptr) : inside(medium), outside(medium) {}

    MediumInterface(std::shared_ptr<Medium> medium = nullptr) : inside(medium) {}

    MediumInterface(std::shared_ptr<Medium> inside, std::shared_ptr<Medium> outside)
        : inside(inside), outside(outside) {}

    //bool IsMediumTransition() const { return inside != outside; }

    std::weak_ptr<Medium> inside, outside; // 物体内部和外部的介质。不检查一致性，客户端必须确保场景数据正确。这里我只启用inside。
};


std::shared_ptr<Medium> MakeHomogeneousMedium(const json& material_config);

#endif