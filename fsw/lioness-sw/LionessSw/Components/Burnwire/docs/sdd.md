# LionessSw::Burnwire

# Burnwire Software Design Description (SDD)

## Overview
The Burnwire component controls a burnwire actuator via a GPIO driver. It supports command-based activation with a safety timeout enforced through a scheduled rate group. The component ensures the burnwire is automatically turned off after a specified duration.

---

## Requirements

| ID     | Requirement |
|--------|------------|
| BW-001 | The burnwire shall turn on and off in response to commands |
| BW-002 | The burnwire component shall provide an event when it is turned on and off |
| BW-003 | The burnwire component shall activate by driving the GPIO pin(s) controlling the burnwire |
| BW-004 | The burnwire component shall be controlled by a safety timeout driven by a scheduled rate group |
| BW-005 | The safety timeout shall emit an event when it is set |
| BW-006 | The burnwire shall report the total burn duration when it stops |

---

## Ports

| Name    | Type            | Description                              |
|---------|-----------------|------------------------------------------|
| gpioSet | Drv::GpioWrite  | Sends GPIO state to the GPIO driver      |
| schedIn | Svc::Sched      | Scheduled execution input (rate group)   |

---

## Commands

| Name           | Description                                 |
|----------------|---------------------------------------------|
| START_BURNWIRE | Starts the burnwire for a specified duration |
| STOP_BURNWIRE  | Stops the burnwire immediately (always drives the GPIO LOW) |

---

## Events

| Name              | Description                                                   |
|-------------------|---------------------------------------------------------------|
| SetBurnwireState  | Emitted when burnwire turns ON or OFF                         |
| SafetyTimerState  | Emits the requested burn duration when burn starts            |
| BurnwireEndCount  | Reports the actual burn duration when burnwire stops          |
| GpioWriteFailed   | Warning when the GPIO driver returns a non-OK status          |

---

## Parameters

| Name         | Description                                              |
|--------------|----------------------------------------------------------|
| SAFETY_TIMER | Maximum allowed burn duration (seconds)                  |

---

## Component State

| Name      | Description                                      |
|-----------|--------------------------------------------------|
| m_state   | Tracks whether burnwire is ON or OFF              |
| m_timer   | Counts elapsed time (ticks/seconds)               |
| m_timeout | Maximum allowed burn duration (seconds)           |

---

## Behavior

1. **`START_BURNWIRE(durationS)`**
   - Rejects `durationS == 0` with `VALIDATION_ERROR`
   - Rejects the command with `BUSY` if a burn is already in progress
   - Clamps `durationS` to `SAFETY_TIMER` (30 s fallback if parameters were never loaded)
   - Sets `m_timeout`, resets `m_timer`, sets state ON
   - Emits `SafetyTimerState`

2. **`schedIn` (rate group, nominally 1 Hz)**
   - Does nothing while OFF
   - First tick after the command: GPIO HIGH, emits `SetBurnwireState(ON)`
   - After the GPIO has been HIGH for `m_timeout` ticks: GPIO LOW, emits
     `BurnwireEndCount` and `SetBurnwireState(OFF)`, returns to idle

3. **`STOP_BURNWIRE`**
   - If burning: same shutdown as the timeout path (reports ticks burned so far)
   - If idle: still drives the GPIO LOW, emits no events

4. **GPIO failures**: any non-`OP_OK` status from the driver emits
   `GpioWriteFailed`. The timer keeps running so a LOW is still commanded.

Commands and `schedIn` are `guarded` (share a component mutex) because they
run on the command dispatcher and rate group threads respectively.

---

## Unit Tests

Located in `test/ut/`, run with `fprime-util check` from this directory.

| Test | Covers |
|------|--------|
| IdleDoesNothing | No GPIO activity without a command |
| NominalBurn | BW-001..006: HIGH for exactly `durationS` ticks, then LOW |
| DefaultSafetyTimerClamp | Clamp to 30 s when parameter not loaded |
| ConfiguredSafetyTimerClamp | Clamp to a commanded `SAFETY_TIMER` value |
| ZeroDurationRejected | `durationS == 0` rejected, no GPIO |
| StartWhileBurningRejected | Second start returns BUSY, original burn unaffected |
| StopDuringBurn | Early stop drives LOW and reports elapsed ticks |
| StopBeforeFirstTick | Stop cancels a pending burn |
| StopWhileIdle | Stop is idempotent |
| BurnAgainAfterCompletion | Component can burn again after finishing |
| GpioFailureReported | Driver errors emit `GpioWriteFailed` |

---

## Notes

- The component assumes a 1 Hz scheduling rate for correct timing behavior.
- GPIO control is abstracted through the `Drv::GpioWrite` interface.
- Safety timeout prevents unintended prolonged burnwire activation.