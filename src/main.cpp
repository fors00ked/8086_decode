#include <stdio.h>
#include <stdlib.h>
#include <cassert>

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

enum Instruction
{
    Mov = 0,
    Add,
    Sub,
    Cmp,
    InstructionMax
};

enum InstructionSubtype
{
    MoveRM = 0,     // Move to/from register/memory
    AddRM,          // Add register/memory
    MoveImmRM,      // Move immediate to register/memory
    MoveImmR,       // Move immediate to register
    AddImm,         // Add immediate
    AddAcc,         // Add to accumulator
    InstructionSubtypeMax
};

const char * Semantics[] =
{
    "mov",
    "add",
    "sub",
    "cmp"
};

static_assert(Instruction::InstructionMax == sizeof(Semantics) / sizeof(Semantics[0]));

const char * GetEffectiveAddress(int rm)
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

    return addr;
}

Instruction GetInstruction(uchar byte, InstructionSubtype * type)
{
    *type = InstructionSubtypeMax;

    if (((byte >> 2) & 0b00111111) == 0b00100010)
    {
        *type = MoveRM;
        return Mov;
    }
    else if (((byte >> 2) & 0b00111111) == 0b00000000)
    {
        *type = AddRM;
        return Add;
    }
    else if (((byte >> 1) & 0b01111111) == 0b01100011)
    {
        *type = MoveImmRM;
        return Mov;
    }
    else if (((byte >> 2) & 0b00111111) == 0b00100000)
    {
        *type = AddImm;
        return Add;
    }
    else if (((byte >> 1) & 0b01111111) == 0b00000010)
    {
        *type = AddAcc;
        return Add;
    }
    else if (((byte >> 4) & 0b00001111) == 0b00001011)
    {
        *type = MoveImmR;
        return Mov;
    }

    return InstructionMax;
}

bool decode (unsigned char * data, int size)
{
    unsigned char * p = data;
    int i = 0;
    while (i < size)
    {
        const uchar byte = p[i];

        InstructionSubtype instructionType;
        Instruction instruction = GetInstruction(byte, &instructionType);

        const char * semantics = Semantics[instruction];

        // MOV/ADD to/from register/memory
        if (instructionType == MoveRM || instructionType == AddRM)
        {
            int extraBytes = 1;
            assert(i + extraBytes < size);

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
                const char * addr = GetEffectiveAddress(rm);
                assert(addr != nullptr);

                // mod == 0b00: no displacement
                ushort disp = 0;
                // 8-bit displacement
                if (mod == 0b01)
                {
                    extraBytes += 1;
                    assert(i + extraBytes < size);
                    const uchar byte3 = p[i + extraBytes];
                    disp = byte3;
                }
                // 16-bit displacement
                else if (mod == 0b10)
                {
                    extraBytes += 2;
                    assert(i + extraBytes < size);
                    const uchar byte3 = p[i + extraBytes - 1];
                    const uchar byte4 = p[i + extraBytes];
                    disp = (((ushort)byte4) << 8) | byte3;
                    extraBytes += 2;
                }

                if (!d)
                {
                    // REG=source
                    if (disp == 0)
                    {
                        printf("%s [%s], %s\n", semantics, addr, reg1);
                    }
                    else
                    {
                        printf("%s [%s + %d], %s\n", semantics, addr, disp, reg1);
                    }
                }
                else
                {
                    // REG=destination
                    if (disp == 0)
                    {
                        printf("%s %s, [%s]\n", semantics, reg1, addr);
                    }
                    else
                    {
                        printf("%s %s, [%s + %d]\n", semantics, reg1, addr, disp);
                    }
                }
            }

            i += 1 + extraBytes;
        }
        // MOV/ADD immediate to register/memory
        else if (instructionType == MoveImmRM || instructionType == AddImm)
        {
            int extraBytes = 1;
            assert(i + extraBytes < size);

            const uchar byte2 = p[i + extraBytes];
            const bool w = (byte & 0b00000001) != 0;
            const bool s = (byte & 0b00000010) != 0;

            const uchar mod = (byte2 & 0b11000000) >> 6;
            const uchar rm = byte2 & 0b00000111;
            if (rm >= NUM_REGS)
            {
                printf("Register index for R/m is too large\n");
                return false;
            }

            // Register mode
            if (mod == 0b11)
            {
                extraBytes += 1;
                assert(i + extraBytes < size);
                uchar data = p[i + extraBytes];

                const char * reg = nullptr;

                // Wide?
                if (!w)
                {
                    reg = byteRegs[rm];
                }
                else
                {
                    reg = wordRegs[rm];
                }

                printf("%s %s, %d\n", semantics, reg, data);
            }
            // Memory mode
            else
            {
                const char * addr = GetEffectiveAddress(rm);
                assert(addr != nullptr);

                // mod == 0b00: no displacement
                ushort disp = 0;
                // 8-bit displacement
                if (mod == 0b01)
                {
                    extraBytes += 1;
                    assert(i + extraBytes < size);
                    const uchar byte3 = p[i + extraBytes];
                    disp = byte3;
                }
                // 16-bit displacement
                else if (mod == 0b10)
                {
                    extraBytes += 2;
                    assert(i + extraBytes < size);
                    const uchar byte3 = p[i + extraBytes - 1];
                    const uchar byte4 = p[i + extraBytes];
                    disp = (((ushort)byte4) << 8) | byte3;
                }

                extraBytes += 1;
                assert(i + extraBytes < size);

                uchar data = p[i + extraBytes];

                const char * prefix = nullptr;

                // Wide?
                if (!s && w)
                {
                    extraBytes += 1;
                    assert(i + extraBytes < size);
                    const uchar byte3 = p[i + extraBytes];
                    data |= (ushort)(byte3) << 8;
                    prefix = "word";
                }
                else
                {
                    prefix = "byte";
                }

                if (disp == 0)
                {
                    printf("%s %s [%s], %d\n", semantics, prefix, addr, data);
                }
                else
                {
                    printf("%s %s [%s + %d], %d\n", semantics, prefix, addr, disp, data);
                }
            }

            i += 1 + extraBytes;
        }
        // MOV immediate to register
        else if (instructionType == MoveImmR)
        {
            const bool w = ((byte >> 3) & 0b00000001) != 0;
            int extraBytes = 1;
            assert(i + extraBytes < size);

            const uchar byte2 = p[i + extraBytes];
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
                extraBytes += 1;
                assert(i + extraBytes < size);
                const uchar byte3 = p[i + extraBytes];
                data |= (ushort)(byte3) << 8;
                reg1 = wordRegs[reg];
            }

            printf("mov %s,%d\n", reg1, data);

            i += 1 + extraBytes;
        }
        // Add to accumulator
        else if (instructionType == AddAcc)
        {
            int extraBytes = 1;
            assert(i + extraBytes < size);

            const uchar byte2 = p[i + extraBytes];
            ushort data = (ushort)byte2;

            const char * reg = nullptr;
            const bool w = (byte & 0b00000001) != 0;
            if (w)
            {
                extraBytes += 1;
                assert(i + extraBytes < size);
                const uchar byte3 = p[i + extraBytes];
                data |= (ushort)(byte3) << 8;
                reg = "ax";
            }
            else
                reg = "al";

            printf("%s %s,%d\n", semantics, reg, data);

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
