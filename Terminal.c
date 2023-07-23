/**
 * Avihai Mordechay, 318341278
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/file.h>

#define MAX_CHARACTERS 510
#define QUOTES '\"'

char *userInput; // The userInput that the user entered (for getLine function).
ssize_t lenUserInput; // Length of the userInput that the user entered (for getLine function).
size_t getLineArg = 0; // For getLine function.
pid_t processId; // the id of the child process.
pid_t backgroundId; // For the background process, when we press ctrl-z we will save the id of the child.

void sigHandler(int sig); // func 1

int isAllSpaces(const char *str); // func

int countChar(const char *str, char chr); // func 3

int wordsCount(const char *str);// func 4

void removeQuotes(char **command);// func 5

void splitCommandByGreater(char **command, int numberOfWords, char ***left, char ***right); //func 6

char *checkVariables(char *str, char ****variables, int *variablesCount); // func 7

char ***allocateVariables(char ***variables, int variablesCount); // func 8

char *nextCommand(char *str, char *command, int *commandIndex); // func 11

void defineVariable(char *command, char ****variables, int *variablesCount); // func 12

char *replaceWordByDollar(char *newStr, char ***variables, int variablesCount); // func 13

char **allocateCommands(const char *str);  // func 14

void splitStringByCommands(char *str, char **commands);  // func 15

char **splitCommandByWords(const char *command, int numberOfWords);  // func 16

void process(char *command, int *numberOfTotalArguments, int *numberOfTotalCommands);  // func 17

void createChildProcess();  // func 18

void launchProcess(char **argv);  // func 19

void pipesProcess(char **command, int numOfPipes);  // func 20

void backgroundProcess(char **command, int numberOfWords);  // func 21

void redirectsToFile(char **command, int numberOfWords);  // func 22

void regularProcess(char **command); // func 23


int main() {
    signal(SIGTSTP, sigHandler);
    signal(SIGCHLD, sigHandler);

    int numberOfTotalArguments = 0;
    int numberOfTotalCommands = 0;
    int correctCountCommands;
    int numberOfVariable = 0;
    int countEnter = 0;
    char *getCwd;
    char *updateUserInput;
    char **commands;
    char ***variables;

    while (1) {
        // Exit condition.
        if (countEnter == 3)
            break;
        // Get the path of the user.
        getCwd = getcwd(NULL, 0);
        printf("#cmd:<%d>|#args:<%d>@%s>", numberOfTotalCommands, numberOfTotalArguments, getCwd);
        // Free the path of the user.
        free(getCwd);
        lenUserInput = getline(&userInput, &getLineArg, stdin);
        // If the user press enter, we will add to countEnter one (10 means enter).
        if (lenUserInput == 1 && userInput[0] == 10) {
            countEnter++;
            continue;
        } else {
            // Extreme case if there is all space in userInput.
            userInput[lenUserInput - 1] = '\0';
            if (isAllSpaces(userInput)) {
                printf("\n");
                continue;
            }
            // Checking if in userInput there is variable to replace.
            updateUserInput = checkVariables(userInput, &variables, &numberOfVariable);
            correctCountCommands = countChar(updateUserInput, ';');
            if (lenUserInput <= MAX_CHARACTERS && correctCountCommands > 0) {
                countEnter = 0;
                commands = allocateCommands(updateUserInput);
                splitStringByCommands(updateUserInput, commands);
                for (int i = 0; i < correctCountCommands; i++)
                    process(commands[i], &numberOfTotalArguments, &numberOfTotalCommands);
                // Free the array of the commands.
                for (int k = 0; k < correctCountCommands; k++)
                    free(commands[k]);
                free(commands);
            } else if (lenUserInput > MAX_CHARACTERS)
                printf("Error: The userInput length is longer than allowed. (510 characters)\n");
            // When we finish to work with the updateUserInput we will free the pointer.
            free(updateUserInput);
        }
    }
    // Free the variables after we finish with the shell.
    if (numberOfVariable > 0) {
        for (int i = 0; i < numberOfVariable; i++) {
            for (int j = 0; j < 2; j++)
                free(variables[i][j]);
            free(variables[i]);
        }
        free(variables);

    }
    free(userInput);
}

/**
The function handles the captured signals.
 */
void sigHandler(int sig) {
    if (sig == SIGCHLD)
        // No waiting but check the status of the son.
        waitpid(-1, NULL, WNOHANG);
    if (sig == SIGTSTP)
        backgroundId = processId;
}

/**
 The function receives the user input and check if the input made up from spaces.
 */
int isAllSpaces(const char *str) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (!isspace(str[i])) {
            return 0;
        }
    }
    return 1;
}

/**
 The function that counts commands by char but if the char in the quotes it will not count it.
 */
int countChar(const char *str, char chr) {
    int countCommands = 0;
    int i = 0;
    int commandLen = 0;
    int inQuote = 0;

    while (str[i] != '\0') {
        if (str[i] == QUOTES)
            inQuote = !inQuote;

        if (str[i] == chr && commandLen > 0 && !inQuote) {
            countCommands++;
        } else if (str[i] != chr)
            commandLen++;
        i++;
    }
    // In the case the command does not contain chr = ';' (it means that there is one command)
    if (countCommands == 0 && commandLen > 0 && chr == ';')
        countCommands++;
    return countCommands;
}

/**
 The function counts words in the string it receives.
 In addition, the function treats what is inside quotes as one word.
 */
int wordsCount(const char *str) {
    int i = 0;
    int count = 0;
    int lenWord = 0;
    unsigned lenCommand = strlen(str);
    while (i < lenCommand) {
        if (str[i] == QUOTES) {
            i++;
            while (str[i] != QUOTES)
                i++;
            i++;
            if (isspace(str[i]) || str[i] == '\0') {
                lenWord = 0;
                i++;
                count++;
            } else if (str[i] == '|' || str[i] == '>') {
                lenWord = 0;
                count++;
            }
        } else if (isspace(str[i]) && lenWord == 0) {
            i++;
        } else if (((str[i] == '|' || str[i] == '>') && lenWord == 0) || (isspace(str[i]) && lenWord >= 1)) {
            lenWord = 0;
            count++;
            i++;
        } else if ((str[i] == '|' || str[i] == '>') && lenWord > 0) {
            lenWord = 0;
            count += 2;
            i++;
        } else {
            i++;
            lenWord++;
        }
    }
    if (lenWord > 0)
        count++;
    return count;
}

/**
This function removes quotation marks from a string and appends the content inside the quotes to the adjacent words.
The function modifies the string in place.
 */
void removeQuotes(char **command) {
    int i = 0, j = 0;
    while ((*command)[i] != '\0') {
        if ((*command)[i] == '\"') { // found a quotation mark
            i++; // move to the next character
            while ((*command)[i] != '\"' && (*command)[i] != '\0') { // find the closing quotation mark
                (*command)[j] = (*command)[i];
                i++;
                j++;
            }
            if ((*command)[i] == '\"') { // if found closing quotation mark
                i++;
            }
        } else { // not a quotation mark
            (*command)[j] = (*command)[i];
            i++;
            j++;
        }
    }
    (*command)[j] = '\0'; // terminate the string
}


/**
The function receives an array of words and divides it into two arrays.
 The first array will contain all the words up to <, in the second array all the words after < and NULL.
 */
void splitCommandByGreater(char **command, int numberOfWords, char ***left, char ***right) {
    int i, j;
    unsigned k;
    int foundChar = 0;

    // allocate memory for left and right arrays
    *left = (char **) malloc((numberOfWords + 1) * sizeof(char *));
    *right = (char **) malloc((numberOfWords + 1) * sizeof(char *));

    // initialize left and right arrays to NULL
    for (i = 0; i < numberOfWords + 1; i++) {
        (*left)[i] = NULL;
        (*right)[i] = NULL;
    }

    // iterate over command array and insert words into left or right
    for (i = 0, j = 0; i < numberOfWords; i++) {
        if (command[i][0] == '>' && strlen(command[i]) == 1) {
            foundChar = 1;
            // reset j to start inserting into right
            j = 0;
        } else if (strchr(command[i], '>') != NULL) {
            k = strchr(command[i], '>') - command[i];
            if (k > 0) {
                // insert word into left
                (*left)[j] = (char *) malloc((k + 1) * sizeof(char));
                strncpy((*left)[j], command[i], k);
                (*left)[j][k] = '\0';
            }
            // reset j to start inserting into right
            j = 0;
            //skip over the char
            k++;
            if (strlen(command[i] + k) > 0) {
                // insert word into right
                (*right)[j] = (char *) malloc((strlen(command[i] + k) + 1) * sizeof(char));
                strcpy((*right)[j], command[i] + k);
                (*right)[j][strlen(command[i] + k)] = '\0';
                j++;
            }
            foundChar = 1;
        } else {
            if (!foundChar) {
                // insert word into left
                (*left)[j] = (char *) malloc((strlen(command[i]) + 1) * sizeof(char));
                strcpy((*left)[j], command[i]);
                (*left)[j][strlen(command[i])] = '\0';
            } else {
                // insert word into right
                (*right)[j] = (char *) malloc((strlen(command[i]) + 1) * sizeof(char));
                strcpy((*right)[j], command[i]);
                (*right)[j][strlen(command[i])] = '\0';
            }
            j++; // increment j to insert next word into left or right
        }
    }
}

/**
 The function receives the user input, an array of variables, the number of existing variables.
 The function goes through an input and checks the following:
 1) If there is a command that defines a new variable.
    If so, it puts it into a new array of variables that is greater than one.
    If not, the function will issue an error message.
 2) If there is an existing variable in the user's input.
    If so, it will replace the variable with its value.
 Finally an updated string of user input will be returned from the function.
 */
char *checkVariables(char *str, char ****variables, int *variablesCount) {
    int commandIndex = 0;
    int inQuote = 0;
    char *newStr = (char *) malloc(sizeof(char) * MAX_CHARACTERS);
    if (newStr == NULL) {
        printf("Error allocating newVariables memory\n");
        free(newStr);
        return NULL;
    }
    memset(newStr, 0, MAX_CHARACTERS);

    while (str[commandIndex] != '\0') {
        if (str[commandIndex] == QUOTES)
            inQuote = !inQuote;
        if (str[commandIndex] == ';' && !inQuote)
            break;
        commandIndex++;
    }
    char *command = (char *) malloc(sizeof(char) * (commandIndex + 1));
    if (command == NULL) {
        printf("Error allocating newVariables memory\n");
        free(command);
        return NULL;
    }
    memset(command, 0, commandIndex + 1);
    strncpy(command, str, commandIndex);
    command[commandIndex] = '\0';
    while (command != NULL) {
        if (strchr(command, '=') != NULL) {
            char testing[MAX_CHARACTERS];
            int i = 0;
            while (command[i] != '=')
                i++;
            strncpy(testing, command, i);
            testing[i] = '\0';
            if (strcmp(testing,"echo") == 0) {
                strcat(newStr, command);
                strcat(newStr, ";");
            } else
                defineVariable(command, variables, variablesCount);
        } else if (strchr(command, '$') != NULL) {
            strcat(newStr, command);
            // if we will find the command in replaceWordByDollar func, we will replace the $var with the value.
            newStr = replaceWordByDollar(newStr, *variables, *variablesCount);
            strcat(newStr, ";");
        } else {
            strcat(newStr, command);
            strcat(newStr, ";");
        }
        command = nextCommand(str, command, &commandIndex);
    }
    return newStr;
}

/**
 The function opens a new set of variables that is greater than 1.
 The function copies the previous variables to the new array and also frees the old variable array.
 */
char ***allocateVariables(char ***variables, int variablesCount) {
    // Allocate memory for the new char***
    char ***newVariables = (char ***) malloc(variablesCount * sizeof(char **));
    if (newVariables == NULL) {
        printf("Error allocating newVariables memory\n");
        free(newVariables);
        return NULL;
    }
    newVariables[variablesCount - 1] = (char **) malloc(2 * sizeof(char *));
    if (newVariables[variablesCount - 1] == NULL) {
        printf("Error allocating newVariables memory\n");
        free(newVariables[variablesCount - 1]);
        return NULL;
    }
    for (int i = 0; i < 2; i++) {
        newVariables[variablesCount - 1][i] = (char *) malloc(sizeof(char) * (MAX_CHARACTERS + 1));
        if (newVariables[variablesCount - 1][i] == NULL) {
            printf("Error allocating newVariables memory\n");
            free(newVariables[variablesCount - 1][i]);
            return NULL;
        }
    }
    if (newVariables == NULL) {
        // Error: allocation failed
        return NULL;
    }
    // Iterate over each command and copy it to the new char***
    for (int i = 0; i < variablesCount - 1; i++) {
        // Allocate memory for the new command (2 words of 50 characters each)
        char **newVariable = malloc(2 * sizeof(char *));
        if (newVariable == NULL) {
            printf("Error allocating newVariable's memory\n");
            // Error: allocation failed
            free(newVariables);
            return NULL;
        }
        for (int j = 0; j < 2; j++) {
            newVariable[j] = malloc((MAX_CHARACTERS + 1) * sizeof(char));
            if (newVariable[j] == NULL) {
                printf("Error allocating newVariable's memory\n");
                // Error: allocation failed
                for (int k = 0; k < j; k++) {
                    free(newVariable[k]);
                }
                free(newVariable);
                free(newVariables);
                return NULL;
            }
            // Copy the word from the original command to the new command
            strncpy(newVariable[j], variables[i][j], MAX_CHARACTERS);
        }
        // Add the new command to the new char***
        newVariables[i] = newVariable;
    }
    if (variablesCount - 1 != 0) {
        // free the old variables;
        for (int i = 0; i < variablesCount - 1; i++) {
            free(variables[i][0]);
            free(variables[i][1]);
            free(variables[i]);
        }
        free(variables);
    }
    // Return the new char***
    return newVariables;
}

/**
    A function that moves to the next command.
    related to the function checkVariables.
 */
char *nextCommand(char *str, char *command, int *commandIndex) {
    int commandLength = 0;
    int inQuote = 0;
    (*commandIndex)++;
    while (str[(*commandIndex) + commandLength] != '\0') {
        if (str[(*commandIndex) + commandLength] == '\"')
            inQuote = !inQuote;
        if (str[(*commandIndex) + commandLength] == ';' && !inQuote)
            break;
        commandLength++;
    }
    if ((*commandIndex) + commandLength > strlen(str) || commandLength == 0) {
        free(command);
        return NULL;
    }
    free(command);
    char *newCommand = (char *) malloc(sizeof(char) * (commandLength + 1));
    memset(newCommand, 0, commandLength + 1);
    strncpy(newCommand, str + (*commandIndex), commandLength);
    newCommand[commandLength] = '\0';
    (*commandIndex) += commandLength;
    return newCommand;
}

/**
 A function that defines a new variable if it is valid.
 variables dimensions:
 1)address
 2)number of the variables.
 3)varible and value.
 4)the words.
 */
void defineVariable(char *command, char ****variables, int *variablesCount) {
    int commandIndex = 0;
    int commandLength = 0;
    int inQuote = 0;
    char testing[MAX_CHARACTERS];
    char variable[MAX_CHARACTERS];
    while (command[commandIndex] != '=')
        commandIndex++;
    strncpy(testing, command, commandIndex);
    testing[commandIndex] = '\0';
    if (wordsCount(testing) != 1) {
        printf("Error: The variable is more than one word.\n");
        return;
    }
    commandIndex++;

    while (command[commandIndex + commandLength] != '\0') {
        if (command[commandIndex + commandLength] == QUOTES)
            inQuote = !inQuote;
        commandLength++;
    }
    strncpy(testing, command + commandIndex, commandLength);
    testing[commandLength] = '\0';
    if (wordsCount(testing) != 1) {
        printf("Error: The value of the variable is more than one word.\n");
        return;
    }
    strncpy(variable, command, commandIndex - 1);
    variable[commandIndex - 1] = '\0';
    for (int j = 0; j < (*variablesCount); j++)
        if (strcmp(variable,(*variables)[j][0]) == 0) {
            printf("Error: The variable is already exists in command %d.\n", j);
            return;
        }
    (*variablesCount)++;
    (*variables) = allocateVariables((*variables), (*variablesCount));
    strcpy((*variables)[(*variablesCount) - 1][0], variable);
    strncpy(variable, command + commandIndex, commandLength);
    variable[commandLength] = '\0';
    strcpy((*variables)[(*variablesCount) - 1][1], variable);
}

/**
 A function that checks if there is an existing variable in the string it receives,
 if so it replaces it with its value and returns the new string.
 */
char *replaceWordByDollar(char *newStr, char ***variables, int variablesCount) {
    char *result = (char *) malloc(MAX_CHARACTERS);
    memset(result, 0, MAX_CHARACTERS);
    if (result == NULL) {
        printf("Error allocating newVariables memory");
        free(result);
        return NULL;
    }
    char word[MAX_CHARACTERS];
    int indexWord = 0, lenNewWord = 0;
    // Check the length of the first word.
    while (newStr[indexWord] != '\0' && newStr[indexWord] != ' ' && newStr[indexWord] != '$')
        indexWord++;
    strncpy(word, newStr, indexWord);
    word[indexWord] = '\0';
    // we will build a new input.
    while (indexWord <= strlen(newStr)) {
        int found = 0;
        int varLength = 0;
        int varIndex = 0;
        // Check in the word if $ existed.
        if (strchr(word, '$') != NULL) {
            // We will find the variable.
            while (varIndex < strlen(word)) {
                if (word[varIndex] == '$') {
                    varIndex++; // skip on '$'
                    while (varIndex + varLength < strlen(word) && word[varIndex + varLength] != ' ' &&
                           word[varIndex + varLength] != '$' && word[varIndex + varLength] != '\"') {
                        varLength++;
                    }
                    char *varName = (char *) malloc(sizeof(char) * (varLength + 1)); // + 1 for \0
                    strncpy(varName, word + varIndex, varLength);
                    varName[varLength] = '\0';
                    if (strlen(varName) == 0) {
                        strcat(result, "$"); // concatenate variable value to result string
                        found = 1;
                    } else {
                        for (int i = 0; i < variablesCount; i++) {
                            if (strcmp(varName,variables[i][0]) == 0) { // check if variable name matches
                                strcat(result, variables[i][1]); // concatenate variable value to result string
                                found = 1;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        strcat(result, " ");
                    }
                    varIndex += varLength;
                    free(varName);
                } else {
                    char singleChar[2];
                    singleChar[0] = word[varIndex];
                    singleChar[1] = '\0';
                    strcat(result, singleChar);
                    varIndex++;
                }

            }
        } else {
            // If the word not include $ we will add it to the new input as is.
            strcat(result, word);
        }
        // Adding the spaces to the new user input and increase the index.
        while (isspace(newStr[indexWord])) {
            strcat(result, " ");
            indexWord++;
        }
        while (newStr[indexWord + lenNewWord] != '\0' && newStr[indexWord + lenNewWord] != ' ') {
            lenNewWord++;
            if (newStr[indexWord + lenNewWord] == '$')
                break;
        }
        if (lenNewWord == 0)
            break;
        memset(word, 0, MAX_CHARACTERS);
        strncpy(word, newStr + indexWord, lenNewWord);
        word[lenNewWord] = '\0';
        indexWord += lenNewWord;
        lenNewWord = 0;
    }
    free(newStr);
    return result;
}

/**
 The function assigns an array of commands according to the number of commands the user entered.
 */
char **allocateCommands(const char *str) {
    char **commands;
    int size = countChar(str, ';');
    commands = (char **) malloc(sizeof(char *) * size);
    if (commands == NULL) {
        printf("Error allocating commands memory\n");
        free(commands);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        commands[i] = (char *) malloc(sizeof(char) * (MAX_CHARACTERS + 1));
        if (commands[i] == NULL) {
            printf("Error allocating commands memory\n");
            for (int k = 0; k < i; k++) {
                free(commands[k]);
            }
            return NULL;
        }
    }
    return commands;
}

/**
 The function inserts the commands entered by the user into the command array.
 */
void splitStringByCommands(char *str, char **commands) {
    int i = 0;
    int n = 0;
    int lenCommand = 0;
    int indexCommand = 0;
    int inQuote = 0;
    while (str[i] != '\0') {
        if (str[i] == '\"')
            inQuote = !inQuote;
        if (str[i] == ';' && !inQuote) {
            if (n == 0) {
                strncpy(commands[indexCommand], str + n, lenCommand);
                commands[indexCommand][lenCommand] = '\0';
            } else {
                n++;
                strncpy(commands[indexCommand], str + n, lenCommand);
                commands[indexCommand][lenCommand] = '\0';
            }
            indexCommand++;
            n += lenCommand;
            lenCommand = 0;
        } else
            lenCommand++;
        i++;

    }
}

/**
 The function assigns a new array of words according to the number of words in the command.
 In addition, the function inserts the words in order into the new array of words.
 In the last position sat null for execvp.
 */
char **splitCommandByWords(const char *command, int numberOfWords) {
    // allocate array of pointers, + 1 for NULL
    char **words = (char **) malloc((numberOfWords + 1) * sizeof(char *));
    if (words == NULL) {
        printf("Error allocating words memory\n");
        free(words);
        return NULL;
    }

    int startIndexWord;
    int wordLen;
    int wordIndex = 0;
    int i = 0;
    int flag = 0;
    for (; wordIndex < numberOfWords; wordIndex++) {
        if (flag == 1) {
            flag = 0;
            continue;
        }
        words[wordIndex] = (char *) malloc(MAX_CHARACTERS * sizeof(char));
        memset(words[wordIndex],0, MAX_CHARACTERS);
        wordLen = 0;
        // We will advance to the beginning of the word.
        while (isspace(command[i]))
            i++;
        startIndexWord = i;
        if (wordIndex < numberOfWords - 1) {
            // We will count the length of the word.
            while (!isspace(command[i]) && command[i] != '|' && command[i] != '>') {
                i++;
                wordLen++;
            }
            strncpy(words[wordIndex], command + startIndexWord, wordLen);
            if (command[i] == '|' || command[i] == '>') {
                if (wordLen > 0) {
                    words[wordIndex + 1] = (char *) malloc(2 * sizeof(char));
                    memset(words[wordIndex + 1],0, 2);
                    flag = 1;
                    if (command[i] == '|')
                        strcpy(words[wordIndex + 1], "|\0");
                    else if (command[i] == '>')
                        strcpy(words[wordIndex + 1], ">\0");
                    i++;
                }
                else if (wordLen == 0){
                    if (command[i] == '|')
                        strcpy(words[wordIndex], "|\0");
                    else if (command[i] == '>')
                        strcpy(words[wordIndex], ">\0");
                    i++;
                }
            }
            if (strchr(words[wordIndex],'\"') != NULL)
                removeQuotes(&(words[wordIndex]));
        } else {
            while (command[i] != '\0') {
                i++;
                wordLen++;
            }
            strncpy(words[wordIndex], command + startIndexWord, wordLen);
            if (strchr(words[wordIndex],'\"') != NULL)
                removeQuotes(&(words[wordIndex]));
        }

    }
    words[wordIndex] = NULL;
    return words;
}

/**
The process will receive an array of words will executes the process depend what include in the command.
if the command include cd or bg, it send Error or signal.
if the  command includes | > & or else its send the function to executes to the appropriate function.
 */
void process(char *command, int *numberOfTotalArguments, int *numberOfTotalCommands) {
    int numberOfWords = wordsCount(command);
    char **commandByWords = splitCommandByWords(command, numberOfWords);
    if (strcmp(*commandByWords,"cd") == 0) {
        printf("Error: cd not supported.\n");
    } else if (strcmp(*commandByWords,"bg") == 0) {
        (*numberOfTotalCommands)++;
        (*numberOfTotalArguments)++;
        kill(backgroundId, SIGCONT);
    } else if (numberOfWords > 10) {
        printf("Error: The command is invalid (more than 10 arguments)\n");
    } else {
        int numOfPipes = countChar(command, '|');
        int numOfAmpersand = countChar(command, '&');
        int numOfReadDirection = countChar(command, '>');
        *numberOfTotalArguments += numberOfWords - numOfPipes;
        (*numberOfTotalCommands)++;
        if (numOfPipes > 0)
            pipesProcess(commandByWords, numOfPipes);
        else if (numOfAmpersand == 1)
            backgroundProcess(commandByWords, numberOfWords);
        else if (numOfReadDirection == 1)
            redirectsToFile(commandByWords, numberOfWords);
        else
            regularProcess(commandByWords);
    }
    // Free the words from the command.
    for (int k = 0; k < numberOfWords; k++)
        free(commandByWords[k]);
    free(commandByWords);
}

/**
The function opens a new child with the global pid defined above.
 */
void createChildProcess() {
    processId = fork();
    if (processId < 0) {
        // In the case the fork failed.
        perror("ERR");
        exit(EXIT_FAILURE);
    }
}

/**
The function executes the command it receives as an argument.
 */
void launchProcess(char **argv) {
    execvp(argv[0], argv);
    // If execvp returns, it means an error occurred
    perror("ERR");
    exit(EXIT_FAILURE);
}

/**
The function receives an array of words and a number of words,
in the array of words there is a pipe, the function performs the pipe process.
 */
void pipesProcess(char **command, int numOfPipes) {
    int pipes[numOfPipes][2]; // array to store file descriptors for pipes
    int i, j;
    int commandStart = 0;

    for (i = 0; i < numOfPipes; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("ERR");
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i <= numOfPipes; i++) {
        int commandLen = 0;

        // Count the number of words in the current command
        while (command[commandStart + commandLen] != NULL &&
               strcmp(command[commandStart + commandLen], "|") != 0) {
            commandLen++;
        }

        char *cmd[commandLen + 1];
        memcpy(cmd, command + commandStart, commandLen * sizeof(char *));
        cmd[commandLen] = NULL;

        createChildProcess();
        if (processId == 0) { // child process
            // If not the first command, redirect input to read end of previous pipe
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("ERR");
                    exit(EXIT_FAILURE);
                }
            }
            // If not the last command, redirect output to write end of next pipe
            if (i < numOfPipes) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("ERR");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe file descriptors
            for (j = 0; j < numOfPipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            launchProcess(cmd);
        }

        // Move the command start index to the next command
        commandStart += commandLen + 1;
    }

    // Close all pipe file descriptors
    for (i = 0; i < numOfPipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to complete
    for (i = 0; i <= numOfPipes; i++) {
        waitpid(-1, NULL, WUNTRACED);
    }
}

/**
 * The function receives an array of words and the amount of words, in an existing array of words an ampersand,
 * the function performs the process in the background.
 */
void backgroundProcess(char **command, int numberOfWords) {
    for (int i = 0; i < strlen(command[numberOfWords - 1]); i++) {
        if (i > 0 && command[numberOfWords - 1][i] == '&') {
            command[numberOfWords - 1][i] = '\0';
            break;
        }
        if (i == 0 && command[numberOfWords - 1][i] == '&') {
            command[numberOfWords - 1] = NULL;
            break;
        }
    }
    createChildProcess();
    if (processId == 0) {
        signal(SIGTSTP, SIG_DFL);
        launchProcess(command);
    }
}

/**
The function receives an array of words and the amount of words,
 in an existing array of words >, the function performs the process that chains to the file.
 */
void redirectsToFile(char **command, int numberOfWords) {
    int fd;
    char **leftSide;
    char **rightSide;
    splitCommandByGreater(command, numberOfWords, &leftSide, &rightSide);
    if (rightSide[0] == NULL)
        return;
    for (int i = 1; i < numberOfWords; i++) {
        if (rightSide[i] != NULL) {
            printf("The name of the file is not illegal\n");
            return;
        }
    }
    createChildProcess();
    if (processId == 0) {
        fd = open(rightSide[0], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if (fd == -1) {
            printf("Error: The file was not opened successfully.\n");
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        launchProcess(leftSide);
    } else if (processId > 0)
        waitpid(-1, NULL, WUNTRACED);
    for (int j = 0; j < numberOfWords; j++) {
        free(leftSide[j]);
        free(rightSide[j]);
    }
    free(leftSide);
    free(rightSide);
}

/**
The function receives an array of words and a number of words, performs a regular process.
 */
void regularProcess(char **command) {
    createChildProcess();
    if (processId == 0) {
        launchProcess(command);
    } else if (processId > 0) {
        // Waiting for the child process to finish.
        waitpid(-1, NULL, WUNTRACED);
    }
}