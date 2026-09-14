#ifndef ABSORPTION_MODEL_H
#define ABSORPTION_MODEL_H

/* Typical seawater pH. Not one of the listed sensors (T, S, D, C) in
 * either image, so this is a fixed assumption pending confirmation
 * with the team - real seawater pH commonly runs 7.5-8.5. */
#define SEAWATER_PH_TYPICAL 8.1

/* Sound speed (Image 3, formula 2). Informational only - not used
 * downstream by the absorption/TL chain, but printed for reference. */
double sound_speed_mps(double temperature_c, double salinity_psu, double depth_m);

/* Francois-Garrison absorption coefficient alpha(f,T,S,D) in dB/km.
 * f in kHz, T in degC, S in PSU, D in m, pH dimensionless.
 * Implements Image 3, sections 3/3.1/3.2/3.3 (boric acid + MgSO4 +
 * pure water contributions). Turbidity is intentionally NOT part of
 * this formula - per Image 3's own note, turbidity is applied later,
 * directly in the transmission-loss step (see parameter_calculator.c). */
double absorption_coefficient_db_per_km(double freq_khz,
                                         double temperature_c,
                                         double salinity_psu,
                                         double depth_m,
                                         double ph);

#endif /* ABSORPTION_MODEL_H */
