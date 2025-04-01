#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "qdl.h"

static struct qdl_device qdl;

bool qdl_debug;

static void print_usage(void)
{
	extern const char *__progname;
	fprintf(stderr,
		"%s -p <sahara dev_node> -s <id:file path> ...\n",
		__progname);
	fprintf(stderr,
		" -p                   --port                      Sahara device node to use\n"
		" -s <id:file path>    --sahara <id:file path>     Sahara protocol file mapping\n"
		"\n"
		"One or more -s instances are required, -p instance is not required.\n"
		"\n"
		"Example: \n"
		"ks -p /dev/mhi0_QAIC_SAHARA -s 1:/opt/qti-aic/firmware/fw1.bin -s 2:/opt/qti-aic/firmware/fw2.bin\n");
}

int main(int argc, char **argv)
{
	bool found_mapping = false;
	char *dev_node = NULL;
	char *serial = NULL;
	long file_id;
	char *colon;
	int opt;
	int ret;

	static struct option options[] = {
		{"port", required_argument, 0, 'p'},
		{"sahara", required_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "p:s:", options, NULL )) != -1) {
		switch (opt) {
		case 'p':
			dev_node = optarg;
			printf("Using port - %s\n", dev_node);
			break;
		case 's':
			found_mapping = true;
			file_id = strtol(optarg, NULL, 10);
			if (file_id < 0 || file_id >= MAPPING_SZ)
				errx(1, "ID:%ld has to be in range of 0 - %d\n", file_id, MAPPING_SZ - 1);

			colon = strchr(optarg, ':');
			if (!colon)
				errx(1, "Sahara mapping requires ID and file path to be divided by a colon");

			qdl.mappings[file_id] = &optarg[colon - optarg + 1];
			printf("Created mapping ID:%ld File:%s\n", file_id, qdl.mappings[file_id]);
			break;
		default:
			print_usage();
			return 1;
		}
	}

	// -s is required
	if (!found_mapping) {
		print_usage();
		return 1;
	}

	if (dev_node) {
		qdl.fd = open(dev_node, O_RDWR);
		if (qdl.fd < 0)
			errx(1, "Unable to open %s\n", dev_node);
	}
	else {
		ret = qdl_open(&qdl, serial);
		if (ret)
			errx(1, "Failed to find edl device, try using -p\n");
	}


	ret = sahara_run(&qdl, qdl.mappings, false, NULL, NULL);
	if (ret < 0)
		goto out_cleanup;

out_cleanup:
	if (dev_node)
		close(qdl.fd);
	else
		qdl_close(&qdl);

	return !!ret;
}
