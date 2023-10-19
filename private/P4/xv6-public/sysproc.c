#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "psched.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;

  myproc()->stillsleep = n;
  
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      // keep checking if killed , sleep fail
      // myproc()->stillsleep = 0;
      release(&tickslock);
      return -1;
    }
    //myproc()->stillsleep = n - (ticks - ticks0);
    // normally sleep
    sleep(&ticks, &tickslock);
  }

  // myproc()->stillsleep = 0;
  
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_nice(void) {
  // need to implement
  int n;
  if(argint(0, &n) < 0)
    return -1;
  
  return nice(n);
}

int sys_getschedstate(void) {
  // need to implement
    struct pschedinfo *psi;
    if(argptr(0, (void*)&psi, sizeof(*psi)) < 0 || psi == 0)
        return -1;

    return getschedstate(psi);
}

