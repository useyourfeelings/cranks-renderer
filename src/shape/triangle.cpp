#include "triangle.h"
#include "../base/efloat.h"
#include <cassert>

std::string Triangle::GetInfoString() const {
    return std::format("[{}, {}, {}] [{}, {}, {}] [{}, {}, {}]", 
        mesh->p[v[0]].x, mesh->p[v[0]].y, mesh->p[v[0]].z,
        mesh->p[v[1]].x, mesh->p[v[1]].y, mesh->p[v[1]].z,
        mesh->p[v[2]].x, mesh->p[v[2]].y, mesh->p[v[2]].z);
}

BBox3f Triangle::ObjectBound() const {
    // Get triangle vertices in _p0_, _p1_, and _p2_
    const Point3f& p0 = mesh->p[v[0]];
    const Point3f& p1 = mesh->p[v[1]];
    const Point3f& p2 = mesh->p[v[2]];
    return Union(BBox3f((WorldToObject)(p0), (WorldToObject)(p1)), (WorldToObject)(p2));
}

BBox3f Triangle::WorldBound() const {
    // Get triangle vertices in _p0_, _p1_, and _p2_
    const Point3f& p0 = mesh->p[v[0]];
    const Point3f& p1 = mesh->p[v[1]];
    const Point3f& p2 = mesh->p[v[2]];

    /*std::cout << GetInfoString();

    auto b = Union(BBox3f(p0, p1), p2);
    std::cout << std::format("WorldBound [{}, {}, {}] [{}, {}, {}]\n",
        b.pMin.x, b.pMin.y, b.pMin.z,
        b.pMax.x, b.pMax.y, b.pMax.z);*/

    return Union(BBox3f(p0, p1), p2);
}

// 目前的理解。
// 允许shared_ptr被删除，此时weak_ptr可判断shared_ptr还是否有效。
// 但如果shared_ptr被替换，weak_ptr无法自动更新，还是绑定老的shared_ptr。
// 能否这样。保留material_id，如果weak_ptr判断shared_ptr失效，则尝试根据material_id去找新的shared_ptr并更新weak_ptr。
// 如果是被删，则material_id找不到新的shared_ptr，返回默认material。
// 如果是更新，则weak_ptr更新为新的shared_ptr。
// 
// 总体是借助material_id来判断shared_ptr的真实情况，进行更新。
std::shared_ptr<Material> TriangleMesh::GetMaterial() {
    auto m = material.lock();
    if (m) {
        //std::cout << "TriangleMesh::GetMaterial() ok" << m->GetJson() << std::endl;
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
            //std::cout << "TriangleMesh::GetMaterial() new" << m->GetJson() << std::endl;
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

std::shared_ptr<Medium> TriangleMesh::GetMedium() {
    auto m = medium.lock();
    if (m) {
        //std::cout << "TriangleMesh::GetMaterial() ok" << m->GetJson() << std::endl;
        return m;
    }
    else {
        // try get real material
        auto scene_ptr = scene.lock();
        assert(scene_ptr);
        auto m = scene_ptr->GetMedium(medium_id);

        if (m) {
            // update
            medium = m;
            medium_id = m->GetID();
            //std::cout << "TriangleMesh::GetMaterial() new" << m->GetJson() << std::endl;
            return m;
        }
        else {
            //throw("failed to get medium");

            return nullptr;
        }
        //else {
        //    // material is gone
        //    material_id = -1;
        //    return nullptr;
        //}
    }
}

TriangleMesh::TriangleMesh(
    const json& config,
    const Transform& ObjectToWorld, int nTriangles, const int* vertexIndices,
    int nVertices,
    //const Point3f* P
    const float* P,
    //const Vector3f* S,
    const float* N,
    //const Point2f* UV,
    //const std::shared_ptr<Texture<float>>& alphaMask,
    //const std::shared_ptr<Texture<float>>& shadowAlphaMask,
    //const int* fIndices
    //int material_id
    std::shared_ptr<Material> material,
    std::shared_ptr<Scene> scene,
    std::shared_ptr<Medium> medium
)
    : Object(config, ObjectTypeScene),
    nTriangles(nTriangles),
    nVertices(nVertices),
    vertexIndices(vertexIndices, vertexIndices + 3 * nTriangles),
    //material_id(material_id)
    material(material),
    scene(scene),
    medium(medium)
    //alphaMask(alphaMask),
    //shadowAlphaMask(shadowAlphaMask)
    {

    material_id = material->GetID();

    if(medium) // medium可不设定
        medium_id = medium->GetID();
    else
        medium_id = 0;

    /*++nMeshes;
    nTris += nTriangles;
    triMeshBytes += sizeof(*this) + this->vertexIndices.size() * sizeof(int) +
        nVertices * (sizeof(*P) + (N ? sizeof(*N) : 0) +
            (S ? sizeof(*S) : 0) + (UV ? sizeof(*UV) : 0) +
            (fIndices ? sizeof(*fIndices) : 0));*/

    // Transform mesh vertices to world space
    std::cout << "points:" << std::endl;
    p.reset(new Point3f[nVertices]);
    for (int i = 0; i < nVertices; ++i) {
        //p[i] = ObjectToWorld(P[i]);
        p[i] = ObjectToWorld(Point3f(P[i * 3], P[i * 3 + 1], P[i * 3 + 2]));
        //std::cout << P[i * 3] << " " << P[i * 3 + 1] << " " << P[i * 3 + 2] << std::endl;
    }

    std::cout << "normal:" << std::endl;
    /*n.reset(new Vector3f[nTriangles]);
    for (int i = 0; i < nTriangles; ++i) {
        //p[i] = ObjectToWorld(P[i]);
        n[i] = ObjectToWorld(Vector3f(N[i * 3], N[i * 3 + 1], N[i * 3 + 2]));
        //std::cout <<"wtf " << N[i * 3] << " " << N[i * 3 + 1] << " " << N[i * 3 + 2] << std::endl;
        std::cout << "wtf " << n[i].x << " " << n[i].y << " " << n[i].z << std::endl;
    }*/

    n.reset(new Vector3f[nVertices]);
    for (int i = 0; i < nVertices; ++i) {
        //p[i] = ObjectToWorld(P[i]);
        n[i] = ObjectToWorld(Vector3f(N[i * 3], N[i * 3 + 1], N[i * 3 + 2]));
        //std::cout <<"wtf " << N[i * 3] << " " << N[i * 3 + 1] << " " << N[i * 3 + 2] << std::endl;
        //std::cout << "wtf " << n[i].x << " " << n[i].y << " " << n[i].z << std::endl;
    }

    /*std::cout << "vertexIndices:" << std::endl;
    for (int i = 0; i < nTriangles; ++i) {
        std::cout << vertexIndices[i * 3] << " " << vertexIndices[i * 3 + 1] << " " << vertexIndices[i * 3 + 2] << std::endl;
    }*/
        

    // Copy _UV_, _N_, and _S_ vertex data, if present
    /*if (UV) {
        uv.reset(new Point2f[nVertices]);
        memcpy(uv.get(), UV, nVertices * sizeof(Point2f));
    }
    if (N) {
        n.reset(new Normal3f[nVertices]);
        for (int i = 0; i < nVertices; ++i) n[i] = ObjectToWorld(N[i]);
    }
    if (S) {
        s.reset(new Vector3f[nVertices]);
        for (int i = 0; i < nVertices; ++i) s[i] = ObjectToWorld(S[i]);
    }

    if (fIndices)
        faceIndices = std::vector<int>(fIndices, fIndices + nTriangles);
    */
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
bool Triangle::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const {
    const float EPSILON = 0.0000001;

    const Point3f& p0 = mesh->p[v[0]];
    const Point3f& p1 = mesh->p[v[1]];
    const Point3f& p2 = mesh->p[v[2]];

    Vector3f edge_1 = p1 - p0;
    Vector3f edge_2 = p2 - p0;

    Vector3f h = Cross(ray.d, edge_2);
    float a = Dot(edge_1, h);

    if (a > -EPSILON && a < EPSILON) {
        return false; // parallel
    }

    float f = 1 / a;
    Vector3f s = ray.o - p0;
    float u = f * Dot(s, h);

    if (u < 0 || u > 1) {
        return false;
    }

    Vector3f q = Cross(s, edge_1);
    float v = f * Dot(ray.d, q);

    if (v < 0 || u + v > 1) {
        return false;
    }

    float t = f * Dot(edge_2, q);

    if (t > EPSILON) {
        if (t > ray.tMax)
            return false;

        *tHit = t;

        auto hitPoint = ray(float(t)); // get hit position

        Vector3f fake_dpdu = edge_1;
        Vector3f fake_dpdv = edge_2;

        // todo: make correct gamma. 8 is fake.
        Vector3f pError = gamma(8) * Abs((Vector3f)hitPoint);

        //std::cout << std::format("Intersect v[0] v[1] v[2] {} {} {}\n", this->v[0], this->v[1], this->v[2]);
        //std::cout << std::format("Intersect {} {} {}\n", mesh->n[this->v[0]].x, mesh->n[this->v[0]].y, mesh->n[this->v[0]].z);

        bool from_outside = false;

        if (Dot(mesh->n[this->v[0]], -ray.d) > 0.0f)
            from_outside = true;

        *isect = SurfaceInteraction(
            hitPoint,
            pError,
            mesh->n[this->v[0]], // 直接用模型的normal // hitPoint - Point3f(0, 0, 0),
            Point2f(u, v), -ray.d,
            fake_dpdu, fake_dpdv,// fake test dpdu, dpdv,
            ray.time, this, from_outside);

        return true;
    }
    else {
        return false;
    }
}

bool Triangle::Intersect(const Ray& ray) const {
    const float EPSILON = 0.0000001;

    const Point3f& p0 = mesh->p[v[0]];
    const Point3f& p1 = mesh->p[v[1]];
    const Point3f& p2 = mesh->p[v[2]];

    Vector3f edge_1 = p1 - p0;
    Vector3f edge_2 = p2 - p0;

    Vector3f h = Cross(ray.d, edge_2);
    float a = Dot(edge_1, h);

    if (a > -EPSILON && a < EPSILON) {
        return false; // parallel
    }

    float f = 1 / a;
    Vector3f s = ray.o - p0;
    float u = f * Dot(s, h);

    if (u < 0 || u > 1) {
        return false;
    }

    Vector3f q = Cross(s, edge_1);
    float v = f * Dot(ray.d, q);

    if (v < 0 || u + v > 1) {
        return false;
    }

    float t = f * Dot(edge_2, q);

    if (t > EPSILON) {
        return true;
    }
    else {
        return false;
    }
}