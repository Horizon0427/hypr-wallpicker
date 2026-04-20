#include "ui.h"
#include "app.h"
#include "fs.h"
#include "raylib.h"
#include "render.h"

SelectionResult RunUI(App *app, const AppConfig *config) {
  SelectionResult result = {0};
  result.valid = false;

  float inradius = HEX_RADIUS * 0.866025f;
  app->scroll_y = 0.0f;
  app->target_scroll_y = 0.0f;

  while (!WindowShouldClose()) {
    Vector2 mousePoint = GetMousePosition();

    int cols = config->cols;
    float spacing = config->spacing;
    float stepX = 1.73205f * HEX_RADIUS + spacing;
    float stepY = 1.5f * HEX_RADIUS + spacing;

    int totalRows = (app->wp_count + cols - 1) / cols;
    float totalWidth = cols * stepX;
    float totalHeight = (2.0f * HEX_RADIUS) + (totalRows - 1) * stepY;

    float startX = (GetScreenWidth() - totalWidth) / 2.0f + stepX / 2.0f;
    float startY;
    float maxScroll = 0.0f;

    if (totalHeight <= GetScreenHeight()) {
      startY = (GetScreenHeight() - totalHeight) / 2.0f + HEX_RADIUS;
      app->target_scroll_y = 0.0f;
    } else {
      startY = HEX_RADIUS + 50.0f;
      maxScroll = totalHeight - GetScreenHeight() + 100.0f;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && maxScroll > 0.0f) {
      app->target_scroll_y += wheel * 120.0f;
    }

    if (app->target_scroll_y > 0.0f)
      app->target_scroll_y = 0.0f;
    if (app->target_scroll_y < -maxScroll)
      app->target_scroll_y = -maxScroll;

    app->scroll_y += (app->target_scroll_y - app->scroll_y) * 0.15f;

    int hoveredIndex = -1;

    for (int i = 0; i < app->wp_count; i++) {
      int row = i / cols;
      int col = i % cols;

      float currentX = startX + (float)col * stepX;
      if ((row % 2) != 0) {
        currentX += stepX / 2.0f;
      }

      float currentY = startY + (float)row * stepY + app->scroll_y;

      app->wallpapers[i].render_x = currentX;
      app->wallpapers[i].render_y = currentY;

      float dx = mousePoint.x - currentX;
      float dy = mousePoint.y - currentY;

      if (hoveredIndex == -1 && (dx * dx + dy * dy) <= (inradius * inradius)) {
        hoveredIndex = i;
      }

      float targetScale = (i == hoveredIndex) ? 1.15f : 1.0f;
      float targetColor = (i == hoveredIndex) ? 255.0f : 130.0f;

      app->wallpapers[i].currentScale +=
          (targetScale - app->wallpapers[i].currentScale) * 0.15f;
      app->wallpapers[i].currentColor +=
          (targetColor - app->wallpapers[i].currentColor) * 0.15f;
    }

    RenderWallpapers(app, hoveredIndex);

    if (hoveredIndex != -1 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      result.valid = true;
      result.full_target_path =
          JoinPath(app->wp_dir, app->wallpapers[hoveredIndex].filename);

      float relX =
          app->wallpapers[hoveredIndex].render_x / (float)GetScreenWidth();
      float relY =
          (GetScreenHeight() - app->wallpapers[hoveredIndex].render_y) /
          (float)GetScreenHeight();
      result.rel_x = relX;
      result.rel_y = relY;

      break;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      break;
    }
  }

  return result;
}
