#include <linux/kprobes.h>
#include "checkpoint.h"

/*
 * these probe handler should lessly do this, increase the `hit` field of the
 * checkpoint structure
 */

int cp_default_kp_prehdl(struct kprobe *kp, struct pt_regs *reg)
{
	struct checkpoint *tmp;
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_kprobe == kp) {
			if (!tmp->hit)
				pr_info("NEW PATH: %s\n", tmp->name);
			tmp->hit++;
			if (!tmp->hit)
				tmp->hit = 1;
			break;
		}
	}
	return 0;
}

int cp_default_ret_hdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* after the function return */
	return 0;
}

int cp_default_ret_entryhdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* before the function called. XXX:should always return 0 */
	struct checkpoint *tmp;
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_retprobe == ri->rp) {
			if (!tmp->hit)
				pr_info("NEW PATH: %s\n", tmp->name);
			tmp->hit++;
			if (!tmp->hit)
				tmp->hit = 1;
			break;
		}
	}
	return 0;
}
