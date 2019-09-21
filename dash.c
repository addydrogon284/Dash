#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>


char *args[100];   //to store paths
int len = 0;       // to store number of paths

char error_message[30] = "An error has occurred\n";
//write(STDERR_FILENO, error_message, strlen(error_message));

void dashcleanspace(char* lop){
	if(lop[strlen(lop)-1]==' ' || lop[strlen(lop)-1]=='\n')
		lop[strlen(lop)-1]='\0';
	if(lop[0]==' ' || lop[0]=='\n') memmove(lop, lop+1, strlen(lop));
}

void generate_tokens(char *input, const char *delimiter, int * list_size, char **token_list){
	//printf("In generate_tokens:%s\n",input);
	char *token;
	int pos = 0;
	token = strtok(input, delimiter);
	while(token){
		token_list[pos]=malloc(sizeof(token)+1);

		// check if token allotted
		if(!token_list[pos]){
			printf("Cannot allocate memory\n");
		}

		//printf("token: %s\n",token);

		strcpy(token_list[pos], token);
		dashcleanspace(token_list[pos]);
		pos++;
		// printf("%s**\n",token);
		token=strtok(NULL,delimiter);
		// printf("**end od loop**\n");
	}
	
	token_list[pos] = NULL;
	// printf("**out od loop**\n");
	*list_size = pos;
	// printf("**out od  777 loop**\n");
}

void dashpath(char **path)
{
	len = 0;
	int position = 1;
	int i = 0;
	while(path[position] != NULL)
	{
		args[i] = malloc(sizeof(path[position])+1);
		args[i] = path[position];
		i++;
		position++;
	}
	len = i;
 // args[i] = malloc(sizeof(path[position])+1);
	args[i] = NULL;
	i = 0;
	while(args[i] != NULL)
	{
		printf("%s\n", args[i]);
		i++;
	}
    //printf("new len == %d\n",len);
}

void parallel(char** cmd,int no_cmd){

	int i, status, count;
	pid_t  pid= 0;
	char *argv[100];
	for(i = 0;i < no_cmd;i++){
		generate_tokens(cmd[i], " ", &count, argv);
		pid = fork();
		if(pid == 0){
			for (int i = 0;i < len;i++)
			{
				//printf("Checking for : %s %s****\n",args[i],argv[0]);
				if(access(args[i],X_OK)==0)
				{
				//child
					char *source = args[i];
					strcat(source,argv[0]);
					execv(source,argv);
					//in case exec is not successfull, exit
					perror("invalid input\n");
					exit(1);

				}
			}
			//printf("Finished executing token %d\n", i);
		}
		else if(pid  == -1)
		{
			printf("Error\n");
			break;
		}
	}
	// wait for all other processes to complete!
	while ((pid = waitpid(-1,&status,0))!=-1) {
		printf("\nProcess %d terminated\n\n",pid);
	}
}

void execute(char ** cmd)
{
	//printf("Normal commands\n");
	int i,status;
	pid_t  pid= 0;

	pid = fork();
	if(pid == 0)
	{
		for (int i = 0;i < len;i++)
		{
			//printf("Checking for : %s %s****\n",args[i],cmd[0]);
			if(access(args[i],X_OK)==0)
			{
			//child
				char *source = args[i];
				strcat(source,cmd[0]);
				execv(source,cmd);
				//in case execv is not successfull, exit
				perror("invalid input\n");
				exit(1);

			}
		}
	}
	else if(pid  == -1)
	{
		printf("Could not fork!! - execute\n");
	}
	else{
		wait(NULL);
	}
}

void redirectopexecute(char** lop,int no_cmd){
	int no_tok,fp;
	char *argv[100];
	dashcleanspace(lop[1]);
	generate_tokens(lop[0]," ",&no_tok,argv);
	if(fork() == 0){

		fp = open(lop[1],O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); 

		if(fp < 0){
			perror("cannot open file\n");
			return;
		}
		//redirecting stdout to file
		dup2(fp,1);   
		fflush(stdout);
		//redirecting stderr to file 
		dup2(fp,2);

		for (int i = 0;i < len;i++)
		{
			//printf("Checking for : %s %s****\n",args[i],argv[0]);
			if(access(args[i],X_OK) == 0)
			{
			//child
				char *source = args[i];
				strcat(source,argv[0]);
				execv(source,argv);
				//fprintf(stderr, "%s\n");
				//in case execv is not successfull, exit
				write(fp, error_message, strlen(error_message));
				perror("invalid input\n");
				exit(1);

			}
		}
		//printf("%s\n",);
		//fclose(fp);
	}
	wait(NULL);
}

int main(int argc, char** argv){
	char *line = NULL;
    //char blank[] = "\n";

	char * token_list[100];
	int list_size = 0;
	//printf("%d\n",argc);
    //printf("%s\n",argv[0]);

	char string[] = "/bin/";
	args[0] = malloc(sizeof(char *)+1);
	strcpy(args[0],string);
	len = 1;


    // First argument is executable file name and then the remaining args
	if (argc > 2) 
	{
		printf("%d\n",argc);
		fprintf(stderr, "Usage: %s <batch_file>\n", argv[0]);
		printf("Too many arguments supplied.\n");
		exit(EXIT_FAILURE);
	}

    if (argc == 2)              //batch mode
    {
    	FILE *fp = fopen(argv[1], "r");
    	while(fgets(line, 499, fp) != NULL)
    	{

    		if(strchr(line,'&')){
    			generate_tokens(line, " & ", &list_size, token_list);
			//printf("Found & in the input: %s*\n",line);
    			parallel(token_list, list_size);
    		}
		else if(strchr(line,'>')){//redirect output to file
			generate_tokens(line, ">", &list_size, token_list);
			if(list_size==2) redirectopexecute(token_list,list_size);
			else printf("Incorrect output redirection!");
		}
		else{
			generate_tokens(line, " ", &list_size, token_list);
			//printf("Tokens successfully generated.. %d\n", list_size);

			if(strstr(token_list[0],"cd")){//cd builtin command
				if (token_list[1] == NULL) {
					fprintf(stderr, "dash: expected argument to \"cd\"\n");
				} else {
					if (chdir(token_list[1]) != 0) {
						perror("dash");
					}
				}
			}
			else if(strstr(token_list[0],"exit")){//exit builtin command
				exit(0);
			}
			else if(strstr(token_list[0],"path")){
				dashpath(token_list);
			}
			//else if(strcmp(token_list[0],blank) == 0)
			//	continue;
			// else executeBasic(params1);
			else execute(token_list);
			
		}
	}
}

    if (argc == 1)                //interactive mode
    {
    	while(1){
    		printf("dash> ");

    	//Take user input using getline()

        size_t bufsize = 0; // have getline allocate a buffer for us
        getline(&line, &bufsize, stdin);


        // printf("string is: %s#\n", line);

        //Check if parallel cmd
        if(strchr(line,'&')){
        	generate_tokens(line, "&", &list_size, token_list);
			//printf("Found & in the input: %s*\n",line);
        	parallel(token_list, list_size);
        }
		else if(strchr(line,'>')){//redirect output to file
			generate_tokens(line, ">", &list_size, token_list);
			if(list_size==2) redirectopexecute(token_list,list_size);
			else printf("Incorrect output redirection!");
		}
		else{
			generate_tokens(line, " ", &list_size, token_list);
			//printf("Tokens successfully generated.. %d\n", list_size);

			if(strstr(token_list[0],"cd")){//cd builtin command
				if (token_list[1] == NULL) {
					fprintf(stderr, "dash: expected argument to \"cd\"\n");
				} else {
					if (chdir(token_list[1]) != 0) {
						perror("dash");
					}
				}
			}
			else if(strstr(token_list[0],"exit")){//exit builtin command
				exit(0);
			}
			else if(strstr(token_list[0],"path")){
				dashpath(token_list);
			}
			//else if(strcmp(token_list[0],blank) == 0)
			//	continue;
			// else executeBasic(params1);
			else execute(token_list);
			
		}
	}
}
return 0;
}