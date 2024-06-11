/*
 *
 * Copyright (c) 2023 HellTM
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include "battery_status.h"
#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_status_state {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

LV_IMG_DECLARE(battery_charging);
LV_IMG_DECLARE(battery_99);
LV_IMG_DECLARE(battery_93);
LV_IMG_DECLARE(battery_87);
LV_IMG_DECLARE(battery_80);
LV_IMG_DECLARE(battery_73);
LV_IMG_DECLARE(battery_67);
LV_IMG_DECLARE(battery_60);
LV_IMG_DECLARE(battery_53);
LV_IMG_DECLARE(battery_47);
LV_IMG_DECLARE(battery_40);
LV_IMG_DECLARE(battery_33);
LV_IMG_DECLARE(battery_27);
LV_IMG_DECLARE(battery_20);
LV_IMG_DECLARE(battery_13);
LV_IMG_DECLARE(battery_7);
LV_IMG_DECLARE(battery_2);

static void set_battery_symbol(lv_obj_t *icon, struct battery_status_state state) {
    uint8_t level = state.level;

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)

    bool usb_present = state.usb_present;

    if (usb_present) {
        lv_img_set_src(icon, &battery_charging);
    } else {
        if (level >= 99) {
            lv_img_set_src(icon, &battery_99);
        } else if (level >= 93) {
            lv_img_set_src(icon, &battery_93);
        } else if (level >= 87) {
            lv_img_set_src(icon, &battery_87);
        } else if (level >= 80) {
            lv_img_set_src(icon, &battery_80);
        } else if (level >= 73) {
            lv_img_set_src(icon, &battery_73);
        } else if (level >= 67) {
            lv_img_set_src(icon, &battery_67);
        } else if (level >= 60) {
            lv_img_set_src(icon, &battery_60);
        } else if (level >= 53) {
            lv_img_set_src(icon, &battery_53);
        } else if (level >= 47) {
            lv_img_set_src(icon, &battery_47);
        } else if (level >= 40) {
            lv_img_set_src(icon, &battery_40);                
        } else if (level >= 33) {
            lv_img_set_src(icon, &battery_33);
        } else if (level >= 27) {
            lv_img_set_src(icon, &battery_27);
        } else if (level >= 20) {
            lv_img_set_src(icon, &battery_20);
        } else if (level >= 13) {
            lv_img_set_src(icon, &battery_13);
        } else if (level >= 7) {
            lv_img_set_src(icon, &battery_7);
        } else {
            lv_img_set_src(icon, &battery_2);
        }
    }

#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
}

void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state) {
        .level = bt_bas_get_battery_level(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

int zmk_widget_battery_status_init(struct zmk_widget_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();

    return 0;
}

lv_obj_t *zmk_widget_battery_status_obj(struct zmk_widget_battery_status *widget) {
    return widget->obj;
}
