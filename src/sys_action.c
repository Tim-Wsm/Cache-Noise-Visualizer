/**
 * @file sys_action.c
 * @date 13 Mar 2017
 *
 * @brief Contains functions which change the state of the system.
 */

#define _GNU_SOURCE /* needed for CPU_ZERO, CPU_SET and sched_setaffinity*/

#include "sys_action.h"
#include <stdio.h>

#include <asm/unistd.h>
#include <signal.h>
#include <unistd.h>

#include <stdlib.h>

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/resource.h>

error_t focus_cpu_core(uint32_t pid, uint32_t cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);

    if (sched_setaffinity(pid, sizeof(set), &set)) {
        return ERROR_SET_AFFINITY;
    }

    return ERROR_NONE;
}

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

error_t enable_cpu_cycle_counter(uint32_t *fd, uint32_t cpu) {
    struct perf_event_attr pe_cycle;

    memset(&pe_cycle, 0, sizeof(struct perf_event_attr));
    pe_cycle.type = PERF_TYPE_HARDWARE;
    pe_cycle.size = sizeof(struct perf_event_attr);
    pe_cycle.config = PERF_COUNT_HW_CPU_CYCLES;
    pe_cycle.disabled = 1;
    pe_cycle.exclude_kernel = 1; // need sudo permission to include
    pe_cycle.exclude_hv = 1;
    *fd = perf_event_open(&pe_cycle, 0, cpu, -1, 0);

    if (*fd == -1) {
        return ERROR_FD_CYCLE;
    }

    if (ioctl(*fd, PERF_EVENT_IOC_RESET, 0)) {
        return ERROR_FD_CYCLE;
    };

    if (ioctl(*fd, PERF_EVENT_IOC_ENABLE, 0)) {
        return ERROR_FD_CYCLE;
    };

    return ERROR_NONE;
}

error_t disable_cpu_cycle_counter(uint32_t fd) {
    if (ioctl(fd, PERF_EVENT_IOC_DISABLE, 0)) {
        return ERROR_FD_CYCLE_CLOSE;
    };
    return ERROR_NONE;
}

error_t run_program(const char *path, const char *arguments, char **env,
                    int *pid) {
    if (path != NULL) {
        wordexp_t p;
        posix_spawnattr_t attr;
        int ret;

        if (posix_spawnattr_init(&attr) != 0) {
            return ERROR_SPAWN_ATTR_INIT;
        }

        // do not copy the tables for the child process
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_USEVFORK);

        posix_spawn_file_actions_t file_actions;

        if (posix_spawn_file_actions_init(&file_actions) != 0) {
            return ERROR_SPAWN_FILE_INIT;
        }

        if (arguments) {
            wordexp(arguments, &p, 0);

            int argc = p.we_wordc + 1;
            char **argv = calloc(sizeof(char *), argc + 1);

            if (argv == NULL) {
                return ERROR_ALLOCATION;
            }

            int i = 0;

            argv[i] = basename(path);

            for (i = 0; i < p.we_wordc; i++) {
                argv[i + 1] = p.we_wordv[i];
            }

            argv[argc] = NULL;

            ret = posix_spawnp(pid, path, &file_actions, &attr, argv, env);

            wordfree(&p);
            free(argv);
        } else {
            char *argv[] = {basename(path), NULL};
            ret = posix_spawnp(pid, path, &file_actions, &attr, argv, env);
        }

        if (ret) {
            return ERROR_SPAWN_SPAWN;
        }
    } else {
        return ERROR_SPAWN_PATH_NULL;
    }

    return ERROR_NONE;
}

int terminated = 0;
void signal_handler(int signo) {
    if (signo == SIGTERM || signo == SIGALRM || signo == SIGINT) {
        terminated = 1;
    }
}

void extern_process_signal_handler(int _signo) {
    pid_t pid = 1;
    int status;

    while (pid > 0) {
        pid = waitpid(-1, &status, WNOHANG);
    }

    terminated = 1;
}
