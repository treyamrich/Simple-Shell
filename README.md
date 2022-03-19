# Simple Shell

## Purpose

This program/project was made for practicing the use of system calls and concurrency in programming.
The program is a simple shell which executes linux commands given by the user.

## Limits

**Strengths:** 
- The program can run an infinite amount of commands since **C++ strings** and **STL Vectors** are used (limited by memory).
- Takes advantage of concurrency since the parent creates all the pipes and children before waiting

**Weaknesses:**
- Only supports pipe. There are other shell operations that could've been implemented.
- Since **C++ strings** are used, an extra step of copying the input from the user into a cstring must take place.


## Additional Helper Function Information

Information on function `parseStrings()`:

    Input: empty vector which will hold arrays of cstring tokens, and a cstring version of the user input from `stdin`.
    Output: None, however, the function alters the input command vector by adding token arrays to it.

    The logic behind the function:
        1) Create a dynamically allocated array of cstrings.
        2a) Get the first token, and if it is not null, loop and get more tokens until none are found.
        2b) If the first token is null, do not enter loop and deallocate the array from step 1.
        3a) Token loop - If the token is not a pipe, add the token to the array and increment the array insert position.
        3b) Else, NULL terminate the array, push the array into the command vector, create a new array on heap, and reset 
            array position to the beginning.
        3c) Finally, get the next token in the cstring and check if it is the last command. If its the last command, 
            NULL terminate array, and push the array to command vector.



Information on function `execCommands()`:

    **Input:** vector of commands, an array of file descriptors for the pipes, and the size of the array.
    The array is used for duplicating file descriptors, and the size is simply passed to the `closefd()` function.

    **Output:** Originates from the calling function `waitAndPrint()`, which prints the process ids and the respective exit code

    The logic behind the function: 
        1) Create an array of pids for the parent process to wait for in the future.
        2) Loop through the command vector and call `fork()` each time.
        3a) Parent process closes the fds for pipe(s) that its new child will be using.
        3b) Child process duplicates the fd for the pipe it will be using, then closes all other fds.
        3c) Child executes the command given by the corresponding index in the command vector.
        4) Parent calls the `waitAndPrint()` function to wait for all children created.



Information on function `closefd` and `waitAndPrint`:
    These functions are helper functions to prevent redundancy. 
        - `closefd` closes all file descriptors in an array.
        - `waitAndPrint` is called by the parent process and waits for each process in the array, printing the exit status of the child. 