/*
Copyright (C) 2010-2014 GRNET S.A.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <math.h>

#include <xseg/xlock.h>

struct thread_data {
    long loops;
    struct xlock *lock;
    long *counter;
    int id;
};

void *race_thread(void *arg)
{
    struct thread_data *th = arg;
    long loops = th->loops;
    struct xlock *lock = th->lock;
    long *counter = th->counter;
    unsigned long serial = 0, oldserial = 0, total = 0, maxdiff = 0, diff = 0;
    double totaldiff = 0.0;
    unsigned long *diffstat;
    long i;

    diffstat = calloc((int)log2(loops), sizeof(unsigned long));
    if (!diffstat) {
        perror("malloc");
        return NULL;
    }

    oldserial = xlock_acquire(lock, XLOCK_UNKNOWN_OWNER);
    xlock_release(lock);

    printf("%d: starting at %lu\n", th->id, oldserial);
    for (i = 0; i < loops; i++) {
        //if ((i & 15) == 0)
        //printf("%d: %lu\n", th->id, i);
        asm volatile ("#boo");
        serial = xlock_acquire(lock, XLOCK_UNKNOWN_OWNER);
        asm volatile ("#bee");
        //serial = oldserial +1;
        (*counter) ++;
        diff = serial - oldserial;
        oldserial = serial;
        if (diff > maxdiff)
            maxdiff = diff;
        diffstat[(int)log2(diff)] ++;
        if (diff > 1) {
            total += 1;
            totaldiff += diff;
        }
        xlock_release(lock);
    }

    xlock_acquire(lock, XLOCK_UNKNOWN_OWNER);
    printf("%d: serial %lu, avediff: %.0lf/%lu = %lf maxdiff: %lu\n",
            th->id, serial, totaldiff, total, totaldiff/total, maxdiff);
    printf("stats:\n");
    for (i = 0; i < (int)log2(loops); i++)
        printf("    %012lu: %lu\n", (unsigned long)powl(2, i), diffstat[i]);
    xlock_release(lock);
    return NULL;
}

int error(const char *msg) {
    perror(msg);
    return 1;
}

long lock_race(long nr_threads, long loops, struct xlock *lock, long *counter)
{
    struct thread_data *th = malloc(nr_threads * sizeof(struct thread_data));
    long t, r;
    if (!th)
        return error("malloc");

    pthread_t *threads = malloc(nr_threads * sizeof(pthread_t));
    if (!threads)
        return error("malloc");

    for (t = 0; t < nr_threads; t++) {
         th[t].id = t;
         th[t].loops = loops;
         th[t].counter = counter;
         th[t].lock = lock;
    }

    for (t = 0; t < nr_threads; t++) {
         r = pthread_create(&threads[t], NULL, race_thread, &th[t]);
         if (r)
            return error("pthread_create");
    }

    for (t = 0; t < nr_threads; t++) {
         pthread_join(threads[t], NULL);
    }

    return nr_threads * loops - *counter;
}

struct xlock lock;
long counter;

int main(int argc, char **argv)
{
    long loops, nr_threads, r;

    if (argc < 3) {
        printf("Usage: xlock_test <nr_threads> <nr_loops>\n");
        return 1;
    }

    nr_threads = atoi(argv[1]);
    if (nr_threads < 0) nr_threads = 2;
    loops = atol(argv[2]);
    if (loops < 0) loops = 1000;

    struct timeval tv0, tv1;
    gettimeofday(&tv0, NULL);
    r = lock_race(nr_threads, loops, &lock, &counter);
    gettimeofday(&tv1, NULL);
    double seconds = tv1.tv_sec + tv1.tv_usec/1000000.0 - tv0.tv_sec - tv0.tv_usec / 1000000.0;
    printf("lock race complete with %ld errors in %lf seconds\n", r, seconds);
    if (r)
        return r;

    return 0;
}
