#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "assets.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#include "../vendor/raylib/src/raylib.h"
#include "../vendor/raylib/src/raymath.h"
// #include <raylib.h>
// #include <raymath.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

#define GAME_TIME 360
#define EARLY_PHASE_TIME 120
#define MIDDLE_PHASE_TIME 120
#define END_PHASE_TIME 120
#define FPS 60
typedef enum {
  EARLY_PHASE,
  MIDDLE_PHASE,
  END_PHASE,
  RESULTS_DISPLAY,
} GamePhase;


typedef enum {
  TOPPING_NONE,
  TOPPING_MUSHROOM,
  TOPPING_OLIVE,
  TOPPING_PEPPERONI,
  TOPPING_BELL_PEPPER,
  TOPPING_CORN,
  TOPPING_FETA,
  TOPPING_SPINACH,
} ToppingType;

typedef enum {
  TOPPING_POSITION_NONE,
  TOPPING_POSITION_FULL,
  TOPPING_POSITION_HALF1,
  TOPPING_POSITION_HALF2,
  TOPPING_POSITION_ALTERNATE1,
  TOPPING_POSITION_ALTERNATE2,
} ToppingPosition;

typedef struct {
  float offset_from_center;
  float rotation;
  ToppingType type;
  ToppingPosition requested_position;
} Topping;

typedef struct {
  Topping* items;
  size_t count;
  size_t capacity;
} Toppings;

#define PIZZA_RADIUS 209
#define PIZZA_BASE_ROTATION_SPEED 31.415 * (PI / 180)

typedef struct {
  size_t number_of_slices;
  size_t order_index;
  Toppings toppings;
  float rotation;
  Vector2 position;
  RenderTexture2D render_tex;
  bool delivered;
} Pizza;

typedef struct {
  Pizza* items;
  size_t count;
  size_t capacity;
} Pizzas;

typedef size_t PizzaIndex;

#define PIZZA_ROTATOR_RADIUS 90

typedef struct {
  float rotation_speed;
  bool active;
  size_t order_index;
  bool spinning;
  PizzaIndex pizza_index;
  Vector2 position;
} PizzaRotator;

typedef struct {
  bool completed;
  double order_time;
  double deliver_time;
  Toppings requested_toppings;
  size_t rotator_index;
  size_t pizza_index;
} Order;

typedef struct {
  Order* items;
  size_t count;
  size_t capacity;
  size_t active;
} Orders;

typedef struct {
  Vector2 position;
  RenderTexture2D render_tex;
} OrderTicket;

typedef struct {
  Vector2 position;
  ToppingType type;
} ConveyorBeltIngredient;

#define EARLY_PHASE_CONVEYOR_BELT_SPEED 3
#define MIDDLE_PHASE_CONVEYOR_BELT_SPEED 5
#define END_PHASE_CONVEYOR_BELT_SPEED 7

#define CONVEYOR_BELT_INGREDIENT_GAP 100

typedef struct {
  ConveyorBeltIngredient* items;
  size_t count;
  size_t capacity;
  float speed;
} ConveyorBelt;

GamePhase game_phase = EARLY_PHASE;
float start_time = 0;

PizzaRotator rotators[2] = {
  {.rotation_speed = 0, .active = false, .order_index = 0, .pizza_index = 0, .position = {0}},
  {.rotation_speed = 0, .active = false, .order_index = 0, .pizza_index = 0, .position = {0}},
};

Pizzas pizzas = {0};

ConveyorBelt conveyor_belt = {0};

Orders orders = {0};

ToppingType topping_selected = TOPPING_NONE;

size_t max_active_orders = 1;
size_t max_toppings_per_pizza = 2;

Texture2D backgroundTex;
Texture2D speedControllerTex;

Texture2D blankOrderTicketTex;
OrderTicket orderTickets[2];

bool draggingLeftCtrl = false;
bool draggingRightCtrl = false;

static void UpdateDrawFrame(void);

int main()
{
  srand(time(NULL));
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spinzza");

  backgroundTex = LoadTexture(BACKGROUND_IMG);
  speedControllerTex = LoadTexture(SPEED_CONTROLLER_IMG);
  blankOrderTicketTex = LoadTexture(ORDER_TICKET_BLANKIMG);

  orderTickets[0].render_tex = LoadRenderTexture(blankOrderTicketTex.width, blankOrderTicketTex.height);
  orderTickets[0].position = (Vector2){.x = 25, .y = 600};
  orderTickets[1].render_tex = LoadRenderTexture(blankOrderTicketTex.width, blankOrderTicketTex.height);
  orderTickets[1].position = (Vector2){.x = SCREEN_WIDTH - blankOrderTicketTex.width, .y = 600};

  conveyor_belt.speed = EARLY_PHASE_CONVEYOR_BELT_SPEED;

  for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
    rotators[i].rotation_speed = PIZZA_BASE_ROTATION_SPEED;
  }
  rotators[0].position = (Vector2){
    .x = 529,
    .y = 832
  };
  rotators[1].position = (Vector2){
    .x = 1419,
    .y = 832
  };

  
#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
	SetTargetFPS(FPS);

	while (!WindowShouldClose())
		UpdateDrawFrame();
#endif

  for (size_t i = 0; i < pizzas.count; i++) {
    if (!pizzas.items[i].delivered) UnloadRenderTexture(pizzas.items[i].render_tex);
  }
  for (size_t i = 0; i < ARRAY_LEN(orderTickets); i++) {
    UnloadRenderTexture(orderTickets[i].render_tex);
  }
  UnloadTexture(blankOrderTicketTex);
  UnloadTexture(backgroundTex);
  UnloadTexture(speedControllerTex);

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
        conveyor_belt.speed = MIDDLE_PHASE_CONVEYOR_BELT_SPEED;
        break;
      }
    } break;
    case MIDDLE_PHASE: {
      if (time  - start_time >= MIDDLE_PHASE) {
        game_phase = END_PHASE;
        conveyor_belt.speed = END_PHASE_CONVEYOR_BELT_SPEED;
        break;
      }

    } break;
    case END_PHASE: {
      if (time - start_time >= MIDDLE_PHASE) {
        max_active_orders = 0;
        game_phase = RESULTS_DISPLAY;
        break;
      }

    } break;
    case RESULTS_DISPLAY: {

    } break;
  }



  if (orders.active < max_active_orders) {
    Order order = {
      .completed = false,
      .order_time = time,
      .deliver_time = 0,
      .requested_toppings = {0},
    };

    Toppings available_toppings = {0};
    da_append(&available_toppings, ((Topping){.type = TOPPING_MUSHROOM}));
    da_append(&available_toppings, ((Topping){.type = TOPPING_OLIVE}));
    da_append(&available_toppings, ((Topping){.type = TOPPING_PEPPERONI}));
    da_append(&available_toppings, ((Topping){.type = TOPPING_BELL_PEPPER}));
    da_append(&available_toppings, ((Topping){.type = TOPPING_CORN}));
    da_append(&available_toppings, ((Topping){.type = TOPPING_FETA}));
    da_append(&available_toppings, ((Topping){.type = TOPPING_SPINACH}));

    for (size_t i = 0; i < max_toppings_per_pizza; i++) {
      int prob = rand() % 1000 + 1;
      Topping topping = {0};
      switch (game_phase) {
        case EARLY_PHASE: {
          if (prob < 990) {
            topping.requested_position = TOPPING_POSITION_FULL;
          } else {
            topping.requested_position = TOPPING_POSITION_HALF1;
          }
        } break;
        case MIDDLE_PHASE: {
          if (prob < 400) {
            topping.requested_position = TOPPING_POSITION_FULL;
          } else if (prob < 600) {
            topping.requested_position = TOPPING_POSITION_HALF1;
          } else if (prob < 800) {
            topping.requested_position = TOPPING_POSITION_HALF2;
          } else if (prob < 900) {
            topping.requested_position = TOPPING_POSITION_ALTERNATE1;
          } else {
            topping.requested_position = TOPPING_POSITION_ALTERNATE2;
          }
        } break;
        case END_PHASE: {
          if (prob < 200) {
            topping.requested_position = TOPPING_POSITION_FULL;
          } else if (prob < 400) {
            topping.requested_position = TOPPING_POSITION_HALF1;
          } else if (prob < 600) {
            topping.requested_position = TOPPING_POSITION_HALF2;
          } else if (prob < 800) {
            topping.requested_position = TOPPING_POSITION_ALTERNATE1;
          } else {
            topping.requested_position = TOPPING_POSITION_ALTERNATE2;
          }
        } break;
        default:
          assert(false && "shouldn't add orders in this state");
      }

      size_t available_toppings_index = rand() % available_toppings.count;
      topping.type = available_toppings.items[available_toppings_index].type;
      da_remove_unordered(&available_toppings, available_toppings_index);

      da_append(&order.requested_toppings, topping);
    }
    da_free(available_toppings);

    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      if (rotators[i].active) continue;
      order.rotator_index = i;
      rotators[i].order_index = orders.count;
      order.pizza_index = pizzas.count;
      rotators[i].pizza_index = pizzas.count;
      rotators[i].active = true;
      rotators[i].spinning = true;
      da_append(&pizzas, ((Pizza){
        .number_of_slices = 6,
        .order_index = orders.count,
        .rotation = 0,
        .position = rotators[i].position,
        .render_tex = LoadRenderTexture(PIZZA_RADIUS*2, PIZZA_RADIUS*2),
        .toppings = {0}, 
        .delivered = false,
      }));
      break;
    }
    da_append(&orders, order);
    orders.active += 1;
  }

  for (size_t i = 0; i < conveyor_belt.count; i++) {
    conveyor_belt.items[i].position.x += conveyor_belt.speed;
    if (conveyor_belt.items[i].position.x > SCREEN_WIDTH) {
      memmove(conveyor_belt.items, &conveyor_belt.items[1], sizeof(ConveyorBeltIngredient)*conveyor_belt.count-1);
      conveyor_belt.count -= 1;
    }
  }
  // TODO: use actual ingredient size
  assert(false && "TODO");
  if (conveyor_belt.count == 0 || da_last(&conveyor_belt).position.x > CONVEYOR_BELT_INGREDIENT_GAP) {
    Toppings required_toppings = {0};
    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      for (size_t j = 0; j < orders.items[rotators[i].order_index].requested_toppings.count; j++){
        da_append(&required_toppings, ((Topping){.type = orders.items[rotators[i].order_index].requested_toppings.items[j].type}));

      }
    }
    int required_toppings_index = rand() % required_toppings.count;
    assert(false && "TODO");
    da_append(&conveyor_belt, ((ConveyorBeltIngredient){
      .type = required_toppings.items[required_toppings_index].type,
    // TODO: use correct middle y of conveyor belt
      .position = {.x = 0, .y = 40},
    }));

    da_free(required_toppings);
  }

  for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
    if (rotators[i].active && rotators[i].spinning) {
      pizzas.items[rotators[i].pizza_index].rotation += rotators[i].rotation_speed * dt;
    }
  }

  if (topping_selected == TOPPING_NONE && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    for (size_t i = 0; i < conveyor_belt.count; i++) {
      assert(false && "TODO");
      // TODO: use actual ingredient pixel size for comparing mouse position with ingredient position
      if (mouse_pos.x < conveyor_belt.items[i].position.x + 18 && mouse_pos.x > conveyor_belt.items[i].position.x && mouse_pos.y < conveyor_belt.items[i].position.y + 18 && mouse_pos.y > conveyor_belt.items[i].position.y) {
        topping_selected = conveyor_belt.items[i].type;
        conveyor_belt.items[i].type = TOPPING_NONE;
        break;
      }
    }
  }

  if (topping_selected != TOPPING_NONE && IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      if (!rotators[i].active && rotators[i].spinning) continue;
      if (!CheckCollisionPointCircle(mouse_pos, rotators[i].position, PIZZA_RADIUS)) continue;
      Vector2 triangle_verts[3] = {
        rotators[i].position,
        {
          .x = rotators[i].position.x - PIZZA_RADIUS*tan(PI/pizzas.items[rotators[i].pizza_index].number_of_slices),
          .y = rotators[i].position.y-PIZZA_RADIUS
        },
        {
          .x = rotators[i].position.x + PIZZA_RADIUS*tan(PI/pizzas.items[rotators[i].pizza_index].number_of_slices),
          .y = rotators[i].position.y-PIZZA_RADIUS
        },
      };
      if (!CheckCollisionPointTriangle(mouse_pos, triangle_verts[0], triangle_verts[1], triangle_verts[2])) continue;

      Topping topping = {
        .type = topping_selected,
        .offset_from_center = Vector2Distance(mouse_pos, rotators[i].position),
        .rotation = atan2f(mouse_pos.y - rotators[i].position.y,
                        mouse_pos.x - rotators[i].position.x) - pizzas.items[rotators[i].pizza_index].rotation,
        .requested_position = TOPPING_POSITION_NONE
      }; 
      da_append(&pizzas.items[rotators[i].pizza_index].toppings, topping);
    }
    topping_selected = TOPPING_NONE;
  }

	BeginDrawing(); {
    ClearBackground(WHITE);
    DrawTexture(backgroundTex, 0, 0, WHITE);

    // Conveyor belt
    for (size_t i = 0; i < conveyor_belt.count; i++) {
      // TODO: use actual textures for drawing the ingrediets
      Color topping_color = BLANK;
      switch (conveyor_belt.items[i].type) {
        case TOPPING_NONE: break;
        case TOPPING_MUSHROOM: topping_color = BEIGE; break;
        case TOPPING_OLIVE: topping_color = DARKGREEN; break;
        case TOPPING_PEPPERONI: topping_color = MAROON; break;
        case TOPPING_BELL_PEPPER: assert(false && "TODO"); break;
        case TOPPING_CORN: assert(false && "TODO"); break;
        case TOPPING_FETA: assert(false && "TODO"); break;
        case TOPPING_SPINACH: assert(false && "TODO"); break;
        default: assert(false && "unknown topping to render");
      }
      DrawRectangle(conveyor_belt.items[i].position.x-18/2, conveyor_belt.items[i].position.y-18/2, 18, 18, topping_color);
      DrawRectangleLines(conveyor_belt.items[i].position.x-18/2, conveyor_belt.items[i].position.y-18/2, 18, 18, BLACK);
    }

    // Flag
    


 

    // RPM controller

    int topY =  817;
    int bottomY = 1025;
    float minSpeed = 31.415 * (PI / 180);
    float maxSpeed = minSpeed * 4;


    int leftY = topY + ((rotators[0].rotation_speed - minSpeed) / (maxSpeed - minSpeed)) * (bottomY - topY);
    int rightY = topY + ((rotators[1].rotation_speed - minSpeed) / (maxSpeed - minSpeed)) * (bottomY - topY);
    //DrawTexture(speedControllerTex, 845, leftY, WHITE);
    //DrawTexture(speedControllerTex, 1030, rightY, WHITE);
    DrawTexture(speedControllerTex, 845, leftY, WHITE);
    DrawTexture(speedControllerTex, 1030, rightY, WHITE);


    bool openToDrag = topping_selected == TOPPING_NONE && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    
    if (CheckCollisionPointRec(mouse_pos, (Rectangle){845.0f, (float)leftY, 69.0f, 38.0f}) && openToDrag && !draggingRightCtrl) {
      draggingLeftCtrl = true;
      
    }
    if (CheckCollisionPointRec(mouse_pos, (Rectangle){1030.0f, (float)rightY, 69.0f, 38.0f}) && openToDrag && !draggingRightCtrl) {
      draggingRightCtrl = true;
      
    }
    if (draggingLeftCtrl) {
      
      rotators[0].rotation_speed = ((float)(((mouse_pos.y - 19) - (float)topY) / (float)(bottomY - topY) * (maxSpeed - minSpeed)) + minSpeed);
      rotators[0].rotation_speed = minSpeed + maxSpeed * (round(((rotators[0].rotation_speed - minSpeed) / maxSpeed) * 4) / 4);
      rotators[0].rotation_speed = (((rotators[0].rotation_speed > minSpeed) ? rotators[0].rotation_speed : minSpeed) < maxSpeed) ? ((rotators[0].rotation_speed > minSpeed) ? rotators[0].rotation_speed : minSpeed) : maxSpeed;

      if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        draggingLeftCtrl = false;
      }
    } 
    if (draggingRightCtrl) {
      DrawRectangle(10,10, 30, 30, BLUE);
      rotators[1].rotation_speed = ((float)(((mouse_pos.y - 19) - (float)topY) / (float)(bottomY - topY) * (maxSpeed - minSpeed)) + minSpeed);
      rotators[1].rotation_speed = minSpeed + maxSpeed * (round(((rotators[1].rotation_speed - minSpeed) / maxSpeed) * 4) / 4);
      rotators[1].rotation_speed = (((rotators[1].rotation_speed > minSpeed) ? rotators[1].rotation_speed : minSpeed) < maxSpeed) ? ((rotators[1].rotation_speed > minSpeed) ? rotators[1].rotation_speed : minSpeed) : maxSpeed;

      
      if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        draggingRightCtrl = false;
      }
    }

    for (size_t i = 0; i < pizzas.count; i ++) {
      if (pizzas.items[i].delivered) continue;
      BeginTextureMode(pizzas.items[i].render_tex); {
        ClearBackground(BLANK);

        DrawCircle(PIZZA_RADIUS, PIZZA_RADIUS, PIZZA_RADIUS, RED);
        for (size_t j = 1; j <= pizzas.items[i].number_of_slices; j++) {
          DrawLine(PIZZA_RADIUS, PIZZA_RADIUS, PIZZA_RADIUS + PIZZA_RADIUS * cosf(j*2*PI/pizzas.items[i].number_of_slices), PIZZA_RADIUS + PIZZA_RADIUS * sinf(j*2*PI/pizzas.items[i].number_of_slices), BLACK);
        }

        for (size_t j = 0; j < pizzas.items[i].toppings.count; j++) {
          Color topping_color = BLANK;
          switch (pizzas.items[i].toppings.items[j].type) {
            case TOPPING_MUSHROOM: topping_color = BEIGE; break;
            case TOPPING_OLIVE: topping_color = DARKGREEN; break;
            case TOPPING_PEPPERONI: topping_color = MAROON; break;
            default: assert(false && "unkown type of topping");
          }
          Rectangle rec = {
            .x = PIZZA_RADIUS + pizzas.items[i].toppings.items[j].offset_from_center * cosf(pizzas.items[i].toppings.items[j].rotation),
            .y = PIZZA_RADIUS + pizzas.items[i].toppings.items[j].offset_from_center * sinf(pizzas.items[i].toppings.items[j].rotation),
            .width = 18,
            .height = 18,
          };
          DrawRectanglePro(rec, (Vector2){.x = 9, .y = 9}, pizzas.items[i].toppings.items[j].rotation * RAD2DEG, topping_color);
        }
      } EndTextureMode();

      DrawTexturePro(
        pizzas.items[i].render_tex.texture,
        (Rectangle){
          0,
          0,
          pizzas.items[i].render_tex.texture.width,
          -pizzas.items[i].render_tex.texture.height
        },
        (Rectangle){
          pizzas.items[i].position.x,
          pizzas.items[i].position.y,
          pizzas.items[i].render_tex.texture.width,
          pizzas.items[i].render_tex.texture.height
        },
        (Vector2){
            pizzas.items[i].render_tex.texture.width/2.0f,
            pizzas.items[i].render_tex.texture.height/2.0f
        },
        pizzas.items[i].rotation * RAD2DEG,
        WHITE
      );
    }

    for (size_t i = 0; i < ARRAY_LEN(orderTickets); i++) {
      BeginTextureMode(orderTickets[i].render_tex); {
        DrawTexture(blankOrderTicketTex, 0, 0, WHITE);
        if (rotators[i].active) {
          size_t order_index = rotators[i].order_index;
          Order order = orders.items[order_index];
          Vector2 position = {orderTickets[i].position.x + 5, orderTickets[i].position.y + 100};
          for (size_t j = 0; j < order.requested_toppings.count; j++) {
            // TODO: render the ingredient name and the positioning
            Topping topping = order.requested_toppings.items[i];
            switch (topping.requested_position) {
              case TOPPING_POSITION_NONE: assert(false && "topping positioning shouldn't be none inside the order");
              case TOPPING_POSITION_FULL: {

              } break;
              case TOPPING_POSITION_HALF1: {

              } break;
              case TOPPING_POSITION_HALF2: {

              } break;
              case TOPPING_POSITION_ALTERNATE1: {

              } break;
              case TOPPING_POSITION_ALTERNATE2: {

              } break;
              default: assert(false && "unkown topping type in order");
            }

            switch (topping.type) {
              case TOPPING_NONE: assert(false && "topping type shouldn't be none inside the order");
              case TOPPING_MUSHROOM: {} break;
              case TOPPING_OLIVE: {} break;
              case TOPPING_PEPPERONI: {} break;
              case TOPPING_BELL_PEPPER: {} break;
              case TOPPING_CORN: {} break;
              case TOPPING_FETA: {} break;
              case TOPPING_SPINACH: {} break;
              default: assert(false && "unkown topping type in order");
            }
            position.y += 50;
          }
        }
      } EndTextureMode();
      DrawTexturePro(
        orderTickets[i].render_tex.texture,
        (Rectangle){
          0,
          0,
          orderTickets[i].render_tex.texture.width,
          -orderTickets[i].render_tex.texture.height
        },
        (Rectangle){
          orderTickets[i].position.x,
          orderTickets[i].position.y,
          orderTickets[i].render_tex.texture.width,
          orderTickets[i].render_tex.texture.height
        },
        (Vector2){0, 0},
        0,
        WHITE
      );
    }

    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      DrawCircleSectorLines((Vector2){rotators[i].position.x, rotators[i].position.y}, PIZZA_RADIUS, 270-180/pizzas.items[rotators[i].pizza_index].number_of_slices, 270+180/pizzas.items[rotators[i].pizza_index].number_of_slices, 100, YELLOW);
    }

    if (topping_selected != TOPPING_NONE) {
      Color topping_color = BLANK;
      switch (topping_selected) {
        case TOPPING_NONE: break;
        case TOPPING_MUSHROOM: topping_color = BEIGE; break;
        case TOPPING_OLIVE: topping_color = DARKGREEN; break;
        case TOPPING_PEPPERONI: topping_color = MAROON; break;
        default: assert(false && "unknown topping to render");
      }
      DrawRectangle(mouse_pos.x-18/2, mouse_pos.y-18/2, 18, 18, topping_color);
      DrawRectangleLines(mouse_pos.x-18/2, mouse_pos.y-18/2, 18, 18, BLACK);
    }
  } EndDrawing();
}
