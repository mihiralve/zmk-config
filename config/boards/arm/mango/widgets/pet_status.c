/*
 *
 * Copyright (c) 2023 HellTM
 * SPDX-License-Identifier: MIT
 *
 */

#include <zmk/event_manager.h>
#include <zmk/hid.h>
#include <zmk/events/wpm_state_changed.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/modifiers.h>
#include "pet_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// SIT
LV_IMG_DECLARE(pet_sit_0);
LV_IMG_DECLARE(pet_sit_1);
LV_IMG_DECLARE(pet_sit_2);
const void *pet_sit_images[] = {
    &pet_sit_0, &pet_sit_1, &pet_sit_2,
};

// SLOW TYPING
LV_IMG_DECLARE(pet_walk_0);
LV_IMG_DECLARE(pet_walk_1);
LV_IMG_DECLARE(pet_walk_2);
const void *pet_walk_images[] = {
    &pet_walk_0, &pet_walk_1, &pet_walk_2,
};

// FAST TYPING
LV_IMG_DECLARE(pet_run_0);
LV_IMG_DECLARE(pet_run_1);
LV_IMG_DECLARE(pet_run_2);
const void *pet_run_images[] = {
    &pet_run_0, &pet_run_1, &pet_run_2,
};

// JUMP
// note that this animation requires 4 frames
LV_IMG_DECLARE(pet_jump_0);
LV_IMG_DECLARE(pet_jump_1);
LV_IMG_DECLARE(pet_jump_2);
LV_IMG_DECLARE(pet_jump_3);
const void *jump_images[] = {
    &pet_jump_0, &pet_jump_1, &pet_jump_2, &pet_jump_3,
};

// BARK
LV_IMG_DECLARE(pet_bark_0);
LV_IMG_DECLARE(pet_bark_1);
LV_IMG_DECLARE(pet_bark_2);
const void *pet_bark_images[] = {
    &pet_bark_0, &pet_bark_1, &pet_bark_2,
};

// DOWN
LV_IMG_DECLARE(pet_down_0);
LV_IMG_DECLARE(pet_down_1);
LV_IMG_DECLARE(pet_down_2);
const void *pet_down_images[] = {
    &pet_down_0, &pet_down_1, &pet_down_2,
};


/* PET STATE */
enum pet_wpm_state {
    unknown,
    sit,
    walk,
    run,
} current_pet_wpm_state = unknown;

enum pet_action_state {
    no_action,
    down,
    bark,
    jump,
} current_pet_action_state = no_action;
bool showed_jump = true;

lv_anim_t anim;
const void **images;
int frame_to_show = 0;
int current_frame_duration = 150;


void animate_images(void * var, int value) {
    lv_obj_t *obj = (lv_obj_t *)var;

    frame_to_show = value;

    // end jump animation if it was shown
    if (frame_to_show == 0 && showed_jump == true) {
        showed_jump = false;
        current_pet_action_state == no_action;
    }

    // set action based on modifiers if pet is not jumping
    if (current_pet_action_state != jump) {
        set_pet_action_state_based_on_modifiers();
    }

    // change state only on frame 0 if pet is jumping
    if (current_pet_action_state != jump || frame_to_show == 0) {
        if (current_pet_action_state == jump) {
            images = jump_images;
        } else if (current_pet_action_state == down) {
            images = pet_down_images;
        } else if (current_pet_action_state == bark) {
            images = pet_bark_images;
        } else if (current_pet_wpm_state == sit) {
            images = pet_sit_images;
        } else if (current_pet_wpm_state == walk) {
            images = pet_walk_images;
        } else if (current_pet_wpm_state == run) {
            images = pet_run_images;
        }
    }

    // signal the jump animation has been shown completely
    if (current_pet_action_state == jump && frame_to_show == 3) {
        showed_jump = true;
    }

    // This makes so the middle frame is reused as 4th frame allowing smoother animation.
    // NOTE that the jump animation is excluded from this behaviour.
    // More info about this in icons/pet_status.c
    if (frame_to_show == 3 && current_pet_action_state != jump) {
        frame_to_show = 1;
    }

    // set the image to show next
    lv_img_set_src(obj, images[frame_to_show]);
}

void set_pet_action_state_based_on_modifiers() {

    // control and gui -> down
    // shift -> bark
    if (((zmk_hid_get_explicit_mods() & MOD_LCTL) != 0) || ((zmk_hid_get_explicit_mods() & MOD_RCTL) != 0) || 
        ((zmk_hid_get_explicit_mods() & MOD_LGUI) != 0) || ((zmk_hid_get_explicit_mods() & MOD_RGUI) != 0)) {
        current_pet_action_state = down;
    } else if (((zmk_hid_get_explicit_mods() & MOD_LSFT) != 0) || ((zmk_hid_get_explicit_mods() & MOD_RSFT) != 0)) {
        current_pet_action_state = bark;
    } else {
        current_pet_action_state = no_action;
    }

    // TODO add caps lock behavior here
}

void init_anim(struct zmk_widget_pet_status *widget) {
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, widget->obj);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t) animate_images);
    lv_anim_set_time(&anim, current_frame_duration * 3);
    lv_anim_set_values(&anim, 0, 3);
    lv_anim_set_delay(&anim, 0);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&anim, current_frame_duration);
    lv_anim_start(&anim);
}

int zmk_widget_pet_status_init(struct zmk_widget_pet_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent);

    current_pet_wpm_state = sit;
    init_anim(widget);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_pet_status_obj(struct zmk_widget_pet_status *widget) { return widget->obj; }

int pet_wpm_event_listener(const zmk_event_t *eh) {
    struct zmk_widget_pet_status *widget;
    struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {

        // Update pet status based on WPM.
        // Configurable in Kconfig.defconfig
        if (ev->state < CONFIG_CUSTOM_WIDGET_PET_WALK_WPM) {
            current_pet_wpm_state = sit;
        } else if (ev->state < CONFIG_CUSTOM_WIDGET_PET_RUN_WPM) {
            current_pet_wpm_state = walk;
        } else {
            current_pet_wpm_state = run;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

int pet_keycode_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    // key presses
    if (ev && ev->state) {
        switch (ev->keycode) {
            case HID_USAGE_KEY_KEYBOARD_SPACEBAR:
                current_pet_action_state = jump;
                showed_jump = false;
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