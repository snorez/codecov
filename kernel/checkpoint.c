#include "checkpoint.h"

struct list_head cproot;	/* all checkpoint we have probed */

void checkpoint_init(void)
{
	INIT_LIST_HEAD(&cproot);
}

static int name_in_cplist(char *name)
{
	struct checkpoint *tmp;

	list_for_each_entry(tmp, &cproot, siblings) {
		if ((strlen(name) == strlen(tmp->name)) &&
			!strcmp(tmp->name, name)) {
			return 1;
		}
	}

	return 0;
}

int (*lookup_symbol_attrs_f)(unsigned long, unsigned long *,
			     unsigned long *, char *, char *);
void (*insn_init_f)(struct insn *, const void *, int, int);
void (*insn_get_length_f)(struct insn *);
static int do_checkpoint_add(char *name, char *func, unsigned long offset,
			     unsigned long level);

static void do_set_jxx_probe(char *func, unsigned long base,
			     struct insn *insn, int opcs, unsigned long level)
{
	void *addr = (char *)insn->kaddr;
	void *next_addr = (char *)addr + insn->length;
	void *addr_jmp;
	char name_tmp[KSYM_NAME_LEN];

	/*
	 * TODO: where this instruction jump to?
	 * if @opcs == 1, then Jbs
	 * if @opcs == 2, then Jvds, four bytes
	 * calculate the next address and the jump address
	 */
	signed int offset;

	if (opcs == 1) {
		offset = (signed int)(insn->immediate.bytes[0]);
		addr_jmp = (void *)((unsigned long)next_addr + offset);
	} else if (opcs == 2) {
		switch (insn->immediate.nbytes) {
		case 1:
			offset = (signed int)insn->immediate.bytes[0];
			break;
		case 2:
			offset = *(signed short *)(insn->immediate.bytes);
			break;
		case 4:
			offset = *(signed int *)(insn->immediate.bytes);
			break;
		default:
			offset = 0;
		}
		addr_jmp = (void *)((unsigned long)next_addr + offset);
	} else
		return;

	if ((next_addr <= (void *)base) || (addr_jmp <= (void *)base))
		return;

	/* now we have two addresses: next_addr, addr_jmp */
	memset(name_tmp, 0, KSYM_NAME_LEN);
	snprintf(name_tmp, KSYM_NAME_LEN, "%s#%ld", func,
			(unsigned long)next_addr-base);
	do_checkpoint_add(name_tmp, func, (unsigned long)next_addr-base, level);
	memset(name_tmp, 0, KSYM_NAME_LEN);
	snprintf(name_tmp, KSYM_NAME_LEN, "%s#%ld", func,
			(unsigned long)addr_jmp-base);
	do_checkpoint_add(name_tmp, func, (unsigned long)addr_jmp-base, level);
	return;
}

/*
 * do_auto_add: parse the @func, and add checkpoint automatic
 * @func: the function need to parse
 * we try our best to add more checkpoints
 * as of now, only support X86_64
 */
static void do_auto_add(char *func, unsigned long level)
{
#ifdef AUTO_ADD
#ifdef CONFIG_X86_64
	char name[KSYM_NAME_LEN], modname[KSYM_NAME_LEN];
	unsigned long addr, size, offset, i = 0;
	struct insn insn;
	int err;

	addr = (unsigned long)kallsyms_lookup_name(func);
	if (unlikely(!addr))
		return;

	if (unlikely(!lookup_symbol_attrs_f))
		lookup_symbol_attrs_f = (void *)kallsyms_lookup_name(
						"lookup_symbol_attrs");
	if (unlikely(!lookup_symbol_attrs_f))
		return;
	if (unlikely(!insn_init_f))
		insn_init_f = (void *)kallsyms_lookup_name("insn_init");
	if (unlikely(!insn_init_f))
		return;
	if (unlikely(!insn_get_length_f))
		insn_get_length_f = (void *)kallsyms_lookup_name(
						"insn_get_length");
	if (unlikely(!insn_get_length_f))
		return;

	err = lookup_symbol_attrs_f(addr, &size, &offset, modname, name);
	if (err)
		return;

	while (i < size) {
		unsigned char opcode_bytes, opc0, opc1;

		insn_init_f(&insn, (void *)(addr+i), MAX_INSN_SIZE, 1);
		insn_get_length_f(&insn);
		if (!insn.length)
			break;

		opcode_bytes = insn.opcode.nbytes;
		opc0 = insn.opcode.bytes[0];
		opc1 = insn.opcode.bytes[1];

		if ((opcode_bytes == 1) && ((opc0 == JCXZ_OPC) ||
					((opc0 <= JG_OPC0) && (opc0 >= JO_OPC0))))
			do_set_jxx_probe(func, (unsigned long)addr, &insn, 1,
					 level);
		else if ((opcode_bytes == 2) && (opc0 == TWO_OPC) &&
				((opc1 <= JG_OPC1) && (opc1 >= JO_OPC1)))
			do_set_jxx_probe(func, (unsigned long)addr, &insn, 2,
					 level);
		i += insn.length;
	}
#endif
#endif
	return;
}

/*
 * alloc a new `checkpoint` and initialised with the given arguments
 * @name: the name of this checkpoint, if use offset, please name it [name]_offxxxx
 * @function: the name of the function will be probed
 * @offset: the offset of the function will be probed
 *
 * we should check if the name is already exist in the cproot;
 */
static int do_checkpoint_add(char *name, char *func, unsigned long offset,
			     unsigned long level)
{
	int err;
	struct checkpoint *new;
	size_t name_len;

	if (name_in_cplist(name))
		return -EEXIST;

	new = kzalloc(sizeof(struct checkpoint), GFP_KERNEL);
	if (unlikely(!new))
		return -ENOMEM;

	err = -EINVAL;
	name_len = strlen(name);
	if (unlikely(name_len > NAME_LEN_MAX))
		goto err_free;
	err = -ENOMEM;
	new->name = kmalloc(name_len+1, GFP_KERNEL);
	if (unlikely(!new->name))
		goto err_free;
	memset(new->name, 0, name_len+1);
	memcpy(new->name, name, name_len);

	/*
	 * now deal with the probe.
	 * If the offset not zero, we alloc a kprobe,
	 * otherwise, we alloc a kretprobe
	 */
	if (unlikely(offset)) {
		new->this_kprobe = kmalloc(sizeof(struct kprobe), GFP_KERNEL);
		if (unlikely(!new->this_kprobe))
			goto err_free2;
		memset(new->this_kprobe, 0, sizeof(struct kprobe));

		new->this_kprobe->pre_handler = cp_default_kp_prehdl;
		new->this_kprobe->symbol_name = func;
		new->this_kprobe->offset = offset;

		if ((err = register_kprobe(new->this_kprobe)))
			goto err_free3;
	} else {
		new->this_retprobe = kmalloc(sizeof(struct kretprobe), GFP_KERNEL);
		if (unlikely(!new->this_retprobe))
			goto err_free2;
		memset(new->this_retprobe, 0, sizeof(struct kretprobe));

		new->this_retprobe->entry_handler = cp_default_ret_entryhdl;
		new->this_retprobe->maxactive = RETPROBE_MAXACTIVE;
		new->this_retprobe->handler = cp_default_ret_hdl;
		new->this_retprobe->kp.addr =
				(kprobe_opcode_t *)kallsyms_lookup_name(func);

		if ((err = register_kretprobe(new->this_retprobe)))
			goto err_free3;
	}

	new->level = level;
	atomic_set(&new->hit, 0);
	atomic_set(&new->enabled, 1);

	list_add_tail(&new->siblings, &cproot);
	return 0;

err_free3:
	kfree(new->this_retprobe);
	kfree(new->this_kprobe);
err_free2:
	kfree(new->name);
err_free:
	kfree(new);
	return err;
}

int checkpoint_add(char *name, char *func, unsigned long offset,
		   unsigned long level, int _auto)
{
	int err = do_checkpoint_add(name, func, offset, level);
	if (!err && !offset && _auto)
		do_auto_add(func, level);
	return err;
}

int checkpoint_xstate(unsigned long name, unsigned long len, unsigned long enable,
		      unsigned long subpath)
{
	char cp_name[KSYM_NAME_LEN];
	int (*func_kp)(struct kprobe *kp);
	int (*func_kretp)(struct kretprobe *rp);
	struct checkpoint *cp;
	int err = 0;

	if (name) {
		if (len > KSYM_NAME_LEN)
			return -EINVAL;

		memset(cp_name, 0, KSYM_NAME_LEN);
		if (copy_from_user(cp_name, (char __user *)name, len))
			return -EFAULT;
	}

	if (enable) {
		func_kp = enable_kprobe;
		func_kretp = enable_kretprobe;
	} else {
		func_kp = disable_kprobe;
		func_kretp = disable_kretprobe;
	}

	if (name) {
		list_for_each_entry(cp, &cproot, siblings) {
			if (((strlen(cp->name) == strlen(cp_name)) &&
				(!strncmp(cp->name, cp_name, strlen(cp_name)))) ||
			    (subpath &&
			     (!strncmp(cp->name, cp_name, strlen(cp_name))) &&
			     (strlen(cp->name) > strlen(cp_name)) &&
			     (*(cp->name + strlen(cp_name)) == '#'))) {
				if (cp->this_kprobe)
					err = func_kp(cp->this_kprobe);
				else if (cp->this_retprobe)
					err = func_kretp(cp->this_retprobe);
				if (!err)
					atomic_set(&cp->enabled, !!enable);
				else
					break;
			}
		}
	} else {
		list_for_each_entry(cp, &cproot, siblings) {
			if (cp->this_kprobe)
				err = func_kp(cp->this_kprobe);
			else if (cp->this_retprobe)
				err = func_kretp(cp->this_retprobe);

			if (err)
				break;
			else
				atomic_set(&cp->enabled, !!enable);
		}
	}
	return err;
}

static void checkpoint_caller_cleanup(struct checkpoint *cp)
{
	int i = 0;
	struct checkpoint_caller *tmp;

	for (i = 0; i < CP_CALLER_MAX; i++) {
		tmp = &cp->caller[i];
		test_and_clear_bit(0,
				(volatile unsigned long *)&(tmp->in_use.counter));
	}
}

void checkpoint_del(char *name)
{
	struct checkpoint *tmp;

	/*
	 * XXX:here we do not need list_for_each_entry_safe,
	 * because when we find the target checkpoint, we jump out the loop
	 */
	list_for_each_entry(tmp, &cproot, siblings) {
		if ((strlen(name) == strlen(tmp->name)) &&
			!strcmp(tmp->name, name)) {
			list_del(&tmp->siblings);
			checkpoint_caller_cleanup(tmp);

			if (unlikely(tmp->this_kprobe)) {
				unregister_kprobe(tmp->this_kprobe);
				kfree(tmp->this_kprobe);
			} else {
				unregister_kretprobe(tmp->this_retprobe);
				kfree(tmp->this_retprobe);
			}
			kfree(tmp->name);
			kfree(tmp);
			break;
		}
	}
}

void checkpoint_restart(void)
{
	struct checkpoint *tmp, *next;

	list_for_each_entry_safe(tmp, next, &cproot, siblings) {
		list_del(&tmp->siblings);
		checkpoint_caller_cleanup(tmp);
		if (unlikely(tmp->this_kprobe)) {
			unregister_kprobe(tmp->this_kprobe);
			kfree(tmp->this_kprobe);
		} else {
			unregister_kretprobe(tmp->this_retprobe);
			kfree(tmp->this_retprobe);
		}
		kfree(tmp->name);
		kfree(tmp);
	}
	INIT_LIST_HEAD(&cproot);
}

/* this function actually returns the number of functions been hit */
unsigned long checkpoint_get_numhit(void)
{
	unsigned long num = 0;
	struct checkpoint *tmp;

	list_for_each_entry(tmp, &cproot, siblings) {
		if (atomic_read(&tmp->hit))
			num++;
	}

	return num;
}

unsigned long checkpoint_count(void)
{
	unsigned long num = 0;
	struct checkpoint *tmp;

	list_for_each_entry(tmp, &cproot, siblings)
		num++;

	return num;
}

unsigned long path_count(void)
{
	unsigned long num = 0;
	struct checkpoint *tmp;
	struct checkpoint_caller *tmp_caller;
	int i = 0;

	list_for_each_entry(tmp, &cproot, siblings) {
		for (i = 0; i < CP_CALLER_MAX; i++) {
			tmp_caller = &tmp->caller[i];
			if (atomic_read(&tmp_caller->in_use))
				num++;
		}
	}

	return num;
}

unsigned long get_cp_status(char *name, enum status_opt option)
{
	unsigned long ret = -1;
	struct checkpoint *cp;

	list_for_each_entry(cp, &cproot, siblings) {
		if ((strlen(cp->name) == strlen(name)) &&
			(!strncmp(cp->name, name, strlen(name)))) {
			switch (option) {
			case STATUS_HIT:
				ret = atomic_read(&cp->hit);
				break;
			case STATUS_LEVEL:
				ret = cp->level;
				break;
			case STATUS_ENABLED:
				ret = atomic_read(&cp->enabled);
				break;
			default:
				ret = -1;
			}
			break;
		}
	}
	return ret;
}

static unsigned long has_level(struct checkpoint *cp, unsigned long level)
{
	if ((level > 64) || (level < 1))
		return 0;
	return cp->level & (1UL<<(level-1));
}

/*
 * get_next_unhit_func, get_next_unhit_cp, get_path_map
 * SHOULD check if current process is the only process running now.
 * and hold the task_list_rwlock until the functions return
 */
int get_next_unhit_func(char __user *buf, size_t len, size_t skip,
			unsigned long level)
{
	int err, num = 0, found = 0;
	size_t name_len;
	struct cov_thread *ct;
	struct checkpoint *cp;
	int i = 0;

	err = -EBUSY;
	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;
		num++;
		if (num > 1)
			goto out;
	}

	err = -ESRCH;
	if (!num)
		goto out;

	list_for_each_entry(cp, &cproot, siblings) {
		if ((!atomic_read(&cp->hit)) && (!strchr(cp->name, '#'))) {
			if (!has_level(cp, level))
				continue;
			if (!skip) {
				found = 1;
				break;
			}
			skip--;
		}
	}

	err = -ENOENT;
	if (!found)
		goto out;

	err = -EINVAL;
	name_len = strlen(cp->name) + 1;
	if (len < name_len)
		goto out;

	err = 0;
	if (copy_to_user(buf, cp->name, name_len))
		err = -EFAULT;

out:
	return err;
}

int get_next_unhit_cp(char __user *buf, size_t len, size_t skip,
		      unsigned long level)
{
	int err, num = 0, found = 0;
	size_t name_len;
	struct cov_thread *ct;
	struct checkpoint *cp;
	int i = 0;

	err = -EBUSY;
	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;

		num++;
		if (num > 1)
			goto out;
	}

	err = -ESRCH;
	if (!num)
		goto out;

	list_for_each_entry(cp, &cproot, siblings) {
		if (!atomic_read(&cp->hit)) {
			if (!has_level(cp, level))
				continue;
			if (!skip) {
				found = 1;
				break;
			}
			skip--;
		}
	}
	err = -ENOENT;
	if (!found)
		goto out;

	err = -EINVAL;
	name_len = strlen(cp->name) + 1;
	if (len < name_len)
		goto out;

	err = 0;
	if (copy_to_user(buf, cp->name, name_len))
		err = -EFAULT;

out:
	return err;
}

/*
 * if *len is not enough, we return -ERETY, userspace should check the new len
 */
int get_path_map(char __user *buf, size_t __user *len)
{
	size_t len_need = 0, len_tmp, copid = 0;
	int err, num = 0;
	struct cov_thread *ct;
	struct checkpoint *cp;
	char *buf_tmp = NULL;
	int i = 0;

	err = -EBUSY;
	for (i = 0; i < COV_THREAD_MAX; i++) {
		ct = &threads[i];

		if (!atomic_read(&ct->in_use))
			continue;

		num++;
		if (num > 1)
			goto out;
	}

	err = -ESRCH;
	if (!num)
		goto out;

	err = -EFAULT;
	if (copy_from_user(&len_tmp, len, sizeof(size_t)))
		goto out;

	/* use +: as the seperator, cp->name+caller_name+caller_addr+:NEXT */
	list_for_each_entry(cp, &cproot, siblings) {
		struct checkpoint_caller *cpcaller;
		int j = 0;

		if (cp->name)
			len_need += strlen(cp->name);
		else
			len_need += strlen("NULL");

		for (j = 0; j < CP_CALLER_MAX; j++) {
			cpcaller = &cp->caller[j];

			if (!atomic_read(&cpcaller->in_use))
				continue;

			len_need += 1;
			if (cpcaller->name[0])
				len_need += strlen(cpcaller->name);
			else
				len_need += strlen("NULL");
			len_need += 16 + 2;	/* for 0x */
		}
		len_need += 1;	/* for : */
	}
	len_need += 1;	/* for \0 */

	err = -EFAULT;
	if (copy_to_user(len, &len_need, sizeof(size_t)))
		goto out;
	err = -EAGAIN;
	if (len_tmp < len_need)
		goto out;

	err = -ENOMEM;
	buf_tmp = kzalloc(len_need, GFP_KERNEL);
	if (!buf_tmp)
		goto out;

	list_for_each_entry(cp, &cproot, siblings) {
		struct checkpoint_caller *cpcaller;
		int j = 0;

		if (cp->name) {
			memcpy(buf_tmp+copid, cp->name, strlen(cp->name));
			copid += strlen(cp->name);
		} else {
			memcpy(buf_tmp,"NULL", 4);
			copid += 4;
		}

		for (j = 0; j < CP_CALLER_MAX; j++) {
			cpcaller = &cp->caller[j];

			if (!atomic_read(&cpcaller->in_use))
				continue;

			memcpy(buf_tmp+copid, "+", 1);
			copid += 1;

			if (cpcaller->name[0]) {
				memcpy(buf_tmp+copid, cpcaller->name,
						strlen(cpcaller->name));
				copid += strlen(cpcaller->name);
				sprintf(buf_tmp+copid, "(0x%016lx)",
					atomic64_read(&cpcaller->address));
				copid += 16 + 4;
			} else {
				memcpy(buf_tmp+copid, "NULL", 4);
				copid += 4;
			}
		}

		memcpy(buf_tmp+copid, ":", 1);
		copid += 1;
	}

	err = 0;
	if (copy_to_user(buf, buf_tmp, copid+1))
		err = -EFAULT;

out:
	kfree(buf_tmp);
	return err;
}

void checkpoint_exit(void)
{
	checkpoint_restart();
}
