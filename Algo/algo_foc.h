#ifndef ALGO_FOC_H
#define ALGO_FOC_H

#include "algo_motor_types.h"

void algoFocInit(void);
void algoFocStep(const motor_feedback_t *feedback,
                 const motor_command_t *command,
                 motor_control_output_t *output);
void algoFocGetDiagnostics(algo_foc_diag_t *diag);

#endif /* ALGO_FOC_H */
