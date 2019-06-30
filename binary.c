/****************************************************/
/* Lab de Compiladores - Prof. Galvão               */
/* File: binary.c                                   */
/* The code generator implementation                */
/* for the C- compiler                              */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"
#include "assembly.h"

const char * opcodes[] =  { "nop", "halt", "add", "addi", "sub", "mult", "divi", "mod", "and", "or", "not", "xor", "slt", "sgt", "sle", "sge", 
                            "shl", "shr", "move", "ldi", "beq", "bne", "jmp", "in", "out", "str", "load", "jr" };

const char * opcodeBins[] =  {  "010010", "010011", "000000", "000010", "000001", "000100", "000101", "011100","001000", "001001", "000111", "001010", "000110", "011011",
                                "011001", "011010", "010111", "011000", "010110", "001100", "001110", "001111", "010000", "010100", "010101", "001101",
                                "001011","010001" }; 

const char * regBins[] = {  "00000", "00001", "00010", "00011", "00100", "00101", "00110", "00111", "01000", "01001", "01010", "01011", "01100", "01101", "01110",
                            "01111", "10000", "10001", "10010", "10011", "10100", "10101", "10110", "10111", "11000", "11001", "11010", "11011", "11100", "11101",
                            "11110", "11111" };


char * getImediate (int im, int size) {
    int i = 0;
    char * bin = (char *) malloc(size + 2);
    size --;
    for (unsigned bit = 1u << size; bit != 0; bit >>= 1) {
        bin[i++] = (im & bit) ? '1' : '0';
    }
    bin[i] = '\0';
    return bin;
}

char * assembly2binary (Instruction i) {
    char * bin = (char *) malloc((32 + 4 + 2) * sizeof(char));
      if (i.format == format1) {
        sprintf(bin, "%s_%s_%s_%s_%s", opcodeBins[i.opcode], regBins[i.reg2], regBins[i.reg3], regBins[i.reg1], "00000000000");
    }
    else if (i.format == format2) {
        if(i.opcode == move)
             sprintf(bin, "%s_%s_%s_%s_%s", opcodeBins[i.opcode], regBins[i.reg2], "00000", regBins[i.reg1], "00000000000");
        else if(i.opcode == str || i.opcode == load || i.opcode == addi)
            sprintf(bin, "%s_%s_%s_%s", opcodeBins[i.opcode], regBins[i.reg2], regBins[i.reg1], getImediate(i.im, 16));
        else
            sprintf(bin, "%s_%s_%s_%s", opcodeBins[i.opcode], regBins[i.reg1], regBins[i.reg2], getImediate(i.im, 16));
    }
    else if (i.format == format3) {
        if(i.opcode == ldi)
            sprintf(bin, "%s_%s_%s_%s", opcodeBins[i.opcode], "00000",regBins[i.reg1], getImediate(i.im, 16));
        else if(i.opcode == in)
            sprintf(bin, "%s_%s_%s_%s_%s", opcodeBins[i.opcode], "00000", "00000", regBins[i.reg1], "00000000000");
        else
         sprintf(bin, "%s_%s_%s_%s", opcodeBins[i.opcode], regBins[i.reg1], "00000", getImediate(i.im, 16));
    }
    else {
        sprintf(bin, "%s_%s", opcodeBins[i.opcode], getImediate(i.im, 26));
    }
    return bin;
}

void generateBinary (AssemblyCode head, int size) {
    AssemblyCode a = head;
    FILE * c = code;
    char * bin;
    while (a != NULL) {
        if (a->kind == instr) {
           // fprintf(c, "\tassign Memoria[%d]\t=\t32'b", a->lineno);
        printf("instrmem[%d] = 32'b", a->lineno);
            bin = assembly2binary(a->line.instruction);
           // fprintf(c, "%s\n", bin);
            printf("%s;// %s\n", bin, opcodes[a->line.instruction.opcode]);
        }
        else {
            printf("//%s\n", a->line.label);
        }
        a = a->next;
    }

}