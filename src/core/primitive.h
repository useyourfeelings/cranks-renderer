#ifndef CORE_PRIMITIVE_H
#define CORE_PRIMITIVE_H

#include <memory>
#include <string>
#include "pbr.h"
#include "geometry.h"
#include "shape.h"
#include "interaction.h"
#include "material.h"

class Primitive {
public:
    // Primitive Interface
    Primitive() {}
    virtual ~Primitive() {};
    //virtual Bounds3f WorldBound() const = 0;
    virtual bool Intersect(const Ray& r, float* tHit, SurfaceInteraction*) const = 0;
    virtual bool Intersect(const Ray& r) const = 0;
    //virtual const AreaLight* GetAreaLight() const = 0;
    //virtual const Material* GetMaterial() const = 0;
    virtual void ComputeScatteringFunctions(SurfaceInteraction* isect, TransportMode mode) const = 0;

    void SetId(int new_id) {
        id = new_id;
    }

    void SetName(std::string &new_name) {
        name = new_name;
    }

    int id;
    std::string name;
};

class GeometricPrimitive : public Primitive {
public:
    // GeometricPrimitive Public Methods
    //virtual Bounds3f WorldBound() const;
    GeometricPrimitive(const std::shared_ptr<Shape>& shape,
        const std::shared_ptr<Material>& material
        //const std::shared_ptr<AreaLight>& areaLight,
        //const MediumInterface& mediumInterface
    ) :shape(shape),
        material(material) {

    }

    bool Intersect(const Ray& r, float* tHit, SurfaceInteraction* isect) const;
    bool Intersect(const Ray& r) const;

    //const AreaLight* GetAreaLight() const;
    //const Material* GetMaterial() const;
    void ComputeScatteringFunctions(SurfaceInteraction* isect,
        TransportMode mode) const;

private:
    // GeometricPrimitive Private Data
    std::shared_ptr<Shape> shape;
    std::shared_ptr<Material> material;
    //std::shared_ptr<AreaLight> areaLight;
    //MediumInterface mediumInterface;
};



#endif