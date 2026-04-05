#!/bin/sh

# $1 = absolute path of wallpaper
# $2 = Relative X coordinate of mouse click
# $3 = Relative Y coordinate of mouse click

WALLPAPER_PATH="$1"
REL_X="$2"
REL_Y="$3"

# active awww to change wallpaper
awww img "$WALLPAPER_PATH" --transition-type grow --transition-pos "$REL_X","$REL_Y" \
  --transition-step 30 --transition-duration 1.2 \
  --transition-fps 60 &

# a symbolic link to the current wallpaper for use by other components.
ln -sf "$WALLPAPER_PATH" "$HOME/.config/hypr/current_wallpaper.png"

# extract colors and reload various components
matugen image "$WALLPAPER_PATH" --source-color-index 0
makoctl reload
hyprctl reload
sleep 0.5
"$HOME/.config/waybar/scripts/reload-waybar.sh"
