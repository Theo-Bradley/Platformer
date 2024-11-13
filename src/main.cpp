#define SDL_MAIN_NEEDED //redefine main to initalize some C and C++ things (needed because we don't use WinMain)
#include <SDL/SDL.h>
#include <glad/glad.h>
#include <SDL/SDL_opengl.h>
#include <box2d/box2d.h>

float plane[12] = {
	-0.5, -0.5, 0.0,
	0.5, -0.5, 0.0,
	0.5, 0.5, 0.0,
	-0.5, 0.5, 0.0};

unsigned int planeIndices[6] = {
	0, 1, 2,
	2, 3, 0
};

const char* vert_default = "#version 330 core\n"
"layout (location = 0) in vec3 position;\n"
"void main()\n"
"{\n"
"gl_Position = vec4(position.x, position.y, position.z, 1.0);\n"
"}\0";

const char* frag_default = "#version 330 core\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = vec4(1.0f, 0.0f, 1.0f, 1.0f);\n"
"}\n\0";

void quit(int code); //quit function prototype
int init();
static void handleEvents();
void handleKeys(SDL_KeyboardEvent* key);
void loop();

SDL_Window* window; //main window
static SDL_Renderer* renderer; //main renderer
SDL_GLContext glContext;
bool running = true;

int main(int argv, char** args)
{
	init();
	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Working", "It's Working!", NULL); //post a basic messagebox

	GLuint VBO = 0;
	GLuint VIO = 0;
	GLuint VAO = 0;
	GLuint defaultProg = 0;
	GLuint defaultVert = 0;
	GLuint defaultFrag = 0;
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &VIO);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VIO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

	defaultVert = glCreateShader(GL_VERTEX_SHADER);
	defaultFrag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(defaultVert, 1, &vert_default, NULL);
	glCompileShader(defaultVert);
	glShaderSource(defaultFrag, 1, &frag_default, NULL);
	glCompileShader(defaultFrag);

	defaultProg = glCreateProgram();
	glAttachShader(defaultProg, defaultVert);
	glAttachShader(defaultProg, defaultFrag);
	glLinkProgram(defaultProg);

	while (running)
	{
		loop();
		//temp render code
		glBindVertexArray(VAO);
		glUseProgram(defaultProg);
		glDrawElements(GL_TRIANGLES, sizeof(planeIndices), GL_UNSIGNED_INT, 0);
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
	//SDL OpenGL:
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_Init(SDL_INIT_EVERYTHING); //initalize all SDL subsystems
	window = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL); //create new SDL OGL window
	if (!window) //if failed to create window
		quit(-1); //quit with error code -1
	glContext = SDL_GL_CreateContext(window); //Create GL context
	if (!glContext)
		quit(-1);
	gladLoadGLLoader(SDL_GL_GetProcAddress); //load GL function pointers through SDL
	//OpenGL
	glClearColor(0.53, 0.81, 0.92, 1.0); //set sky blue clear color RGBA
	int width, height; //window width and height
	SDL_GetWindowSize(window, &width, &height); //get current window size
	glViewport(0, 0, width, height); //create viewport
	glClear(GL_COLOR_BUFFER_BIT); //clear back buffer
	SDL_GL_SwapWindow(window); //swap buffers
	glClear(GL_COLOR_BUFFER_BIT); //clear back buffer
	glDisable(GL_CULL_FACE); //disable face culling for now
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//Box2d
	b2WorldDef worldDef = b2DefaultWorldDef(); //create a world definition for box2d
	b2WorldId pWorld = b2CreateWorld(&worldDef); //create a box2d world from that definition
	
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
	SDL_GL_SwapWindow(window);


}

void draw()
{
}