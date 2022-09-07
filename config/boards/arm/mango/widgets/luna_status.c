#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <dt-bindings/zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/modifiers.h>
#include <zmk/display/widgets/luna_status.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/* images and animations */
// sits
LV_IMG_DECLARE(luna_sit1);
LV_IMG_DECLARE(luna_sit2);
const void *sits_images[] = {
    &luna_sit1, &luna_sit2,
};

// walks
LV_IMG_DECLARE(luna_walk1);
LV_IMG_DECLARE(luna_walk2);
const void *walks_images[] = {
    &luna_walk1, &luna_walk2,
};

// runs
LV_IMG_DECLARE(luna_run1);
LV_IMG_DECLARE(luna_run2);
const void *runs_images[] = {
    &luna_run1, &luna_run2,
};

// jumps
const void *jumps_images[] = {
    &luna_run1, &luna_run2,
};

// barks
LV_IMG_DECLARE(luna_bark1);
LV_IMG_DECLARE(luna_bark2);
const void *barks_images[] = {
    &luna_bark1, &luna_bark2,
};

// sneaks
LV_IMG_DECLARE(luna_sneak1);
LV_IMG_DECLARE(luna_sneak2);
const void *sneaks_images[] = {
    &luna_sneak1, &luna_sneak2,
};
/* end images and animations */

/* widget states */
enum luna_running_state {
    unknown,
    sits,
    walks,
    runs,
} current_luna_running_state = unknown;

enum luna_action_state {
    no_action,
    sneaks,
    barks,
    jumps,
} current_luna_action_state = no_action, anim_luna_action_state = no_action;

lv_anim_t anim;
const void **images;
/* end widget states */

void animate_images(void *var, lv_anim_value_t value) {
    lv_obj_t *obj = (lv_obj_t *)var;

    // end of jump, restore the y pos
    if (value == 1 && anim_luna_action_state == jumps) {
        lv_coord_t luna_y = lv_obj_get_y(obj);
        lv_obj_set_y(obj, luna_y + 10);
    }

    // change action on frame 0
    if (value == 0) {
        anim_luna_action_state = current_luna_action_state;

        if (current_luna_action_state == jumps) {
            images = jumps_images;
            lv_coord_t luna_y = lv_obj_get_y(obj);
            lv_obj_set_y(obj, luna_y - 10);
        } else if (current_luna_action_state == sneaks) {
            images = sneaks_images;
        } else if (current_luna_action_state == barks) {
            images = barks_images;
        } else if (current_luna_running_state == sits) {
            images = sits_images;
        } else if (current_luna_running_state == walks) {
            images = walks_images;
        } else if (current_luna_running_state == runs) {
            images = runs_images;
        }
        current_luna_action_state = no_action;
    }

    lv_img_set_src(obj, images[value]);
}

void init_anim(struct zmk_widget_luna_status *widget) {
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, widget->obj);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t) animate_images);
    lv_anim_set_time(&anim, CONFIG_ZMK_WIDGET_LUNA_WIDGET_FRAME_DURATION);
    lv_anim_set_values(&anim, 0, 1);
    lv_anim_set_delay(&anim, CONFIG_ZMK_WIDGET_LUNA_WIDGET_FRAME_DURATION);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&anim, CONFIG_ZMK_WIDGET_LUNA_WIDGET_FRAME_DURATION);
    lv_anim_start(&anim);
}

int zmk_widget_luna_status_init(struct zmk_widget_luna_status *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent, NULL);

    lv_img_set_auto_size(widget->obj, true);

    current_luna_running_state = sits;
    init_anim(widget);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_luna_status_obj(struct zmk_widget_luna_status *widget) { return widget->obj; }

int luna_wpm_event_listener(const zmk_event_t *eh) {
    struct zmk_widget_luna_status *widget;
    struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        LOG_DBG("Set the WPM %d", ev->state);

        // update luna status based on wpm
        if (ev->state < CONFIG_ZMK_WIDGET_LUNA_WIDGET_WALK_WPM) {
            current_luna_running_state = sits;
        } else if (ev->state < CONFIG_ZMK_WIDGET_LUNA_WIDGET_RUN_WPM) {
            current_luna_running_state = walks;
        } else {
            current_luna_running_state = runs;
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

int luna_keycode_event_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev) {
        switch (ev->keycode) {
            case HID_USAGE_KEY_KEYBOARD_SPACEBAR:
                LOG_DBG("Got SPACE, telling luna to jump");
                current_luna_action_state = jumps;
                break;
            case HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE:
            case HID_USAGE_KEY_KEYBOARD_DELETE_FORWARD:
                LOG_DBG("Got backspace/delete, telling luna to bark");
                current_luna_action_state = barks;
                break;
            case HID_USAGE_KEY_KEYBOARD_LEFTCONTROL:
            case HID_USAGE_KEY_KEYBOARD_LEFTSHIFT:
            case HID_USAGE_KEY_KEYBOARD_LEFTALT:
            case HID_USAGE_KEY_KEYBOARD_LEFT_GUI:
            case HID_USAGE_KEY_KEYBOARD_RIGHTCONTROL:
            case HID_USAGE_KEY_KEYBOARD_RIGHTSHIFT:
            case HID_USAGE_KEY_KEYBOARD_RIGHTALT:
            case HID_USAGE_KEY_KEYBOARD_RIGHT_GUI:
                LOG_DBG("Got modifiers, telling luna to sneak");
                current_luna_action_state = sneaks;
                break;
            default:
                break;
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(zmk_widget_luna_status, luna_wpm_event_listener);
ZMK_SUBSCRIPTION(zmk_widget_luna_status, zmk_wpm_state_changed);

ZMK_LISTENER(keycode_state, luna_keycode_event_listener);
ZMK_SUBSCRIPTION(keycode_state, zmk_keycode_state_changed);