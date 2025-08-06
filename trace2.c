/*
 * trace.c: location of main() to start the simulator
 * trace1 is for part 1 of the assignment and trace2 is for part 2.
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
        printf("%s", "here");
        printf("%d", CPU->memory[0x8202]);

        if (out == 1)
        {
            // this means the file was unopenable
            // we terminate this program immediately.
            printf("Could not open %s", filename);
            return 1;
        }
    }

    FILE *output_file = fopen(output_filename, "w");

    while (1)
    {
        int status = UpdateMachineState(CPU, output_file);
        if (status == 1)
        {
            break;
        }
    }

    // int status = UpdateMachineState(CPU, output_file);

    fclose(output_file);
    free(CPU);

    return 0;
}