/*
Copyright 2023 Morgan Venable @_claussen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H
#include <stdbool.h>
#include <stdint.h>
#include "svalboard.h"
#include "features/achordion.h"
#include "keymap_support.h"

// in keymap.c:
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
void pointing_device_init_user(void) {
    set_auto_mouse_layer(MH_AUTO_BUTTONS_LAYER); // only required if AUTO_MOUSE_DEFAULT_LAYER is not set to index of <mouse_layer>
    set_auto_mouse_enable(true);         // always required before the auto mouse feature will work
}
#endif

#if (defined MH_AUTO_BUTTONS && defined PS2_MOUSE_ENABLE && defined MOUSEKEY_ENABLE) || defined(POINTING_DEVICE_AUTO_MOUSE_MH_ENABLE)

static uint16_t mh_auto_buttons_timer;
extern int tp_buttons; // mousekey button state set in action.c and used in ps2_mouse.c

void mouse_mode(bool);

#endif

#if defined(POINTING_DEVICE_AUTO_MOUSE_MH_ENABLE)

#define SCROLL_DIVISOR 20

static int _ds_l_x = 0;
static int _ds_l_y = 0;
static int _ds_r_x = 0;
static int _ds_r_y = 0;

static bool left_scroll_hold = false, right_scroll_hold = false;

report_mouse_t pointing_device_task_combined_user(report_mouse_t reportMouse1, report_mouse_t reportMouse2) {
    report_mouse_t ret_mouse;
    if (reportMouse1.x == 0 && reportMouse1.y == 0 && reportMouse2.x == 0 && reportMouse2.y == 0)
        return pointing_device_combine_reports(reportMouse1, reportMouse2);

    if (global_saved_values.left_scroll || left_scroll_hold) {
        int div_x;
        int div_y;

        reportMouse1.y = -reportMouse1.y;

        _ds_l_x += reportMouse1.x;
        _ds_l_y += reportMouse1.y;

        div_x = _ds_l_x / SCROLL_DIVISOR;
        div_y = _ds_l_y / SCROLL_DIVISOR;
        if (div_x != 0) {
            reportMouse1.h += div_x;
            _ds_l_x -= div_x * SCROLL_DIVISOR;
        }

        if (div_y != 0) {
            reportMouse1.v += div_y;
            _ds_l_y -= div_y * SCROLL_DIVISOR;
        }
        reportMouse1.x = 0;
        reportMouse1.y = 0;
    }

    if (global_saved_values.right_scroll || right_scroll_hold) {
        int div_x;
        int div_y;

        reportMouse2.y = -reportMouse2.y;

        _ds_r_x += reportMouse2.x;
        _ds_r_y += reportMouse2.y;

        div_x = _ds_r_x / SCROLL_DIVISOR;
        div_y = _ds_r_y / SCROLL_DIVISOR;
        if (div_x != 0) {
            reportMouse2.h += div_x;
            _ds_r_x -= div_x * SCROLL_DIVISOR;
        }

        if (div_y != 0) {
            reportMouse2.v += div_y;
            _ds_r_y -= div_y * SCROLL_DIVISOR;
        }
        reportMouse2.x = 0;
        reportMouse2.y = 0;
    }

    if (mh_auto_buttons_timer) {
        mh_auto_buttons_timer = timer_read();
    } else {
        mouse_mode(true);
#if defined CONSOLE_ENABLE
        print("task - combined mh_auto_buttons: on\n");
#endif
    }
    ret_mouse = pointing_device_combine_reports(reportMouse1, reportMouse2);

    return pointing_device_task_user(ret_mouse);
}

static int snipe_x = 0;
static int snipe_y = 0;

static int snipe_div = 1;
report_mouse_t pointing_device_task_user(report_mouse_t reportMouse) {
    if (reportMouse.x == 0 && reportMouse.y == 0)
        return reportMouse;

    if (mh_auto_buttons_timer) {
        mh_auto_buttons_timer = timer_read();
    } else {
        mouse_mode(true);
#if defined CONSOLE_ENABLE
        print("user - mh_auto_buttons: on\n");
#endif
    }

    if (snipe_div != 1) {
        int div_x;
        int div_y;
        snipe_x += reportMouse.x;
        snipe_y += reportMouse.y;

        reportMouse.x = 0;
        reportMouse.y = 0;

        div_x = snipe_x / snipe_div;
        div_y = snipe_y / snipe_div;
        if (div_x != 0) {
            reportMouse.x = div_x;
            snipe_x -= div_x * snipe_div;
        }

        if (div_y != 0) {
            reportMouse.y = div_y;
            snipe_y -= div_y * snipe_div;
        }
    }
    return reportMouse;
}
#endif

void mh_change_timeouts(void) {
    if (sizeof(mh_timer_choices) / sizeof(int16_t) - 1 <= global_saved_values.mh_timer_index) {
        global_saved_values.mh_timer_index = 0;
    } else {
        global_saved_values.mh_timer_index++;
    }
    uprintf("mh_timer:%d\n", mh_timer_choices[global_saved_values.mh_timer_index]);
    write_eeprom_kb();
}

void toggle_achordion(void) {
    global_saved_values.disable_achordion = !global_saved_values.disable_achordion;
    write_eeprom_kb();
}

#define LAYER_2345_MASK (0x3C)
void check_layer_67(void) {
    if ((layer_state & LAYER_2345_MASK) == LAYER_2345_MASK) {
        layer_on(6);
        layer_on(7);
    } else {
        layer_off(6);
        layer_off(7);
    }
}

bool in_mod_tap = false;
int8_t in_mod_tap_layer = -1;
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {

    // Abort additional processing if userspace code did
    if (!process_record_user(keycode, record)) { return false;}

    if (!global_saved_values.disable_achordion && !process_achordion(keycode, record)) { return false; }

    // We are in a mod tap, with a KC_TRANSPARENT, lets make it transparent...
    if (IS_QK_MOD_TAP(keycode) && ((keycode & 0xFF) == KC_TRANSPARENT) &&
        record->tap.count > 0 && !in_mod_tap &&
        in_mod_tap_layer == -1 && record->event.pressed) {

        in_mod_tap_layer = get_highest_layer(layer_state);
        layer_state = layer_state & ~(1 << in_mod_tap_layer);

        action_exec(record->event);

        in_mod_tap = true;

        return false;
    }

    // Fix things up on the release for the mod_tap case.
    if (!record->event.pressed && in_mod_tap) {
        in_mod_tap = false;
        layer_state = layer_state | (1 << in_mod_tap_layer);
        in_mod_tap_layer = -1;
    }
    // If console is enabled, it will print the matrix position and status of each key pressed
#ifdef CONSOLE_ENABLE
    uprintf("KL: kc: 0x%04X, col: %2u, row: %2u, pressed: %u, time: %5u, int: %u, count: %u\n", keycode, record->event.key.col, record->event.key.row, record->event.pressed, record->event.time, record->tap.interrupted, record->tap.count);
#endif

    if (mh_auto_buttons_timer) {
        switch (keycode) {
            case KC_BTN1:
            case KC_BTN2:
            case KC_BTN3:
            case KC_BTN4:
            case KC_BTN5:
            case KC_WH_U:
            case KC_WH_D:
            case KC_WH_R:
            case KC_WH_L:
            case KC_LSFT:
            case KC_RSFT:
            case KC_LCTL:
            case KC_RCTL:
            case KC_LALT:
            case KC_RALT:
            case KC_LGUI:
            case KC_RGUI:
            case SV_LEFT_SCROLL_TOGGLE:
            case SV_RIGHT_SCROLL_TOGGLE:
            case SV_LEFT_SCROLL_HOLD:
            case SV_RIGHT_SCROLL_HOLD:
            case SV_SNIPER_2:
            case SV_SNIPER_3:
            case SV_SNIPER_5:
            case SV_RECALIBRATE_POINTER:
                break;
            default:
#ifdef CONSOLE_ENABLE
                uprintf("process_record - mh_auto_buttons: off\n");
#endif
                mouse_mode(false);
        }
    }
    if (record->event.pressed) { // key pressed
        switch (keycode) {
            case SV_LEFT_DPI_INC:
                increase_left_dpi();
                return false;
            case SV_LEFT_DPI_DEC:
                decrease_left_dpi();
                return false;
            case SV_RIGHT_DPI_INC:
                increase_right_dpi();
                return false;
            case SV_RIGHT_DPI_DEC:
                decrease_right_dpi();
                return false;
            case SV_LEFT_SCROLL_TOGGLE:
                global_saved_values.left_scroll = !global_saved_values.left_scroll;
                //write_eeprom_kb();
                return false;
            case SV_RIGHT_SCROLL_TOGGLE:
                global_saved_values.right_scroll = !global_saved_values.right_scroll;
                //write_eeprom_kb();
                return false;
            case SV_RECALIBRATE_POINTER:
                recalibrate_pointer();
                return false;
            case SV_MH_CHANGE_TIMEOUTS:
                mh_change_timeouts();
                return false;
            case SV_CAPS_WORD:
                caps_word_toggle();
                return false;
            case SV_TOGGLE_ACHORDION:
                toggle_achordion();
                return false;
            case SV_TOGGLE_23_67:
                layer_on(2);
                layer_on(3);
                check_layer_67();
                return false;
            case SV_TOGGLE_45_67:
                layer_on(4);
                layer_on(5);
                check_layer_67();
                return false;
            case SV_SNIPER_2:
                snipe_x *= 2;
                snipe_y *= 2;
                snipe_div *= 2;
                return false;
            case SV_SNIPER_3:
                snipe_div *= 3;
                snipe_x *= 3;
                snipe_y *= 3;
                return false;
            case SV_SNIPER_5:
                snipe_div *= 5;
                snipe_x *= 5;
                snipe_y *= 5;
                return false;
            case SV_LEFT_SCROLL_HOLD:
                left_scroll_hold = true;
                return false;
            case SV_RIGHT_SCROLL_HOLD:
                right_scroll_hold = true;
                return false;
            case SV_OUTPUT_STATUS:
                output_keyboard_info();
                return false;
        }
    } else { // key released
        switch (keycode) {
            // These keys are all holds and require un-setting upon release.
            case SV_TOGGLE_23_67:
                layer_off(2);
                layer_off(3);
                check_layer_67();
                return false;
            case SV_TOGGLE_45_67:
                layer_off(4);
                layer_off(5);
                check_layer_67();
                return false;
            case SV_SNIPER_2:
                snipe_div /= 2;
                snipe_x /= 2;
                snipe_y /= 2;
                return false;
            case SV_SNIPER_3:
                snipe_div /= 3;
                snipe_x /= 3;
                snipe_y /= 3;
                return false;
            case SV_SNIPER_5:
                snipe_div /= 5;
                snipe_x /= 5;
                snipe_y /= 5;
                return false;
            case SV_LEFT_SCROLL_HOLD:
                left_scroll_hold = false;
                return false;
            case SV_RIGHT_SCROLL_HOLD:
                right_scroll_hold = false;
                return false;
        }
    }

    // Neither the user nor the keyboard handled the event, so continue with normal handling
    return true;
};

#if defined MH_AUTO_BUTTONS && defined PS2_MOUSE_ENABLE && defined MOUSEKEY_ENABLE
void ps2_mouse_moved_user(report_mouse_t *mouse_report) {
    if (mh_auto_buttons_timer) {
        mh_auto_buttons_timer = timer_read();
    } else {
        if (!tp_buttons) {
            mouse_mode(true);
#if defined CONSOLE_ENABLE
            print("ps2 / mh_auto_buttons: on\n");
#endif
        }
    }
}
#endif

void matrix_scan_kb(void) {
    if (!global_saved_values.disable_achordion) {
        achordion_task();
    }

    if ((mh_timer_choices[global_saved_values.mh_timer_index] >= 0) && mh_auto_buttons_timer && (timer_elapsed(mh_auto_buttons_timer) > mh_timer_choices[global_saved_values.mh_timer_index])) {
        if (!tp_buttons) {
            mouse_mode(false);
#if defined CONSOLE_ENABLE
            print("matrix - mh_auto_buttons: off\n");
#endif
        }
    }

    matrix_scan_user();
}

void mouse_mode(bool on) {
    if (on) {
        layer_on(MH_AUTO_BUTTONS_LAYER);
        mh_auto_buttons_timer = timer_read();
    } else {
        layer_off(MH_AUTO_BUTTONS_LAYER);
        mh_auto_buttons_timer = 0;
    }
}
