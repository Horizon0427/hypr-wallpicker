#ifndef APP_H
#define APP_H

#include "raylib.h"
#include <limits.h>
#include <stdbool.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define HEX_RADIUS 200.0f
#define DEFAULT_COLS 5
#define DEFAULT_SPACING 15.0f

typedef struct {
  Texture2D tex;
  char filename[PATH_MAX];
  float currentScale;
  float currentColor;
} Wallpaper;

typedef struct {
  bool valid;
  char full_target_path[PATH_MAX * 2];
  float rel_x;
  float rel_y;
} SelectionResult;

#endif
