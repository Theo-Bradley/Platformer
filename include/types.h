#pragma once
#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stack>
#include <fstream>
#include <iostream>
#include <string>

#define MAX_SPRITES 128

float plane[] = {
	-0.5f, -0.5f, 0.0f,
	0.0f, 0.0f,
	0.5f, -0.5f, 0.0f,
	1.0f, 0.0f,
	0.5f, 0.5f, 0.0f,
	1.0f, 1.0f,
	-0.5f, 0.5f, 0.0f,
	0.0f, 1.0f };

unsigned int planeIndices[6] = {
	0, 1, 2,
	2, 3, 0
};


class DrawableObject;
static unsigned int PopFreeIndex();
static void PushFreeIndex(unsigned int freeIndex);
glm::vec4 CalculateScreenRect(glm::mat4 projViewMat);
bool RectContainsPoint(glm::vec4 rect, glm::vec3 point);
bool RectContainsSprite(glm::vec4 rect, DrawableObject* obj);

GLuint errShader = 0;

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
	glm::mat4 pvmMatrix;
	glm::fvec4 atlasRect; //l,r,t,b
};
class DrawableObject : public Object
{
private:
	InstanceAttributes myInstanceAttributes; //instance attributes for this instance
	InstanceAttributes* GlobalInstanceAttributes; //ref to global loosely packed instance attribs array
	unsigned int attributeIndex; //index in the global instanceAttributes array
	bool isVisible; //should this sprite be drawn right now
	glm::mat4 pvMatrix; //proj view matrix

public:

	DrawableObject(glm::fvec2 _position, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes, glm::mat4 _pvMatrix) : Object(_position)
	{
		init(_atlasRect, _instanceAttributes, _pvMatrix);
	}

	DrawableObject(glm::fvec2 _position, glm::fvec2 _scale, glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes,
	glm::mat4 _pvMatrix) : Object(_position, _scale)
	{
		init(_atlasRect, _instanceAttributes, _pvMatrix);
	}

	DrawableObject(glm::fvec2 _position, glm::float32 _rotation, glm::fvec2 _scale, glm::fvec4 _atlasRect,
		InstanceAttributes* _instanceAttributes, glm::mat4 _pvMatrix) : Object(_position, _rotation, _scale)
	{
		init(_atlasRect, _instanceAttributes, _pvMatrix);
	}

	void EnterScreen()
	{
		attributeIndex = PopFreeIndex(); //get a free index
		//add code to above line to catch case where there are no free indices
		SetAttributes(&myInstanceAttributes); //set local copy of attributes to appropriate spot in global attribs
		isVisible = true;
	}

	void LeaveScreen()
	{
		//push my index to free indices
		//unset my index
		isVisible = false;
	}

	glm::mat4 CalculateCombinedMatrix(glm::mat4 pvMat)
	{
		glm::mat4 model = glm::mat4(1.0f); //identity matrix
		model  = glm::translate(model, glm::vec3(position.x, position.y, 0.0f)); //apply translation
		model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, -1.0f)); //apply rotation (about object center)
		model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0f)); //apply scale
		return pvMat * model; //multiply with combined projection view matrix
	}

	glm::mat4 CalculateCombinedMatrix()
	{
		glm::mat4 model = glm::mat4(1.0f); //identity matrix
		model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f)); //apply translation
		model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, -1.0f)); //apply rotation (about object center)
		model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0f)); //apply scale
		return pvMatrix * model; //multiply with stored combined projection view matrix
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
		myInstanceAttributes = *newAttributes;
		GlobalInstanceAttributes[attributeIndex] = *newAttributes;
	}

	InstanceAttributes* GetGlobalAttributes()
	{
		return &(GlobalInstanceAttributes[attributeIndex]);
	}

	void init(glm::fvec4 _atlasRect, InstanceAttributes* _instanceAttributes, glm::mat4 _pvMatrix)
	{
		GlobalInstanceAttributes = _instanceAttributes; //copy pointer to global array of attributes
		myInstanceAttributes.pvmMatrix = CalculateCombinedMatrix(_pvMatrix); //assign attributes to local struct
		myInstanceAttributes.atlasRect = _atlasRect; //..
		pvMatrix = _pvMatrix;
		CheckVisible();
		//TEST CODE
		isVisible = true; //we always want to draw for testing purposes
	}

	bool CheckVisible()
	{
		//add check of isVisible to avoid extra calling of Enter/LeaveScreen
		if (true) //replace true with proper onscreen detection
		{
			if (!isVisible)
				EnterScreen();
			return true;
		}
		if (true) //replace true with offscreen check
		{
			if (isVisible)
				LeaveScreen();
			return false;
		}
	}

	void Move(glm::vec2 _amt)
	{
		position += _amt;
		if (CheckVisible())
		{
			myInstanceAttributes.pvmMatrix = CalculateCombinedMatrix();
			SetAttributes(&myInstanceAttributes);
		}
	}

	InstanceAttributes GetMyAttribs()
	{
		return myInstanceAttributes;
	}
};

class Shader
{
public:
	GLuint program;
	bool isLoaded;

	Shader()
	{
		program = 0;
		isLoaded = false;
	}

	Shader(const char* _vertPath, const char* _fragPath)
	{
		LoadShaderFromFile(_vertPath, _fragPath);
	}

	Shader(std::string _vertPath, std::string _fragPath)
	{
		LoadShaderFromFile(_vertPath.c_str(), _fragPath.c_str());
	}

	bool LoadShaderFromFile(const char* vertPath, const char* fragPath)
	{
		//load source from file
		std::ifstream file;
		std::string currLine;
		file.open(vertPath, std::ios::in); //load file to read as binary at the end
		std::string vertSource; //create string to hold source
		while (std::getline(file, currLine)) //load file
		{
			vertSource.append(currLine); //write in each line
			vertSource.append("\n"); //append the newline that got eaten by getline
		}
		file.close(); //close file
		file.open(fragPath, std::ios::in); //..
		std::string fragSource; //..
		while (std::getline(file, currLine)) //load file
		{
			fragSource.append(currLine); //..
			fragSource.append("\n"); //..
		}
		file.close(); //..

		const char* cstrVertSource = vertSource.c_str(); //store the c string for 
		const char* cstrFragSource = fragSource.c_str();

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); //init empty shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //..
		glShaderSource(vertexShader, 1, &cstrVertSource, NULL); //load shader source
		glCompileShader(vertexShader); //compile shader source
		glShaderSource(fragmentShader, 1, &cstrFragSource, NULL); //..
		glCompileShader(fragmentShader); //..

		GLint compStatus = GL_FALSE; //init
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compStatus); //get GL_LINK_STATUS

		/*Get shader errors
		int logsize;
		char* log = new char[4096];
		glGetShaderInfoLog(fragmentShader, 4096, &logsize, log); //*/
		program = glCreateProgram(); //init empty program
		glAttachShader(program, vertexShader); //attach shaders
		glAttachShader(program, fragmentShader); //..
		glLinkProgram(program); //link program

		GLint linkStatus = GL_FALSE; //init
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus); //get GL_LINK_STATUS
		if (linkStatus == GL_FALSE) //if linking failed
		{
			program = errShader; //use the error shader
		}

		glDeleteShader(vertexShader); //cleanup shaders as they are contained in the program
		glDeleteShader(fragmentShader); //..

		isLoaded = true; //loaded shader

		return !(program == errShader); //return true if succeeded, false if the program is the errShader (failed)
	}

	~Shader()
	{
		if (isLoaded)
			glDeleteProgram(program);
	}
};

glm::vec4 CalculateScreenRect(glm::mat4 projViewMat)
{
	glm::vec3 tl = glm::vec4(-1, -1, 0, 1) * glm::inverse(projViewMat);
	glm::vec3 br = glm::vec4(1, 1, 0, 1) * glm::inverse(projViewMat);
	return glm::vec4(tl.x, br.x, br.y, tl.y);
}

bool RectContainsPoint(glm::vec4 rect, glm::vec3 point)
{
	if (point.x >= rect.x && point.x <= rect.y && point.y <= rect.z && point.y >= rect.w)
		return true;
	return false;
}

bool RectContainsSprite(glm::vec4 rect, DrawableObject* obj)
{
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(obj->position.x, obj->position.y, 0.0f)); //apply translation
	modelMatrix = glm::rotate(modelMatrix, obj->rotation, glm::vec3(0.0f, 0.0f, -1.0f)); //apply rotation (about object center)
	modelMatrix = glm::scale(modelMatrix, glm::vec3(obj->scale.x, obj->scale.y, 1.0f)); //apply scale
	glm::vec3 tl = modelMatrix * glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f);
	glm::vec3 tr = modelMatrix * glm::vec4(0.5f, 0.5f, 0.0f, 1.0f);
	glm::vec3 bl = modelMatrix * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f);
	glm::vec3 br = modelMatrix * glm::vec4(0.5f, -0.5f, 0.0f, 1.0f);

	if (RectContainsPoint(rect, tl))
		return true;
	if (RectContainsPoint(rect, tr))
		return true;
	if (RectContainsPoint(rect, bl))
		return true;
	if (RectContainsPoint(rect, br))
		return true;
	return false;
}