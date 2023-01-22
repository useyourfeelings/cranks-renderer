#ifndef CORE_PRIMITIVE_H
#define CORE_PRIMITIVE_H

#include <memory>
#include <string>
#include "object.h"
#include "pbr.h"
#include "geometry.h"
#include "shape.h"
#include "interaction.h"
#include "material.h"
#include "scene.h"

// 大量的基础形状。不作为Object。
class Primitive {//: public Object {
public:
    // Primitive Interface
    Primitive(){
    }

    virtual ~Primitive() {};
    virtual BBox3f WorldBound() const = 0;
    virtual bool Intersect(const Ray& r, float* tHit, SurfaceInteraction*) const = 0;
    virtual bool Intersect(const Ray& r) const = 0;
    //virtual const AreaLight* GetAreaLight() const = 0; // pbrt page 249 如果此物体发光，会带一个AreaLight。
    //virtual const Material* GetMaterial() const = 0;
    virtual void ComputeScatteringFunctions(MemoryBlock& mb, SurfaceInteraction* isect,
        //const std::map<int, std::shared_ptr<Material>>& material_list,
        TransportMode mode) const = 0;


    virtual std::string GetInfoString() = 0;
};

class GeometricPrimitive : public Primitive {
public:
    // GeometricPrimitive Public Methods
    GeometricPrimitive(const std::string& name,
        const std::shared_ptr<Shape>& shape,
        //const std::shared_ptr<Material>& material,
        //int material_id,
        //Object* parent_obj
        std::shared_ptr<Object> scene_obj
        //const std::shared_ptr<AreaLight>& areaLight,
        //const MediumInterface& mediumInterface
    ) : shape(shape)
        //parent_obj(parent_obj),
        //material_id(material_id) 
    {

        this->scene_obj = scene_obj;
    }

    virtual BBox3f WorldBound() const;

    bool Intersect(const Ray& r, float* tHit, SurfaceInteraction* isect) const;
    bool Intersect(const Ray& r) const;

    //const AreaLight* GetAreaLight() const;
    //const Material* GetMaterial() const;
    void ComputeScatteringFunctions(
        MemoryBlock& mb,
        SurfaceInteraction* isect,
        //const std::map<int, std::shared_ptr<Material>>& material_list,
        TransportMode mode) const;

    std::string GetInfoString() {
        return shape->GetInfoString();
    }

private:
    // GeometricPrimitive Private Data
    std::shared_ptr<Shape> shape;
    //std::shared_ptr<Material> material;
    //int material_id;
    //Object* parent_obj; // 无保护。必须手动确保删除parent时把此物体删除。
    std::weak_ptr<Object> scene_obj;
    //std::shared_ptr<AreaLight> areaLight;
    //MediumInterface mediumInterface;
};



class BasicModel : public Object {
public:
    // TriangleMesh Public Methods
    BasicModel(const json& new_config, const Transform& ObjectToWorld, 
        //const float* P,
        //const Vector3f* S,
        //const Normal3f* N,
        //const Point2f* uv,
        //const std::shared_ptr<Texture<float>>& alphaMask,
        //const std::shared_ptr<Texture<float>>& shadowAlphaMask,
        //const int* faceIndices
        //int material_id
        std::shared_ptr<Material> material,
        std::shared_ptr<Scene> scene,
        std::shared_ptr<Medium> medium
    );

    virtual std::shared_ptr<Material> GetMaterial();

    //std::unique_ptr<Point3f[]> p;
    //std::unique_ptr<Normal3f[]> n;
    //std::unique_ptr<Vector3f[]> s;
    //std::unique_ptr<Point2f[]> uv;
    //std::shared_ptr<Texture<float>> alphaMask, shadowAlphaMask;
    //std::vector<int> faceIndices;

    int material_id;
    std::weak_ptr<Material> material;
    std::weak_ptr<Medium> medium;
    std::weak_ptr<Scene> scene;
};


#endif