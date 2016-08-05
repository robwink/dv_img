
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


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>



void setup(const char* img_name);
void cleanup(int ret);
int load_image(const char* path);
int handle_events();

//TODO combine/reorg
void adjust_rect();
void set_rect_actual();
void set_rect_bestfit();
void set_rect_zoom();


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

int zoom;

SDL_Rect rect;

cvector_str files;


// works same as SUSv2 libgen.h dirname except that
// dirpath is user provided output buffer, assumed large
// enough, return value is dirpath
char* dirname(const char* path, char* dirpath)
{

#define PATH_SEPARATOR '/'

	if (!path || !path[0]) {
		dirpath[0] = '.';
		dirpath[1] = 0;
		return dirpath;
	}

	char* last_slash = strrchr(path, PATH_SEPARATOR);
	if (last_slash) {
		strncpy(dirpath, path, last_slash-path);
		puts("1");
	} else {
		dirpath[0] = '.';
		dirpath[1] = 0;
		puts("2");
	}

	printf("%p\ndirpath = %s\n", dirpath, dirpath);
	return dirpath;
}

// same as SUSv2 basename in libgen.h except base is output
// buffer
char* basename(const char* path, char* base)
{
	if (!path || !path[0]) {
		base[0] = '.';
		base[1] = 0;
		return base;
	}

	int end = strlen(path) - 1;

	if (path[end] == PATH_SEPARATOR)
		end--;

	int start = end;
	while (path[start] != PATH_SEPARATOR && start != 0)
		start--;
	if (path[start] == PATH_SEPARATOR)
		start++;

	memcpy(base, &path[start], end-start+1);
	base[end-start+1] = 0;

	return base;
}






int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("usage: %s image_name\n", argv[0]);
		exit(0);
	}

	char* path = argv[1];
	char dirpath[4096] = { 0 };
	char fullpath[4096] = { 0 };
	char img_name[1024] = { 0 };
	int dirlen = 0;

	//dirname
	dirname(path, dirpath);
	basename(path, img_name);

	printf("%p\ndirpath = %s\n", dirpath, dirpath);

	setup(img_name);


	if (!load_image(path)) {
		cleanup(0);
	}
	



	DIR* dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1);
	}

	struct dirent* entry;
	struct stat file_stat;

	cvec_str(&files, 0, 20);

	int ret;
	while ((entry = readdir(dir))) {
		ret = snprintf(fullpath, 4096, "%s/%s", dirpath, entry->d_name);
		if (ret >= 4096) {
			printf("path too long\n");
			cleanup(0);
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}
		if (S_ISREG(file_stat.st_mode)) {
			cvec_push_str(&files, fullpath);
		}
	}
	closedir(dir);

	qsort(files.a, files.size, sizeof(char*), cmp_string_lt);

	for (int i=0; i<files.size; ++i) {
		if (!strcmp(files.a[i], path))
			img_index = i;
		puts(files.a[i]);
	}

	set_rect_bestfit();


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



//need to think about best way to organize following 4 functions' functionality
void adjust_rect()
{
	rect.x = (scr_width-rect.w)/2;
	rect.y = (scr_height-rect.h)/2;
}


void set_rect_bestfit()
{
	float aspect = img_width/(float)img_height;
	int h, w;
	
	h = MIN(MIN(scr_height, scr_width/aspect), img_height);
	w = h * aspect;

	rect.x = (scr_width-w)/2;
	rect.y = (scr_height-h)/2;
	rect.w = w;
	rect.h = h;
}

void set_rect_actual()
{
	rect.x = (scr_width-img_width)/2;
	rect.y = (scr_height-img_height)/2;
	rect.w = img_width;
	rect.h = img_height;
}

void set_rect_zoom()
{
	float aspect = img_width/(float)img_height;
	int h, w;
	
	h = rect.h * (1.0 + zoom*0.05);
	w = h * aspect;

	rect.x = (scr_width-w)/2;
	rect.y = (scr_height-h)/2;
	rect.w = w;
	rect.h = h;
}



int load_image(const char* path)
{
	int n;
	img = stbi_load(path, &img_width, &img_height, &n, 4);
	printf("path = %s\n", path);
	if (!img) {
		printf("failed to load %s: %s\n", path, stbi_failure_reason());
		return 0;
	}

	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, img_width, img_height);
	if (!tex) {
		printf("Error: %s\n", SDL_GetError());
		return 0;
	}

	if (SDL_UpdateTexture(tex, NULL, img, img_width*4)) {
		printf("Error updating texture: %s\n", SDL_GetError());
		return 0;
	}

	return 1;
}

void setup(const char* img_name)
{
	win = NULL;
	ren = NULL;
	bmp = NULL;
	tex = NULL;

	img = NULL;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	//not sure if necessary
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(1);
	}

	scr_width = 640;
	scr_height = 480;

	//char title_buf[1024] = { "dv_img" };

	win = SDL_CreateWindow(img_name, 100, 100, scr_width, scr_height, SDL_WINDOW_RESIZABLE);
	if (!win) {
		printf("Error: %s\n", SDL_GetError());
		exit(1);
	}

	//ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
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
	int ret;
	char title_buf[1024];

	int remake_rect = 0;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			return 1;
		case SDL_KEYDOWN:
			sc = e.key.keysym.scancode;
			switch (sc) {

			case SDL_SCANCODE_1:
				set_rect_actual();
				break;

			case SDL_SCANCODE_F:
				set_rect_bestfit();
				break;

			case SDL_SCANCODE_F11:
				SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
				break;

			case SDL_SCANCODE_RIGHT:
			case SDL_SCANCODE_DOWN:
				puts("right");
				stbi_image_free(img);
				SDL_DestroyTexture(tex);

				//find next available image
				do {
					img_index = (img_index + 1) % files.size;
				} while (!(ret = load_image(files.a[img_index])));

				SDL_SetWindowTitle(win, basename(files.a[img_index], title_buf));

				set_rect_bestfit();
				break;

			case SDL_SCANCODE_LEFT:
			case SDL_SCANCODE_UP:
				stbi_image_free(img);
				SDL_DestroyTexture(tex);

				//find next available image
				do {
					img_index = (img_index-1 < 0) ? files.size-1 : img_index-1;
				} while (!(ret = load_image(files.a[img_index])));

				SDL_SetWindowTitle(win, basename(files.a[img_index], title_buf));

				set_rect_bestfit();
				break;

			case SDL_SCANCODE_ESCAPE:
				return 1;
			default:
				;
			}

			break;
		case SDL_MOUSEBUTTONDOWN:

			break;

		case SDL_MOUSEWHEEL:
			if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL)
				zoom = e.wheel.y;
			else
				zoom = -e.wheel.y;
			set_rect_zoom();
			break;

		case SDL_WINDOWEVENT:
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				//printf("window size %d x %d\n", e.window.data1, e.window.data2);
				scr_width = e.window.data1;
				scr_height = e.window.data2;

				adjust_rect();

				break;
			case SDL_WINDOWEVENT_EXPOSED:
				puts("exposed event");
				break;
			default:
				;
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
