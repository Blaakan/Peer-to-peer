#include "peer.h"

int checkcmd(char * cmd, char * message)
{
	int i;
	
	for(i=0; cmd[i] != '\0' && message[i] != '\0'; i++)
	{
		if(cmd[i] != message[i])
			return 0;
	}
	
	return 1;
}

char ** split(char * Str, char delim)
{
	int i=0;
	int nSubStr = 0 ;
	int index2=0;
	int index=0;
	int nblettre=0;
	int TailleString = strlen(Str);
	
	for( i = 0; Str[i] != '\0' ;i++ )
	{
		if (Str[i] == delim && Str[i+1] != '\0')
			nSubStr++;
	}
	
	char** TabSubStr = (char**) calloc(nSubStr+1,sizeof(char*));
	
	for( i = 0 ; Str[i] != '\0' && i < TailleString ; i+=(nblettre+1) )
	{	
		nblettre=0;		
		int j;
		for( j = i ; Str[j] != delim && Str[j] != '\0' ; j++,nblettre++ );
		TabSubStr[index2] = (char *) calloc((nblettre)+1 , sizeof(char));
		for(j = i,index =0 ; Str[j] != delim && Str[j] != '\0';j++,index++ )
		{
			TabSubStr[index2][index] = Str[j];
		}
		TabSubStr[index2][index]='\0';
		
		index2++;
	}
	
	return TabSubStr;
}
