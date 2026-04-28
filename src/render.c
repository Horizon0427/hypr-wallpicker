#include "render.h"
#include "raylib.h"
#include <stddef.h>

void RenderWallpapers(const App *app, int hoveredIndex, float angle_deg) {

  if (app == NULL || app->wallpapers == NULL || app->wp_count <= 0)
    return;

  BeginDrawing();
  ClearBackground(BLANK);

  for (int i = 0; i < app->wp_count; i++) {
    if (i == hoveredIndex)
      continue;

    float currentX = app->wallpapers[i].render_x;
    float currentY = app->wallpapers[i].render_y;
    float scale = app->wallpapers[i].currentScale;
    unsigned char c = (unsigned char)app->wallpapers[i].currentColor;

    Color tint = (Color){c, c, c, 255};
    Rectangle sourceRec = {0.0f, 0.0f, (float)app->img_size,
                           (float)app->img_size};
    Rectangle destRec = {currentX, currentY, (float)app->img_size * scale,
                         (float)app->img_size * scale};
    Vector2 origin = {((float)app->img_size * scale) / 2.0f,
                      ((float)app->img_size * scale) / 2.0f};

    DrawTexturePro(app->wallpapers[i].tex, sourceRec, destRec, origin, 0.0f,
                   tint);
  }

  if (hoveredIndex >= 0 && hoveredIndex < app->wp_count) {
    float currentX = app->wallpapers[hoveredIndex].render_x;
    float currentY = app->wallpapers[hoveredIndex].render_y;
    Vector2 currentCenter = {currentX, currentY};

    float scale = app->wallpapers[hoveredIndex].currentScale;
    unsigned char c = (unsigned char)app->wallpapers[hoveredIndex].currentColor;
    Color tint = (Color){c, c, c, 255};

    Rectangle sourceRec = {0.0f, 0.0f, (float)app->img_size,
                           (float)app->img_size};
    Rectangle destRec = {currentX, currentY, (float)app->img_size * scale,
                         (float)app->img_size * scale};
    Vector2 origin = {((float)app->img_size * scale) / 2.0f,
                      ((float)app->img_size * scale) / 2.0f};

    DrawTexturePro(app->wallpapers[hoveredIndex].tex, sourceRec, destRec,
                   origin, 0.0f, tint);

    DrawPolyLinesEx(currentCenter, 6, HEX_RADIUS * scale, 30.0f + angle_deg,
                    8.0f, WHITE);
  }

  EndDrawing();
}
