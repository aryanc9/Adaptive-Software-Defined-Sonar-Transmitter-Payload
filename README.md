# Physics-Based Adaptive Sonar System

A real-time, physics-based adaptive sonar system for underwater
exploration and marine mapping. The system uses environmental
measurements to select an appropriate sonar operating mode and then
calculates the acoustic and electrical transmission requirements needed
for reliable target detection with minimum sufficient energy.

## Project Overview

Conventional sonar systems often operate with fixed transmission
parameters. This project introduces an adaptive approach in which sonar
parameters are selected according to the current underwater environment.

The system measures:

-   Temperature
-   TDS / salinity
-   Depth / pressure
-   Turbidity

The measurements are calibrated, filtered, classified into environmental
zones, and converted into a weighted environmental score. The score
determines the minimum required operating mode:

**Economy → Normal → Robust → Maximum**

The selected mode determines the appropriate waveform and initial
transmission parameters. The system then performs physics-based acoustic
calculations to determine the actual transmission requirements.

## Core Acoustic Calculation

The acoustic calculation is the main engineering component of the
project.

The calculation chain is:

``` text
Environmental Conditions + Sonar Requirements
                    ↓
             Sound Speed
                    ↓
        Francois–Garrison Model
                    ↓
       Absorption Coefficient
                    ↓
          Absorption Loss
                    ↓
          Spreading Loss
                    ↓
       Total Transmission Loss
                    ↓
        Required Source Level
                    ↓
          Acoustic Pressure
                    ↓
        Transducer RMS Voltage
                    ↓
          Electrical Power
                    ↓
           Pulse Energy
                    ↓
          Average Power
```

### 1. Sound Speed

The system estimates underwater sound speed from temperature, salinity
and depth:

``` text
c = 1412 + 3.21T + 1.19S + 0.0167D
```

where `c` is sound speed in m/s.

### 2. Absorption Coefficient

Frequency-dependent acoustic absorption is calculated using the
Francois--Garrison model:

``` text
α = αB + αMg + αW
```

The model accounts for contributions from:

-   Boric acid
-   Magnesium sulfate
-   Pure water

Frequency, temperature, salinity, depth and pH are used as required by
the model.

> Turbidity is not directly included in the standard Francois--Garrison
> absorption equation. A separate empirical scattering/attenuation model
> can be added if turbidity-dependent acoustic attenuation is required.

### 3. Transmission Loss

Absorption loss:

``` text
TLabs = αR
```

Spreading loss for the spherical-spreading approximation:

``` text
TLspread = 20 log10(R)
```

Total transmission loss:

``` text
TL = TLspread + TLabs
```

The project keeps the range units consistent by using meters for the
spreading calculation and kilometers for the absorption term when `α` is
expressed in dB/km.

### 4. Required Source Level

For active sonar:

``` text
SNR = SL - TL - NL + TS
```

Therefore:

``` text
SLrequired = SNRreq + TL + NL - TS
```

This calculation determines the source level required to achieve the
specified detection SNR.

### 5. Acoustic Pressure

The required source level is converted to acoustic pressure using:

``` text
SL = 20 log10(p / pref)
```

Therefore:

``` text
p = pref × 10^(SL/20)
```

where:

``` text
pref = 1 µPa
```

### 6. Transducer Voltage

Using the actual transducer sensitivity:

``` text
VRMS = V0 × 10^((SLrequired - SL0)/20)
```

Peak-to-peak voltage:

``` text
VPP = 2√2 × VRMS
```

### 7. Electrical Power

For a resistive load:

``` text
P = VRMS² / RL
```

For example, for a 50 Ω load:

``` text
P = VRMS² / 50
```

### 8. Pulse Energy and Average Power

Energy per pulse:

``` text
Epulse = P × Tp
```

Duty cycle:

``` text
D = Tp / Tping
```

Average transmit power:

``` text
Pavg = P × D
```

These calculations allow the system to balance detection performance
against energy consumption.

## Adaptive Algorithm

The adaptive control process follows:

``` text
START
  ↓
Acquire sensor data
  ↓
Calibration and unit conversion
  ↓
Digital filtering
  ↓
Environmental zone classification
  ↓
Create environmental state
  ↓
Compare with previous state
  ↓
Check significant sensor changes
  ↓
Critical-change / persistence check
  ↓
Calculate weighted environmental score
  ↓
Select minimum sufficient operating mode
  ↓
Select waveform
  ↓
Calculate acoustic requirements
  ↓
Select transmission parameters
  ↓
Check hardware limits
  ↓
Generate waveform
  ↓
DAC / interface
  ↓
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
│
├── firmware/
│   ├── source/
│   │   ├── main.c
│   │   ├── app_controller.c
│   │   ├── environment_classifier.c
│   │   ├── adaptive_decision.c
│   │   ├── telemetry.c
│   │   ├── sensor_acquisition.c            ← later
│   │   ├── sensor_processing.c             ← later
│   │   ├── parameter_calculator.c          ← later
│   │   ├── safety_manager.c                ← later
│   │   ├── waveform_generator.c            ← later
│   │   ├── window_function.c               ← later
│   │   ├── dac_dma_output.c                ← later
│   │   └── transmission_controller.c       ← later
│   │
│   ├── include/
│   │   ├── sonar_types.h
│   │   ├── project_config.h
│   │   ├── app_controller.h
│   │   ├── environment_classifier.h
│   │   ├── adaptive_decision.h
│   │   ├── telemetry.h
│   │   ├── sensor_acquisition.h            ← later
│   │   ├── parameter_calculator.h          ← later
│   │   ├── safety_manager.h                ← later
│   │   ├── waveform_generator.h            ← later
│   │   └── dac_dma_output.h                ← later
│   │
│   ├── board/
│   │   ├── pin_mux.c                       ← generated / SDK-based
│   │   ├── pin_mux.h
│   │   ├── clock_config.c
│   │   ├── clock_config.h
│   │   └── peripheral_config.c             ← later
│   │
│   ├── generated/
│   │   └── Do not manually edit generated config files
│   │
│   ├── CMakeLists.txt / MCUXpresso project files
│   └── README.md
│
├── pc_analysis/
│   ├── notebooks/
│   │   ├── 01_algorithm_test.ipynb
│   │   ├── 02_lfm_chirp_test.ipynb
│   │   └── 03_fft_spectrogram.ipynb
│   │
│   ├── scripts/
│   │   ├── generate_waveform_table.py
│   │   ├── uart_logger.py
│   │   └── compare_mcu_python.py
│   │
│   └── requirements.txt
│
├── docs/
│   ├── architecture.md
│   ├── algorithm_assumptions.md
│   ├── hardware_interface.md
│   ├── test_plan.md
│   └── change_log.md
│
├── test_data/
│   ├── expected_outputs/
│   └── uart_logs/
│
├── waveform_tables/
│   └── README.md
│
├── .gitignore
└── README.md
```

## Project Status

**Development / Prototype**

The acoustic calculation framework and adaptive algorithm are defined.
Hardware-specific implementation requires final selection and
characterization of the transducer, amplifier, DAC/interface, sensors
and power system.

## License

Add the project's chosen license here, for example:

``` text
MIT License
```

## Acknowledgment

The acoustic calculation framework uses the Francois--Garrison approach
for seawater absorption and standard active-sonar transmission-loss and
source-level relationships.
