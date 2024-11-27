#define SDL_MAIN_NEEDED //redefine main to initalize some C and C++ things (needed because we don't use WinMain)
#include <SDL/SDL.h>
#include <glad/glad.h>
#include <SDL/SDL_opengl.h>
#include <box2d/box2d.h>
#include "types.h"

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
void handleKeys(SDL_KeyboardEvent* key);
void loop();
void draw();
//std::string Path(std::string assetPath);
#define Path(assetPath)  std::string(SDL_GetBasePath() + std::string(##assetPath##))

SDL_Window* window; //main window
static SDL_Renderer* renderer; //main renderer
SDL_GLContext glContext;
bool running = true;
std::stack<unsigned int> FreeInstanceAttributeIndices; //only one copy allowed, main thread access only
InstanceAttributes GlobalInstanceAttributes[MAX_SPRITES];
InstanceAttributes GPUInstanceAttributes[MAX_SPRITES];
unsigned int onscreenSprites = 0; //number of sprites to draw with glDrawElementsInstanced
glm::mat4 projViewMat; //combined projection view matrix
glm::vec4 screenRect; //screen rectangle in world space (l, r, t, b)

DrawableObject* skibidi;
DrawableObject* toilet;
Shader* basicShader;
SDL_Surface* upArrow;

int main(int argv, char** args)
{
	init();
	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Working", "It's Working!", NULL); //post a basic messagebox
	GLuint VBO = 0;
	GLuint VIO = 0;
	GLuint VAO = 0;
	GLuint IBO = 0;
	glGenBuffers(1, &VBO); //generate empty buffer
	glGenBuffers(1, &VIO); //..
	glGenVertexArrays(1, &VAO); //.. vertex array object
	glBindVertexArray(VAO); //bind Vertex Array Object
	glBindBuffer(GL_ARRAY_BUFFER, VBO); //bind buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW); //populate buffer
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); //set attribute read params
	glEnableVertexAttribArray(0); //enable attrib
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); //..
	glEnableVertexAttribArray(1); //..
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VIO); //bind index buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW); //populate
	glGenBuffers(1, &IBO); //generate instance attribute buffer on GPU
	glBindBuffer(GL_ARRAY_BUFFER, IBO);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 20 * sizeof(GL_FLOAT), (void*)0); //assign attribute struct (layout location + element index), element size, element type (mat4 is 4 rows of 4 floats), stride (size of struct in bytes), start offset (bytes)
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 20 * sizeof(GL_FLOAT), (void*)(4 * sizeof(GL_FLOAT))); //..
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 20 * sizeof(GL_FLOAT), (void*)(8 * sizeof(GL_FLOAT))); //..
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 20 * sizeof(GL_FLOAT), (void*)(12 * sizeof(GL_FLOAT))); //..
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 20 * sizeof(GL_FLOAT), (void*)(16 * sizeof(GL_FLOAT))); //..
	glVertexAttribDivisor(2, 1); //tell GPU to update atribute on for each instance when drawing with glDraw..Instanced()
	glVertexAttribDivisor(3, 1); //..
	glVertexAttribDivisor(4, 1); //..
	glVertexAttribDivisor(5, 1); //..
	glVertexAttribDivisor(6, 1); //..
	glEnableVertexAttribArray(2); //"enable" the attribute
	glEnableVertexAttribArray(3); //I think this just lets opengl read from it at draw time
	glEnableVertexAttribArray(4); //not 100% sure
	glEnableVertexAttribArray(5); //..
	glEnableVertexAttribArray(6); //..

	GLuint arrowTex;
	glUseProgram(basicShader->program);
	glGenTextures(1, &arrowTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, arrowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, upArrow->w, upArrow->h, 0, GL_RGB, GL_UNSIGNED_BYTE, (upArrow->pixels));
	glGenerateMipmap(GL_TEXTURE_2D);
	glUniform1i(glGetUniformLocation(basicShader->program, "tex"),0);
	const glm::vec2 atlasSize = glm::vec2(256.f, 128.f);
	glUniform2f(glGetUniformLocation(basicShader->program, "atlasSize"), atlasSize.x, atlasSize.y);

	while (running)
	{
		loop();

		skibidi->Move(glm::vec2(0.0003f, 0));
		//toilet->Move(glm::vec2(glm::sin(SDL_GetTicks()), 0));
		//temp render code
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(VAO); //bind plane VAO
		
		onscreenSprites = 0;
		skibidi->DrawInstanced(GPUInstanceAttributes, &onscreenSprites); //populate tightly packed array
		toilet->DrawInstanced(GPUInstanceAttributes, &onscreenSprites); //replace with some loop over all instanced sprites in this level
		
		glBindBuffer(GL_ARRAY_BUFFER, IBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GPUInstanceAttributes), GPUInstanceAttributes, GL_DYNAMIC_DRAW); //populate instance attrib buffer with tightly packed array

		if (basicShader->isLoaded)
		{
			glUseProgram(basicShader->program); //use basic shader
			glDrawElementsInstanced(GL_TRIANGLES, sizeof(planeIndices), GL_UNSIGNED_INT, 0, onscreenSprites); //draw indexed verts instance
		}
		draw();
	}

	return 0;
}

void quit(int code)
{
	SDL_Quit(); //quit SDL
	exit(code); //exit the application with the error code
}

int init()
{
	//Initialization:
	//SDL:
	//SDL with OpenGL:
	SDL_Init(SDL_INIT_EVERYTHING); //initalize all SDL subsystems
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

	for (unsigned int i = 1; i <= MAX_SPRITES; i++)
	{
		FreeInstanceAttributeIndices.push(MAX_SPRITES - i); //populate free indices stack
	}

	glm::mat4 viewMat = glm::mat4(1.0f); //identity matrix
	viewMat = glm::translate(viewMat, -glm::vec3(0.0f, 0.0f, 1.0f)); //translate by inverse of camera position

	float aspect = (float)width / (float)height; //calculate aspect ratio
	glm::mat4 projMat = glm::ortho(-aspect, aspect, -1.0f, 1.0f); //calculate orthographic projection matrix
	projViewMat = projMat * viewMat; //combine projection and view matrices into one matrix

	//Box2d
	b2WorldDef worldDef = b2DefaultWorldDef(); //create a world definition for box2d
	b2WorldId pWorld = b2CreateWorld(&worldDef); //create a box2d world from that definition
	
	skibidi = new DrawableObject(glm::fvec2(1.0f, 0.0f), glm::radians(45.0f), glm::fvec2(0.25f), glm::fvec4(0.0f, 128.0f, 0.0f, 128.0f), GlobalInstanceAttributes, projViewMat);
	toilet = new DrawableObject(glm::fvec2(-1.0f, 0.0f), glm::fvec2(0.1f), glm::fvec4(128.0f, 256.0f, 0.0f, 128.0f), GlobalInstanceAttributes, projViewMat);

	basicShader = new Shader(Path("assets/shaders/basic.vert"), Path("assets/shaders/basic.frag"));

	std::string yammaYamma = Path("assets/sprites/arrow.bmp");
	upArrow = SDL_LoadBMP(yammaYamma.c_str());

	screenRect = CalculateScreenRect(projViewMat);

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
			handleKeys(&event.key);
			break;
		}
	}
}

static void handleKeys(SDL_KeyboardEvent* key)
{
	switch (key->keysym.sym)
	{
	case SDLK_ESCAPE:
		quit(1);
		break;
	}
}

void loop()
{
	handleEvents();
}

void draw()
{
	//...
	SDL_GL_SwapWindow(window); //swap buffers
}

static unsigned int PopFreeIndex()
{
	unsigned int newFreeIndex = FreeInstanceAttributeIndices.top();
	FreeInstanceAttributeIndices.pop();
	return newFreeIndex;
}

static void PushFreeIndex(unsigned int freeIndex)
{
	FreeInstanceAttributeIndices.push(freeIndex);
}

/*std::string Path(std::string assetPath)
{
	std::string path(SDL_GetBasePath() + assetPath);
	return path;
}*/