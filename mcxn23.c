/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MCXN236_Project.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "sonar_types.h"
#include "absorption_model.h"
#include "parameter_calculator.h"
#include "waveform_selector.h"
#include "sensor_acquisition.h"

/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */
sensor_input_t in;
double fc, alpha;
calculation_result_t calc;
/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("Hello World\r\n");
    sensor_acquisition_init();
    sensor_acquisition_read(&in);
    fc = select_centre_frequency_khz(in.temperature_c, in.salinity_psu, in.depth_m, &alpha);
    calculate_transmit_parameters(fc, alpha, &in, &calc);
    PRINTF("fc=%.1f kHz, power=%.3f mW\r\n", calc.freq_khz, calc.power_w*1000.0);

    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        i++ ;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        __asm volatile ("nop");
    }
    return 0 ;
}
s
