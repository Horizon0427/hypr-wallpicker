# [hypr-wallpicker](https://github.com/Horizon0427/hypr-wallpicker) 

Discussion Type

Show & Tell

### Discussion Content

## Share your hypr-wallpicker setup / apply-wallpaper.sh workflow

This thread is for showing how you use **hypr-wallpicker** in your actual setup.

If you’ve customized your `apply-wallpaper.sh` workflow, feel free to share it here.

## What would be useful to share

- your compositor / WM / DE
  - Hyprland
  - i3
  - Sway
  - Gnome
  - KDE
  - etc.
- whether you’re on:
  - **Wayland**
  - **X11**
- your wallpaper engine / setter
  - `awww`
  - `swww`
  - `feh`
  - `nitrogen`
  - etc.
- post-apply actions
  - symlink current wallpaper
  - reload bars / notifications
  - regenerate lockscreen assets
  - reload compositor config
  - update terminal colors
  - trigger theme scripts

## Example things to post

- your `apply-wallpaper.sh`
- your hook chain / post-apply commands
- screenshots or clips of your workflow
- “first-run sane defaults” suggestions for your backend
- what you think should be built-in vs left to user hooks

## Why this thread matters

The project direction is:

- **small core**
- **sane defaults**
- **user hooks**
- **composable integrations**

Real-world workflows are useful for deciding:

- what should stay example-only
- what should become a documented preset
- what (if anything) deserves built-in fallback behavior for:
  - **Wayland** (Hyprland-first)
  - **X11** (i3-first)

## Please avoid posting

- secrets / tokens / private paths you don’t want public
- machine-specific sensitive info
- giant unrelated dotfile dumps without context

If you post a script, a short explanation of what it does is appreciated.

Would especially love to see:
- Hyprland + `awww` / `swww` setups
- i3 + `feh` + `picom` setups
- Custom setup's integrating quickshell etc...  
- lockscreen + bar + notification reload flows
