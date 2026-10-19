#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define ERODE  0
#define DILATE 1

typedef struct {
	const double *src;
	double *dst;
	int n;
	int from, to;
	int mode;
	int sleep_sec;
} Task;

static void die(const char *msg) {
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

static double now_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void generate(double *m, int n, unsigned seed) {
	srand(seed);
	for (long i = 0; i < (long)n * n; i++) {
		m[i] = rand() / (double)RAND_MAX * 100.0;
	}
}

static void filter_rows(const double *src, double *dst, int n, int from, int to, int mode) {
	for (int i = from; i < to; i++) {
		for (int j = 0; j < n; j++) {
			double best = src[(long)i * n + j];

			for (int di = -1; di <= 1; di++) {
				for (int dj = -1; dj <= 1; dj++) {
					int a = i + di;
					int b = j + dj;

					if (a < 0 || a >= n || b < 0 || b >= n) {
						continue;
					}

					double v = src[(long)a * n + b];
					if (mode == ERODE) {
						if (v < best) best = v;
					} else {
						if (v > best) best = v;
					}
				}
			}

			dst[(long)i * n + j] = best;
		}
	}
}

static void *worker(void *arg) {
	Task *t = (Task *)arg;

	if (t->sleep_sec > 0) {
		sleep(t->sleep_sec);
	}
	filter_rows(t->src, t->dst, t->n, t->from, t->to, t->mode);

	return NULL;
}

static void run_seq(const double *input, double *result, int n, int k, int mode) {
	size_t size = (size_t)n * n * sizeof(double);
	double *a = malloc(size);
	double *b = malloc(size);
	if (a == NULL || b == NULL) die("not enough memory");

	memcpy(a, input, size);

	for (int step = 0; step < k; step++) {
		filter_rows(a, b, n, 0, n, mode);
		double *tmp = a; a = b; b = tmp;
	}

	memcpy(result, a, size);
	free(a);
	free(b);
}

static void run_par(const double *input, double *result, int n, int k, int mode,
                    int threads, int demo_sleep) {
	size_t size = (size_t)n * n * sizeof(double);
	double *a = malloc(size);
	double *b = malloc(size);
	pthread_t *ids = malloc(threads * sizeof(pthread_t));
	Task *tasks = malloc(threads * sizeof(Task));
	if (a == NULL || b == NULL || ids == NULL || tasks == NULL) die("not enough memory");

	memcpy(a, input, size);

	for (int step = 0; step < k; step++) {

		for (int t = 0; t < threads; t++) {
			tasks[t].src = a;
			tasks[t].dst = b;
			tasks[t].n = n;
			tasks[t].from = (int)((long)t * n / threads);
			tasks[t].to = (int)((long)(t + 1) * n / threads);
			tasks[t].mode = mode;
			tasks[t].sleep_sec = (step == 0) ? demo_sleep : 0;

			if (pthread_create(&ids[t], NULL, worker, &tasks[t]) != 0) {
				die("failed to create thread");
			}
		}

		for (int t = 0; t < threads; t++) {
			pthread_join(ids[t], NULL);
		}

		double *tmp = a; a = b; b = tmp;
	}

	memcpy(result, a, size);
	free(a);
	free(b);
	free(ids);
	free(tasks);
}

static void print_matrix(const char *title, const double *m, int n) {
	printf("%s:\n", title);
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			printf("%8.3f ", m[(long)i * n + j]);
		}
		printf("\n");
	}
}

static void usage(const char *prog) {
	fprintf(stderr,
		"usage: %s -m seq|par [-t threads] [-n size] [-k iterations] [-r repeats]\n"
		"          [-s seed] [-d seconds] [-p]\n"
		"  -m  режим: seq (последовательный) или par (параллельный)\n"
		"  -t  максимальное число потоков (для par), по умолчанию 1\n"
		"  -n  размер матрицы n x n, по умолчанию 1000\n"
		"  -k  сколько раз накладывать фильтры, по умолчанию 10\n"
		"  -r  сколько раз повторить замер (берётся среднее), по умолчанию 3\n"
		"  -s  зерно генератора случайных чисел, по умолчанию 1\n"
		"  -d  пауза потоков на первом шаге, чтобы успеть посмотреть их в ps/top (используйте с -r 1)\n"
		"  -p  вывести матрицы (только если n <= 12)\n",
		prog);
	exit(1);
}

int main(int argc, char **argv) {
	const char *mode_name = NULL;
	int threads = 1, n = 1000, k = 10, repeats = 3, demo_sleep = 0;
	unsigned seed = 1;
	int print = 0;

	int opt;
	while ((opt = getopt(argc, argv, "m:t:n:k:r:s:d:p")) != -1) {
		switch (opt) {
		case 'm': mode_name = optarg; break;
		case 't': threads = atoi(optarg); break;
		case 'n': n = atoi(optarg); break;
		case 'k': k = atoi(optarg); break;
		case 'r': repeats = atoi(optarg); break;
		case 's': seed = (unsigned)atoi(optarg); break;
		case 'd': demo_sleep = atoi(optarg); break;
		case 'p': print = 1; break;
		default: usage(argv[0]);
		}
	}

	if (mode_name == NULL) usage(argv[0]);
	int parallel;
	if (strcmp(mode_name, "par") == 0) parallel = 1;
	else if (strcmp(mode_name, "seq") == 0) parallel = 0;
	else usage(argv[0]);

	if (n < 1 || k < 1 || threads < 1 || repeats < 1) die("n, k, t and r must be positive");

	size_t size = (size_t)n * n * sizeof(double);
	double *input = malloc(size);
	double *eroded = malloc(size);
	double *dilated = malloc(size);
	if (input == NULL || eroded == NULL || dilated == NULL) die("not enough memory");

	generate(input, n, seed);

	fprintf(stderr, "PID = %d\n", (int)getpid());

	double total = 0.0;
	for (int r = 0; r < repeats; r++) {
		double t0 = now_ms();

		if (parallel) {
			run_par(input, eroded, n, k, ERODE, threads, demo_sleep);
			run_par(input, dilated, n, k, DILATE, threads, demo_sleep);
		} else {
			run_seq(input, eroded, n, k, ERODE);
			run_seq(input, dilated, n, k, DILATE);
		}

		total += now_ms() - t0;
	}

	printf("mode=%s threads=%d n=%d k=%d repeats=%d avg_ms=%.3f\n",
	       mode_name, parallel ? threads : 1, n, k, repeats, total / repeats);

	if (print && n <= 12) {
		print_matrix("input", input, n);
		print_matrix("erosion", eroded, n);
		print_matrix("dilation", dilated, n);
	}

	free(input);
	free(eroded);
	free(dilated);
	return 0;
}
