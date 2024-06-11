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
const void *pet_jump_images[] = {
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

lv_anim_t anim;
const void **images;
int current_frame = 0;
bool jump_interrupt = false;
bool restart_animation = false;
bool allow_frame_duration_change = false;
int max_frame_duration = 300;
int min_frame_duration = 100;
int max_jump_frame_duration = 150;
int current_frame_duration = 150;


lv_obj_t *zmk_widget_pet_status_obj(struct zmk_widget_pet_status *widget) { return widget->obj; }

void animate_images(void * var, int value) {
    lv_obj_t *obj = (lv_obj_t *)var;
    int frame_to_show = value;
    current_frame = value;

    // Recreate animation based on WPM
    // This only happens on frame 0
    if(restart_animation || (allow_frame_duration_change && current_frame == 0)) {
        // reset bool if animation is restarted
        restart_animation = false;

        // prevent frame duration change until next cycle
        allow_frame_duration_change = false;

        // make sure there is a frame at screen
        if (current_frame == 3 && images != pet_jump_images) {
                frame_to_show = 1;
        }
        lv_img_set_src(obj, images[frame_to_show]);

        // restart animation
        struct zmk_widget_pet_status *widget;
        SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
            lv_anim_del(var, (lv_anim_exec_xcb_t) animate_images);
            init_anim(widget);
        }
    } else {
        if (current_pet_action_state == jump) {

            // reset jump variable
            if (current_frame == 3 && jump_interrupt == false && images == pet_jump_images) {
                current_pet_action_state = no_action;
                set_pet_action_state_based_on_modifiers();
            }

            // start jump only on frame 0
            if (current_frame == 0) {
                images = pet_jump_images;
                jump_interrupt == true;
            }

            // reset interrupt value
            if (current_frame == 2 && images == pet_jump_images) {
                jump_interrupt == false;
            }

        } else {

            // Change image set every frame.
            if (current_pet_action_state == down) {
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

            set_pet_action_state_based_on_modifiers();

            if (current_pet_action_state == jump) {
                restart_animation = true;
            }
        }
    }

    // Signal a frame duration change during the next frame 0
    if (current_frame == 3) {
        allow_frame_duration_change = true;
    }

    // This makes so the middle frame is reused as 4th frame allowing smoother animation.
    // NOTE that the jump animation is excluded from this behaviour.
    // More info about this in icons/pet_status.c
    if (current_frame == 3 && images != pet_jump_images) {
        frame_to_show = 1;
    }

    // set the image to show next
    lv_img_set_src(obj, images[frame_to_show]);
}

void init_anim(struct zmk_widget_pet_status *widget) {
    // Initialize the animation
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

void set_pet_action_state_based_on_modifiers() {
    // This allows better precision for held down keys
    // control and gui -> down
    // alt and shift -> bark
    if (((zmk_hid_get_explicit_mods() & MOD_LCTL) != 0) || ((zmk_hid_get_explicit_mods() & MOD_RCTL) != 0) || 
        ((zmk_hid_get_explicit_mods() & MOD_LGUI) != 0) || ((zmk_hid_get_explicit_mods() & MOD_RGUI) != 0)) {
        current_pet_action_state = down;
    } else if (((zmk_hid_get_explicit_mods() & MOD_LSFT) != 0) || ((zmk_hid_get_explicit_mods() & MOD_RSFT) != 0) ||
               ((zmk_hid_get_explicit_mods() & MOD_LALT) != 0) || ((zmk_hid_get_explicit_mods() & MOD_RALT) != 0)) {
        current_pet_action_state = bark;
    } else {
        if (current_pet_action_state != jump) {
            current_pet_action_state = no_action;
        }
    }

    // TODO add caps lock behavior here
}

int pet_wpm_event_listener(const zmk_event_t *eh) {
    struct zmk_widget_pet_status *widget;
    struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {

        // Calculate current frame duration
        current_frame_duration = (max_frame_duration - (ev->state * 3));

        // Clamp the frame duration value
        if (current_frame_duration <= min_frame_duration) {
            current_frame_duration = min_frame_duration;
        }

        // Max jump frame duration
        if (current_pet_action_state == jump && current_frame_duration > max_jump_frame_duration) {
            current_frame_duration = max_jump_frame_duration;
        }

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
    struct zmk_widget_pet_status *widget;
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    // key presses
    if (ev) {
        switch (ev->keycode) {
            case HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE:
            case HID_USAGE_KEY_KEYBOARD_ESCAPE:
            case HID_USAGE_KEY_KEYBOARD_RETURN_ENTER:
                if (ev->state) {
                    current_pet_action_state = jump;
                }
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