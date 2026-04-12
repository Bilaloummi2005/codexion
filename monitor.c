#include "codexion.h"


void* monitor(void *arg){
    t_thread *coders = (t_thread *)arg;   // correct cast
    int n = coders[0].requirements->n_coders;
    long time_to_burnout = coders[0].requirements->time_to_burnout;
    long required = coders[0].requirements->number_of_compiles_required; 
    int done;
    long last_compile;
    int count;
    while (1){
        usleep(1000);
        for (int i=0; i < n; i++){
            pthread_mutex_lock(&coders[i].state_mutex);
            last_compile = coders[i].last_compile;
            pthread_mutex_unlock(&coders[i].state_mutex);
            if (get_time() - last_compile > time_to_burnout){
                set_burned_out(coders[i].requirements);
                log_state(&coders[i], "burned out", "");
                // coders[i].requirements->burned_out = 1;
                return NULL;
            }
        }
        done = 1;
        for (int i=0; i < n; i++){
            pthread_mutex_lock(&coders[i].state_mutex);
            count = coders[i].compile_count;
            pthread_mutex_unlock(&coders[i].state_mutex);
            if (count < required)
                done = 0;
        }
        if (done){
            set_completed(coders[0].requirements);
            // coders[0].requirements->completed = 1;
            return NULL;
        }
    }
}

