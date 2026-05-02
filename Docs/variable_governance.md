# Variable Governance Baseline

## Goal

This project must keep variable lifetime, ownership, and write paths explicit.
No variable that influences control, safety, or UI flow may become a "public mutable bag".

The minimum rule is:

- every important variable has one owner
- every write path is intentional
- every cross-thread read is either a snapshot or a message/event
- no module is allowed to mutate another module's state by bypassing its API

## Ownership Classes

### 1. Module-private state

Use `static` file-scope variables for state that belongs to exactly one module.

Examples:

- `g_menu_state` in `app_menu.c`
- `g_oled_u8g2` in `hal_oled_if.c`
- `g_key_runtime[]` in `drv_keys.c`

Rules:

- only the owning `.c` file may modify it
- never expose it with `extern`
- expose operations through functions instead

### 2. Published runtime snapshot

`app_runtime_t` is the system-visible summary of current status.
It is **not** a free-for-all shared object.

Owner:

- `app_main.c`

Allowed writes:

- `appRuntimeIncrementSeconds()`
- `appRuntimeIncrementControlTicks()`
- `appRuntimeIncrementCommTicks()`
- `appRuntimePublishFastLoop()`

Allowed reads:

- `appRuntimeGetSnapshot()`

Rules:

- never cast a const pointer to writable
- never modify runtime fields directly from tasks or HAL
- multi-field reads must use snapshot copy, not piecemeal field access

### 3. Command object

`app_command_t` is the desired-control input for the control thread.

Owner:

- `app_main.c`

Allowed writes:

- `appCommandSubmitFromUi()`
- `appCommandSubmitFromComm()`
- `appCommandSubmitFromParam()`
- `appCommandSubmitFromSafety()`

Allowed reads:

- `appCommandGetSnapshot()`

Rules:

- future UI/comm/parameter modules must not hold writable command pointers
- command edits must pass through one API so source and revision can be traced

### 4. Algorithm internal state

Algorithm contexts such as PID, ramp, observer state belong to the algorithm module.

Examples:

- `g_current_pid`
- `g_current_ramp`
- `g_speed_observer`

Rules:

- owned only by `Algo`
- not directly modified by `App`
- reset/init through explicit algorithm functions only

## State Authority

There must be exactly one authority for each state class.

### Product state

Authority:

- `app_state_machine.c`

Published mirror:

- `app_runtime.state`

Interpretation:

- `g_app_state` is the true internal state
- `app_runtime.state` is a published summary for observers

### Fault flags

Authority:

- `app_fault_mgr.c` produces fault result from feedback

Published mirror:

- `app_runtime.fault_flags`

### Gate enable

Authority:

- control path decision in `app_fastloop.c`

Hardware mirror:

- low-level driver state in gate/GPIO path

## Cross-Thread Exchange Rules

Use one of these patterns only:

1. snapshot copy
2. event flag / message / queue
3. single-writer counter API

Forbidden patterns:

- exporting writable global structs
- direct field writes from multiple threads
- "temporary" casts that bypass ownership rules
- relying on `volatile` as a synchronization strategy

## Current Approved Write Matrix

### `app_runtime_t`

- main thread:
  - `rtos_seconds`
- control thread:
  - `control_ticks`
  - fastloop publish package
- comm thread:
  - `comm_ticks`

No other writer is allowed.

### `app_command_t`

- currently default command is owned by `app_main.c`
- future UI/comm writes must go through the source-specific submit APIs
- every command update must carry an explicit source tag
- UI menu actions may only edit a temporary command candidate and then submit it through `appCommandSubmitFromUi()`

### `app_menu_state_t`

- only UI thread through `app_menu.c`

### key runtime

- only key driver through `drv_keys.c`

## Review Checklist

When adding any variable, ask:

1. Who owns it?
2. Who is allowed to write it?
3. Who is allowed to read it?
4. Does it cross thread/ISR boundaries?
5. If yes, is access via snapshot/message/API instead of raw pointer?
6. If the system fails, can I trace all write sites in one grep?

If the answer to 6 is "no", the design is not acceptable.
