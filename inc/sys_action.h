/**
 * @file sys_action.h
 * @date 13 Mar 2017
 *
 * @brief Contains functions which change the state of the system.
 */

#pragma once

#include "error.h"
#include <spawn.h>
#include <stdint.h>
#include <sys/wait.h>
#include <wordexp.h>

/**
 * @brief Locks a process to one CPU core
 *
 * @param pid Process id which will be bound
 * @param cpu id of the CPU the process will be bound to
 *
 * @retval ERROR_SET_AFFINITY
 * @retval ERROR_NONE
 *
 */
error_t focus_cpu_core(uint32_t pid, uint32_t cpu);

/**
 * @brief Enables a CPU cycle counter which can be read with RDPMC
 *
 * @param fd pointer to the file descriptor of the initialized perf_event
 * @param cpu id of the CPU the counter will be bound to
 *
 * @retval ERROR_FD_CYCLE
 * @retval ERROR_NONE
 *
 */
error_t enable_cpu_cycle_counter(uint32_t *fd, uint32_t cpu);

/**
 * @brief Disables a CPU cycle counter
 *
 * @param fd file descriptor of the perf_event which gets disabled
 *
 * @retval ERROR_FD_CYCLE_CLOSE
 * @retval ERROR_NONE
 *
 */
error_t disable_cpu_cycle_counter(uint32_t fd);

/**
 * @brief Runs a given program with given arguments as a child process.
 *
 * @param path Path of the program or a name
 * @param arguments A string which contains shell-like arguments.
 * @param env A environment which is passed to the program.
 * @param pid The PID of the started process is written into.
 *
 * @retval ERROR_SPAWN_ATTR_INIT
 * @retval ERROR_SPAWN_FILE_INIT
 * @retval ERROR_ALLOCATION
 * @retval ERROR_SPAWN_SPAWN
 * @retval ERROR_SPAWN_PATH_NULL
 * @retval ERROR_NONE
 *
 */
error_t run_program(const char *path, const char *arguments, char **env,
                    int *pid);

/**
 * @brief Is 0 on default and changes to 1 if a signal got intercepted by
 * signal_handler.
 *
 */
extern int terminated;

/**
 * @brief Handles incoming signals and sets the global variable terminated.
 *
 * @param signo Takes a SIGNAL identifier as input.
 *
 * If signo is either SIGTERM ,SIGALRM or SIGINT the global variable terminated
 * gets set to 1.
 */
void signal_handler(int signo);

/**
 * @brief Handles incoming SIGCHLD signals and sends SIGTERM after the child has
 * exited.
 *
 * @param _signo Takes a SIGNAL identifier as input.
 *
 * If a SIGCHILD is received then this process waits for the child and then
 * sends a SIGTERM to itself. This will cause this program to stop.
 */
void extern_process_signal_handler(int _signo);
