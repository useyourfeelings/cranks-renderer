#include "camera.h"
#include "film.h"
//#include "../tool/logger.h"

//float Camera::GenerateRayDifferential(const CameraSample& sample,
//    RayDifferential* rd) const {
//
//    Log("GenerateRayDifferential wtf");
//
//    return 0;
//}

void Camera::AddSample(const Point2f& pixel, Spectrum L, float sampleWeight, int samplesPerPixel) {
    int pixel_index = pixel.x + pixel.y * resolutionX;
    film->pixels[pixel_index] += L * sampleWeight / samplesPerPixel;
}

void PerspectiveCamera::SetPerspectiveData(Point3f pos, Point3f look, Vector3f up, float fov, float aspect_ratio, float near, float far, int resX, int resY) {
    Log("SetPerspectiveData");
    this->CameraToWorld = Inverse(LookAt(pos, look, up));
    CameraToWorld.LogSelf();

    //float aspectRatio = film->fullResolution.x / film->fullResolution.y;

    this->asp = aspect_ratio;
    this->fov = fov;
    this->near = near;
    this->far = far;
    this->resolutionX = resX;
    this->resolutionY = resY;

    Point2f screenMin, screenMax;

    if (aspect_ratio > 1.f) {
        screenMin.x = -aspect_ratio;
        screenMax.x = aspect_ratio;
        screenMin.y = -1.f;
        screenMax.y = 1.f;
    }
    else {
        screenMin.x = -1.f;
        screenMax.x = 1.f;
        screenMin.y = -1.f / aspect_ratio;
        screenMax.y = 1.f / aspect_ratio;
    }

    // screenWindow是趋近于ndc的一个东西

    // camera转到ndc
    //CameraToScreen = Perspective(fov, asp, 1e-2f, 1000.f);
    CameraToScreen = Perspective(fov, asp, near, far);

    // 从ndc转到2d像素坐标。
    ScreenToRaster =
        //Scale(film->fullResolution.x, film->fullResolution.y, 1) * // 再scale。放大到film的真实大小，也就是最终图像的2d坐标。
        Scale(resX, resY, 1) * // 再scale。放大到film的真实大小，也就是最终图像的2d坐标。
        Scale(1 / (screenMax.x - screenMin.x),     // 再scale归一，映射到[0, 1]。注意y是负的，就是越往下越大。
            1 / (screenMin.y - screenMax.y), 1) *
        Translate(Vector3f(-screenMin.x, -screenMax.y, 0)); // 先Translate，根据上面的screen。整体往右下挪，左上挪到原点。

    RasterToScreen = Inverse(ScreenToRaster); // 逆

    // 2d到ndc，再到camera。
    // 可以从相片上的某个点还原到camera空间。
    RasterToCamera = Inverse(CameraToScreen) * RasterToScreen;
}

PerspectiveCamera::PerspectiveCamera(Point3f pos, Point3f look, Vector3f up, float fov, float asp, float near, float far, int resX, int resY):
	//Camera(film), 
    near(near), far(far), fov(fov), asp(asp)
{
	//A = film->fullResolution.x * film->fullResolution.y;

    SetPerspectiveData(pos, look, up, fov, asp, near, far, resX, resY);
}

//float PerspectiveCamera::GenerateRay(const CameraSample& sample, Ray* ray) const {
//    // Compute raster and camera sample positions
//    Point3f pFilm = Point3f(sample.pFilm.x, sample.pFilm.y, 0);
//    Point3f pCamera = RasterToCamera(pFilm);
//    *ray = Ray(Point3f(0, 0, 0), Normalize(Vector3f(pCamera)));
//    // Modify ray for depth of field
//    if (lensRadius > 0) {
//        // Sample point on lens
//        Point2f pLens = lensRadius * ConcentricSampleDisk(sample.pLens);
//
//        // Compute point on plane of focus
//        float ft = focalDistance / ray->d.z;
//        Point3f pFocus = (*ray)(ft);
//
//        // Update ray for effect of lens
//        ray->o = Point3f(pLens.x, pLens.y, 0);
//        ray->d = Normalize(pFocus - ray->o);
//    }
//    ray->time = Lerp(sample.time, shutterOpen, shutterClose);
//    ray->medium = medium;
//    *ray = CameraToWorld(*ray);
//    return 1;
//}

float PerspectiveCamera::GenerateRayDifferential(const CameraSample& sample,
    RayDifferential* ray) const {
    //ProfilePhase prof(Prof::GenerateCameraRay);
    // Compute raster and camera sample positions
    Point3f pFilm = Point3f(sample.pFilm.x, sample.pFilm.y, 0);
    Point3f pCamera = RasterToCamera(pFilm); // 从相片上的某个点还原到camera空间的near面上。

    Log("GenerateRayDifferential RasterToCamera");
    pFilm.LogSelf();
    Log("screen to camera");
    pCamera.LogSelf();

    // 相机空间里相机就在原点。pCamera-0即可得到光的方向。
    Vector3f dir = Normalize(Vector3f(pCamera.x, pCamera.y, pCamera.z));

    *ray = RayDifferential(Point3f(0, 0, 0), dir); // 相机空间的光
    Log("ray init");
    ray->LogSelf();
    
    /* 暂时忽略
    // Modify ray for depth of field
    if (lensRadius > 0) {
        // Sample point on lens
        Point2f pLens = lensRadius * ConcentricSampleDisk(sample.pLens);

        // Compute point on plane of focus
        Float ft = focalDistance / ray->d.z;
        Point3f pFocus = (*ray)(ft);

        // Update ray for effect of lens
        ray->o = Point3f(pLens.x, pLens.y, 0);
        ray->d = Normalize(pFocus - ray->o);
    }

    // Compute offset rays for _PerspectiveCamera_ ray differentials
    if (lensRadius > 0) {
        // Compute _PerspectiveCamera_ ray differentials accounting for lens

        // Sample point on lens
        Point2f pLens = lensRadius * ConcentricSampleDisk(sample.pLens);
        Vector3f dx = Normalize(Vector3f(pCamera + dxCamera));
        Float ft = focalDistance / dx.z;
        Point3f pFocus = Point3f(0, 0, 0) + (ft * dx);
        ray->rxOrigin = Point3f(pLens.x, pLens.y, 0);
        ray->rxDirection = Normalize(pFocus - ray->rxOrigin);

        Vector3f dy = Normalize(Vector3f(pCamera + dyCamera));
        ft = focalDistance / dy.z;
        pFocus = Point3f(0, 0, 0) + (ft * dy);
        ray->ryOrigin = Point3f(pLens.x, pLens.y, 0);
        ray->ryDirection = Normalize(pFocus - ray->ryOrigin);
    }
    else {
        ray->rxOrigin = ray->ryOrigin = ray->o;
        ray->rxDirection = Normalize(Vector3f(pCamera) + dxCamera);
        ray->ryDirection = Normalize(Vector3f(pCamera) + dyCamera);
    }
    ray->time = Lerp(sample.time, shutterOpen, shutterClose);
    ray->medium = medium;
    */

    *ray = CameraToWorld(*ray); // 回到world

    Log("ray to world");
    CameraToWorld.LogSelf();
    ray->LogSelf();

    ray->hasDifferentials = true;
    return 1;
}