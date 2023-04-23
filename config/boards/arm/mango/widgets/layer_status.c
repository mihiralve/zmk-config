/*
 *
 * Copyright (c) 2023 HellTM
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include "layer_status.h"
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

LV_IMG_DECLARE(layer_0);
LV_IMG_DECLARE(layer_1);
LV_IMG_DECLARE(layer_2);
LV_IMG_DECLARE(layer_3);
LV_IMG_DECLARE(layer_unknown);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_status_state {
    uint8_t index;
    const char *label;
};

static struct layer_status_state get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){.index = index, 
                                       .label = zmk_keymap_layer_label(index)};
}

static void set_layer_indicator(lv_obj_t *icon, struct layer_status_state state) {
    switch (state.index) {
    case 0:
        lv_img_set_src(icon, &layer_0);
        break;
    case 1:
        lv_img_set_src(icon, &layer_1);
        break;
    case 2:
        lv_img_set_src(icon, &layer_2);
        break;
    case 3:
        lv_img_set_src(icon, &layer_3);
        break;
    default:
        lv_img_set_src(icon, &layer_unknown);
        break;
    }
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_indicator(widget->obj, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent);

    sys_slist_append(&widgets, &widget->node);

    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget) {
    return widget->obj;
}
