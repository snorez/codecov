#include "../userlib/codecov.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int err;
	err = cov_register();
	if (err == -1) {
		printf("register err\n");
		return -1;
	}

	unsigned long num_hit, num_total;
	err = get_numhit(&num_hit);
	if (err == -1) {
		perror("get_numhit err");
		return -1;
	}
	err = get_numtotal(&num_total);
	if (err == -1) {
		perror("get_numtotal err");
		return -1;
	}
	printf("before add...\n");
	printf("num_hit: %ld, num_total: %ld\n", num_hit, num_total);

	err = checkpoint_add("ovl_mount", "ovl_mount", 0);
	if (err == -1) {
		perror("checkpoint_add err");
		return -1;
	}
	err = checkpoint_add("ovl_mount_v1", "ovl_mount_v1", 0);
	if (err == -1) {
		perror("checkpoint_add err");
		return -1;
	}

	printf("after add, before del...\n");
	getchar();

	err = get_numhit(&num_hit);
	if (err == -1) {
		perror("get_numhit err");
		return -1;
	}
	err = get_numtotal(&num_total);
	if (err == -1) {
		perror("get_numtotal err");
		return -1;
	}
	printf("num_hit: %ld, num_total: %ld\n", num_hit, num_total);

	err = checkpoint_restart();
	if (err == -1) {
		perror("checkpoint_del err");
		return -1;
	}

	err = get_numhit(&num_hit);
	if (err == -1) {
		perror("get_numhit err");
		return -1;
	}
	err = get_numtotal(&num_total);
	if (err == -1) {
		perror("get_numtotal err");
		return -1;
	}
	printf("after del...\n");
	printf("num_hit: %ld, num_total: %ld\n", num_hit, num_total);

	cov_unregister();
	return 0;
}
