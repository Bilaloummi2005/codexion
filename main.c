#include "codexion.h"


int main(int argc, char* argv[]) {
    if (argc < 9)
    {
        fprintf(stderr, "Usage: %s n burnout compile debug refactor turns cooldown scheduler\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    t_args requirements;
    requirements.n_coders = n;
    requirements.time_to_burnout = atoi(argv[2]);
    requirements.time_to_compile = atoi(argv[3]);
    requirements.time_to_debug = atoi(argv[4]);
    requirements.time_to_refactor = atoi(argv[5]);
    requirements.number_of_compiles_required = atoi(argv[6]);
    requirements.dongle_cooldown = atoi(argv[7]);
    requirements.scheduler = argv[8];
    requirements.burned_out = 0;
    requirements.completed = 0;
    pthread_mutex_init(&requirements.log_mutex, NULL);
    pthread_mutex_init(&requirements.burned_mutex, NULL);
    pthread_t t[n];
    t_thread data[n];
    t_dongle mutex[n];
    pthread_t m;

    for (int i = 0; i < n; i++){
        mutex[i].last_release = get_time() - requirements.dongle_cooldown;
        mutex[i].queue_size = 0;
        mutex[i].edf_size = 0;
        mutex[i].edf_q = malloc(2 * sizeof(t_waiter));
        mutex[i].queue = malloc(2 * sizeof(int));
        pthread_cond_init(&mutex[i].cond, NULL);
        pthread_mutex_init(&mutex[i].mutex, NULL);
    }
    

    for (int i = 0; i < n; i++){
        // printf("thread %d started execution:\n", i);
        pthread_mutex_init(&data[i].state_mutex, NULL);
        data[i].requirements = &requirements;
        data[i].last_compile = get_time();
        data[i].deadline = data[i].last_compile + data[i].requirements->time_to_burnout;
        data[i].id = i + 1;
        data[i].compile_count = 0;
        data[i].left_dongle = &mutex[i];
        data[i].right_dongle = &mutex[(i + 1) % n];
        data[i].start_time = get_time();
        if (pthread_create(&t[i], NULL, &routine, &data[i])) 
            return 1;
    }
    if (pthread_create(&m, NULL, &monitor, &data))
        return 1;
    if (pthread_join(m, NULL)) 
        return 1;

    for (int i = 0; i < n; i++){
        if (pthread_join(t[i], NULL)) {
            return 1;
        }
        // printf("thread %d finished it execution\n", i + 1);
    }
    for (int i = 0; i < n; i++){
        pthread_mutex_destroy(&mutex[i].mutex);
        free(mutex[i].queue);
        free(mutex[i].edf_q);
        pthread_cond_destroy(&mutex[i].cond);
    }
    return 0;
}