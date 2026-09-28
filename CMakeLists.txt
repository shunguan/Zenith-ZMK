# ZMK's own headers (zmk/usb.h, zmk/event_manager.h, ...) live in the app tree.
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

zephyr_library()

if(CONFIG_ZENITH_DONGLE_BACKLIGHT_OFF_ON_USB_SUSPEND)
  zephyr_library_sources(src/dongle_backlight.c)
else()
  # zephyr_library() with no sources fails to configure, and this file is
  # compiled out for the halves and settings_reset.
  zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)
endif()
