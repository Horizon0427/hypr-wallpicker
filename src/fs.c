#define _POSIX_C_SOURCE 200809L

#include "fs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>

static bool EnsureDirExists(const char *dir_path) {
  struct stat st = {0};
  
  if (stat(dir_path, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      fprintf(stderr, "Error: '%s' exists but is not a directory\n", dir_path);     
      
      return false;
    }
  return true;
 }

  if (mkdir(dir_path, 0755) !=0) {
  fprintf(stderr, "Error trying to create directory '%s': %s\n", dir_path, strerror(errno));

  return false;
}

return true;
}


bool GetCacheDir(char *out_path, size_t max_len) {
  const char *home = getenv("HOME");
  if (home == NULL) {
    fprintf(stderr, "Error: unable to find HOME environment variable\n");
    return false;
  }

  int n = snprintf(out_path, max_len, "%s/.cache/wallpicker", home);
  if (n < 0 || (size_t)n >= max_len) {
    fprintf(stderr, "Error: cache path is too long\n");
    return false;
  }

  return EnsureDirExists(out_path);
}

char *GetDefaultWallpaperDir(void) {
  const char *home = getenv("HOME");
  if (home == NULL) {
    fprintf(stderr, "Error: unable to find HOME environment variable\n");
    return NULL;
  }

  int needed = snprintf(NULL, 0, "%s/Pictures/wallpapers", home);
  if (needed < 0) {
    fprintf(stderr, "Error: failed to build wallpaper directory path\n");
    return NULL;
  }

  char *out = malloc((size_t)needed + 1);
  if (out == NULL) {
    fprintf(stderr, "Error: out of memory\n");
    return NULL;
  }

  snprintf(out, (size_t)needed + 1, "%s/Pictures/wallpapers", home);
  return out;
}

char *JoinPath(const char *dir, const char *name) {
  size_t len_dir = strlen(dir);
  size_t len_name = strlen(name);
  bool need_slash = (len_dir > 0 && dir[len_dir - 1] != '/');

  size_t total = len_dir + (need_slash ? 1u : 0u) + len_name + 1u;

  char *out = malloc(total);
  if (out == NULL) {
    fprintf(stderr, "Error: out of memory\n");
    return NULL;
  }

  memcpy(out, dir, len_dir);
  size_t pos = len_dir;

  if (need_slash) {
    out[pos++] = '/';
  }

  memcpy(out + pos, name, len_name);
  out[pos + len_name] = '\0';

  return out;
}

char *BuildCacheImagePath(const char *cache_dir, const char *filename) {
  const char *dot = strrchr(filename, '.');
  size_t base_len = (dot != NULL && dot != filename) ? (size_t)(dot - filename)
                                                     : strlen(filename);

  int needed =
      snprintf(NULL, 0, "%s/%.*s.png", cache_dir, (int)base_len, filename);
  if (needed < 0) {
    fprintf(stderr, "Error: failed to build cache image path\n");
    return NULL;
  }

  char *out = malloc((size_t)needed + 1);
  if (out == NULL) {
    fprintf(stderr, "Error: out of memory\n");
    return NULL;
  }

  snprintf(out, (size_t)needed + 1, "%s/%.*s.png", cache_dir, (int)base_len,
           filename);
  return out;
}

bool HasExtension(const char *filename, const char *ext) {
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return false;
  return strcasecmp(dot, ext) == 0;
}

bool IsSupportedWallpaperFile(const char *filename) {
  return HasExtension(filename, ".png") || HasExtension(filename, ".jpg") ||
         HasExtension(filename, ".jpeg");
}
