# Phase 2 (v2 diagram) code — read this first

Implements the three images shared 2026-09-13:
- Image 1: Adaptive Sonar Waveform Selection & Parameter Calculation Algorithm
- Image 2: Waveform selection flow (4-parameter low/medium/high tree)
- Image 3: Underwater Acoustic Calculations formula sheet

## Build
```
gcc -Wall -Wextra -std=c11 main_v2_demo.c absorption_model.c parameter_calculator.c waveform_selector.c -o sonar_v2_demo.exe -lm
./sonar_v2_demo.exe
```
(-lm must come AFTER the source files or the linker won't find sqrt/exp/pow/log10.)

## Verified against the images
- All 4 waveform-selection test scenarios (base/clear/harsh/medium) select
  the expected waveform type per Image 2's decision tree. PASS.
- Absorption formula matches Image 3 sections 3/3.1/3.2/3.3 term-for-term. PASS.

## OPEN ISSUE — needs team decision before this is trustworthy: range units
Image 1 labels the range knob "Rm (km)" and the pulse-duration table
"Range (km)", with alpha explicitly "dB/km". Taking that literally and
applying it consistently in TL = 20log10(Rm) + alpha*Rm + 0.002*fc*C
(all in km) produces physically broken results:
  - Near range (100 m i.e. 0.1 km): TL goes NEGATIVE (impossible)
  - Far range (100 km): required amplitude reaches ~10^9-10^12 uPa
    (impossible for a compact AUV transducer)

The standard textbook version of this formula mixes units on purpose:
  TL = 20*log10(R_in_metres) + alpha_dB_per_km * R_in_km + 0.002*fc*C
This gives sane numbers at both scales (45 dB at 100 m, ~109 dB at 100 km)
because 20log10 uses the conventional 1-metre reference distance, while
the absorption term legitimately uses km since alpha comes out of
Francois-Garrison as dB/km.

parameter_calculator.c currently implements the ALL-KM version (literal
to the diagram) so it can be checked against the source. Before this
code is trusted for real parameter values, confirm with whoever drew
Image 1 whether:
  (a) Rm should actually be metres, not km (matches the original
      Phase-1 range table, which used metres up to 200 m), or
  (b) the mixed-unit formula above was intended, or
  (c) something else entirely.
Once confirmed, only the TL line in calculate_transmit_parameters()
needs to change.

## Other assumptions baked in, also worth confirming
- The 6 candidate centre frequencies (50/100/200/300/400/500 kHz) were
  inferred from the 6 rows of the NSL table - Image 1 never lists them
  explicitly.
- NSL "Depth 1/2/3" zone boundaries (<=10m / 11-50m / >50m) were reused
  from Image 2's own depth thresholds - Image 1/3 never define them.
- Seawater pH = 8.1 (typical) - not one of the sensed parameters.

## Relationship to Phase 1 code (sonar_demo.c, shared earlier)
Kept completely separate on purpose. Phase 1 used a turbidity-only
waveform rule and a meter-based range/frequency table with a power-
constraint step-down loop; Phase 2 (this code) uses the 4-parameter
waveform tree and candidate-frequency-by-minimum-absorption instead,
with no power-constraint loop at all. These two are NOT reconciled -
that's a decision for the team, not something I resolved for you.
