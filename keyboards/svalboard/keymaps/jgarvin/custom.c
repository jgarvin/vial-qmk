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

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    printf("LT3_SCROLL_LEFT_TOGGLE keycode: %04x\n", LT3_SCROLL_LEFT_TOGGLE);

    static bool pressed = false;
    static bool anything_else_pressed_while_holding = false;
    static keyrecord_t left_scroll_record;
    static bool recursing = false;
    if(recursing) {
        return true;
    }

    print("===============================================\n");
    printf("keycode %d\n", keycode);
    printf("record->event.pressed %d\n", record->event.pressed);
    printf("old_anything_else_pressed_while_holding %d\n", anything_else_pressed_while_holding);
    bool old_pressed = pressed;
    printf("old_pressed %d\n", pressed);
    if(keycode == LT3_SCROLL_LEFT_TOGGLE)
    {
        pressed = record->event.pressed;
        printf("pressed %d\n", pressed);
        if(old_pressed)
        {
            if(!record->event.pressed)
            {
                if(anything_else_pressed_while_holding)
                {
                    print("layer 3 off\n");
                    anything_else_pressed_while_holding = false;
                    printf("anything_else_pressed_while_holding %d\n", anything_else_pressed_while_holding);
                    layer_off(3);
                }
                else
                {
                    print("tapping left scroll toggle\n");
                    left_scroll_record         = *record;
                    left_scroll_record.keycode = SV_LEFT_SCROLL_TOGGLE;
                     left_scroll_record.event.pressed = true;
                    recursing = true;
                    print("recursing to allow through other key while this key is held")
                    process_record(&left_scroll_record);
                    recursing = false;
                    left_scroll_record.event.pressed = false;
                    return false;
                }
            }
        }
    }
    else if(old_pressed && record->event.pressed && !anything_else_pressed_while_holding)
    {
        print("layer 3 on\n");
        layer_on(3);
        anything_else_pressed_while_holding = true;
        printf("anything_else_pressed_while_holding %d\n", anything_else_pressed_while_holding);
        recursing = true;
        print("recursing to allow through other key while this key is held");
        process_record(record);
        recursing = false;
        return false;
    }

    return true;
}
