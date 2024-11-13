#pragma once
#include <GLM/glm.hpp>
#include <stack>

#define MAX_SPRITES 128

class Object
{
public:
	glm::fvec2 position;
	glm::float32 rotation;
	glm::fvec2 scale;

	Object(glm::fvec2 _position, glm::float32 _rotation, glm::fvec2 _scale)
	{
		position = _position;
		rotation = _rotation;
		scale = _scale;
	}
	Object(glm::fvec2 _position, glm::float32 _rotation)
	{
		position = _position;
		rotation = _rotation;
		scale = glm::fvec2(1.0f, 1.0f);
	}
	Object(glm::fvec2 _position)
	{
		position = _position;
		rotation = 0;
		scale = glm::fvec2(1.0f, 1.0f);
	}
	Object(glm::fvec2 _position, glm::fvec2 _scale)
	{
		position = _position;
		rotation = 0;
		scale = _scale;
	}

	Object()
	{
		position = glm::fvec2(0.0f, 0.0f);
		rotation = 0;
		scale = glm::fvec2(1.0f, 1.0f);
	}
};
struct InstanceAttributes
{
	glm::fvec2 position;
	glm::float32 rotation;
	glm::fvec2 scale;
	glm::fvec4 atlasRect;
};
class DrawableObject : public Object
{
private:
	InstanceAttributes myInstanceAttributes;
	InstanceAttributes* GlobalInstanceAttributes;
	unsigned int attributeIndex; //index in the global instanceAttributes array

public:

	DrawableObject(glm::fvec2 _position, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes, std::stack<unsigned int>* freeIndices) : Object(_position)
	{
		init(_atlasRect, _instanceAttributes, freeIndices);
	}

	DrawableObject(glm::fvec2 _position, glm::fvec2 _scale, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes, std::stack<unsigned int>* freeIndices) : Object(_position, _scale)
	{
		init(_atlasRect, _instanceAttributes, freeIndices);
	}

	DrawableObject(glm::fvec2 _position, glm::int32 _rotation, glm::fvec2 _scale, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes, std::stack<unsigned int>* freeIndices) : Object(_position, _rotation, _scale)
	{
		init(_atlasRect, _instanceAttributes, freeIndices);
	}

	void SetAttributes(InstanceAttributes* newAttributes)
	{
		GlobalInstanceAttributes[attributeIndex] = *newAttributes;
	}

	InstanceAttributes* GetAttributes()
	{
		return &(GlobalInstanceAttributes[attributeIndex]);
	}

	void init(glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes, std::stack<unsigned int>* freeIndices)
	{
		attributeIndex = freeIndices->top(); //get an available index
		freeIndices->pop(); //remove it from stack as it is no longer free
		myInstanceAttributes.position = position; //assign attributes to local struct
		myInstanceAttributes.rotation = rotation; //..
		myInstanceAttributes.scale = scale; //..
		myInstanceAttributes.atlasRect = _atlasRect; //..
		GlobalInstanceAttributes = _instanceAttributes; //copy pointer to global array of attributes
		SetAttributes(&myInstanceAttributes); //set local copy of attributes to appropriate spot in global attribs
	}
};