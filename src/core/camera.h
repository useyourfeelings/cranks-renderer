#ifndef CORE_CAMERA_H
#define CORE_CAMERA_H

#include<memory>
#include<list>
#include "pbr.h"
#include "transform.h"
#include "spectrum.h"

struct CameraSample {
    Point2f pFilm;
    Point2f pLens;
    float time;
};

class Camera {
public:
    //Camera(const Transform& CameraToWorld, std::shared_ptr<Film> film):film(film), CameraToWorld(CameraToWorld) {};
    //Camera(std::shared_ptr<Film> film) :film(film) {};

    Camera() {};
    
    virtual ~Camera() {};

    virtual void SetPerspectiveData(Point3f pos, Point3f look, Vector3f up, float fov, float aspect_ratio, float near, float far, int resX, int resY)  = 0;

    virtual float GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const = 0;

    void AddSample(const Point2f& pixel, Spectrum L, float sampleWeight, int samplesPerPixel);

    /*void SetFilm(std::shared_ptr<Film> film) {
        this->film = film;
    };*/

    void SetFilm();

    Transform CameraToWorld;
    std::shared_ptr<Film> film;

    std::list<std::shared_ptr<Film>> films;

    int resolutionX, resolutionY;
};

class PerspectiveCamera : public Camera  {
public:
    // PerspectiveCamera Public Methods
    //PerspectiveCamera(const Transform &CameraToWorld, const BBox2f& screenWindow, float fov, float asp, std::shared_ptr<Film> film, float near, float far);

    PerspectiveCamera();
    //PerspectiveCamera(Point3f pos, Point3f look, Vector3f up, float fov, float asp, float near, float far, int resX, int resY);
    //float GenerateRay(const CameraSample& sample, Ray*) const;
    
    float GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const;
    
    /*Spectrum We(const Ray& ray, Point2f* pRaster2 = nullptr) const;
    void Pdf_We(const Ray& ray, float* pdfPos, float* pdfDir) const;
    Spectrum Sample_Wi(const Interaction& ref, const Point2f& sample,
        Vector3f* wi, Float* pdf, Point2f* pRaster,
        VisibilityTester* vis) const;*/

    void SetPerspectiveData(Point3f pos, Point3f look, Vector3f up, float fov, float aspect_ratio, float near, float far, int resX, int resY);

    // ProjectiveCamera Data
    Transform CameraToScreen, RasterToCamera;
    Transform ScreenToRaster, RasterToScreen;
    float lensRadius, focalDistance;

//private:
    // PerspectiveCamera Private Data
    Vector3f dxCamera, dyCamera;
    float A;
    float near;
    float far;
    float fov, asp;
};


#endif