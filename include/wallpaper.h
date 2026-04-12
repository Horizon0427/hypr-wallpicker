#ifndef WALLPAPER_H
#define WALLPAPER_H

#include "app.h"
#include <stdbool.h>

bool InitWallpaperResources(App *app);
bool LoadWallpapers(App *app);
void UnloadWallpapers(App *app);

#endif
