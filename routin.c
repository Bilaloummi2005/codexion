
#include "codexion.h"

void* routine(void *arg)
{
    t_thread *data = (t_thread *)arg;
    int time_to_compile = data->requirements->time_to_compile;
    int time_to_debug = data->requirements->time_to_debug;
    int time_to_refactor = data->requirements->time_to_refactor;

    while(!is_burned_out(data->requirements) && !is_completed(data->requirements)){
        // add_to_queue(data);
        if (data->id % 2){
            if (!lock_dongle(data->right_dongle, data->left_dongle, data))
                return 0;
        }else{
            if (!lock_dongle(data->left_dongle, data->right_dongle, data))
                return 0;
        }

        pthread_mutex_lock(&data->state_mutex);
        data->last_compile = get_time();
        pthread_mutex_unlock(&data->state_mutex);

        if(is_burned_out(data->requirements)){
            if (data->id % 2)
                realise_dongle(data->left_dongle, data->right_dongle);
            else
                realise_dongle(data->right_dongle, data->left_dongle);
            return (NULL);
        }

        log_state(data, "is compiling", GRN);
        usleep(time_to_compile*1000);
        
        if (data->id % 2)
            realise_dongle(data->left_dongle, data->right_dongle);
        else
            realise_dongle(data->right_dongle, data->left_dongle);

        pthread_mutex_lock(&data->state_mutex);
        data->compile_count++;
        pthread_mutex_unlock(&data->state_mutex);

        if(is_burned_out(data->requirements))
            return (NULL);

        log_state(data, "is debugging", RED);
        usleep(time_to_debug*1000);
        
        if(is_burned_out(data->requirements))
            return (NULL);

        log_state(data, "is refactoring", BLU);
        usleep(time_to_refactor * 1000);
    }
    return NULL;
}
