#ifndef CORE_CAMERA_H
#define CORE_CAMERA_H

#include<memory>
#include "transform.h"
#include "film.h"

struct CameraSample {
    Point2f pFilm;
    Point2f pLens;
    float time;
};

class Camera {
public:
    Camera(const Transform& CameraToWorld, std::shared_ptr<Film> film):film(film) {};
    virtual ~Camera() {};

    virtual float GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const = 0;

    void AddSample(const Point2f& pixel, Spectrum L, float sampleWeight = 1.);

    Transform CameraToWorld;
    std::shared_ptr<Film> film;
};

//class ProjectiveCamera:public Camera {
//public:
//	ProjectiveCamera() {}
//
//
//
//
//};


class PerspectiveCamera : public Camera  {
public:
    // PerspectiveCamera Public Methods
    PerspectiveCamera(const Transform &CameraToWorld, const BBox2f& screenWindow,
        float fov, float asp, std::shared_ptr<Film> film, float near, float far);
    //float GenerateRay(const CameraSample& sample, Ray*) const;
    
    float GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const;
    
    /*Spectrum We(const Ray& ray, Point2f* pRaster2 = nullptr) const;
    void Pdf_We(const Ray& ray, float* pdfPos, float* pdfDir) const;
    Spectrum Sample_Wi(const Interaction& ref, const Point2f& sample,
        Vector3f* wi, Float* pdf, Point2f* pRaster,
        VisibilityTester* vis) const;*/

    // ProjectiveCamera Data
    Transform CameraToScreen, RasterToCamera;
    Transform ScreenToRaster, RasterToScreen;
    float lensRadius, focalDistance;

//private:
    // PerspectiveCamera Private Data
    //Vector3 dxCamera, dyCamera;
    float A;
    float near;
    float far;
    float fov, asp;
};


#endif