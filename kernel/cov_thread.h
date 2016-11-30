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
	struct task_struct *task;
	char *buffer;
	unsigned long sample_id;
};
extern struct list_head task_list_root;
extern rwlock_t task_list_rwlock;

extern int task_in_list(struct task_struct *task);
extern void cov_thread_init(void);
extern int cov_thread_add(unsigned long id);
extern void cov_thread_del(void);
extern void cov_thread_exit(void);

#endif
