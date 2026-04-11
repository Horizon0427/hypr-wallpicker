#define _POSIX_C_SOURCE 200809L

#include "app.h"
#include "apply.h"
#include "fs.h"
#include "raylib.h"

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Image GenerateHexMask(int size, float radius);
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

  DIR *dir = opendir(app.wp_dir);
  struct dirent *ent;

  if (dir == NULL) {
    fprintf(stderr, "Error: Unable to open directory %s\n", app.wp_dir);
    AppShutdown(&app);
    return 1;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (IsSupportedWallpaperFile(ent->d_name)) {
      app.capacity++;
    }
  }
  closedir(dir);

  if (app.capacity == 0) {
    printf("No wallpapers in .png, .jpg, or .jpeg format were found in %s.\n",
           app.wp_dir);
    AppShutdown(&app);
    return 0;
  }

  app.wallpapers = calloc((size_t)app.capacity, sizeof(Wallpaper));
  if (app.wallpapers == NULL) {
    fprintf(stderr,
            "Fatal Error: memory allocation failed (attempted to allocate %d "
            "wallpaper slots)\n",
            app.capacity);
    AppShutdown(&app);
    return 1;
  }

  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
  InitWindow(config->window_width, config->window_height, "wallpicker");

  app.img_size = (int)(HEX_RADIUS * 2.0f);
  app.hex_mask = GenerateHexMask(app.img_size, HEX_RADIUS);

  app.wp_count = 0;
  dir = opendir(app.wp_dir);

  if (dir == NULL) {
    fprintf(stderr, "Error: Unable to reopen directory %s\n", app.wp_dir);
    AppShutdown(&app);
    return 1;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (!IsSupportedWallpaperFile(ent->d_name)) {
      continue;
    }

    BeginDrawing();
    ClearBackground(BLANK);
    DrawText("Loading & Caching Wallpapers...", GetScreenWidth() / 2 - 250,
             GetScreenHeight() / 2, 30, WHITE);
    DrawText(ent->d_name, GetScreenWidth() / 2 - 250,
             GetScreenHeight() / 2 + 40, 20, GRAY);
    EndDrawing();

    char *full_img_path = JoinPath(app.wp_dir, ent->d_name);
    char *cache_img_path = BuildCacheImagePath(app.cache_dir, ent->d_name);

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
        float scaleX = (float)app.img_size / (float)img.width;
        float scaleY = (float)app.img_size / (float)img.height;
        float scale = (scaleX > scaleY) ? scaleX : scaleY;

        int newW = (int)roundf((float)img.width * scale);
        int newH = (int)roundf((float)img.height * scale);

        if (newW < app.img_size) {
          newW = app.img_size;
        }
        if (newH < app.img_size) {
          newH = app.img_size;
        }

        ImageResize(&img, newW, newH);

        int cropX = (newW - app.img_size) / 2;
        int cropY = (newH - app.img_size) / 2;
        ImageCrop(&img,
                  (Rectangle){(float)cropX, (float)cropY, (float)app.img_size,
                              (float)app.img_size});

        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        ImageAlphaMask(&img, app.hex_mask);

        if (!ExportImage(img, cache_img_path)) {
          fprintf(stderr, "Warning: failed to write cache image: %s\n",
                  cache_img_path);
        }
      }
    }

    if (img.data != NULL && img.width > 1 && app.wp_count < app.capacity) {
      int n = snprintf(app.wallpapers[app.wp_count].filename,
                       sizeof(app.wallpapers[app.wp_count].filename), "%s",
                       ent->d_name);

      if (n < 0 || (size_t)n >= sizeof(app.wallpapers[app.wp_count].filename)) {
        fprintf(stderr, "Warning: filename too long, skipping: %s\n",
                ent->d_name);
        UnloadImage(img);
        free(full_img_path);
        free(cache_img_path);
        continue;
      }

      app.wallpapers[app.wp_count].tex = LoadTextureFromImage(img);

      if (app.wallpapers[app.wp_count].tex.id != 0) {
        app.wallpapers[app.wp_count].currentScale = 1.0f;
        app.wallpapers[app.wp_count].currentColor = 130.0f;
        app.wp_count++;
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
  closedir(dir);

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

  if (app->wallpapers != NULL) {
    for (int i = 0; i < app->wp_count; i++) {
      if (app->wallpapers[i].tex.id != 0) {
        UnloadTexture(app->wallpapers[i].tex);
      }
    }

    free(app->wallpapers);
    app->wallpapers = NULL;
  }

  app->wp_count = 0;
  app->capacity = 0;

  if (app->hex_mask.data != NULL) {
    UnloadImage(app->hex_mask);
    app->hex_mask = (Image){0};
  }

  if (app->wp_dir != NULL) {
    free(app->wp_dir);
    app->wp_dir = NULL;
  }

  if (IsWindowReady()) {
    CloseWindow();
  }
}

static Image GenerateHexMask(int size, float radius) {
  Image mask = GenImageColor(size, size, BLANK);
  float cx = (float)size / 2.0f;
  float cy = (float)size / 2.0f;

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
