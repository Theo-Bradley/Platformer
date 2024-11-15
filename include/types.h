#pragma once
#include <GLM/glm.hpp>
#include <stack>

#define MAX_SPRITES 128

static unsigned int PopFreeIndex();
static void PushFreeIndex(unsigned int freeIndex);

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
	glm::fvec2 scale;
	glm::fvec4 atlasRect;
	glm::float32 rotation;
};
class DrawableObject : public Object
{
private:
	InstanceAttributes myInstanceAttributes; //instance attributes for this instance
	InstanceAttributes* GlobalInstanceAttributes; //ref to global loosely packed instance attribs array
	unsigned int attributeIndex; //index in the global instanceAttributes array
	bool isVisible; //should this sprite be drawn right now

public:

	DrawableObject(glm::fvec2 _position, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes) : Object(_position)
	{
		init(_atlasRect, _instanceAttributes);
	}

	DrawableObject(glm::fvec2 _position, glm::fvec2 _scale, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes) : Object(_position, _scale)
	{
		init(_atlasRect, _instanceAttributes);
	}

	DrawableObject(glm::fvec2 _position, glm::int32 _rotation, glm::fvec2 _scale, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes) : Object(_position, _rotation, _scale)
	{
		init(_atlasRect, _instanceAttributes);
	}

	void EnterScreen()
	{
		attributeIndex = PopFreeIndex(); //get a free index
		//add code to above line to catch case where there are no free indices
		SetAttributes(&myInstanceAttributes); //set local copy of attributes to appropriate spot in global attribs
	}

	void LeaveScreen()
	{
		//push my index to free indices
		//unset my index
	}

	bool DrawInstanced(InstanceAttributes* GPUInstancedAttributes, unsigned int* instanceCount)
	{
		if (isVisible) //if we should be drawn right now
		{
			GPUInstancedAttributes[*instanceCount] = myInstanceAttributes; //fill next free space in array with my attribs
			(*instanceCount)++; //increment the drawn instance count
			return true; //return true (we will be drawn)
		}
		else return false; //else return false (we won't be drawn)
	}

	void SetAttributes(InstanceAttributes* newAttributes)
	{
		GlobalInstanceAttributes[attributeIndex] = *newAttributes;
	}

	InstanceAttributes* GetAttributes()
	{
		return &(GlobalInstanceAttributes[attributeIndex]);
	}

	void init(glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes)
	{
		GlobalInstanceAttributes = _instanceAttributes; //copy pointer to global array of attributes
		myInstanceAttributes.position = position; //assign attributes to local struct
		myInstanceAttributes.rotation = rotation; //..
		myInstanceAttributes.scale = scale; //..
		myInstanceAttributes.atlasRect = _atlasRect; //..
		CheckVisible();
		//TEST CODE
		isVisible = true; //we always want to draw for testing purposes
	}

	void CheckVisible()
	{
		//add check of isVisible to avoid extra calling of Enter/LeaveScreen
		if (true) //replace true with proper onscreen detection
			EnterScreen();
		else
			LeaveScreen();
	}
};