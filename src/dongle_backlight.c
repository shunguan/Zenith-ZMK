/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * Turn the Prospector's display backlight off while the USB host is asleep.
 *
 * The dongle is permanently USB powered, so it never reaches ZMK_ACTIVITY_SLEEP
 * (ZMK gates deep sleep on !is_usb_power_present()) and ZMK's own
 * BLANK_ON_IDLE only blanks the LCD controller, leaving the backlight lit.
 * The backlight is a separate PWM LED, so something has to drive it to 0
 * explicitly -- and the honest signal for "the computer went to sleep" is USB
 * bus suspend, not key inactivity.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *const pwm_leds_dev = DEVICE_DT_GET_ONE(pwm_leds);

/* Same addressing the module's own brightness.c uses for this LED. */
#define DISP_BL DT_NODE_CHILD_IDX(DT_NODELABEL(disp_bl))

static bool blanked;

static void set_backlight(uint8_t percent) {
    if (!device_is_ready(pwm_leds_dev)) {
        LOG_ERR("backlight device not ready, cannot set %d%%", percent);
        return;
    }

    int ret = led_set_brightness(pwm_leds_dev, DISP_BL, percent);
    if (ret < 0) {
        LOG_ERR("failed to set backlight to %d%%: %d", percent, ret);
    }
}

static int usb_suspend_listener(const zmk_event_t *eh) {
    /*
     * The event payload cannot answer this: ZMK maps USB_DC_SUSPEND,
     * USB_DC_CONFIGURED and USB_DC_RESUME all onto ZMK_USB_CONN_HID, so
     * conn_state does not change across a suspend. The event IS still raised for
     * every status code though (usb_status_cb submits the notifier work
     * unconditionally, SOF aside), so read the raw status here.
     */
    switch (zmk_usb_get_status()) {
    case USB_DC_SUSPEND:
        if (!blanked) {
            blanked = true;
            LOG_INF("USB suspended, backlight off");
            set_backlight(0);
        }
        break;

    case USB_DC_RESUME:
    case USB_DC_CONFIGURED:
        if (blanked) {
            blanked = false;
            LOG_INF("USB resumed, backlight to %d%%", CONFIG_PROSPECTOR_FIXED_BRIGHTNESS);
            set_backlight(CONFIG_PROSPECTOR_FIXED_BRIGHTNESS);
        }
        break;

    default:
        break;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(zenith_dongle_backlight, usb_suspend_listener);
ZMK_SUBSCRIPTION(zenith_dongle_backlight, zmk_usb_conn_state_changed);
