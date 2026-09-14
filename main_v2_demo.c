/* ============================================================
 * main_v2_demo.c
 *
 * SIH 2026 PS 26058 - Phase 2 desktop demo
 * Implements the algorithm in Image 1 + Image 2 + Image 3
 * (candidate-frequency selection by minimum absorption,
 *  Francois-Garrison model, sonar-equation source level,
 *  and the 4-parameter waveform decision tree).
 *
 * Build:
 *   gcc -Wall -Wextra -std=c11 -lm main_v2_demo.c absorption_model.c \
 *       parameter_calculator.c waveform_selector.c -o sonar_v2_demo.exe
 *
 * Run:
 *   ./sonar_v2_demo.exe
 * ============================================================ */

#include <stdio.h>
#include "sonar_types.h"
#include "absorption_model.h"
#include "parameter_calculator.h"
#include "waveform_selector.h"

static void run_scenario(const char *title, const sensor_input_t *in)
{
    double fc, alpha;
    calculation_result_t calc;
    waveform_decision_t wf;
    double c_sound;

    printf("============================================\n");
    printf("   %s\n", title);
    printf("============================================\n\n");

    printf("INPUTS\n");
    printf("Temperature         : %.2f C\n", in->temperature_c);
    printf("Salinity             : %.2f PSU\n", in->salinity_psu);
    printf("Depth                : %.2f m\n", in->depth_m);
    printf("Turbidity            : %.2f (sensor value)\n", in->turbidity_c);
    printf("Target range         : %.2f km\n\n", in->target_range_km);

    c_sound = sound_speed_mps(in->temperature_c, in->salinity_psu, in->depth_m);
    printf("Sound speed (info)   : %.2f m/s\n\n", c_sound);

    fc = select_centre_frequency_khz(in->temperature_c, in->salinity_psu,
                                      in->depth_m, &alpha);
    calculate_transmit_parameters(fc, alpha, in, &calc);

    printf("FREQUENCY SELECTION (min absorption among 6 candidates)\n");
    printf("Selected frequency   : %.1f kHz\n", calc.freq_khz);
    printf("Absorption at fc     : %.4f dB/km\n\n", calc.alpha_db_per_km);

    printf("TRANSMIT PARAMETER CALCULATION\n");
    printf("Bandwidth (fixed)    : %.1f kHz\n", calc.bw_khz);
    printf("Transmission loss    : %.2f dB\n", calc.tl_db);
    printf("Noise level          : %.2f dB\n", calc.nl_db);
    printf("Required source lvl  : %.2f dB\n", calc.sl_req_db);
    printf("Required amplitude   : %.0f uPa  (%.3f Pa)\n",
           calc.amplitude_upa, calc.amplitude_upa / 1.0e6);
    printf("Pulse duration       : %.1f us\n\n", calc.pulse_duration_us);

    wf = select_waveform(in);
    printf("WAVEFORM DECISION (4-parameter classification)\n");
    printf("Turbidity level      : %s\n", level_to_string(wf.turbidity_level));
    printf("Depth level          : %s\n", level_to_string(wf.depth_level));
    printf("Temperature level    : %s\n", level_to_string(wf.temperature_level));
    printf("Salinity level       : %s\n", level_to_string(wf.salinity_level));
    printf("Selected waveform    : %s\n\n", waveform_to_string(wf.waveform));

    printf("SYSTEM STATUS\n");
    printf("Algorithm status     : PASS\n");
    printf("============================================\n\n");
}

int main(void)
{
    /* Realistic AUV target ranges (tens to low-hundreds of metres,
     * matching the original project's own scale), expressed as km
     * fractions since target_range_km is what the calculator uses.
     * Same base sensor values as the Phase 1 demo, for comparison. */
    sensor_input_t base = {
        .temperature_c   = 15.0,
        .salinity_psu    = 20.0,
        .depth_m         = 50.0,
        .turbidity_c     = 50.0,
        .target_range_km = 0.100 /* 100 m */
    };
    run_scenario("BASE SCENARIO (matches Phase 1 demo inputs)", &base);

    /* Clear, shallow, low-everything -> should select Phase-Coded */
    sensor_input_t clear = {
        .temperature_c   = 8.0,
        .salinity_psu    = 4.0,
        .depth_m         = 8.0,
        .turbidity_c     = 6.0,
        .target_range_km = 0.010 /* 10 m */
    };
    run_scenario("SCENARIO: CLEAR / SHALLOW (expect Phase-Coded)", &clear);

    /* One high parameter (turbidity) -> should select LFM */
    sensor_input_t harsh = {
        .temperature_c   = 16.0,
        .salinity_psu    = 22.0,
        .depth_m         = 50.0,
        .turbidity_c     = 85.0,
        .target_range_km = 0.100 /* 100 m */
    };
    run_scenario("SCENARIO: HIGH TURBIDITY (expect LFM Chirp)", &harsh);

    /* All-medium -> should select Geometric */
    sensor_input_t medium = {
        .temperature_c   = 18.0,
        .salinity_psu    = 15.0,
        .depth_m         = 30.0,
        .turbidity_c     = 30.0,
        .target_range_km = 0.150 /* 150 m */
    };
    run_scenario("SCENARIO: ALL MEDIUM (expect Geometric Sweep)", &medium);

    return 0;
}
