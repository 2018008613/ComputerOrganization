#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

typedef struct stateStruct {
	int pc;
	int mem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
} stateType;

void printState(stateType *);
int getRegA(int n);
int getRegB(int n);
int getDest(int n);
int getOffset(int n);

int main(int argc, char *argv[])
{
	char line[MAXLINELENGTH];
	stateType state;
	FILE *filePtr;
	if (argc != 2) {
		printf("error: usage: %s <machine-code file>\n", argv[0]);
		exit(1);
	}
	filePtr = fopen(argv[1], "r");
	if (filePtr == NULL) {
		printf("error: can't open file %s", argv[1]);
		perror("fopen");
		exit(1);
	}
	state.pc = 0;
	for (int i = 0; i < NUMREGS; i++) 
		state.reg[i] = 0;

	/* read in the entire machine-code file into memory */
	for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;state.numMemory++) {
		if (sscanf(line, "%d", state.mem + state.numMemory) != 1) {
			printf("error in reading address %d\n", state.numMemory);
			exit(1);
		}
		printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
	}
	int cnt = 1;
	while (1)
	{
		printState(&state);
		int n = state.mem[state.pc];
		int op = n >> 22;

		int regA, regB, dest, offset, branch;
		//add
		if (op == 0)
		{
			regA = getRegA(n);
			regB = getRegB(n);
			dest = getDest(n);
			state.reg[dest] = state.reg[regA] + state.reg[regB];
			state.pc++;
		}
		//nor
		else if (op == 1)
		{
			regA = getRegA(n);
			regB = getRegB(n);
			dest = getDest(n);
			state.reg[dest] = ~(state.reg[regA] | state.reg[regB]);
			state.pc++;
		}
		//lw
		else if (op == 2)
		{
			regA = getRegA(n);
			regB = getRegB(n);
			offset = getOffset(n);
			if (state.reg[regA] + offset < 0 || state.numMemory <= state.reg[regA] + offset)
			{
				printf("error in lw\n");
				fclose(filePtr);
				exit(1);
			}
			state.reg[regB] = state.mem[state.reg[regA] + offset];
			state.pc++;
		}
		//sw
		else if (op == 3)
		{
			regA = getRegA(n);
			regB = getRegB(n);
			offset = getOffset(n);
			if (state.reg[regA] + offset < 0 || state.numMemory <= state.reg[regA] + offset) 
			{
				printf("error in sw\n");
				fclose(filePtr);
				exit(1);
			}
			state.mem[state.reg[regA] + offset] = state.reg[regB];
		}
		//beq
		else if (op == 4)
		{
			regA = getRegA(n);
			regB = getRegB(n);
			offset = getOffset(n);
			branch = state.pc + 1 + offset;
			if (branch < 0 || state.numMemory <= branch)
			{
				printf("error in beq\n");
				fclose(filePtr);
				exit(1);
			}
			if (state.reg[regA] == state.reg[regB])
				state.pc = branch;
			else
				state.pc++;
		}
		//jair
		else if (op == 5)
		{
			regA = getRegA(n);
			regB = getRegB(n);
			state.reg[regB] = state.pc + 1;
			if (state.reg[regA] < 0 || state.numMemory <= state.reg[regA])
			{
				printf("error in jair\n");
				fclose(filePtr);
				exit(1);
			}
			state.pc = state.reg[regA];
		}
		//halt
		else if (op == 6)
		{
			state.pc++;
			printf("machine halted\n");
			printf("total of %d instructions executed\n", cnt);
			printf("final state of machine:\n");
			break;
		}
		//noop
		else if (op == 7)
		{
			state.pc++;
		}
		else
		{
			printf("error in op\n");
			fclose(filePtr);
			exit(1);
		}
		cnt++;
	}
	printState(&state);
	fclose(filePtr);
	return(0);
}
void printState(stateType *statePtr)
{
	int i;
	printf("\n@@@\nstate:\n");
	printf("\tpc %d\n", statePtr->pc);
	printf("\tmemory:\n");
	for (i = 0; i < statePtr->numMemory; i++) {
		printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
	}
	printf("\tregisters:\n");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
	printf("end state\n");
}

int getRegA(int n)
{
	return ((n >> 19) & 7);
}

int getRegB(int n)
{
	return ((n >> 16) & 7);
}

int getDest(int n)
{
	return (n & 7);
}

int getOffset(int n)
{
	int ret = (0x0000FFFF & n);
	if (ret >= 32768)
		return ret - 65536;
	return ret;
}
