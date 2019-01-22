#include <stdio.h>
#include <stdlib.h> // getenv
#include <unistd.h> //getcwd, gethostname
#include <string.h> //strcpy, strcmp
#include <dirent.h> //opendir
#include <errno.h>  //opendir

#define PATH_TO_HISTORY_FILE "/tmp/microShellHistory"
void clear()
{
    printf("\e[1;1H\e[2J");
}

void init()
{
    clear();
}

// https://stackoverflow.com/a/779960
char *str_replace(char *orig, char *rep, char *with)
{
    char *result;  // the return string
    char *ins;     // the next insert point
    char *tmp;     // varies
    int len_rep;   // length of rep (the string to remove)
    int len_with;  // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;     // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count)
    {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;
    while (count--)
    {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}

char *getUserName()
{
    return getenv("USER");
}

char *getHomeDir()
{
    return getenv("HOME");
}

const char *getHostName()
{
    static char hostname[255];
    gethostname(hostname, sizeof(hostname));
    return hostname;
}
const char *getFullDir()
{
    static char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    return cwd;
}
const char *getDir()
{
    static char *changedString;
    changedString = str_replace((char *)getFullDir(), getHomeDir(), "~");
    return changedString;
}
char *color(const char *textString, char *colorName)
{
    static char colorText[255];
    char *colorString;
    if (colorName == "red")
    {
        colorString = "\x1B[31m";
    }
    else if (colorName == "green")
    {
        colorString = "\x1B[32m";
    }
    else if (colorName == "blue")
    {
        colorString = "\x1B[34m";
    }
    else if (colorName == "orange")
    {
        colorString = "\x1B[33m";
    }
    snprintf(colorText, sizeof(colorText), "%s%s\033[0m", colorString, textString);
    return colorText;
}
void printMainLine()
{
    printf("%s", color(getUserName(), "blue"));
    printf("%s", color("@", "blue"));
    printf("%s", color(getHostName(), "blue"));
    printf("%s", color(":", "orange"));
    printf("%s", color(getDir(), "green"));
    printf(" %s ", color("$", "red"));
}

const char *takeInput()
{
    static char str[40];
    if (fgets(str, sizeof str, stdin)) //Używanie gets zostało usunięte w standardzie C11
    {
        str[strcspn(str, "\n")] = '\0';
    }
    return str;
}

void unrecognizedCommand(const char *input)
{
    printf("Command %s not found or exited with error status\n", color(input, "red"));
}

void problemWithCommand(const char *command)
{
    printf("Command %s has a problem.\n", color(command, "red"));
}
int executeCommandFromPath(const char *command)
{
    FILE *fp;
    char path[1035];

    fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("Failed to run command\n");
        exit(1);
    }

    while (fgets(path, sizeof(path) - 1, fp) != NULL)
    {
        printf("%s", path);
    }

    if (pclose(fp))
    {
        return 0;
    }
}
void helpCommand()
{
    puts("\n######### MicroColorShell ##########"
         "\n#                                  #"
         "\n#     Author: Kamil Bartczak       #"
         "\n#                                  #"
         "\n### List of supported functions: ###"
         "\n#                                  #"
         "\n#>> help - show this window        #"
         "\n#>> cd - Go to path (~ works)      #"
         "\n#>> history - read history of      #"
         "\n#   commands                       #"
         "\n#>> clear - clear console          #"
         "\n#>> ls - listing files and         #"
         "\n#   directory from path (~ works)  #"
         "\n#>> exit - exit console            #"
         "\n#>> touch - create file (~ works)  #"
         "\n#>> whoami - read login username   #"
         "\n#>> pwd - read current path        #"
         "\n#                                  #"
         "\n###    Additional functions:     ###"
         "\n#                                  #"
         "\n#>> Colors                         #"
         "\n#>> Recognize empty enter          #"
         "\n#>> Recognize bad commands         #"
         "\n#>> Recognize bad paths            #"
         "\n#>> Recognize '~' sign             #"
         "\n#>> History save to file           #"
         "\n####################################\n");
}

// https://stackoverflow.com/a/11198630
static char **parseInput(const char *input)
{
    char **res = NULL;

    char str[1024];
    strcpy(str, input);
    char *p = strtok(str, " ");
    int n_spaces = 0, i;

    /* split string and append tokens to 'res' */
    while (p)
    {
        res = realloc(res, sizeof(char *) * ++n_spaces);

        if (res == NULL)
            exit(-1); /* memory allocation failed */

        res[n_spaces - 1] = p;

        p = strtok(NULL, " ");
    }

    /* realloc one extra element for the last NULL */

    res = realloc(res, sizeof(char *) * (n_spaces + 1));
    res[n_spaces] = 0;
    return res;
}

const char *changeTyldToHomeDir(char *path)
{
    const char *changedDir;
    changedDir = str_replace(path, "~", getHomeDir());
    return changedDir;
}
void cdCommand(char **parsedInput)
{
    char *fullPath;
    char *changedDir;
    fullPath = parsedInput[1];

    if (parsedInput[1] == NULL)
    {
        printf("%s", getenv("HOME"));
        chdir(getenv("HOME"));
    }
    else
    {
        changedDir = (char *)changeTyldToHomeDir(fullPath);
        DIR *searchDir = opendir(changedDir);
        if (searchDir)
        {
            closedir(searchDir);
            chdir(changedDir);
        }
        else if (ENOENT == errno)
        {
            problemWithCommand("cd");
            printf("Path %s not found.\n", color(fullPath, "orange"));
        }
    }
}

void historyCommand()
{
    int c;
    FILE *file;
    file = fopen(PATH_TO_HISTORY_FILE, "r");
    if (file)
    {
        while ((c = getc(file)) != EOF)
            putchar(c);
        fclose(file);
    }
}
void addToHistory(const char *command)
{
    FILE *out = fopen(PATH_TO_HISTORY_FILE, "ab+");
    fprintf(out, "> %s\n", (char *)command);
    fclose(out);
}

void lsCommand(const char *path)
{
    DIR *folder;
    struct dirent *entry;

    if ((folder = opendir(path)) == NULL)
    {
        problemWithCommand("ls");
        printf("Path %s not found.\n", color(path, "orange"));
    }
    else
    {
        while ((entry = readdir(folder)) != NULL)
        {
            printf("%s   ", entry->d_name);
        }
        printf("\n");
        closedir(folder);
    }
}
void rmCommand(const char *path)
{
    if (remove(path) == 0)
        printf("%s\n", color("The file was removed correctly", "green"));
    else
    {
        printf("%s\n", color("The file can not be removed", "red"));
    }
}
void touchCommand(const char *path)
{
    FILE *out = fopen(path, "ab+");
    if (out == NULL)
    {
        printf("%s\n", color("The file can not be created", "red"));
    }
    else
    {
        printf("%s\n", color("The file was created correctly", "green"));
    }
    fclose(out);
}
int recognizeInput(const char *input)
{

    if (strcmp(input, ""))
    {
        addToHistory(input);
        char **parsedInput = parseInput(input);
        if (!strcmp(parsedInput[0], "exit"))
        {
            printf("Goodbye\n");
            return 0;
        }
        else if (!strcmp(parsedInput[0], "help"))
        {
            helpCommand();
        }
        else if (!strcmp(parsedInput[0], "clear"))
        {
            clear();
        }
        else if (!strcmp(parsedInput[0], "cd"))
        {
            cdCommand(parsedInput);
        }
        else if (!strcmp(parsedInput[0], "whoami"))
        {
            printf("%s\n", getUserName());
        }
        else if (!strcmp(parsedInput[0], "history"))
        {
            historyCommand();
        }
        else if (!strcmp(parsedInput[0], "pwd"))
        {
            printf("%s\n", getFullDir());
        }
        else if (!strcmp(parsedInput[0], "ls"))
        {

            if (parsedInput[1] == NULL)
            {
                lsCommand(getFullDir());
            }
            else
            {
                lsCommand(changeTyldToHomeDir(parsedInput[1]));
            }
        }
        else if (!strcmp(parsedInput[0], "touch"))
        {
            if (parsedInput[1] == NULL)
            {
                problemWithCommand("touch");
                printf("Touch must have an %s.\n", color("argument", "orange"));
            }
            else
            {
                touchCommand(changeTyldToHomeDir(parsedInput[1]));
            }
        }
        else if (!strcmp(parsedInput[0], "rm"))
        {
            if (parsedInput[1] == NULL)
            {
                problemWithCommand("rm");
                printf("Rm must have an %s.\n", color("argument", "orange"));
            }
            else
            {
                rmCommand(changeTyldToHomeDir(parsedInput[1]));
            }
        }
        else
        {
            if (!executeCommandFromPath(input))
            {
                unrecognizedCommand(parsedInput[0]);
            }
        }
    }

    return 1;
}

int main()
{
    init();

    while (1)
    {
        printMainLine();
        if (!recognizeInput(takeInput()))
            break;
    }

    return 0;
}