#ifndef SONAR_TYPES_H
#define SONAR_TYPES_H

/* ============================================================
 * sonar_types.h
 *
 * SIH 2026 PS 26058 - Phase 2 (v2 diagram) shared types.
 *
 * Implements the algorithm shown in:
 *   Image 1: "Adaptive Sonar Waveform Selection & Parameter
 *             Calculation Algorithm"
 *   Image 2: "Waveform selection flow"
 *   Image 3: "Underwater Acoustic Calculations - Complete Formulae"
 *
 * This is Phase 2. Phase 1 (sonar_demo.c, the range-table /
 * turbidity-table version) is kept separate on purpose - the two
 * frequency-selection and waveform-selection strategies conflict
 * and have NOT been reconciled by the team yet. See README_v2.md.
 * ============================================================ */

typedef struct {
    double temperature_c;   /* T   : degC */
    double salinity_psu;    /* S   : PSU */
    double depth_m;         /* D   : m */
    double turbidity_c;     /* C   : sensor value (assumed NTU) */
    double target_range_km; /* Rm  : km, per Image 1's knob label */
} sensor_input_t;

typedef enum {
    LEVEL_LOW = 0,
    LEVEL_MEDIUM = 1,
    LEVEL_HIGH = 2
} level_t;

typedef enum {
    WAVE_PHASE_CODED = 0,
    WAVE_GEOMETRIC_SWEEP = 1,
    WAVE_LFM_CHIRP = 2
} waveform_type_t;

typedef struct {
    double freq_khz;              /* selected centre frequency fc */
    double alpha_db_per_km;       /* absorption at fc */
    double tl_db;                 /* transmission loss */
    double nl_db;                 /* noise level */
    double sl_req_db;             /* required source level */
    double amplitude_upa;         /* required amplitude, re 1 uPa */
    double bw_khz;                /* fixed bandwidth, 5 kHz */
    double pulse_duration_us;     /* from range lookup table */
} calculation_result_t;

typedef struct {
    level_t turbidity_level;
    level_t depth_level;
    level_t temperature_level;
    level_t salinity_level;
    waveform_type_t waveform;
} waveform_decision_t;

#endif /* SONAR_TYPES_H */
