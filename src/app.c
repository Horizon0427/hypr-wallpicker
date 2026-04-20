#define _POSIX_C_SOURCE 200809L

#include "app.h"
#include "apply.h"
#include "fs.h"
#include "raylib.h"
#include "ui.h"
#include "wallpaper.h"

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

void FreeSelectionResult(SelectionResult *result) {
  if (result != NULL) {
    if (result->full_target_path != NULL) {
      free(result->full_target_path);
      result->full_target_path = NULL;
    }
    result->valid = false;
    result->rel_x = 0.0f;
    result->rel_y = 0.0f;
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

  if (!GetCacheDir(app.cache_dir, sizeof(app.cache_dir))) {
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

  SelectionResult result = RunUI(&app, config);

  if (result.valid) {
    ApplyWallpaper(result.full_target_path, result.rel_x, result.rel_y);
  }

  FreeSelectionResult(&result);

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
