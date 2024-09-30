#include <stdint.h>
#include "keycodes.h"
#include "quantum/quantum_keycodes.h"
#include "quantum/action.h"

uint8_t combo_ref_from_layer(uint8_t layer) {
    // for the wm management layers, we want the combo layer to be
    // different so that we can setup that the combo for enter should
    // actually do gui+enter
    if(layer == 3 || layer == 4) {
        return 7;
    }
    // Prefer to define combos in terms of their layer 0 keys, at least
    // for now. This enables us to be able to press three home row keys to
    // press enter, even if we are in a layer where those home row keys
    // have been defined to be something else.
    return 0;
}