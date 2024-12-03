#pragma once
#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stack>
#include <fstream>
#include <iostream>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8 //change this have ifdef WINDOWS or something similar for crossplatform compilation
#include <stb/stb_image.h>

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
glm::vec4 CalculateScreenRect(glm::mat4 projViewMat);
bool RectContainsPoint(glm::vec4 rect, glm::vec3 point);
bool RectContainsSprite(glm::vec4 rect, DrawableObject* obj);

GLuint errShader = 0;
glm::vec4 screenRect; //screen rectangle in world space (l, r, t, b)
b2WorldId pWorld;

class Object
{
protected:
	glm::vec2 position;
	glm::float32 rotation;
	glm::vec2 scale;

public:
	Object(glm::vec2 _position, glm::float32 _rotation, glm::vec2 _scale)
	{
		position = _position;
		rotation = _rotation;
		scale = _scale;
	}
	Object(glm::vec2 _position, glm::float32 _rotation)
	{
		position = _position;
		rotation = _rotation;
		scale = glm::vec2(1.0f, 1.0f);
	}
	Object(glm::vec2 _position)
	{
		position = _position;
		rotation = 0;
		scale = glm::vec2(1.0f, 1.0f);
	}
	Object(glm::vec2 _position, glm::vec2 _scale)
	{
		position = _position;
		rotation = 0;
		scale = _scale;
	}

	Object()
	{
		position = glm::vec2(0.0f, 0.0f);
		rotation = 0;
		scale = glm::vec2(1.0f, 1.0f);
	}

	virtual void Move(glm::vec2 _amt)
	{
		position += _amt;
	}

	virtual void Scale(glm::vec2 _amt)
	{
		scale *= _amt;
	}

	virtual void Rotate(glm::float32 _amt)
	{
		rotation = glm::mod(rotation + _amt, 2 * glm::pi<float>());
	}

	virtual void SetPosition(glm::vec2 _pos)
	{
		position = _pos;
	}

	virtual void SetScale(glm::vec2 _scale)
	{
		scale = _scale;
	}

	virtual void SetRotation(glm::float32 _rot)
	{
		rotation = _rot;
	}

	virtual glm::vec2 GetPosition()
	{
		return position;
	}

	virtual glm::vec2 GetScale()
	{
		return scale;
	}

	virtual glm::float32 GetRotation()
	{
		return rotation;
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
	bool isVisible = false; //should this sprite be drawn right now
	glm::mat4 pvMatrix; //proj view matrix
	bool outdatedAttribs = true; //do we need to recalc attribs when drawing

public:

	DrawableObject(glm::vec2 _position, glm::vec4 _atlasRect, glm::mat4 _pvMatrix) : Object(_position)
	{
		init(_atlasRect, _pvMatrix);
	}

	DrawableObject(glm::vec2 _position, glm::vec2 _scale, glm::vec4 _atlasRect,
		glm::mat4 _pvMatrix) : Object(_position, _scale)
	{
		init(_atlasRect, _pvMatrix);
	}

	DrawableObject(glm::vec2 _position, glm::float32 _rotation, glm::vec2 _scale, glm::vec4 _atlasRect,
		glm::mat4 _pvMatrix) : Object(_position, _rotation, _scale)
	{
		init(_atlasRect, _pvMatrix);
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
		if (*instanceCount + 1 > MAX_SPRITES) //if MAX_SPRITES are already ready to be drawn
			return false; //exit early
		if (isVisible) //if we should be drawn right now
		{
			if (outdatedAttribs)
			{
				myInstanceAttributes.pvmMatrix = CalculateCombinedMatrix();
				outdatedAttribs = false;
			}
			GPUInstancedAttributes[*instanceCount] = myInstanceAttributes; //fill next free space in array with my attribs
			(*instanceCount)++; //increment the drawn instance count
			return true; //return true (we will be drawn)
		}
		return false; //else return false (we won't be drawn)
	}

	void init(glm::fvec4 _atlasRect, glm::mat4 _pvMatrix)
	{
		myInstanceAttributes.pvmMatrix = CalculateCombinedMatrix(_pvMatrix); //assign attributes to local struct
		myInstanceAttributes.atlasRect = _atlasRect; //..
		pvMatrix = _pvMatrix;
		CheckVisible();
	}

	bool CheckVisible()
	{
		if (RectContainsSprite(screenRect, this)) //if onscreen
		{
			if (!isVisible)
				isVisible = true;
			return true;
		}
		else //offscreen
		{
			if (isVisible)
				isVisible = false;
			return false;
		}
	}

	void Move(glm::vec2 _amt)
	{
		position += _amt;
		outdatedAttribs = true;
	}

	void Scale(glm::vec2 _amt)
	{
		scale *= _amt;
		outdatedAttribs = true;
	}

	void Rotate(glm::float32 _amt)
	{
		rotation = glm::mod(rotation + _amt, 2 * glm::pi<float>());
		outdatedAttribs = true;
	}

	void SetPosition(glm::vec2 _pos)
	{
		position = _pos;
		outdatedAttribs = true;
	}

	void SetScale(glm::vec2 _scale)
	{
		scale = _scale;
		outdatedAttribs = true;
	}

	void SetRotation(glm::float32 _rot)
	{
		rotation = _rot;
		outdatedAttribs = true;
	}

	InstanceAttributes GetMyAttribs()
	{
		return myInstanceAttributes;
	}
};

class PhysicsObject : public DrawableObject
{
public:
	b2BodyId pBody;
	
	PhysicsObject(glm::vec2 _position, glm::vec2 _scale, glm::vec4 _atlasRect, glm::mat4 _pvMatrix)
		:DrawableObject(_position, _scale, _atlasRect, _pvMatrix)
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.position = b2Vec2{(float)position.x, (float)position.y};
		bodyDef.type = b2_dynamicBody;
		pBody = b2CreateBody(pWorld, &bodyDef);
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.friction = 0.5f;
		b2Polygon bPoly = b2MakeBox(scale.x/2.f, scale.y/2.f);
		b2CreatePolygonShape(pBody, &shapeDef, &bPoly);
	}

	~PhysicsObject()
	{
		b2DestroyBody(pBody);
	}

	void UpdateBody()
	{
		b2Vec2 newPosition = b2Body_GetPosition(pBody);
		b2Rot newRotation = b2Body_GetRotation(pBody);
		SetPosition(glm::vec2(newPosition.x, newPosition.y));
		SetRotation(glm::float32(b2Rot_GetAngle(newRotation)));
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

class Texture
{
public:
	bool isLoaded = false;
	int width = 0;
	int height = 0;
	int channels = 0;
	GLenum glType = GL_NONE;
	GLuint texture = 0;

	Texture()
	{
	}

	Texture(std::string _path)
	{
		unsigned char* data = stbi_load(_path.c_str(), &width, &height, &channels, 0); //load image from disk
		if (data != NULL) //if image was loaded properly
		{
			switch (channels) //set image type
			{
			case 1:
				glType = GL_RED;
				break;
			case 2:
				glType = GL_RG;
				break;
			case 3:
				glType = GL_RGB;
				break;
			case 4:
				glType = GL_RGBA;
				break;
			default: //incompatible number of channels
				stbi_image_free(data); //release image data
				return; //exit constructor
			}

			glActiveTexture(GL_TEXTURE0); //select the default texture unit (to avoid messing up another texture unit's texture)
			glGenTextures(1, &texture); //gen empty tex
			glBindTexture(GL_TEXTURE_2D, texture); //bind it
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //set wrapping values
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //..
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR); //set filter values
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //..
			glTexImage2D(GL_TEXTURE_2D, 0, glType, width, height, 0, glType, GL_UNSIGNED_BYTE, data); //populate texture
			glGenerateMipmap(GL_TEXTURE_2D); //generate mipmap
			isLoaded = true; //finished loading
		}
		stbi_image_free(data); //free the image data as it's now unused
	}

	~Texture()
	{
		glDeleteTextures(1, &texture);
	}
};

class Key
{
private:
	bool flop = false;

public:
	SDL_KeyCode keyCode;

	Key(SDL_KeyCode _keyCode)
	{
		keyCode = _keyCode;
	}

	bool Press() //returns true on first call
	{
		if (!flop)
		{
			flop = true;
			return true;
		}
		else return false;
	}

	bool Release() //returns true on first call
	{
		if (flop)
		{
			flop = false;
			return true;
		}
		else return false;
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
	return true;
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(obj->GetPosition().x, obj->GetPosition().y, 0.0f)); //apply translation
	modelMatrix = glm::rotate(modelMatrix, obj->GetRotation(), glm::vec3(0.0f, 0.0f, -1.0f)); //apply rotation (about object center)
	modelMatrix = glm::scale(modelMatrix, glm::vec3(obj->GetScale().x, obj->GetScale().y, 1.0f)); //apply scale
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