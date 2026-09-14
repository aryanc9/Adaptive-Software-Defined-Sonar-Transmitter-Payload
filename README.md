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

Power amplifier
  ↓
Transducer
  ↓
Transmit acoustic signal
  ↓
Monitor environment / feedback
  ↓
Repeat
```

## Example Operating Modes

  Mode      Typical Waveform     Objective               Relative Energy
  --------- -------------------- ----------------------- -----------------
  Economy   CW / short pulse     Minimum energy          Low
  Normal    LFM chirp            Balanced performance    Medium
  Robust    Phase-coded pulse    Better detection        High
  Maximum   High-bandwidth LFM   Difficult environment   Very High

The exact frequency, bandwidth, pulse width, amplitude and duty cycle
should be determined from the selected transducer, amplifier, operating
frequency range and acoustic calculations.

## Change Management

To avoid unnecessary waveform switching, the adaptive algorithm can use:

-   Sensor-change threshold
-   Hysteresis
-   Persistence checking
-   Critical-change detection
-   Minimum waveform dwell time

For example, a change can be confirmed only after several consecutive
sensor readings show the same environmental transition.

## System Architecture

``` text
┌──────────────────────┐
│ Environmental Sensors│
│                      │
│ Temperature          │
│ TDS / Salinity       │
│ Depth / Pressure     │
│ Turbidity             │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│      MCXN236         │
│    Microcontroller   │
│                      │
│ Acquire & Filter     │
│ Classify Environment │
│ Calculate Score      │
│ Select Mode          │
│ Acoustic Calculation │
│ Select Waveform      │
│ Generate / Control TX│
└──────────┬───────────┘
           │
           ▼
      ┌─────────┐
      │   DAC   │
      └────┬────┘
           ▼
   ┌───────────────┐
   │ Power Amplifier│
   └───────┬───────┘
           ▼
     ┌──────────┐
     │Transducer│
     └────┬─────┘
          ▼
   Underwater Signal
          │
          ▼
     Target / Echo
          │
          ▼
   Optional Feedback
          │
          └──────────────→ Adaptive Loop
```

## Hardware

The proposed prototype is based around an **NXP MCXN236
microcontroller** and includes:

-   MCXN236 development platform
-   Temperature sensor
-   TDS / salinity sensor
-   Depth / pressure sensor
-   Turbidity sensor
-   DAC or digital waveform interface
-   Power amplifier
-   Underwater acoustic transducer
-   Optional receiver / echo-feedback interface
-   Power supply

Actual electrical limits must be verified against the selected DAC,
amplifier and transducer datasheets.

## Software / Firmware Functions

The firmware is intended to implement:

1.  Sensor acquisition
2.  ADC processing
3.  Calibration and unit conversion
4.  Digital filtering
5.  Environmental classification
6.  Weighted scoring
7.  Operating-mode selection
8.  Acoustic calculations
9.  Waveform parameter calculation
10. Waveform generation
11. Hardware-limit checking
12. DAC / transmitter control
13. Continuous monitoring
14. Adaptive parameter updates

## Key Inputs

``` text
T       = Temperature (°C)
S       = Salinity (PSU)
D       = Depth (m)
f       = Frequency (kHz)
R       = Target range
pH      = Seawater pH
B       = Bandwidth (kHz)
Tp      = Pulse duration (s)
NL      = Ambient noise level (dB)
TS      = Target strength (dB)
SNRreq  = Required SNR (dB)
```

## Key Outputs

``` text
c          = Sound speed
α          = Absorption coefficient
TLabs      = Absorption loss
TLspread   = Spreading loss
TL         = Total transmission loss
SLrequired = Required source level
p          = Required acoustic pressure
VRMS       = Required RMS transducer voltage
VPP        = Peak-to-peak voltage
P          = Electrical transmit power
Epulse     = Energy per pulse
Pavg       = Average transmit power
```

## Why This Approach?

The key advantage of the project is that waveform selection and acoustic
power calculation are connected.

Instead of simply selecting a stronger waveform when the environment
becomes difficult, the system estimates the actual acoustic losses and
calculates the minimum transmission level required to satisfy the target
SNR.

This enables:

-   Environment-aware sonar operation
-   Physics-based parameter selection
-   Adaptive frequency and bandwidth
-   Adaptive pulse duration and amplitude
-   Reduced unnecessary transmit power
-   Improved energy efficiency
-   Real-time operation suitable for AUV applications
-   Closed-loop adaptation to changing underwater conditions

## Important Assumptions

-   The standard Francois--Garrison model is used for
    frequency-dependent absorption.
-   Spherical spreading is used as the basic spreading-loss
    approximation.
-   Shallow-water or long-range operation may require a cylindrical or
    hybrid propagation model.
-   Turbidity is not directly represented by the standard
    Francois--Garrison absorption model.
-   Transducer sensitivity must come from the actual transducer
    datasheet.
-   Electrical power calculated from `VRMS²/RL` represents load/signal
    power, not total battery power.
-   Amplifier efficiency and other system losses must be added for
    battery-power estimation.
-   Units must remain consistent throughout the calculation chain.

## Project Objective

The primary objective is to develop a compact, low-power,
software-defined sonar transmitter that can **sense the underwater
environment, calculate acoustic propagation requirements, select the
minimum sufficient waveform and transmission parameters, and
continuously adapt its operation in real time**.

## Repository Structure

A recommended repository structure is:

``` text
adaptive_sonar_transmitter/
firmware/
├── source/
│   ├── main.c
│   ├── app_state_machine.c
│   ├── sensor_acquisition.c
│   ├── sensor_processing.c
│   ├── environment_classifier.c
│   ├── adaptive_decision.c
│   ├── parameter_calculator.c
│   ├── safety_manager.c
│   ├── waveform_generator.c
│   ├── window_function.c
│   ├── dac_dma_output.c
│   ├── transmission_controller.c
│   ├── telemetry.c
│   └── power_manager.c
│
├── include/
│   ├── sonar_types.h
│   ├── project_config.h
│   ├── hardware_config.h
│   └── waveform_types.h
│
├── board/
│   ├── pin_mux.c
│   ├── clock_config.c
│   └── peripheral_config.c
│
└── README.md
```

## Project Status

**Development / Prototype**

The acoustic calculation framework and adaptive algorithm are defined.
Hardware-specific implementation requires final selection and
characterization of the transducer, amplifier, DAC/interface, sensors
and power system.

