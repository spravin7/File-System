#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>


int isTooLongInput(char *cli_input);		// checks wheather cli input exceeds input buffer

void removeWhiteSpaces(char **cli_input);	// remove white spaces from cli input

int parse_cliInput(  char *cli_input,   char *command,   char **arguments, int *noOfOperators);

char  *getNextWord(char **cli_input);						// travel cli input string

int parse_h(char *cli_input);							// parse h(help) command
int parse_vdcpto_vdcpfrom(  char *cli_input,   char **arguments);		// parse commands vdcpto and vdcpfrom
int parse_vdrm(  char *cli_input,   char **arguments, int *noOfOperators);	// parse command vdrm
int parse_vdls(  char *cli_input);						// parse command vdls

void help();									// prints list of commands



void help() // list of commands
{

printf("\n");

printf("h  \n");
printf("vdls  \n");
printf("vdcpto [AD FILEPATH] [NEW FILE NAME AFTER COPIED ON VD] \n");
printf("vdcpfrom [AD FILEPATH] [VD FILE NAME] \n");
printf("vdrm [FILE] ......  \n");
//printf("vdrn [VD FILE NAME] [NEW FILE NAME]  \n");

printf("\n");
}

int isTooLongInput(char *cli_input)
{
return(cli_input[strlen(cli_input)-1] !='\n' );
} 


void removeWhiteSpaces(char **cli_input)
{
	int ptr1=0;
	int ptr2=0;

	size_t cli_inputLen = strlen(*cli_input);


 for( ptr2=0;  ptr2<cli_inputLen && (((*cli_input)[ptr2] == ' ') || ((*cli_input)[ptr2] == '\t'));  ptr2++)
{
// this loop removes leading white spaces
}

(*cli_input)=(*cli_input)+ptr2;


cli_inputLen =strlen(*cli_input);
ptr2 = cli_inputLen-1;

while(((*cli_input)[ptr2] == ' ') || ((*cli_input)[ptr2] == '\t') || ((*cli_input)[ptr2] == '\n'))
{
// this loop removes trailing white spaces
ptr2--;
}

(*cli_input)[ptr2+1] = '\0';


cli_inputLen =strlen(*cli_input);

ptr1=0;
ptr2=0;


	while(ptr2 < cli_inputLen)
	{
 		while(((*cli_input)[ptr2] != ' ') && ((*cli_input)[ptr2] != '\t') && ((*cli_input)[ptr2] != '\0') )
		{
 			(*cli_input)[ptr1++] = (*cli_input)[ptr2++];

		}


		if(((*cli_input)[ptr2] == ' ') || ((*cli_input)[ptr2] == '\t') )
		{
 			(*cli_input)[ptr1++] = ' ';
			ptr2++;

 		}

		while(((*cli_input)[ptr2] == ' ') || ((*cli_input)[ptr2] == '\t') )
		{
 			ptr2++;
		}


		if((*cli_input)[ptr2] == '\0')
			{
				(*cli_input)[ptr1++] ='\0';// (*cli_input)[ptr2++];
			}

	}
return ;
}




char  *getNextWord(char **cli_input)
{
	int cnt1=0;
	int cnt2=0;
	char *word=calloc(300,1);

	while(((*cli_input)[cnt1] == ' ') || ((*cli_input)[cnt1] == '\t')  )
	{
		cnt1++;
	}

	if((*cli_input)[cnt1] == '\n')
	{
		strcpy(word,"\n\0");
		*cli_input+=cnt1+1;
		return word;
	}

	if((*cli_input)[cnt1]=='\0')
		{
			strcpy(word,"\0");
			*cli_input+=cnt1;
			return word;
}

while( ((*cli_input)[cnt1] != ' ') && ((*cli_input)[cnt1] != '\t') &&  ((*cli_input)[cnt1] != '\n') && ((*cli_input)[cnt1] != '\0') )
{

	word[cnt2++] = (*cli_input)[cnt1++];
}
	word[cnt2] = '\0';
	*cli_input+=cnt1;
	return word;
}

/*
1 Invalid h command
*/
int parse_h(char *cli_input)
{

char *cli_input1 = cli_input;
char *word = getNextWord(&cli_input1);

if((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0))
	return 0;

	return 1;
}

int parse_cliInput(  char *cli_input, char *command, char **arguments, int *noOfOperators)
{


char *cli_input1 = cli_input;
char *word = getNextWord(&cli_input1);
int result;
 if((strcmp(word,"h")==0) || (strcmp(word,"help")==0))
{
strcpy(command, "h");

if(parse_h(cli_input1))
	return 1;

	return 0;
}

else if(strcmp(word,"vdls")==0)
{/*
1 invalid vdls command
*/
  	strcpy(command, "vdls");
	if(parse_vdls(cli_input1))
		return 2;

	 return 0;
}

else if((strcmp(word,"vdcpto")==0) || (strcmp(word,"vdcpfrom")==0) )
{
 if(strcmp(word,"vdcpto")==0)
strcpy(command, "vdcpto");
else
	strcpy(command, "vdcpfrom");

 if(result = parse_vdcpto_vdcpfrom(cli_input1, arguments))
{
 	if(result ==1)
	return 3;

else if(result ==2)
	return 4;

else if(result == 3)
	return 5;
}

/*
1 invalid %s command AD-filePath and VD-fileName operator missing
2 Invalid %s command VD-fileName operator missing
*/


	return 0;
}

else if(strcmp(word,"vdrm")==0)
{
strcpy(command, "vdrm");

	if(result  = parse_vdrm(cli_input1, arguments, noOfOperators))
		{
if(result ==1)
	return 6;

else if(result ==2)
	return 7;
}
	/*
1 vdrm missing file oprand
2 operator buffer exceeded
*/

	 return 0;
}
return 8;
}

/*
1 invalid command VD_fileName and VD_newFileName operators missing
2 invalid command too many operators
3
*/

int parse_vdrn(char *cli_input, char **arguments)
{

char *cli_input1 = cli_input;
char *word;

	for(int i=0; i<3 ; i++)
	{
		word = getNextWord(&cli_input1);
 		if((i==0) && ((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0)))
			{
  				return 1;
			}
		if((i==2) ){
			if((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0) )
				return 0;
			else
				return 2;
		}

		strcpy(arguments[i],word);
	}

}
/*
1 invalid %s command AD-filePath and VD-fileName operator missing
2 Invalid %s command VD-fileName operator missing

*/

int parse_vdcpto_vdcpfrom(  char *cli_input,   char **arguments)
{

	char *cli_input1 = cli_input;
	char *word;

	for(int i=0; i<3 ; i++)
	{
 		word = getNextWord(&cli_input1);

		if((i==0) && ((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0)))
		{
			return 1;
		}

		else if((i==1) && ((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0) ))
		{
				return 2;
		}


		else if((i==2) )
		{
			if((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0) )
				return 0;
			else
				return 3;
		}

 		strcpy(arguments[i],word);
 	}

}

/*
1 vdrm missing file oprand
2 operator buffer exceeded
*/

int parse_vdrm(  char *cli_input,   char **arguments, int *noOfOperators )
{
	char *cli_input1 = cli_input;
	char *word = getNextWord(&cli_input1);
	*noOfOperators = 0;
	int i;

	if((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0))
		return 1;

	for( i=0 ;((strcmp(word,"\0") !=0) && (strcmp(word,"\n\0")!=0)) && (i<100); i++ )
	{
		strcpy(arguments[i],word);
		word=getNextWord(&cli_input1);
		(*noOfOperators)++;
	}
		if(((strcmp(word,"\0") !=0) && (strcmp(word,"\n\0")!=0)) && (i ==100) )
			return 2;

	return 0;
}

/*
1 invalid vdls command
*/

int parse_vdls(  char *cli_input)
{

	char *cli_input1 = cli_input;
	char *word = getNextWord(&cli_input1);

	if((strcmp(word,"\0")==0) || (strcmp(word,"\n\0")==0))
		return 0;

	else
		return 1;
}
