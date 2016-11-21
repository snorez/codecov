#include <linux/kprobes.h>
#include "checkpoint.h"

extern struct list_head cproot;
extern struct list_head cphit_root;
extern rwlock_t cproot_rwlock;
extern rwlock_t cphit_root_rwlock;

/*
 * these probe handler should do this, increase the `hit` field of the
 * checkpoint structure
 */

int cp_default_kp_prehdl(struct kprobe *kp, struct pt_regs *reg)
{
	struct checkpoint *tmp;

	read_lock(&cproot_rwlock);
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
	read_unlock(&cproot_rwlock);

	return 0;
}

static struct checkpoint *cphit_current(void)
{
	struct checkpoint *ret;

	read_lock(&cphit_root_rwlock);
	if (list_empty(&cphit_root))
		ret = NULL;
	else
		ret = ((struct cphit *)(cphit_root.prev))->cp;
	read_unlock(&cphit_root_rwlock);

	return ret;
}

int cp_default_ret_hdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct checkpoint *current_cp = NULL;
	struct cphit *tmp = NULL;

	read_lock(&cproot_rwlock);
	list_for_each_entry(current_cp, &cproot, siblings) {
		if (current_cp->this_retprobe == ri->rp) {
			write_lock(&cphit_root_rwlock);
			/* XXX: we should reverse the list */
			list_for_each_entry_reverse(tmp, &cphit_root, list) {
				if (tmp->cp == current_cp)
					break;
			}
			if (tmp) {
				list_del(&tmp->list);
				kfree(tmp);
			} else {
				/* tmp should never be NULL */
				pr_err("not in cphit_root list\n");
			}
			write_unlock(&cphit_root_rwlock);
		}
	}
	read_unlock(&cproot_rwlock);

	return 0;
}

int cp_default_ret_entryhdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* before the function called. XXX:should always return 0 */
	struct checkpoint *tmp;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_retprobe == ri->rp) {
			if (!tmp->hit)
				pr_info("NEW PATH: %s\n", tmp->name);
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
