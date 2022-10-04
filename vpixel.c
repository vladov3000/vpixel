#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>

typedef enum {
	EMPTY,
	WALL,
	SAND,
	WATER,
	PLANT,
	BORDER,
	FIRE,
	STEAM,
} CellType;

typedef struct {
	CellType type;
	union {
		int fire_t;
	};
} Cell;

#define INITIAL_PAINT_CELL (Cell) { WALL }
#define FIRE_T_MAX         100

SDL_Color cell_to_color(Cell cell) {
	switch (cell.type) {
	case EMPTY:  return (SDL_Color) {   0,   0,   0, 255 };
	case WALL:   return (SDL_Color) { 100, 100, 100, 255 };
	case SAND:   return (SDL_Color) { 255, 255,   0, 255 };
	case WATER:  return (SDL_Color) {   0,   0, 255, 255 };
	case PLANT:  return (SDL_Color) {   0, 255,   0, 255 };
	case BORDER: return (SDL_Color) { 255, 255, 255, 255 };
	case FIRE:   {
		float t = (float) cell.fire_t / FIRE_T_MAX;
		return (SDL_Color) { 200 + 55 * t, 100 + 50 * t, 0, 255 };
	}
	case STEAM:  return (SDL_Color) { 200, 200, 200, 255 };
	}
}

void key_to_cell(SDL_Keycode key, Cell* out) {
	switch (key) {
	case SDL_SCANCODE_1: *out = (Cell) { WALL }; break;
	case SDL_SCANCODE_2: *out = (Cell) { SAND }; break;
	case SDL_SCANCODE_3: *out = (Cell) { WATER }; break;
	case SDL_SCANCODE_4: *out = (Cell) { PLANT }; break;
	case SDL_SCANCODE_5: *out = (Cell) { FIRE, .fire_t = 0 }; break;
	}
}

Cell get_cell(Cell* cells, int cells_w, int cells_h, int i, int j) {
	return 0 <= i && i < cells_h && 0 <= j && j < cells_w ?
		cells[cells_w * i + j] : (Cell) { BORDER };
}

void update_cell(Cell* cells, int cells_w, int cells_h, int i, int j) {
	int idx = cells_w * i + j;

	Cell above = get_cell(cells, cells_w, cells_h, i - 1, j);
	Cell below = get_cell(cells, cells_w, cells_h, i + 1, j);
	Cell left  = get_cell(cells, cells_w, cells_h, i, j - 1);
	Cell right = get_cell(cells, cells_w, cells_h, i, j + 1);
	
	switch (cells[idx].type) {
						
	case SAND:
		if (below.type == EMPTY || below.type == WATER) {
			cells[idx]           = below;
			cells[idx + cells_w] = (Cell) { SAND };
		}
		break;
						
	case WATER:
		if (below.type == EMPTY) {
			cells[idx]           = (Cell) { EMPTY };
			cells[idx + cells_w] = (Cell) { WATER };
		}
						
		else if (left.type == EMPTY) {
			cells[idx]     = (Cell) { EMPTY };
			cells[idx - 1] = (Cell) { WATER };
		}

		else if (right.type == EMPTY) {
			cells[idx]     = (Cell) { EMPTY };
			cells[idx + 1] = (Cell) { WATER };
		}
		break;

	case PLANT:
		if (above.type == WATER)
			cells[idx - cells_w] = (Cell) { PLANT };
		else if (below.type == WATER)
			cells[idx + cells_w] = (Cell) { PLANT };
		else if (left.type == WATER)
			cells[idx - 1] = (Cell) { PLANT };
		else if (right.type == WATER)
			cells[idx + 1] = (Cell) { PLANT };
		break;

	case FIRE: {

		/*
		if (above.type == PLANT)
			cells[idx - cells_w] = (Cell) { FIRE };
		if (below.type == PLANT)
			cells[idx + cells_w] = (Cell) { FIRE };
		*/
		printf("%d %d %d\n", right.type, idx, j);
		if (right.type == PLANT)
			cells[idx - 1] = (Cell) { FIRE };
		if (left.type == PLANT)
			cells[idx + 1] = (Cell) { FIRE };
		
		if (above.type == WATER)
			cells[idx - cells_w] = (Cell) { STEAM };
		if (below.type == WATER)
			cells[idx + cells_w] = (Cell) { STEAM };
		if (right.type == WATER)
			cells[idx - 1] = (Cell) { STEAM };
		if (left.type == WATER)
			cells[idx + 1] = (Cell) { STEAM };

		/*
		if (below.type == EMPTY) {
			Cell temp            = cells[idx];
			cells[idx]           = (Cell) { EMPTY };
			cells[idx + cells_w] = temp;
		}
		*/
		
		cells[idx].fire_t++;
		if (cells[idx].fire_t > FIRE_T_MAX)
			cells[idx].type = EMPTY;
		
		break;
	}

	case STEAM:
		if (above.type == EMPTY) {
			cells[idx - cells_w] = cells[idx];
			cells[idx]           = (Cell) { EMPTY };
		}
			
		break;

	default:
		break;
	}
}

int main(void) {
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);

	SDL_Window* window = SDL_CreateWindow("vpixel",
																				SDL_WINDOWPOS_CENTERED,
																				SDL_WINDOWPOS_CENTERED,
																				700,
																				525,
																				0);
	assert(window != NULL);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	assert(renderer != NULL);

	int cells_w        = 400;
	int cells_h        = 300;
	int texture_planes = 4;
	int texture_pitch  = cells_w * texture_planes;
	SDL_Texture* texture = SDL_CreateTexture(renderer,
																					 SDL_PIXELFORMAT_RGBA8888,
																					 SDL_TEXTUREACCESS_STREAMING,
																					 cells_w,
																					 cells_h);
	assert(texture != NULL);

	int   n_cells = texture_pitch * cells_h;
	char* pixels  = malloc(n_cells);
	assert(pixels != NULL);

	Cell* cells = malloc(n_cells * sizeof(Cell));
	for (int i = 0; i < n_cells; i++)
		cells[i].type = EMPTY;

	bool paint      = false;
	Cell paint_cell = INITIAL_PAINT_CELL;
	int  paint_r    = 5;

	Uint64 last_tick = SDL_GetTicks64();
	Uint64 ticks_per_update = 1000;

	while (true) {
		
		SDL_Event event;
		while (SDL_PollEvent(&event))
			switch (event.type) {
				
			case SDL_QUIT:
				goto CLEANUP;

			case SDL_KEYDOWN:
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
					goto CLEANUP;
				key_to_cell(event.key.keysym.scancode, &paint_cell);
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT)
					paint = true;
				break;

			case SDL_MOUSEBUTTONUP:
				if (event.button.button == SDL_BUTTON_LEFT)
					paint = false;
				break;
			}

		Uint64 new_tick = SDL_GetTicks64();

		while (new_tick - last_tick > ticks_per_update) {

			if (paint) {
				int window_w, window_h;
				SDL_GetWindowSize(window, &window_w, &window_h);
			
				int mouse_x, mouse_y;
				SDL_GetMouseState(&mouse_x, &mouse_y);

				int paint_x = mouse_x * cells_w / window_w;
				int paint_y = mouse_y * cells_h / window_h;

				for (int i = -paint_r; i < paint_r; i++)
					for (int j = -paint_r; j < paint_r; j++)
						if (i * i + j * j <= paint_r * paint_r) {
							size_t idx = cells_w * (paint_y + i) + (paint_x + j);
							cells[idx] = paint_cell;
						}
			}


			for (size_t i = cells_h - 1; i < cells_h; i--) {
				bool left = rand() > RAND_MAX / 2;
				for (size_t j = left ? 0 : cells_w - 1; j < cells_w; j += left ? 1 : -1)
					update_cell(cells, cells_w, cells_h, i, j);
			}

			for (size_t i = 0; i < cells_h; i++)
				for (size_t j = 0; j < cells_w; j++) {
					size_t  cells_idx = cells_w * i + j;
					size_t pixels_idx = texture_pitch * i + texture_planes * j;
				
					SDL_Color color = cell_to_color(cells[cells_idx]);
					char*     pixel = pixels + pixels_idx;
				
					pixel[0] = color.a;
					pixel[1] = color.b;
					pixel[2] = color.g;
					pixel[3] = color.r;
				}

			assert(SDL_UpdateTexture(texture, NULL, pixels, texture_pitch) == 0);
			if (SDL_UpdateTexture(texture, NULL, pixels, texture_pitch) != 0)
				puts(SDL_GetError());

			assert(SDL_RenderCopy(renderer, texture, NULL, NULL) == 0);

			SDL_RenderPresent(renderer);

			new_tick -= ticks_per_update;
			last_tick = new_tick;
		}
	}

 CLEANUP:
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
