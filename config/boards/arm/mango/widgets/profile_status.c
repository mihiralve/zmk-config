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
#include "profile_status.h"
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_selection_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

LV_IMG_DECLARE(usb_profile);
LV_IMG_DECLARE(bluetooth_profile_1);
LV_IMG_DECLARE(bluetooth_profile_2);
LV_IMG_DECLARE(bluetooth_profile_3);
LV_IMG_DECLARE(bluetooth_profile_4);
LV_IMG_DECLARE(bluetooth_profile_5);
LV_IMG_DECLARE(bluetooth_profile_unknown);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct profile_status_state {
    enum zmk_endpoint selected_endpoint;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t active_profile_index;
};

static struct profile_status_state get_state(const zmk_event_t *_eh) {
    return (struct profile_status_state){.selected_endpoint = zmk_endpoints_selected(),
                                        .active_profile_connected =
                                            zmk_ble_active_profile_is_connected(),
                                        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
                                        .active_profile_index = zmk_ble_active_profile_index()};
    ;
}

static void set_profile_indicator(lv_obj_t *icon, struct profile_status_state state) {
    switch (state.selected_endpoint) {
    case ZMK_ENDPOINT_USB:
        lv_img_set_src(icon, &usb_profile);
        break;
    case ZMK_ENDPOINT_BLE:
        switch (state.active_profile_index) {
        case 0:
            lv_img_set_src(icon, &bluetooth_profile_1);
            break;
        case 1:
            lv_img_set_src(icon, &bluetooth_profile_2);
            break;
        case 2:
            lv_img_set_src(icon, &bluetooth_profile_3);
            break;
        case 3:
            lv_img_set_src(icon, &bluetooth_profile_4);
            break;
        case 4:
            lv_img_set_src(icon, &bluetooth_profile_5);
            break;
        default:
            lv_img_set_src(icon, &bluetooth_profile_unknown);
            break;
        }
    }
}

static void profile_status_update_cb(struct profile_status_state state) {
    struct zmk_widget_profile_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_profile_indicator(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_profile_status, struct profile_status_state,
                            profile_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_profile_status, zmk_endpoint_selection_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_profile_status, zmk_usb_conn_state_changed);
#endif

#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_profile_status, zmk_ble_active_profile_changed);
#endif

int zmk_widget_profile_status_init(struct zmk_widget_profile_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_profile_status_init();
    return 0;
}

lv_obj_t *zmk_widget_profile_status_obj(struct zmk_widget_profile_status *widget) {
    return widget->obj;
}
