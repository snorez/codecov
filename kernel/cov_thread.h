#ifndef COV_THREAD_H
#define COV_THREAD_H

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/types.h>

/*
 * provide multi-thread support
 * each process gets a coverr, a current call stack root, and something else
 */

#define THREAD_BUFFER_SIZE	PAGE_SIZE*0x10
struct checkpoint;
struct coverr;
#define COV_THREAD_MAX	64
struct cov_thread {
	/*
	 * we COULD NOT just hold a pointer to current process task_struct,
	 * we SHOULD use `get_task_struct` and hold the pointer,
	 * so that the pointer can not be point to another process task_struct,
	 * and then use `put_task_struct` to release the task_struct;
	 */
	struct task_struct *task;
	char *buffer;
	atomic64_t prev_addr;		/* for kprobe, not kretprobe */

	unsigned long sample_id;
	unsigned long is_test_case;	/* is current process test case */

	/*
	 * the following two fields should use test_and_set_bit atomic_read
	 */
	atomic_t in_use;
	atomic_t is_sample_effective;	/* current process has NEW PATH */
};
extern struct cov_thread threads[];

extern int task_in_list(struct task_struct *task);
extern int task_is_test_case(struct task_struct *task);
extern int cov_thread_init(void);
extern int cov_thread_add(unsigned long id, int is_test_case);
extern void cov_thread_del(void);
extern void cov_thread_check(void);
extern int cov_thread_effective(void);
extern void cov_thread_exit(void);

#endif
