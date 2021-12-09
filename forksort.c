#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>


typedef struct child{
    int pid;
    int write_pipe[2];
    int read_pipe[2];
}child;


const char *prog;

static void usage(){
    fprintf(stderr, "[%s] usage: %s\n", prog, prog);
    exit(EXIT_FAILURE);
}

static void error_exit(char *msg, int e){
    fprintf(stderr, "%s: %d\n",msg, e);
    exit(EXIT_FAILURE);
}

static void fork_child(child *child ){

    if((pipe(child->write_pipe) == -1) || (pipe(child->read_pipe) == -1)){
        error_exit("creating pipe failed", errno);
    }
    
    switch((child->pid = fork())){
        case -1:
            error_exit("fork failed", errno);
            break;
        case 0:
        //child
            //dup write end
            close(child->write_pipe[0]);
            if(dup2(child->write_pipe[1], STDOUT_FILENO) == -1)
                error_exit("duplicate write end failed", errno);
            close(child->write_pipe[1]);

            //dup read end
            close(child->read_pipe[1]);
            if(dup2(child->read_pipe[0], STDIN_FILENO) == -1)
                error_exit("duplicate read end failed", errno);
            close(child->read_pipe[0]);

            if(execlp(prog, prog, NULL) == -1)
                error_exit("exec failed", errno);
        default:
        //parent
            close(child->write_pipe[1]);
            close(child->read_pipe[0]);
            break;
    }
}
/*
static void read_from_child(char ***output, FILE *f){

    char *tmp = NULL;
    size_t length = 0;
    int size = 0;
    while (getline(&tmp, &length, f) != -1)
    {
        output[size] = malloc(strlen(tmp));
        strncpy(output[size], tmp, strlen(tmp)); 
    }

    free(tmp);
}*/

static void mergesort(char **sol1, int size1, char **sol2, int size2){

    int i1 = 0;
    int i2 = 0;
    int cmp_size = 0;

    while((i1 < (size1)) && (i2 < (size2))){
        /*
        if(strlen(sol1[i1]) > strlen(sol2[i2]))
            cmp_size = strlen(sol2[i2]);
        else
            cmp_size = strlen(sol1[i1]);
        */
        if(strncmp(sol1[i1], sol2[i2], 2) < 0){
            fprintf(stdout, "%s", sol1[i1]);
            free(sol1[i1]);
            i1++;
        }
        else{
            fprintf(stdout, "%s", sol2[i2]);
            free(sol2[i2]);
            i2++;
        }
    }
    if(i2 == size2){
        for(;i1 < size1; i1++){
            fprintf(stdout, "%s", sol1[i1]);
            free(sol1[i1]);
        }
    }
    else{
        for(;i2 < size2; i2++){
            fprintf(stdout, "%s", sol2[i2]);
            free(sol2[i2]);
        }
    }
}


static void read_and_merge(FILE *c1, FILE *c2){

    char **c1_solution = malloc(sizeof(char *));
    char **c2_solution = malloc(sizeof(char *));
    int c1_solution_size = 0;
    int c2_solution_size = 0;
    int index = 0;
    size_t l = 0;
    
    char *tmp = NULL;

    while(getline(&tmp, &l, c1) != -1){
        c1_solution_size ++;
        c1_solution = realloc(c1_solution, c1_solution_size * sizeof(char *));
        c1_solution[index] = malloc(strlen(tmp));
        strncpy(c1_solution[index], tmp, strlen(tmp));
        index++;
    }
    index = 0;
    while(getline(&tmp, &l, c2) != -1){
        c2_solution_size ++;
        c2_solution = realloc(c2_solution, c2_solution_size * sizeof(char *));
        c2_solution[index] = malloc(strlen(tmp));
        strncpy(c2_solution[index], tmp, strlen(tmp));
        index++;
    }

    mergesort(c1_solution, c1_solution_size, c2_solution, c2_solution_size);
    free(tmp);
    free(c1_solution);
    free(c2_solution);
}



int main(int argc, char **argv){

    prog = argv[0];

    char *line1 = NULL;
    char *line2 = NULL;

    size_t length1 = 0;
    size_t length2 = 0;
    
    if((getline(&line1, &length1, stdin)) == -1){
    //wrong input
        usage();
    }

    if((getline(&line2, &length2, stdin)) == -1){
    //only 1 line as input 
        fprintf(stdout, "%s", line1);
        fflush(stdout);
        free(line1);
        free(line2);
        exit(EXIT_SUCCESS); 
    }

//at least 2 lines as input

    child child1;
    child child2;

    fork_child(&child1);
    fork_child(&child2);
    
//parent
    int c1_solution_size = 0;
    int c2_solution_size = 0;
    int status;

    FILE *c1_write = fdopen(child1.read_pipe[1], "w");
    FILE *c1_read = fdopen(child1.write_pipe[0], "r");
    FILE *c2_write = fdopen(child2.read_pipe[1], "w");
    FILE *c2_read = fdopen(child2.write_pipe[0], "r");

    fprintf(c1_write, "%s", line1);
    c1_solution_size++;
    fprintf(c2_write, "%s", line2);
    c2_solution_size++;
    
    //read and write remaining lines
    while (1)
    {
        if((getline(&line1, &length1, stdin)) == -1)
            break;
        fprintf(c1_write, "%s", line1);
        c1_solution_size++;

        if((getline(&line2, &length2, stdin)) == -1)
            break;
        fprintf(c2_write, "%s", line2);
        c2_solution_size++;
    }
    
    fflush(c1_write);
    fflush(c2_write);
    close(child1.read_pipe[1]);
    close(child2.read_pipe[1]);
    fclose(c1_write);
    fclose(c2_write);
    
    waitpid(child1.pid, &status, 0);
    waitpid(child2.pid, &status, 0);

    char *c1_solution[c1_solution_size];
    char *c2_solution[c2_solution_size];

    read_and_merge(c1_read, c2_read);
    
    //mergesort(c1_solution, c1_solution_size, c2_solution, c2_solution_size);
    //fflush(stdout);

    fclose(c1_read);
    fclose(c2_read);
    close(child1.write_pipe[0]);
    close(child1.read_pipe[0]);

    free(line1);
    free(line2);
    

    return EXIT_SUCCESS;
}

    



     
