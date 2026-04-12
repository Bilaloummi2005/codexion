#include "codexion.h"

int lock_dongle(t_dongle *first_dongle, t_dongle *second_dongle, t_thread *data)
{
    struct timespec ts;
    int dongle_cooldown = data->requirements->dongle_cooldown;
    int use_edf = (strcmp(data->requirements->scheduler, "edf") == 0);

    pthread_mutex_lock(&first_dongle->mutex);

    if (use_edf) {
        heap_push(first_dongle, data->id, data->deadline);
    } else {
        first_dongle->queue[first_dongle->queue_size] = data->id;
        first_dongle->queue_size++;
    }

    while (1) {
        if (is_burned_out(data->requirements)) {
            if (use_edf) {
                heap_remove_by_id(first_dongle, data->id);  // when leaving due to burnout
                pthread_cond_broadcast(&first_dongle->cond);
            } else {
                remove_from_queue(first_dongle);
                pthread_cond_broadcast(&first_dongle->cond);
            }
            pthread_mutex_unlock(&first_dongle->mutex);
            return 0;
        }
        if (use_edf) {
            data->deadline = data->last_compile + data->requirements->time_to_burnout;
            heap_update(first_dongle, data->id, data->deadline);
            if (first_dongle->edf_q[0].id == data->id &&
                get_time() - first_dongle->last_release >= dongle_cooldown)
                break;
        } else {
            if (first_dongle->queue[0] == data->id &&
                get_time() - first_dongle->last_release >= dongle_cooldown)
                break;
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 1000 * 1000;
        if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
        pthread_cond_timedwait(&first_dongle->cond, &first_dongle->mutex, &ts);
    }

    if (use_edf){
        heap_pop(first_dongle);  // when you're the winner (you're at root)
    }else
        remove_from_queue(first_dongle);

    log_state(data, "has taken a dongle", YEL);

    if (data->requirements->n_coders == 1){
        pthread_mutex_unlock(&first_dongle->mutex);
        return 0;
    }

    pthread_mutex_lock(&second_dongle->mutex);

    if (use_edf) {
        heap_push(second_dongle, data->id, data->deadline);
    } else {
        second_dongle->queue[second_dongle->queue_size] = data->id;
        second_dongle->queue_size++;
    }

    while (1) {
        if (is_burned_out(data->requirements)) {
            if (use_edf) {
                heap_remove_by_id(second_dongle, data->id);  // when leaving due to burnout
                pthread_cond_broadcast(&second_dongle->cond);
            } else {
                remove_from_queue(second_dongle);
                pthread_cond_broadcast(&second_dongle->cond);
            }
            pthread_mutex_unlock(&first_dongle->mutex);
            pthread_mutex_unlock(&second_dongle->mutex);
            return 0;
        }
        if (use_edf) {
            data->deadline = data->last_compile + data->requirements->time_to_burnout;
            heap_update(second_dongle, data->id, data->deadline);
            if (second_dongle->edf_q[0].id == data->id &&
                get_time() - second_dongle->last_release >= dongle_cooldown)
                break;
        } else {
            if (second_dongle->queue[0] == data->id &&
                get_time() - second_dongle->last_release >= dongle_cooldown)
                break;
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 1000 * 1000;
        if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
        pthread_cond_timedwait(&second_dongle->cond, &second_dongle->mutex, &ts);
    }

    if (use_edf){
        heap_pop(second_dongle);  // when you're the winner (you're at root)
    }else
        remove_from_queue(second_dongle);

    log_state(data, "has taken a dongle", YEL);
    return 1;
}

void realise_dongle(t_dongle *first_dongle, t_dongle *second_dongle){
    first_dongle->last_release = get_time();
    pthread_cond_broadcast(&first_dongle->cond);
    pthread_mutex_unlock(&first_dongle->mutex);

    second_dongle->last_release = get_time();
    pthread_cond_broadcast(&second_dongle->cond);
    pthread_mutex_unlock(&second_dongle->mutex);
}
