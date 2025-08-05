/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

// Global variable defining the current state of the machine
MachineState *CPU;

int main(int argc, char **argv)
{
    // Check if at least 2 arguments are provided
    if (argc < 3)
    {
        printf("Invalid arguments. Usage: ./trace <outputfile> <file1> [file2] ...\n");
        return -1;
    }

    // The first arg is output file
    char *output_filename = argv[1];

    CPU = (MachineState *)malloc(sizeof(MachineState));

    // reset and clear CPU
    Reset(CPU);
    ClearSignals(CPU);

    for (int i = 2; i < argc; i++)
    {
        char *filename = argv[i];
        int out = ReadObjectFile(filename, CPU);

        if (out == 1)
        {
            // this means the file was unopenable
            // we terminate this program immediately.
            printf("Could not open %s", filename);
            return 1;
        }
    }

    FILE *output_file = fopen(output_filename, "w");

    // loop through every user and os code address and print out the instruction if it's not 0.
    for (int memoryAddress = 0; memoryAddress <= 0x1FFF; memoryAddress++)
    {
        int instruction = CPU->memory[memoryAddress];
        if (instruction != 0)
        {
            fprintf(output_file, "address: %05d contents: 0x%04X\n", memoryAddress, instruction);
        }
    }
    for (int memoryAddress = 0x8000; memoryAddress < 0x9FFF; memoryAddress++)
    {
        int instruction = CPU->memory[memoryAddress];
        if (instruction != 0)
        {
            fprintf(output_file, "address: %05d contents: 0x%04X\n", memoryAddress, instruction);
        }
    }

    fclose(output_file);
    free(CPU);

    return 0;
}
