#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <float.h>

#include "float_vec.h"
#include "barrier.h"
#include "utils.h"


floats*
sample(float* data, long size, int P)
{
    int samps_count = 3*(P-1);
    float samps[samps_count];

    for (int i = 0; i < samps_count; i++) 
    {
        samps[i] = data[rand_int(size)];
    }

    qsort(&samps, samps_count, sizeof(float), compare_floats);

    floats* samples = make_floats(0);
    floats_push(samples, 0.0f);
    for (int i = 0; i < samps_count; i=i+3) 
    {
        floats_push(samples, samps[i+1]);
    }
    floats_push(samples, FLT_MAX);

    return samples;
}

void
sort_worker(int pnum, float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    float range_min = samps->data[pnum];
    float range_max = samps->data[pnum + 1];

    floats* xs = make_floats(0);
    for (int i = 0; i < size; i++) 
    {
        if (data[i] >= range_min && data[i] < range_max)
        {
            floats_push(xs, data[i]);
        }
    }
    sizes[pnum] = xs->size;

    printf("%d: start %.04f, count %ld\n", pnum, samps->data[pnum], xs->size);

    // TODO: some other stuff

    qsort_floats(xs);

    barrier_wait(bb);

    int start_write = 0;
    for (int i = 0; i < pnum; i++)
    {
        start_write = start_write + sizes[i];
    }

    for (int i = 0; i < sizes[pnum]; i++)
    {
        data[start_write + i] = xs->data[i];
    }



     free_floats(xs);
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    pid_t kids[P];

    for (int ii = 0; ii < P; ++ii) {
        if((kids[ii] = fork()))
        {
            // parent
            int rv = waitpid(kids[ii], 0, WNOHANG);
            check_rv(rv);

        } else {
            // child
            sort_worker(ii, data, size, P, samps, sizes, bb);
            exit(0);
        }
    }

    // TODO: spawn P processes, each running sort_worker

    // for (int ii = 0; ii < P; ++ii) {
    //     //int rv = waitpid(kids[ii], 0, 0);
    //     //check_rv(rv);
    // }
}

void
sample_sort(float* data, long size, int P, long* sizes, barrier* bb)
{
    floats* samps = sample(data, size, P);
    run_sort_workers(data, size, P, samps, sizes, bb);
    free_floats(samps);
}

int
main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage:\n");
        printf("\t%s P data.dat\n", argv[0]);
        return 1;
    }

    const int P = atoi(argv[1]);
    const char* fname = argv[2];

    seed_rng();

    int rv;
    struct stat st;
    rv = stat(fname, &st);
    check_rv(rv);

    const int fsize = st.st_size;
    if (fsize < 8) {
        printf("File too small.\n");
        return 1;
    }

    int fd = open(fname, O_RDWR);
    check_rv(fd);

    void* file = mmap(0, fsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); // TODO: load the file with mmap.

    // TODO: These should probably be from the input file.
    long count = *(long*)file;
    printf("count = %ld\n", count);

    float *data = (float*)file + sizeof(long)/sizeof(float);

    long sizes_bytes = P * sizeof(long);
    long* sizes = mmap(0, sizes_bytes, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0); // TODO: This should be shared

    barrier* bb = make_barrier(P);

    sample_sort(data, count, P, sizes, bb);

    free_barrier(bb);

    // TODO: munmap your mmaps

    return 0;
}

