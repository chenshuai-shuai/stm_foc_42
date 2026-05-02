# STM32 FOC Four-Layer Architecture Baseline

## 1. Goal

This project adopts a fixed four-layer architecture:

- `App`: application orchestration and product behavior
- `Algo`: motor-control algorithms and control data flow
- `Hal`: hardware abstraction for board-facing services
- `Drv`: low-level peripheral drivers and ISR-facing code

The goals are:

- high readability
- high real-time determinism
- clear interrupt boundaries
- easy reuse across future motor projects

## 2. Core Rules

The rules below are mandatory.

1. Calls are only allowed from upper layer to lower layer.
2. `App` cannot directly touch STM32 registers or StdPeriph.
3. `Algo` cannot directly call `Drv`.
4. `Drv` cannot contain business logic, state-machine policy, or control strategy.
5. Fast current loop stays in ISR path, not in RTOS thread.
6. RTOS threads are used for slow management, communication, UI, diagnostics, and mode control.
7. Shared data between ISR and thread context must have a single owner and a defined exchange point.
8. Dynamic allocation is not allowed in hard real-time paths.
9. Blocking APIs are forbidden in ISR and forbidden in high-rate control paths.
10. Every module exposes one `.h` for interface and one `.c` for implementation unless there is a clear reason not to.

## 3. Layer Responsibilities

### 3.1 App Layer

The `App` layer owns:

- product state machine
- startup and shutdown sequences
- fault recovery flow
- task creation and scheduling policy
- parameter persistence policy
- communication command dispatch
- UI behavior

The `App` layer must not own:

- PWM register writes
- ADC sample capture
- Clarke/Park/SVPWM math details
- board pin toggling details

Typical files:

- `app_main.c`
- `app_task_control.c`
- `app_task_comm.c`
- `app_task_ui.c`
- `app_state_machine.c`
- `app_fault_mgr.c`

### 3.2 Algo Layer

The `Algo` layer owns:

- Clarke/Park transforms
- current loop
- speed loop
- position loop
- observers and estimators
- trajectory and ramp generation
- current/voltage limiting
- motor control context and parameter structures

The `Algo` layer must be hardware-agnostic as much as possible.

The `Algo` layer receives inputs such as:

- phase currents
- bus voltage
- rotor angle
- speed feedback
- target commands

The `Algo` layer outputs:

- voltage commands
- dq references
- duty requests
- fault and saturation flags

Typical files:

- `algo_foc.c`
- `algo_pid.c`
- `algo_observer.c`
- `algo_limit.c`
- `algo_ramp.c`
- `algo_motor_ctx.h`

### 3.3 Hal Layer

The `Hal` layer owns board-level abstraction.

It converts generic project intent into concrete board services:

- PWM start/stop
- ADC result fetch
- encoder read
- bus voltage read
- temperature read
- gate enable
- fault input sampling
- UART frame send
- OLED service

The `Hal` layer is the only layer allowed to combine multiple drivers into one board-facing function.

Typical files:

- `hal_motor_if.c`
- `hal_sense_if.c`
- `hal_gate_if.c`
- `hal_uart_if.c`
- `hal_oled_if.c`
- `hal_board.c`

### 3.4 Drv Layer

The `Drv` layer owns the lowest-level hardware access:

- GPIO setup
- TIM setup
- ADC setup
- DMA setup
- USART setup
- NVIC setup
- direct StdPeriph calls
- ISR entry functions

This layer may be very close to registers and StdPeriph APIs.
It must stay simple, predictable, and narrow.

Typical files:

- `drv_tim1_pwm.c`
- `drv_adc1_injected.c`
- `drv_dma.c`
- `drv_usart1.c`
- `drv_gpio.c`
- `drv_oled_i2c_soft.c`
- `drv_irq.c`

## 4. Real-Time Partitioning

The project uses a split control model.

### 4.1 ISR Path

The ISR path owns the fastest control work:

- ADC end-of-conversion handling
- current sample capture
- electrical angle update if it is coupled to fast timing
- FOC current loop
- duty update
- cycle-level protection response

The ISR path must be:

- fixed execution path
- branch-light
- allocation-free
- print-free
- lock-free whenever possible

### 4.2 RTOS Thread Path

The RTOS path owns slower tasks:

- motor mode state machine
- target command updates
- speed loop if bandwidth allows thread execution
- communication
- parameter service
- OLED refresh
- logging and telemetry

Recommended priority order:

1. hardware protection ISR
2. ADC/PWM synchronization ISR
3. control manager thread
4. communication thread
5. UI thread
6. background thread

## 5. Data Flow Contract

Use explicit data contracts between layers.

Recommended ownership:

- `Drv` owns raw peripheral state.
- `Hal` owns board-facing snapshots.
- `Algo` owns control context and control outputs.
- `App` owns command targets and product state.

Recommended exchange objects:

- `motor_feedback_t`
- `motor_command_t`
- `motor_control_output_t`
- `motor_fault_t`
- `app_runtime_t`

Rules:

1. ISR writes fast feedback snapshot.
2. `Algo` consumes the snapshot and produces control output.
3. `Hal` commits output to hardware.
4. `App` only observes summarized runtime state unless it needs a command path.

## 6. File Naming Convention

Use prefixes to make ownership obvious:

- `app_*` for App
- `algo_*` for Algo
- `hal_*` for Hal
- `drv_*` for Drv

Examples:

- `app_task_control.c`
- `algo_foc.c`
- `hal_motor_if.c`
- `drv_tim1_pwm.c`

## 7. Dependency Rules

Allowed include directions:

- `App -> Algo`
- `App -> Hal`
- `Algo -> Hal`
- `Hal -> Drv`
- `Drv -> Library`

Forbidden include directions:

- `Drv -> App`
- `Drv -> Algo`
- `Algo -> App`
- `Library -> project modules`

## 8. Recommended Initial Directory Layout

```text
App/
  app_main.c
  app_task_control.c
  app_task_comm.c
  app_task_ui.c
  app_state_machine.c

Algo/
  algo_foc.c
  algo_pid.c
  algo_limit.c
  algo_motor_ctx.h

Hal/
  hal_board.c
  hal_motor_if.c
  hal_sense_if.c
  hal_uart_if.c
  hal_oled_if.c

Drv/
  drv_gpio.c
  drv_tim1_pwm.c
  drv_adc1.c
  drv_usart1.c
  drv_irq.c
```

## 9. ChibiOS Integration Rules

`ChibiOS` is infrastructure, not a business layer.

Recommended placement:

- RTOS startup and thread creation live in `App`
- ISR handlers stay in `Drv`
- thread-safe board services live in `Hal`
- algorithm code remains RTOS-agnostic in `Algo`

Do not let `Algo` depend on `ChibiOS` headers unless absolutely necessary.

## 10. Definition of Done For New Modules

A new module is acceptable only if:

1. Its layer ownership is obvious from the filename.
2. It has a narrow public header.
3. It does not break dependency direction.
4. It does not introduce blocking behavior into hard real-time paths.
5. It has explicit comments only where the timing or hardware behavior is not self-evident.

## 11. Suggested First Refactor Sequence

1. Keep the current `ChibiOS` kernel integration as the system foundation.
2. Move future thread creation from `main.c` into `App`.
3. Build `Hal` interfaces for motor outputs, sensing, UART, and OLED.
4. Move fast peripheral control and interrupt entry into `Drv`.
5. Introduce `Algo` control context and keep FOC math isolated from hardware.
6. Keep `App` responsible for mode management, faults, and operator behavior.
