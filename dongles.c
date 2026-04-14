#include "codexion.h"

static int	enqueue(t_dongle *dongle, t_thread *data, int use_edf)
{
	if (use_edf)
		heap_push(dongle, data->id, data->deadline);
	else
	{
		dongle->queue[dongle->queue_size] = data->id;
		dongle->queue_size++;
	}
	return (1);
}

static int	is_my_turn(t_dongle *dongle, t_thread *data, int use_edf, long cooldown)
{
	if (use_edf)
	{
		data->deadline = data->last_compile + data->requirements->time_to_burnout;
		heap_update(dongle, data->id, data->deadline);
		if (dongle->edf_q[0].id == data->id
			&& get_time() - dongle->last_release >= cooldown)
			return (1);
	}
	else
	{
		if (dongle->queue[0] == data->id
			&& get_time() - dongle->last_release >= cooldown)
			return (1);
	}
	return (0);
}

static void	wait_on_dongle(t_dongle *dongle)
{
	struct timespec	ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 1000 * 1000;
	if (ts.tv_nsec >= 1000000000)
	{
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}
	pthread_cond_timedwait(&dongle->cond, &dongle->mutex, &ts);
}

static int	wait_for_dongle(t_dongle *dongle, t_thread *data, int use_edf, long cooldown)
{
	while (1)
	{
		if (is_burned_out(data->requirements))
		{
			if (use_edf)
				heap_remove_by_id(dongle, data->id);
			else
				remove_from_queue(dongle);
			pthread_cond_broadcast(&dongle->cond);
			return (0);
		}
		if (is_my_turn(dongle, data, use_edf, cooldown))
			break ;
		wait_on_dongle(dongle);
	}
	if (use_edf)
		heap_pop(dongle);
	else
		remove_from_queue(dongle);
	return (1);
}

static int	acquire_one(t_dongle *dongle, t_thread *data, int use_edf, long cooldown)
{
	pthread_mutex_lock(&dongle->mutex);
	enqueue(dongle, data, use_edf);
	if (!wait_for_dongle(dongle, data, use_edf, cooldown))
	{
		pthread_mutex_unlock(&dongle->mutex);
		return (0);
	}
	log_state(data, "has taken a dongle", YEL);
	return (1);
}

int	lock_dongle(t_dongle *first, t_dongle *second, t_thread *data)
{
	int		use_edf;
	long	cooldown;

	use_edf = (strcmp(data->requirements->scheduler, "edf") == 0);
	cooldown = data->requirements->dongle_cooldown;
	if (!acquire_one(first, data, use_edf, cooldown))
		return (0);
	if (data->requirements->n_coders == 1)
	{
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	if (!acquire_one(second, data, use_edf, cooldown))
	{
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	return (1);
}
