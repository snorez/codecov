#include "./cov_thread.h"

struct cov_thread threads[COV_THREAD_MAX];

int task_in_list(struct task_struct *task)
{
	struct cov_thread *ct;
	int i = 0;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;

		if (ct->task == task)
			return 1;
	}

	return 0;
}

int task_is_test_case(struct task_struct *task)
{
	struct cov_thread *ct;
	int i = 0;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;

		if (ct->task != task)
			continue;

		return ct->is_test_case;
	}
	return 0;
}

int cov_thread_init(void)
{
	int err = 0, i = 0;
	struct cov_thread *ct;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		ct->buffer = kzalloc(THREAD_BUFFER_SIZE, GFP_KERNEL);
		if (!ct->buffer) {
			err = -ENOMEM;
			break;
		}
	}

	if (err) {
		for (; i > 0; i--) {
			ct = &threads[i-1];
			kfree(ct->buffer);
		}
	}

	return err;
}

int cov_thread_add(unsigned long id, int is_test_case)
{
	struct task_struct *task = current;
	struct cov_thread *ct;
	int i = 0;

	if (unlikely(task_in_list(task)))
		return -EEXIST;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (test_and_set_bit(0,
				(volatile unsigned long *)&(ct->in_use.counter)))
			continue;

		get_task_struct(task);
		ct->task = task;
		ct->sample_id = id;
		ct->is_test_case = is_test_case;
		atomic_set(&ct->is_sample_effective, 0);
		atomic64_set(&ct->prev_addr, 0);
		memset(ct->buffer, 0, THREAD_BUFFER_SIZE);

		return 0;
	}

	return -EBUSY;
}

void cov_thread_del(void)
{
	struct task_struct *task = current;
	struct cov_thread *ct;
	int i = 0;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;

		if (ct->task != task)
			continue;

		put_task_struct(ct->task);
		ct->task = NULL;
		atomic_set(&ct->is_sample_effective, 0);
		atomic64_set(&ct->prev_addr, 0);
		ct->sample_id = 0;
		ct->is_test_case = 0;
		test_and_clear_bit(0,
				(volatile unsigned long *)&(ct->in_use.counter));
	}
}

/*
 * TODO: make sure there is only one process running now
 */
void cov_thread_check(void)
{
	struct cov_thread *ct;
	int i = 0;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;

		if (atomic_read(&ct->task->usage) == 1) {
			put_task_struct(ct->task);
			test_and_clear_bit(0,
				(volatile unsigned long *)&(ct->in_use.counter));
		}
	}
}

int cov_thread_effective(void)
{
	int effective = 0;
	struct cov_thread *ct;
	int i = 0;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;

		if (ct->task != current)
			continue;

		effective = atomic_read(&ct->is_sample_effective);
	}

	return effective;
}

void cov_thread_exit(void)
{
	struct cov_thread *ct;
	int i = 0;

	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		kfree(ct->buffer);
	}
}
