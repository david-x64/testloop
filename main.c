#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct string_list {
    char* s;
    struct string_list* next;
} string_list;

static long i = 10;
static char* test = NULL;
static string_list* testargs = NULL;
static int testargs_counter = 0;

static error_t parser(int key, char* arg, struct argp_state* state);

static bool runTest();

int main(int argc, char* argv[]) {
    argp_program_version = "0.1.0";
    argp_program_bug_address = "david.zhang.x64@gmail.com";

    struct argp_option options[2] = {
            {
                .name = "iterations",
                .key = 'i',
                .arg = "value",
                .doc = "Repeat test for this many iterations"
            },
            {0}
    };

    struct argp argp = {
            .args_doc = "test",
            .doc = "Executes the test in a loop for the specified amount of iterations. Stops when a test fails.",
            .options = options,
            .parser = parser
    };

    error_t error = argp_parse(&argp, argc, argv, 0, NULL, NULL);
    if(error) {
        return EXIT_FAILURE;
    }

    if(test == NULL) {
        fprintf(stderr, "Missing argument 'test'.\n");
        return EX_USAGE;
    }

    bool success = true;
    int k = 0;
    while(success && k++ < i) {
        printf("run execution nr. %i\n", k);
        success = runTest();
        if(success) {
            printf("execution nr. %i was successful.\n", k);
        }
    }

    if(success) {
        printf("successfully completed %li executions.\n", i);
        return EXIT_SUCCESS;
    }
    printf("program failed on execution %i\n", k);
    return EXIT_FAILURE;
}

/**
 * Parse commandline options and arguments.
 * Options after ' -- ' will not be parsed, they will be treated as arguments
 * to the test program.
 *
 * @param key
 * @param arg
 * @param state
 * @return
 */
error_t parser(int key, char* arg, struct argp_state* state) {
    switch(key) {
        case 'i': {
            errno = 0;
            char* endptr = NULL;
            long result = strtol(arg, &endptr, 10);

            if (errno != 0) {
                perror("Failed to parse number of iterations: strtol");
                return EX_USAGE;
            }

            if (*endptr != '\0') {
                fprintf(stderr, "Number of iterations is invalid: %s\n", arg);
                return EX_USAGE;
            }

            i = result;
            break;
        }
        case ARGP_KEY_ARG:
            if(test == NULL) {
                size_t len = strlen(arg);
                test = malloc(len + 1);
                strcpy(test, arg);
                break;
            }
            testargs_counter++;
            string_list* lastTestarg = testargs;
            string_list* newTestarg = malloc(sizeof(string_list));
            if(lastTestarg == NULL) {
                testargs = newTestarg;
            } else {
                while(lastTestarg->next != NULL) {
                    lastTestarg = lastTestarg->next;
                }
                lastTestarg->next = newTestarg;
            }
            size_t len = strlen(arg);
            newTestarg->s = malloc(len + 1);
            strcpy(newTestarg->s, arg);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static void execTest();

/**
 * Fork and execute the test. The parent process waits for child to complete.
 * Return true if the child (test) exits normally and reports success via its
 * exit code.
 *
 * @return true for success, false otherwise
 */
bool runTest() {
    errno = 0;
    pid_t pid = fork();
    if(pid != 0) {
        if(errno || pid == -1) {
            fprintf(stderr, "failed to create child process\n");
            if(errno) {
                perror("fork");
            }
            return false;
        }
        int status = 0;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return true;
        }
        return false;
    }
    execTest();
    return false;
}

/**
 * Execute the test program, passing all test arguments.
 * If this function returns, then only because it failed.
 */
void execTest() {
    char** argv = malloc(sizeof(char*) * (testargs_counter + 2));

    argv[0] = test;
    size_t idx = 1;

    string_list* arg = testargs;
    while(arg != NULL) {
        argv[idx] = malloc(strlen(arg->s) + 1);
        strcpy(argv[idx], arg->s);
        idx++;
        arg = arg->next;
    }
    argv[idx] = NULL;

    errno = 0;
    execvp(test, argv);
    fprintf(stderr, "failed to start test process\n");
    if(errno != 0) {
        perror("execvp");
    }
}
