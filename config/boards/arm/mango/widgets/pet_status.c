/*
 *
 * Copyright (c) 2023 HellTM
 * SPDX-License-Identifier: MIT
 *
 */

#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/modifiers.h>
#include <zmk/display/widgets/pet_status.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// SIT
LV_IMG_DECLARE(pet_sit_1);
LV_IMG_DECLARE(pet_sit_2);
const void *pet_sit_images[] = {
    &pet_sit_1, &pet_sit_2,
};

// WALK
LV_IMG_DECLARE(pet_walk_1);
LV_IMG_DECLARE(pet_walk_2);
const void *pet_walk_images[] = {
    &pet_walk_1, &pet_walk_2,
};

// RUN
LV_IMG_DECLARE(pet_run_1);
LV_IMG_DECLARE(pet_run_2);
const void *pet_run_images[] = {
    &pet_run_1, &pet_run_2,
};

// SPACE
const void *space_images[] = {
    &pet_run1,
};

// SHIFT
LV_IMG_DECLARE(pet_shift_1);
LV_IMG_DECLARE(pet_shift_2);
const void *pet_shift_images[] = {
    &pet_shift_1, &pet_shift_2,
};

// CTRL
LV_IMG_DECLARE(pet_ctrl_1);
LV_IMG_DECLARE(pet_ctrl_2);
const void *pet_ctrl_images[] = {
    &pet_ctrl_1, &pet_ctrl_2,
};


/* PET STATE */
enum pet_running_state {
    unknown,
    sit,
    walk,
    run,
} current_pet_running_state = unknown;

enum pet_action_state {
    no_action,
    ctrl,
    shift,
    space,
} current_pet_action_state = no_action, anim_pet_action_state = no_action;

lv_anim_t anim;
const void **images;


void animate_images(void *var, lv_anim_value_t value) {
    lv_obj_t *obj = (lv_obj_t *)var;

    // end of space action, restore the y pos
    if (value == 1 && anim_pet_action_state == space) {
        lv_coord_t pet_y = lv_obj_get_y(obj);
        lv_obj_set_y(obj, pet_y + 10);
    }

    // change action on frame 0
    if (value == 0) {
        anim_pet_action_state = current_pet_action_state;

        if (current_pet_action_state == space) {
            images = space_images;
            lv_coord_t pet_y = lv_obj_get_y(obj);
            lv_obj_set_y(obj, pet_y - 10);
        } else if (current_pet_action_state == ctrl) {
            images = ctrl_images;
        } else if (current_pet_action_state == shift) {
            images = shift_images;
        } else if (current_pet_running_state == sit) {
            images = sit_images;
        } else if (current_pet_running_state == walk) {
            images = walk_images;
        } else if (current_pet_running_state == run) {
            images = run_images;
        }
        current_pet_action_state = no_action;
    }

    lv_img_set_src(obj, images[value]);
}

void init_anim(struct zmk_widget_pet_status *widget) {
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, widget->obj);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t) animate_images);
    lv_anim_set_time(&anim, CONFIG_CUSTOM_WIDGET_PET_WIDGET_FRAME_DURATION);
    lv_anim_set_values(&anim, 0, 1);
    lv_anim_set_delay(&anim, CONFIG_CUSTOM_WIDGET_PET_WIDGET_FRAME_DURATION);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&anim, CONFIG_CUSTOM_WIDGET_PET_WIDGET_FRAME_DURATION);
    lv_anim_start(&anim);
}

int zmk_widget_pet_status_init(struct zmk_widget_pet_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent, NULL);

    lv_img_set_auto_size(widget->obj, true);

    current_pet_running_state = sit;
    init_anim(widget);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_pet_status_obj(struct zmk_widget_pet_status *widget) { return widget->obj; }

int pet_wpm_event_listener(const zmk_event_t *eh) {
    struct zmk_widget_pet_status *widget;
    struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        LOG_DBG("Set the WPM %d", ev->state);

        // update pet status based on wpm
        if (ev->state < CONFIG_CUSTOM_WIDGET_PET_WIDGET_WALK_WPM) {
            current_pet_running_state = sits;
        } else if (ev->state < CONFIG_CUSTOM_WIDGET_PET_WIDGET_RUN_WPM) {
            current_pet_running_state = walk;
        } else {
            current_pet_running_state = run;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

int pet_keycode_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev) {
        switch (ev->keycode) {
            case HID_USAGE_KEY_KEYBOARD_SPACEBAR:
                current_pet_action_state = space;
                break;
            case HID_USAGE_KEY_KEYBOARD_RIGHTSHIFT:
            case HID_USAGE_KEY_KEYBOARD_LEFTSHIFT:
                current_pet_action_state = shift;
                break;
            case HID_USAGE_KEY_KEYBOARD_LEFTCONTROL:
            case HID_USAGE_KEY_KEYBOARD_RIGHTCONTROL:
                current_pet_action_state = ctrl;
                break;
            default:
                break;
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(zmk_widget_pet_status, pet_wpm_event_listener);
ZMK_SUBSCRIPTION(zmk_widget_pet_status, zmk_wpm_state_changed);

ZMK_LISTENER(keycode_state, pet_keycode_event_listener);
ZMK_SUBSCRIPTION(keycode_state, zmk_keycode_state_changed);