#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "assets.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#include "../vendor/raylib/src/raylib.h"
#include "../vendor/raylib/src/raymath.h"
// #include "raylib.h"
// #include "raymath.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 400

#define GAME_TIME 360
#define EARLY_PHASE_TIME 120
#define MIDDLE_PHASE_TIME 120
#define END_PHASE_TIME 120

typedef enum {
  EARLY_PHASE,
  MIDDLE_PHASE,
  END_PHASE,
  RESULTS_DISPLAY,
} GamePhase;

#define PIZZA_RADIUS 90
#define PIZZA_BASE_ROTATION_SPEED 100

typedef enum {
  NONE,
  MUSHROOM,
  OLIVE,
  PEPPERONI,
} ToppingType;

typedef struct {
  float offset_from_center;
  float rotation;
  ToppingType type;
} Topping;

typedef struct {
  Topping* items;
  size_t count;
  size_t capacity;
} Toppings;

typedef struct {
  size_t number_of_slices;
  Toppings toppings;
  float rotation;
} Pizza;

typedef struct {
  float rotation_speed;
  bool active;
  Pizza pizza;
  RenderTexture2D tex;
  Vector2 position;
} PizzaRotator;

typedef struct {
  bool completed;
  double order_time;
  double deliver_time;
  Toppings requested;
} Order;

typedef struct {
  Order* items;
  size_t count;
  size_t capacity;
  size_t active;
} Orders;

PizzaRotator rotators[3] = {
  {.rotation_speed = 0, .active = false, .pizza = {0}, .tex = {0}, .position = {0}},
  {.rotation_speed = 0, .active = false, .pizza = {0}, .tex = {0}, .position = {0}},
  {.rotation_speed = 0, .active = false, .pizza = {0}, .tex = {0}, .position = {0}},
};

GamePhase game_phase = EARLY_PHASE;
Orders orders = {0};
ToppingType topping_selected = NONE;
int min_time_from_last_order = 15;
int max_active_orders = 1;
float start_time = 0;

static void UpdateDrawFrame(void);

int main()
{

	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spinzza");

  for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
    rotators[i].rotation_speed = PIZZA_BASE_ROTATION_SPEED;
    rotators[i].position = (Vector2){
      .x = PIZZA_RADIUS*(2*i+1)+65*(i+1),
      .y = SCREEN_HEIGHT*13/20
    };
    rotators[i].tex = LoadRenderTexture(PIZZA_RADIUS*2, PIZZA_RADIUS*2); 
    rotators[i].pizza.number_of_slices = 6;
  }
  rotators[1].active = true;

#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
	SetTargetFPS(60);

	while (!WindowShouldClose())
		UpdateDrawFrame();
#endif

  UnloadRenderTexture(rotators[1].tex);

	CloseWindow();

	return 0;
}

static void UpdateDrawFrame(void)
{
  float dt = GetFrameTime();
  Vector2 mouse_pos = GetMousePosition();
  double time = GetTime();

  switch (game_phase) {
    case EARLY_PHASE: {
      if (time - start_time >= EARLY_PHASE_TIME) {
        game_phase = MIDDLE_PHASE;
        max_active_orders = 2;
        break;
      }
    } break;
    case MIDDLE_PHASE: {
      if (time  - start_time >= MIDDLE_PHASE) {
        game_phase = END_PHASE;
        max_active_orders = 4;
        break;
      }

    } break;
    case END_PHASE: {
      if (time - start_time >= MIDDLE_PHASE) {
        game_phase = RESULTS_DISPLAY;
        break;
      }

    } break;
    case RESULTS_DISPLAY: {

    } break;
  }

  if (orders.count == 0 || (time - da_last(&orders).deliver_time > min_time_from_last_order && orders.active < max_active_orders)) {
    if ((rand() % 100 + 1) > (99 - (int)(time - (orders.count == 0 ? 0 : da_last(&orders).deliver_time) - min_time_from_last_order))) {
    Order order = {
        .completed = false,
        .order_time = time,
        .deliver_time = 0,
      };
      da_append(&orders, order);
      orders.active += 1;
      printf("new order\n");
    }
  }

  for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
    if (rotators[i].active) {
      rotators[i].pizza.rotation += rotators[i].rotation_speed * dt / PIZZA_RADIUS;
    }
  }

  if (topping_selected == NONE && IsMouseButtonDown(MOUSE_BUTTON_LEFT) 
    && mouse_pos.y < SCREEN_HEIGHT*3/10 && mouse_pos.x < SCREEN_WIDTH*2/5) {
    if (mouse_pos.x < SCREEN_WIDTH*2/5*1/3) {
      topping_selected = MUSHROOM;
    } else if (mouse_pos.x < SCREEN_WIDTH*2/5*2/3) {
      topping_selected = OLIVE;
    } else {
      topping_selected = PEPPERONI;
    }
  }

  if (topping_selected != NONE && IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      if (!rotators[i].active) continue;
      if (!CheckCollisionPointCircle(mouse_pos, rotators[i].position, PIZZA_RADIUS)) continue;
      Vector2 triangle_verts[3] = {
        rotators[i].position,
        {
          .x = rotators[i].position.x - PIZZA_RADIUS*tan(PI/rotators[i].pizza.number_of_slices),
          .y = rotators[i].position.y-PIZZA_RADIUS
        },
        {
          .x = rotators[i].position.x + PIZZA_RADIUS*tan(PI/rotators[i].pizza.number_of_slices),
          .y = rotators[i].position.y-PIZZA_RADIUS
        },
      };
      if (!CheckCollisionPointTriangle(mouse_pos, triangle_verts[0], triangle_verts[1], triangle_verts[2])) continue;

      Topping topping = {
        .type = topping_selected,
        .offset_from_center = Vector2Distance(mouse_pos, rotators[i].position),
        .rotation = atan2f(mouse_pos.y - rotators[i].position.y,
                        mouse_pos.x - rotators[i].position.x) - rotators[i].pizza.rotation,
      }; 
      da_append(&rotators[i].pizza.toppings, topping);
    }
    topping_selected = NONE;
  }

	BeginDrawing(); {
    ClearBackground(WHITE);

    DrawRectangle(0, 0, SCREEN_WIDTH*2/5*1/3, SCREEN_HEIGHT*3/10, BEIGE);
    DrawLine(SCREEN_WIDTH*2/5*1/3, 0, SCREEN_WIDTH*2/5*1/3, SCREEN_HEIGHT*3/10, BLACK);
    DrawRectangle(SCREEN_WIDTH*2/5*1/3, 0, SCREEN_WIDTH*2/5*1/3, SCREEN_HEIGHT*3/10, DARKGREEN);
    DrawLine(SCREEN_WIDTH*2/5*2/3, 0, SCREEN_WIDTH*2/5*2/3, SCREEN_HEIGHT*3/10, BLACK);
    DrawRectangle(SCREEN_WIDTH*2/5*2/3, 0, SCREEN_WIDTH*2/5*1/3, SCREEN_HEIGHT*3/10, MAROON);
    DrawLine(SCREEN_WIDTH*2/5, 0, SCREEN_WIDTH*2/5, SCREEN_HEIGHT*3/10, BLACK);

    DrawRectangle(SCREEN_WIDTH*2/5, 0, SCREEN_WIDTH*3/5, SCREEN_HEIGHT*3/10, GRAY);

    DrawRectangle(0, SCREEN_HEIGHT*3/10, SCREEN_WIDTH, SCREEN_HEIGHT*7/10, BROWN);
    DrawLine(0, SCREEN_HEIGHT*3/10, SCREEN_WIDTH, SCREEN_HEIGHT*3/10, BLACK);

    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      if (rotators[i].active) {
        BeginTextureMode(rotators[i].tex); {
          ClearBackground(BLANK);

          DrawCircle(PIZZA_RADIUS, PIZZA_RADIUS, PIZZA_RADIUS, RED);
          for (size_t j = 1; j <= rotators[i].pizza.number_of_slices; j++) {
            DrawLine(PIZZA_RADIUS, PIZZA_RADIUS, PIZZA_RADIUS + PIZZA_RADIUS * cosf(j*2*PI/rotators[i].pizza.number_of_slices), PIZZA_RADIUS + PIZZA_RADIUS * sinf(j*2*PI/rotators[i].pizza.number_of_slices), BLACK);
          }

          for (size_t j = 0; j < rotators[i].pizza.toppings.count; j++) {
            Color topping_color = BLANK;
            switch (rotators[i].pizza.toppings.items[j].type) {
              case MUSHROOM: topping_color = BEIGE; break;
              case OLIVE: topping_color = DARKGREEN; break;
              case PEPPERONI: topping_color = MAROON; break;
              default: assert(false && "unkown type of topping");
            }
            Rectangle rec = {
              .x = PIZZA_RADIUS + rotators[i].pizza.toppings.items[j].offset_from_center * cosf(rotators[i].pizza.toppings.items[j].rotation),
              .y = PIZZA_RADIUS + rotators[i].pizza.toppings.items[j].offset_from_center * sinf(rotators[i].pizza.toppings.items[j].rotation),
              .width = 18,
              .height = 18,
            };
            DrawRectanglePro(rec, (Vector2){.x = 9, .y = 9}, rotators[i].pizza.toppings.items[j].rotation * RAD2DEG, topping_color);
          }
        } EndTextureMode();

        DrawTexturePro(
          rotators[i].tex.texture,
          (Rectangle){
              0,
              0,
              rotators[i].tex.texture.width,
              -rotators[i].tex.texture.height
          },
          (Rectangle){
              rotators[i].position.x,
              rotators[i].position.y,
              rotators[i].tex.texture.width,
              rotators[i].tex.texture.height
          },
          (Vector2){
              rotators[i].tex.texture.width/2.0f,
              rotators[i].tex.texture.height/2.0f
          },
          rotators[i].pizza.rotation * RAD2DEG,
          WHITE
        );

        DrawCircleSectorLines((Vector2){rotators[i].position.x, rotators[i].position.y}, PIZZA_RADIUS, 270-180/rotators[i].pizza.number_of_slices, 270+180/rotators[i].pizza.number_of_slices, 100, YELLOW);
      } else {
        DrawCircle(PIZZA_RADIUS*(2*i+1)+65*(i+1), SCREEN_HEIGHT*13/20, PIZZA_RADIUS, GRAY);
      }
    }

    if (topping_selected != NONE) {
      Color topping_color = BLANK;
      switch (topping_selected) {
        case NONE: break;
        case MUSHROOM: topping_color = BEIGE; break;
        case OLIVE: topping_color = DARKGREEN; break;
        case PEPPERONI: topping_color = MAROON; break;
        default: assert(false && "unknown topping to render");
      }
      DrawRectangle(mouse_pos.x-18/2, mouse_pos.y-18/2, 18, 18, topping_color);
      DrawRectangleLines(mouse_pos.x-18/2, mouse_pos.y-18/2, 18, 18, BLACK);
    }
  } EndDrawing();
}
