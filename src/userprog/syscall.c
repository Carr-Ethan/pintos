#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "userprog/process.h"


static void syscall_handler (struct intr_frame *);
static void syscall_exit (int status);
static void validate_ptr (const void *vaddr);
static void validate_buffer (const void *buff, unsigned size);
static int syscall_write(int fd, const void *buff, unsigned size);

static void validate_ptr(const void *vaddr){
  if (vaddr == NULL || !is_user_vaddr(vaddr)){
    syscall_exit(-1);
  }

  if (pagedir_get_page(thread_current()->pagedir, vaddr) == NULL){
    syscall_exit(-1);
  }
}

static void validate_buff(const void *buff, unsigned size){
  const char *ptr = (const char *) buff;
  for (unsigned i = 0; i < size; i++){
    validate_ptr(ptr + i);
  }
}

static void syscall_exit(int status){
  struct thread *cur = thread_current();
  cur->exit_status = status;
  thread_exit();
}

static int syscall_write(int fd, const void *buff, unsigned size){
  if(fd != 1){
    return -1;
  }

  putbuf(buff, size);
  return size;
}

static void syscall_handler(struct intr_frame *f){
  validate_ptr(f->esp);
  uint32_t *esp = (uint32_t *)f->esp;

  int syscall_no = (int)esp[0];

  switch(syscall_no){
    case SYS_HALT:
      shutdown_power_off();
      break;

    case SYS_EXIT:
      validate_ptr(&esp[1]);
      syscall_exit((int)esp[1]);
      break;
    case SYS_WRITE:
      {
        validate_ptr(&esp[1]);
        validate_ptr(&esp[2]);
        validate_ptr(&esp[3]);

        int fd = (int) esp[1];
        const void *buffer = (const void *) esp[2];
        unsigned size = (unsigned) esp[3];

        validate_buff(buffer, size);
        f->eax = syscall_write(fd, buffer, size);
        break;
      }
    default:
      syscall_exit(-1);
  }
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// static void
// syscall_handler (struct intr_frame *f UNUSED) 
// {
//   printf ("system call!\n");
//   thread_exit ();
// }
