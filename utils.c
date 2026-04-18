/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: boummi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/18 12:00:00 by boummi            #+#    #+#             */
/*   Updated: 2026/04/18 12:00:00 by boummi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long	get_time(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int	ft_usleep(long ms, t_args *req)
{
	long	end;

	end = get_time() + ms;
	while (get_time() < end)
	{
		if (is_burned_out(req))
			return (0);
		usleep(1000);
	}
	return (1);
}

void	log_state(t_thread *data, char *msg, char *clr)
{
	long	time;

	pthread_mutex_lock(&data->requirements->log_mutex);
	if (strcmp(msg, "burned out") && is_burned_out(data->requirements))
	{
		pthread_mutex_unlock(&data->requirements->log_mutex);
		return ;
	}
	time = get_time() - data->start_time;
	printf("%s%ld %d %s\n" RESET, clr, time, data->id, msg);
	pthread_mutex_unlock(&data->requirements->log_mutex);
}
