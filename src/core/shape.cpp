#include "shape.h"
#include "interaction.h"

Shape::~Shape() {}

Shape::Shape(const Transform &ObjectToWorld, const Transform &WorldToObject):
	ObjectToWorld(ObjectToWorld),
	WorldToObject(WorldToObject)
	{}

Interaction Shape::Sample() {
	return Interaction();
}

float Shape::Pdf() {
	return 0;
}

BBox3f Shape::WorldBound() const {
	return (ObjectToWorld)(ObjectBound());
}