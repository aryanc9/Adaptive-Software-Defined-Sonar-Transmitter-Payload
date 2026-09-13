#include <math.h>
#include <stddef.h>
#include "parameter_calculator.h"
#include "absorption_model.h"

const double CANDIDATE_FREQUENCIES_KHZ[6] = { 50.0, 100.0, 200.0, 300.0, 400.0, 500.0 };

/* NSL table (Image 1, Step 6). Rows = CANDIDATE_FREQUENCIES_KHZ order. */
static const double NSL_TABLE[6][3] = {
    /* Depth1, Depth2, Depth3 */
    { 70.0, 64.0, 58.0 },  /* 50 kHz  */
    { 64.0, 58.0, 52.0 },  /* 100 kHz */
    { 58.0, 52.0, 46.0 },  /* 200 kHz */
    { 54.0, 48.0, 42.0 },  /* 300 kHz */
    { 51.0, 45.0, 39.0 },  /* 400 kHz */
    { 48.0, 42.0, 36.0 }   /* 500 kHz */
};

/* Pulse duration table (Image 1, Step 9). Index-matched to RANGE_TABLE_KM. */
static const double RANGE_TABLE_KM[15] = {
    1, 2, 3, 4, 5, 10, 20, 30, 40, 50, 60, 80, 100, 150, 200
};
static const double PULSE_DURATION_TABLE_US[15] = {
    10, 10, 10, 10, 30, 30, 50, 80, 110, 130, 160, 210, 270, 400, 530
};

double select_centre_frequency_khz(double temperature_c,
                                    double salinity_psu,
                                    double depth_m,
                                    double *out_alpha_db_per_km)
{
    int i;
    double best_freq = CANDIDATE_FREQUENCIES_KHZ[0];
    double best_alpha = absorption_coefficient_db_per_km(
        best_freq, temperature_c, salinity_psu, depth_m, SEAWATER_PH_TYPICAL);

    for (i = 1; i < 6; i++) {
        double f = CANDIDATE_FREQUENCIES_KHZ[i];
        double a = absorption_coefficient_db_per_km(
            f, temperature_c, salinity_psu, depth_m, SEAWATER_PH_TYPICAL);
        if (a < best_alpha) {
            best_alpha = a;
            best_freq = f;
        }
    }

    if (out_alpha_db_per_km) {
        *out_alpha_db_per_km = best_alpha;
    }
    return best_freq;
}

int depth_zone_from_depth_m(double depth_m)
{
    if (depth_m <= 10.0) return 1;
    if (depth_m <= 50.0) return 2;
    return 3;
}

double noise_spectral_level_db(double freq_khz, int depth_zone)
{
    int i;
    int zone_idx = depth_zone - 1;
    if (zone_idx < 0) zone_idx = 0;
    if (zone_idx > 2) zone_idx = 2;

    for (i = 0; i < 6; i++) {
        if (CANDIDATE_FREQUENCIES_KHZ[i] == freq_khz) {
            return NSL_TABLE[i][zone_idx];
        }
    }
    return -1.0; /* not an exact table entry */
}

double pulse_duration_lookup_us(double range_km)
{
    int i;
    int n = (int)(sizeof(RANGE_TABLE_KM) / sizeof(RANGE_TABLE_KM[0]));

    /* Nearest table entry >= requested range (conservative: longer
     * pulse duration rather than under-provisioning). Falls back to
     * the longest-range entry if range exceeds the table. */
    for (i = 0; i < n; i++) {
        if (RANGE_TABLE_KM[i] >= range_km) {
            return PULSE_DURATION_TABLE_US[i];
        }
    }
    return PULSE_DURATION_TABLE_US[n - 1];
}

void calculate_transmit_parameters(double fc_khz,
                                    double alpha_db_per_km,
                                    const sensor_input_t *in,
                                    calculation_result_t *result)
{
    int zone = depth_zone_from_depth_m(in->depth_m);
    double nsl = noise_spectral_level_db(fc_khz, zone);

    /* Standard mixed-unit transmission loss: spreading loss uses the
     * conventional 1-metre reference distance, absorption uses km
     * (natural units for Francois-Garrison). This is the fix agreed
     * for the demo - see README_v2.md for why the diagram's literal
     * all-km formula was replaced. */
    double range_m = in->target_range_km * 1000.0;
    double tl = 20.0 * log10(range_m)
              + alpha_db_per_km * in->target_range_km
              + 0.002 * fc_khz * in->turbidity_c;

    double nl = nsl + 10.0 * log10(FIXED_BANDWIDTH_KHZ);

    double sl_req = SNR_THRESHOLD_DB + 2.0 * tl - TARGET_STRENGTH_DB
                   + nl - DETECTION_INDEX_DB;

    double amplitude_upa = pow(10.0, sl_req / 20.0); /* x 1 uPa reference */

    result->freq_khz          = fc_khz;
    result->alpha_db_per_km   = alpha_db_per_km;
    result->tl_db             = tl;
    result->nl_db             = nl;
    result->sl_req_db         = sl_req;
    result->amplitude_upa     = amplitude_upa;
    result->bw_khz            = FIXED_BANDWIDTH_KHZ;
    result->pulse_duration_us = pulse_duration_lookup_us(in->target_range_km);
}
