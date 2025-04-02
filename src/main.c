/**
 * @file main.c
 * @date 9 Nov 2017
 *
 * @brief Contains all functions which are necessary for the program execution.
 */
#include "alloc.h"
#include "error.h"
#include "profile.h"
#include "sys_action.h"
#include "sys_info.h"

#include <argp.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Used to identify the argument in the argument parser.
 */
#define PROGRAM_IDENTIFIER 3000
#define PROGRAM_ARGS_IDENTIFIER 3001

extern char **environ;

/**
 * @brief Specifies the current name and version of the program.
 */

const char *argp_program_version = "cache-profiler 1.0.0";
/**
 * @brief Specifies the bug email address.
 */
const char *argp_program_bug_address = "no one";

/**
 * @brief Specifies the current header for the help command.
 */
const char doc[] =
    "profiler -- A program which measures the cache noise "
    "on a CPU core.\n\n"
    "OPERATION MODES:\n\n"
    "  profile\t\tUsing assembly to compute the time of a cache access.\n"
    "  bench\t\t\tBenchmarking the system.\n"
    "  info\t\t\tDisplays information about the cache."
    "\n\n OPTIONS:";

/**
 * @brief Specifies the command.
 */
const char args_doc[] = "<OPERATION MODE>";

/**
 * @brief Specifies the available options.
 */
static struct argp_option arg_options[] = {
    {"iter", 'i', "ITERATIONS", 0, "Specifies the amount of iterations."},
    {"seconds", 's', "SECONDS", 0,
     "Specifies the duration of the measurement in seconds."},
    {"cpu", 'c', "ID", 0,
     "Specifies the CPU core on which this program will run. If the process "
     "gets bound by a different program set this to -1."},
    {"pid", 'p', "PID", 0,
     "Specifies the PID of the another process. The process is moved to the"
     "same CPU core if one is specified. This can not be used with the "
     "--program and"
     "the --program-args arguments."},
    {"level", 'l', "LEVEL", 0,
     "Specifies which cache level should be analyzed."},
    {"program", PROGRAM_IDENTIFIER, "PROGRAM", 0,
     "Specifies a program which will be started before the measurement. This "
     "can not be used with the --pid argument."},
    {"program-args", PROGRAM_ARGS_IDENTIFIER, "ARGS", 0,
     "Specifies arguments for the program which will be run before the "
     "measurement. This can not be used with --pid argument."},
    {"output", 'o', "FILE", 0,
     "Saves the time measurement into a file instead of stdio."},
    {0}};

/**
 * @brief Available program arguments.
 *
 * This contains all available which can be set via the command line.
 */
typedef struct {
    char *mode; /**< Specifies the mode which will be executed. arguments#mode.
                 */
    int level;  /**< Specifies which cache level should be measured.
                   arguments#level. */
    int iter;   /**< Specifies the amount of measurement iterations.
                   arguments#iter. */
    int pid; /**< Specifies which other process should be moved to the same CPU
                core. arguments#pid. */
    int seconds; /**< Specifies the duration of the measurement in seconds
                    arguments#seconds*/
    int cpu; /**< Specifies which CPU core should be used.. arguments#cpu. */
    char *output_file; /**< Specifies the output file. arguments#output_file. */
    char *program;     /**< Specifies a program. arguments#output_file. */
    char *program_args; /**< Specifies the arguments of a program.
                           arguments#output_file. */
} arguments_t;

/**
 * @brief Parses the input for program arguments.
 *
 * @return error if the command is not valid or too many arguments are given.
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    arguments_t *arguments = state->input;

    switch (key) {
    case 'p':
        arguments->pid = atoi(arg);
        break;
    case 'c':
        arguments->cpu = atoi(arg);
        break;
    case 'l':
        arguments->level = atoi(arg);
        break;
    case 'i':
        arguments->iter = atoi(arg);
        break;
    case 's':
        arguments->seconds = atoi(arg);
        break;
    case 'o':
        arguments->output_file = arg;
        break;
    case PROGRAM_IDENTIFIER:
        arguments->program = arg;
        break;
    case PROGRAM_ARGS_IDENTIFIER:
        arguments->program_args = arg;
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num >= 1) {
            argp_usage(state);
        }
        arguments->mode = arg;
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 1) {
            /* Not enough arguments. */
            argp_usage(state);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/**
 * @brief Represents the final structure which contains all information about
 * the parser.
 */
static struct argp argp = {arg_options, parse_opt, args_doc, doc};

int main(int argc, char **argv) {
    output_t output;
    arguments_t arguments;
    cache_info_t cache;

    arguments.level = 1;
    arguments.iter = 0;
    arguments.pid = 0;
    arguments.cpu = 0;
    arguments.seconds = 0;
    arguments.output_file = NULL;
    arguments.program = NULL;
    arguments.program_args = NULL;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    pid_t this_pid = getpid();
    void *buffer = NULL;

    if (!this_pid) {
        printf("Could not get the PID of this process.");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        printf("Error while setting signal handler SIGTERM.");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGALRM, signal_handler) == SIG_ERR) {
        printf("Error while setting signal handler SIGALRM.");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("Error while setting signal handler SIGINT.");
        exit(EXIT_FAILURE);
    }

    {
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = extern_process_signal_handler;

        if (sigaction(SIGCHLD, &sa, NULL)) {
            printf("Error while setting signal handler SIGCHLD.");
            exit(EXIT_FAILURE);
        }
    }

    if (arguments.cpu > -1) {
        printf("Binding this process(%u) to CPU %d.\n", this_pid,
               arguments.cpu);
        EXIT_ON_FAIL(focus_cpu_core(this_pid, arguments.cpu),
                     "Error while setting CPU affinity of this process");
    } else {
        printf("WARNING: you have to bind this process(%u) to a fixed CPU by "
               "yourself.\n",
               this_pid);

        uint32_t core;
        EXIT_ON_FAIL(get_current_cpu_core(&core),
                     "Error while retrieving current CPU core");
        arguments.cpu = core;
    }

    printf(" --------------------------------------------------------------\n");

    printf("Using L%d cache on CPU %d\n", arguments.level, arguments.cpu);

    EXIT_ON_FAIL(cache_info_new(&cache, arguments.cpu, arguments.level),
                 "Error while initializing the cache info");

    cache_info_print(&cache);

    printf(" --------------------------------------------------------------\n");

    if (!strcmp(arguments.mode, "info")) {
        printf("Do nothing ... done.\n");
    } else {
        if (!has_root_access()) {
            fprintf(stderr, "ERROR this program needs root permissions\n");
            exit(EXIT_FAILURE);
        }

        EXIT_ON_FAIL(
            can_use_rdpmc(),
            "While checking if the rdpmc instruction can be used in userspace");

        if (!strcmp(arguments.mode, "bench")) {
            if (arguments.iter < 1) {
                arguments.iter = 1000000;
            }

            printf("Starting benchmark with %d iterations ...\n",
                   arguments.iter);
            EXIT_ON_FAIL(benchmark(arguments.iter), "Error while benchmarking");
        } else if (!strcmp(arguments.mode, "profile")) {
            if (arguments.output_file == NULL) {
                EXIT_ON_FAIL(outputc_stdout(&output, stdout),
                             "Error while creating output for stdout.");
            } else {
                EXIT_ON_FAIL(outputc_hd5_file(&output, arguments.output_file),
                             "Error while creating output file for HDF5.");
            }

            printf("Start profiling ");

            if (arguments.iter) {
                printf("for %d iterations ", arguments.iter);
            }

            if (arguments.seconds) {
                printf("for %d seconds ", arguments.seconds);
            }
            printf("\n\n");

            EXIT_ON_FAIL(alloc_aligned(&buffer, &cache),
                         "Failed to allocate an aligned buffer.");

            if (arguments.pid) {
                printf("Binding the given process(%d) to CPU %d.\n",
                       arguments.pid, arguments.cpu);
                EXIT_ON_FAIL(
                    focus_cpu_core(arguments.pid, arguments.cpu),
                    "Error while setting CPU affinity of the given process");
            } else if (arguments.program) {
                printf("Starting external program.\n");
                EXIT_ON_FAIL(run_program(arguments.program,
                                         arguments.program_args, environ,
                                         &(arguments.pid)),
                             "Error while starting external program");
                printf("Started external program with PID %d.\n",
                       arguments.pid);
            }

            if (arguments.seconds > 0) {
                alarm(arguments.seconds);
            }

            EXIT_ON_FAIL(
                profile(&cache, arguments.cpu, arguments.iter, buffer, &output),
                "Error while profiling");
        } else {
            fprintf(stderr, "Unknown operation mode %s.\n", arguments.mode);
            goto FINALIZE;
        }

        if (arguments.pid) {
            if (arguments.program) {
                printf("Sending SIGKILL to external process with PID %d.\n",
                       arguments.pid);
                kill(arguments.pid, SIGKILL);
            }
        }

        printf("Finished. Bye :)\n");
    }

FINALIZE:
    output_close(&output);
    error_t error_code;
    if (buffer != NULL && (error_code = free_aligned(buffer, &cache))) {
        fprintf(stderr, "Error while freeing buffer, %s(%d) ",
                decode_error(error_code), error_code);
        if (errno) {
            fprintf(stderr, ":%s(%d)", strerror(errno), errno);
        }
        fprintf(stderr, "\n");
    }

    return EXIT_SUCCESS;
}
