#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINELENGTH 1000

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);
int findLabelIdx(char *ch);
int getRex(char* ch);
int chkRex(char* ch);


typedef struct instruc {
	char label[1001];
	char opcode[1001];
	char arg0[1001];
	char arg1[1001];
	char arg2[1001];
}instr;

int instr_cnt = 0;
instr ins[65536];

int main(int argc, char *argv[])
{
	char *inFileString, *outFileString;
	FILE *inFilePtr, *outFilePtr;
	char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH], arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
	
	if (argc != 3) {
		printf("error: usage: %s <assembly-code-file> <machine-code-file>\n", argv[0]);
		exit(1);
	}

	inFileString = argv[1];
	outFileString = argv[2];
	inFilePtr = fopen(inFileString, "r");

	if (inFilePtr == NULL) {
		printf("error in opening %s\n", inFileString);
		exit(1);
	}
	outFilePtr = fopen(outFileString, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileString);
		exit(1);
	}

	/* here is an example for how to use readAndParse to read a line from
		inFilePtr */
	while (1)
	{
		if (!readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2))
			break;

		int label_len = strlen(label);

		//label length is longer than 6 or first lable is not letter
		if (label_len != 0)
		{
			if (label_len > 6 || !((97 <= label[0] && label[0] <= 122) || (65 <= label[0] && label[0] <= 90)))
			{
				printf("label error %s !\n", label);
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}

			//label has symbol
			for (int i = 0; i < label_len; i++)
			{
				if (!((97 <= label[i] && label[i] <= 122) || (65 <= label[i] && label[i] <= 90) || (48 <= label[i] && label[i] <= 57)))
				{
					printf("label error %s !\n", label);
					fclose(inFilePtr);
					fclose(outFilePtr);
					exit(1);
				}
			}
			//check duplicate label
			for (int i = 0;i < instr_cnt;i++)
			{
				if (strcmp(ins[i].label, label) == 0)
				{
					printf("label error %s !\n", label);
					fclose(inFilePtr);
					fclose(outFilePtr);
					exit(1);
				}
			}
		}

		strcpy(ins[instr_cnt].label, label);
		strcpy(ins[instr_cnt].opcode, opcode);
		strcpy(ins[instr_cnt].arg0, arg0);
		strcpy(ins[instr_cnt].arg1, arg1);
		strcpy(ins[instr_cnt].arg2, arg2);

		instr_cnt++;

	}
	/* this is how to rewind the file ptr so that you start reading from the
		   beginning of the file */
	rewind(inFilePtr);
	/* after doing a readAndParse, you may want to do the following to test the
		opcode */
	for (int i = 0;i < instr_cnt;i++)
	{
		int ret, op, regA, regB, dest, offset;
		/*R-type instructions (add, nor):
		bits 24-22: opcode
		bits 21-19: reg A
		bits 18-16: reg B
		bits 15-3: unused (should all be 0)
		bits 2-0: destReg*/
		if (!strcmp(ins[i].opcode, "add") || !strcmp(ins[i].opcode, "nor"))
		{
			op = 0;
			if (!strcmp(ins[i].opcode, "nor"))
				op = 1;
			if (!chkRex(ins[i].arg0) || !chkRex(ins[i].arg1) || !chkRex(ins[i].arg2))
			{
				printf("invalid regA, regB, dest\n");
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}
			regA = getRex(ins[i].arg0);
			regB = getRex(ins[i].arg1);
			dest = getRex(ins[i].arg2);
			// if value is over or less than bit, error.
			if (regA < 0 || regA > 7 || regB < 0 || regB > 7 || dest < 0 || dest > 7)
			{
				printf("regA, regB, dest error\n");
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}
			ret = ((op << 22) | (regA << 19) | (regB << 16) | dest);
			fprintf(outFilePtr, "%d\n", ret);
		}
		/*I-type instructions (lw, sw, beq):
		bits 24-22: opcode
		bits 21-19: reg A
		bits 18-16: reg B
		bits 15-0: offsetField (a 16-bit, 2's complement number with a range of -32768 to 32767)*/
		else if (!strcmp(ins[i].opcode, "lw") || !strcmp(ins[i].opcode, "sw") || !strcmp(ins[i].opcode, "beq"))
		{
			op = 2;
			if (!strcmp(ins[i].opcode, "sw"))
				op = 3;
			else if (!strcmp(ins[i].opcode, "beq"))
				op = 4;
			if (!chkRex(ins[i].arg0) || !chkRex(ins[i].arg1) || !chkRex(ins[i].arg2))
			{
				printf("invalid regA, regB, dest\n");
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}
			regA = getRex(ins[i].arg0);
			regB = getRex(ins[i].arg1);
			offset = getRex(ins[i].arg2);
			// if value is over or less than bit, error.
			if (regA < 0 || regA > 7 || regB < 0 || regB > 7 || offset < 0 || offset >= 65536)
			{
				printf("regA, regB, offset error\n");
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}
			if (op == 4 && !isNumber(ins[i].arg2))
				offset = 0x0000FFFF & (offset - (i + 1));
			ret = (op << 22) | (regA << 19) | (regB << 16) | offset;
			fprintf(outFilePtr, "%d\n", ret);

		}
		/*J-type instructions (jalr):
		bits 24-22: opcode
		bits 21-19: reg A
		bits 18-16: reg B
		bits 15-0: unused (should all be 0)*/
		else if (!strcmp(ins[i].opcode, "jalr"))
		{
			op = 5;
			if (!chkRex(ins[i].arg0) || !chkRex(ins[i].arg1))
			{
				printf("invalid regA, regB\n");
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}
			regA = getRex(ins[i].arg0);
			regB = getRex(ins[i].arg1);
			if (regA < 0 || regA > 7 || regB < 0 || regB > 7)
			{
				printf("regA, regB error\n");
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}

			ret = (op << 22) | (regA << 19) | (regB << 16);
			fprintf(outFilePtr, "%d\n", ret);
		}
		/*O-type instructions (halt, noop):
		bits 24-22: opcode
		bits 21-0: unused (should all be 0)*/
		else if (!strcmp(ins[i].opcode, "halt") || !strcmp(ins[i].opcode, "noop"))
		{
			op = 6;
			if (!strcmp(ins[i].opcode, "noop"))
				op = 7;
			ret = (op << 22);
			fprintf(outFilePtr, "%d\n", ret);
		}
		// .fill
		else if (!strcmp(ins[i].opcode, ".fill"))
		{
			if (!chkRex(ins[i].arg0))
			{
				printf("invalid fill\n");
				fclose(inFilePtr);
				fclose(outFilePtr);
				exit(1);
			}
			ret = getRex(ins[i].arg0);
			fprintf(outFilePtr, "%d\n", ret);
		}
		else
		{
			printf("invalid opcode %d %s\n", i, ins[i].opcode);
			fclose(inFilePtr);
			fclose(outFilePtr);
			exit(1);
		}
	}
	return(0);
}
/*Read and parse a line of the assembly - language file.Fields are returned
	* in label, opcode, arg0, arg1, arg2(these strings must have memory already
		* allocated to them).
	* Return values :
*0 if reached end of file
	* 1 if all went well
	* exit(1) if line is too long.
	*/
int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0, char *arg1, char *arg2)
{
	char line[MAXLINELENGTH];
	char *ptr = line;
	/* delete prior values */
	label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';
	/* read the line from the assembly-language file */
	if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
		/* reached end of file */
		return(0);
	}
	/* check for line too long (by looking for a \n) */
	if (strchr(line, '\n') == NULL) {
		/* line too long */
		printf("error: line too long\n");
		exit(1);
	}
	/* is there a label? */
	ptr = line;
	if (sscanf(ptr, "%[^\t\n\r ]", label)) {
		/* successfully read label; advance pointer over the label */
		ptr += strlen(label);
	}
	/*
	 * Parse the rest of the line.  Would be nice to have real regular
	 * expressions, but scanf will suffice.
	 */
	sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]", opcode, arg0, arg1, arg2);
	return(1);
}
int isNumber(char *string)
{    /* return 1 if string is a number */
	int i;
	return((sscanf(string, "%d", &i)) == 1);
}

//find label index
int findLabelIdx(char *ch)
{
	for (int i = 0;i < instr_cnt;i++)
	{
		if (!strcmp(ch, ins[i].label))
			return i;
	}
	return -1;
}

//get Rex number
int getRex(char* ch)
{
	if (isNumber(ch))
		return atoi(ch);
	return findLabelIdx(ch);
}

// check if this value is valid
int chkRex(char* ch)
{
	if (isNumber(ch))
		return 1;
	if (findLabelIdx(ch) == -1)
		return 0;
	return 1;
}
