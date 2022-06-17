#include "sphere.h"

bool Sphere::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const {
	ray.LogSelf();
	WorldToObject.LogSelf();

	// ray到local坐标。球在原点。
	Ray r = WorldToObject(ray);

	r.LogSelf();

	// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

	// 设s为球面上的交点
	// 对于球 ||s - c||=r
	// 对于ray s = o + dt
	// ||o + dt - c|| = r
	// c = 0, 0, 0
	// (o + dt) dot (o + dt) = r*r  自己点乘自己是长度的平方
	// 算出来得到 oo + 2tod + ttdd = rr
	// 解t的一元二次方程
	// a = dd
	// b = 2od
	// c = oo - rr
	// att + bt + c = 0

	float t1, t2, t;

	float a = Dot(r.d, r.d);

	float b = 2 * Dot(Vector3f(r.o.x, r.o.y, r.o.z), r.d);

	float c = Dot(Vector3f(r.o.x, r.o.y, r.o.z), Vector3f(r.o.x, r.o.y, r.o.z)) - radius * radius;

	Log("Sphere wtf1 %f %f %f", a, b, c);

	if (Quadratic(a, b, c, &t1, &t2) == 0)
		return false; // no root

	Log("Sphere wtf2");

	if (t1 > r.tMax || t2 <= 0) // out of range
		return false;

	Log("Sphere wtf3");

	// find nearest t and check
	t = t1;
	if (t <= 0) {
		t = t2;
		if (t > r.tMax)
			return false;
	}

	Log("Sphere wtf4");

	auto hitPoint = ray(t); // get hit position


	// todo 

	/*
		x = r sin(θ) cos(φ)
		y = r sin(θ) sin(φ)
		z = r cos(θ)
	*/

	// radius default = 1
	// zmin default -radius
	// zmax default radius
	// thetaMin default -1
	// thetaMax default 1
	// phimax default = 360      Radians(360) 2pi

	float phi = std::atan2(hitPoint.y, hitPoint.x);
	if (phi < 0)
		phi += 2 * Pi;

	float u = phi / phiMax; // φ百分比
	float theta = std::acos(Clamp(hitPoint.z / radius, -1, 1));
	float v = (theta - thetaMin) / (thetaMax - thetaMin); // θ从-1开始的百分比。总量2。

	// ？？
	float zRadius = std::sqrt(hitPoint.x * hitPoint.x + hitPoint.y * hitPoint.y);
	zRadius = hitPoint.z;

	float cosPhi = hitPoint.x / zRadius;
	float sinPhi = hitPoint.y / zRadius;

	// 偏微分
	// 以u，也就是φ的百分比变化，作为微小变化，对球体方程求导。及dpdu。

	// 他为什么用百分比作为微小量而不是直接用φ？

	// 微分的理解
	// 比如对x = r sin(θ) cos(φ)求u的偏导。得到x的变化率。
	// 同理得到yz的变化率。那么xyz在几何意义上实际是一个向量/方向。
	// 思考一下2d上的微分同样是一种方向。

	// d (r sin(θ) cos(φ)) /du
	// = rsin(θ) d cos(u*phiMax)/du
	// = -rsin(θ)sin(φ)phiMax
	// = -y*phiMax
	// 同样可求y和z上的偏导数。

	// 最后得到dpdu
	Vector3f dpdu(-phiMax * hitPoint.y, phiMax * hitPoint.x, 0);

	// 这个是可以在普通xyz坐标系验证的。
	// 粗略看一下。θ不变的情况下，z是不变的。可简化成xy的2d平面。
	// 看看一个圆上切线方向。确实就是(-y, x)，方向能对上。如果严格按u来算的话想必结果是一致的。

	// 同样可算出dpdv。
	Vector3f dpdv;


	// 最后生成信息并转回world
	*isect = (ObjectToWorld)(SurfaceInteraction(hitPoint, Normalize(Cross(dpdu, dpdv)), Point2f(u, v), -r.d, r.time, this));

	*tHit = t;

	return true;
}

bool Sphere::Intersect(const Ray& ray) const {

	// ray到local坐标。球在原点。
	Ray r = WorldToObject(ray);

	// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

	float t1, t2, t;

	float a = Dot(r.d, r.d);

	float b = 2 * Dot(Vector3f(r.o.x, r.o.y, r.o.z), r.d);

	float c = Dot(Vector3f(r.o.x, r.o.y, r.o.z), Vector3f(r.o.x, r.o.y, r.o.z)) - radius * radius;

	if (Quadratic(a, b, c, &t1, &t2) == 0)
		return false; // no root

	if (t1 > r.tMax || t2 <= 0) // out of range
		return false;

	// find nearest t and check
	t = t1;
	if (t <= 0) {
		t = t2;
		if (t > r.tMax)
			return false;
	}

	return true;
}

Interaction Sphere::Sample() {
	return Interaction();
}

float Sphere::Pdf() {
	return 0;
}