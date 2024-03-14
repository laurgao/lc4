/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>

// Laura's helper functions
// Common bits of the opcode to retrieve

unsigned char getReg1(unsigned short int instruction)
{
    // get the first register in the opcode (bits 9-11)
    return (instruction >> 9) & 0x7;
}

unsigned char getReg2(unsigned short int instruction)
{
    // get the second register (bits 6-8)
    return (instruction >> 6) & 0x7;
}

unsigned char getReg3(unsigned short int instruction)
{
    // get the third register (bits 0-2)
    return instruction & 0x7;
}

void fputhex1(char input, FILE *output)
{
    fprintf(output, "%X", (int)input);
}

// convert int to 4 digit hex string
void fputhex4(unsigned short int input, FILE *output)
{
    char hexStr[5];
    sprintf(hexStr, "%04X", input); // Convert integer to hex string
    fputs(hexStr, output);
}

short int sign_extend_5_to_16(short int num)
{
    // Check if the sign bit (5th bit) is set
    if (num & 0x10)
    {
        // If sign bit is set, extend by setting higher bits to 1
        return num | 0xFFE0; // 0xFFE0 has the higher 11 bits set to 1
    }
    else
    {
        // If sign bit is not set, return the number as is
        return num;
    }
}

short int sign_extend_6_to_16(short int num)
{
    // Check if the sign bit (6th bit) is set
    if (num & 0x20)
    {
        // If sign bit is set, extend by setting higher bits to 1
        return num | 0xFFC0; // 0xFFC0 has the higher 10 bits set to 1
    }
    else
    {
        // If sign bit is not set, return the number as is
        return num;
    }
}

short int sign_extend_9_to_16(short int num)
{
    // Check if the sign bit (9th bit) is set
    if (num & 0x100)
    {
        // If sign bit is set, extend by setting higher bits to 1
        return num | 0xFE00; // 0xFE00 has the higher 7 bits set to 1
    }
    else
    {
        // If sign bit is not set, return the number as is
        return num;
    }
}

short int sign_extend_11_to_16(short int num)
{
    // Check if the sign bit (11th bit) is set
    if (num & 0x400)
    {
        // If sign bit is set, extend by setting higher bits to 1
        return num | 0xF800; // 0xF800 has the higher 5 bits set to 1
    }
    else
    {
        // If sign bit is not set, return the number as is
        return num;
    }
}

//////////////////////////

/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState *CPU)
{
    CPU->PC = 0x8200;
    CPU->PSR = 0x8002; // set psr[15] to 1 and psr[1] to 1 (https://edstem.org/us/courses/44946/discussion/3961721)
    for (int i = 0; i < 8; i++)
    {
        CPU->R[i] = 0;
    }
    for (int i = 0; i < 65536; i++)
    {
        CPU->memory[i] = 0;
    }
}

/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState *CPU)
{
    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;

    CPU->NZPVal = 0;
    CPU->regInputVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
}

/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState *CPU, FILE *output)
{
    // write pc in hex
    fputhex4(CPU->PC, output);
    fputs(" ", output);

    // write instruction in bin
    char instructionBinStr[17];
    sprintf(instructionBinStr, "%016b", CPU->memory[CPU->PC]); // Convert integer to binary string
    fputs(instructionBinStr, output);
    fputs(" ", output);

    // regFile WE/input register/register value
    fputhex1(CPU->regFile_WE, output);
    fputs(" ", output);
    fputhex1(CPU->regFile_WE == 0 ? 0 : CPU->rdMux_CTL, output);
    fputs(" ", output);
    fputhex4(CPU->regFile_WE == 0 ? 0 : CPU->regInputVal, output);
    fputs(" ", output);

    // NZP WE/value
    fputhex1(CPU->NZP_WE, output);
    fputs(" ", output);
    fputhex1(CPU->NZP_WE == 0 ? 0 : CPU->NZPVal, output);
    fputs(" ", output);

    // data WE/addr/value
    if (CPU->DATA_WE == 0)
    {
        CPU->dmemAddr = 0;
        CPU->dmemValue = 0;
    }
    fputhex1(CPU->DATA_WE, output);
    fputs(" ", output);
    fputhex4(CPU->dmemAddr, output);
    fputs(" ", output);
    fputhex4(CPU->dmemValue, output);

    fputs("\n", output);
}

/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState *CPU, FILE *output)
{
    // if the PC is in data then return 1
    // data is 0x2000 to 0x7FFF and 0xA000 to 0xFFFF
    if ((CPU->PC >= 0x2000 && CPU->PC <= 0x7FFF) || (CPU->PC >= 0xA000 && CPU->PC <= 0xFFFF))
    {
        printf("Exception: Attempted to execute data\n");
        return 1;
    }
    // If we are in OS code and the most significant bit of the PSR is 0, then we throw exception and return 1
    if (CPU->PC >= 0x8000 && CPU->PC <= 0x9FFF)
    {
        // if the most significant bit of the PSR is 0, then we are in OS code
        // if it's 1, then we are in user code and we return 1
        if (!(CPU->PSR & 0x8000))
        {
            printf("Exception: Attempted to execute OS code while in user mode\n");
            return 1;
        }
    }

    // read in one instruction
    unsigned short int instruction = CPU->memory[CPU->PC];

    // print instruction and address
    // printf("executing instruction at PC: ");
    // printf("%X ", CPU->PC);
    // printf("%04X ", instruction);
    // printf("%016b ", instruction);
    // printf("\n");

    // if PC is 0x80FF, then we are done
    if (CPU->PC == 0x80FF)
    {
        return 1;
    }
    // extrac the opcode from the instruction and set cases based on it
    unsigned short int opcode = instruction >> 12;
    // print opcode and print instruction in binaryu

    switch (opcode)
    {
    case 0:
        // branch
        BranchOp(CPU, output);
        break;
    case 1:
        // arithmetic
        ArithmeticOp(CPU, output);
        break;
    case 2:
        // cmp
        ComparativeOp(CPU, output);
        break;
    case 4:
        // jsr
        JSROp(CPU, output);
        break;
    case 5:
        // and/not/or/xor
        LogicalOp(CPU, output);
        break;
    case 6:
        // LDR
        // set control signals
        CPU->NZP_WE = 1; // ldr and str don't affect nzp
        CPU->DATA_WE = 0;
        CPU->regFile_WE = 1;

        CPU->rdMux_CTL = getReg1(instruction);
        CPU->rsMux_CTL = getReg2(instruction);
        short int imm6 = instruction & 0x3F;
        imm6 = sign_extend_6_to_16(imm6);
        unsigned short int address = CPU->R[CPU->rsMux_CTL] + imm6;
        CPU->R[CPU->rdMux_CTL] = CPU->memory[address];
        CPU->regInputVal = CPU->R[CPU->rdMux_CTL];

        // If the address is in OS data then we throw exception and return 1
        if (address >= 0xA000 && address <= 0xFFFF)
        {
            // if the most significant bit of the PSR is 1, then we are in OS code
            // if it's 0, then we are in user code and we return 1
            if (!(CPU->PSR & 0x8000))
            {
                printf("Exception: Attempted to write to OS code or data while in user mode\n");
                return 1;
            }
        }

        // If the address is in code then we throw exception and return 1
        if (address < 0x2000 || (address >= 0x8000 && address <= 0x9FFF))
        {
            printf("Exception: Attempted to load data from a code address\n");
            return 1;
        }

        WriteOut(CPU, output);
        CPU->PC++;
        break;
    case 7:
        // STR
        // set control signals
        CPU->NZP_WE = 0; // ldr and str don't affect nzp
        CPU->DATA_WE = 1;
        CPU->regFile_WE = 0;

        CPU->rtMux_CTL = getReg1(instruction);
        CPU->rsMux_CTL = getReg2(instruction);
        imm6 = instruction & 0x3F;
        imm6 = sign_extend_6_to_16(imm6);
        CPU->dmemAddr = CPU->R[CPU->rsMux_CTL] + imm6;
        CPU->dmemValue = CPU->R[CPU->rtMux_CTL];

        // If the address is in OS data then we throw exception and return 1
        if (CPU->dmemAddr >= 0xA000 && CPU->dmemAddr <= 0xFFFF)
        {
            // if the most significant bit of the PSR is 1, then we are in OS code
            // if it's 0, then we are in user code and we return 1
            if (!(CPU->PSR & 0x8000))
            {
                printf("Exception: Attempted to write to OS data while in user mode. Address: %04X\n", CPU->dmemAddr);
                return 1;
            }
        }

        // If the address is in code then we throw exception and return 1
        if (CPU->dmemAddr < 0x2000 || (CPU->dmemAddr >= 0x8000 && CPU->dmemAddr <= 0x9FFF))
        {
            printf("Exception: Attempted to store to code address %04X\n", CPU->dmemAddr);
            // print instruction, immediate, and rsmux ctl
            printf("Instruction: %016b\n", instruction);
            printf("Immediate: %016b\n", imm6);
            printf("Rs: %016b\n", CPU->R[CPU->rsMux_CTL]);

            return 1;
        }

        // print the address
        printf("STR Address: %04X\n", CPU->dmemAddr);

        CPU->memory[CPU->dmemAddr] = CPU->dmemValue;

        WriteOut(CPU, output);
        CPU->PC++;
        break;
    case 8:
        // rti
        // set psr bit 15 to 0
        CPU->PSR &= 0x7FFF;

        // set control signals
        CPU->NZP_WE = 0;
        CPU->DATA_WE = 0;
        CPU->regFile_WE = 0;

        WriteOut(CPU, output);
        CPU->PC = CPU->R[7];
        break;
    case 9:
        // CONST Rd IMM9
        CPU->rdMux_CTL = getReg1(instruction);
        short int imm9 = instruction & 0x1FF;
        imm9 = sign_extend_9_to_16(imm9);
        CPU->R[CPU->rdMux_CTL] = imm9;

        CPU->DATA_WE = 0;
        CPU->regFile_WE = 1;
        CPU->NZP_WE = 1;
        CPU->regInputVal = imm9;
        SetNZP(CPU, imm9);

        WriteOut(CPU, output);
        CPU->PC++;
        break;
    case 10:
        // shift
        ShiftModOp(CPU, output);
        break;
    case 12:
        // jmp
        JumpOp(CPU, output);
        break;
    case 13:
        // HICONST Rd, UIMM8
        // Rd = (Rd & 0xFF) | (UIMM8 << 8)

        // set control signals
        CPU->NZP_WE = 1;
        CPU->DATA_WE = 0;
        CPU->regFile_WE = 1;

        CPU->rdMux_CTL = getReg1(CPU->memory[CPU->PC]);
        short int UIMM8 = CPU->memory[CPU->PC] & 0xFF;
        CPU->regInputVal = (CPU->R[CPU->rdMux_CTL] & 0xFF) | (UIMM8 << 8);
        CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;

        SetNZP(CPU, CPU->regInputVal);
        WriteOut(CPU, output);
        CPU->PC++;
        break;
    case 15:
        // TRAP

        // set control signals
        CPU->NZP_WE = 1;
        CPU->NZPVal = 1;
        CPU->DATA_WE = 0;
        CPU->regFile_WE = 1;

        // set R7 to PC + 1
        CPU->regInputVal = CPU->PC + 1;
        CPU->rdMux_CTL = 7;
        CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;
        // get the trap vector (bits 0-7)
        unsigned short int trapVector = CPU->memory[CPU->PC] & 0xFF;

        // set psr[15] to 1
        CPU->PSR |= 0x8000;

        WriteOut(CPU, output);
        // PC = (x8000 | trapVector)
        CPU->PC = 0x8000 | trapVector;
        break;
    default:
        // invalid opcode
        printf("Invalid opcode: %d\n", opcode);
        return 1;
    }
    return 0;
}

//////////////// PARSING HELPER FUNCTIONS ///////////////////////////

/*
 * Parses rest of branch operation and updates state of machine.
 */
void BranchOp(MachineState *CPU, FILE *output)
{
    // set control signals
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
    CPU->regFile_WE = 0;

    // extract the condition bits. any 000-111 is valid representing nzp respectively
    // (bits 9, 10, 11)
    unsigned short int condition = getReg1(CPU->memory[CPU->PC]);
    // extract the PCoffset9
    short int PCoffset9 = CPU->memory[CPU->PC] & 0x1FF;
    // sign extend to 16 bits
    PCoffset9 = sign_extend_9_to_16(PCoffset9);

    // calculate the new PC
    unsigned short int newPC = CPU->PC + PCoffset9 + 1;

    // print the machine state before updating the PC
    WriteOut(CPU, output);

    // check if the condition is met
    unsigned short int nzp = CPU->PSR & 0x7;
    unsigned short int conditionMet = nzp & condition;
    if (conditionMet)
    {
        // set the PC to the new PC
        CPU->PC = newPC;
    }
    else
    {
        // increment the PC
        CPU->PC++;
    }
}

/*
 * Parses rest of arithmetic operation and prints out.
 */
void ArithmeticOp(MachineState *CPU, FILE *output)
{

    // get bit 5 of the instruction
    short int bit5 = (CPU->memory[CPU->PC] >> 5) & 0x1;

    CPU->rdMux_CTL = getReg1(CPU->memory[CPU->PC]);
    CPU->rsMux_CTL = getReg2(CPU->memory[CPU->PC]);
    short int result;
    short int immediate;

    switch (bit5)
    {
    case 0:
        // get the second operand (bits 0-2) (Rt)
        CPU->rtMux_CTL = getReg3(CPU->memory[CPU->PC]);
        // get the opcode (bits 3-4)
        short int opcode = (CPU->memory[CPU->PC] >> 3) & 0x3;
        // calculate the result
        switch (opcode)
        {
        case 0:
            // ADD
            result = CPU->R[CPU->rsMux_CTL] + CPU->R[CPU->rtMux_CTL];
            break;
        case 1:
            // MUL
            result = CPU->R[CPU->rsMux_CTL] * CPU->R[CPU->rtMux_CTL];
            break;
        case 2:
            // SUB
            result = CPU->R[CPU->rsMux_CTL] - CPU->R[CPU->rtMux_CTL];
            break;
        case 3:
            // DIV
            result = CPU->R[CPU->rsMux_CTL] / CPU->R[CPU->rtMux_CTL];
            break;
        default:
            // invalid opcode
            printf("Invalid opcode: %d\n", opcode);
            break;
        }
        break;
    case 1:
        // get the immediate value (bits 0-4)
        immediate = CPU->memory[CPU->PC] & 0x1F;
        immediate = sign_extend_5_to_16(immediate);
        // perform addition
        result = CPU->R[CPU->rsMux_CTL] + immediate;
        break;
    }

    // set the result
    CPU->regInputVal = result;
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;

    // set control signals
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regFile_WE = 1;

    SetNZP(CPU, result);

    // print the machine state after the instruction
    WriteOut(CPU, output);

    CPU->PC++;
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState *CPU, FILE *output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    // set control signals
    CPU->rsMux_CTL = getReg1(instruction);
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regFile_WE = 0;

    // get the sub opcode (bits 7-8)
    short int subOpcode = (CPU->memory[CPU->PC] >> 7) & 0x3;
    unsigned short int result;
    short int immediate;

    switch (subOpcode)
    {
    case 0:
        // CMP
        CPU->rtMux_CTL = getReg3(instruction);
        // subtract the two registers and set the NZP bits (signed)
        // we have to interpret the registers as signed bits
        // please
        SetNZP(CPU, CPU->R[CPU->rsMux_CTL] - CPU->R[CPU->rtMux_CTL]);
        break;
    case 1:
        // CMPU
        CPU->rtMux_CTL = getReg3(instruction);
        // subtract the two registers and set the NZP bits (but do it unsigned)
        result = (unsigned short int)(CPU->R[CPU->rsMux_CTL] - CPU->R[CPU->rtMux_CTL]);
        SetNZP(CPU, result);
        break;
    case 2:
        // CMPI
        // get the immediate (bits 0-6)
        immediate = CPU->memory[CPU->PC] & 0x7F;
        // subtract the register and the immediate and set the NZP bits (signed)
        SetNZP(CPU, CPU->R[CPU->rsMux_CTL] - immediate);
        break;
    case 3:
        // CMPIU
        // get immediate
        immediate = CPU->memory[CPU->PC] & 0x7F;
        // subtract the register and the immediate and set the NZP bits (unsigned)
        result = (unsigned short int)(CPU->R[CPU->rsMux_CTL] - immediate);
        SetNZP(CPU, result);
        break;
    default:
        // invalid sub opcode
        printf("Invalid sub opcode: %d\n", subOpcode);
        break;
    }
    WriteOut(CPU, output);
    CPU->PC++;
}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState *CPU, FILE *output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];

    // set control signals
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regFile_WE = 1;

    // set Rd
    CPU->rdMux_CTL = getReg1(instruction);
    // set Rs
    CPU->rsMux_CTL = getReg2(instruction);
    // get bit 5
    short int bit5 = (CPU->memory[CPU->PC] >> 5) & 0x1;

    short int immediate;
    short int result;
    if (bit5 == 1)
    {
        // AND Rd Rs IMM5
        // get the immediate (bits 0-4)
        immediate = CPU->memory[CPU->PC] & 0x1F;
        immediate = sign_extend_5_to_16(immediate);
        // perform the AND operation
        result = CPU->R[CPU->rsMux_CTL] & immediate;
    }
    else
    {
        // get bits 4-3
        short int bits43 = (CPU->memory[CPU->PC] >> 3) & 0x3;

        // set Rt
        CPU->rtMux_CTL = getReg3(instruction);
        switch (bits43)
        {

        case 1:
            // NOT Rd Rs
            result = ~CPU->R[CPU->rsMux_CTL];
            break;
        case 2:
            // OR Rd Rs Rt
            result = CPU->R[CPU->rsMux_CTL] | CPU->R[CPU->rtMux_CTL];
            break;
        case 3:
            // XOR Rd Rs Rt
            result = CPU->R[CPU->rsMux_CTL] ^ CPU->R[CPU->rtMux_CTL];
            break;
        case 0:
            // AND Rd Rs Rt
            result = CPU->R[CPU->rsMux_CTL] & CPU->R[CPU->rtMux_CTL];
            break;
        default:
            // invalid bits 4-3
            printf("Invalid bits 4-3 in logic op: %d\n", bits43);
            break;
        }
    }

    CPU->regInputVal = result;
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;
    // set the nzp bits
    SetNZP(CPU, CPU->R[CPU->rdMux_CTL]);
    // write out
    WriteOut(CPU, output);
    // increment the PC
    CPU->PC++;
}

/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState *CPU, FILE *output)
{
    // set control signals
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
    CPU->regFile_WE = 0;

    unsigned short int newPC;

    // bit 11 tells you if this is a jmp or jmpr
    short int bit11 = (CPU->memory[CPU->PC] >> 11) & 0x1;
    short int PCoffset11;
    switch (bit11)
    {
    case 0:
        // JMPR
        // get the base register (bits 6-8)
        CPU->rsMux_CTL = getReg2(CPU->memory[CPU->PC]);
        // set the PC to the base register
        newPC = CPU->R[CPU->rsMux_CTL];
        break;
    case 1:
        // JMP
        // get the PCoffset11 (bits 0-10)
        PCoffset11 = CPU->memory[CPU->PC] & 0x7FF;
        PCoffset11 = sign_extend_11_to_16(PCoffset11);
        // set the PC to the PCoffset11
        newPC = CPU->PC + 1 + PCoffset11;
        break;
    }
    WriteOut(CPU, output);
    CPU->PC = newPC;
}

/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState *CPU, FILE *output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];

    // set control signals
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
    CPU->regFile_WE = 0;

    unsigned short int newPC;

    // set R7 to PC + 1
    CPU->R[7] = CPU->PC + 1;
    // bit 11 tells you if this is a jsr or jsrr
    short int bit11 = (instruction >> 11) & 0x1;
    short int PCoffset11;
    switch (bit11)
    {
    case 0:
        // JSRR
        // get the base register (bits 6-8)
        CPU->rsMux_CTL = getReg2(instruction);
        // set the PC to the base register
        newPC = CPU->R[CPU->rsMux_CTL];
        break;
    case 1:
        // JSR
        // get the PCoffset11 (bits 0-10)
        PCoffset11 = instruction & 0x7FF;
        PCoffset11 = sign_extend_11_to_16(PCoffset11);
        // set the PC to the PCoffset11
        newPC = CPU->PC + 1 + PCoffset11;
        break;
    }
    WriteOut(CPU, output);
    CPU->PC = newPC;
}

/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState *CPU, FILE *output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];

    // set control signals
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regFile_WE = 1;

    // get sub opcode (bits 4-5)
    short int subOpcode = (instruction >> 4) & 0x3;
    // get the first register (bits 9-11)
    CPU->rdMux_CTL = getReg1(instruction);
    // get the second register (bits 6-8)
    CPU->rsMux_CTL = getReg2(instruction);

    short int immediate;

    switch (subOpcode)
    {
    case 0:
        // <<
        // get the immediate (bits 0-3)
        immediate = instruction & 0xF;
        // shift the register left by the immediate
        CPU->R[CPU->rdMux_CTL] = CPU->R[CPU->rsMux_CTL] << immediate;
        break;
    case 1:
        // >>> (unsigned right shift)
        // get the immediate (bits 0-3)
        immediate = instruction & 0xF;
        // cast to unsigned
        unsigned short int unsigned_immediate = (unsigned short int)immediate;
        // shift the register right by the immediate
        CPU->R[CPU->rdMux_CTL] = CPU->R[CPU->rsMux_CTL] >> unsigned_immediate;
        break;
    case 2:
        // >>
        // get the immediate (bits 0-3)
        immediate = instruction & 0xF;
        // shift the register right by the immediate
        CPU->R[CPU->rdMux_CTL] = CPU->R[CPU->rsMux_CTL] >> immediate;
        break;
    case 3:
        // MOD
        // get Rt
        CPU->rtMux_CTL = getReg3(instruction);
        // get the remainder of the division of Rs and Rt
        CPU->R[CPU->rdMux_CTL] = CPU->R[CPU->rsMux_CTL] % CPU->R[CPU->rtMux_CTL];
        break;
    default:
        // invalid sub opcode
        printf("Invalid sub opcode: %d\n", subOpcode);
        break;
    }

    CPU->regInputVal = CPU->R[CPU->rdMux_CTL];
    // set the nzp bits
    SetNZP(CPU, CPU->R[CPU->rdMux_CTL]);
    // write out
    WriteOut(CPU, output);
    // increment the PC
    CPU->PC++;
}

/*
 * Set the NZP bits in the PSR.
 */
void SetNZP(MachineState *CPU, short int result)
{
    if (CPU->NZP_WE == 0)
    {
        return;
    }

    // the last 3 bits of the PSR are the NZP bits.
    // P: 001, Z: 010, N: 100
    // set the NZP bits:
    unsigned short and_mask = 0xFFF8; // Binary: 1111111111111000
    unsigned short or_mask;
    if (result == 0)
    {
        // Z
        or_mask = 0x0002; // Binary: 0000000000000010
    }
    else if (result > 0)
    {
        // P
        or_mask = 0x0001;
    }
    else
    {
        // N
        or_mask = 0x0004;
    }
    // to check if a number is negative, we check if the 15th bit is set
    // if it is, then the number is negative

    if (result & 0x8000) // Binary: 1000000000000000
    {
        // N
        or_mask = 0x0004;
    }
    CPU->PSR = (CPU->PSR & and_mask) | or_mask;

    // set the NZPVal as the last 3 bits of the PSR
    CPU->NZPVal = CPU->PSR & 0x7;
}
