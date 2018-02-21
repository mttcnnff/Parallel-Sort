// Author: Nat Tuck
// CS3650 starter code

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

void
seed_rng()
{
    struct timeval tv;
    gettimeofday(&tv, 0);

    long pid = getpid();
    long sec = tv.tv_sec;
    long usc = tv.tv_usec;

    srandom(pid ^ sec ^ usc);
}

void
check_rv(int rv)
{
    if (rv == -1) {
        perror("oops");
        fflush(stdout);
        fflush(stderr);
        abort();
    }
}

int
rand_int(long max)
{
    int r = random();
    float q = (float)r/RAND_MAX; 
    return (int)(q*max);
}

int
compare_floats(const void* a, const void* b)
{
    float *aa = (float*) a;
    float *bb = (float*) b;

    if (*aa < *bb)
    {
        return -1;
    }
    else if (*aa > *bb)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
