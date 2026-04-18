/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routin.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: boummi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/18 12:00:00 by boummi            #+#    #+#             */
/*   Updated: 2026/04/18 12:00:00 by boummi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	acquire_dongles(t_thread *data)
{
	if (data->id % 2)
	{
		if (!lock_dongle(data->right_dongle, data->left_dongle, data))
			return (0);
	}
	else
	{
		usleep(1000);
		if (!lock_dongle(data->left_dongle, data->right_dongle, data))
			return (0);
	}
	return (1);
}

static void	release_dongles(t_thread *data)
{
	if (data->id % 2)
		realise_dongle(data->left_dongle, data->right_dongle);
	else
		realise_dongle(data->right_dongle, data->left_dongle);
}

static int	compile(t_thread *data)
{
	pthread_mutex_lock(&data->state_mutex);
	data->last_compile = get_time();
	pthread_mutex_unlock(&data->state_mutex);
	if (is_burned_out(data->requirements))
	{
		release_dongles(data);
		return (0);
	}
	log_state(data, "is compiling", GRN);
	if (!ft_usleep(data->requirements->time_to_compile, data->requirements))
	{
		release_dongles(data);
		return (0);
	}
	release_dongles(data);
	pthread_mutex_lock(&data->state_mutex);
	data->compile_count++;
	pthread_mutex_unlock(&data->state_mutex);
	return (1);
}

static int	rest(t_thread *data)
{
	if (is_burned_out(data->requirements))
		return (0);
	log_state(data, "is debugging", RED);
	if (!ft_usleep(data->requirements->time_to_debug, data->requirements))
		return (0);
	log_state(data, "is refactoring", BLU);
	if (!ft_usleep(data->requirements->time_to_refactor, data->requirements))
		return (0);
	return (1);
}

void	*routine(void *arg)
{
	t_thread	*data;

	data = (t_thread *)arg;
	while (!is_burned_out(data->requirements)
		&& !is_completed(data->requirements))
	{
		if (!acquire_dongles(data))
			return (NULL);
		if (!compile(data))
			return (NULL);
		if (!rest(data))
			return (NULL);
	}
	return (NULL);
}
