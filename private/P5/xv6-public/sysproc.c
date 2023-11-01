#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "mmap.h"

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
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
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

void* sys_mmap(void) {
  // todo
  // 1. ANON | FIXED | SHARED
  int addr;
  int length, prot, flags, fd;
  int offset;
  void* addrmap;

  // Extract arguments
  if(argint(0, &addr) < 0)
    return (void*)-1;
  if(argint(1, &length) < 0)
    return (void*)-1;
  if(argint(2, &prot) < 0)
    return (void*)-1;
  if(argint(3, &flags) < 0)
    return (void*)-1;
  if(argint(4, &fd) < 0)
    return (void*)-1;
  if(argint(5, &offset) < 0)
    return (void*)-1;

  // temp 0 
  addrmap = (void *)mmap((void*)addr, length, prot, flags, fd, offset);
  if(addrmap == (void*)-1)
    return (void*)-1;
  myproc()->maparray[0].addr = addrmap;
  myproc()->maparray[0].flag = flags;
  myproc()->maparray[0].len = length;
  myproc()->maparray[0].prot = prot;

  // implementation in proc.c
  return addrmap;
}

int sys_munmap(void) {
  int addr;
  int length;

  // temp solution, only use array[0]
  myproc()->maparray[0].addr = (void*)0;
  myproc()->maparray[0].flag = 0;
  myproc()->maparray[0].len = 0;
  myproc()->maparray[0].prot = 0;

  // extract args
  if(argint(0, &addr) < 0)
    return -1;
  if(argint(1, &length) < 0)
    return -1;


  length = PGROUNDUP(length);

  // Unmap pages and free memory
  if(deallocuvm(myproc()->pgdir, (uint)addr + length, (uint)addr) == 0){
    return -1;
  }

  return 0;
}

