/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: boummi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/18 12:00:00 by boummi            #+#    #+#             */
/*   Updated: 2026/04/18 12:00:00 by boummi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	remove_from_queue(t_dongle *dongle)
{
	int	i;

	i = 0;
	while (i < dongle->queue_size - 1)
	{
		dongle->queue[i] = dongle->queue[i + 1];
		i++;
	}
	dongle->queue_size--;
}

void	heap_swap(t_waiter *a, t_waiter *b)
{
	t_waiter	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

int	heap_bubble_up(t_waiter *heap, int index)
{
	int	parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (heap[index].deadline < heap[parent].deadline
			|| (heap[index].deadline == heap[parent].deadline
				&& heap[index].id < heap[parent].id))
		{
			heap_swap(&heap[index], &heap[parent]);
			index = parent;
		}
		else
			break ;
	}
	return (index);
}

void	heap_bubble_down(t_waiter *heap, int size, int index)
{
	int	smallest;
	int	left;
	int	right;

	while (1)
	{
		smallest = index;
		left = 2 * index + 1;
		right = 2 * index + 2;
		if (left < size && (heap[left].deadline < heap[smallest].deadline
				|| (heap[left].deadline == heap[smallest].deadline
					&& heap[left].id < heap[smallest].id)))
			smallest = left;
		if (right < size && (heap[right].deadline < heap[smallest].deadline
				|| (heap[right].deadline == heap[smallest].deadline
					&& heap[right].id < heap[smallest].id)))
			smallest = right;
		if (smallest != index)
		{
			heap_swap(&heap[index], &heap[smallest]);
			index = smallest;
		}
		else
			break ;
	}
}

void	heap_push(t_dongle *dongle, int id, long deadline)
{
	dongle->edf_q[dongle->edf_size].id = id;
	dongle->edf_q[dongle->edf_size].deadline = deadline;
	dongle->edf_size++;
	heap_bubble_up(dongle->edf_q, dongle->edf_size - 1);
}
