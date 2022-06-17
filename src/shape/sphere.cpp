#include "sphere.h"

bool Sphere::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const {
	ray.LogSelf();
	WorldToObject.LogSelf();

	// ray��local���ꡣ����ԭ�㡣
	Ray r = WorldToObject(ray);

	r.LogSelf();

	// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

	// ��sΪ�����ϵĽ���
	// ������ ||s - c||=r
	// ����ray s = o + dt
	// ||o + dt - c|| = r
	// c = 0, 0, 0
	// (o + dt) dot (o + dt) = r*r  �Լ�����Լ��ǳ��ȵ�ƽ��
	// ������õ� oo + 2tod + ttdd = rr
	// ��t��һԪ���η���
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
		x = r sin(��) cos(��)
		y = r sin(��) sin(��)
		z = r cos(��)
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

	float u = phi / phiMax; // �հٷֱ�
	float theta = std::acos(Clamp(hitPoint.z / radius, -1, 1));
	float v = (theta - thetaMin) / (thetaMax - thetaMin); // �ȴ�-1��ʼ�İٷֱȡ�����2��

	// ����
	float zRadius = std::sqrt(hitPoint.x * hitPoint.x + hitPoint.y * hitPoint.y);
	zRadius = hitPoint.z;

	float cosPhi = hitPoint.x / zRadius;
	float sinPhi = hitPoint.y / zRadius;

	// ƫ΢��
	// ��u��Ҳ���Ǧյİٷֱȱ仯����Ϊ΢С�仯�������巽���󵼡���dpdu��

	// ��Ϊʲô�ðٷֱ���Ϊ΢С��������ֱ���æգ�

	// ΢�ֵ����
	// �����x = r sin(��) cos(��)��u��ƫ�����õ�x�ı仯�ʡ�
	// ͬ��õ�yz�ı仯�ʡ���ôxyz�ڼ���������ʵ����һ������/����
	// ˼��һ��2d�ϵ�΢��ͬ����һ�ַ���

	// d (r sin(��) cos(��)) /du
	// = rsin(��) d cos(u*phiMax)/du
	// = -rsin(��)sin(��)phiMax
	// = -y*phiMax
	// ͬ������y��z�ϵ�ƫ������

	// ���õ�dpdu
	Vector3f dpdu(-phiMax * hitPoint.y, phiMax * hitPoint.x, 0);

	// ����ǿ�������ͨxyz����ϵ��֤�ġ�
	// ���Կ�һ�¡��Ȳ��������£�z�ǲ���ġ��ɼ򻯳�xy��2dƽ�档
	// ����һ��Բ�����߷���ȷʵ����(-y, x)�������ܶ��ϡ�����ϸ�u����Ļ���ؽ����һ�µġ�

	// ͬ�������dpdv��
	Vector3f dpdv;


	// ���������Ϣ��ת��world
	*isect = (ObjectToWorld)(SurfaceInteraction(hitPoint, Normalize(Cross(dpdu, dpdv)), Point2f(u, v), -r.d, r.time, this));

	*tHit = t;

	return true;
}

bool Sphere::Intersect(const Ray& ray) const {

	// ray��local���ꡣ����ԭ�㡣
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