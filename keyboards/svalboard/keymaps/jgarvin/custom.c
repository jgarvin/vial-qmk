#include <stdint.h>
#include "keycodes.h"
#include "quantum/quantum_keycodes.h"
#include "quantum/action.h"
#include "custom.h"

uint8_t combo_ref_from_layer(uint8_t layer) {
    // for the wm management layers, we want the combo layer to be
    // different so that we can setup that the combo for enter should
    // actually do gui+enter
    /* if(layer == 3 || layer == 4) { */
    /*     return 7; */
    /* } */

    // Prefer to define combos in terms of their layer 0 keys, at least
    // for now. This enables us to be able to press three home row keys to
    // press enter, even if we are in a layer where those home row keys
    // have been defined to be something else.
    return 0;
}

// Vial doesn't let you define LT(kc) keys that call custom keys like
// the scroll toggles that sval has. If you try to implement the
// equivalent with a tap dance key then you have to hold the key long
// enough for qmk to be sure it's a hold, even though as soon as you
// press another key while holding it's unambiguous you intend to use
// the layer switch functionality.
typedef struct quicktap {
    uint16_t trigger_keycode;
    uint16_t standalone_tap_keycode;
    int layer;
    bool pressed;
    bool anything_else_pressed_while_holding;
} quicktap_t;

quicktap_t quicktaps[2];

// for now using pre init since the sval keymap.c uses post init
void keyboard_pre_init_user(void)
{
    quicktaps[0].trigger_keycode = LT7_SCROLL_LEFT_TOGGLE;
    quicktaps[0].standalone_tap_keycode = SV_LEFT_SCROLL_TOGGLE;
    quicktaps[0].layer = 7;
    quicktaps[1].trigger_keycode = LT3_SCROLL_RIGHT_TOGGLE;
    quicktaps[1].standalone_tap_keycode = SV_RIGHT_SCROLL_TOGGLE;
    quicktaps[1].layer = 3;

    printf("LT7_SCROLL_LEFT_TOGGLE keycode: %04x\n", LT7_SCROLL_LEFT_TOGGLE);
    printf("LT3_SCROLL_RIGHT_TOGGLE keycode: %04x\n", LT3_SCROLL_RIGHT_TOGGLE);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static bool recursing = false;
    if(recursing) {
        return true;
    }

    for(int i = 0; i < sizeof(quicktaps)/sizeof(quicktaps[0]); i++)
    {
        quicktap_t* qt = &quicktaps[i];

        if(i == 0)
        {
            print("===============================================\n");
            printf("keycode %d\n", keycode);
            printf("record->event.pressed %d\n", record->event.pressed);
            printf("old_anything_else_pressed_while_holding %d\n", qt->anything_else_pressed_while_holding);
            printf("old_pressed %d\n", qt->pressed);
        }

        bool old_pressed = qt->pressed;
        if(keycode == qt->trigger_keycode)
        {
            qt->pressed = record->event.pressed;
            printf("pressed %d\n", qt->pressed);
            if(old_pressed)
            {
                if(!record->event.pressed)
                {
                    if(qt->anything_else_pressed_while_holding)
                    {
                        printf("layer %d off\n", qt->layer);
                        qt->anything_else_pressed_while_holding = false;
                        printf("anything_else_pressed_while_holding %d\n", qt->anything_else_pressed_while_holding);
                        recursing = true;
                        process_record(record);
                        recursing = false;
                        layer_off(qt->layer);
                        return false;
                    }
                    else
                    {
                        print("tapping left scroll toggle\n");
                        keyrecord_t tap_record = *record;
                        tap_record.keycode = qt->standalone_tap_keycode;
                        print("recursing to allow through other key while this key is held");
                        recursing = true;
                        tap_record.event.pressed = true;
                        process_record(&tap_record);
                        tap_record.event.pressed = false;
                        process_record(&tap_record);
                        recursing = false;
                        return false;
                    }
                }
            }
        }
        else if(old_pressed && record->event.pressed && !qt->anything_else_pressed_while_holding)
        {
            printf("layer %d on\n", qt->layer);
            layer_on(qt->layer);
            qt->anything_else_pressed_while_holding = true;
            printf("anything_else_pressed_while_holding %d\n", qt->anything_else_pressed_while_holding);
            print("recursing to allow through other key while this key is held");
            recursing = true;
            process_record(record);
            recursing = false;
            return false;
        }
    }

    return true;
}
