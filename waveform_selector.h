#ifndef WAVEFORM_SELECTOR_H
#define WAVEFORM_SELECTOR_H

#include "sonar_types.h"

/* Implements Image 2 "Waveform selection flow" exactly:
 *   Turbidity : <=10 low, 11-60 medium, >60 high
 *   Depth     : <=10 low, 11-50 medium, >50 high
 *   Temp      : <=10 low, 11-25 medium, >25 high
 *   Salinity  : <=5  low, 6-25  medium, >25 high
 *
 * Decision logic:
 *   any parameter HIGH    -> LFM chirp
 *   else any MEDIUM       -> Geometric sweep
 *   else all LOW          -> Phase-coded pulse
 *   (anything else, e.g. invalid input) -> LFM chirp (safe default)
 *
 * This REPLACES the earlier Phase-1 turbidity-only placeholder rule
 * (Section 15/16 of the original context doc) as the team's real
 * waveform-selection logic.
 */
waveform_decision_t select_waveform(const sensor_input_t *in);

const char *level_to_string(level_t level);
const char *waveform_to_string(waveform_type_t w);

#endif /* WAVEFORM_SELECTOR_H */
