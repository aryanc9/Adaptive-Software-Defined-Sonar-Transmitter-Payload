#include <math.h>
#include "absorption_model.h"

double sound_speed_mps(double temperature_c, double salinity_psu, double depth_m)
{
    /* c = 1412 + 3.21T + 1.19S + 0.0167D  (Image 3, section 2) */
    return 1412.0 + 3.21 * temperature_c + 1.19 * salinity_psu + 0.0167 * depth_m;
}

double absorption_coefficient_db_per_km(double freq_khz,
                                         double temperature_c,
                                         double salinity_psu,
                                         double depth_m,
                                         double ph)
{
    double T = temperature_c;
    double S = salinity_psu;
    double D = depth_m;
    double f = freq_khz;

    /* --- 3.1 Boric acid contribution --- */
    double f1 = 0.78 * sqrt(S / 35.0) * exp(T / 26.0);
    double alpha_b = 0.106 * (f1 * f1 * f) / (f1 * f1 + f * f)
                    * pow(10.0, (ph - 8.0) / 0.56);

    /* --- 3.2 Magnesium sulfate contribution --- */
    double f2 = 42.0 * exp(T / 17.0);
    double alpha_mg = 0.52 * (1.0 + T / 43.0) * (S / 35.0)
                     * (f2 * f2 * f) / (f2 * f2 + f * f)
                     * exp(-D / 6.0);

    /* --- 3.3 Pure water contribution --- */
    double alpha_w = 0.00049 * f * f * exp(-(T / 27.0 + D / 17.0));

    return alpha_b + alpha_mg + alpha_w; /* dB/km */
}
