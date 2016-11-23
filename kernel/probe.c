#include <linux/kprobes.h>
#include "checkpoint.h"
#include "cov_thread.h"
#include "./thread_buffer.h"

/*
 * these probe handler should do this, increase the `hit` field of the
 * checkpoint structure
 */

int cp_default_kp_prehdl(struct kprobe *kp, struct pt_regs *reg)
{
	struct checkpoint *tmp;

	if (!task_in_list(current))
		return 0;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_kprobe == kp) {
			if (!tmp->hit)
				ctbuf_print("NEW PATH: %s\n", tmp->name);
			tmp->hit++;
			if (!tmp->hit)
				tmp->hit = 1;
			break;
		}
	}
	read_unlock(&cproot_rwlock);

	return 0;
}

int cp_default_ret_hdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	if (!task_in_list(current))
		return 0;
	return 0;
}

int cp_default_ret_entryhdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* before the function called. XXX:should always return 0 */
	struct checkpoint *tmp;

	if (!task_in_list(current))
		return 0;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_retprobe == ri->rp) {
			if (!tmp->hit)
				ctbuf_print("NEW PATH: %s\n", tmp->name);
			tmp->hit++;
			if (!tmp->hit)
				tmp->hit = 1;

			/*
			 * TODO:add this checkpoint into cphit_root
			 * and check the parent field to see if this is a NEW PATH
			 */
			break;
		}
	}
	read_unlock(&cproot_rwlock);

	return 0;
}
