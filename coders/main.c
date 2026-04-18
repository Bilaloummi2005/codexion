/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: boummi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/18 12:00:00 by boummi            #+#    #+#             */
/*   Updated: 2026/04/18 12:00:00 by boummi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	launch_threads(pthread_t *threads, t_thread *coders, t_args *req)
{
	int	i;

	i = 0;
	while (i < req->n_coders)
	{
		if (pthread_create(&threads[i], NULL, &routine, &coders[i]))
			return (0);
		i++;
	}
	return (1);
}

static void	join_threads(pthread_t *threads, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		pthread_join(threads[i], NULL);
		i++;
	}
}

static int	run(t_args *req)
{
	pthread_t	*threads;
	t_thread	*coders;
	t_dongle	*dongles;
	pthread_t	mon;
	int			i;

	threads = malloc(sizeof(pthread_t) * req->n_coders);
	coders = malloc(sizeof(t_thread) * req->n_coders);
	dongles = malloc(sizeof(t_dongle) * req->n_coders);
	if (!threads || !coders || !dongles || !init_dongles(dongles,
			req->n_coders, req->dongle_cooldown))
		return (free(threads), free(coders), free(dongles), 1);
	i = -1;
	while (++i < req->n_coders)
		init_coder(&coders[i], i, req, dongles);
	if (!launch_threads(threads, coders, req) || pthread_create(&mon, NULL,
			&monitor, coders) || pthread_join(mon, NULL))
		return (cleanup(dongles, req->n_coders), free(threads), free(coders),
			free(dongles), 1);
	join_threads(threads, req->n_coders);
	cleanup(dongles, req->n_coders);
	return (free(threads), free(coders), free(dongles), 0);
}

int	main(int argc, char *argv[])
{
	t_args	req;

	if (!parse_args(&req, argc, argv))
		return (1);
	return (run(&req));
}
