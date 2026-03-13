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

typedef struct s_simulation {
    int         stop;           // flag: 1 = simulation should end
    pthread_mutex_t stop_mutex; // protects the stop flag
    pthread_mutex_t log_mutex;  // serializes all printf output
} t_simulation;

typedef struct s_dongle
{
    pthread_mutex_t mutex;
    long last_release;
} t_dongle;

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
    int turns;
    long start_time;
    long last_compile;
    t_args requirements;
    t_dongle *right_dongle;
    t_dongle *left_dongle;
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
    int turns = data->turns;
    int time_to_burnout = data->requirements.time_to_burnout;
    int time_to_compile = data->requirements.time_to_compile;
    int time_to_debug = data->requirements.time_to_debug;
    int time_to_refactor = data->requirements.time_to_refactor;
    int dongle_cooldown = data->requirements.dongle_cooldown;
    while(turns--){

        // printf("befor sleep right coldown %ld %ld coder %d\n", get_time() - data->right_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);
        // printf("befor sleep left coldown %ld %ld coder %d\n", get_time() - data->left_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);
        
        // if (get_time() - data->right_dongle->last_release < data->requirements.dongle_cooldown)
        // {
        //     printf("right coldown %ld %d coder %d\n", get_time() - data->right_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);
        //     usleep((data->requirements.dongle_cooldown - (get_time() - data->right_dongle->last_release)) * 1000);
        //     printf("right coldown %ld %d coder %d\n", get_time() - data->right_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);

        // }
        // if (get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown)
        // {
        //     printf("after sleep left coldown %d %d coder %d\n", get_time() - data->left_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);
        //     usleep((data->requirements.dongle_cooldown - (get_time() - data->left_dongle->last_release)) * 1000);
        //     printf("after sleep left coldown %d %d coder %d\n", get_time() - data->left_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);

        // }
        if (data->id % 2){
            // printf("%d %d\n", get_time() - data->right_dongle->last_release, data->requirements.dongle_cooldown);
            pthread_mutex_lock(&data->right_dongle->mutex);
            pthread_mutex_lock(&data->left_dongle->mutex);
        }else{
            // printf("%d %d\n", get_time() - data->right_dongle->last_release, data->requirements.dongle_cooldown);
            pthread_mutex_lock(&data->left_dongle->mutex);
            pthread_mutex_lock(&data->right_dongle->mutex);
        }
        // printf("right coldown %d %d coder %d\n", get_time() - data->right_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);
        // printf("left coldown %d %d coder %d\n", get_time() - data->left_dongle->last_release, get_time() - data->left_dongle->last_release < data->requirements.dongle_cooldown, data->id);
        
        // printf("%d has taken right dongle.mutex\n", data->id);
        // printf("%d has taken left dongle.mutex\n", data->id);
        printf("diff bet last compile and now %ld, time to burnout %d\n", get_time() - data->last_compile, time_to_burnout);
        if (get_time() - data->last_compile > time_to_burnout)
        {
            printf("you failde dumbass\n");
            break;
        }
        printf(GRN "%ld %d is compiling\n" RESET, get_time() - data->start_time, data->id);
        data->last_compile = get_time();
        usleep(time_to_compile*1000);

        // usleep(dongle_cooldown*1000);
        data->right_dongle->last_release = get_time();
        data->left_dongle->last_release = get_time();
        pthread_mutex_unlock(&data->right_dongle->mutex);
        pthread_mutex_unlock(&data->left_dongle->mutex);
        // printf("%d has droped right dongle\n", data->id);
        // printf("%d has droped left dongle\n", data->id);

        printf(RED "%d is debugging\n" RESET, data->id);
        usleep(time_to_debug*1000);

        printf(BLU "%d is refactoring\n" RESET, data->id);
        usleep(time_to_refactor * 1000);
        printf(YEL "coder %d turn %d\n" RESET, data->id, i);
        // usleep(500);
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc < 9)
    {
        fprintf(stderr, "Usage: %s n burnout compile debug refactor turns cooldown scheduler\n", argv[0]);
        return 1;
    }

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
    t_dongle mutex[n];

    for (int i = 0; i < n; i++){
        mutex[i].last_release = get_time() - requirements.dongle_cooldown;
        pthread_mutex_init(&mutex[i].mutex, NULL);
    }
    
    for (int i = 0; i < n; i++){
        // printf("thread %d started execution:\n", i);
        data[i].id = i + 1;
        data[i].turns = requirements.number_of_compiles_required;
        data[i].left_dongle = &mutex[i];
        data[i].right_dongle = &mutex[(i + 1) % n];
        data[i].start_time = get_time();
        data[i].requirements = requirements;
        data[i].last_compile = get_time();
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
        pthread_mutex_destroy(&mutex[i].mutex);
    }
    return 0;
}