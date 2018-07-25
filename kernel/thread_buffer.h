#ifndef THREAD_BUFFER_H
#define THREAD_BUFFER_H

#include <stdarg.h>
#include <linux/uaccess.h>
#include "./cov_thread.h"

#define	FUNC_IN_STR	">>>"
#define	FUNC_OUT_STR	"<<<"
#define	INFUNC_IN_STR	"-->"
struct buffer_user {
	char __user *buffer;
	size_t len;
};

extern int ctbuf_print(const char *fmt, ...);
extern int ctbuf_get(char __user *buf, size_t len);

#endif
