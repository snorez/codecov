#include "../userlib/codecov.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int err;
	err = cov_register();
	if (err == -1) {
		printf("register failed\n");
		return -1;
	}

	unsigned long num_hit = 1, num_total = 1;
	errno = 0;
	err = get_numhit(&num_hit);
	if (err == -1) {
		printf("get num_hit failed: %s\n", strerror(errno));
		return -1;
	}

	err = get_numtotal(&num_total);
	if (err == -1) {
		printf("get num_total failed: %s\n", strerror(errno));
		return -1;
	}

	printf("%ld, %ld\n", num_hit, num_total);

	cov_unregister();
	return 0;
}
