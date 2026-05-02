# App Layer

This layer owns application behavior, RTOS task orchestration, state machines,
fault policy, and product-level logic.

It may call `Algo` and `Hal`.
It must not directly operate STM32 registers or StdPeriph peripherals.
