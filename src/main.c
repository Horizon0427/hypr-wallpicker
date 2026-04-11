#define _POSIX_C_SOURCE 200809L

#include "app.h"

int main(int argc, char **argv) {
  AppConfig config = AppConfigFromArgs(argc, argv);
  return AppRun(&config);
}
