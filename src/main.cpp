#define SDL_MAIN_NEEDED
#include <SDL\SDL.h>
#include <box2d/box2d.h>

void quit(int code);

static SDL_Window* window; //main window
static SDL_Renderer* renderer;

int main(int argv, char** args)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("Platformer", 0, 0, 1920, 1080, SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (!window)
		quit(-1);
	b2WorldDef worldDef = b2DefaultWorldDef();
	b2WorldId pWorld = b2CreateWorld(&worldDef);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Working", "It's Working!", NULL);
	return 0;
}

void quit(int code)
{
	SDL_Quit();
	exit(code);
}