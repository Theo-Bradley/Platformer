#define SDL_MAIN_NEEDED //redefine main to initalize some C and C++ things (needed because we don't use WinMain)
#include <SDL/SDL.h>
#include <box2d/box2d.h>

void quit(int code); //quit function prototype

static SDL_Window* window; //main window
static SDL_Renderer* renderer; //main renderer

int main(int argv, char** args)
{
	//Initialization:
	SDL_Init(SDL_INIT_EVERYTHING); //initalize all SDL subsystems
	window = SDL_CreateWindow("Platformer", 0, 0, 1920, 1080, SDL_WINDOW_FULLSCREEN_DESKTOP); //create new SDL window
	if (!window) //if failed to create window
		quit(-1); //quit with error code -1
	b2WorldDef worldDef = b2DefaultWorldDef(); //create a world definition for box2d
	b2WorldId pWorld = b2CreateWorld(&worldDef); //create a box2d world from that definition
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Working", "It's Working!", NULL); //post a basic messagebox

	return 0;
}

void quit(int code)
{
	SDL_Quit(); //quit SDL
	exit(code); //exit the application with the error code
}