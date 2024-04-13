#include "Object.h"

Object::Object(const Transform& transform) : m_transform (transform)
{
}

void Object::SetTexture(const Texture& texture)
{
	this->m_texture = texture;
}
