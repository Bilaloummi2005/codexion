#include "codexion.h"

static int	check_burnout(t_thread *coders, int n, long time_to_burnout)
{
	int		i;
	long	last_compile;

	i = 0;
	while (i < n)
	{
		pthread_mutex_lock(&coders[i].state_mutex);
		last_compile = coders[i].last_compile;
		pthread_mutex_unlock(&coders[i].state_mutex);
		if (get_time() - last_compile > time_to_burnout)
		{
			set_burned_out(coders[i].requirements);
			log_state(&coders[i], "burned out", "");
			return (1);
		}
		i++;
	}
	return (0);
}

static int	check_completed(t_thread *coders, int n, long required)
{
	int	i;
	int	count;

	i = 0;
	while (i < n)
	{
		pthread_mutex_lock(&coders[i].state_mutex);
		count = coders[i].compile_count;
		pthread_mutex_unlock(&coders[i].state_mutex);
		if (count < required)
			return (0);
		i++;
	}
	return (1);
}

void	*monitor(void *arg)
{
	t_thread	*coders;
	int			n;
	long		time_to_burnout;
	long		required;

	coders = (t_thread *)arg;
	n = coders[0].requirements->n_coders;
	time_to_burnout = coders[0].requirements->time_to_burnout;
	required = coders[0].requirements->number_of_compiles_required;
	while (1)
	{
		usleep(1000);
		if (check_burnout(coders, n, time_to_burnout))
			return (NULL);
		if (check_completed(coders, n, required))
		{
			set_completed(coders[0].requirements);
			return (NULL);
		}
	}
}
