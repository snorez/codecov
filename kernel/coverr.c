#include "coverr.h"

struct list_head coverr_root;

void coverr_init(void)
{
	INIT_LIST_HEAD(&coverr_root);
}

static int task_in_err_list(struct task_struct *task)
{
	struct coverr *tmp;
	list_for_each_entry(tmp, &coverr_root, siblings) {
		if (tmp->task == task)
			return 1;
	}
	return 0;
}

int coverr_add(struct task_struct *task)
{
	struct coverr *new;

	if (!task)
		return -EINVAL;

	if (task_in_err_list(task))
		return -EEXIST;

	new = kmalloc(sizeof(struct coverr), GFP_KERNEL);
	if (!new)
		return -ENOMEM;

	new->task = task;
	new->buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!new->buffer) {
		kfree(new);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&new->siblings);
	list_add_tail(&new->siblings, &coverr_root);
	return 0;
}

void coverr_del(struct task_struct *task)
{
	struct coverr *tmp;
	list_for_each_entry(tmp, &coverr_root, siblings) {
		if (tmp->task == task) {
			list_del(&tmp->siblings);
			kfree(tmp->buffer);
			kfree(tmp);
			break;
		}
	}
}

void coverr_exit(void)
{
	struct coverr *tmp, *next;
	list_for_each_entry_safe(tmp, next, &coverr_root, siblings) {
		list_del(&tmp->siblings);
		kfree(tmp->buffer);
		kfree(tmp);
	}
	INIT_LIST_HEAD(&coverr_root);
}
