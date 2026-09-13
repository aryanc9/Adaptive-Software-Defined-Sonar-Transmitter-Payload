#include "waveform_selector.h"

static level_t classify(double value, double low_max, double medium_max)
{
    if (value <= low_max) return LEVEL_LOW;
    if (value <= medium_max) return LEVEL_MEDIUM;
    return LEVEL_HIGH;
}

waveform_decision_t select_waveform(const sensor_input_t *in)
{
    waveform_decision_t d;

    d.turbidity_level   = classify(in->turbidity_c,     10.0, 60.0);
    d.depth_level       = classify(in->depth_m,         10.0, 50.0);
    d.temperature_level = classify(in->temperature_c,   10.0, 25.0);
    d.salinity_level    = classify(in->salinity_psu,     5.0, 25.0);

    if (d.turbidity_level == LEVEL_HIGH || d.depth_level == LEVEL_HIGH ||
        d.temperature_level == LEVEL_HIGH || d.salinity_level == LEVEL_HIGH) {
        d.waveform = WAVE_LFM_CHIRP;
    } else if (d.turbidity_level == LEVEL_MEDIUM || d.depth_level == LEVEL_MEDIUM ||
               d.temperature_level == LEVEL_MEDIUM || d.salinity_level == LEVEL_MEDIUM) {
        d.waveform = WAVE_GEOMETRIC_SWEEP;
    } else if (d.turbidity_level == LEVEL_LOW && d.depth_level == LEVEL_LOW &&
               d.temperature_level == LEVEL_LOW && d.salinity_level == LEVEL_LOW) {
        d.waveform = WAVE_PHASE_CODED;
    } else {
        d.waveform = WAVE_LFM_CHIRP; /* safe default */
    }

    return d;
}

const char *level_to_string(level_t level)
{
    switch (level) {
        case LEVEL_LOW:    return "Low";
        case LEVEL_MEDIUM: return "Medium";
        case LEVEL_HIGH:   return "High";
        default:           return "Unknown";
    }
}

const char *waveform_to_string(waveform_type_t w)
{
    switch (w) {
        case WAVE_PHASE_CODED:     return "Phase-Coded";
        case WAVE_GEOMETRIC_SWEEP: return "Geometric Sweep";
        case WAVE_LFM_CHIRP:       return "LFM Chirp";
        default:                   return "Unknown";
    }
}
