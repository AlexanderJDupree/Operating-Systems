// Example for testing part of CS333 P2.
// Refactored by Alexander DuPree
#ifdef CS333_P2
#include "types.h"
#include "user.h"

static int
uidTest(uint nval)
{
  uint uid = getuid();

  printf(1, "\nCurrent UID is: %d\n", uid);
  printf(1, "Setting UID to %d\n", nval);

  if (setuid(nval) < 0  || (uid = getuid()) != nval)
  {
    printf(2, "SETUID FAILURE: uid: %d != nval:%d\n", uid, nval);
    return -1; // UID FAIL
  }

  printf(1, "Current UID is: %d\n", uid);
  printf(1, "\nConfirm with CTRL-P:\n");
  sleep(5 * TPS);  // now type control-p

  return 0; // UID SUCCESS
}

static int
gidTest(uint nval)
{
  uint gid = getgid();
  printf(1, "\nCurrent GID is: %d\n", gid);
  printf(1, "Setting GID to %d\n", nval);

  if (setgid(nval) < 0 || (gid = getgid()) != nval)
  {
    printf(2, "SETGID FAILURE: gid: %d != nval:%d\n", gid, nval);
    return -1; // GID FAIL
  }

  printf(1, "Current GID is: %d\n", gid);
  printf(1, "\nConfirm with CTRL-P:\n");
  sleep(5 * TPS);  // now type control-p

  return 0; // GID SUCCESS
}

static int
forkTest(uint nval)
{
  int pid;
  int puid;
  int pgid;
  int ppid = getpid();

  printf(1, "\nSetting UID to %d and GID to %d before fork(). Value"
                  " should be inherited\n", nval, nval + 1);

  if ((puid = setuid(nval)) < 0)
    printf(2, "FORKTEST FAILURE: Invalid UID: %d\n", nval);
  if ((pgid = setgid(nval+1)) < 0)
    printf(2, "FORKTEST FAILURE: Invalid UID: %d\n", nval);

  printf(1, "Before fork(), UID = %d, GID = %d\n", getuid(), getgid());

  pid = fork();
  if (pid == 0) {  // child

    int uid  = getuid();
    int gid  = getgid();
    int ppid_ = getppid();

    printf(1, "Child: UID is: %d, GID is: %d\n", uid, gid);

    // Assert childs uid/gid is the same as the parents
    if(uid != puid)
    {
      printf(2, "FAILURE: Child UID: %d != Parents UID: %d\n", uid, puid);
    }
    if(gid != pgid)
    {
      printf(2, "FAILURE: Child GID: %d != Parents GID: %d\n", gid, pgid);
    }
    if(ppid_ != ppid)
    {
      printf(2, "FAILURE: Child PPID: %d != Parents PID: %d\n", ppid_, ppid);
    }
    printf(1, "\nConfirm with CTRL-P:\n");
    sleep(5 * TPS);  // now type control-p
    exit();
    return 0; // Not reached
  }
  else
  {
    pid = wait();
    return 0; // FORK SUCCESS
  }
}

static int
invalidTest(uint nval)
{
  int status = 0;
  printf(1, "\nSetting UID to %d. This test should FAIL\n", nval);
  if (setuid(nval) < 0)
  {
    printf(1, "SUCCESS! The setuid system call indicated failure\n");
  }
  else
  {
    printf(2, "FAILURE! The setuid system call indicates success\n");
    status = -1;
  }

  printf(1, "\nSetting GID to %d. This test should FAIL\n", nval);
  if (setgid(nval) < 0)
  {
    printf(1, "SUCCESS! The setgid system call indicated failure\n");
  }
  else
  {
    printf(2, "FAILURE! The setgid system call indicates success\n");
    status = -1;
  }

  printf(1, "\nSetting UID to %d. This test should FAIL\n", -1);
  if (setuid(-1) < 0)
  {
    printf(1, "SUCCESS! The setuid system call indicated failure\n");
  }
  else
  {
    printf(2, "FAILURE! The setgid system call indicates success\n");
    status = -1;
  }

  printf(1, "\nSetting GID to %d. This test should FAIL\n", -1);
  if (setgid(-1) < 0)
  {
    printf(1, "SUCCESS! The setuid system call indicated failure\n");
  }
  else
  {
    printf(2, "FAILURE! The setgid system call indicates success\n");
    status = -1;
  }
  return status;
}

static int
testuidgid(void)
{
  uint success = 1;

  // get/set uid test
  if(uidTest(100) < 0) { success = 0; }

  // get/set gid test
  if(gidTest(200) < 0) { success = 0; };

  // getppid test
  printf(1, "\nMy parent process is: %d\n", getppid());

  // fork tests to demonstrate UID/GID inheritance
  if(forkTest(111) < 0) { success = 0; };

  // tests for invalid values for uid and gid
  // UID_MAX and GID_MAX is 32767
  if(invalidTest(32768) < 0) { success = 0; };

  if(success)
  {
    printf(1, "** TEST UID/GID: All Tests Pass! **\n");
  }
  else
  {
    printf(1, "** TEST UID/GID: Failure! Check log for details. **\n");
  }

  return success;
}

int
main() 
{
  testuidgid();
  exit();
}
#endif // CS333_P2
