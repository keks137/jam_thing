#include "raylib.h"
#include <math.h>
#include <stddef.h>
#include "vassert.h"
#include <string.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

typedef enum {
	Mushroom,
	Pepperoni,

} IngredientKind;
#define MAX_INGREDIENTS 32
#define MAX_PIZZAS 12
#define MAX_SLICES 32
typedef struct {
	IngredientKind kind;
	Vector2 pos; // NOTE: offset from center of non-rotated pizza
} Ingredient;
typedef struct {
	Ingredient ingredients[MAX_INGREDIENTS];
} Slice;
typedef struct {
	Slice slices[MAX_SLICES];
	Vector2 pos;
	size_t slices_count;
	float rotation;
	float rot_speed;
} Pizza;
typedef struct {
	Pizza pizzas[MAX_PIZZAS];
	size_t pizza_count;
	Texture pizza_texture;
	float pizza_texture_scale;
	int screen_width;
	int screen_height;
} GameState;

GameState gs;

size_t get_pizza()
{
	VENSURE(gs.pizza_count < MAX_PIZZAS);
	size_t index = gs.pizza_count;
	gs.pizza_count++;
	return index;
}
void draw_pizza(const Pizza *p)
{
	float sw = gs.pizza_texture.width * gs.pizza_texture_scale;
	float sh = gs.pizza_texture.height * gs.pizza_texture_scale;
	float radius = sh / 2;

	Rectangle source = { 0, 0, gs.pizza_texture.width, gs.pizza_texture.height };
	Rectangle dest = { p->pos.x, p->pos.y, sw, sh };
	Vector2 origin = { sw / 2.0f, sh / 2.0f };

	DrawTexturePro(gs.pizza_texture, source, dest, origin, p->rotation, WHITE);

	float slice_step = 360.0 / p->slices_count;
	for (float i = 0; i < p->slices_count; i++) {
		DrawText(TextFormat("%f", slice_step), 0, 0, 20, GREEN);
		float a = (i*slice_step + p->rotation) * DEG2RAD;

		Vector2 end = {
			p->pos.x + cosf(a) * radius,
			p->pos.y + sinf(a) * radius
		};

		DrawLineEx(p->pos, end, 3, BLACK);
	}
}
void UpdateDrawFrame(void)
{
	float delta = GetFrameTime();

	BeginDrawing();
	ClearBackground(RAYWHITE);
	for (size_t i = 0; i < gs.pizza_count; i++) {
		Pizza *p = &gs.pizzas[i];
		p->rotation += delta * 10 * p->rot_speed;
		draw_pizza(p);
	}
	// DrawFPS(10, 10);
	EndDrawing();
}
int main()
{
	gs.screen_width = 1920;
	gs.screen_height = 1080;
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(gs.screen_width, gs.screen_height, "raylib - project_name");
	gs.pizza_texture = LoadTexture("assets/Pizza.PNG");
	gs.pizza_texture_scale = 0.40;

	size_t pizza_index = get_pizza();
	{
		Pizza *p = &gs.pizzas[pizza_index];
		p->pos = (Vector2){ gs.screen_width / 2.0,
				    gs.screen_height / 2.0 };
		p->rot_speed = 1.0;
		p->slices_count = 6;
	}
#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
	// SetTargetFPS(60); // Set our game to run at 60 frames-per-second
	while (!WindowShouldClose())
		UpdateDrawFrame();
#endif
	CloseWindow();
	return 0;
}
