#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */
#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR will not implemented for this project */
#define HALT 6
#define NOOP 7
#define NOOPINSTRUCTION 0x1c00000
typedef struct IFIDStruct {
	int instr;
	int pcPlus1;
} IFIDType;
typedef struct IDEXStruct {
	int instr;
	int pcPlus1;
	int readRegA;
	int readRegB;
	int offset;
} IDEXType;
typedef struct EXMEMStruct {
	int instr;
	int branchTarget;
	int aluResult;
	int readRegB;
} EXMEMType;
typedef struct MEMWBStruct {
	int instr;
	int writeData;
} MEMWBType;
typedef struct WBENDStruct {
	int instr;
	int writeData;
} WBENDType;
typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; /* number of cycles run so far */
} stateType;

int getOffset(int n);
int isDataHazard(int instr1, int instr2);
void printState(stateType *statePtr);
int field0(int instruction);
int field1(int instruction);
int field2(int instruction);
int opcode(int instruction);
void printInstruction(int instr);

FILE* fp;

int main(int argc, char **argv)
{
	if (argc != 2) 
	{
		printf("error in argument\n");
		exit(1);
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) 
	{
		printf("file open error\n");
		exit(1);
	}

	stateType state;

	state.pc = 0;
	for (int i = 0; i < NUMREGS; i++) 
		state.reg[i] = 0;
	state.numMemory = 0;
	state.cycles = 0;

	state.IFID.instr = NOOPINSTRUCTION;
	state.IFID.pcPlus1 = 0;
	state.IDEX.instr = NOOPINSTRUCTION;
	state.IDEX.pcPlus1 = 0;
	state.IDEX.readRegA = 0;
	state.IDEX.readRegB = 0;
	state.IDEX.offset = 0;
	state.EXMEM.instr = NOOPINSTRUCTION;
	state.EXMEM.branchTarget = 0;
	state.EXMEM.aluResult = 0;
	state.EXMEM.readRegB = 0;
	state.MEMWB.instr = NOOPINSTRUCTION;
	state.MEMWB.writeData = 0;
	state.WBEND.instr = NOOPINSTRUCTION;
	state.WBEND.writeData = 0;

	char ch[1001];
	while(1)
	{
		if (fgets(ch, 1000, fp) == NULL)
			break;
		if (sscanf(ch, "%d", &state.instrMem[state.numMemory]) != 1) 
		{
			printf("error read memory\n");
			exit(1);
		}
		printf("memory[%d]=%d\n", state.numMemory, state.instrMem[state.numMemory]);
		state.dataMem[state.numMemory] = state.instrMem[state.numMemory];
		state.numMemory++;
	}

	printf("%d memory words\n", state.numMemory);
	printf("\tinstruction memory:\n");
	for (int i = 0; i < state.numMemory; i++) 
	{
		printf("\t\tinstrMem[ %d ] ", i);
		printInstruction(state.instrMem[i]);
	}

	stateType newState;

	newState.pc = 0;
	for (int i = 0; i < NUMREGS; i++)
		newState.reg[i] = 0;

	newState.cycles = 0;

	newState.IFID.instr = NOOPINSTRUCTION;
	newState.IDEX.instr = NOOPINSTRUCTION;
	newState.EXMEM.instr = NOOPINSTRUCTION;
	newState.MEMWB.instr = NOOPINSTRUCTION;
	newState.WBEND.instr = NOOPINSTRUCTION;

	int op;

	while (1)
	{
		printState(&state);

		/* check for halt */
		if (opcode(state.MEMWB.instr) == HALT) {
			printf("machine halted\n");
			printf("total of %d cycles executed\n", state.cycles);
			exit(0);
		}

		newState = state;
		newState.cycles++;

		/* --------------------- IF stage --------------------- */
		if (!isDataHazard(state.IFID.instr, state.IDEX.instr))
		{
			newState.IFID.instr = state.instrMem[state.pc];
			newState.pc = state.pc + 1;
			newState.IFID.pcPlus1 = newState.pc;
		}
		

		/* --------------------- ID stage --------------------- */


		if (isDataHazard(state.IFID.instr, state.IDEX.instr)) 
		{
			newState.IDEX.instr = NOOPINSTRUCTION;
			newState.IDEX.pcPlus1 = 0;
			newState.IDEX.readRegA = 0;
			newState.IDEX.readRegB = 0;
			newState.IDEX.offset = 0;
		}
		else 
		{
			newState.IDEX.instr = state.IFID.instr;
			newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
			newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
			newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
			newState.IDEX.offset = getOffset(field2(state.IFID.instr));
		}

		/* --------------------- EX stage --------------------- */
		
		newState.EXMEM.instr = state.IDEX.instr;
		newState.EXMEM.branchTarget = state.IDEX.pcPlus1 + state.IDEX.offset;
		newState.EXMEM.readRegB = state.IDEX.readRegB;

		op = opcode(newState.EXMEM.instr);
		if (op == ADD)
			newState.EXMEM.aluResult = state.IDEX.readRegA + state.IDEX.readRegB;
		else if (op == NOR)
			newState.EXMEM.aluResult = ~(state.IDEX.readRegA | state.IDEX.readRegB);
		else if (op == LW || op == SW)
			newState.EXMEM.aluResult = state.IDEX.readRegA + state.IDEX.offset;
		else if (op == BEQ)
			newState.EXMEM.aluResult = state.IDEX.readRegA - state.IDEX.readRegB;

		//forwarding
		if (op == ADD || op == NOR) 
		{
			if (field0(newState.IDEX.instr) == field2(newState.EXMEM.instr))
				newState.IDEX.readRegA = newState.EXMEM.aluResult;
			if (field1(newState.IDEX.instr) == field2(newState.EXMEM.instr))
				newState.IDEX.readRegB = newState.EXMEM.aluResult;
		}


		/* --------------------- MEM stage --------------------- */
		newState.MEMWB.instr = state.EXMEM.instr;
		op = opcode(newState.MEMWB.instr);
		if (op == ADD || op == NOR)
			newState.MEMWB.writeData = state.EXMEM.aluResult;
		else if (op == LW)
		{
			newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
			//forwarding
			int op1 = opcode(newState.IDEX.instr);
			if(op1 != NOOP || op1 != HALT)
			{
				if (field0(newState.IDEX.instr) == field1(newState.MEMWB.instr))
					newState.IDEX.readRegA = newState.MEMWB.writeData;
				if (field1(newState.IDEX.instr) == field1(newState.MEMWB.instr))
					newState.IDEX.readRegB = newState.MEMWB.writeData;
			}
			int op2 = opcode(newState.EXMEM.instr);
			if (op2 == SW)
			{
				if (field1(newState.EXMEM.instr) == field1(newState.MEMWB.instr))
					newState.EXMEM.readRegB = newState.MEMWB.writeData;
			}
		}
		else if (op == SW)
		{
			newState.MEMWB.writeData = state.EXMEM.readRegB;
			newState.dataMem[state.EXMEM.aluResult] = newState.MEMWB.writeData;
		}
		else if (op == BEQ)
		{
			if (state.EXMEM.aluResult == 0) 
			{
				newState.pc = state.EXMEM.branchTarget;
				newState.IFID.instr = NOOPINSTRUCTION;
				newState.IDEX.instr = NOOPINSTRUCTION;
				newState.EXMEM.instr = NOOPINSTRUCTION;
			}
		}

		/* --------------------- WB stage --------------------- */
		newState.WBEND.instr = state.MEMWB.instr;
		op = opcode(newState.WBEND.instr);
		if (op == ADD || op == NOR)
		{
			newState.reg[field2(newState.WBEND.instr)] = state.MEMWB.writeData;
			newState.WBEND.writeData = state.MEMWB.writeData;
		}
		else if (op == LW)
		{
			newState.reg[field1(newState.WBEND.instr)] = state.MEMWB.writeData;
			newState.WBEND.writeData = state.MEMWB.writeData;
		}

		// forwarding
		if (op == LW)
		{
			int op1 = opcode(newState.IDEX.instr);
			if (op1 != NOOP || op1 != HALT)
			{
				if (field0(newState.IDEX.instr) == field1(newState.WBEND.instr))
					newState.IDEX.readRegA = newState.WBEND.writeData;
				if (field1(newState.IDEX.instr) == field1(newState.WBEND.instr))
					newState.IDEX.readRegB = newState.WBEND.writeData;
			}
		}

		state = newState;
	}
}

int getOffset(int n) 
{
	if (n & (1 << 15)) {
		n -= (1 << 16);
	}
	return n;
}

int isDataHazard(int instr1, int instr2)
{
	int op1 = opcode(instr1);
	int op2 = opcode(instr2);
	if (op2 == LW && (op1 == ADD || op1 == BEQ || op1 == NOR) && (field0(instr1) == field1(instr2) || field1(instr1) == field1(instr2)))
		return 1;
	if (op2 == LW && (op1 == LW || op1 == SW) && (field0(instr1) == field1(instr2)))
		return 1;
	return 0;
}

void
printState(stateType *statePtr)
{
	int i;
	printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
	printf("\tpc %d\n", statePtr->pc);
	printf("\tdata memory:\n");
	for (i = 0; i < statePtr->numMemory; i++) {
		printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
	}
	printf("\tregisters:\n");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
	printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IFID.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
	printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IDEX.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
	printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
	printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
	printf("\t\toffset %d\n", statePtr->IDEX.offset);
	printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->EXMEM.instr);
	printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
	printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
	printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
	printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB.instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
	printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->WBEND.instr);
	printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}
int
field0(int instruction)
{
	return((instruction >> 19) & 0x7);
}
int
field1(int instruction)
{
	return((instruction >> 16) & 0x7);
}
int
field2(int instruction)
{
	return(instruction & 0xFFFF);
}
int
opcode(int instruction)
{
	return(instruction >> 22);
}
void
printInstruction(int instr)
{
	char opcodeString[10];
	if (opcode(instr) == ADD) {
		strcpy(opcodeString, "add");
	}
	else if (opcode(instr) == NOR) {
		strcpy(opcodeString, "nor");
	}
	else if (opcode(instr) == LW) {
		strcpy(opcodeString, "lw");
	}
	else if (opcode(instr) == SW) {
		strcpy(opcodeString, "sw");
	}
	else if (opcode(instr) == BEQ) {
		strcpy(opcodeString, "beq");
	}
	else if (opcode(instr) == JALR) {
		strcpy(opcodeString, "jalr");
	}
	else if (opcode(instr) == HALT) {
		strcpy(opcodeString, "halt");
	}
	else if (opcode(instr) == NOOP) {
		strcpy(opcodeString, "noop");
	}
	else {
		strcpy(opcodeString, "data");
	}
	printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
		field2(instr));
}

