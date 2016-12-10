#include "./cov_thread.h"

struct list_head task_list_root;
rwlock_t task_list_rwlock;

int task_in_list(struct task_struct *task)
{
	struct cov_thread *ct;

	read_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == task) {
			read_unlock(&task_list_rwlock);
			return 1;
		}
	}
	read_unlock(&task_list_rwlock);
	return 0;
}

int task_is_test_case(struct task_struct *task)
{
	struct cov_thread *ct;

	read_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == task) {
			if (ct->is_test_case) {
				read_unlock(&task_list_rwlock);
				return 1;
			}
		}
	}
	read_unlock(&task_list_rwlock);
	return 0;
}

void cov_thread_init(void)
{
	INIT_LIST_HEAD(&task_list_root);
	rwlock_init(&task_list_rwlock);
}

int cov_thread_add(unsigned long id, int is_test_case)
{
	struct task_struct *task = current;
	struct cov_thread *new;

	if (unlikely(task_in_list(task)))
		return -EEXIST;

	new = kmalloc(sizeof(*new), GFP_KERNEL);
	if (unlikely(!new))
		return -ENOMEM;
	memset(new, 0, sizeof(*new));

	new->task = task;
	new->sample_id = id;
	new->is_test_case = is_test_case;

	new->buffer = kmalloc(THREAD_BUFFER_SIZE, GFP_KERNEL);
	if (unlikely(!new->buffer)) {
		kfree(new);
		return -ENOMEM;
	}
	memset(new->buffer, 0, THREAD_BUFFER_SIZE);

	write_lock(&task_list_rwlock);
	get_task_struct(task);
	list_add_tail(&new->list, &task_list_root);
	write_unlock(&task_list_rwlock);

	return 0;
}

void cov_thread_del(void)
{
	struct task_struct *task = current;
	struct cov_thread *ct;

	write_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == task) {
			list_del(&ct->list);
			put_task_struct(ct->task);
			kfree(ct->buffer);
			kfree(ct);
			break;
		}
	}
	write_unlock(&task_list_rwlock);
}

void cov_thread_exit(void)
{
	struct cov_thread *tmp, *next;

	/*
	 * we should also check the list, to see if any task_struct->usage
	 * is 1, that means the process has exited, userspace forget to call
	 * cov_unregister.
	 */
	write_lock(&task_list_rwlock);
	list_for_each_entry_safe(tmp, next, &task_list_root, list) {
		if (atomic_read(&tmp->task->usage) == 1)
			put_task_struct(tmp->task);
		list_del(&tmp->list);
		kfree(tmp->buffer);
		kfree(tmp);
	}
	INIT_LIST_HEAD(&task_list_root);
	write_unlock(&task_list_rwlock);
}
