#include "codexion.h"

long get_time()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void log_state(t_thread *data, char *msg, char *clr)
{
    pthread_mutex_lock(&data->requirements->log_mutex);
    if (strcmp(msg, "burned out") && is_burned_out(data->requirements))
    {
        usleep(1000);
        pthread_mutex_unlock(&data->requirements->log_mutex);
        return;
    }
    long t = get_time() - data->start_time;
    printf("%s%ld %d %s\n" RESET, clr, t, data->id, msg);
    pthread_mutex_unlock(&data->requirements->log_mutex);
}

t_waiter heap_pop(t_dongle *dongle)
{
    t_waiter top = dongle->edf_q[0];
    dongle->edf_size--;
    dongle->edf_q[0] = dongle->edf_q[dongle->edf_size];
    heap_bubble_down(dongle->edf_q, dongle->edf_size, 0);
    return top;
}
void heap_update(t_dongle *dongle, int id, long new_deadline)
{
    int i;

    i = 0;
    while (i < dongle->edf_size)
    {
        if (dongle->edf_q[i].id == id)
        {
            dongle->edf_q[i].deadline = new_deadline;
            i = heap_bubble_up(dongle->edf_q, i);
            heap_bubble_down(dongle->edf_q, dongle->edf_size, i);
            return;
        }
        i++;
    }
}

void heap_remove_by_id(t_dongle *dongle, int id)
{
    int i;

    i = 0;
    while (i < dongle->edf_size)
    {
        if (dongle->edf_q[i].id == id)
        {
            dongle->edf_size--;
            dongle->edf_q[i] = dongle->edf_q[dongle->edf_size];
            i = heap_bubble_up(dongle->edf_q, i);
            heap_bubble_down(dongle->edf_q, dongle->edf_size, i);
            return;
        }
        i++;
    }
}
