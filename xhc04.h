#ifndef XHC04_H
#define XHC04_H

#include <stdio.h>
#include "pico/stdlib.h"


typedef struct StatusReport
{
    uint16_t magic;             // OFFSET 0x00
    uint8_t  day;               // OFFSET 0x02
    uint16_t wc_x_int;          // OFFSET 0x03
    uint16_t wc_x_frac;
    uint16_t wc_y_int;
    uint16_t wc_y_frac;
    uint16_t wc_z_int;
    uint16_t wc_z_frac;
    uint16_t mc_x_int;
    uint16_t mc_x_frac;
    uint16_t mc_y_int;
    uint16_t mc_y_frac;
    uint16_t mc_z_int;
    uint16_t mc_z_frac;
    uint16_t feedrate_ovr;
    uint16_t spindle_speed_ovr;
    uint16_t feedrate;
    uint16_t spindle_speed;
    uint8_t step_mul;
    uint8_t state;
    uint8_t padding1;
    uint8_t padding2;
    uint8_t padding3;
    uint8_t padding4;
    uint8_t padding5;
} status_report_t;


#endif