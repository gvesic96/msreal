//ALU APLIKACIJA

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	FILE* fp;
	char expression[10];

	int operand1, operand2 = 0;
	char operation = '0';

	while(1)
	{

		printf("Unesite izraz u obliku broj+broj: ");
		scanf("%s", expression);

		sscanf(expression, "%d%c%d", &operand1, &operation, &operand2);

		if(operand1){

		   fp = fopen("/dev/alu", "w");
		   if(fp == NULL)
		   {
			printf("Problem pri otvaranju /dev/alu\n");
			return -1;
		   }

		   fprintf(fp, "regA=%d\n", operand1);

		   if(fclose(fp))
		   {
			printf("Problem pri zatvaranju /dev/alu\n");
			return -1;
		   }
		}//if


		if(operand2){
		   fp = fopen("/dev/alu", "w");
		   if(fp == NULL)
		   {
			printf("Problem pri otvaranju /dev/alu\n");
			return -1;
		   }

		   fprintf(fp, "regB=%d\n", operand2);

		   if(fclose(fp))
		   {
			printf("Problem pri zatvaranju /dev/alu\n");
			return -1;
		   }
		}//if(operand2)


		fp=fopen("/dev/alu", "w");
		if(fp==NULL){
			printf("Problem pri otvaraju /dev/alu\n");
			return -1;
		}

		switch(operation)
		{
		case '+': fputs("regA+regB ", fp); break;
		case '-': fputs("regA-regB ", fp); break;
		case '*': fputs("regA*regB ", fp); break;
		case '/': fputs("regA/regB ", fp); break;
		default: puts("Operation not set."); break;
		}

		if(fclose(fp)){
			printf("Problem pri zatvaranju /dev/alu\n");
			return -1;
		}


	}
	return 0;
}

