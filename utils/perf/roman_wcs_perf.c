/* Enable POSIX 2008 (clock_gettime) and glibc default extensions (drand48). */
#define _DEFAULT_SOURCE

/*
 * roman_wcs_perf.c
 *
 * Benchmark libasdf-gwcs WCS evaluation on Roman Space Telescope ASDF files.
 *
 * NOTE: Most of this code could be repurposed for more general benchmarking
 * as well; for now the main target was the Roman sample calibration files
 * specifically.
 *
 * Measures three phases per file:
 *   parse_cold  first asdf_open + asdf_get_gwcs + asdf_gwcs_eval_create
 *               (reflects OS page-cache state at invocation time)
 *   parse_hot   same sequence immediately after, page cache definitely warm
 *   eval        asdf_gwcs_eval_2d at each N in {1,10,100,...,1M}, multiple reps
 *
 * Output CSV (long format):
 *   library,file,detector,phase,n_points,rep,time_s
 *
 * Usage:
 *   roman_wcs_perf [-o output.csv] file1.asdf [file2.asdf ...]
 */

#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <asdf.h>
#include <asdf/gwcs/core.h>
#include <asdf/gwcs/eval.h>
#include <asdf/gwcs/wcs.h>

#define IMAGE_NX   4088.0
#define IMAGE_NY   4088.0
#define RAND_SEED  42UL

/* Eval sweep: N values and repetitions per N.
 * Upper bound is the active science area of a Roman WFI detector (4088 x 4088 px;
 * full array is 4096 x 4096 with a 4-pixel reference pixel border on each edge). */
static const size_t N_SWEEP[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 16711744};
static const int    N_REPS[]  = {50, 50, 30, 15, 8, 5, 3, 2, 2};
static const size_t N_N_SWEEP = sizeof(N_SWEEP) / sizeof(N_SWEEP[0]);


static double now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/*
 * Open filepath, deserialize WCS, create eval context.
 * On success writes *file_out and *eval_out and returns 0.
 * On failure returns -1; *file_out and *eval_out are undefined.
 */
static int open_and_load(const char *filepath,
                         asdf_file_t **file_out,
                         asdf_gwcs_eval_t **eval_out) {
    asdf_file_t *file = asdf_open(filepath, "r");
    if (!file)
        return -1;

    asdf_gwcs_t *wcs = NULL;
    if (asdf_get_gwcs(file, "roman/meta/wcs", &wcs) != ASDF_VALUE_OK || !wcs) {
        asdf_close(file);
        return -1;
    }

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, wcs, NULL, &err);
    if (!eval) {
        asdf_close(file);
        return -1;
    }

    *file_out = file;
    *eval_out = eval;
    return 0;
}

/*
 * Read the detector string (e.g. "WFI01") from the open file's ASDF tree
 * and write it lowercased into buf[buflen].  Falls back to "unknown".
 */
static void read_detector(asdf_file_t *file, char *buf, size_t buflen) {
    const char *raw = NULL;
    size_t raw_len = 0;
    if (asdf_get_string(file, "roman/meta/instrument/detector",
                        &raw, &raw_len) == ASDF_VALUE_OK && raw && raw_len > 0) {
        size_t n = raw_len < buflen - 1 ? raw_len : buflen - 1;
        memcpy(buf, raw, n);
        buf[n] = '\0';

        // Normalize the detector name to all lowercase
        for (size_t idx = 0; buf[idx]; idx++)
            if (buf[idx] >= 'A' && buf[idx] <= 'Z')
                buf[idx] += 'a' - 'A';
    } else {
        strncpy(buf, "unknown", buflen - 1);
        buf[buflen - 1] = '\0';
    }
}


/* Evict filepath from the OS page cache via POSIX_FADV_DONTNEED. */
static void evict_file(const char *filepath) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0)
        return;
    struct stat st;
    if (fstat(fd, &st) == 0)
        posix_fadvise(fd, 0, st.st_size, POSIX_FADV_DONTNEED);
    close(fd);
}


/* Benchmark one file. Returns 1 on success, 0 on failure. */
static int bench_file(const char *filepath, FILE *fout,
                      double *xin, double *yin,
                      double *xout, double *yout) {
    char detector[64] = "unknown";

    /* Cold parse--evict first to ensure page cache is cold. */
    evict_file(filepath);
    double t0 = now_s();
    asdf_file_t *file_cold = NULL;
    asdf_gwcs_eval_t *eval_cold = NULL;
    if (open_and_load(filepath, &file_cold, &eval_cold) != 0) {
        fprintf(stderr, "  error: failed to load WCS (cold), skipping\n");
        return 0;
    }
    double cold_s = now_s() - t0;

    /* Read detector from the now-open file (outside timing window). */
    read_detector(file_cold, detector, sizeof(detector));
    asdf_gwcs_eval_destroy(eval_cold);
    asdf_close(file_cold);

    fprintf(fout, "libasdf_gwcs,%s,%s,parse_cold,0,0,%.9f,1\n",
            filepath, detector, cold_s);
    fflush(fout);

    /* Hot parse (page cache warm; keep result for eval) */
    t0 = now_s();
    asdf_file_t *file = NULL;
    asdf_gwcs_eval_t *eval = NULL;
    if (open_and_load(filepath, &file, &eval) != 0) {
        fprintf(stderr, "  error: failed to load WCS (hot), skipping\n");
        return 0;
    }
    double hot_s = now_s() - t0;

    fprintf(fout, "libasdf_gwcs,%s,%s,parse_hot,0,0,%.9f,1\n",
            filepath, detector, hot_s);
    fflush(fout);

    fprintf(stderr, "  parse cold=%.3f ms  hot=%.3f ms\n",
            cold_s * 1e3, hot_s * 1e3);

    /* Eval sweep */
    double *rep_times = malloc(N_REPS[0] * sizeof(double));  /* N_REPS[0] is max */
    if (!rep_times) {
        fprintf(stderr, "  error: out of memory for rep_times\n");
        asdf_gwcs_eval_destroy(eval);
        asdf_close(file);
        return 0;
    }

    for (size_t ni = 0; ni < N_N_SWEEP; ni++) {
        size_t N = N_SWEEP[ni];
        int    reps = N_REPS[ni];

        /* Reproducible random pixel coordinates; regenerate per N level. */
        srand48((long)RAND_SEED);
        for (size_t jdx = 0; jdx < N; jdx++) {
            xin[jdx] = drand48() * IMAGE_NX;
            yin[jdx] = drand48() * IMAGE_NY;
        }

        for (int rep = 0; rep < reps; rep++) {
            t0 = now_s();
            asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, N);
            rep_times[rep] = now_s() - t0;

            fprintf(fout, "libasdf_gwcs,%s,%s,eval,%zu,%d,%.9f,1\n",
                    filepath, detector, N, rep, rep_times[rep]);
        }
        fflush(fout);

        /* Sort rep_times in-place to find median. */
        for (int a = 0; a < reps - 1; a++)
            for (int b = a + 1; b < reps; b++)
                if (rep_times[b] < rep_times[a]) {
                    double tmp = rep_times[a];
                    rep_times[a] = rep_times[b];
                    rep_times[b] = tmp;
                }
        double median = rep_times[reps / 2];
        fprintf(stderr, "  N=%-8zu  median=%.3f ms  (%.0f px/s)\n",
                N, median * 1e3, (double)N / median);
    }

    free(rep_times);
    asdf_gwcs_eval_destroy(eval);
    asdf_close(file);
    return 1;
}


int main(int argc, char *argv[]) {
    const char *outpath = "c_perf_results.csv";
    int first_file_arg = 1;

    if (argc >= 3 && strcmp(argv[1], "-o") == 0) {
        outpath = argv[2];
        first_file_arg = 3;
    }

    if (first_file_arg >= argc) {
        fprintf(stderr,
                "Usage: %s [-o output.csv] file1.asdf [file2.asdf ...]\n",
                argv[0]);
        return 1;
    }

    FILE *fout = fopen(outpath, "w");
    if (!fout) {
        fprintf(stderr, "error: cannot open %s for writing\n", outpath);
        return 1;
    }
    fprintf(fout, "library,file,detector,phase,n_points,rep,time_s,blas_threads\n");

    size_t max_n = N_SWEEP[N_N_SWEEP - 1];
    double *xin  = malloc(max_n * sizeof(double));
    double *yin  = malloc(max_n * sizeof(double));
    double *xout = malloc(max_n * sizeof(double));
    double *yout = malloc(max_n * sizeof(double));
    if (!xin || !yin || !xout || !yout) {
        fprintf(stderr, "error: out of memory allocating coordinate arrays\n");
        fclose(fout);
        return 1;
    }

    int n_success = 0;
    int nfiles = argc - first_file_arg;

    for (int idx = first_file_arg; idx < argc; idx++) {
        const char *filepath = argv[idx];
        fprintf(stderr, "[%d/%d] %s\n",
                idx - first_file_arg + 1, nfiles, filepath);
        if (bench_file(filepath, fout, xin, yin, xout, yout))
            n_success++;
    }

    free(xin); free(yin); free(xout); free(yout);
    fclose(fout);

    fprintf(stderr, "\nSummary: %d/%d files succeeded\n", n_success, nfiles);
    fprintf(stderr, "Output: %s\n", outpath);
    return (n_success == 0) ? 1 : 0;
}
