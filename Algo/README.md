# Algo Layer

This layer owns motor-control algorithms and control data structures.

It should stay as hardware-agnostic as practical.
It may depend on `Hal` interfaces when hardware feedback or output abstraction is required.
It must not directly call low-level drivers.
