#include "fs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static bool EnsureDirExists(const char *dir_path) {
  struct stat st = {0};
  if (stat(dir_path, &st) == -1) {
    if (mkdir(dir_path, 0755) != 0) {
      fprintf(stderr, "Error tring to create directory '%s' : %s\n", dir_path,
              strerror(errno));
      return false;
    }
  }
  return true;
}

bool GetCacheDir(char *out_path, size_t max_len) {
  const char *home = getenv("HOME");
  if (home == NULL) {
    fprintf(stderr, "Error unable to find HOME environment variable\n");
    return false;
  }

  snprintf(out_path, max_len, "%s/.cache/wallpicker", home);

  return EnsureDirExists(out_path);
}
