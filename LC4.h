// This file specifies the datatype of our MachineState object.
// The MachineState object is a simulator of the internal state of an LC4 computer.

#include "string.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    // program counter register -- stores current memory address we are running.
    unsigned short int PC;

    // PSR = program status register. records the status of the CPU.
    // PSR[0] = P, PSR[1] = Z, PSR[2] = N, PSR[15] = privillege bit (user mode/OS mode)
    unsigned short int PSR;

    // 8 registers x 16 bits each
    unsigned short int R[8];

    // control signals -- these are stored as 8 bits but we only use the 3 lower bits
    unsigned char rsMux_CTL;
    unsigned char rtMux_CTL;
    unsigned char rdMux_CTL;

    // one-bit control signals (we use the lowest bit) representing write-enabled permissions
    // on various registers.
    unsigned char regFile_WE;
    unsigned char NZP_WE;
    unsigned char DATA_WE;

    // helpful values
    unsigned short int regInputVal;
    unsigned short int NZPVal;
    unsigned short int dmemAddr;
    unsigned short int dmemValue;

    // 2^16 x 16 bit machine memory
    unsigned short int memory[65536];
} MachineState;

// Executes one LC4 datapath cycle
int UpdateMachineState(MachineState *CPU, FILE *output);

// Write out current CPU state to file output
void WriteOut(MachineState *CPU, FILE *output);

// various instructions:
void BranchOp(MachineState *CPU, FILE *output);
void ArithmeticOp(MachineState *CPU, FILE *output);
void ComparativeOp(MachineState *CPU, FILE *output);
void LogicalOp(MachineState *CPU, FILE *output);
void JumpOp(MachineState *CPU, FILE *output);
void JSROp(MachineState *CPU, FILE *output);
void ShiftModOp(MachineState *CPU, FILE *output);

// Sets NZP bits in the PSR
void SetNZP(MachineState *CPU, short result);

// resets the machine state. Like PennSim `reset` command.
void Reset(MachineState *CPU);

// set internal values to 0.
void ClearSignals(MachineState *CPU);
