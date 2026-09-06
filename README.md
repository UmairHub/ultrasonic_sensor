# HC-SR04 Ultrasonic Distance Sensor — STM32 Nucleo-F446RE

Bare-metal (register-level) driver for the HC-SR04 ultrasonic distance sensor, using polling with a hardware timer (TIM2) as an accurate microsecond stopwatch. Part of the v1 autonomous robot build — obstacle detection stage.

## How It Works

1. Send a 10µs HIGH pulse on **TRIG**
2. The sensor emits an ultrasonic burst and raises **ECHO** high
3. **ECHO** stays high for exactly as long as the sound takes to travel to an object and back
4. Distance (cm) = echo pulse duration (µs) / 58

## Pin Connections

| HC-SR04 Pin | Nucleo Pin | Function |
|-------------|------------|----------|
| VCC         | 5V         | Power (HC-SR04 requires 5V, not 3.3V) |
| GND         | GND        | Ground |
| TRIG        | PB0        | GPIO output — trigger pulse |
| ECHO        | PB1        | GPIO input — echo pulse (**via voltage divider**) |

## Critical Wiring Note — Voltage Divider Required

HC-SR04's **ECHO** output is 5V logic. STM32 GPIO inputs are 3.3V max and **not 5V tolerant** on this pin. A voltage divider is required between ECHO and PB1:

```
ECHO ---[1kΩ]---+---[2kΩ]--- GND
                 |
                PB1
```

This steps the 5V signal down to a safe ~3.3V before reaching the Nucleo. Do not connect ECHO directly to PB1.

## Timing Reference

- TIM2 configured as a free-running 32-bit counter at 1MHz (1 tick = 1µs), independent of delay-loop timing issues encountered elsewhere in this project.
- Trigger pulse: 10µs HIGH on TRIG.
- Echo timeout: ~30-40ms (covers the sensor's full ~4m range, corresponding to ~23ms round-trip time).
- Recommended minimum delay between trigger cycles: **60ms+**, to allow the previous ultrasonic burst to fully settle before retriggering. Triggering too soon after the previous reading is a known cause of intermittent timeout/invalid readings.

## Known Issue: Intermittent Timeouts

During initial testing, most readings returned a timeout (printed as `-1`, i.e. `0xFFFFFFFF` read as signed) with only occasional valid readings coming through. Suspected causes, in order of likelihood:

1. **Retrigger interval too short** — original test loop used a ~50ms delay (itself imprecise, from an uncalibrated `for`-loop delay) between readings, under the recommended 60ms+ settle time.
2. **Timeout value borderline** — original 30ms timeout is close to the sensor's max-range round-trip time; increasing to 35-40ms gives more margin.
3. **Wiring reliability** — loose/marginal jumper wire connections on TRIG or ECHO (a recurring issue in this project's other sensor wiring) can cause intermittent failures.

**Next steps to resolve**: increase the delay between trigger cycles, increase timeout margins, and verify firm wiring connections (re-seat jumpers, check voltage divider resistor connections).

## Code Structure

- `TIM2_Init_Microsecond()` — configures TIM2 as a 1MHz free-running counter for accurate timing, independent of CPU clock/loop-delay uncertainty.
- `HCSR04_GPIO_Init()` — configures PB0 (TRIG) as output, PB1 (ECHO) as input.
- `HCSR04_Trigger()` — sends the 10µs trigger pulse using TIM2 for precise timing.
- `HCSR04_ReadDistanceCM()` — triggers a reading, measures the ECHO pulse width via TIM2, returns distance in cm (or `0xFFFFFFFF` on timeout/no echo).

## Status

- ✅ Basic triggering and echo measurement confirmed working (at least one valid reading obtained: 39 cm)
- ⬜ Reliability fix in progress (see Known Issue above)
- ⬜ Not yet integrated into main robot avoidance logic — this is a standalone test project

## Related

Part of the v1 build plan: motor (L298N) + IMU (MPU6050) + distance sensor (HC-SR04) → reactive obstacle avoidance. Encoders deferred for now (motor on hand has no encoder).
