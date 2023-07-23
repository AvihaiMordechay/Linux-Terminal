# Linux-Terminal
Authored by Avihai Mordechay

==Description==
The program simulates shell, in fact the program receives commands from the user and passes them on for execution in a format suitable for an operating system. 
In addition, the program will contain a data structure that will save variables that the user has defined.
Structure program:
Once the program has input from the user, its check if the user has used variables and if so, it will perform the required actions (define the variable or replaces the variable in the value into the user input).
Then, the program will divide the user input by commands into an array and run on them in loop.
In the loop the program will divide each command to words into an array. 
When the program holds an array of words of the command, the program will create a child process and send the array of the words to execution.
After all the commands have been sent for execution, we will receive another input from the user and execute the above again.
The exit condition is pressing 3 times enter.

Program memory:
getCwd: Contain the path of the user.
updateUserInput: Contain the user input.
commands: Contain the commands of the user input.
command: Contain the words of the command.
Variables: Contain the variables and they value.

Main functions:
checkVariables: The function handles everything related to defining variables and replacing the variables with its value into the user input.
splitStringByCommands: The function divides the user input to commands.
splitCommandByWords: The function divides the command to words.


==Program Files==
Terminal.c
==How to compile?==
compile: gcc Terminal.c -o Terminal
run: ./Terminal

==Input:==
User commands

==Output:==
The content of the command the user ran.
