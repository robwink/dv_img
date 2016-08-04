
#define CVECTOR_IMPLEMENTATION
#include "cvector.h"

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "c_utils.h"

#include <stdio.h>

//POSIX works with MinGW64
#include <sys/stat.h>
#include <dirent.h>



#include <SDL2/SDL.h>



void setup();
void cleanup(int ret);
void load_image(const char* path);
int handle_events();
void set_rect();


SDL_Window* win;
SDL_Renderer* ren;
SDL_Surface* bmp;
SDL_Texture* tex;



unsigned char* img;

int scr_width;
int scr_height;

int img_width;
int img_height;
int img_index;

float zoom;

SDL_Rect rect;

cvector_str files;




int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("usage: %s image_name\n", argv[0]);
		exit(0);
	}

	setup();

	char* path = argv[1];

	load_image(path);
	
	char dirpath[4096] = { 0 };

	//dirname
	char* last_slash = strrchr(path, '/');
	if (last_slash) {
		strncpy(dirpath, path, last_slash-path+1);
	} else {
		dirpath[0] = '.';
	}

	int dirlen = last_slash-path+1;


	DIR* dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1);
	}

	struct dirent* entry;
	struct stat file_stat;

	cvec_str(&files, 0, 20);

	while ((entry = readdir(dir))) {
		dirpath[dirlen] = 0;
		strcat(dirpath, entry->d_name);
		if (stat(dirpath, &file_stat)) {
			perror("stat");
		}

		if (S_ISREG(file_stat.st_mode)) {
			cvec_push_str(&files, dirpath);
		}
	}

	qsort(files.a, files.size, sizeof(char*), cmp_string_lt);

	for (int i=0; i<files.size; ++i) {
		if (!strcmp(files.a[i], path))
			img_index = i;
		puts(files.a[i]);
	}





/*
 * alternative to stb_image, SDL has built in bmp handling
	bmp = SDL_LoadBMP("../hello.bmp");
	if (!bmp) {
		printf("Error: %s\n", SDL_GetError());
		cleanup();
	}
*/


	set_rect();


	while (1) {
		if (handle_events()) {
			break;
		}

		SDL_SetRenderDrawColor(ren, 0,0,0,0);
		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, tex, NULL, &rect);
		//SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}


	cleanup(0);

	//never get here
	return 0;
}

#define MAX(X, Y) ((X > Y) ? X : Y)
#define MIN(X, Y) ((X < Y) ? X : Y)


void set_rect()
{
	float aspect = img_width/(float)img_height;
	
	int h = MIN(MIN(scr_height, scr_width/aspect), img_height);
	int w = h * aspect;

	rect.x = (scr_width-w)/2;
	rect.y = (scr_height-h)/2;
	rect.w = w;
	rect.h = h;
}


void load_image(const char* path)
{
	int n;
	img = stbi_load(path, &img_width, &img_height, &n, 4);
	if (!img) {
		printf("failed to load %s: %s\n", path, stbi_failure_reason());
		cleanup(1);
	}

	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, img_width, img_height);
	if (!tex) {
		printf("Error: %s\n", SDL_GetError());
		cleanup(1);
	}

	if (SDL_UpdateTexture(tex, NULL, img, img_width*4)) {
		printf("Error updating texture: %s\n", SDL_GetError());
		cleanup(1);
	}
}

void setup()
{
	win = NULL;
	ren = NULL;
	bmp = NULL;
	tex = NULL;

	img = NULL;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(1);
	}

	scr_width = 640;
	scr_height = 480;

	win = SDL_CreateWindow("dv_img", 100, 100, scr_width, scr_height, SDL_WINDOW_RESIZABLE);
	if (!win) {
		printf("Error: %s\n", SDL_GetError());
		exit(1);
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	//ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	if (!ren) {
		printf("Error: %s\n", SDL_GetError());
		cleanup(1);
	}
	SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
}


int handle_events()
{
	SDL_Event e;
	int sc;
	int height, width;

	int remake_rect = 0;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			return 1;
		case SDL_KEYDOWN:
			sc = e.key.keysym.scancode;
			switch (sc) {

			case SDL_SCANCODE_F:
				SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
				break;

			case SDL_SCANCODE_RIGHT:
			case SDL_SCANCODE_DOWN:
				img_index = (img_index + 1) % files.size;

				stbi_image_free(img);
				SDL_DestroyTexture(tex);

				//loads and creates a new texture
				load_image(files.a[img_index]);

				set_rect();

				break;

			case SDL_SCANCODE_ESCAPE:
				return 1;
			default:
				;
			}

			break;
		case SDL_MOUSEBUTTONDOWN:

			break;

		case SDL_WINDOWEVENT:
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				//printf("window size %d x %d\n", e.window.data1, e.window.data2);
				scr_width = e.window.data1;
				scr_height = e.window.data2;

				set_rect();

				break;
			}
			break;
		}
	}
	return 0;
}

void cleanup(int ret)
{
	stbi_image_free(img);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	SDL_Quit();

	exit(ret);
}
