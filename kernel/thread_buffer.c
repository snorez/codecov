#include "./thread_buffer.h"

int ctbuf_print(const char *fmt, ...)
{
	struct task_struct *task = current;
	struct cov_thread *ct;
	va_list args;
	int i = 0;

	va_start(args, fmt);
	write_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == task) {
			size_t left;
			char *pos = ct->buffer + strlen(ct->buffer);
			if ((pos != ct->buffer) && (*(pos-1) != '\n'))
				*pos++ = '\n';
			left = THREAD_BUFFER_SIZE - strlen(ct->buffer);

			i = vsnprintf(pos, left, fmt, args);
			if ((i == 0) || (i < left))
				break;
			/* maybe left is not enough for fmt */
			memset(ct->buffer, 0, THREAD_BUFFER_SIZE);
			i = vsnprintf(ct->buffer, THREAD_BUFFER_SIZE, fmt, args);
		}
	}
	write_unlock(&task_list_rwlock);
	va_end(args);
	return i;
}

int ctbuf_get(char __user *buf, size_t len)
{
	struct task_struct *task = current;
	struct cov_thread *ct;
	int err = -ESRCH;

	write_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == task) {
			size_t len_ct = strlen(ct->buffer);
			len = (len < len_ct) ? len : len_ct;
			err = 0;
			if (copy_to_user(buf, ct->buffer, len))
				err = -EFAULT;
			memset(ct->buffer, 0, THREAD_BUFFER_SIZE);
			break;
		}
	}
	write_unlock(&task_list_rwlock);

	return err;
}
