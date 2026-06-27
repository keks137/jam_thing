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

#define EARLY_PHASE_TIME 40
#define MIDDLE_PHASE_TIME 120
#define END_PHASE_TIME 160
#define TOTAL_GAME_TIME 360
#define FPS 60

#define min(a,b) (((a) < (b)) ? (a) : (b))



typedef enum {
  START_SCREEN,
  TUTORIAL_PHASE,
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
  TOPPING_ONION,
  __topping_type_count
} ToppingType;

#define TOPPING_POSITION_ICON_SCALE 0.6

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
  float speedModifier;
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
  size_t speed_setting;
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

typedef struct {
  const char* text;
  Vector2 position;
  int size;
  int opacity;
} TextEffect;

typedef struct {
  TextEffect* items;
  size_t count;
  size_t capacity;
} TextEffects;

TextEffects textEffects = {0};

#define BASE_SCORE 50

GamePhase game_phase = START_SCREEN;
float start_time = 0;
float phase_start_time = 0;

PizzaRotator rotators[2] = {
  {.rotation_speed = 0, .speed_setting = 0, .active = false, .order_index = 0, .pizza_index = 0, .position = {0}},
  {.rotation_speed = 0, .speed_setting = 0, .active = false, .order_index = 0, .pizza_index = 0, .position = {0}},
};

Pizzas pizzas = {0};
int pizzasFinished = 0;
int pizzasTossed = 0;

ConveyorBelt conveyor_belt = {0};

Orders orders = {0};

ToppingType topping_selected = TOPPING_NONE;

size_t max_active_orders = 1;
size_t max_toppings_per_pizza = 2;

Texture2D backgroundTex;
Texture2D speedControllerTex;

Texture2D pizzaBaseTex;
Texture2D pizzaMaskTex;

#define INGREDIENT_SCALE 0.3
#define STANDARD_INGREDIENT_HITBOX_WIDTH 90
#define STANDARD_INGREDIENT_HITBOX_HEIGHT 90
Texture2D toppingsTex[__topping_type_count];

Texture2D toppingPositionIconsTex[__topping_position_count];

Texture2D tossButtonImg;
Texture2D serverButtonImg;

Texture2D foregroundBelt;
Texture2D robotArmLeft;
Texture2D robotArmRight;

Texture2D leftArmHead;
Texture2D leftArmRod;
Texture2D leftArmBearing;

Texture2D rightArmHead;
Texture2D rightArmRod;
Texture2D rightArmBearing;

Sound tossPizza;
Sound newPizza;
Sound placeIngredient;
Sound changeKnob;

Texture2D startScreen;

Music retroMusic;

Texture2D conveyorBeltFrames[3];
int currentBeltFrame = 0;
float beltTimer = 0;

Texture2D gameTimePointer;
Texture2D tutorialTex;
int tutorialX;
Texture2D scoreboardTex;
float scoreboardAnimationTime = 0;

Texture2D blankOrderTicketTex;
OrderTicket orderTickets[2];

bool draggingLeftCtrl = false;
bool draggingRightCtrl = false;

float leftArmAngle = PI;
float rightArmAngle = 0;

Font summer_font;
Font cheese_font;

float ptrRotation = -117; // 113

size_t score = 0;
bool firstPizzaServed = false;

int startTime;
bool timerActive = false;

char* titles[6] = {
  "Certified Dough Brain! (F)",
  "Pizza Menace! (E)",
  "Kitchen Menace! (D)",
  "Slice Specialist! (C)",
  "Topping Titan! (B)",
  "Superior Spin Master! (A)"
};

bool musicPlaying = false;
bool begin = false;

static void UpdateDrawFrame(void);
void LoadAssets(void);
void UnloadAssets(void);
void UpdateScore(Pizza pizza, Order order, size_t speed_setting);
void UpdateOrders(double time);
void UpdateConveyorBelt(void);
void PickupToppingFromConveyorBelt(Vector2 mouse_pos);
void PlaceToppingOnPizza(Vector2 mouse_pos);
void TossOrServe(Vector2 mouse_pos, double time);
void DrawConveyorBelt(float dt);
void DrawRobotArms(void);
void UpdateAndDrawSpeedControllers(Vector2 mouse_pos);
void DrawPizzas(void);
void DrawOrderTicket(void);
void DrawPickedUpTopping(Vector2 mouse_pos);
void DrawTextEffects(void);
void DrawTutorial(void);
void Reset(void);
void DrawEndScreen(void);
void DrawStartScreen(void);


int main()
{
  srand(time(NULL));
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pizza Panic");

  InitAudioDevice();

  LoadAssets();
  
#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
	SetTargetFPS(FPS);

	while (!WindowShouldClose())
		UpdateDrawFrame();
#endif

  UnloadAssets();

	CloseWindow();

	return 0;
}

static void UpdateDrawFrame(void)
{
  float dt = GetFrameTime();
  Vector2 mouse_pos = GetMousePosition();
  double time = GetTime();
  if (IsMouseButtonDown(0) && !musicPlaying) {
      PlayMusicStream(retroMusic);
      musicPlaying = true;

  }
  switch (game_phase) {
    case START_SCREEN: {
      if (begin) {
        game_phase = TUTORIAL_PHASE;
        phase_start_time = start_time;
      }
      
    } break;
    case TUTORIAL_PHASE: {
      if (firstPizzaServed) {
        game_phase = EARLY_PHASE;
        start_time = time;
        phase_start_time = time;
        max_active_orders = 1;
        conveyor_belt.speed = EARLY_PHASE_CONVEYOR_BELT_SPEED;
      }
    } break;
    case EARLY_PHASE: {
      if (time - phase_start_time >= EARLY_PHASE_TIME) {
        game_phase = MIDDLE_PHASE;
        max_active_orders = 2;
        conveyor_belt.speed = MIDDLE_PHASE_CONVEYOR_BELT_SPEED;
        phase_start_time = time;
      }
    } break;
    case MIDDLE_PHASE: {
      if (time  - phase_start_time >= MIDDLE_PHASE_TIME) {
        game_phase = END_PHASE;
        conveyor_belt.speed = END_PHASE_CONVEYOR_BELT_SPEED;
        phase_start_time = time;
      }
    } break;
    case END_PHASE: {
      if (time - phase_start_time >= END_PHASE_TIME) {
        game_phase = RESULTS_DISPLAY;
        max_active_orders = 0;
      }

    } break;
    case RESULTS_DISPLAY: {

    } break;
  }

  if (!(game_phase == RESULTS_DISPLAY || game_phase == START_SCREEN)) {
    UpdateOrders(time);

    UpdateConveyorBelt();

    // Rotate pizzas
    for (size_t i = 0; i < ARRAY_LEN(rotators); i++) {
      if (rotators[i].active && rotators[i].spinning) {
        pizzas.items[rotators[i].pizza_index].rotation += rotators[i].rotation_speed * pizzas.items[rotators[i].pizza_index].speedModifier * dt;
      }
    }

    PickupToppingFromConveyorBelt(mouse_pos);

    PlaceToppingOnPizza(mouse_pos);

    TossOrServe(mouse_pos, time);

    if (game_phase > TUTORIAL_PHASE && game_phase <= END_PHASE)
      ptrRotation = (((time - start_time) / (float)TOTAL_GAME_TIME) * (113 - -117)) - 117;
  }
  
  if (IsKeyPressed(KEY_R)) {
    game_phase = RESULTS_DISPLAY;
  }

  UpdateMusicStream(retroMusic);

	BeginDrawing(); {
    ClearBackground(WHITE);
    
    DrawTexture(backgroundTex, 0, 0, WHITE);


    DrawTexture(tossButtonImg, 105, 977, WHITE);
    DrawTexture(serverButtonImg, 21, 977, WHITE);
    DrawTexture(tossButtonImg, 1829, 977, WHITE);
    DrawTexture(serverButtonImg, 1745, 977, WHITE);

    DrawConveyorBelt(dt);

    DrawTexturePro(gameTimePointer, (Rectangle){0,0,27,95},(Rectangle){960, 148, 27, 95}, (Vector2){13, 80}, ptrRotation, WHITE);

    int textSize = MeasureText(TextFormat("%d", score), 70);
    DrawText(TextFormat("%d", score), SCREEN_WIDTH / 2 - textSize / 2, 619, 70, YELLOW);

    if (game_phase != RESULTS_DISPLAY && game_phase != START_SCREEN) {
      UpdateAndDrawSpeedControllers(mouse_pos);
    }

    DrawPizzas();

    DrawOrderTicket();

    DrawRobotArms();

    DrawPickedUpTopping(mouse_pos);

    DrawTextEffects();

    DrawTutorial();

    if (game_phase == RESULTS_DISPLAY) {
      DrawEndScreen();
    }
    if (game_phase == START_SCREEN) {
      DrawStartScreen();
    }
  } EndDrawing();
}

void LoadAssets(void) {
  tossPizza = LoadSound(ORDER_OUT_SFX);
  newPizza = LoadSound(ORDER_IN_SFX);
  placeIngredient = LoadSound(PLACE_INGREDIENT_SFX);
  changeKnob = LoadSound(CHANGE_KNOB_SFX);

  retroMusic = LoadMusicStream(RETRO_MUSIC);
  retroMusic.looping = true;
  
  startScreen = LoadTexture(TITLE_SCREEN_IMG);

  summer_font = LoadFont(SUMMER_FONT);
  cheese_font = LoadFontEx(CHEESE_FONT, 150, NULL, 0);
  GenTextureMipmaps(&cheese_font.texture);
  SetTextureFilter(cheese_font.texture, TEXTURE_FILTER_BILINEAR);

  robotArmLeft = LoadTexture(ROBOT_ARM_LEFT_IMG);
  robotArmRight = LoadTexture(ROBOT_ARM_RIGHT_IMG);

  leftArmHead = LoadTexture(LEFT_CLAW_HEAD_IMG);
  leftArmRod = LoadTexture(LEFT_CLAW_ROD_IMG);
  leftArmBearing = LoadTexture(LEFT_CLAW_BEARING_IMG);

  rightArmHead = LoadTexture(RIGHT_CLAW_HEAD_IMG);
  rightArmRod = LoadTexture(RIGHT_CLAW_ROD_IMG);
  rightArmBearing = LoadTexture(RIGHT_CLAW_BEARING_IMG);

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
  pizzaMaskTex = LoadTexture(PIZZA_MASK_IMG);

  toppingsTex[TOPPING_MUSHROOM] = LoadTexture(MUSHROOM_IMG);
  toppingsTex[TOPPING_OLIVE] = LoadTexture(OLIVE_IMG);
  toppingsTex[TOPPING_PEPPERONI] = LoadTexture(PEPPERONI_IMG);
  toppingsTex[TOPPING_BELL_PEPPER] = LoadTexture(BELL_PEPPER_IMG);
  toppingsTex[TOPPING_CORN] = LoadTexture(CORN_IMG);
  toppingsTex[TOPPING_FETA] = LoadTexture(FETA_IMG);
  toppingsTex[TOPPING_SPINACH] = LoadTexture(SPINACH_IMG);
  toppingsTex[TOPPING_ONION] = LoadTexture(ONION_IMG);

  toppingPositionIconsTex[TOPPING_POSITION_FULL] = LoadTexture(PIZZA_ICON_FULL_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_HALF1] = LoadTexture(PIZZA_ICON_TOP_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_HALF2] = LoadTexture(PIZZA_ICON_BOTTOM_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_ALTERNATE1] = LoadTexture(PIZZA_ICON_ALTERNATING_1_IMG);
  toppingPositionIconsTex[TOPPING_POSITION_ALTERNATE2] = LoadTexture(PIZZA_ICON_ALTERNATING_2_IMG);

  tossButtonImg = LoadTexture(TOSS_BUTTON_IMG);
  serverButtonImg = LoadTexture(SERVER_BUTTON_IMG);

  gameTimePointer = LoadTexture(GAME_TIME_POINTER);
  tutorialTex = LoadTexture(TUTORIAL_IMG);
  tutorialX = SCREEN_WIDTH - tutorialTex.width * 1.1;
  scoreboardTex = LoadTexture(SCORE_BOARD_IMG);

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
}

void UnloadAssets(void) {
  UnloadSound(tossPizza);
  UnloadSound(newPizza);
  UnloadSound(placeIngredient);
  UnloadSound(changeKnob);

  UnloadMusicStream(retroMusic);
  
  UnloadTexture(startScreen);

  UnloadFont(summer_font);
  UnloadFont(cheese_font);
  GenTextureMipmaps(&cheese_font.texture);
  SetTextureFilter(cheese_font.texture, TEXTURE_FILTER_BILINEAR);

  UnloadTexture(robotArmLeft);
  UnloadTexture(robotArmRight);

  UnloadTexture(leftArmHead);
  UnloadTexture(leftArmRod);
  UnloadTexture(leftArmBearing);

  UnloadTexture(rightArmHead);
  UnloadTexture(rightArmRod);
  UnloadTexture(rightArmBearing);

  UnloadTexture(backgroundTex);
  UnloadTexture(speedControllerTex);
  UnloadTexture(blankOrderTicketTex);

  UnloadTexture(foregroundBelt);
  UnloadTexture(conveyorBeltFrames[0]);
  UnloadTexture(conveyorBeltFrames[1]);
  UnloadTexture(conveyorBeltFrames[2]);

  UnloadRenderTexture(orderTickets[0].render_tex);
  UnloadRenderTexture(orderTickets[1].render_tex);

  UnloadTexture(pizzaBaseTex);
  UnloadTexture(pizzaMaskTex);

  UnloadTexture(toppingsTex[TOPPING_MUSHROOM]);
  UnloadTexture(toppingsTex[TOPPING_OLIVE]);
  UnloadTexture(toppingsTex[TOPPING_PEPPERONI]);
  UnloadTexture(toppingsTex[TOPPING_BELL_PEPPER]);
  UnloadTexture(toppingsTex[TOPPING_CORN]);
  UnloadTexture(toppingsTex[TOPPING_FETA]);
  UnloadTexture(toppingsTex[TOPPING_SPINACH]);
  UnloadTexture(toppingsTex[TOPPING_ONION]);

  UnloadTexture(toppingPositionIconsTex[TOPPING_POSITION_FULL]);
  UnloadTexture(toppingPositionIconsTex[TOPPING_POSITION_HALF1]);
  UnloadTexture(toppingPositionIconsTex[TOPPING_POSITION_HALF2]);
  UnloadTexture(toppingPositionIconsTex[TOPPING_POSITION_ALTERNATE1]);
  UnloadTexture(toppingPositionIconsTex[TOPPING_POSITION_ALTERNATE2]);

  UnloadTexture(tossButtonImg);
  UnloadTexture(serverButtonImg);

  UnloadTexture(gameTimePointer);
  UnloadTexture(tutorialTex);
  UnloadTexture(scoreboardTex);

  for (size_t i = 0; i < pizzas.count; i++) {
    if (!pizzas.items[i].delivered) UnloadRenderTexture(pizzas.items[i].render_tex);
  }
}

void Reset() {
  pizzasFinished = 0;
  pizzasTossed = 0;
  textEffects.count = 0;
  printf("RESETTING\n");
  begin = false;

  game_phase = START_SCREEN;
  start_time = 0;
  phase_start_time = 0;

  rotators[0] = (PizzaRotator){.rotation_speed = 0, .active = false, .order_index = 0, .pizza_index = 0, .position = {0}};
  rotators[1] = (PizzaRotator){.rotation_speed = 0, .active = false, .order_index = 0, .pizza_index = 0, .position = {0}};
  
  scoreboardAnimationTime = 0;
  pizzas = (Pizzas){0};

  conveyor_belt = (ConveyorBelt){0};

  orders = (Orders){0};

  topping_selected = TOPPING_NONE;

  max_active_orders = 1;
  max_toppings_per_pizza = 2;

  currentBeltFrame = 0;
  beltTimer = 0;

  ptrRotation = -117;
  score = 0;
  firstPizzaServed = false;

  timerActive = false;

  conveyor_belt.speed = EARLY_PHASE_CONVEYOR_BELT_SPEED;

  orderTickets[0].render_tex = LoadRenderTexture(blankOrderTicketTex.width, blankOrderTicketTex.height);
  orderTickets[0].position = (Vector2){.x = 12, .y = 520};
  orderTickets[1].render_tex = LoadRenderTexture(blankOrderTicketTex.width, blankOrderTicketTex.height);
  orderTickets[1].position = (Vector2){.x = 1734, .y = 520};

  
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

  tutorialX = SCREEN_WIDTH - tutorialTex.width * 1.1;
}

float easeOutBounce(float x) {
  const float n1 = 7.5625f;
  const float d1 = 2.75f;

  if (x < 1.0f / d1) {
    return n1 * x * x;
  } else if (x < 2.0f / d1) {
    x -= 1.5f / d1;
    return n1 * x * x + 0.75f;
  } else if (x < 2.5f / d1) {
    x -= 2.25f / d1;
    return n1 * x * x + 0.9375f;
  } else {
    x -= 2.625f / d1;
    return n1 * x * x + 0.984375f;
  }
}

void DrawEndScreen(void) {
  scoreboardAnimationTime = min(1, scoreboardAnimationTime + 1 * GetFrameTime());

  int topY = (easeOutBounce(scoreboardAnimationTime) * (scoreboardTex.height + (SCREEN_HEIGHT - scoreboardTex.height) / 2.0f)) - scoreboardTex.height;
  int topX = (SCREEN_WIDTH - scoreboardTex.width) / 2;
  DrawTexture(scoreboardTex, topX, topY, WHITE);
  
  
  const char* pizzas_finished = TextFormat("Pizzas Finished: %d", pizzasFinished);
  const char* pizzas_tossed = TextFormat("Pizzas Tossed: %d", pizzasTossed);
  const char* final_score = TextFormat("Final Score: %d", score);

  Vector2 finishedSize = MeasureTextEx(cheese_font, pizzas_finished, 40, 3);
  Vector2 tossedSize = MeasureTextEx(cheese_font, pizzas_tossed, 40, 3);
  Vector2 scoreSize = MeasureTextEx(cheese_font, final_score, 40, 3);

  DrawTextEx(cheese_font, pizzas_finished, (Vector2){SCREEN_WIDTH / 2.0f - finishedSize.x / 2, topY + 200}, 40, 3, (Color){109, 61, 37, 255});
  DrawTextEx(cheese_font, pizzas_tossed, (Vector2){SCREEN_WIDTH / 2.0f - tossedSize.x / 2, topY + 240}, 40, 3, (Color){109, 61, 37, 255});
  DrawTextEx(cheese_font, final_score, (Vector2){SCREEN_WIDTH / 2.0f - scoreSize.x / 2, topY + 280}, 40, 3, (Color){109, 61, 37, 255});

  int titleI = 0;
  if (score < 150) {
    titleI = 0;
  } else if (score < 350) {
    titleI = 1;
  } else if (score < 500) {
    titleI = 2;
  }else if (score < 750) {
    titleI = 3;
  }else if (score < 900) {
    titleI = 4;
  }else {
    titleI = 5;
  }

  Vector2 prefaceSize = MeasureTextEx(cheese_font, "You have earned the title: ", 25, 2);
    DrawTextEx(cheese_font, "You have earned the title: ", (Vector2){SCREEN_WIDTH / 2.0f - prefaceSize.x / 2, topY + 380}, 25, 2, (Color){109, 61, 37, 255});


  Vector2 titleSize = MeasureTextEx(cheese_font, titles[titleI], 50, 4);
  DrawTextEx(cheese_font, titles[titleI], (Vector2){SCREEN_WIDTH / 2.0 - titleSize.x / 2, topY + 420}, 50, 4, (Color){109, 61, 37, 255});


  Rectangle resetButton = (Rectangle){SCREEN_WIDTH / 2.0 - scoreboardTex.width / 2.0 + 256, SCREEN_HEIGHT / 2.0 - scoreboardTex.height / 2.0 + 511, 208, 79};
  Vector2 mousePos = GetMousePosition();

  if (CheckCollisionPointRec(mousePos, resetButton) && IsMouseButtonPressed(0)) {
    Reset();
  }
}

void DrawStartScreen(void) {
  DrawTexture(startScreen, 0, 0, WHITE);
  if (IsMouseButtonPressed(0)) {
    begin = true;
  }
}


size_t countSetBits(unsigned char n) {
  size_t bits = 0;
  for (size_t i = 1; i < (1 << 8); i = i << 1) {
    if ((i & n) != 0) bits += 1;
  }
  return bits;
}

float CalculateAccuracy(size_t matched, ToppingPosition pos) {
  switch (pos) {
  case TOPPING_POSITION_FULL: {
    if (matched == 6) return 1.0f;
    if (matched == 5) return 0.85f;
    if (matched == 4) return 0.65f;
    if (matched == 3) return 0.40f;
    return 0.0f;
  } break;
  case TOPPING_POSITION_HALF1:
  case TOPPING_POSITION_HALF2:
  case TOPPING_POSITION_ALTERNATE1:
  case TOPPING_POSITION_ALTERNATE2: {
    if (matched == 3) return 1.0f;
    if (matched == 2) return 0.50f;
    if (matched == 1) return 0.15f;
    return 0.0f;
  } break;
  default: assert(false && "Other topping positions shouldn't be present when updating score");
  }
}

void UpdateScore(Pizza pizza, Order order, size_t speed_setting) {
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

  unsigned char full_comparator = 0b00111111;
  unsigned char half_slice_comparators[] = {0b00111000, 0b00011100, 0b00001110, 0b00000111, 0b00100011, 0b00110001};
  unsigned char alternate_slice_comparators[] = {0b00101010, 0b00010101};

  float pattern_accuracy = 0.0;
  float contanimation_penalty = 0.0;
  float pizza_type_multiplier = 0;

  for (size_t i = 0; i < order.requested_toppings.count; i++) {
    Topping current_topping = order.requested_toppings.items[i];
    if (toppings_present_on_slice[current_topping.type] == 0) continue;

    switch (current_topping.requested_position) {
      case TOPPING_POSITION_NONE: assert(false && "topping position in order should not be none");
      case TOPPING_POSITION_FULL: {
        pizza_type_multiplier += 1.0f;

        float acc = CalculateAccuracy(countSetBits(full_comparator & toppings_present_on_slice[current_topping.type]), current_topping.requested_position);
        pattern_accuracy += acc;
      } break;
      case TOPPING_POSITION_HALF1: 
      case TOPPING_POSITION_HALF2: {
        pizza_type_multiplier += 1.5f;

        size_t matched_pattern = 0;
        float best_acc = 0;
        for (size_t j = 0; j < ARRAY_LEN(half_slice_comparators); j++) {
          unsigned char expected = half_slice_comparators[j];
          unsigned char actual = toppings_present_on_slice[current_topping.type];
          size_t matched = countSetBits(expected & actual);
          float acc = CalculateAccuracy(matched, current_topping.requested_position);
          if (acc > best_acc) {
              best_acc = acc;
              matched_pattern = j;
          }
        }
        pattern_accuracy += best_acc;

        size_t other_topping = i == 0 ? 1 : 0;
        if (i < other_topping && order.requested_toppings.items[other_topping].requested_position != TOPPING_POSITION_FULL) {
          size_t contanimated_slices = countSetBits(half_slice_comparators[matched_pattern] & toppings_present_on_slice[order.requested_toppings.items[other_topping].type]);
          contanimation_penalty += contanimated_slices * 0.1f;
        }
      } break;

      case TOPPING_POSITION_ALTERNATE1: 
      case TOPPING_POSITION_ALTERNATE2: {
        pizza_type_multiplier += 2.0f;

        size_t matched_pattern = 0;
        float best_acc = 0;
        for (size_t j = 0; j < ARRAY_LEN(alternate_slice_comparators); j++) {
          unsigned char expected = alternate_slice_comparators[j];
          unsigned char actual = toppings_present_on_slice[current_topping.type];
          size_t correct = pizza.number_of_slices - countSetBits(expected ^ actual);
          float acc = CalculateAccuracy(correct, current_topping.requested_position);
          if (acc > best_acc) {
              best_acc = acc;
              matched_pattern = j;
          }
        }
        pattern_accuracy += best_acc;
        size_t other_topping = i == 0 ? 1 : 0;
        if (i < other_topping && order.requested_toppings.items[other_topping].requested_position != TOPPING_POSITION_FULL) {
          size_t contanimated_slices = countSetBits(alternate_slice_comparators[matched_pattern] & toppings_present_on_slice[order.requested_toppings.items[other_topping].type]);
          contanimation_penalty += contanimated_slices * 0.1f;
        }
      } break;
      default: assert(false && "Topping position is unkown in order");
    }
  }

  float speed_multiplier = 0;
  switch (speed_setting) {
    case 0: speed_multiplier = 1.0f; break;
    case 1: speed_multiplier = 1.25f; break;
    case 2: speed_multiplier = 1.75f; break;
    case 3: speed_multiplier = 2.0f; break;
    default: assert(false && "unkown speed setting");
  }

  pattern_accuracy = pattern_accuracy / order.requested_toppings.count;
  pizza_type_multiplier /= order.requested_toppings.count;
  float final_accuracy = fmax(0, pattern_accuracy - contanimation_penalty);

  int scoreChange = BASE_SCORE * final_accuracy * pizza_type_multiplier * speed_multiplier;
  da_append(&textEffects, ((TextEffect){
    .text = TextFormat("%d", score),
    .opacity = 255, 
    .position =(Vector2){852, 619 - 30},
    .size = 60
  }));

  score += scoreChange;
}

void UpdateOrders(double time) {
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
    da_append(&available_toppings, ((Topping){.type = TOPPING_ONION}));

    int prob = rand() % 1000 + 1;
    Topping topping = {0};
    switch (game_phase) {
      case TUTORIAL_PHASE: {
        if (prob < 500) {
          topping.requested_position = TOPPING_POSITION_FULL;
        } else {
          topping.requested_position = TOPPING_POSITION_HALF1;
        }
      }
      case EARLY_PHASE: {
        if (prob < 500) {
          topping.requested_position = TOPPING_POSITION_FULL;
        } else {
          topping.requested_position = TOPPING_POSITION_HALF1;
        }
      } break;
      case MIDDLE_PHASE: {
        if (prob < 200) {
          topping.requested_position = TOPPING_POSITION_FULL;
        } else if (prob < 700) {
          topping.requested_position = TOPPING_POSITION_HALF1;
        } else {
          topping.requested_position = TOPPING_POSITION_ALTERNATE1;
        }
      } break;
      case END_PHASE: {
        if (prob < 200) {
          topping.requested_position = TOPPING_POSITION_FULL;
        } else if (prob < 600) {
          topping.requested_position = TOPPING_POSITION_HALF1;
        } else {
          topping.requested_position = TOPPING_POSITION_ALTERNATE1;
        }
      } break;
      default:
        assert(false && "shouldn't add orders in this state");
    }

    size_t available_toppings_index = rand() % available_toppings.count;
    topping.type = available_toppings.items[available_toppings_index].type;
    da_remove_unordered(&available_toppings, available_toppings_index);


    da_append(&order.requested_toppings, topping);

    prob = rand() % 1000 + 1;
    Topping topping2 = {0};
    switch (game_phase) {
      case TUTORIAL_PHASE: {
        if (topping.requested_position == TOPPING_POSITION_HALF1) {
          topping2.requested_position = TOPPING_POSITION_HALF2;
        } else {
          if (prob < 500) {
            topping2.requested_position = TOPPING_POSITION_FULL;
          } else {
            topping2.requested_position = TOPPING_POSITION_HALF1;
          }
        }
      }
      case EARLY_PHASE: {
        if (topping.requested_position == TOPPING_POSITION_HALF1) {
          topping2.requested_position = TOPPING_POSITION_HALF2;
        } else {
          if (prob < 500) {
            topping2.requested_position = TOPPING_POSITION_FULL;
          } else {
            topping2.requested_position = TOPPING_POSITION_HALF1;
          }
        }
      } break;
      case MIDDLE_PHASE: {
        if (topping.requested_position == TOPPING_POSITION_HALF1) {
          topping2.requested_position = TOPPING_POSITION_HALF2;
        } else if (topping.requested_position == TOPPING_POSITION_ALTERNATE1) {
          topping2.requested_position = TOPPING_POSITION_ALTERNATE2;
        } else {
          if (prob < 200) {
            topping2.requested_position = TOPPING_POSITION_FULL;
          } else if (prob < 700) {
            topping2.requested_position = TOPPING_POSITION_HALF1;
          } else {
            topping2.requested_position = TOPPING_POSITION_ALTERNATE1;
          }
        }
      } break;
      case END_PHASE: {
        if (topping.requested_position == TOPPING_POSITION_HALF1) {
          topping2.requested_position = TOPPING_POSITION_HALF2;
        } else if (topping.requested_position == TOPPING_POSITION_ALTERNATE1) {
          topping2.requested_position = TOPPING_POSITION_ALTERNATE2;
        } else {
          if (prob < 200) {
            topping2.requested_position = TOPPING_POSITION_FULL;
          } else if (prob < 600) {
            topping2.requested_position = TOPPING_POSITION_HALF1;
          } else {
            topping2.requested_position = TOPPING_POSITION_ALTERNATE1;
          }
        }
      } break;
      default:
        assert(false && "shouldn't add orders in this state");
    }

    available_toppings_index = rand() % available_toppings.count;
    topping2.type = available_toppings.items[available_toppings_index].type;
    da_remove_unordered(&available_toppings, available_toppings_index);

    da_append(&order.requested_toppings, topping2);

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
        .speedModifier = 1 + (GetRandomValue(0, 20) / 100.0f)
      }));
      break;
    }
    da_append(&orders, order);
    orders.active += 1;
  }
}

void UpdateConveyorBelt(void) {
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
    || da_last(&conveyor_belt).position.x + toppingsTex[da_last(&conveyor_belt).type].width/2.0f > CONVEYOR_BELT_INGREDIENT_GAP)
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
      .position = {.x = 0, .y = 120 - toppingsTex[required_toppings.items[required_toppings_index].type].height/2.0f * INGREDIENT_SCALE},
    }));

    da_free(required_toppings);
  }
}

void PickupToppingFromConveyorBelt(Vector2 mouse_pos) {
  if (topping_selected == TOPPING_NONE && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    for (size_t i = 0; i < conveyor_belt.count; i++) {
      ConveyorBeltIngredient ingredient = conveyor_belt.items[i];
      if (
        mouse_pos.x < ingredient.position.x - STANDARD_INGREDIENT_HITBOX_WIDTH / 2.0f + STANDARD_INGREDIENT_HITBOX_WIDTH * 1.5f
        && mouse_pos.x > ingredient.position.x - STANDARD_INGREDIENT_HITBOX_WIDTH / 2.0f
        && mouse_pos.y < ingredient.position.y - STANDARD_INGREDIENT_HITBOX_HEIGHT / 2.0f + STANDARD_INGREDIENT_HITBOX_HEIGHT * 1.5f
        && mouse_pos.y > conveyor_belt.items[i].position.y - STANDARD_INGREDIENT_HITBOX_HEIGHT / 2.0f
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
}

void PlaceToppingOnPizza(Vector2 mouse_pos) {
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
      PlaySound(placeIngredient);
    }
    topping_selected = TOPPING_NONE;
  }
}

void TossOrServe(Vector2 mouse_pos, double time) {
  Rectangle leftTossButton = {105, 977, 78, 78};
  Rectangle rightTossButton = {1829, 977, 78, 78};
  Rectangle leftServeButton = {21, 977, 78, 78};
  Rectangle rightServeButton = {1745, 977, 78, 78};

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, leftTossButton)) {
    if (rotators[0].active) {
      pizzasTossed++;
      PlaySound(tossPizza);
      pizzas.items[rotators[0].pizza_index].rotation = 0;
      pizzas.items[rotators[0].pizza_index].toppings.count = 0;
    }
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, rightTossButton)) {
    if (rotators[1].active) {
      pizzasTossed++;
      PlaySound(tossPizza);
      pizzas.items[rotators[1].pizza_index].rotation = 0;
      pizzas.items[rotators[1].pizza_index].toppings.count = 0;
      
    }
  }
  
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, leftServeButton)) {
    if (rotators[0].active) {
      pizzasFinished++;
      PlaySound(newPizza);
      if (!firstPizzaServed) firstPizzaServed = true;
      rotators[0].spinning = false;
      rotators[0].active = false;
      orders.items[rotators[0].order_index].completed = true;
      orders.items[rotators[0].order_index].deliver_time = time;
      orders.active -= 1;
      pizzas.items[rotators[0].pizza_index].delivered = true;
      UpdateScore(pizzas.items[rotators[0].pizza_index], orders.items[rotators[0].order_index], rotators[0].speed_setting);
    }
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, rightServeButton)) {
    if (rotators[1].active) {
      pizzasFinished++;
      PlaySound(newPizza);
      if (!firstPizzaServed) firstPizzaServed = true;
      rotators[1].spinning = false;
      rotators[1].active = false;
      orders.items[rotators[1].order_index].completed = true;
      orders.items[rotators[1].order_index].deliver_time = time;
      orders.active -= 1;
      pizzas.items[rotators[1].pizza_index].delivered = true;
      UpdateScore(pizzas.items[rotators[1].pizza_index], orders.items[rotators[1].order_index], rotators[1].speed_setting);
    }
  }
}

void DrawConveyorBelt(float dt) {
  beltTimer += dt;
  if (beltTimer >= 1.0f/6.0f) {
    beltTimer = 0;
    if (game_phase >= TUTORIAL_PHASE && game_phase <= END_PHASE)
      currentBeltFrame = (currentBeltFrame + 1) % 3;
  }

  DrawTexture(conveyorBeltFrames[currentBeltFrame], 0, 30, WHITE);
  for (size_t i = 0; i < conveyor_belt.count; i++) {
    Texture2D toppingTexture = toppingsTex[conveyor_belt.items[i].type];
    float targetWidth = toppingTexture.width * INGREDIENT_SCALE;
    float targetHeight = toppingTexture.height * INGREDIENT_SCALE;
    Rectangle targetRect = {conveyor_belt.items[i].position.x, conveyor_belt.items[i].position.y, targetWidth, targetHeight};
    DrawTexturePro(toppingTexture, (Rectangle){0,0, toppingTexture.width, toppingTexture.height}, targetRect, (Vector2){0,0}, 0, WHITE);
  }

  DrawTexture(foregroundBelt, 0, 0, WHITE);
}

void DrawRobotArms(void) {
  Vector2 left_arm_attached_pos = {820,385};
  Vector2 left_arm_attached_rod_pos = {820 - leftArmBearing.width*2.1, 385 - leftArmBearing.height/2.0f};
  Vector2 left_arm_rotating_part_pos = {675, left_arm_attached_rod_pos.y + leftArmBearing.height/2.0f};
  float left_arm_target_rot = 0;
  if (game_phase >= TUTORIAL_PHASE && game_phase <= END_PHASE) {
    if (GetMouseX() <= left_arm_attached_pos.x) left_arm_target_rot = atan2f(left_arm_rotating_part_pos.y - GetMouseY(),  left_arm_rotating_part_pos.x - GetMouseX() ) * RAD2DEG;
    left_arm_target_rot = Clamp(left_arm_target_rot, -95, 95);
  }

  DrawTexturePro(
    leftArmRod,
    (Rectangle){
      0,
      0,
      leftArmRod.width,
      leftArmRod.height
    },
    (Rectangle){
      left_arm_attached_rod_pos.x,
      left_arm_attached_rod_pos.y,
      leftArmRod.width*3,
      leftArmRod.height
    },
    (Vector2){0, 0},
    0,
    WHITE
  );
  DrawTextureV(leftArmBearing, (Vector2){left_arm_attached_pos.x-leftArmBearing.width/2.0, left_arm_attached_pos.y-leftArmBearing.height/2.0}, WHITE);
  DrawTexturePro(
    robotArmLeft,
    (Rectangle){
      0, 
      0,
      robotArmLeft.width,
      robotArmLeft.height
    },
    (Rectangle){
      left_arm_rotating_part_pos.x,
      left_arm_rotating_part_pos.y,
      robotArmLeft.width,
      robotArmLeft.height
    },
    (Vector2){
      robotArmLeft.width - 40,
      robotArmLeft.height/2.0,
    },
    left_arm_target_rot,
    WHITE
  );

  Vector2 right_arm_attached_pos = {1099,385};
  Vector2 right_arm_attached_rod_pos = {1099, 385 - rightArmBearing.height/2.0};
  Vector2 right_arm_rotating_part_pos = {1244, right_arm_attached_rod_pos.y + rightArmBearing.height/2.0};
  float right_arm_target_rot = 180;
  if (GetMouseX() >= right_arm_attached_pos.x) right_arm_target_rot = atan2f(right_arm_rotating_part_pos.y - GetMouseY(),  right_arm_rotating_part_pos.x - GetMouseX() ) * RAD2DEG;
  right_arm_target_rot = (int)right_arm_target_rot % 360;
  right_arm_target_rot = (right_arm_target_rot >= 85) ? right_arm_target_rot : (right_arm_target_rot <= -85) ? right_arm_target_rot : -85;

  DrawTextureV(rightArmBearing, (Vector2){right_arm_attached_pos.x-rightArmBearing.width/2.0, right_arm_attached_pos.y-rightArmBearing.height/2.0}, WHITE);

  DrawTexturePro(
    rightArmRod,
    (Rectangle){
      0,
      0,
      rightArmRod.width,
      rightArmRod.height
    },
    (Rectangle){
      right_arm_attached_rod_pos.x,
      right_arm_attached_rod_pos.y,
      rightArmRod.width*3,
      rightArmRod.height
    },
    (Vector2){0, 0},
    0,
    WHITE
  );

  DrawTexturePro(
    robotArmLeft,
    (Rectangle){
      0, 
      0,
      robotArmRight.width,
      robotArmRight.height
    },
    (Rectangle){
      right_arm_rotating_part_pos.x,
      right_arm_rotating_part_pos.y,
      robotArmRight.width,
      robotArmRight.height
    },
    (Vector2){
      robotArmRight.width - 40,
      robotArmRight.height/2.0,
    },
    right_arm_target_rot,
    WHITE
  );
}

void UpdateAndDrawSpeedControllers(Vector2 mouse_pos) {
  int topY =  840;
  int bottomY = 1010;
  float minSpeed = 31.415 * (PI / 180);
  float maxSpeed = minSpeed * 4;

  int leftY = topY + (1.0f - (rotators[0].rotation_speed - minSpeed) / (maxSpeed - minSpeed)) * (bottomY - topY);
  int rightY = topY + (1.0f - (rotators[1].rotation_speed - minSpeed) / (maxSpeed - minSpeed)) * (bottomY - topY);

  DrawTexture(speedControllerTex, 826, leftY, WHITE);
  DrawTexture(speedControllerTex, 1018, rightY, WHITE);

  bool openToDrag = topping_selected == TOPPING_NONE && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  
  if (CheckCollisionPointRec(mouse_pos, (Rectangle){826.0f, (float)leftY, 69.0f, 38.0f}) && openToDrag && !draggingLeftCtrl && pizzas.items[rotators[0].pizza_index].toppings.count < 1) {
    draggingLeftCtrl = true;
  }

  if (CheckCollisionPointRec(mouse_pos, (Rectangle){1018.0f, (float)rightY, 69.0f, 38.0f}) && openToDrag && !draggingRightCtrl && pizzas.items[rotators[1].pizza_index].toppings.count < 1) {
    draggingRightCtrl = true;
  }

  if (draggingLeftCtrl) {
    if (rotators[0].active && rotators[0].spinning) {
      rotators[0].rotation_speed = ((float)(1.0f - ((mouse_pos.y - 19 * 4) - (float)topY) / (float)(bottomY - topY) * (maxSpeed - minSpeed)) + minSpeed);
      rotators[0].rotation_speed = minSpeed + maxSpeed * (round(((rotators[0].rotation_speed - minSpeed) / maxSpeed) * 4) / 4);
      rotators[0].rotation_speed = (((rotators[0].rotation_speed > minSpeed) ? rotators[0].rotation_speed : minSpeed) < maxSpeed) ? ((rotators[0].rotation_speed > minSpeed) ? rotators[0].rotation_speed : minSpeed) : maxSpeed;
      rotators[0].speed_setting = (rotators[0].rotation_speed / minSpeed) - 1;

      if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        draggingLeftCtrl = false;
      }
    }
  } 
  if (draggingRightCtrl) {
    if (rotators[1].active && rotators[1].spinning) {
     
      rotators[1].rotation_speed = ((float)(1.0f - ((mouse_pos.y - 19 * 4) - (float)topY) / (float)(bottomY - topY) * (maxSpeed - minSpeed)) + minSpeed);
      rotators[1].rotation_speed = minSpeed + maxSpeed * (round(((rotators[1].rotation_speed - minSpeed) / maxSpeed) * 4) / 4);
      rotators[1].rotation_speed = (((rotators[1].rotation_speed > minSpeed) ? rotators[1].rotation_speed : minSpeed) < maxSpeed) ? ((rotators[1].rotation_speed > minSpeed) ? rotators[1].rotation_speed : minSpeed) : maxSpeed;

      
      if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        draggingRightCtrl = false;
      }
    }
  }
}

void DrawPizzas(void) {
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
    DrawTexturePro(
      pizzaMaskTex,
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
      180,
      WHITE
    );
  }
}

void DrawOrderTicket(void) {
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
}

void DrawPickedUpTopping(Vector2 mouse_pos) {
  if (topping_selected != TOPPING_NONE) {
    Texture2D toppingTexture = toppingsTex[(assert(topping_selected != TOPPING_NONE), topping_selected)];
    float targetWidth = toppingTexture.width * INGREDIENT_SCALE * 1.25f;
    float targetHeight = toppingTexture.height * INGREDIENT_SCALE * 1.25f;
    Rectangle targetRect = {mouse_pos.x - targetWidth / 2, mouse_pos.y - targetHeight / 2, targetWidth, targetHeight};
    DrawTexturePro(toppingTexture, (Rectangle){0,0, toppingTexture.width, toppingTexture.height}, targetRect, (Vector2){0,0}, 0, WHITE);
  }
}

void DrawTextEffects(void) {
  for (int i = textEffects.count - 1; i >= 0; i--) {
    TextEffect textEffect = textEffects.items[i];
    DrawText(textEffect.text, textEffect.position.x, textEffect.position.y, textEffect.size, (Color){255, 255, 255, textEffect.opacity});
    textEffects.items[i].opacity -= 2;
    textEffects.items[i].position.y -= 2;
    if (textEffects.items[i].opacity <= 0 || textEffects.items[i].position.y + textEffects.items[i].size <= 0 || textEffects.items[i].size <= 3) {
      da_remove_unordered(&textEffects, i);
    }
  }
}

void DrawTutorial(void) {
  if (!firstPizzaServed)
    DrawTexture(tutorialTex, tutorialX, (SCREEN_HEIGHT - tutorialTex.height) / 2, WHITE);
}
