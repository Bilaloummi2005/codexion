#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>  

#define RED   "\x1b[31m"
#define GRN   "\x1b[32m"
#define YEL   "\x1b[33m"
#define BLU   "\x1b[34m"
#define RESET "\x1b[0m"


typedef struct s_args
{
    long time_to_burnout;
    long time_to_compile;
    long time_to_debug;
    long time_to_refactor;
    long number_of_compiles_required;
    long dongle_cooldown;
    char* scheduler;
} t_args;

typedef struct s_thread
{
    int id;
    long start_time;
    long last_compile;
    t_args requirements;
    pthread_mutex_t *right_dongle;
    pthread_mutex_t *left_dongle;
} t_thread;

long get_time()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void* routine(void *arg)
{
    t_thread *data = (t_thread *)arg;
    int i = 0;
    int turns = data->requirements.number_of_compiles_required;
    while(i++ < turns){
        if (data->id % 2){
        pthread_mutex_lock(data->right_dongle);
        pthread_mutex_lock(data->left_dongle);
        }else{
        pthread_mutex_lock(data->left_dongle);
        pthread_mutex_lock(data->right_dongle);
        }
        // printf("%d has taken right dongle\n", data->id);
        // printf("%d has taken left dongle\n", data->id);
        printf(GRN "%ld %d is compiling\n" RESET, get_time() - data->start_time, data->id);
        data->last_compile = get_time();
        usleep(1000);

        pthread_mutex_unlock(data->right_dongle);
        pthread_mutex_unlock(data->left_dongle);
        // printf("%d has droped right dongle\n", data->id);
        // printf("%d has droped left dongle\n", data->id);

        printf(RED "%d is debugging\n" RESET, data->id);
        usleep(1000);

        printf(BLU "%d is refactoring\n" RESET, data->id);
        usleep(1000);
        printf(YEL "coder %d turn %d\n" RESET, data->id, i);
        // usleep(500);
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    int n = atoi(argv[1]);
    t_args requirements;
    requirements.time_to_burnout = atoi(argv[2]);
    requirements.time_to_compile = atoi(argv[3]);
    requirements.time_to_debug = atoi(argv[4]);
    requirements.time_to_refactor = atoi(argv[5]);
    requirements.number_of_compiles_required = atoi(argv[6]);
    requirements.dongle_cooldown = atoi(argv[7]);
    requirements.scheduler = argv[8];
    pthread_t t[n];
    t_thread data[n];
    pthread_mutex_t mutex[n];

    for (int i = 0; i < n; i++){
        pthread_mutex_init(&mutex[i], NULL);
    }
    
    for (int i = 0; i < n; i++){
        // printf("thread %d started execution:\n", i);
        data[i].id = i + 1;
        data[i].left_dongle = &mutex[i];
        data[i].right_dongle = &mutex[(i + 1) % n];
        data[i].start_time = get_time();
        data[i].requirements = requirements;
        if (pthread_create(&t[i], NULL, &routine, &data[i])) {
            return 1;
        }
    }

    for (int i = 0; i < n; i++){
        if (pthread_join(t[i], NULL)) {
            return 1;
        }
        printf("thread %d finished it execution\n", i + 1);
    }
    for (int i = 0; i < n; i++){
        pthread_mutex_destroy(&mutex[i]);
    }
    return 0;
}