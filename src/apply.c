#define _POSIX_C_SOURCE 200809L

#include "apply.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static bool RunUserHook(const char *wall_path, const char *relX_str,
                        const char *relY_str) {
  const char *home = getenv("HOME");
  if (home == NULL) {
    return false;
  }

  char hook_path[PATH_MAX];
  int n = snprintf(hook_path, sizeof(hook_path),
                   "%s/.config/hypr-wallpicker/apply-wallpaper.sh", home);

  if (n < 0 || (size_t)n >= sizeof(hook_path)) {
    return false;
  }

  if (access(hook_path, X_OK) == 0) {
    execl("/bin/sh", "sh", hook_path, wall_path, relX_str, relY_str, NULL);
    perror("execl user hook failed");
  }
  return false;
}

static bool RunBuiltinFallback(const char *wall_path, const char *relX_str,
                               const char *relY_str) {

  char script[] =
      "if command -v awww >/dev/null 2>&1; then "
      "  awww img \"$1\" --transition-type grow --transition-pos \"$2,$3\" "
      "    --transition-step 30 --transition-duration 1.2 "
      "    --transition-fps 60; "
      "fi; "

      //        ***LEGACY FALLBACK LEFT IN FOR NOW***

      /*    "mkdir -p \"$HOME/.config/hypr\"; "
            "ln -sfn \"$1\" \"$HOME/.config/hypr/current_wallpaper.png\"; "
            "if command -v matugen >/dev/null 2>&1; then "
            "  matugen image \"$1\" --source-color-index 0; "
            "fi; "
            "command -v makoctl >/dev/null 2>&1 && makoctl reload; "
            "command -v hyprctl >/dev/null 2>&1 && hyprctl reload; "
            "if [ -x \"$HOME/.config/waybar/scripts/reload-waybar.sh\" ]; then "
            "  \"$HOME/.config/waybar/scripts/reload-waybar.sh\"; "
            "fi"
      */
      ;

  execl("/bin/sh", "sh", "-c", script, "--", wall_path, relX_str, relY_str,
        NULL);

  perror("execl built-in fallback failed");
  return false;
}

bool ApplyWallpaper(const char *wall_path, float rel_x, float rel_y) {
  char relX_str[32];
  char relY_str[32];

  snprintf(relX_str, sizeof(relX_str), "%.3f", rel_x);
  snprintf(relY_str, sizeof(relY_str), "%.3f", rel_y);

  pid_t pid = fork();

  if (pid == 0) {
    if (!RunUserHook(wall_path, relX_str, relY_str)) {
      RunBuiltinFallback(wall_path, relX_str, relY_str);
    }

    _exit(1);
  } else if (pid < 0) {
    perror("fork failed");
    return false;
  }

  printf("Wallpaper change triggered: %s\n", wall_path);
  return true;
}
