#ifndef COV_THREAD_H
#define COV_THREAD_H

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>

/*
 * provide multi-thread support
 * each process gets a coverr, a current call stack root, and something else
 */

#define THREAD_BUFFER_SIZE	PAGE_SIZE*8
struct checkpoint;
struct coverr;
struct cov_thread {
	struct list_head list;
	/*
	 * we COULD NOT just hold a pointer to current process task_struct,
	 * we SHOULD use `get_task_struct` and hold the pointer,
	 * so that the pointer can not be point to another process task_struct,
	 * and then use `put_task_struct` to release the task_struct;
	 */
	struct task_struct *task;
	char *buffer;
	unsigned long sample_id;
	unsigned long prev_addr;	/* for kprobe, not kretprobe */
	int is_test_case;		/* is current process test case */
};
extern struct list_head task_list_root;
extern rwlock_t task_list_rwlock;

extern int task_in_list(struct task_struct *task);
extern int task_is_test_case(struct task_struct *task);
extern void cov_thread_init(void);
extern int cov_thread_add(unsigned long id, int is_test_case);
extern void cov_thread_del(void);
extern void cov_thread_check(void);
extern void cov_thread_exit(void);

#endif
