#pragma once
#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <box2d/box2d.h>
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
bool RectContainsPoint(glm::vec4 rect, glm::vec3 point);
bool RectContainsSprite(glm::vec4 rect, DrawableObject* obj);
template <typename T> int sgn(T val); //code from https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c

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
	glm::vec2 scaleFactor;
};

class DrawableObject : public Object
{
private:
	InstanceAttributes myInstanceAttributes; //instance attributes for this instance
	bool isVisible = false; //should this sprite be drawn right now
	glm::mat4* pvMatrix; //proj view matrix
	bool outdatedAttribs = true; //do we need to recalc attribs when drawing

public:

	DrawableObject(glm::vec2 _position, glm::vec4 _atlasRect, glm::mat4* _pvMatrix) : Object(_position)
	{
		init(_atlasRect, _pvMatrix, CalculateScaleFactor());
	}

	DrawableObject(glm::vec2 _position, glm::vec2 _scale, glm::vec4 _atlasRect,
		glm::mat4* _pvMatrix) : Object(_position, _scale)
	{
		init(_atlasRect, _pvMatrix, CalculateScaleFactor());
	}

	DrawableObject(glm::vec2 _position, glm::float32 _rotation, glm::vec2 _scale, glm::vec4 _atlasRect,
		glm::mat4* _pvMatrix) : Object(_position, _rotation, _scale)
	{
		init(_atlasRect, _pvMatrix, CalculateScaleFactor());
	}

	glm::mat4 CalculateCombinedMatrix(glm::mat4 pvMat)
	{
		glm::mat4 model = glm::mat4(1.0f); //identity matrix
		model  = glm::translate(model, glm::vec3(position.x, position.y, 0.0f)); //apply translation
		model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, -1.0f)); //apply rotation (about object center)
		model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0f)); //apply scale
		return pvMat * model; //multiply with combined projection view matrix
	}

	void AttribsOutdated()
	{
		outdatedAttribs = true;
	}

	glm::mat4 CalculateCombinedMatrix()
	{
		glm::mat4 model = glm::mat4(1.0f); //identity matrix
		model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f)); //apply translation
		model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, -1.0f)); //apply rotation (about object center)
		model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0f)); //apply scale
		return *pvMatrix * model; //multiply with stored combined projection view matrix
	}

	glm::vec2 CalculateScaleFactor()
	{
		if (scale.y > scale.x)
			return glm::vec2(1.0f, scale.y / scale.x);
		else
			return glm::vec2(scale.x / scale.y, 1.0f);
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
				myInstanceAttributes.scaleFactor = CalculateScaleFactor();
				outdatedAttribs = false;
			}
			GPUInstancedAttributes[*instanceCount] = myInstanceAttributes; //fill next free space in array with my attribs
			(*instanceCount)++; //increment the drawn instance count
			return true; //return true (we will be drawn)
		}
		return false; //else return false (we won't be drawn)
	}

	void init(glm::fvec4 _atlasRect, glm::mat4* _pvMatrix, glm::vec2 _scaleFactor)
	{
		myInstanceAttributes.pvmMatrix = CalculateCombinedMatrix(*_pvMatrix); //assign attributes to local struct
		myInstanceAttributes.atlasRect = _atlasRect; //..
		myInstanceAttributes.scaleFactor = _scaleFactor;
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

	virtual void Move(glm::vec2 _amt)
	{
		position += _amt;
		outdatedAttribs = true;
	}

	virtual void Scale(glm::vec2 _amt)
	{
		scale *= _amt;
		outdatedAttribs = true;
	}

	virtual void Rotate(glm::float32 _amt)
	{
		rotation = glm::mod(rotation + _amt, 2 * glm::pi<float>());
		outdatedAttribs = true;
	}

	virtual void SetPosition(glm::vec2 _pos)
	{
		position = _pos;
		outdatedAttribs = true;
	}

	virtual void SetScale(glm::vec2 _scale)
	{
		scale = _scale;
		outdatedAttribs = true;
	}

	virtual void SetRotation(glm::float32 _rot)
	{
		rotation = _rot;
		outdatedAttribs = true;
	}

	InstanceAttributes GetMyAttribs()
	{
		return myInstanceAttributes;
	}
};

struct PhysicsUserData
{
	bool isGround;
	bool shouldDamage;
	int damage;
};

class PhysicsObject : public DrawableObject
{
public:
	b2BodyId pBody;
	PhysicsUserData userData;

	PhysicsObject(glm::vec2 _position, glm::float32 _rot, glm::vec2 _scale, glm::vec4 _atlasRect, glm::mat4* _pvMatrix, PhysicsUserData _userData, bool isDynamic)
		:DrawableObject(_position, _scale, _atlasRect, _pvMatrix)
	{
		userData = _userData;
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.position = b2Vec2{(float)position.x, (float)position.y};
		if (isDynamic)
			bodyDef.type = b2BodyType::b2_dynamicBody;
		else
			bodyDef.type = b2BodyType::b2_staticBody;
		bodyDef.userData = &userData;
		pBody = b2CreateBody(pWorld, &bodyDef);
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.friction = 0.5f;
		b2Polygon bPoly = b2MakeBox(scale.x/2.f, scale.y/2.f);
		b2CreatePolygonShape(pBody, &shapeDef, &bPoly);
		b2Body_SetTransform(pBody, b2Vec2{position.x, position.y}, b2MakeRot(rotation));
	}

	PhysicsObject(glm::vec2 _position, glm::float32 _rot, glm::vec2 _scale, glm::vec4 _atlasRect, glm::mat4* _pvMatrix, PhysicsUserData _userData, float platformPct)
		:DrawableObject(_position, _scale, _atlasRect, _pvMatrix)
	{
		userData = _userData;
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.position = b2Vec2{ (float)position.x, (float)position.y };
		bodyDef.type = b2BodyType::b2_staticBody;
		bodyDef.userData = &userData;
		pBody = b2CreateBody(pWorld, &bodyDef);
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.friction = 0.5f;
		b2Polygon bPoly = b2MakeOffsetBox(scale.x / 2.f, (scale.y * platformPct) / 2.f, b2Vec2{ 0.0f, (-scale.y + scale.y * platformPct)/2}, 0);
		b2CreatePolygonShape(pBody, &shapeDef, &bPoly);
		b2Body_SetTransform(pBody, b2Vec2{ position.x, position.y }, b2MakeRot(rotation));
	}


	~PhysicsObject()
	{
		b2DestroyBody(pBody);
	}

	virtual void Move(glm::vec2 _amt)
	{
		DrawableObject::Move(_amt);
		b2Body_SetTransform(pBody, b2Vec2{position.x, position.y}, b2Body_GetRotation(pBody));
	}

	virtual void Scale(glm::vec2 _amt)
	{
		DrawableObject::Scale(_amt);
		b2Polygon newCollider = b2MakeBox(scale.x / 2.0f, scale.y / 2.0f);
		UpdateCollider(&newCollider);
	}

	virtual void Rotate(glm::float32 _amt)
	{
		DrawableObject::Rotate(_amt);
		b2Body_SetTransform(pBody, b2Body_GetPosition(pBody), b2MakeRot(rotation));
	}

	virtual void SetPosition(glm::vec2 _pos)
	{
		DrawableObject::SetPosition(_pos);
		b2Body_SetTransform(pBody, b2Vec2{ position.x, position.y }, b2Body_GetRotation(pBody));
	}

	virtual void SetScale(glm::vec2 _scale)
	{
		DrawableObject::SetScale(_scale);
		b2Polygon newCollider = b2MakeBox(scale.x / 2.0f, scale.y / 2.0f);
		UpdateCollider(&newCollider);
	}

	virtual void SetRotation(glm::float32 _rot)
	{
		DrawableObject::SetRotation(_rot);
		b2Body_SetTransform(pBody, b2Body_GetPosition(pBody), b2MakeRot(rotation));
	}

	void UpdateBody()
	{
		b2Vec2 newPosition = b2Body_GetPosition(pBody);
		b2Rot newRotation = b2Body_GetRotation(pBody);
		SetPosition(glm::vec2(newPosition.x, newPosition.y));
		SetRotation(glm::float32(b2Rot_GetAngle(newRotation)));
	}

	void UpdateCollider(b2Polygon* newCollider)
	{
		b2ShapeId shape;
		b2Body_GetShapes(pBody, &shape, 1);
		b2Shape_SetPolygon(shape, newCollider);
		b2Body_SetMassData(pBody, b2ComputePolygonMass(newCollider, b2Shape_GetDensity(shape)));
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

	Texture(std::string _path, GLenum _textureUnit)
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

			glActiveTexture(_textureUnit); //select the default texture unit (to avoid messing up another texture unit's texture)
			glGenTextures(1, &texture); //gen empty tex
			glBindTexture(GL_TEXTURE_2D, texture); //bind it
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //set wrapping values
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); //..
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

class Player : public PhysicsObject
{
public:
	float health = 100.f;
	float isAlive = true;
	bool isGrounded = false;

	Player(glm::vec2 _pos, glm::vec4 _atlasRect, glm::mat4* _pvMatrix) :
		PhysicsObject(_pos, 0.0f, glm::vec2(0.1f, 0.1f), _atlasRect, _pvMatrix, PhysicsUserData{false}, true)
	{
	}

	void Die()
	{
		isAlive = false;
	}

	void Hit(float _amt)
	{
		health -= _amt;
		if (health <= 0)
			Die();
	}

	void TestContacts()
	{
		b2ContactEvents worldContactEvents = b2World_GetContactEvents(pWorld); //get contact events
		for (int i = 0; i < worldContactEvents.beginCount; i++) //loop over each begin contact event
		{
			b2BodyId A = b2Shape_GetBody(worldContactEvents.beginEvents[i].shapeIdA); //get bodies for each contact
			b2BodyId B = b2Shape_GetBody(worldContactEvents.beginEvents[i].shapeIdB); //..

			if (B.index1 == pBody.index1) //if body B is the player
			{ //swap bodies a and b for ease of coding
				b2BodyId temp = B;
				B = A;
				A = temp;
			}
			if (A.index1 == pBody.index1) //if body A is the player
			{
				void* contactUserData = b2Body_GetUserData(B); //get the user data
				if (contactUserData != nullptr) //if the user data is not null
				{
					PhysicsUserData* contactPhysicsUserData = (PhysicsUserData*)contactUserData; //it must be PhysicsUserData so we can just C cast it
					if (contactPhysicsUserData->shouldDamage == true) //if it should damage the player
					{
						Hit(contactPhysicsUserData->damage); //damage by amount
					}
					if (contactPhysicsUserData->isGround) //if it's ground
					{
						isGrounded = true; //we can jump
					}
				}
			}
		}
	}

	void Jump(float _jumpSpeed)
	{
		if (isGrounded)
		{
			b2Vec2 vel = b2Body_GetLinearVelocity(pBody);
			float mass = b2Body_GetMass(pBody);
			b2Body_ApplyLinearImpulseToCenter(pBody, b2Vec2{ mass * vel.x * 0.3f, mass * _jumpSpeed }, true);
			isGrounded = false;
		}
	}
};

class Enemy : public PhysicsObject
{
public:
	Player* player;

	Enemy(glm::vec2 _pos, glm::vec2 _scale, glm::vec4 _atlasRect, glm::mat4* _pvMatrix, Player* _player, int _damage) :
		PhysicsObject(_pos, 0.0f, _scale, _atlasRect, _pvMatrix, PhysicsUserData{ false, true, _damage}, true)
	{
		player = _player;
	}

	virtual void UpdateEnemy()
	{
	}
};

class PatrolEnemy : public Enemy
{
public:
	float* waypoints;
	unsigned int waypointCount;
	unsigned int currentWaypoint = 0;
	const float patrolSpeed = 0.45f;

	PatrolEnemy(glm::vec2 _pos, glm::vec2 _scale, glm::vec4 _atlasRect, glm::mat4* _pvMatrix, Player* _player, int _damage,
		float* _waypoints, unsigned int _waypointCount) :
		Enemy(_pos, _scale, _atlasRect, _pvMatrix, _player, _damage)
	{
		waypointCount = _waypointCount;
		waypoints = new float[waypointCount]; //populate waypoints with new empty array
		for (unsigned int i = 0; i < waypointCount; i++) //iterate over each waypoint
		{
			waypoints[i] = _waypoints[i]; //copy in waypoints
		} //this means _waypoints doesn't need to live longer than the constructor
	}

	void UpdateEnemy()
	{
		if (waypointCount > 0)
		{
			float dir = waypoints[currentWaypoint] - position.x; //get x distance to waypoint
			if (glm::abs(dir) > scale.x) //if further than scale
			{
				dir = sgn(dir); //turn into -1 0 +1 depending on sign
				b2Body_SetLinearVelocity(pBody, b2Vec2{ dir * patrolSpeed, b2Body_GetLinearVelocity(pBody).y }); //move towards waypoint at speed patrolSpeed
			}
			else //if closer than scale
			{
				currentWaypoint = (currentWaypoint + 1) % waypointCount; //increment waypoint number
			}
		}
	}
};

class Platform : public PhysicsObject
{
public:
	Platform(glm::vec2 _pos, glm::float32 _rot, glm::vec2 _scale, glm::vec4 _atlasRect, glm::mat4* _pvMatrix) :
		PhysicsObject(_pos, _rot, _scale, _atlasRect, _pvMatrix, PhysicsUserData{ true, false, 0 }, 47.f / 127.f) {}
};

class Camera : public Object
{
private:
	bool outdatedView = true;
public:
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 projView;
	float depth;

	Camera(glm::vec3 _pos, float _rot, int width, int height) : Object(glm::vec2(_pos.x, _pos.y), _rot)
	{
		depth = _pos.z;
		view = GetView();
		float aspect = (float)width / (float)height; //calculate aspect ratio
		proj = glm::ortho(-aspect, aspect, -1.0f, 1.0f, 0.25f, 5.0f); //calculate orthographic projection matrix
	}

	glm::mat4 GetView()
	{
		if (outdatedView)
		{
			glm::vec3 forward = glm::vec3(0.0, 0.0, -1.0);
			glm::vec4 up4 = glm::vec4(0.0, 1.0, 0.0, 0.0);
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0), rotation, glm::vec3(0.0f, 0.0f, -1.0f));
			up4 = rotationMatrix * up4;
			glm::vec3 up3 = glm::vec3(up4.x, up4.y, up4.z);
			view = glm::lookAt(glm::vec3(position.x, position.y, depth), glm::vec3(position.x, position.y, depth) + forward, up3);
		}
		return view;
	}

	glm::vec4 CalculateScreenRect()
	{
		glm::mat4 projViewMat = proj * GetView();
		glm::vec3 tl = glm::vec4(-1, -1, 0, 1) * glm::inverse(projViewMat);
		glm::vec3 br = glm::vec4(1, 1, 0, 1) * glm::inverse(projViewMat);
		return glm::vec4(tl.x, br.x, br.y, tl.y);
	}

	glm::mat4* GetProjView()
	{
		projView = proj * GetView();
		return &projView;
	}

	bool Follow(glm::vec2 target, unsigned int deltaTime)
	{
		const float lerpFac = 0.5f;
		glm::vec2 amt = (target - position)^2 * lerpFac * (deltaTime / 1000.f);
		if (amt.length() > 0)
		{
			Move(amt);
			return true;
		}
		else
			return false;
	}

	void Move(glm::vec2 _amt)
	{
		Object::Move(_amt);
		outdatedView = true;
	}

	void Scale(glm::vec2 _amt)
	{
		Object::Scale(_amt);
		outdatedView = true;
	}

	void Rotate(glm::float32 _amt)
	{
		Object::Rotate(_amt);
		outdatedView = true;
	}

	void SetPosition(glm::vec2 _pos)
	{
		Object::SetPosition(_pos);
		outdatedView = true;
	}

	void SetScale(glm::vec2 _scale)
	{
		Object::SetScale(_scale);
		outdatedView = true;
	}

	void SetRotation(glm::float32 _rot)
	{
		Object::SetRotation(_rot);
		outdatedView = true;
	}
};

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

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
} //code from https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c