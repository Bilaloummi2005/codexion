#include "codexion.h"


int is_burned_out(t_args *req)
{
    int val;

    pthread_mutex_lock(&req->burned_mutex);
    val = req->burned_out;
    pthread_mutex_unlock(&req->burned_mutex);
    return val;
}

void set_burned_out(t_args *req)
{
    pthread_mutex_lock(&req->burned_mutex);
    req->burned_out = 1;
    pthread_mutex_unlock(&req->burned_mutex);
}

int is_completed(t_args *req)
{
    int val;
    pthread_mutex_lock(&req->burned_mutex);
    val = req->completed;
    pthread_mutex_unlock(&req->burned_mutex);
    return val;
}

void set_completed(t_args *req)
{
    pthread_mutex_lock(&req->burned_mutex);
    req->completed = 1;
    pthread_mutex_unlock(&req->burned_mutex);
}

void realise_dongle(t_dongle *first_dongle, t_dongle *second_dongle){
    first_dongle->last_release = get_time();
    pthread_cond_broadcast(&first_dongle->cond);
    pthread_mutex_unlock(&first_dongle->mutex);

    second_dongle->last_release = get_time();
    pthread_cond_broadcast(&second_dongle->cond);
    pthread_mutex_unlock(&second_dongle->mutex);
}
