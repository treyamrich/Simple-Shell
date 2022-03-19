#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
using namespace std;

void parseString(vector<char**> &commands, char* cstring);
void execCommands(vector<char**>&, int*, const int&);
void closefd(int* fd, const int& fdSize);
void waitAndPrint(pid_t* pids, int cmdAmt);

int main(int argc, char* argv[]) {
    //Shell prompt  
    cout << "myshell$";
    string input;
    getline(cin, input);
    //Check if the input is non-empty
    if(input.length() > 0) {
        //Create a string and copy the cstring
        char* copyCStr = new char[input.length()+1];
        const char* cstr = input.c_str();
        for(unsigned long int i = 0; i <= input.length(); i++) {
            copyCStr[i] = cstr[i];
        }

        //Parse cstring into tokens, organize into an argv for each command.
        vector<char**> commands;
        parseString(commands, copyCStr);

        //Run multiple commands with n-1 pipes for n > 1
        if(commands.size() > 1) {
            const int numDesc = (commands.size() - 1) * 2;
            int fileDesc[numDesc];
            for(int j = 0; j < numDesc; j+=2) {
                if(pipe(fileDesc + j) < 0)
                    perror("Error creating pipe");
            }
            execCommands(commands, fileDesc, numDesc);
        } else { //Run a single command
            pid_t c_pid;
            if((c_pid = fork()) < 0) {
                perror("Error creating child process");
            } else if(c_pid > 0) { 
                waitAndPrint(&c_pid, 1); //Wait on 1 child
            } else if(execvp(commands[0][0], commands[0]) < 0) {
                perror("Error executing command");
                exit(1);
            }
        }
        
        //Delete each dynamically allocated argv[] from parseString()
        for(vector<char**>::iterator it = commands.begin(); it != commands.end(); ++it) {
            delete [] *it;
        }
        //Deallocate copied cstring
        delete [] copyCStr;
    }
    return 0;
}

/*Preconditions: Command vector and cstring should ideally not be empty, but it won't break the function.
Postconditions: Function dynamically creates arrays for each command delimited by a pipe. Each array is pushed
into the given command vector. If the command vector size is greater than 0, the memory MUST BE DEALLOCATED
on the heap, after use.*/

void parseString(vector<char**> &commands, char* cstring) {

    char** args;
    args = new char*[21]; //Assume maximum of 20 tokens
    int argPos = 0;
    char* token = strtok(cstring, " ");
    
    while(token) {
        if(*token != '|') { //If no pipe add token to args
            args[argPos] = token;
            argPos++;
        } else { //If pipe, push args to command vector, create another args array, reset array insert position
            args[20] = NULL;
            commands.push_back(args);
            args = new char*[21];
            argPos = 0;
        }
        if(!(token = strtok(NULL, " "))) { //Get next token, if it's the final token, push to command vector
            args[20] = NULL;
            commands.push_back(args);
        }
    }
    if(argPos == 0) delete [] args; //If cstring was empty, delete args
}

/*Preconditions: fd must not be a nullptr and fdSize must non-zero, command vector size must be > 1
Postconditions: Function creates children for each command, allowing them to execute them with the system call execvp().
The parent will wait for each child and print the exit status using waitAndPrint().*/

void execCommands(vector<char**> &commands, int* fd, const int& fdSize) {
    pid_t c_pids[commands.size()];

    /*Fork for each command and pass the command array to child to call execvp.
    Parent: closes its own descriptors as its creating more children. At the end of the for loop, all parent descriptors will be closed.
    Child: duplicates descriptors it needs, closes the rest, and executes using execvp()*

    2*i is necessary because there are 2 file descriptors for each pipe.
                fd[1] = the write end of first pipe
                2*i-2 = read end of the prev pipe.
                2*i+1 = write end of the next pipe.*/
    for(int i = 0; i < commands.size(); i++) {
        if((c_pids[i] = fork()) < 0) { 
            perror("Error creating child process");
        } else if(c_pids[i] > 0) { 
            //Parent
            if(i == 0) { //Closes write end of first pipe
                close(fd[1]); 
            } else if(i == commands.size() - 1) { //Closes read end of last pipe
                close(fd[2*i-2]); 
            } else { //Intermediate pipes, close read of prev pipe and write of next pipe
                close(fd[2*i-2]);
                close(fd[2*i+1]);
            }
        } else { 
            //Child
            if(i == 0) { //Duplicate the write end of first pipe
                if(dup2(fd[1], 1) < 0)
                    perror("Error duplicating file descriptor");
                //Close ALL file descriptors
                closefd(fd, fdSize);
            } else if(i == commands.size() - 1) { //Duplicate read end of last pipe
                if(dup2(fd[2*i-2], 0) < 0)
                    perror("Error duplicating file descriptor");
                //Only need to close 1, since parent closed all others
                close(fd[2*i-2]); 
            } else { //Intermediate pipes, duplicate read of prev pipe and write of next pipe
                if(dup2(fd[2*i-2], 0) < 0 || dup2(fd[2*i+1], 1) < 0)
                    perror("Error duplicating file descriptor");
                //Close ALL file descriptors
                closefd(fd, fdSize);
            } 
            //Run command
            if(execvp(commands[i][0], commands[i]) < 0) {
                perror("Error executing command");
                exit(1);
            }
        }
    }
    //Wait for children
    waitAndPrint(c_pids, commands.size());
}

/*Precondition: None
Precondition: This helper function traverses the array and closes all the file descriptors in the array*/
void closefd(int* fd, const int& fdSize) {
    for(int j = 0; j < fdSize; j++) {
        close(fd[j]);
    }
}

/*Precondition: the pid_t pointer should not be null
Postcondtion: Function traverses the given process id array and calls the system call wait() for each pid.
It prints out the exit status to stdout, and the error message (if there is one).*/
void waitAndPrint(pid_t* pids, int cmdAmt) {
    int exitStatus;
    for(int k = 0; k < cmdAmt; k++) {
        if(wait(&exitStatus) < 0)
            perror("Error waiting for child process");
        cout << "process " << pids[k] << " exits with " << exitStatus << endl;
    }
}