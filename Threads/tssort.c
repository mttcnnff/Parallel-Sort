#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <float.h>
#include <sys/types.h>

#include "float_vec.h"
#include "barrier.h"
#include "utils.h"

typedef struct sort_worker_struct {
    int pnum;
    floats* input;
    const char* output;
    int P;
    floats* samps;
    long* sizes;
    barrier* bb;
} sort_args;

floats*
sample(floats* input, int P)
{
    int samps_count = 3*(P-1);
    float samps[samps_count];

    for (int i = 0; i < samps_count; i++) 
    {
        samps[i] = input->data[rand_int(input->size)];
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

void*
sort_worker(sort_args* args)
{
    int rv;
    float range_min = args->samps->data[args->pnum];
    float range_max = args->samps->data[args->pnum + 1];

    floats* xs = make_floats(0);

    for(int i = 0; i < args->input->size; i++)
    {
        if(args->input->data[i] >= range_min && args->input->data[i] < range_max)
        {
            floats_push(xs, args->input->data[i]);
        }
    }
    args->sizes[args->pnum] = xs->size;

    printf("%d: start %.04f, count %ld\n", args->pnum, args->samps->data[args->pnum], args->sizes[args->pnum]);

    qsort_floats(xs);

    barrier_wait(args->bb);
    
    int offset = 0;
    for(int i = 0; i < args->pnum; i++)
    {
        offset = offset + args->sizes[i];
    }

    int ofd = open(args->output, O_WRONLY, 0644);
    check_rv(ofd);

    rv = lseek(ofd, sizeof(long) + (offset*sizeof(float)), SEEK_SET);
    check_rv(rv);

    write(ofd, xs->data, (xs->size*sizeof(float)));

    rv = close(ofd);
    check_rv(rv);
    free_floats(xs);
    return 0;
}

void
run_sort_workers(floats* input, const char* output, int P, floats* samps, long* sizes, barrier* bb)
{
    pthread_t threads[P];
    sort_args args[P];
    int rv;

    // TODO: Spawn P threads running sort_worker
    for (int i = 0; i < P; i++) {

        args[i].pnum = i;
        args[i].bb = bb;
        args[i].input = input;
        args[i].output = output;
        args[i].P = P;
        args[i].samps = samps;
        args[i].sizes = sizes;

        rv = pthread_create(&(threads[i]), 0, (void*)sort_worker, &(args[i]));
        assert(rv == 0);
    }

    for (int i = 0; i < P; i++) {
        rv = pthread_join(threads[i], 0);
        check_rv(rv);
    }

    return;
}

void
sample_sort(floats* input, const char* output, int P, long* sizes, barrier* bb)
{
    floats* samps = sample(input, P);
    run_sort_workers(input, output, P, samps, sizes, bb);
    free_floats(samps);
}

int
main(int argc, char* argv[])
{
    alarm(120);
    int rv;

    if (argc != 4) {
        printf("Usage:\n");
        printf("\t%s P input.dat output.dat\n", argv[0]);
        return 1;
    }

    const int P = atoi(argv[1]);
    const char* iname = argv[2];
    const char* oname = argv[3];

    seed_rng();

    int input_fd = open(iname, O_RDONLY, 0644);
    check_rv(input_fd);

    long count;
    rv = read(input_fd, &count, sizeof(long));
    check_rv(rv);

    rv = lseek(input_fd, sizeof(long), SEEK_SET);
    check_rv(rv);

    
    float* fs = malloc(count * sizeof(float));
    rv = read(input_fd, fs, count * sizeof(float));
    check_rv(rv);

    floats* input = make_floats(0);
    for (int i = 0; i < count; i++)
    {
        floats_push(input, fs[i]);
    }

    // printf("Count = %ld\n", count);

    // floats_print(input);

    int ofd = open(oname, O_CREAT|O_TRUNC|O_WRONLY, 0644);
    check_rv(ofd);
    rv = ftruncate(ofd, sizeof(count) + (count*sizeof(float)));
    check_rv(rv);

    write(ofd, &count, sizeof(count));

    rv = close(ofd);
    check_rv(rv);

    barrier* bb = make_barrier(P);

    long* sizes = malloc(P * sizeof(long));
    sample_sort(input, oname, P, sizes, bb);

    free(sizes);

    free_barrier(bb);
    free_floats(input);

    return 0;
}

