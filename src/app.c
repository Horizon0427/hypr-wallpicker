#define _POSIX_C_SOURCE 200809L

#include "app.h"
#include "apply.h"
#include "fs.h"
#include "wallpaper.h"
#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void AppShutdown(App *app);


SessionBackend DetectSessionBackend(void) {
  const char *xdg_session_type = getenv("XDG_SESSION_TYPE");
  const char *wayland_display = getenv("WAYLAND_DISPLAY");
  const char *display = getenv("DISPLAY");

  if (xdg_session_type != NULL) {
    if (strcmp(xdg_session_type, "wayland") == 0) {
      return SESSION_BACKEND_WAYLAND;
    }
    if (strcmp(xdg_session_type, "x11") == 0) {
      return SESSION_BACKEND_X11;
    }
  }


  if (wayland_display != NULL && wayland_display[0] != '\0') {
    return SESSION_BACKEND_WAYLAND;
  }

  if (display != NULL && display[0] != '\0') {
    return SESSION_BACKEND_X11;
  }

  return SESSION_BACKEND_UNKNOWN;
}


const char *SessionBackendName(SessionBackend backend) {
  switch (backend) {
  case SESSION_BACKEND_WAYLAND:
    return "Wayland";
  case SESSION_BACKEND_X11:
    return "X11";
  case SESSION_BACKEND_UNKNOWN:
  default:
    return "Unknown";
  }
}


AppConfig AppConfigFromArgs(int argc, char **argv) {
  AppConfig config = {0};

  config.window_width = DEFAULT_WINDOW_WIDTH;
  config.window_height = DEFAULT_WINDOW_HEIGHT;
  config.cols = DEFAULT_COLS;
  config.spacing = DEFAULT_SPACING;
  config.backend = DetectSessionBackend();

  if (argc > 1) {
    config.wallpaper_dir = argv[1];
  } else {
    config.wallpaper_dir = NULL;
  }
  return config;
}


int AppRun(const AppConfig *config) {
  App app = {0};

  if (config == NULL) {
    fprintf(stderr, "Error: invalid app config\n");
    return 1;
  }

  if (config->wallpaper_dir != NULL) {
    app.wp_dir = strdup(config->wallpaper_dir);
    if (app.wp_dir == NULL) {
      fprintf(stderr, "Error: out of memory\n");
      return 1;
    }
  } else {
    app.wp_dir = GetDefaultWallpaperDir();
    if (app.wp_dir == NULL) {
      return 1;
    }
  }


  if (!GetCacheDir(app.cache_dir, sizeof(app.cache_dir))) {
    AppShutdown(&app);
    return 1;
  }

  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
  InitWindow(config->window_width, config->window_height, "wallpicker");

  if (!InitWallpaperResources(&app)) {
    fprintf(stderr, "Error: failed to initialize wallpaper resources\n");
    AppShutdown(&app);
    return 1;
  }

  if (!LoadWallpapers(&app)) {
    AppShutdown(&app);
    return 1;
  }

  if (app.wp_count == 0) {
    printf("No wallpapers in .png, .jpg, or .jpeg format were found in %s.\n",
           app.wp_dir);
    AppShutdown(&app);
    return 0;
  }

  SetTargetFPS(60);

  {
    float inradius = HEX_RADIUS * 0.866025f;
    app.scroll_y = 0.0f;
    app.target_scroll_y = 0.0f;

    while (!WindowShouldClose()) {
      Vector2 mousePoint = GetMousePosition();

      int cols = config->cols;
      float spacing = config->spacing;
      float stepX = 1.73205f * HEX_RADIUS + spacing;
      float stepY = 1.5f * HEX_RADIUS + spacing;

      int totalRows = (app.wp_count + cols - 1) / cols;
      float totalWidth = cols * stepX;
      float totalHeight = (2.0f * HEX_RADIUS) + (totalRows - 1) * stepY;

      float startX = (GetScreenWidth() - totalWidth) / 2.0f + stepX / 2.0f;
      float startY;
      float maxScroll = 0.0f;

      if (totalHeight <= GetScreenHeight()) {
        startY = (GetScreenHeight() - totalHeight) / 2.0f + HEX_RADIUS;
        app.target_scroll_y = 0.0f;
      } else {
        startY = HEX_RADIUS + 50.0f;
        maxScroll = totalHeight - GetScreenHeight() + 100.0f;
      }

      {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && maxScroll > 0.0f) {
          app.target_scroll_y += wheel * 120.0f;
        }
      }

      if (app.target_scroll_y > 0.0f) {
        app.target_scroll_y = 0.0f;
      }
      if (app.target_scroll_y < -maxScroll) {
        app.target_scroll_y = -maxScroll;
      }

      app.scroll_y += (app.target_scroll_y - app.scroll_y) * 0.15f;

      int hoveredIndex = -1;

      for (int i = 0; i < app.wp_count; i++) {
        int row = i / cols;
        int col = i % cols;

        float currentX = startX + (float)col * stepX;
        if ((row % 2) != 0) {
          currentX += stepX / 2.0f;
        }

        float currentY = startY + (float)row * stepY + app.scroll_y;

        float dx = mousePoint.x - currentX;
        float dy = mousePoint.y - currentY;

        if ((dx * dx + dy * dy) <= (inradius * inradius)) {
          hoveredIndex = i;
          break;
        }
      }


      for (int i = 0; i < app.wp_count; i++) {
        float targetScale = (i == hoveredIndex) ? 1.15f : 1.0f;
        float targetColor = (i == hoveredIndex) ? 255.0f : 130.0f;

        app.wallpapers[i].currentScale +=
            (targetScale - app.wallpapers[i].currentScale) * 0.15f;
        app.wallpapers[i].currentColor +=
            (targetColor - app.wallpapers[i].currentColor) * 0.15f;
      }


      BeginDrawing();
      ClearBackground(BLANK);

      for (int i = 0; i < app.wp_count; i++) {
        if (i == hoveredIndex) {
          continue;
        }

        int row = i / cols;
        int col = i % cols;

        float currentX = startX + (float)col * stepX;
        if ((row % 2) != 0) {
          currentX += stepX / 2.0f;
        }

        float currentY = startY + (float)row * stepY + app.scroll_y;

        float scale = app.wallpapers[i].currentScale;
        unsigned char c = (unsigned char)app.wallpapers[i].currentColor;
        Color tint = (Color){c, c, c, 255};

        Rectangle sourceRec = {0.0f, 0.0f, (float)app.img_size,
                               (float)app.img_size};
        Rectangle destRec = {currentX, currentY, (float)app.img_size * scale,
                             (float)app.img_size * scale};
        Vector2 origin = {((float)app.img_size * scale) / 2.0f,
                          ((float)app.img_size * scale) / 2.0f};

        DrawTexturePro(app.wallpapers[i].tex, sourceRec, destRec, origin, 0.0f,
                       tint);
      }


      if (hoveredIndex != -1) {
        int row = hoveredIndex / cols;
        int col = hoveredIndex % cols;

        float currentX = startX + (float)col * stepX;
        if ((row % 2) != 0) {
          currentX += stepX / 2.0f;
        }

        float currentY = startY + (float)row * stepY + app.scroll_y;
        Vector2 currentCenter = {currentX, currentY};

        float scale = app.wallpapers[hoveredIndex].currentScale;
        unsigned char c = (unsigned char)app.wallpapers[hoveredIndex].currentColor;
        Color tint = (Color){c, c, c, 255};

        Rectangle sourceRec = {0.0f, 0.0f, (float)app.img_size,
                               (float)app.img_size};
        Rectangle destRec = {currentX, currentY, (float)app.img_size * scale,
                             (float)app.img_size * scale};
        Vector2 origin = {((float)app.img_size * scale) / 2.0f,
                          ((float)app.img_size * scale) / 2.0f};

        DrawTexturePro(app.wallpapers[hoveredIndex].tex, sourceRec, destRec,
                       origin, 0.0f, tint);
        DrawPolyLinesEx(currentCenter, 6, HEX_RADIUS * scale, 30.0f, 8.0f,
                        WHITE);

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          char *full_target_path =
              JoinPath(app.wp_dir, app.wallpapers[hoveredIndex].filename);

          if (full_target_path != NULL) {
            float relX = currentX / (float)GetScreenWidth();
            float relY =
                (GetScreenHeight() - currentY) / (float)GetScreenHeight();

            ApplyWallpaper(full_target_path, relX, relY);
            free(full_target_path);
          }

          EndDrawing();
          break;
        }
      }

      if (IsKeyPressed(KEY_ESCAPE)) {
        EndDrawing();
        break;
      }

      EndDrawing();
    }
  }

  AppShutdown(&app);
  return 0;
}

static void AppShutdown(App *app) {
  if (app == NULL) {
    return;
  }

  UnloadWallpapers(app);

  if (app->wp_dir != NULL) {
    free(app->wp_dir);
    app->wp_dir = NULL;
  }

  if (IsWindowReady()) {
    CloseWindow();
  }
}
