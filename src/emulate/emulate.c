//
// Created by Ahmad Jamsari on 29/05/2024.
//

// Loads binary data, then emulates the execution and fetch cycle
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitmanipulation.h"
#include "branch.h"
#include "dataprocessing.h"
#include "datatransfer.h"
#include "memory.h"
#include "../debug.h"

static void readInputFile(const char *inputPath);

// should be able to handle both stdoutput and file
static void outputHandler(FILE *outputFile);

// honestly the rest of these static functions could be their separate file
// but up to you guys
static void execute();  // executes the code using the instructions in memory

// this function should determine the instruction type
// then call the relevant instruction handler (from one of the holder files)
// could use a better name. ie it routes it to smth that knows how to handle it
// this will hopefully be called from execute()
// instructionHandler will use fetch() to fetch instructions.
static void instructionHandler(INST instruction);

// should read the next 4 bytes starting at PC
// depending on the ordering used by the instruction handlers,
// this function may need to be changed
// could use bit manipulation to place the 4 bytes in the right order

int main(const int argc, char **argv) {
    // Binary Loader
    // Reads 4 bytes as an INST
    // Identifies INST as 1 of the 5 possible instructions
    // Passes them to the relevant modules that handles the instruction
    // Eg. Data processing, Data Transfer, Branch etc
    // Each instruction handler should return a bool indicating success
    // Errors are handled here.

    // Binary Loader start, handle command line arguments
    FILE *outputFile = stdout;  // default outputFile to stdout

    PRINT("Emulator start.\n");

    if (argc == 3) {
        // have 2 arguments, still need to check the files are valid
        // check outputFile
        if ((outputFile = fopen(argv[2], "w")) == NULL) {
            perror("Error opening output file");
            return EXIT_FAILURE;
        }
    }
    PRINT("Attempting to read file: \n");
    readInputFile(argv[1]);  // opens and closes the file as we only need it at start

    // execute should be a loop
    execute();

    // program finished running so output the file
    outputHandler(outputFile);

    PRINT("Emulating running\n");
    fclose(outputFile);
    return EXIT_SUCCESS;
}

// reads the inputFile and it will attempt to read over memory
// honestly might be better for this to both take a path, open the file
// and also close the file
// returns void cus I'll probably just do exit();
static void readInputFile(const char *inputPath) {
    FILE *inputFile;
    if ((inputFile = fopen(inputPath, "rb")) == NULL) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    fseek(inputFile, 0L, SEEK_END);                            // Sets file pointer to the end
    size_t fileSize = ftell(inputFile);                        // basically returns the size of file in bytes
    rewind(inputFile);                                         // put file pointer to start
    size_t bytesRead = fread(memory, 1, fileSize, inputFile);  // reads file into memory
    assert(bytesRead == fileSize);                             // ensures it read enough bytes
    PRINT("File read succesfully.\nCalling printMemory...\n");
    printMemory(0, 30);
    PRINT("Non-zero memory:\n");

    for (int32_t address = 0; address < (1 << 21) - 4; address += 4) {
        // use instFromAddress as this is exactly what we want
        const INST valAtAddress = getInstAtAddr(address);
        if (valAtAddress != 0) {
            // yes this looks like hell and yes PRIx32 is portable, it lives in inttypes.h
            PRINT("0x%8.8" PRIx32 ": %8.8" PRIx32 "\n", address, valAtAddress);
        }
    }
    fclose(inputFile);
}

// could be nice to make a function that prints out 64 bit vs 32 bit values
// in the same format
static void outputHandler(FILE *outputFile) {
    PRINT("outputHandler is called. TODO() \n");
    fprintf(outputFile, "Registers:\n");

    // now print out registers
    // TO CHANGE: CHANGE 31 TO A MACRO
    for (int i = 0; i < 31; i++) {
        // uses cool string concatenation feature in C, look it up if you're skeptical
        fprintf(outputFile, "X%.2d = %16.16" PRIx64 "\n", i, registers[i]);
    }

    // print out PC now
    fprintf(outputFile, "PC = ");
    fprintf(outputFile, "%16.16" PRIx64 "\n", programCounter);

    // END PC

    // even making pState an integer or array
    // would help a lot
    fprintf(outputFile, "PSTATE : ");

    if (pstate.N) {
        fprintf(outputFile, "N");
    } else {
        fprintf(outputFile, "-");
    }

    if (pstate.Z) {
        fprintf(outputFile, "Z");
    } else {
        fprintf(outputFile, "-");
    }

    if (pstate.C) {
        fprintf(outputFile, "C");
    } else {
        fprintf(outputFile, "-");
    }

    if (pstate.V) {
        fprintf(outputFile, "V");
    } else {
        fprintf(outputFile, "-");
    }

    fprintf(outputFile, "\n");

    // PSTATE DONE

    // memory printing time
    // I can already seeing this code having similarities to execute
    fprintf(outputFile, "Non-zero memory:\n");

    for (int32_t address = 0; address < (1 << 21) - 4; address += 4) {
        // use instFromAddress as this is exactly what we want
        const INST valAtAddress = getInstAtAddr(address);
        if (valAtAddress != 0) {
            // yes this looks like hell and yes PRIx32 is portable, it lives in
            // inttypes.h
            fprintf(outputFile, "0x%8.8" PRIx32 ": %8.8" PRIx32 "\n", address, valAtAddress);
        }
    }
}

static void execute() {
    PRINT("Default execute function called.\n");
    // should be a loop
    // should call instructionHandler()
    INST instruction;
    while ((instruction = fetch()) != HALT) {
        PRINT("Fetching instruction...\n 0x%8.8" PRIx64 ": %8.8" PRIx32 "\n", programCounter, instruction);
        instructionHandler(instruction);
    }
}

#define isSDT(instruction)                                                                 \
    extractBits((instruction), 25, 29) == 28 && extractBits((instruction), 31, 31) == 1 && \
        extractBits((instruction), 23, 23) == 0
#define isLL(instruction) extractBits((instruction), 24, 29) == 24 && extractBits((instruction), 31, 31) == 0
#define isBr(instruction) extractBits((instruction), 26, 29) == 5
#define isReg(instruction) extractBits((instruction), 25, 27) == 5
#define isImm(instruction) extractBits((instruction), 26, 28) == 4

static void instructionHandler(INST instruction) {
    if (isImm(instruction))
        immediateHandler(instruction);
    else if (isReg(instruction))
        registerHandler(instruction);
    else if (isSDT(instruction))
        singleDataTransferHandler(instruction);
    else if (isLL(instruction))
        loadLiteralHandler(instruction);
    else if (isBr(instruction))
        branchHandler(instruction);
    else
        assert(false && "Error: Not a valid instruction type.");

    if (!(isBr(instruction))) programCounter += 4;
}
