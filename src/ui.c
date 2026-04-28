#include "ui.h"
#include "app.h"
#include "fs.h"
#include "raylib.h"
#include "render.h"
#include <math.h>

SelectionResult RunUI(App *app, const AppConfig *config) {
  SelectionResult result = {0};
  result.valid = false;

  float inradius = HEX_RADIUS * 0.866025f;
  app->scroll_offset = 0.0f;
  app->target_scroll_offset = 0.0f;
  while (!WindowShouldClose()) {
    Vector2 mousePoint = GetMousePosition();

    float rad = config->angle * DEG2RAD;
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    int cols = config->cols < 1 ? 1 : config->cols;
    float spacing = config->spacing;
    float stepX = 1.73205f * HEX_RADIUS + spacing;
    float stepY = 1.5f * HEX_RADIUS + spacing;

    int totalRows = (app->wp_count + cols - 1) / cols;
    float totalHeight = (2.0f * HEX_RADIUS) + (totalRows - 1) * stepY;

    float center_x = ((cols - 1) * stepX + stepX / 2.0f) / 2.0f;
    float center_y = (totalRows - 1) * stepY / 2.0f;

    float max_scroll_amplitude = totalHeight / 2.0f;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && max_scroll_amplitude > 0.0f) {
      app->target_scroll_offset += wheel * 120.0f;
    }

    if (app->target_scroll_offset > max_scroll_amplitude)
      app->target_scroll_offset = max_scroll_amplitude;
    if (app->target_scroll_offset < -max_scroll_amplitude)
      app->target_scroll_offset = -max_scroll_amplitude;

    app->scroll_offset +=
        (app->target_scroll_offset - app->scroll_offset) * 0.15f;

    int hoveredIndex = -1;

    for (int i = 0; i < app->wp_count; i++) {
      int row = i / cols;
      int col = i % cols;

      float local_x = (float)col * stepX;
      if ((row % 2) != 0)
        local_x += stepX / 2.0f;
      float local_y = (float)row * stepY;

      local_x -= center_x;
      local_y = local_y - center_y + app->scroll_offset;

      float rot_x = local_x * cosA - local_y * sinA;
      float rot_y = local_x * sinA + local_y * cosA;

      float currentX = GetScreenWidth() / 2.0f + rot_x;
      float currentY = GetScreenHeight() / 2.0f + rot_y;

      app->wallpapers[i].render_x = currentX;
      app->wallpapers[i].render_y = currentY;

      float dx = mousePoint.x - currentX;
      float dy = mousePoint.y - currentY;

      float dx_abs = fabsf(dx);
      float dy_abs = fabsf(dy);

      float horiz_radius = inradius;
      float vert_radius = HEX_RADIUS;

      if (dx_abs <= horiz_radius && dy_abs <= vert_radius) {
        if (1.73205f * dy_abs + dx_abs <= 1.73205f * HEX_RADIUS) {
          if (hoveredIndex == -1) {
            hoveredIndex = i;
          }
        }
      }

      float targetScale = (i == hoveredIndex) ? 1.15f : 1.0f;
      float targetColor = (i == hoveredIndex) ? 255.0f : 130.0f;

      app->wallpapers[i].currentScale +=
          (targetScale - app->wallpapers[i].currentScale) * 0.15f;
      app->wallpapers[i].currentColor +=
          (targetColor - app->wallpapers[i].currentColor) * 0.15f;
    }

    RenderWallpapers(app, hoveredIndex, config->angle);

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
