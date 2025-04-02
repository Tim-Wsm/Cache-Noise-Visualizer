/**
 * @file profile.c
 * @date 9 Nov 2017
 *
 * @brief Contains all functions which are necessary for the profiling process.
 */
#include "profile.h"
#include "sys_action.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>

void prime(uint64_t line_size, uint64_t set_count, uint64_t way_count,
           uint64_t total_size, const void *buffer) {

    volatile uint64_t buffer_offset = 0;
    volatile uint64_t set = 0;
    volatile uint64_t way = 0;

    asm volatile(
        //---------------------- load buffer set wise
        //---------------------------

        "primeouterloop%=:;"

        // way = way_count - 1
        "mov %[way_count],%[way];"
        "dec %[way];"

        "primeinnerloop%=:;"

        // buffer_offset = (way * cache->set_count+ set) * cache->line_size;
        "xor %[buffer_offset], %[buffer_offset];"
        "add %[way], %[buffer_offset];"
        "imulq %[set_count],%[buffer_offset];"
        "add %[set],%[buffer_offset];"
        "imulq %[line_size],%[buffer_offset];"

        "mfence;"
        "mov (%[buffer],%[buffer_offset],1), %%rax;" // access the memory
        "mfence;"

        "dec %[way];" // way--

        // if 0 < way go to innerloop
        "cmp $0, %[way];"
        "jnl primeinnerloop%=;"

        "inc  %[set];" // set++

        // if set < set_cout goto outerloop
        "cmp %[set_count],%[set];"
        "jl primeouterloop%=;"
        : // no output
        : [buffer] "r"(buffer), [set] "r"(set), [way] "r"(way),
          [total_size] "r"(total_size), [buffer_offset] "r"(buffer_offset),
          [set_count] "r"(set_count), [line_size] "r"(line_size),
          [way_count] "r"(way_count)
        : "memory", "eax", "ebx", "ecx", "edx", "rax", "rdx", "r8");
}

void probe(uint64_t line_size, uint64_t set_count, uint64_t way_count,
           const void *buffer, const uint32_t *result) {

    volatile uint64_t buffer_offset = 0;
    volatile uint64_t result_index = 0;
    volatile uint64_t set = 0;
    volatile uint64_t way = 0;

    asm volatile(
        "outerloop%=:;"

        "xor %[way],%[way];" // way = 0
        "innerloop%=:;"

        /* buffer_offset = (way * cache->set_count+ set) * cache->line_size;*/
        // buffer_offset = 0
        "xor %[buffer_offset], %[buffer_offset];"

        // buffer_offset = buffer_offset + way = way
        "add %[way], %[buffer_offset];"

        // buffer_offset = buffer_offset * set_count = way * set_count
        "imulq %[set_count],%[buffer_offset];"

        // buffer_offset = buffer_offset + set = way * set_count + set
        "add %[set],%[buffer_offset];"

        // buffer_offset = buffer_offset * line_size = (way * set_count + set) *
        // line_size
        "imulq %[line_size],%[buffer_offset];"

        /* ---------------------- measurement --------------------------------*/
        "cpuid;" // serialize execution
        "mov $1073741825, %%ecx;"
        "rdpmc;"            // load timestamps into edx:eax
        "mov %%eax, %%r8d;" // save the lower 32 bit of the first timestamp

        "mfence;"
        "mov (%[buffer],%[buffer_offset],1), %%rax;" // access the memory
        "mfence;"

        "mov $1073741825, %%ecx;"
        "rdpmc;"            // load timestamps into edx:eax
        "sub %%r8d, %%eax;" // r8 = eax-r8 (end - start)

        // write result into result buffer
        // (nontemporal hint -> doesn't modify the cache)
        "movnti %%eax, (%[result],%[result_index],4);"

        "mfence;"
        "cpuid;" // serialize execution
        /* -------------------------------------------------------------------*/

        "inc %[way];"          // way++
        "inc %[result_index];" // result_index++

        // if way < associativity goto innerloop
        "cmp %[way_count],%[way];"
        "jl innerloop%=;"

        "inc  %[set];" // set++

        // if set < set_count goto outerloop
        "cmp %[set_count],%[set];"
        "jl outerloop%=;"
        : // no output
        : [buffer] "r"(buffer), [result] "r"(result), [set] "r"(set),
          [result_index] "r"(result_index), [way] "r"(way),
          [buffer_offset] "r"(buffer_offset), [set_count] "r"(set_count),
          [line_size] "r"(line_size), [way_count] "r"(way_count)
        // "r" = stored in registers
        : "memory", "eax", "ebx", "ecx", "edx", "rax", "rdx", "r8");
}

error_t profile(const cache_info_t *cache, uint32_t cpu, uint32_t iterations,
                const void *buffer, output_t *output) {

    uint32_t result[cache->total_size / cache->line_size];
    uint32_t fd_cycle;
    FORWARD_ON_FAIL(enable_cpu_cycle_counter(&fd_cycle, cpu));

    for (int j = 0; (j < iterations || !iterations) && !terminated;
         j += (iterations ? 1 : 0)) {
        prime(cache->line_size, cache->set_count, cache->ways_of_associativity,
              cache->total_size, buffer);
        sched_yield();
        probe(cache->line_size, cache->set_count, cache->ways_of_associativity,
              buffer, result);
        FORWARD_ON_FAIL(outputw_mat_ui32(
            output, result, cache->ways_of_associativity, cache->set_count));
    }

    FORWARD_ON_FAIL(disable_cpu_cycle_counter(fd_cycle));

    return ERROR_NONE;
}

int cmpfunc(const void *a, const void *b) { return (*(int *)a - *(int *)b); }

error_t benchmark(uint64_t iterations) {
    uint32_t *result = malloc(iterations * sizeof(uint32_t));
    uint32_t fd_cycle;

    FORWARD_ON_FAIL(enable_cpu_cycle_counter(&fd_cycle, 0));
    uint64_t buffer[1];

    for (uint64_t result_index = 0; result_index < iterations; result_index++) {
        asm volatile(
            "cpuid;" // serialize execution
            "mov $1073741825, %%ecx;"
            "rdpmc;" // pseudo serialize and load timestamps into edx:eax
            "mov %%eax, %%r8d;" // save the lower 32 bit of the first timestamp

            "mfence;"
            "mov (%[buffer]), %%rax;" // access the memory
            "mfence;"

            "mov $1073741825, %%ecx;"
            "rdpmc;" // pseudo serialize and load timestamps into edx:eax
            "sub %%r8d, %%eax;" // r8 = eax-r8 (end - start)

            // write result into result buffer
            // (nontemporal hint -> doesn't modify the cache)
            "movnti %%eax, (%[result],%[result_index],4);"

            "mfence;"
            "cpuid;" // serialize execution
            :        // no output
            : [buffer] "r"(buffer), [result] "r"(result),
              [result_index] "r"(result_index)
            // "r" = stored in registers
            : "memory", "eax", "ebx", "ecx", "edx", "rax", "rdx", "r8");
    }

    FORWARD_ON_FAIL(disable_cpu_cycle_counter(fd_cycle));

    // sort all results
    qsort(result, iterations, sizeof(uint32_t), cmpfunc);

    // calculate median
    double middle = iterations / 2;
    uint32_t median = 0;
    if (floor(middle) == ceil(middle)) {
        median = result[(size_t)middle];
    } else {
        median = result[((size_t)floor(middle)) + 1];
    }
    printf("median: %u\n", median);

    // calculate arithmetic_mean
    uint64_t accumulator = 0;
    for (uint32_t i = 0; i < iterations; i++) {
        accumulator += result[i];
    }
    double arithmetic_mean = accumulator / ((double)(iterations));
    printf("arithmetic mean: %lf\n", arithmetic_mean);

    // calculate the empirical variance
    double s2 = 0;
    for (uint32_t i = 0; i < iterations; i++) {
        s2 = pow(((double)result[i]) - arithmetic_mean, 2);
    }
    s2 = s2 / (((double)(iterations)) - 1.0);
    printf("empirical variance: %lf\n", s2);

    int over_1000_counter = 0;
    int other_median_counter = 0;
    for (int i = 0; i < iterations; i++) {
        if (result[i] != median) {
            other_median_counter++;
        }
        if (result[i] > 1000) {
            over_1000_counter++;
        }
    }
    printf("%d values are bigger than 1000 (probably reschedules)\n",
           over_1000_counter);
    printf("%d values differ from the median(%%%lf)\n", other_median_counter,
           other_median_counter / (double)iterations);

    free(result);
    return ERROR_NONE;
}
