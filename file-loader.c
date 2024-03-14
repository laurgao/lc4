#include "file-loader.h"

// current memory array location
unsigned short int memoryAddress;

// convert hex characters of 2 words to an int
// of the format "XX XX " where X is a hex digit. I want to extract the digits and convert to an int
// max 16 bits
unsigned short int two_words_to_int(unsigned char *buffer)
{
    return buffer[0] * 16 * 16 + buffer[1];
}

// Read an object file, load instructions into instruction register
int ReadObjectFile(char *filename, MachineState *CPU)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        // If any of the files do not exist, we exit / return.
        perror("Error opening file");
        return 1;
    }
    unsigned char buffer[2];

    int i = 0;
    unsigned short int start_address;
    int num_instructions;
    while ((fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (i == 0)
        {
            // if the first 2 words are not "CA DE " or "DA DA", we skip the line

            if (!((buffer[0] == 0xCA && buffer[1] == 0xDE) || (buffer[0] == 0xDA && buffer[1] == 0xDA)))
            {
                // If it's a symbol, we skip ahead n words.
                if (buffer[0] == 0xC3 && buffer[1] == 0xB7)
                {
                    // symbol
                    printf("symbol\n");
                    unsigned char buffer2[2];
                    // the next 2 words are the address
                    fread(buffer2, 1, sizeof(buffer2), file);
                    // the next 2 words are the number of bytes to skip
                    fread(buffer2, 1, sizeof(buffer2), file);
                    int num_bytes_to_skip = two_words_to_int(buffer2);
                    printf("skipping %d bytes\n", num_bytes_to_skip);
                    fseek(file, num_bytes_to_skip, SEEK_CUR);
                }
                else if (buffer[0] == 0xF1 && buffer[1] == 0x7E)
                {
                    // number
                    printf("number\n");
                    // the next 2 words are the number of bytes to skip
                    unsigned char buffer2[2];
                    fread(buffer2, 1, sizeof(buffer2), file);
                    int num_bytes_to_skip = two_words_to_int(buffer2);
                    printf("skipping %d bytes\n", num_bytes_to_skip);
                    fseek(file, num_bytes_to_skip, SEEK_CUR);
                }
                else if (buffer[0] == 0x00 && buffer[1] == 0x00)
                {
                    // end of file
                    printf("end of file\n");
                    break;
                }
                else if (buffer[0] == 0x71 && buffer[1] == 0x5E)
                {
                    // number
                    // skip ahead 3 more 2-byte blocks
                    unsigned char buffer2[2];
                    fread(buffer2, 1, sizeof(buffer2), file);
                    fread(buffer2, 1, sizeof(buffer2), file);
                    fread(buffer2, 1, sizeof(buffer2), file);
                }
                else
                {
                    printf("invalid obj header.");
                }
                continue;
            }
        }
        else if (i == 1)
        {
            // here we have start memory address
            start_address = two_words_to_int(buffer);
            printf("start address: %04x\n", start_address);
        }
        else if (i == 2)
        {
            // here we have number of instructions
            num_instructions = two_words_to_int(buffer);
            printf("num instructions: %d\n", num_instructions);
        }
        else
        {
            // so now we have a buffer of size 2 containing 2 words.
            // these two words together form an instruction.
            int adjusted_i = i - 3;
            memoryAddress = start_address + adjusted_i;
            if (adjusted_i < num_instructions)
            {
                unsigned short int instruction = two_words_to_int(buffer);
                // print the instruction
                printf("%d ", i);
                printf("%02x", buffer[0]);
                printf("%02x ", buffer[1]);
                printf("%04x ", instruction);
                printf("%016b ", instruction);
                printf("%04x ", memoryAddress);
                CPU->memory[memoryAddress] = instruction;
                printf("%b", CPU->memory[memoryAddress]);
                printf("\n");

                if (adjusted_i == num_instructions - 1)
                {
                    // we have reached the end of this set of instructions so we reset i
                    i = -1;
                }
            }
            else
            {
                printf("bleh we should not be here.");
            }
        }
        i++;
    }

    fclose(file);
    return 0;
}
