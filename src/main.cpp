#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uchar;
const int NUM_REGS = 8;

const char * byteRegs[NUM_REGS] =
{
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh"
};

const char * wordRegs[NUM_REGS] =
{
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di"
};

bool decode (unsigned char * data, int size)
{
    unsigned char * p = data;
    int i = 0;
    while (i < size)
    {
        uchar byte = p[i];
        uchar opCode = byte & 0b10001000;
        if (opCode == 0b10001000)
        {
            if (i + 1 >= size)
            {
                printf("Not enough bytes in instruction stream\n");
                return false;
            }
            
            uchar byte2 = p[i+1];
            uchar mod = (byte2 & 0b11000000) >> 6;
            // Register to register
            if (mod == 0b11)
            {
                unsigned char reg = (byte2 & 0b00111000) >> 3;
                if (reg >= NUM_REGS)
                {
                    printf("Register index for REG is too large\n");
                    return false;
                }

                unsigned char rm = byte2 & 0b00000111;
                if (rm >= NUM_REGS)
                {
                    printf("Register index for R/m is too large\n");
                    return false;
                }
                
                const char * reg1 = 0, * reg2 = 0;
                const int w = (byte & 0b00000001) != 0;
                if (w == 0)
                {
                    reg1 = byteRegs[reg];
                    reg2 = byteRegs[rm];
                }
                else
                {
                    reg1 = wordRegs[reg];
                    reg2 = wordRegs[rm];
                }
                
                const char * r1 = 0, * r2 = 0;
                
                const int d = (byte & 0b00000010) != 0;
                if (d == 0)
                {
                    // REG=source
                    r1 = reg2;
                    r2 = reg1;
                    
                }
                else
                {
                    // REG=destination
                    r1 = reg1;
                    r2 = reg2;
                }
                
                printf("mov %s,%s\n", r1, r2);
            }
            else
            {
                printf("Only register to register mode is supported\n");
                return false;
            }
            
            i += 2;
        }
        else
        {
            printf("Instruction not supported\n");
            return false;
        }
    }
    
    return true;
}

int main(int argc, const char * argv[])
{
    if (argc <= 1)
    {
        printf("No input file\n");
        return 1;
    }
    
    FILE * file = fopen(argv[1], "rb");
    fseek(file, 0, SEEK_END);
    const int size = (int)ftell(file);
    uchar * data = (uchar*)malloc(size);
    fseek(file, 0, SEEK_SET);
    fread(data, 1, size, file);
    fclose(file);
    printf("bits 16\n\n");
    if (!decode(data, size))
    {
        printf("Decoding failed\n");
        return 1;
    }
    free(data);
    return 0;
}
