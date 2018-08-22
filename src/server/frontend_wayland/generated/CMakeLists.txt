add_library(
  mirfrontend-waylandgenerated OBJECT

  wayland.c                     wayland.h                       wayland_wrapper.h
  xdg-shell-unstable-v6.c       xdg-shell-unstable-v6.h         xdg-shell-unstable-v6_wrapper.h
  xdg-shell.c                   xdg-shell.h                     xdg-shell_wrapper.h
  wlr-layer-shell-unstable-v1.c wlr-layer-shell-unstable-v1.h   wlr-layer-shell-unstable-v1_wrapper.h
)

add_dependencies(
  mirfrontend-waylandgenerated
  refresh-wayland-wrapper
)
