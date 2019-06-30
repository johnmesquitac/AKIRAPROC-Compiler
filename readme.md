To run: 

$ make

$ ./tiny YourCodeHere.txt

Main Structure:
- tiny.l: defines the grammar dictionary
- tiny.y: tokenizer and lemmatize
- globals.h: defines token(node) structure
- util.c: build syntax tree
- analyze.c & symtab.c : creates symbol table
- cgen.c: creates quadruples with three address code
- assembly.c: creates assembly code
- binary.c: creates binary code to AKIRAPROC instruction memory
