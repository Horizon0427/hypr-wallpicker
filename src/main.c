#define _POSIX_C_SOURCE 200809L

#include "app.h"
#include "apply.h"
#include "fs.h"
#include "raylib.h"

#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
#define HEX_RADIUS 200.0f

typedef struct {
  Texture2D tex;
  char filename[PATH_MAX];
  float currentScale;
  float currentColor;
} Wallpaper;
*/



Image GenerateHexMask(int size, float radius) {
  Image mask = GenImageColor(size, size, BLANK);
  float cx = size / 2.0f;
  float cy = size / 2.0f;

  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      float dx = fabsf((float)x - cx);
      float dy = fabsf((float)y - cy);

      if (dx <= 0.866025f * radius && dy <= radius - dx * 0.57735f) {
        ImageDrawPixel(&mask, x, y, WHITE);
      } else {
        ImageDrawPixel(&mask, x, y, BLANK);
      }
    }
  }
  return mask;
}

int main(int argc, char **argv) {
  char *wp_dir = NULL;

  if (argc > 1) {
    wp_dir = strdup(argv[1]);
    if (wp_dir == NULL) {
      fprintf(stderr, "Error: out of memory\n");
      return 1;
    }
  } else {
    wp_dir = GetDefaultWallpaperDir();
    if (wp_dir == NULL) {
      return 1;
    }
  }

  /*
    char cache_dir[PATH_MAX];
    const char *home = getenv("HOME");
    if (home == NULL) {
      fprintf(stderr, "Error: Unable to retrieve HOME environment variable!\n");
      return 1;
    }

    snprintf(cache_dir, sizeof(cache_dir), "%s/.cache/wallpicker", home);

    char mkdir_cmd[PATH_MAX + 128];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", cache_dir);
    system(mkdir_cmd);
  */

  char cache_dir[PATH_MAX];
  if (!GetCacheDir(cache_dir, sizeof(cache_dir))) {
    free(wp_dir);
    return 1; // exit if the func fails to read or make
  }

  int capacity = 0;
  DIR *dir = opendir(wp_dir);
  struct dirent *ent;

  if (dir == NULL) {
    fprintf(stderr, "Error: Unable to open directory %s\n", wp_dir);
    free(wp_dir);
    return 1;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (IsSupportedWallpaperFile(ent->d_name)) {
      capacity++;
    }
  }
  closedir(dir);

  if (capacity == 0) {
    printf("No wallpapers in .png, .jpg, or .jpeg format were found in %s.\n",
           wp_dir);
    free(wp_dir);
    return 0;
  }

  Wallpaper *wallpapers = malloc(capacity * sizeof(Wallpaper));
  if (wallpapers == NULL) {
    fprintf(stderr,
            "Fatal Error: malloc memory allocation failed (attempted to "
            "allocate %d wallpaper space)\n",
            capacity);
    free(wp_dir);
    return 1;
  }

  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
  InitWindow(1920, 1080, "wallpicker");

  int imgSize = (int)(HEX_RADIUS * 2.0f);
  Image hexMask = GenerateHexMask(imgSize, HEX_RADIUS);

  int wpCount = 0;
  dir = opendir(wp_dir);

  if (dir == NULL) {
    fprintf(stderr, "Error: Unable to reopen directory %s\n", wp_dir);
    UnloadImage(hexMask);
    free(wallpapers);
    free(wp_dir);
    CloseWindow();
    return 1;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (IsSupportedWallpaperFile(ent->d_name)) {
      BeginDrawing();
      ClearBackground(BLANK);
      DrawText("Loading & Caching Wallpapers...", GetScreenWidth() / 2 - 250,
               GetScreenHeight() / 2, 30, WHITE);
      DrawText(ent->d_name, GetScreenWidth() / 2 - 250,
               GetScreenHeight() / 2 + 40, 20, GRAY);
      EndDrawing();

      char *full_img_path = JoinPath(wp_dir, ent->d_name);
      char *cache_img_path = BuildCacheImagePath(cache_dir, ent->d_name);

      if (full_img_path == NULL || cache_img_path == NULL) {
        free(full_img_path);
        free(cache_img_path);
        continue;
      }

      Image img = (Image){0};

      if (access(cache_img_path, F_OK) == 0) {
        img = LoadImage(cache_img_path);
      } else {
        img = LoadImage(full_img_path);

        if (img.data != NULL && img.width > 1) {
          float scaleX = (float)imgSize / img.width;
          float scaleY = (float)imgSize / img.height;
          float scale = (scaleX > scaleY) ? scaleX : scaleY;

          int newW = (int)roundf(img.width * scale);
          int newH = (int)roundf(img.height * scale);

          if (newW < imgSize)
            newW = imgSize;
          if (newH < imgSize)
            newH = imgSize;

          ImageResize(&img, newW, newH);

          int cropX = (newW - imgSize) / 2;
          int cropY = (newH - imgSize) / 2;
          ImageCrop(&img, (Rectangle){cropX, cropY, imgSize, imgSize});
          ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
          ImageAlphaMask(&img, hexMask);

          if (!ExportImage(img, cache_img_path)) {
            fprintf(stderr, "Warning: failed to write cache image: %s\n",
                    cache_img_path);
          }
        }
      }

      if (img.data != NULL && img.width > 1 && wpCount < capacity) {
        int n = snprintf(wallpapers[wpCount].filename,
                         sizeof(wallpapers[wpCount].filename), "%s",
                         ent->d_name);
        if (n < 0 || (size_t)n >= sizeof(wallpapers[wpCount].filename)) {
          fprintf(stderr, "Warning: filename too long, skipping: %s\n",
                  ent->d_name);
          UnloadImage(img);
          free(full_img_path);
          free(cache_img_path);
          continue;
        }

        wallpapers[wpCount].tex = LoadTextureFromImage(img);
        if (wallpapers[wpCount].tex.id != 0) {
          wallpapers[wpCount].currentScale = 1.0f;
          wallpapers[wpCount].currentColor = 130.0f;
          wpCount++;
        } else {
          fprintf(stderr, "Warning: failed to create texture for %s\n",
                  ent->d_name);
        }
      }

      if (img.data != NULL) {
        UnloadImage(img);
      }

      free(full_img_path);
      free(cache_img_path);
    }
  }
  closedir(dir);

  UnloadImage(hexMask);
  SetTargetFPS(60);

  float inradius = HEX_RADIUS * 0.866025f;
  float scrollY = 0.0f;
  float targetScrollY = 0.0f;

  while (!WindowShouldClose()) {
    Vector2 mousePoint = GetMousePosition();

    int COLS = 5;
    float spacing = 15.0f;
    float stepX = 1.73205f * HEX_RADIUS + spacing;
    float stepY = 1.5f * HEX_RADIUS + spacing;

    int totalRows = (wpCount + COLS - 1) / COLS;
    float totalWidth = COLS * stepX;
    float totalHeight = (2.0f * HEX_RADIUS) + (totalRows - 1) * stepY;

    float startX = (GetScreenWidth() - totalWidth) / 2.0f + stepX / 2.0f;
    float startY;
    float maxScroll = 0.0f;

    if (totalHeight <= GetScreenHeight()) {
      startY = (GetScreenHeight() - totalHeight) / 2.0f + HEX_RADIUS;
      targetScrollY = 0.0f;
    } else {
      startY = HEX_RADIUS + 50.0f;
      maxScroll = totalHeight - GetScreenHeight() + 100.0f;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && maxScroll > 0.0f) {
      targetScrollY += wheel * 120.0f;
    }

    if (targetScrollY > 0.0f)
      targetScrollY = 0.0f;
    if (targetScrollY < -maxScroll)
      targetScrollY = -maxScroll;

    scrollY += (targetScrollY - scrollY) * 0.15f;

    int hoveredIndex = -1;

    for (int i = 0; i < wpCount; i++) {
      int row = i / COLS;
      int col = i % COLS;
      float currentX = startX + col * stepX;
      if (row % 2 != 0)
        currentX += stepX / 2.0f;
      float currentY = startY + row * stepY + scrollY;

      float dx = mousePoint.x - currentX;
      float dy = mousePoint.y - currentY;
      if ((dx * dx + dy * dy) <= (inradius * inradius)) {
        hoveredIndex = i;
        break;
      }
    }

    for (int i = 0; i < wpCount; i++) {
      float targetScale = (i == hoveredIndex) ? 1.15f : 1.0f;
      float targetColor = (i == hoveredIndex) ? 255.0f : 130.0f;

      wallpapers[i].currentScale +=
          (targetScale - wallpapers[i].currentScale) * 0.15f;
      wallpapers[i].currentColor +=
          (targetColor - wallpapers[i].currentColor) * 0.15f;
    }

    BeginDrawing();
    ClearBackground(BLANK);

    for (int i = 0; i < wpCount; i++) {
      if (i == hoveredIndex)
        continue;

      int row = i / COLS;
      int col = i % COLS;
      float currentX = startX + col * stepX;
      if (row % 2 != 0)
        currentX += stepX / 2.0f;
      float currentY = startY + row * stepY + scrollY;

      float scale = wallpapers[i].currentScale;
      unsigned char c = (unsigned char)wallpapers[i].currentColor;
      Color tint = (Color){c, c, c, 255};

      Rectangle sourceRec = {0, 0, imgSize, imgSize};
      Rectangle destRec = {currentX, currentY, imgSize * scale,
                           imgSize * scale};
      Vector2 origin = {(imgSize * scale) / 2.0f, (imgSize * scale) / 2.0f};

      DrawTexturePro(wallpapers[i].tex, sourceRec, destRec, origin, 0.0f, tint);
    }

    if (hoveredIndex != -1) {
      int row = hoveredIndex / COLS;
      int col = hoveredIndex % COLS;
      float currentX = startX + col * stepX;
      if (row % 2 != 0)
        currentX += stepX / 2.0f;
      float currentY = startY + row * stepY + scrollY;
      Vector2 currentCenter = {currentX, currentY};

      float scale = wallpapers[hoveredIndex].currentScale;
      unsigned char c = (unsigned char)wallpapers[hoveredIndex].currentColor;
      Color tint = (Color){c, c, c, 255};

      Rectangle sourceRec = {0, 0, imgSize, imgSize};
      Rectangle destRec = {currentX, currentY, imgSize * scale,
                           imgSize * scale};
      Vector2 origin = {(imgSize * scale) / 2.0f, (imgSize * scale) / 2.0f};

      DrawTexturePro(wallpapers[hoveredIndex].tex, sourceRec, destRec, origin,
                     0.0f, tint);
      DrawPolyLinesEx(currentCenter, 6, HEX_RADIUS * scale, 30.0f, 8.0f, WHITE);

      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        char *full_target_path =
            JoinPath(wp_dir, wallpapers[hoveredIndex].filename);

        if (full_target_path != NULL) {
          float relX = currentX / (float)GetScreenWidth();
          float relY = (GetScreenHeight() - currentY) / (float)GetScreenHeight();

          ApplyWallpaper(full_target_path, relX, relY);
          free(full_target_path);
        }

        break;
      }
    }

    if (IsKeyPressed(KEY_ESCAPE))
      break;

    EndDrawing();
  }

  for (int i = 0; i < wpCount; i++) {
    UnloadTexture(wallpapers[i].tex);
  }

  free(wallpapers);
  free(wp_dir);
  CloseWindow();
  return 0;
}
