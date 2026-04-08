#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# Hypr-Wallpicker Hook Template (Wayland)
# ==============================================================================

# Arguments passed by the C program
WALLPAPER_PATH="${1:-}"
REL_X="${2:-0.5}"
REL_Y="${3:-0.5}"

if [[ -z "$WALLPAPER_PATH" ]]; then
  echo "usage: $0 <wallpaper_path> [rel_x] [rel_y]" >&2
  exit 1
fi

# ------------------------------------------------------------------------------
# Apply the wallpaper
# ------------------------------------------------------------------------------
# Here we use 'awww' as the default engine to utilize the X/Y click coordinates
# for a growing animation.

if command -v awww >/dev/null 2>&1; then
  awww img "$WALLPAPER_PATH" \
    --transition-type grow \
    --transition-pos "$REL_X,$REL_Y" \
    --transition-step 30 \
    --transition-duration 1.2 \
    --transition-fps 60
else
  echo "awww not found. Please install a wallpaper engine." >&2
fi

# ------------------------------------------------------------------------------
# State Management (Symlinking)
# ------------------------------------------------------------------------------
# To maintain a symlink to the current wallpaper so
# other scripts (like lockscreens or fetch tools) can read it easily.
# Generic approach (Recommended):
mkdir -p "$HOME/.cache/wallpicker"
ln -sfn "$WALLPAPER_PATH" "$HOME/.cache/wallpicker/current_wallpaper.png"

# Hyprland specific approach
# You can create a soft-link for other use by uncommenting the code below.

# mkdir -p "$HOME/.config/hypr"
# ln -sfn "$WALLPAPER_PATH" "$HOME/.config/hypr/current_wallpaper.png"

# ------------------------------------------------------------------------------
# 3. Color Extraction (Optional, matugen required)
# ------------------------------------------------------------------------------
# Generate system color schemes based on the new wallpaper.
if command -v matugen >/dev/null 2>&1; then
  matugen image "$WALLPAPER_PATH" --source-color-index 0
fi

# ------------------------------------------------------------------------------
# 4. Reload UI Components (Post-commands)
# ------------------------------------------------------------------------------

# command -v makoctl >/dev/null 2>&1 && makoctl reload
# command -v swaync-client >/dev/null 2>&1 && swaync-client -rs
# command -v hyprctl >/dev/null 2>&1 && hyprctl reload
