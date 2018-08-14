#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// SDL
#include <SDL2/SDL.h>

static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 480;

static SDL_Texture *texture_from_bmp(const char *const file,
		SDL_Renderer *const ren)
{
	SDL_Surface *const image = SDL_LoadBMP(file);
	if (!image) {
		fprintf(stderr, "SDL_LoadBMP: %s\n", SDL_GetError());
		return NULL;
	}

	SDL_Texture *const tex = SDL_CreateTextureFromSurface(ren, image);
	SDL_FreeSurface(image);

	if (!tex) {
		fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n",
				SDL_GetError());
		return NULL;
	}

	return tex;
}

void render_texture(SDL_Texture *const tex, SDL_Renderer *ren,
		const int x, const int y)
{
	SDL_Rect dst = {
		.x = x,
		.y = y,
	};

	// unspec. pixelformat, unspec. texture access
	// query width and height of texture
	SDL_QueryTexture(tex, NULL, NULL,
			&dst.w, &dst.h);
	// no source rectangle
	SDL_RenderCopy(ren, tex, NULL, &dst);
}


int main(void)
{
	int ret = 0;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return (1);
	}

	/* Path to our binary */
	char *const basePath = SDL_GetBasePath();
	fprintf(stderr, "SDL Basepath: %s\n", basePath);

	SDL_Window *win = SDL_CreateWindow("Title",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			SCREEN_WIDTH,
			SCREEN_HEIGHT,
			SDL_WINDOW_SHOWN);
	if (!win) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		ret = 2;
		goto free_basePath;
	}

	SDL_Renderer *ren = SDL_CreateRenderer(win,
			-1, /* first renderer available */
			SDL_RENDERER_ACCELERATED |
			SDL_RENDERER_PRESENTVSYNC);
	if (!ren) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		ret = 3;
		goto free_window;
	}

	SDL_Texture *tex;
	{
		/* calculate path */
		const char hello[] = "hello.bmp";
		const size_t len = strlen(hello) + strlen(basePath);
		char *const imagePath = calloc(sizeof (char), len+1);
		const int r = snprintf(imagePath, len+1, "%s%s", basePath, hello);
		assert (r >= 0);
		assert ((unsigned)r == len);

		tex = texture_from_bmp(imagePath, ren);
		free(imagePath);

		if (!tex) {
			ret = 5;
			goto free_renderer;
		}
	}

	/* display image, wait 3 seconds */
	for (int i = 0; i < 3; ++i) {
		SDL_RenderClear(ren);
		// render entire texture
		render_texture(tex, ren, 0, 0);
		SDL_RenderPresent(ren);
		SDL_Delay(1000);
	}

	goto free_surface;
free_surface:
	SDL_DestroyTexture(tex);
free_renderer:
	SDL_DestroyRenderer(ren);
free_window:
	SDL_DestroyWindow(win);
free_basePath:
	SDL_free(basePath);
	SDL_Quit();

	return (ret);
}
