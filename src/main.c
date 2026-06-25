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
  __topping_type_count
} ToppingType;

#define TOPPING_POSITION_ICON_SCALE 1.5

typedef enum {
  TOPPING_POSITION_NONE,
  TOPPING_POSITION_FULL,
  TOPPING_POSITION_HALF1,
  TOPPING_POSITION_HALF2,
  TOPPING_POSITION_ALTERNATE1,
  TOPPING_POSITION_ALTERNATE2,
  __topping_position_count
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

#define PIZZA_RADIUS 236
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

#define CONVEYOR_BELT_INGREDIENT_GAP 400

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

Texture2D pizzaBaseTex;

#define INGREDIENT_SCALE 0.3
Texture2D toppingsTex[__topping_type_count];

Texture2D toppingPositionIconsTex[__topping_position_count];

Texture2D tossButtonImg;
Texture2D serverButtonImg;

Texture2D foregroundBelt;
Texture2D conveyorBeltFrames[3];
int currentBeltFrame = 0;
float beltTimer = 0;

Texture2D gameTimePointer;

Texture2D blankOrderTicketTex;
OrderTicket orderTickets[2];

bool draggingLeftCtrl = false;
bool draggingRightCtrl = false;

Font summer_font;

float ptrRotation = 0;

size_t score = 0;
char* scoreStr = NULL;

static void UpdateDrawFrame(void);
void UpdateScore(Pizza pizza, Order order);

int main()
{
  srand(time(NULL));
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spinzza");

  asprintf(&scoreStr, "0");

  summer_font = LoadFont(SUMMER_FONT);

  backgroundTex = LoadTexture(BACKGROUND_IMG);
  speedControllerTex = LoadTexture(SPEED_CONTROLLER_IMG);
  blankOrderTicketTex = LoadTexture(ORDER_TICKET_BLANK_IMG);

  foregroundBelt = LoadTexture(BELT_FOREGROUND);
  conveyorBeltFrames[0] = LoadTexture(BELT_FRAME_1);
  conveyorBeltFrames[1] = LoadTexture(BELT_FRAME_2);
  conveyorBeltFrames[2] = LoadTexture(BELT_FRAME_3);

  orderTickets[0].render_tex = LoadRenderTexture(blankOrderTicketTex.width, blankOrderTicketTex.height);
  orderTickets[0].position = (Vector2){.x = 12, .y = 520};
  orderTickets[1].render_tex = LoadRenderTexture(blankOrderTicketTex.width, blankOrderTicketTex.height);
  orderTickets[1].position = (Vector2){.x = 1734, .y = 520};

  conveyor_belt.speed = EARLY_PHASE_CONVEYOR_BELT_SPEED;

  pizzaBaseTex = LoadTexture(PIZZA_BASE_IMG);

  toppingsTex[TOPPING_MUSHROOM] = LoadTexture(MUSHROOM_IMG);
  toppingsTex[TOPPING_OLIVE] = LoadTexture(OLIVE_IMG);
  toppingsTex[TOPPING_PEPPERONI] = LoadTexture(PEPPERONI_IMG);
  toppingsTex[TOPPING_BELL_PEPPER] = LoadTexture(BELL_PEPPER_IMG);
  toppingsTex[TOPPING_CORN] = LoadTexture(CORN_IMG);
  toppingsTex[TOPPING_FETA] = LoadTexture(FETA_IMG);
  toppingsTex[TOPPING_SPINACH] = LoadTexture(SPINACH_IMG);

  toppingPositionIconsTex[TOPPING_POSITION_FULL] = LoadTexture(PIZZA_ICON_FULL_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_HALF1] = LoadTexture(PIZZA_ICON_TOP_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_HALF2] = LoadTexture(PIZZA_ICON_BOTTOM_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_ALTERNATE1] = LoadTexture(PIZZA_ICON_ALTERNATING_1_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_ALTERNATE2] = LoadTexture(PIZZA_ICON_ALTERNATING_2_IMG);

  tossButtonImg = LoadTexture(TOSS_BUTTON_IMG);
  serverButtonImg = LoadTexture(SERVER_BUTTON_IMG);

  gameTimePointer = LoadTexture(GAME_TIME_POINTER);

  for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
    rotators[i].rotation_speed = PIZZA_BASE_ROTATION_SPEED;
  }
  rotators[0].position = (Vector2){
    .x = 503,
    .y = 795
  };
  rotators[1].position = (Vector2){
    .x = 1415,
    .y = 795
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
      if (time  - start_time >= MIDDLE_PHASE_TIME) {
        game_phase = END_PHASE;
        conveyor_belt.speed = END_PHASE_CONVEYOR_BELT_SPEED;
        break;
      }

    } break;
    case END_PHASE: {
      if (time - start_time >= END_PHASE_TIME) {
        game_phase = RESULTS_DISPLAY;
        max_active_orders = 0;
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

  if (
    orders.active > 0 &&
    (conveyor_belt.count == 0 
    || da_last(&conveyor_belt).position.x + toppingsTex[da_last(&conveyor_belt).type].width/2 > CONVEYOR_BELT_INGREDIENT_GAP)
  ) {
    Toppings required_toppings = {0};
    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      if (!rotators[i].active) continue;
      for (size_t j = 0; j < orders.items[rotators[i].order_index].requested_toppings.count; j++){
        da_append(&required_toppings, ((Topping){.type = orders.items[rotators[i].order_index].requested_toppings.items[j].type}));
      }
    }
    int required_toppings_index = rand() % required_toppings.count;
    da_append(&conveyor_belt, ((ConveyorBeltIngredient){
      .type = required_toppings.items[required_toppings_index].type,
      .position = {.x = 0, .y = 120 - toppingsTex[required_toppings.items[required_toppings_index].type].height/2 * INGREDIENT_SCALE},
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
      ConveyorBeltIngredient ingredient = conveyor_belt.items[i];
      if (
        mouse_pos.x < ingredient.position.x + toppingsTex[ingredient.type].width * INGREDIENT_SCALE 
        && mouse_pos.x > ingredient.position.x 
        && mouse_pos.y < ingredient.position.y + toppingsTex[ingredient.type].height * INGREDIENT_SCALE 
        && mouse_pos.y > conveyor_belt.items[i].position.y 
        && (mouse_pos.x < 805 || mouse_pos.x > 1115)
        && mouse_pos.x > 135
        && mouse_pos.x < 1920 - 135
      ) {
        topping_selected = conveyor_belt.items[i].type;
        conveyor_belt.items[i].type = TOPPING_NONE;
        break;
      }
    }
  }

  if (topping_selected != TOPPING_NONE && IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      if (!rotators[i].active && !rotators[i].spinning) continue;
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

  ptrRotation++;

	BeginDrawing(); {
    ClearBackground(WHITE);
    
    DrawTexture(backgroundTex, 0, 0, WHITE);

    beltTimer += dt;
    if (beltTimer >= 1.0f/6.0f) {
      beltTimer = 0;
      currentBeltFrame = (currentBeltFrame + 1) % 3;
    }
    DrawTexture(conveyorBeltFrames[currentBeltFrame], 0, 30, WHITE);

    DrawTexture(tossButtonImg, 105, 977, WHITE);
    DrawTexture(serverButtonImg, 21, 977, WHITE);
    DrawTexture(tossButtonImg, 1829, 977, WHITE);
    DrawTexture(serverButtonImg, 1745, 977, WHITE);

    Rectangle leftTossButton = {105, 977, 78, 78};
    Rectangle rightTossButton = {1829, 977, 78, 78};
    Rectangle leftServeButton = {21, 977, 78, 78};
    Rectangle rightServeButton = {1745, 977, 78, 78};

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, leftTossButton)) {
      if (rotators[0].active) {
        pizzas.items[rotators[0].pizza_index].rotation = 0;
        pizzas.items[rotators[0].pizza_index].toppings.count = 0;
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, rightTossButton)) {
      if (rotators[1].active) {
        pizzas.items[rotators[1].pizza_index].rotation = 0;
        pizzas.items[rotators[1].pizza_index].toppings.count = 0;
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, leftServeButton)) {
      if (rotators[0].active) {
        rotators[0].spinning = false;
        rotators[0].active = false;
        orders.items[rotators[0].order_index].completed = true;
        orders.items[rotators[0].order_index].deliver_time = time;
        orders.active -= 1;
        pizzas.items[rotators[0].pizza_index].delivered = true;
        UpdateScore(pizzas.items[rotators[0].pizza_index], orders.items[rotators[0].order_index]);
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, rightServeButton)) {
      if (rotators[1].active) {
        rotators[1].spinning = false;
        rotators[1].active = false;
        orders.items[rotators[1].order_index].completed = true;
        orders.items[rotators[1].order_index].deliver_time = time;
        orders.active -= 1;
        pizzas.items[rotators[1].pizza_index].delivered = true;
        UpdateScore(pizzas.items[rotators[1].pizza_index], orders.items[rotators[1].order_index]);
      }
    }

    // Conveyor belt
    for (size_t i = 0; i < conveyor_belt.count; i++) {
      Texture2D toppingTexture = toppingsTex[conveyor_belt.items[i].type];
      float targetWidth = toppingTexture.width * INGREDIENT_SCALE;
      float targetHeight = toppingTexture.height * INGREDIENT_SCALE;
      Rectangle targetRect = {conveyor_belt.items[i].position.x, conveyor_belt.items[i].position.y, targetWidth, targetHeight};
      DrawTexturePro(toppingTexture, (Rectangle){0,0, toppingTexture.width, toppingTexture.height}, targetRect, (Vector2){0,0}, 0, WHITE);
      
    }

    DrawTexture(foregroundBelt, 0, 0, WHITE);

    DrawTexturePro(gameTimePointer, (Rectangle){0,0,27,95},(Rectangle){960, 148, 27, 95}, (Vector2){13, 80}, ptrRotation, WHITE);

    DrawText(scoreStr, 852, 619, 70, YELLOW);

    // RPM controller

    int topY =  840;
    int bottomY = 1010;
    float minSpeed = 31.415 * (PI / 180);
    float maxSpeed = minSpeed * 4;


    int leftY = topY + ((rotators[0].rotation_speed - minSpeed) / (maxSpeed - minSpeed)) * (bottomY - topY);
    int rightY = topY + ((rotators[1].rotation_speed - minSpeed) / (maxSpeed - minSpeed)) * (bottomY - topY);
    //DrawTexture(speedControllerTex, 845, leftY, WHITE);
    //DrawTexture(speedControllerTex, 1030, rightY, WHITE);
    DrawTexture(speedControllerTex, 826, leftY, WHITE);
    DrawTexture(speedControllerTex, 1018, rightY, WHITE);

    bool openToDrag = topping_selected == TOPPING_NONE && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    
    if (CheckCollisionPointRec(mouse_pos, (Rectangle){826.0f, (float)leftY, 69.0f, 38.0f}) && openToDrag && !draggingRightCtrl) {
      draggingLeftCtrl = true;
      
    }
    if (CheckCollisionPointRec(mouse_pos, (Rectangle){1018.0f, (float)rightY, 69.0f, 38.0f}) && openToDrag && !draggingRightCtrl) {
      draggingRightCtrl = true;
      
    }
    if (draggingLeftCtrl) {
      if (rotators[0].active && rotators[0].spinning) {
        rotators[0].rotation_speed = ((float)(((mouse_pos.y - 19) - (float)topY) / (float)(bottomY - topY) * (maxSpeed - minSpeed)) + minSpeed);
        rotators[0].rotation_speed = minSpeed + maxSpeed * (round(((rotators[0].rotation_speed - minSpeed) / maxSpeed) * 4) / 4);
        rotators[0].rotation_speed = (((rotators[0].rotation_speed > minSpeed) ? rotators[0].rotation_speed : minSpeed) < maxSpeed) ? ((rotators[0].rotation_speed > minSpeed) ? rotators[0].rotation_speed : minSpeed) : maxSpeed;

        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          draggingLeftCtrl = false;
        }
      }
    } 
    if (draggingRightCtrl) {
      if (rotators[1].active && rotators[1].spinning) {
       
        rotators[1].rotation_speed = ((float)(((mouse_pos.y - 19) - (float)topY) / (float)(bottomY - topY) * (maxSpeed - minSpeed)) + minSpeed);
        rotators[1].rotation_speed = minSpeed + maxSpeed * (round(((rotators[1].rotation_speed - minSpeed) / maxSpeed) * 4) / 4);
        rotators[1].rotation_speed = (((rotators[1].rotation_speed > minSpeed) ? rotators[1].rotation_speed : minSpeed) < maxSpeed) ? ((rotators[1].rotation_speed > minSpeed) ? rotators[1].rotation_speed : minSpeed) : maxSpeed;

        
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          draggingRightCtrl = false;
        }
      }
    }

    for (size_t i = 0; i < pizzas.count; i ++) {
      if (pizzas.items[i].delivered) continue;
      BeginTextureMode(pizzas.items[i].render_tex); {
        ClearBackground(BLANK);

        DrawTexture(pizzaBaseTex, 0, 0, WHITE);
        for (size_t j = 1; j <= pizzas.items[i].number_of_slices; j++) {
          DrawLine(PIZZA_RADIUS, PIZZA_RADIUS, PIZZA_RADIUS + PIZZA_RADIUS * cosf(j*2*PI/pizzas.items[i].number_of_slices), PIZZA_RADIUS + PIZZA_RADIUS * sinf(j*2*PI/pizzas.items[i].number_of_slices), BLACK);
        }

         for (size_t j = 0; j < pizzas.items[i].toppings.count; j++) {
          Texture2D toppingTexture = toppingsTex[(assert(pizzas.items[i].toppings.items[j].type != TOPPING_NONE), pizzas.items[i].toppings.items[j].type)];
          float targetWidth = toppingTexture.width * INGREDIENT_SCALE;
          float targetHeight = toppingTexture.height * INGREDIENT_SCALE;
          Rectangle rec = {
            .x = PIZZA_RADIUS + pizzas.items[i].toppings.items[j].offset_from_center * cosf(pizzas.items[i].toppings.items[j].rotation),
            .y = PIZZA_RADIUS + pizzas.items[i].toppings.items[j].offset_from_center * sinf(pizzas.items[i].toppings.items[j].rotation),
            .width = targetWidth,
            .height = targetHeight,
          };
          DrawTexturePro(toppingTexture, (Rectangle){0,0, toppingTexture.width, toppingTexture.height},rec, (Vector2){.x = targetWidth / 2, .y = targetHeight / 2}, pizzas.items[i].toppings.items[j].rotation * RAD2DEG, WHITE);
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
          Vector2 position = {20, 150};
          for (size_t j = 0; j < order.requested_toppings.count; j++) {
            Topping topping = order.requested_toppings.items[j];
            ToppingPosition topping_position = topping.requested_position;

            DrawTextureEx(
              toppingPositionIconsTex[(assert(topping_position != TOPPING_POSITION_NONE && topping_position < __topping_position_count && "topping positioning is either none or unkown"), topping_position)],
              position,
              0,
              TOPPING_POSITION_ICON_SCALE,
              WHITE
            );

            Texture2D tex = toppingsTex[(assert(topping.type != TOPPING_NONE && topping.type < __topping_type_count && "topping type is either none or unkown inside order"), topping.type)];
            float scale = 0.3;
            switch(topping.type) {
              case TOPPING_MUSHROOM: scale = 0.25; break;
              case TOPPING_BELL_PEPPER: scale = 0.2; break;
              default: break;
            }
            DrawTexturePro(
              tex,
              (Rectangle){
                0,
                0,
                tex.width,
                tex.height
              },
              (Rectangle){
                position.x + 70,
                position.y,
                tex.width * scale,
                tex.height * scale
              },
              (Vector2){0, 0},
              0,
              WHITE
            );
            position.y += 100;
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
      if (rotators[i].active)
        DrawCircleSectorLines((Vector2){rotators[i].position.x, rotators[i].position.y}, PIZZA_RADIUS, 270-180/pizzas.items[rotators[i].pizza_index].number_of_slices, 270+180/pizzas.items[rotators[i].pizza_index].number_of_slices, 100, YELLOW);
    }

    if (topping_selected != TOPPING_NONE) {
      Texture2D toppingTexture = toppingsTex[(assert(topping_selected != TOPPING_NONE), topping_selected)];
      float targetWidth = toppingTexture.width * INGREDIENT_SCALE;
      float targetHeight = toppingTexture.height * INGREDIENT_SCALE;
      Rectangle targetRect = {mouse_pos.x - targetWidth / 2, mouse_pos.y - targetHeight / 2, targetWidth, targetHeight};
      DrawTexturePro(toppingTexture, (Rectangle){0,0, toppingTexture.width, toppingTexture.height}, targetRect, (Vector2){0,0}, 0, WHITE);
    }
  } EndDrawing();
}

void UpdateScore(Pizza pizza, Order order) {

  // Toppings toppings_per_slice[pizza.number_of_slices];
  unsigned char toppings_present_on_slice[__topping_type_count];
  for (size_t i = 0; i < __topping_type_count; i++) {
    toppings_present_on_slice[i] = 0;
  }
  for (size_t i = 0; i < pizza.toppings.count; i++) {
    float angle = fmodf(pizza.toppings.items[i].rotation, 2*PI);
    if (angle < 0) angle += 2*PI;
    int slice = (int)(angle * pizza.number_of_slices / (2*PI));

    toppings_present_on_slice[pizza.toppings.items[i].type] |= 1 << slice;
  }

  unsigned char half_slice_comparators[] = {0b00111000, 0b00011100, 0b00001110, 0b00000111, 0b00100011, 0b00110001};
  unsigned char alternate_slice_comparators[] = {0b00101010, 0b00010101};

  for (size_t i = 0; i < order.requested_toppings.count; i++) {
    Topping current_topping = order.requested_toppings.items[i];

    switch (current_topping.requested_position) {
      case TOPPING_POSITION_NONE: assert(false && "topping position in order should not be none");
      case TOPPING_POSITION_FULL: {
        for (unsigned char j = 1; j < (1 << pizza.number_of_slices); j = j << 1) {
          if ((toppings_present_on_slice[current_topping.type] & j) != 0) {
            score += 1;
          }
        }
      } break;
      case TOPPING_POSITION_HALF1: {
        // check other topping which is requested
        size_t other_order = i == 0 ? 1 : 0;
        if (order.requested_toppings.items[other_order].requested_position == TOPPING_POSITION_HALF2) {
          if ((toppings_present_on_slice[current_topping.type] & toppings_present_on_slice[other_order]) == 0) {
            bool perfect_half = false;
            for (size_t j = 0; j < ARRAY_LEN(half_slice_comparators); j++) {
              if ((half_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == half_slice_comparators[j]) {
                perfect_half = true;
                break;
              }
            }
            if (perfect_half) {
              score += pizza.number_of_slices / 2;
              score += 2;
            } else {
              // TODO: what is score if the half isn't perfect, that is there are less than half the slices with the topping or they are not continuos
              score += 1;
            }
          } else {
            bool perfect_half = false;
            for (size_t j = 0; j < ARRAY_LEN(half_slice_comparators); j++) {
              if ((half_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == half_slice_comparators[j]) {
                perfect_half = true;
                break;
              }
            }
            if (perfect_half) {
              // TODO: what is score if half is perfect but intersecting with other topping which is half2
              score += pizza.number_of_slices / 2 - 1;
              score += 1;
            } else {
              // TODO: what is score if the half isn't perfect, that is there are less than half the slices with the topping or they are not continuos and it's intersecting with half2
              score += 1;
            }
          }
        } else {
            bool perfect_half = false;
            for (size_t j = 0; j < ARRAY_LEN(half_slice_comparators); j++) {
              if ((half_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == half_slice_comparators[j]) {
                perfect_half = true;
                break;
              }
            }
            if (perfect_half) {
              score += pizza.number_of_slices;
              score += 1;
            } else {
              // TODO: what is score if the half isn't perfect, that is there are less than half the slices with the topping or they are not continuos
            }
        }
      } break;
      case TOPPING_POSITION_HALF2: {
        // check other topping which is requested
        size_t other_order = i == 0 ? 1 : 0;
        if (order.requested_toppings.items[other_order].requested_position == TOPPING_POSITION_HALF1) {
          if ((toppings_present_on_slice[current_topping.type] & toppings_present_on_slice[order.requested_toppings.items[other_order].type]) == 0) {
            bool perfect_half = false;
            for (size_t j = 0; j < ARRAY_LEN(half_slice_comparators); j++) {
              if ((half_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == half_slice_comparators[j]) {
                perfect_half = true;
                break;
              }
            }
            if (perfect_half) {
              score += pizza.number_of_slices / 2;
              score += 2;
            } else {
              // TODO: what is score if the half isn't perfect, that is there are less than half the slices with the topping or they are not continuos
              score += 1;
            }
          } else {
            bool perfect_half = false;
            for (size_t j = 0; j < ARRAY_LEN(half_slice_comparators); j++) {
              if ((half_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == half_slice_comparators[j]) {
                perfect_half = true;
                break;
              }
            }
            if (perfect_half) {
              // TODO: what is score if half is perfect but intersecting with other topping which is half2
              score += pizza.number_of_slices / 2 - 1;
              score += 1;
            } else {
              // TODO: what is score if the half isn't perfect, that is there are less than half the slices with the topping or they are not continuos and it's intersecting with half2
              score += 1;
            }
          }
        } else {
            bool perfect_half = false;
            for (size_t j = 0; j < ARRAY_LEN(half_slice_comparators); j++) {
              if ((half_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == half_slice_comparators[j]) {
                perfect_half = true;
                break;
              }
            }
            if (perfect_half) {
              score += pizza.number_of_slices / 2;
              score += 1;
            } else {
              // TODO: what is score if the half isn't perfect, that is there are less than half the slices with the topping or they are not continuos
            }
        }
      } break;

      case TOPPING_POSITION_ALTERNATE1: {
        size_t other_order = i == 0 ? 1 : 0;
        if (order.requested_toppings.items[other_order].requested_position == TOPPING_POSITION_ALTERNATE2) {
          if ((toppings_present_on_slice[current_topping.type] & toppings_present_on_slice[order.requested_toppings.items[other_order].type]) == 0) {
            bool perfect_alternating = false;
            for (size_t j = 0; j < ARRAY_LEN(alternate_slice_comparators); j++) {
              if ((alternate_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == alternate_slice_comparators[j]) {
                perfect_alternating = true;
                break;
              }
            }
            if (perfect_alternating) {
              score += pizza.number_of_slices / 2;
              score += 2;
            } else {
              // TODO: what is score if alternating slices are not perfect
              score += 1;
            }
          } else {
            bool perfect_alternating = false;
            for (size_t j = 0; j < ARRAY_LEN(alternate_slice_comparators); j++) {
              if ((alternate_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == alternate_slice_comparators[j]) {
                perfect_alternating = true;
                break;
              }
            }
            if (perfect_alternating) {
              // TODO: what is score if alternating slices are perfect but it's intersecting with other alternate toppings
              score += pizza.number_of_slices / 2 - 1;
              score += 1;
            } else {
              // TODO: what is score if alternating slices are not perfect and it's intersecting with other alternate toppings
              score += 1;
            }
          }
        } else {
            bool perfect_alternating = false;
            for (size_t j = 0; j < ARRAY_LEN(alternate_slice_comparators); j++) {
              if ((alternate_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == alternate_slice_comparators[j]) {
                perfect_alternating = true;
                break;
              }
            }
            if (perfect_alternating) {
              score += pizza.number_of_slices / 2;
              score += 1;
            } else {
              // TODO: what is score if alternating slices are not perfect
            }
        }
      } break;
      case TOPPING_POSITION_ALTERNATE2: {
        size_t other_order = i == 0 ? 1 : 0;
        if (order.requested_toppings.items[other_order].requested_position == TOPPING_POSITION_ALTERNATE1) {
          if ((toppings_present_on_slice[current_topping.type] & toppings_present_on_slice[order.requested_toppings.items[other_order].type]) == 0) {
            bool perfect_alternating = false;
            for (size_t j = 0; j < ARRAY_LEN(alternate_slice_comparators); j++) {
              if ((alternate_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == alternate_slice_comparators[j]) {
                perfect_alternating = true;
                break;
              }
            }
            if (perfect_alternating) {
              score += pizza.number_of_slices / 2;
              score += 2;
            } else {
              // TODO: what is score if alternating slices are not perfect
              score += 1;
            }
          } else {
            bool perfect_alternating = false;
            for (size_t j = 0; j < ARRAY_LEN(alternate_slice_comparators); j++) {
              if ((alternate_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == alternate_slice_comparators[j]) {
                perfect_alternating = true;
                break;
              }
            }
            if (perfect_alternating) {
              // TODO: what is score if alternating slices are perfect but it's intersecting with other alternate toppings
              score += pizza.number_of_slices / 2 - 1;
              score += 1;
            } else {
              // TODO: what is score if alternating slices are not perfect and it's intersecting with other alternate toppings
              score += 1;
            }
          }
        } else {
            bool perfect_alternating = false;
            for (size_t j = 0; j < ARRAY_LEN(alternate_slice_comparators); j++) {
              if ((alternate_slice_comparators[j] & toppings_present_on_slice[current_topping.type]) == alternate_slice_comparators[j]) {
                perfect_alternating = true;
                break;
              }
            }
            if (perfect_alternating) {
              score += pizza.number_of_slices / 2;
              score += 1;
            } else {
              // TODO: what is score if alternating slices are not perfect
            }
        }
      } break;
      default: assert(false && "Topping position is unkown in order");
    }
  }

  if (scoreStr != NULL) {
    free(scoreStr);
  }
  asprintf(&scoreStr, "%zu", score);
}
