#ifndef PARAMETER_CALCULATOR_H
#define PARAMETER_CALCULATOR_H

#include "sonar_types.h"

/* ---- Constants from Image 1, Step 2 "Define Constants & Requirements" ---- */
#define DETECTION_INDEX_DB   18.0   /* DI */
#define TARGET_STRENGTH_DB  (-20.0) /* TS */
#define SNR_THRESHOLD_DB     15.0   /* SNRth */
#define FIXED_BANDWIDTH_KHZ   5.0   /* BW, from Delta_R = 0.15 m */

/* 6 candidate centre frequencies (kHz). NOT given as explicit numbers
 * in Image 1 (only "0-500 kHz, 6 candidates"); these are taken from
 * the 6 rows of the NSL table (Image 1, Step 6), which is the only
 * place 6 concrete frequency values appear. Confirm with the team
 * before treating this as final. */
extern const double CANDIDATE_FREQUENCIES_KHZ[6];

/* Runs Steps 3-4: computes alpha for each candidate frequency and
 * returns the one with minimum absorption, plus that alpha value. */
double select_centre_frequency_khz(double temperature_c,
                                    double salinity_psu,
                                    double depth_m,
                                    double *out_alpha_db_per_km);

/* Noise Spectral Level lookup (Image 1, Step 6 table). freq_khz must
 * be one of CANDIDATE_FREQUENCIES_KHZ (exact match). depth_zone is
 * 1, 2, or 3 - see depth_zone_from_depth_m(). Returns NSL in dB re
 * 1 uPa^2/Hz, or -1.0 if freq_khz is not an exact table entry. */
double noise_spectral_level_db(double freq_khz, int depth_zone);

/* Maps a raw depth reading to NSL table zone 1/2/3. The images never
 * define what "Depth 1/2/3" means, so this reuses Image 2's own
 * depth thresholds (<=10 / 11-50 / >50 m) as the zone boundaries -
 * an assumption, not a given. Confirm with the team. */
int depth_zone_from_depth_m(double depth_m);

/* Runs the full Step 5-9 chain for an already-selected centre
 * frequency: TL, NL, required SL, required amplitude, fixed BW,
 * and pulse duration lookup by range. Fills result->* fields
 * except freq_khz/alpha_db_per_km (caller should set those first). */
void calculate_transmit_parameters(double fc_khz,
                                    double alpha_db_per_km,
                                    const sensor_input_t *in,
                                    calculation_result_t *result);

/* Pulse duration lookup table (Image 1, Step 9). Table is labeled
 * "Range (km)" with the SAME numeric values as the old range-in-
 * metres table from the earlier context doc. That is very likely a
 * unit labeling error on the poster (200 km is an implausible AUV
 * sonar range) - flagged, not silently fixed. This function follows
 * Image 1 literally (range in km) since that is what's drawn. */
double pulse_duration_lookup_us(double range_km);

#endif /* PARAMETER_CALCULATOR_H */
