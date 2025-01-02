#define SDL_MAIN_NEEDED //redefine main to initalize some C and C++ things (needed because we don't use WinMain)
#include <SDL/SDL.h>
#include <glad/glad.h>
#include <SDL/SDL_opengl.h>
#include "types.h"
#include <vector>
#include <algorithm>

const char* vert_default = "#version 330 core\n"
"struct InstanceAttributes\n"
"{\n"
"mat4 pvmMatrix;\n"
"vec4 atlasRect;\n"
"};\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 2) in InstanceAttributes attribs;\n"
"void main()\n"
"{\n"
"gl_Position = attribs.pvmMatrix * vec4(position, 1.0);\n"
"}\0";

const char* frag_default = "#version 330 core\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = vec4(1.0f, 0.0f, 1.0f, 1.0f);\n"
"}\0";

void quit(int code); //quit function prototype
int init();
static void handleEvents();
void handleKeysDown(SDL_KeyboardEvent* key);
void handleKeysUp(SDL_KeyboardEvent* key);
void loop();
void draw();
void LoadLevel1();
void LoadLevel2();
#define Path(assetPath) std::string(SDL_GetBasePath() + std::string(##assetPath##))

#pragma region KeyDefinitions
Key LeftKey = Key(SDLK_a);
Key RightKey = Key(SDLK_d);
Key JumpKey = Key(SDLK_SPACE);
#pragma endregion

SDL_Window* window; //main window
static SDL_Renderer* renderer; //main renderer
SDL_GLContext glContext;
bool running = true;
InstanceAttributes GPUInstanceAttributes[MAX_SPRITES];
unsigned int onscreenSprites = 0; //number of sprites to draw with glDrawElementsInstanced
unsigned int onscreenPlatforms = 0;
GLuint quadVBO = 0; //quad vertex attribute buffer object
GLuint quadVIO = 0; //quad vertex indices buffer object
GLuint quadVAO = 0; //quad vertex array object
GLuint instanceAttributeBuffer = 0; //instance attribute buffer object
std::vector<DrawableObject*> sprites;
std::vector<Platform*> platforms;
std::vector<Enemy*> enemies;
float newMoveSpeed = 0.0f;

Shader* basicShader;
Shader* platformShader;
Texture* atlas;
Texture* platformAtlas;
Platform* floorPlatform;
Camera* mainCamera;

bool canJump = false;

int main(int argv, char** args)
{
	init();
	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Working", "It's Working!", NULL); //post a basic messagebox

	elapsedTime = SDL_GetTicks64();
	//LoadLevel1();
	LoadLevel2();
	while (running)
	{
		loop();
		draw();
	}

	return 0;
}

void quit(int code)
{
	SDL_Quit(); //quit SDL
	b2DestroyWorld(pWorld); //destroy the physics world
	exit(code); //exit the application with the error code
}

int init()
{
	//Initialization:
	//SDL:
	//SDL with OpenGL:
	SDL_Init(SDL_INIT_EVERYTHING); //initalize all SDL subsystems
	elapsedTime = SDL_GetTicks64(); //get inital elapsed time
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //..
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); //use OpenGL 4.5 core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5); //..
	window = SDL_CreateWindow("Platformer", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL); //create new SDL OGL window //SDL_WINDOW_FULLSCREEN_DESKTOP |
	if (!window) //if failed to create window
		quit(-1); //quit with error code -1
	glContext = SDL_GL_CreateContext(window); //Create GL context
	if (!glContext) //if failed to create context
		quit(-1); //quit
	gladLoadGLLoader(SDL_GL_GetProcAddress); //load GL function pointers through SDL
	//OpenGL
	glClearColor(0.53f, 0.81f, 0.92f, 1.0); //set sky blue clear color RGBA
	int width, height; //window width and height
	SDL_GetWindowSize(window, &width, &height); //get current window size
	glViewport(0, 0, width, height); //create viewport
	glClear(GL_COLOR_BUFFER_BIT); //clear back buffer
	SDL_GL_SwapWindow(window); //swap buffers
	glClear(GL_COLOR_BUFFER_BIT); //clear back buffer
	glDisable(GL_CULL_FACE); //disable face culling for now
	glEnable(GL_BLEND); //enable alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//create default shader
	GLuint defaultVert = 0;
	GLuint defaultFrag = 0;
	defaultVert = glCreateShader(GL_VERTEX_SHADER); //initalize shader
	defaultFrag = glCreateShader(GL_FRAGMENT_SHADER); //..
	glShaderSource(defaultVert, 1, &vert_default, NULL); //load shader source
	glCompileShader(defaultVert); //compile shader source
	glShaderSource(defaultFrag, 1, &frag_default, NULL); //..
	glCompileShader(defaultFrag); //..

	errShader = glCreateProgram(); //initalize program
	glAttachShader(errShader, defaultVert); //attach vert shader
	glAttachShader(errShader, defaultFrag); //.. frag ..
	glLinkProgram(errShader); //link shaders into one program

	basicShader = new Shader(Path("assets/shaders/basic.vert"), Path("assets/shaders/basic.frag"));
	platformShader = new Shader(Path("assets/shaders/platform.vert"), Path("assets/shaders/platform.frag"));

	glGenBuffers(1, &quadVBO); //generate empty buffer
	glGenBuffers(1, &quadVIO); //..
	glGenVertexArrays(1, &quadVAO); //.. vertex array object
	glBindVertexArray(quadVAO); //bind Vertex Array Object
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO); //bind buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW); //populate buffer
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); //set attribute read params
	glEnableVertexAttribArray(0); //enable attrib
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); //..
	glEnableVertexAttribArray(1); //..
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadVIO); //bind index buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW); //populate
	glGenBuffers(1, &instanceAttributeBuffer); //generate instance attribute buffer on GPU
	glBindBuffer(GL_ARRAY_BUFFER, instanceAttributeBuffer);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 22 * sizeof(GL_FLOAT), (void*)0); //assign attribute struct (layout location + element index), element size, element type (mat4 is 4 rows of 4 floats), stride (size of struct in bytes), start offset (bytes)
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 22 * sizeof(GL_FLOAT), (void*)(4 * sizeof(GL_FLOAT))); //..
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 22 * sizeof(GL_FLOAT), (void*)(8 * sizeof(GL_FLOAT))); //..
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 22 * sizeof(GL_FLOAT), (void*)(12 * sizeof(GL_FLOAT))); //..
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 22 * sizeof(GL_FLOAT), (void*)(16 * sizeof(GL_FLOAT))); //..
	glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, 22 * sizeof(GL_FLOAT), (void*)(20 * sizeof(GL_FLOAT))); //..
	glVertexAttribDivisor(2, 1); //tell GPU to update atribute on for each instance when drawing with glDraw..Instanced()
	glVertexAttribDivisor(3, 1); //..
	glVertexAttribDivisor(4, 1); //..
	glVertexAttribDivisor(5, 1); //..
	glVertexAttribDivisor(6, 1); //..
	glVertexAttribDivisor(7, 1); //..
	glEnableVertexAttribArray(2); //"enable" the attribute
	glEnableVertexAttribArray(3); //I think this just lets opengl read from it at draw time
	glEnableVertexAttribArray(4); //not 100% sure
	glEnableVertexAttribArray(5); //..
	glEnableVertexAttribArray(6); //..
	glEnableVertexAttribArray(7); //..

	glUseProgram(platformShader->program);
	glUniform1i(glGetUniformLocation(platformShader->program, "tex"), 1);
	const glm::vec2 platformAtlasSize = glm::vec2(128.f, 256.f);
	glUniform2f(glGetUniformLocation(platformShader->program, "atlasSize"), platformAtlasSize.x, platformAtlasSize.y);

	mainCamera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f), 0.f, width, height);

	screenRect = mainCamera->CalculateScreenRect();

	//Box2d
	b2WorldDef worldDef = b2DefaultWorldDef(); //create a world definition for box2d
	pWorld = b2CreateWorld(&worldDef); //create a box2d world from that definition
	
	player = new Player(glm::vec2(-0.50f, 0.0f), glm::vec4(0.0f, 127.0f, 0.0f, 127.0f), mainCamera->GetProjView());
	float enemyWaypoints[2] = { 0.0f, 1.0f };

	sprites.push_back(player);

	//floorPlatform = new Platform(glm::vec2(0.0f, -0.5f), 0.0f, glm::vec2(3.0f, 0.5f), glm::vec4(0.f, 128.f, 0.f, 128.f), mainCamera->GetProjView());
	//platforms.push_back(floorPlatform);

	atlas = new Texture(Path("assets/sprites/Atlas.png"), GL_TEXTURE0);
	platformAtlas = new Texture(Path("assets/sprites/PlatformAtlas.png"), GL_TEXTURE1);

	glUseProgram(basicShader->program);
	glUniform1i(glGetUniformLocation(basicShader->program, "tex"), 0);
	glUniform2f(glGetUniformLocation(basicShader->program, "atlasSize"), atlas->width, atlas->height);

	return 0;
}

static void handleEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			quit(1);
			break;
		case SDL_KEYDOWN:
			handleKeysDown(&event.key);
			break;
		case SDL_KEYUP:
			handleKeysUp(&event.key);
			break;
		}
	}
}

static void handleKeysDown(SDL_KeyboardEvent* _key)
{
	SDL_Keycode key = _key->keysym.sym;

	if (key == LeftKey.keyCode)
	{
		if (LeftKey.Press())
		{
			newMoveSpeed -= moveSpeed;
		}
		return;
	}
	if (key == RightKey.keyCode)
	{
		if (RightKey.Press())
		{
			newMoveSpeed += moveSpeed;
		}
		return;
	}
	if (key == JumpKey.keyCode)
	{
		if (JumpKey.Press())
		{
			player->Jump(jumpSpeed);
		}
		return;
	}
}

static void handleKeysUp(SDL_KeyboardEvent* _key)
{
	SDL_Keycode key = _key->keysym.sym;
	if (key == SDLK_ESCAPE)
	{
		quit(1);
		return;
	}
	if (key == LeftKey.keyCode)
	{
		if (LeftKey.Release())
		{
			newMoveSpeed += moveSpeed;
		}
		return;
	}
	if (key == RightKey.keyCode)
	{
		if (RightKey.Release())
		{
			newMoveSpeed -= moveSpeed;
		}
		return;
	}
	if (key == JumpKey.keyCode)
	{
		JumpKey.Release();
		return;
	}
}

void loop()
{
	unsigned long long int oldElapsedTime = elapsedTime;
	elapsedTime = SDL_GetTicks64(); //get elapsed time in ms
	deltaTime = elapsedTime - oldElapsedTime;
	handleEvents();

	if (newMoveSpeed < 0) //if moving left
	{
		b2Body_SetLinearVelocity(player->pBody, b2Vec2{ glm::min(b2Body_GetLinearVelocity(player->pBody).x, newMoveSpeed),
				b2Body_GetLinearVelocity(player->pBody).y }); //apply velocity
	}
	else
	{
		if (newMoveSpeed > 0) //if moving right
		{
			b2Body_SetLinearVelocity(player->pBody, b2Vec2{ glm::max(b2Body_GetLinearVelocity(player->pBody).x, newMoveSpeed),
				b2Body_GetLinearVelocity(player->pBody).y }); //apply velocity
		}
	}
	b2World_Step(pWorld, deltaTime / 1000.f, 4); //step the physics world
	std::for_each(enemies.begin(), enemies.end(), [&](Enemy* enemy) {enemy->UpdateEnemy(); });
	player->UpdateBody();

	player->TestContacts();

	if (mainCamera->Follow(glm::vec2(player->GetPosition().x, player->GetPosition().y + 0.66f), deltaTime)) //if camera following moved the camera
	{
		mainCamera->GetProjView(); //update mainCamera.projView
		std::for_each(sprites.begin(), sprites.end(), [&](DrawableObject* sprite) {sprite->AttribsOutdated(); }); //loop over each sprite and tell it to
		std::for_each(platforms.begin(), platforms.end(), [&](DrawableObject* platform) {platform->AttribsOutdated(); });//update its instance attribs
	}
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(quadVAO); //bind plane VAO

	onscreenSprites = 0;
	std::for_each(sprites.begin(), sprites.end(), [&](DrawableObject* sprite) {sprite->DrawInstanced(GPUInstanceAttributes, &onscreenSprites); });
	glBindBuffer(GL_ARRAY_BUFFER, instanceAttributeBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GPUInstanceAttributes), GPUInstanceAttributes, GL_DYNAMIC_DRAW); //populate instance attrib buffer with tightly packed array

	if (basicShader->isLoaded)
		glUseProgram(basicShader->program); //use basic shader
	else
		glUseProgram(errShader);

	glDrawElementsInstanced(GL_TRIANGLES, sizeof(planeIndices), GL_UNSIGNED_INT, 0, onscreenSprites); //draw indexed verts instance

	onscreenPlatforms = 0;
	std::for_each(platforms.begin(), platforms.end(), [&](DrawableObject* platform) {platform->DrawInstanced(GPUInstanceAttributes, &onscreenPlatforms); });
	glBindBuffer(GL_ARRAY_BUFFER, instanceAttributeBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GPUInstanceAttributes), GPUInstanceAttributes, GL_DYNAMIC_DRAW);

	if (platformShader->isLoaded)
		glUseProgram(platformShader->program);
	else
		glUseProgram(errShader);

	glDrawElementsInstanced(GL_TRIANGLES, sizeof(planeIndices), GL_UNSIGNED_INT, 0, onscreenPlatforms);

	SDL_GL_SwapWindow(window); //swap buffers
}

void LoadLevel1()
{
	//delete old enemies
	//set player position
	mainCamera->SetPosition(glm::vec2(0.f, 0.f));
	platforms.clear();
	Platform* platform = new Platform(glm::vec2(-0.35f, -0.66f), 0, glm::vec2(1.5f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(1.25f, -0.66f), 0, glm::vec2(0.80f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(2.40f, -0.66f), 0, glm::vec2(0.60f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(4.15f, -0.66f), 0, glm::vec2(2.00f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	float* waypoints = new float[2] { 1.25f, 1.60f };
	Enemy* enemy = new PatrolEnemy(glm::vec2(1.60f, -0.30f), glm::vec2(0.10f), glm::vec4(0.f, 127.f, 0.f, 127.f), mainCamera->GetProjView(), player, 10, waypoints, 2, nullptr);
	sprites.push_back(enemy);
	enemies.push_back(enemy);
	delete[](waypoints);
	waypoints = new float[2] { 3.20f, 3.65f };
	enemy = new PatrolEnemy(glm::vec2(3.40f, -0.30f), glm::vec2(0.10f), glm::vec4(0.f, 127.f, 0.f, 127.f), mainCamera->GetProjView(), player, 10, waypoints, 2, nullptr);
	sprites.push_back(enemy);
	enemies.push_back(enemy);
	delete[](waypoints);
	waypoints = new float[2] { 3.70f, 4.15f };
	enemy = new PatrolEnemy(glm::vec2(3.90f, -0.30f), glm::vec2(0.10f), glm::vec4(0.f, 127.f, 0.f, 127.f), mainCamera->GetProjView(), player, 10, waypoints, 2, nullptr);
	sprites.push_back(enemy);
	enemies.push_back(enemy);
	delete[](waypoints);
	waypoints = new float[1] { 4.20f };
	enemy = new PatrolEnemy(glm::vec2(5.00f, -0.20f), glm::vec2(0.166f), glm::vec4(0.f, 127.f, 0.f, 127.f), mainCamera->GetProjView(), player, 15, waypoints, 1, ActivateWhenNearPlayer);
	enemy->DeActivate();
	enemy->health = 20.00f;
	sprites.push_back(enemy);
	enemies.push_back(enemy);
	delete[](waypoints);
}

void UnloadLevel()
{
	//delete enemies and remove sprites
	//delete platforms and empty platforms[]
	//reset HP
}

void LoadLevel2()
{
	mainCamera->SetPosition(glm::vec2(0.00f));
	platforms.clear();
	Platform* platform = new Platform(glm::vec2(-0.05f, -0.66f), 0, glm::vec2(1.00f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(1.35f, -0.66f), 0, glm::vec2(1.00f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(1.75f, -0.20f), 0, glm::vec2(0.50f, 0.125f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(3.50f, -0.66f), 0, glm::vec2(2.33f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(5.30f, -0.66f), 0, glm::vec2(0.67f, 0.33f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	platform = new Platform(glm::vec2(3.225f, -0.20f), 0, glm::vec2(1.75f, 0.125f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView());
	platforms.push_back(platform);
	Enemy* enemy = new Enemy(glm::vec2(1.25f, -0.66f), glm::vec2(0.10f), glm::vec4(0.0f, 127.f, 0.0f, 127.f), mainCamera->GetProjView(), player, 10, nullptr);
	sprites.push_back(enemy);
	enemies.push_back(enemy);
	Saw* saw = new Saw(glm::vec2(1.50f, -0.66f), glm::vec2(0.15f), glm::vec4(128.f, 256.f, 0.f, 127.f), mainCamera->GetProjView(), player);
	sprites.push_back(saw);
	enemies.push_back(saw);
}