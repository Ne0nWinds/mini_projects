#include "general.h"
#include "vector.h"

enum {
  LMOUSE = 1,
};

struct game_input {
  v2 movement;
  v2 mouse_pos;
  u32 buttons;
};
typedef struct game_input game_input;

void GameInit();
void GameMain(f64 delta, game_input input);
void GameRender();
