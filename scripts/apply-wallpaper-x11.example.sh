#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# Hypr-Wallpicker Hook Template (X11)
#
# This script is executed automatically by wallpicker when a wallpaper is clicked.
# To activate it, rename this file to 'apply-wallpaper.sh' and place it in:
# ~/.config/hypr-wallpicker/
# ==============================================================================

# Arguments passed by the C program
WALLPAPER_PATH="${1:-}"
# X/Y coordinates are passed but generally unused by simple X11 setters like feh
# REL_X="${2:-0.5}"
# REL_Y="${3:-0.5}"

if [[ -z "$WALLPAPER_PATH" ]]; then
  echo "usage: $0 <wallpaper_path>" >&2
  exit 1
fi

# ------------------------------------------------------------------------------
# 1. Apply the wallpaper
# ------------------------------------------------------------------------------

if command -v feh >/dev/null 2>&1; then
  feh --bg-fill "$WALLPAPER_PATH"
else
  echo "feh not found. Please install feh or another wallpaper engine." >&2
fi

# ------------------------------------------------------------------------------
# 2. State Management (Symlinking)
# ------------------------------------------------------------------------------
# To maintain a symlink to the current wallpaper so
# other scripts (like lockscreens or fetch tools) can read it easily.
# Generic approach (Recommended):
mkdir -p "$HOME/.cache/wallpicker"
ln -sfn "$WALLPAPER_PATH" "$HOME/.cache/wallpicker/current_wallpaper.png"

# ------------------------------------------------------------------------------
# 3. Color Extraction (Optional, pywal required)
# ------------------------------------------------------------------------------
# Generate system color schemes based on the new wallpaper.
# In X11, 'wal' (pywal) is the most popular equivalent to matugen.
# Uncomment below to enable:

# if command -v wal >/dev/null 2>&1; then
#   wal -i "$WALLPAPER_PATH" -q
# fi

# ------------------------------------------------------------------------------
# 4. Reload UI Components (Post-commands)
# ------------------------------------------------------------------------------
# After colors are generated, reload your desktop components to apply the new theme.
# For example, you can uncomment the ones you actually use in your dotfiles:

# i3wm reload
# command -v i3-msg >/dev/null 2>&1 && i3-msg reload >/dev/null 2>&1 || true

# Polybar restart example
# if command -v polybar-msg >/dev/null 2>&1; then
#   polybar-msg cmd restart
# fi
