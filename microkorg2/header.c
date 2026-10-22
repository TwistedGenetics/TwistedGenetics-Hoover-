/*
 *  File: header.c
 *
 *  Hoover - microKORG 2 oscillator (osc) unit header.
 *  osc units get 13 params max: 3 on page 1, then pages of 5.
 *  Names: max 8 characters.
 */

#include "unit.h"
#include "runtime.h"

// The load-meter build (tools/mk2-meter) reuses this header with its own
// name and unit ID, so both can be loaded side by side.
#ifndef HOOVER_UNIT_NAME
#define HOOVER_UNIT_NAME "Hoover"
#endif
#ifndef HOOVER_UNIT_ID
#define HOOVER_UNIT_ID 0x7U  // 1 = VectorFM, 2 = Formant Mutator, 3 = Rollers, 4 = Dusty Pads, 5 = Horizon, 6 = (Dread Bass), 7 = Hoover, 8 = EraCrush
#endif

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x5477476EU,  // 'TwGn' (TwistedGenetics)
    .unit_id = HOOVER_UNIT_ID,
    .version = 0x00010000U,  // 1.0.0
    .name = HOOVER_UNIT_NAME,
    .num_presets = 0,
    .num_params = 12,
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name

        // Page 1
        {0,     3,  0,   0, k_unit_param_type_strings, 0, 0, 0, {"Mode"}},
        {0,   100,  0,  45, k_unit_param_type_percent, 0, 0, 0, {"Detune"}},
        {-24,  24,  0,  12, k_unit_param_type_none,    0, 0, 0, {"Dive"}},

        // Page 2
        {0,   100,  0,  35, k_unit_param_type_percent, 0, 0, 0, {"DiveTime"}},
        {0,   100,  0,  60, k_unit_param_type_percent, 0, 0, 0, {"PWM"}},
        {0,   100,  0,  30, k_unit_param_type_percent, 0, 0, 0, {"PWM Rate"}},
        {0,   100,  0,  50, k_unit_param_type_percent, 0, 0, 0, {"Chorus"}},
        {0,   100,  0,  25, k_unit_param_type_percent, 0, 0, 0, {"Sub"}},

        // Page 3
        {0,   100,  0,  20, k_unit_param_type_percent, 0, 0, 0, {"Drive"}},
        {0,   100,  0,  30, k_unit_param_type_percent, 0, 0, 0, {"Drift"}},
        {-24,  24,  0,   0, k_unit_param_type_none,    0, 0, 0, {"Semi"}},
        {-50,  50,  0,   0, k_unit_param_type_none,    0, 0, 0, {"Fine"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}}};
