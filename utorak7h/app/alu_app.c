//ALU APLIKACIJA

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	FILE* fp;
	char input_string[8];
	char cmd_exit[8] = {'e','x','i','t','\0'};

	int operand1, operand2 = 0;
	char operation = '0';
	int i = 0;
	int ret = 0;


	char *str;//za dimanicku alokaciju stringa
	size_t num_of_bytes = 12;

	while(1)
	{

		printf("Unesite izraz ili komandu: ");
		scanf("%s", input_string);

		ret = 0;
		i = 0;
		while(input_string[i] != '\0')
		{
		  if(input_string[i] == cmd_exit[i]){
		    ret = 1;
		  }else{
		    ret = 0; break;
		  }
		  i++;
		}

		if(ret == 1) break;

		sscanf(input_string, "%d%c%d", &operand1, &operation, &operand2);

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
		case '+': fputs("regA+regB\n", fp); ret=1; break;
		case '-': fputs("regA-regB\n", fp); ret=1; break;
		case '*': fputs("regA*regB\n", fp); ret=1; break;
		case '/': fputs("regA/regB\n", fp); ret=1; break;
		default: puts("Operation not set."); ret=0; break;
		}

		if(fclose(fp)){
			printf("Problem pri zatvaranju /dev/alu\n");
			return -1;
		}


		//TREBA DODATI CITANJE REZULTATA
		if(ret==1){

		  fp=fopen("/dev/alu", "r");
		  if(fp==NULL){
			printf("Problem pri otvaranju /dev/alu\n");
			return -1;
		  }

		  str = (char *)malloc(num_of_bytes+1);
		  getline(&str, &num_of_bytes, fp);

		  if(fclose(fp)){
			printf("Problem pri zatvaranju /dev/alu\n");
			return -1;
		  }
		}

		printf("Procitani reuzltat (string) je: %s\n", str);
		free (str);


	}
	return 0;
}

