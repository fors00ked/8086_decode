#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
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
        const uchar byte = p[i];
        if (((byte >> 2) & 0b00111111) == 0b00100010)
        {
            int extraBytes = 1;
            if (i + 1 >= size)
            {
                printf("Not enough bytes in instruction stream\n");
                return false;
            }

            const uchar byte2 = p[i+1];
            const uchar reg = (byte2 & 0b00111000) >> 3;
            const bool w = (byte & 0b00000001) != 0;
            const bool d = (byte & 0b00000010) != 0;
            const char * reg1 = nullptr;
            // Wide?
            if (!w)
            {
                reg1 = byteRegs[reg];
            }
            else
            {
                reg1 = wordRegs[reg];
            }

            const uchar mod = (byte2 & 0b11000000) >> 6;
            const uchar rm = byte2 & 0b00000111;
            if (rm >= NUM_REGS)
            {
                printf("Register index for R/m is too large\n");
                return false;
            }

            // Register to register
            if (mod == 0b11)
            {
                if (reg >= NUM_REGS)
                {
                    printf("Register index for REG is too large\n");
                    return false;
                }

                const char * reg2 = nullptr;
                // Wide?
                if (!w)
                {
                    reg2 = byteRegs[rm];
                }
                else
                {
                    reg2 = wordRegs[rm];
                }

                const char * r1 = 0, * r2 = 0;

                if (!d)
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
            // Memory mode, no displacement
            else
            {
                const char * addr = nullptr;
                switch (rm) {
                    case 0b000:
                        addr = "bx + si";
                        break;
                    case 0b001:
                        addr = "bx + di";
                        break;
                    case 0b010:
                        addr = "bp + si";
                        break;
                    case 0b011 :
                        addr = "bp + di";
                        break;
                    case 0b100:
                        addr = "si";
                        break;
                    case 0b101:
                        addr = "di";
                        break;
                    case 0b110:
                        addr = "bp";
                        break;
                    case 0b111:
                        addr = "bx";
                        break;
                    default:
                        break;
                }

                // mod == 0b00: no displacement
                ushort disp = 0;
                // 8-bit displacement
                if (mod == 0b01)
                {
                    if (i + 2 >= size)
                    {
                        printf("Not enough bytes in instruction stream\n");
                        return false;
                    }

                    const uchar byte3 = p[i + 2];
                    disp = byte3;
                    extraBytes += 1;
                }
                // 16-bit displacement
                else if (mod == 0b10)
                {
                    if (i + 3 >= size)
                    {
                        printf("Not enough bytes in instruction stream\n");
                        return false;
                    }

                    const uchar byte3 = p[i + 2];
                    const uchar byte4 = p[i + 3];
                    disp = (((ushort)byte4) << 8) | byte3;
                    extraBytes += 2;
                }

                if (!d)
                {
                    // REG=source
                    if (disp == 0)
                    {
                        printf("mov [%s], %s\n", addr, reg1);
                    }
                    else
                    {
                        printf("mov [%s + %d], %s\n", addr, disp, reg1);
                    }
                }
                else
                {
                    // REG=destination
                    if (disp == 0)
                    {
                        printf("mov %s, [%s]\n", reg1, addr);
                    }
                    else
                    {
                        printf("mov %s, [%s + %d]\n", reg1, addr, disp);
                    }
                }
            }

            i += 1 + extraBytes;
        }
        else if (((byte >> 4) & 0b00001111) == 0b00001011)
        {
            const bool w = ((byte >> 3) & 0b00000001) != 0;
            const int extraBytes = w ? 2 : 1;
            if (i + extraBytes >= size)
            {
                printf("Not enough bytes in instruction stream\n");
                return false;
            }

            const uchar byte2 = p[i + 1];
            const uchar reg = byte & 0b00000111;
            if (reg >= NUM_REGS)
            {
                printf("Register index for REG is too large\n");
                return false;
            }

            const char * reg1 = nullptr;
            ushort data = (ushort)byte2;

            // Wide?
            if (!w)
            {
                reg1 = byteRegs[reg];
            }
            else
            {
                const uchar byte3 = p[i + 2];
                data |= (ushort)(byte3) << 8;
                reg1 = wordRegs[reg];
            }

            printf("mov %s,%d\n", reg1, data);

            i += 1 + extraBytes;
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
