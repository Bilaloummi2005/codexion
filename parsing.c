/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: boummi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/18 12:00:00 by boummi            #+#    #+#             */
/*   Updated: 2026/04/18 12:00:00 by boummi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	parse(const char *str, long *out)
{
	long	result;
	int		index;

	if (!str || !*str || str[0] == '-')
		return (0);
	result = 0;
	index = 0;
	while (str[index])
	{
		if (str[index] < '0' || str[index] > '9')
			return (0);
		result = result * 10 + (str[index] - '0');
		if (result > 2147483647)
			return (0);
		index++;
	}
	*out = result;
	return (1);
}

static int	parse_values(t_args *req, char **argv)
{
	if (!parse(argv[1], &req->n_coders)
		|| !parse(argv[2], &req->time_to_burnout)
		|| !parse(argv[3], &req->time_to_compile)
		|| !parse(argv[4], &req->time_to_debug)
		|| !parse(argv[5], &req->time_to_refactor)
		|| !parse(argv[6], &req->number_of_compiles_required)
		|| !parse(argv[7], &req->dongle_cooldown)){
		printf("Error: invalid numeric argument(s). Expected positive integers.\n");
		return (0);
	}
	return (1);
}

static int	is_valid_config(t_args *req, char *scheduler)
{
	if (req->n_coders < 1 || req->time_to_burnout <= 0
		|| req->time_to_compile <= 0 || req->time_to_debug <= 0
		|| req->time_to_refactor <= 0 || req->number_of_compiles_required < 1
		|| req->dongle_cooldown < 0){
		printf("Error: configuration values out of range.\n");
		return (0);
	}
	if (strcmp(scheduler, "edf") && strcmp(scheduler, "fifo"))
		return (0);
	return (1);
}

int	parse_args(t_args *req, int argc, char **argv)
{
	if (argc != 9 || !parse_values(req, argv) || !is_valid_config(req, argv[8]))
		return (0);
	req->scheduler = argv[8];
	req->burned_out = 0;
	req->completed = 0;
	pthread_mutex_init(&req->log_mutex, NULL);
	pthread_mutex_init(&req->burned_mutex, NULL);
	return (1);
}
