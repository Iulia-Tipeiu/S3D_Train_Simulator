#pragma once
#include "Transform.h"
#include "Texture.h"
class Object
{
protected:
	Transform m_transform;
	Texture m_texture;
	Object(const Transform& transform);

	void SetTexture(const Texture& texture);

};

