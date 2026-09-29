
#include <math.h>
#include <stddef.h>
#include "parameter_calculator.h"
#include "absorption_model.h"

/*
 * Candidate frequencies in descending order.
 * Step-down sequence:
 * 500 -> 400 -> 300 -> 200 -> 100 kHz
 */
const double CANDIDATE_FREQUENCIES_KHZ[5] = {
    500.0, 400.0, 300.0, 200.0, 100.0
};

/*a
 * NSL table, index-matched to CANDIDATE_FREQUENCIES_KHZ.
 *
 * Columns:
 *   Zone 1: depth <= 10 m
 *   Zone 2: 10 m < depth <= 50 m
 *   Zone 3: depth > 50 m
 */
static const double NSL_TABLE[5][3] = {
    { 48.0, 42.0, 36.0 },  /* 500 kHz */
    { 51.0, 45.0, 39.0 },  /* 400 kHz */
    { 54.0, 48.0, 42.0 },  /* 300 kHz */
    { 58.0, 52.0, 46.0 },  /* 200 kHz */
    { 64.0, 58.0, 52.0 }   /* 100 kHz */
};

/*
 * Pulse duration lookup table.
 * Range is in km, pulse duration is in microseconds.
 */
static const double RANGE_TABLE_KM[15] = {
    1, 2, 3, 4, 5, 10, 20, 30, 40, 50, 60, 80, 100, 150, 200
};

static const double PULSE_DURATION_TABLE_US[15] = {
    10, 10, 10, 10, 30, 30, 50, 80, 110, 130, 160, 210, 270, 400, 530
};


/*
 * --------------------------------------------------------------------------
 * Acoustic Level -> Electrical Power reference model
 * --------------------------------------------------------------------------
 *
 * SLref = 150 dB re 1 uPa @ 1 m
 * Vref  = 1 Vrms
 * Load  = 50 ohms
 *
 * The conversion is:
 *
 * Vrms = Vref * 10^((SL - SLref) / 20)
 *
 * For example:
 *
 * SL = 128.26 dB
 *
 * Vrms = 1 * 10^((128.26 - 150) / 20)
 *      ~= 0.0818 V
 *
 * Vpp = 2 * sqrt(2) * Vrms
 *     ~= 0.231 V
 *
 * P = Vrms^2 / R
 *   ~= 0.000134 W
 *   = 0.134 mW
 *
 * IMPORTANT:
 * This is a hypothetical/demo transducer reference model.
 * It is NOT a physical transducer calibration.
 */
#define SL_REFERENCE_DB_RE_UPA      150.0
#define V_REFERENCE_RMS_V             1.0
#define ELECTRICAL_LOAD_OHM          50.0


/*
 * Select the candidate centre frequency having the lowest
 * calculated absorption coefficient.
 */
double select_centre_frequency_khz(double temperature_c,
                                    double salinity_psu,
                                    double depth_m,
                                    double *out_alpha_db_per_km)
{
    int i;

    double best_freq = CANDIDATE_FREQUENCIES_KHZ[0];

    double best_alpha = absorption_coefficient_db_per_km(
        best_freq,
        temperature_c,
        salinity_psu,
        depth_m,
        SEAWATER_PH_TYPICAL
    );

    for (i = 1; i < 5; i++) {

        double f = CANDIDATE_FREQUENCIES_KHZ[i];

        double a = absorption_coefficient_db_per_km(
            f,
            temperature_c,
            salinity_psu,
            depth_m,
            SEAWATER_PH_TYPICAL
        );

        if (a < best_alpha) {
            best_alpha = a;
            best_freq = f;
        }
    }

    if (out_alpha_db_per_km != NULL) {
        *out_alpha_db_per_km = best_alpha;
    }

    return best_freq;
}


/*
 * Convert depth into one of the three NSL zones.
 */
int depth_zone_from_depth_m(double depth_m)
{
    if (depth_m <= 10.0) {
        return 1;
    }

    if (depth_m <= 50.0) {
        return 2;
    }

    return 3;
}


/*
 * Return the NSL corresponding to frequency and depth zone.
 */
double noise_spectral_level_db(double freq_khz, int depth_zone)
{
    int i;

    int zone_idx = depth_zone - 1;

    if (zone_idx < 0) {
        zone_idx = 0;
    }

    if (zone_idx > 2) {
        zone_idx = 2;
    }

    for (i = 0; i < 5; i++) {

        if (CANDIDATE_FREQUENCIES_KHZ[i] == freq_khz) {
            return NSL_TABLE[i][zone_idx];
        }
    }

    return -1.0;
}


/*
 * Look up pulse duration based on target range.
 *
 * The first table entry whose range is >= requested range
 * is selected.
 */
double pulse_duration_lookup_us(double range_km)
{
    int i;

    int n = (int)(
        sizeof(RANGE_TABLE_KM) /
        sizeof(RANGE_TABLE_KM[0])
    );

    for (i = 0; i < n; i++) {

        if (RANGE_TABLE_KM[i] >= range_km) {
            return PULSE_DURATION_TABLE_US[i];
        }
    }

    return PULSE_DURATION_TABLE_US[n - 1];
}


/*
 * --------------------------------------------------------------------------
 * Calculate all electrical/acoustic parameters for ONE frequency.
 * --------------------------------------------------------------------------
 *
 * Steps:
 *
 * 5. Acoustic SL -> electrical Vrms
 * 6. Vrms -> Vpp
 * 7. Vrms -> electrical power
 *
 * The power constraint is handled by calculate_transmit_parameters().
 */
static void calculate_one_frequency(double fc_khz,
                                    double alpha_db_per_km,
                                    const sensor_input_t *in,
                                    calculation_result_t *result)
{
    int zone = depth_zone_from_depth_m(in->depth_m);

    double nsl = noise_spectral_level_db(
        fc_khz,
        zone
    );

    /*
     * Step 9:
     * Bandwidth = k * centre frequency
     */
    double bw_khz = BW_K_FACTOR * fc_khz;


    /*
     * Transmission Loss
     *
     * Spreading loss uses metres.
     * Absorption uses kilometres.
     */
    double range_m = in->target_range_km * 1000.0;

    double tl =
          20.0 * log10(range_m)
        + alpha_db_per_km * in->target_range_km
        + 0.002 * fc_khz * in->turbidity_c;


    /*
     * Noise Level
     */
    double nl =
        nsl + 10.0 * log10(bw_khz);


    /*
     * Required Source Level
     *
     * SLreq = SNR threshold
     *       + 2*TL
     *       - Target Strength
     *       + Noise Level
     *       - Detection Index
     */
    double sl_req =
          SNR_THRESHOLD_DB
        + 2.0 * tl
        - TARGET_STRENGTH_DB
        + nl
        - DETECTION_INDEX_DB;


    /*
     * ----------------------------------------------------------------------
     * STEP 5: Acoustic Level -> Electrical RMS Voltage
     *
     * Vrms = Vref * 10^((SL - SLref)/20)
     *
     * Reference:
     *
     * SLref = 150 dB re 1 uPa @ 1 m
     * Vref  = 1 Vrms
     *
     * Example:
     *
     * SL = 128.26 dB
     *
     * Vrms ~= 0.0818 V
     * ----------------------------------------------------------------------
     */
    double vrms_v =
        V_REFERENCE_RMS_V *
        pow(
            10.0,
            (sl_req - SL_REFERENCE_DB_RE_UPA) / 20.0
        );


    /*
     * ----------------------------------------------------------------------
     * STEP 6: RMS Voltage -> Peak-to-Peak Voltage
     *
     * For a sinusoidal waveform:
     *
     * Vpp = 2 * sqrt(2) * Vrms
     * ----------------------------------------------------------------------
     */
    double vpp_v =
        2.0 * sqrt(2.0) * vrms_v;


    /*
     * ----------------------------------------------------------------------
     * STEP 7: Electrical Voltage -> Power
     *
     * P = Vrms^2 / R
     *
     * R = 50 ohms
     * ----------------------------------------------------------------------
     */
    double power_w =
        (vrms_v * vrms_v) /
        ELECTRICAL_LOAD_OHM;
        double power_mw = power_w * 1000.0;


    /*
     * Store calculated results.
     */
    result->freq_khz          = fc_khz;
    result->alpha_db_per_km   = alpha_db_per_km;
    result->tl_db             = tl;
    result->nl_db             = nl;
    result->sl_req_db         = sl_req;

    /*
     * amplitude_upa now stores the acoustic amplitude corresponding
     * to the required source level.
     *
     * This preserves the existing structure of calculation_result_t.
     */
    result->amplitude_upa =
        pow(10.0, sl_req / 20.0);

    result->bw_khz =
        bw_khz;

    result->pulse_duration_us =
        pulse_duration_lookup_us(
            in->target_range_km
        );

   result->vpp_v =
    vpp_v;

result->vrms_v =
    vrms_v;

result->power_w =
    power_w;

result->power_mw =
    power_mw;
}


/*
 * --------------------------------------------------------------------------
 * Adaptive transmit parameter calculation
 * --------------------------------------------------------------------------
 *
 * Frequency sequence:
 *
 * 500 kHz
 *    ↓ if power > limit
 * 400 kHz
 *    ↓ if power > limit
 * 300 kHz
 *    ↓ if power > limit
 * 200 kHz
 *    ↓ if power > limit
 * 100 kHz
 *
 * The algorithm stops as soon as the calculated power is within
 * POWER_THRESHOLD_W.
 */
void calculate_transmit_parameters(double fc_initial_khz,
                                    double alpha_db_per_km_initial,
                                    const sensor_input_t *in,
                                    calculation_result_t *result)
{
    int idx;
    int i;

    double fc = fc_initial_khz;
    double alpha = alpha_db_per_km_initial;

    int steps = 0;


    /*
     * Find the initial frequency in the candidate list.
     */
    idx = -1;

    for (i = 0; i < 5; i++) {

        if (CANDIDATE_FREQUENCIES_KHZ[i] == fc) {

            idx = i;
            break;
        }
    }


    /*
     * If the initial frequency isn't exactly one of the
     * candidate frequencies, start from the highest frequency.
     */
    if (idx < 0) {
        idx = 0;
        fc = CANDIDATE_FREQUENCIES_KHZ[0];

        alpha = absorption_coefficient_db_per_km(
            fc,
            in->temperature_c,
            in->salinity_psu,
            in->depth_m,
            SEAWATER_PH_TYPICAL
        );
    }


    /*
     * Adaptive frequency loop.
     */
    for (;;) {

        /*
         * Calculate all parameters for the current frequency.
         */
        calculate_one_frequency(
            fc,
            alpha,
            in,
            result
        );


        /*
         * Check electrical power constraint.
         */
        if (result->power_w <= POWER_THRESHOLD_W) {

            result->power_ok = 1;
            break;
        }


        /*
         * If already at 100 kHz, this is the minimum allowed
         * frequency. Stop even if power is still above the limit.
         */
        if (idx >= 4) {

            result->power_ok = 0;
            break;
        }


        /*
         * Step down to the next lower frequency.
         */
        idx++;

        fc =
            CANDIDATE_FREQUENCIES_KHZ[idx];


        /*
         * Recalculate absorption at the new frequency.
         */
        alpha =
            absorption_coefficient_db_per_km(
                fc,
                in->temperature_c,
                in->salinity_psu,
                in->depth_m,
                SEAWATER_PH_TYPICAL
            );

        steps++;
    }


    /*
     * Number of frequency reductions performed.
     *
     * Example:
     *
     * 500 -> 400 -> 300
     *
     * step_down_count = 2
     */
    result->step_down_count = steps;
}
