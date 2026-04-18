/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: boummi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/18 12:00:00 by boummi            #+#    #+#             */
/*   Updated: 2026/04/18 17:21:27 by boummi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	enqueue(t_dongle *dongle, t_thread *data, int use_edf)
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

void	heap_pop(t_dongle *dongle)
{
	dongle->edf_size--;
	dongle->edf_q[0] = dongle->edf_q[dongle->edf_size];
	heap_bubble_down(dongle->edf_q, dongle->edf_size, 0);
}

void	heap_update(t_dongle *dongle, int id, long new_deadline)
{
	int	i;

	i = 0;
	while (i < dongle->edf_size)
	{
		if (dongle->edf_q[i].id == id)
		{
			dongle->edf_q[i].deadline = new_deadline;
			i = heap_bubble_up(dongle->edf_q, i);
			heap_bubble_down(dongle->edf_q, dongle->edf_size, i);
			return ;
		}
		i++;
	}
}

void	heap_remove_by_id(t_dongle *dongle, int id)
{
	int	i;

	i = 0;
	while (i < dongle->edf_size)
	{
		if (dongle->edf_q[i].id == id)
		{
			dongle->edf_size--;
			dongle->edf_q[i] = dongle->edf_q[dongle->edf_size];
			i = heap_bubble_up(dongle->edf_q, i);
			heap_bubble_down(dongle->edf_q, dongle->edf_size, i);
			return ;
		}
		i++;
	}
}
