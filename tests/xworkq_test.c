/*
 * Copyright 2013 GRNET S.A. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GRNET S.A. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GRNET S.A OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and
 * documentation are those of the authors and should not be
 * interpreted as representing official policies, either expressed
 * or implied, of GRNET S.A.
 */

#include <xseg/xworkq.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <xseg/xlock.h>
#include <sys/time.h>


unsigned long sum = 0;
struct xlock lock;

void jobfn(void *q, void *arg)
{
	unsigned long c = (unsigned long) arg;
	sum += c;
}

int test1(unsigned long n)
{
	struct xworkq wq;
	unsigned long i;
	xworkq_init(&wq, &lock, 0);
	sum = 0;
	xlock_release(&lock);

	for (i = 0; i < n; i++) {
		xworkq_enqueue(&wq, jobfn, (void *)1);
	}

	xworkq_destroy(&wq);

	return ((sum == n)? 0 : -1);
}

struct thread_arg{
	struct xworkq *wq;
	unsigned long n;
	unsigned long num;
};

void *thread_test(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	unsigned long n = targ->n;
	unsigned long i;

	for (i = 0; i < n; i++) {
		xworkq_enqueue(targ->wq, jobfn, (void *)targ->num);
	}


	return NULL;
}

int test2(unsigned long n, unsigned long nr_threads)
{
	int i, r;
	struct xworkq wq;
	xworkq_init(&wq, &lock, 0);
	sum = 0;
	xlock_release(&lock);

	struct thread_arg *targs = malloc(sizeof(struct thread_arg)*nr_threads * n);
	pthread_t *threads = malloc(sizeof(pthread_t) * nr_threads);

	for (i = 0; i < nr_threads; i++) {
		targs[i].num = i+1;
		targs[i].n = n;
		targs[i].wq = &wq;
	}
	for (i = 0; i < nr_threads; i++) {
		r = pthread_create(&threads[i], NULL, thread_test, &targs[i]);
		if (r) {
			fprintf(stderr, "error pthread_create\n");
			return -1;
		}
	}

	for (i = 0; i < nr_threads; i++) {
		pthread_join(threads[i], NULL);
	}



	free(targs);
	free(threads);
	xworkq_destroy(&wq);

	unsigned long expected_sum = 0;
	for (i = 0; i < nr_threads; i++) {
		expected_sum += n*(i+1);
	}
	return ((sum == expected_sum) ? 0 : -1);
}

int test3(unsigned long n, unsigned long nr_threads)
{
	int i, r;
	struct xworkq wq;
	xworkq_init(&wq, &lock, 0);
	sum = 0;
	xlock_release(&lock);

	struct thread_arg *targs = malloc(sizeof(struct thread_arg)*nr_threads * n);
	pthread_t *threads = malloc(sizeof(pthread_t) * nr_threads);

	for (i = 0; i < nr_threads; i++) {
		targs[i].num = i+1;
		targs[i].n = n;
		targs[i].wq = &wq;
	}
	for (i = 0; i < nr_threads; i++) {
		r = pthread_create(&threads[i], NULL, thread_test, &targs[i]);
		if (r) {
			fprintf(stderr, "error pthread_create\n");
			return -1;
		}
	}

	for (i = 0; i < nr_threads; i++) {
		pthread_join(threads[i], NULL);
	}



	free(targs);
	free(threads);
	xworkq_destroy(&wq);

	unsigned long expected_sum = 0;
	for (i = 0; i < nr_threads; i++) {
		expected_sum += n*(i+1);
	}
	return ((sum == expected_sum) ? 0 : -1);
}

int main(int argc, const char *argv[])
{
	struct timeval start, end, tv;
	int r;
	int n = atoi(argv[1]);
	int t = atoi(argv[2]);

	fprintf(stderr, "Running test1\n");
	gettimeofday(&start, NULL);
	r = test1(n);
	if (r < 0){
		fprintf(stderr, "Test1: FAILED\n");
		return -1;
	}
	gettimeofday(&end, NULL);
	timersub(&end, &start, &tv);
	fprintf(stderr, "Test1: PASSED\n");
	fprintf(stderr, "Test time: %ds %dusec\n\n", (int)tv.tv_sec, (int)tv.tv_usec);

	fprintf(stderr, "running test2\n");
	gettimeofday(&start, NULL);
	r = test2(n, t);
	if (r < 0){
		fprintf(stderr, "test2: failed\n");
		return -1;
	}
	gettimeofday(&end, NULL);
	fprintf(stderr, "test2: passed\n");
	timersub(&end, &start, &tv);
	fprintf(stderr, "Test time: %ds %dusec\n\n", (int)tv.tv_sec, (int)tv.tv_usec);

	fprintf(stderr, "running test3\n");
	gettimeofday(&start, NULL);
	r = test3(n, t);
	if (r < 0){
		fprintf(stderr, "test3: failed\n");
		return -1;
	}
	gettimeofday(&end, NULL);
	fprintf(stderr, "test3: passed\n");
	timersub(&end, &start, &tv);
	fprintf(stderr, "Test time: %ds %dusec\n\n", (int)tv.tv_sec, (int)tv.tv_usec);

	return 0;
}
