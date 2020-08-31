
#include <stdio.h>  
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <fcntl.h>

//Global Variables:
int totLen = 0 , numOfCommands = 0 , redirectionFlag; 
char *file;
//Forward Declarations:
void printUserFolder(); 
char** getArgv(char* cmd , int argc);
int getArgc (char* cmd);
char* checkForPipe(char* cmd);
int checkIfDone(char** argv, int argc);
void execRegularCommand(char **argv,int argc);
void execPipeCommand(char **argv,int argc,char* rcmd);
void redirectionCheck(char* cmd);
void getFileName(char* chPtr);
void redirection();

//------------------------------------//

int main ()
{
    while(1)
    {      
        printUserFolder();
        char cmd[512];
        fgets(cmd,510,stdin);
        if(strcmp(cmd,"\n")==0)
            continue;
        cmd[strlen(cmd)+1] = '\n';
        cmd[strlen(cmd)+2] = '\0';
        totLen = totLen + strlen(cmd);
        numOfCommands++;
        char *rcmd = checkForPipe(cmd);
        if(rcmd == NULL) //Case piped command entered and the left command is with redirection (Illegal).
            redirectionCheck(cmd);
        int argc = getArgc(cmd);
        char** argv = getArgv(cmd,argc);
        if(checkIfDone(argv,argc)==1)
            break;    
        if(rcmd != NULL)
            execPipeCommand(argv,argc,rcmd);
        else
            execRegularCommand(argv,argc);
    }
    return 0;
}

//the next function compares the 1st argument of argv to "done" and finishing the program. 
int checkIfDone(char** argv, int argc)
{
    if(strcmp(argv[0],"done")==0)
    {
        for(int i=0;i<=argc;i++)
        free(argv[i]);
        free(argv);
        printf("Num of commands: %d\n",numOfCommands);
        printf("Total length of all commands: %d\n",totLen);
        printf("Average length of all commands: %f\n",(double)((double)totLen/(double)numOfCommands));
        printf("See you next time\n");
        return 1;
    }
    return 0;
}

// the next function prints the username and the current folder
void printUserFolder()
{
    struct passwd *user = getpwuid(geteuid());
    char* username = user->pw_name;
    if(username==NULL)
    {
        printf("username error\n");
        exit(EXIT_FAILURE);
    }
    char folderpath[PATH_MAX]; 
    if(getcwd(folderpath, PATH_MAX)==NULL)
    {
        printf("folderpath error\n");
        exit(EXIT_FAILURE);
    }
    printf("%s@%s>",username,folderpath);
}

//the next function converst a long string with spaces into an array of argv
char** getArgv(char* cmd , int argc)
{
    char** argv;
    int count = 0 , wCount = 0 ; 
    argv  = (char**)malloc((argc+1)*sizeof(char*));
    if (argv == NULL)
    {
        perror("malloc failed\n");
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<strlen(cmd);i++)
        {
            if(cmd [i] != ' ')
            {
                count++;
                if(cmd[i+1]== ' '||cmd[i+1]== '\n')
                {
                    i++;
                    argv[wCount] = (char*)malloc((count+1)*sizeof(char));
                    if (argv[wCount] == NULL)
                    {
                        for(int k = 0; k<wCount;k++)
                            free(argv[k]);
                        free(argv);    
                        perror("malloc failed\n");
                        exit(EXIT_FAILURE);
                    }
                    for(int j = 0;j<count;j++)
                            argv[wCount][j] = cmd[i-count+j];
                    argv[wCount][count]= '\0';                           
                    wCount++;
                    count = 0;
                }
            }      
        } 
    argv[argc] = NULL;
    return argv;
}

//the next function gets a string and returns the amount of the words in the string.
int getArgc(char* cmd)
{
    int argC = 0 ;
        for(int i=0;;i++)
        {
            if(cmd[i] == '\0')
            {
                break;
            }    
            if(cmd[i] != ' ')
                if(cmd[i+1] == ' ' || cmd[i+1] == '\n')
                    argC++;
        }  
    return argC;    
}

//the next function checks if user inputs '|' , incase '|' typed it erases the right command from cmd and returns it in a different string. 
char* checkForPipe(char* cmd)
{
    for(int i=0;i<strlen(cmd);i++)
        if(cmd[i]=='|')
        {
            char *rcmd = (char*)malloc((strlen(cmd)-i+1)*sizeof(char));
            int count =0;
            for(int j=i+1;j<strlen(cmd);j++)
            {
                rcmd[count]=cmd[j];
                count++;
            }
            rcmd[count+1]= '\0';
            cmd[i] = '\n';
            cmd[i+1] = '\0';
            return rcmd;           
        }
    return NULL;    
}

//the next function gets the array argv , and execute the command of(argv[0])
void execRegularCommand(char **argv,int argc)
{
    pid_t pid = fork();
    if (pid<0)
    {
        perror("fork failed\n");
        exit(EXIT_FAILURE);
    }            
    if(pid==0)
    {
        if(strcmp(argv[0],"cd")==0)
        {
            for(int i=0;i<=argc;i++)
                free(argv[i]);
            free(argv);
            printf("Command not supported (yet)\n");
            exit(0);
        }
        else 
        {
            redirection();
            execvp(argv[0], argv);
        }            
    }
    if (pid>0)
    {
        wait(NULL);
        for(int i=0;i<=argc;i++)
            free(argv[i]);
        free(argv);
    }            
}

//the next function gets the array argv , execute the command of(argv[0]) then pipes it to the input of the command "rcmd"
void execPipeCommand(char **argv,int argc,char* rcmd)
{
    int pipe_fd[2];
    if(pipe(pipe_fd)==-1)
    {
        perror("cannot open pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if (pid<0)
    {
        perror("fork failed\n");
        exit(EXIT_FAILURE);
    }            
    if(pid==0)
    {  
        close(pipe_fd[0]);             
        if(dup2(pipe_fd[1],STDOUT_FILENO)==-1)
        {
            perror("dup2 failed");
            exit(EXIT_FAILURE); 
        }
        close(pipe_fd[1]); 
        execvp(argv[0], argv);
        exit(1);
    }
    if(pid>0)
    {
        close(pipe_fd[1]); 
        wait(NULL);
        redirectionCheck(rcmd);
        int rargc = getArgc(rcmd);
        char** rargv = getArgv(rcmd,rargc);
        free(rcmd); 
        pid_t rcmdpid = fork();
        if(rcmdpid<0)
        {
            perror("fork failed\n");
            exit(EXIT_FAILURE);
        }
        if(rcmdpid==0)
        {
            close(pipe_fd[1]);
            if(dup2(pipe_fd[0],STDIN_FILENO)==-1)
            {
                perror("dup2 failed");
                exit(EXIT_FAILURE); 
            }
            redirection();                            
            execvp(rargv[0], rargv);
            exit(1);
        }
        if(rcmdpid>0)
        {  
            close(pipe_fd[0]);
            wait(NULL);
            for(int i=0;i<=argc;i++)
                free(argv[i]);
            free(argv);
            for(int i=0;i<=rargc;i++)
                free(rargv[i]);
            free(rargv);  
        }
    }
}

//the next function checks if there is need to do a redirection and changes the flag (GLOBAL VARIABLE) according to the result.
void redirectionCheck(char* cmd)
{   
    file = NULL;
    char* chrPtr = strstr(cmd,">>");
    if(chrPtr != NULL)
    {
        getFileName(chrPtr+2);
        *chrPtr = '\n';
        *(chrPtr+1) = '\0';
        redirectionFlag =1;
    }
    chrPtr = strstr(cmd,"2>");
    if(chrPtr != NULL)
    {
        getFileName(chrPtr+2);
        *chrPtr = '\n';
        *(chrPtr+1) = '\0';
        redirectionFlag =2;
    }
    chrPtr = strchr(cmd,'>');
    if(chrPtr != NULL)
    {
        getFileName(chrPtr+1);
        *chrPtr = '\n';
        *(chrPtr+1) = '\0';
        redirectionFlag =3;
    }
    chrPtr = strchr(cmd,'<');
    if(chrPtr != NULL)
    {
        getFileName(chrPtr+1);
        *chrPtr = '\n';
        *(chrPtr+1) = '\0';
        redirectionFlag =4;
    }
    if(file == NULL)
        redirectionFlag = -1;
}

//the next function redirects the file descriptors according to the flag.
void redirection()
{
    int fd;
    if(redirectionFlag == -1)
        return;
    if(redirectionFlag == 1) //>>
    { 
        fd = open(file, O_WRONLY  | O_APPEND | O_CREAT , S_IWUSR);
        if(dup2(fd,STDOUT_FILENO)==-1)
        {
            perror("dup2 failed");
            exit(EXIT_FAILURE); 
        }
        free(file);
        return;
    }
    if(redirectionFlag == 2) // 2>
    {
        fd = open(file, O_WRONLY | O_TRUNC | O_CREAT , S_IRWXU);
        if(dup2(fd,STDERR_FILENO)==-1)
        {
            perror("dup2 failed");
            exit(EXIT_FAILURE); 
        }
        free(file);
        return;
    }    
    if(redirectionFlag == 3) // >
    {
        fd = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);  
        if(dup2(fd,STDOUT_FILENO)==-1)
        {
            perror("dup2 failed");
            exit(EXIT_FAILURE); 
        }
        free(file);
        return;
    }
    if(redirectionFlag == 4) // <
    {
        fd = open(file, O_RDONLY);
        if(dup2(fd,STDIN_FILENO)==-1)
        {
            perror("dup2 failed");
            exit(EXIT_FAILURE); 
        }
        free(file);
        return;
    }
}

//the next function gets a pointer to the string exactly when the name of the redirected file, and changes the file name to it.
void getFileName(char* chrPtr)
{
    int i=0 , counter =0 , j=0;
    while(*(chrPtr+i) != '\0')
        i++;
    file = (char*)malloc((i)*sizeof(char));    
    while(*(chrPtr+j)== ' ')
    {
        j++;
        counter++;
    } 
    for(;j<i;j++)
    {
        if(*(chrPtr+j) == '\n')
        {
            file[j-counter] = '\0';
            break;
        }
        else
            file[j-counter] = *(chrPtr+j);
    }   
}
