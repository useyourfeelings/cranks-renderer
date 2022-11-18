#include "primitive.h"

BBox3f GeometricPrimitive::WorldBound() const {
    return shape->WorldBound();
}

bool GeometricPrimitive::Intersect(const Ray& r, float* tHit, SurfaceInteraction* isect) const {
    if (!shape->Intersect(r, tHit, isect))
        return false;

    Log("shape->Intersect");

    isect->primitive = this;
    return true;
}

bool GeometricPrimitive::Intersect(const Ray& r) const {
	return shape->Intersect(r);
}

void GeometricPrimitive::ComputeScatteringFunctions(
    MemoryBlock& mb, 
    SurfaceInteraction* isect, 
    //const std::map<int, std::shared_ptr<Material>>& material_list,
    TransportMode mode) const {
    //ProfilePhase p(Prof::ComputeScatteringFuncs);
    /*if (material)
        material->ComputeScatteringFunctions(isect, mode);*/

    //if (material_list.contains(this->material_id)) {
    //    material_list.at(this->material_id)->ComputeScatteringFunctions(isect, mode);
    //}
    //else {
    //    // use default material

    //}

    if (auto parent = scene_obj.lock()) {
        auto material = parent->GetMaterial();

        if (material) {
            material->ComputeScatteringFunctions(mb, isect, mode);
            //std::cout << "GeometricPrimitive::ComputeScatteringFunctions has material" << std::endl;
        }
        else {
            //std::cout << "GeometricPrimitive::ComputeScatteringFunctions no material" << std::endl;
        }
            
    }

}

BasicModel::BasicModel(
    const std::string& name,
    const Transform& ObjectToWorld,
    //const Point3f* P
    //const float* P,
    //const Vector3f* S,
    //const Normal3f* N,
    //const Point2f* UV,
    //const std::shared_ptr<Texture<float>>& alphaMask,
    //const std::shared_ptr<Texture<float>>& shadowAlphaMask,
    //const int* fIndices
    //int material_id
    std::shared_ptr<Material> material,
    std::shared_ptr<Scene> scene
)
    : Object(name),
    //material_id(material_id)
    material(material),
    scene(scene)
    //alphaMask(alphaMask),
    //shadowAlphaMask(shadowAlphaMask)
{
    material_id = material->GetID();
}

std::shared_ptr<Material> BasicModel::GetMaterial() {
    auto m = material.lock();
    if (m) {
        return m;
    }
    else {
        // try get real material
        auto scene_ptr = scene.lock();
        assert(scene_ptr);
        auto m = scene_ptr->GetMaterial(material_id);

        if (m) {
            // update
            material = m;
            material_id = m->GetID();
            return m;
        }
        else {
            throw("failed to get material");
        }
        //else {
        //    // material is gone
        //    material_id = -1;
        //    return nullptr;
        //}
    }
}