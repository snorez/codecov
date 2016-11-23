#include "./codecov.h"

/*
 * this file do these things:
 *	* provide filesystem interfaces, could use ioctl
 *	* some initialize things
 */

static long cov_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long res;

	switch (cmd) {
	case COV_COUNT_HIT:
		res = checkpoint_get_numhit();
		return copy_to_user((unsigned long __user *)arg, &res,
				    sizeof(unsigned long));

	case COV_COUNT_CP:
		res = checkpoint_count();
		return copy_to_user((unsigned long __user *)arg, &res,
				    sizeof(unsigned long));

	case COV_ADD_CP: {
		struct checkpoint_user new;
		char *name, *func;

		if (copy_from_user(&new, (unsigned long __user *)arg, sizeof(new)))
			return -EFAULT;
		if ((new.name_len > NAME_LEN_MAX) || (new.func_len > FUNC_LEN_MAX))
			return -EINVAL;

		name = kmalloc(new.name_len+1, GFP_KERNEL);
		if (!name)
			return -ENOMEM;
		memset(name, 0, new.name_len+1);
		if (copy_from_user(name, (unsigned long __user *)new.name,
				   new.name_len)) {
			kfree(name);
			return -EFAULT;
		}

		func = kmalloc(new.func_len+1, GFP_KERNEL);
		if (!func) {
			kfree(name);
			return -ENOMEM;
		}
		memset(func, 0, new.func_len+1);
		if (copy_from_user(func, (unsigned long __user *)new.func,
				   new.func_len)) {
			kfree(name);
			kfree(func);
			return -EFAULT;
		}

		res = checkpoint_add(name, func, new.offset);
		kfree(name);
		kfree(func);
		return res;
	}

	case COV_DEL_CP: {
		struct checkpoint_user new;
		char *name;

		if (copy_from_user(&new, (unsigned long __user *)arg, sizeof(new)))
			return -EFAULT;
		if (new.name_len > NAME_LEN_MAX)
			return -EINVAL;

		name = kmalloc(new.name_len+1, GFP_KERNEL);
		if (!name)
			return -ENOMEM;
		memset(name, 0, new.name_len+1);
		if (copy_from_user(name, (unsigned long __user *)new.name,
				   new.name_len)) {
			kfree(name);
			return -EFAULT;
		}

		checkpoint_del(name);
		kfree(name);
		return 0;
	}

	case COV_RESTART_CP:
		checkpoint_restart();
		return 0;

	case COV_REGISTER:
		return cov_thread_add();

	case COV_UNREGISTER:
		cov_thread_del();
		return 0;

	case COV_GET_BUFFER: {
		struct buffer_user bu;
		if (copy_from_user(&bu, (void __user *)arg, sizeof(bu)))
			return -EFAULT;
		return ctbuf_get(bu.buffer, bu.len);
	}

	default:
		return -ENOTSUPP;
	}
	return 0;
}

static struct dentry *cov_entry;
static struct file_operations cov_ops = {
	.owner = THIS_MODULE,
	/* XXX: since when the ioctl changed to unlocked_ioctl */
	.unlocked_ioctl = cov_ioctl,
};

static int __init codecov_init(void)
{
	/* TODO */
	cov_entry = debugfs_create_file("codecov_entry", S_IRWXU | S_IROTH, NULL,
					NULL, &cov_ops);
	if (!cov_entry)
		return -1;

	cov_thread_init();
	checkpoint_init();

	return 0;
}

static void __exit codecov_exit(void)
{
	/* TODO */
	debugfs_remove(cov_entry);
	cov_thread_exit();
	checkpoint_exit();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zerons");
module_init(codecov_init);
module_exit(codecov_exit);
