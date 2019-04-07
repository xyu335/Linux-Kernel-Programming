#include <linux/module.h>
#include <linux/kernel.h> 

#include "mp3_given.h"
#include <linux/proc_fs.h> // fs include proc_create
#include <linux/fs.h>  // fs include file_operaion
#include <linux/list.h>  // list
#include <linux/slab.h> // kmem allocator
#include <linux/vmalloc.h> // vmalloc interface
#include <asm-generic/param.h> // support for USER_HZ to translate the jiffies with mcroseconds
#include <linux/sched.h> // for the task struct defination
#include <asm-generic/uaccess.h> // copy from user, to user 


MODULE_AUTHOR("GROUP_ID");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS 423 MP2");


/* global variable */
char * proc_filename;
char * proc_dir;
struct proc_dir_entry * fp;
struct proc_dir_entry * dir;
struct list_head HEAD;



// mp3 struct
struct mp3_task_struct{
  struct task_struct * tsk;
  struct list_head node;
  // struct timer_list tm;
  unsigned long cpu_time;
  unsigned long cpu_starttime;
  unsigned long min_flt; // page fault
  unsigned long maj_flt; // IO page fault
  unsigned int pid;
};

int reg_entry(int pid)
{
  printk(KERN_DEBUG "[reg entry] entered for %d...\n", pid);
  return 0;
}


int unreg_entry(int pid)
{
  printk(KERN_DEBUG "[unreg entry] entered for %d...\n", pid);
  return 0;
}


/* read callback, for all the samples */
static ssize_t myread(struct file *fp, char __user * userbuff, size_t len, loff_t * offset) 
{

	return 0;
}


/* write callback, handle R and U requests */
static ssize_t mywrite(struct file * fp, const char __user * userbuff, size_t len, loff_t * offset)
{
  printk(KERN_DEBUG "[Write Callback] triggered");
  char buff[len+1];                                                                                     
  buff[len] = 0;
  int copied = copy_from_user(buff, userbuff, len);
  if ((buff[0] != 'R' && buff[0] != 'U') || strlen(buff) < 3) 
      printk(KERN_ALERT "The operation is not supported. Please choose either R(register), U(unregister) + pid as input...\n");
  int pid = 0;
  buff[strlen(buff)] = 0;
  kstrtoint(buff+2, 10, &pid); // char *, int type, container 
  char ops = buff[0];

  if (pid == 0) 
  {
    printk(KERN_ALERT "Failed with parse proc file input, please check your pid input...\n");
    return len;
  }

  switch (ops){
    case 'R':
      reg_entry(pid);
      break;
    case 'U':
      unreg_entry(pid);
      break;
    default: 
      printk(KERN_ALERT "this should not happen.\n");
  }
  printk(KERN_DEBUG "Parsed input is: ops - %d; pid - %d", ops, pid);
  printk(KERN_DEBUG "The input is %s \n", buff);
  return len; // mywrite will finish normally without return 0
}

// link function with proc file
static struct file_operations f_ops = {
  .owner=THIS_MODULE,
  .read=myread,
  .write=mywrite
};

/* init list_head, and kmemcache */
int __init mp3_init(void)
{
  // set up proc file entry
  // kernel list_head init
  // callback _ file operation
  printk(KERN_ALERT "[MODULE LOADING]... \n");
  proc_dir = "mp3";
  proc_filename = "status";
  dir = proc_mkdir(proc_dir, NULL);
  fp = proc_create(proc_filename, 0666, dir, &f_ops);
  if (!fp){
    printk(KERN_ALERT "File entry creation failed...\n");
    proc_remove(dir);
    return -ENOMEM;
  }
  printk(KERN_DEBUG "[init] list, slab, kernel thread\n current USER_HZ is %d...", USER_HZ);
  // INIT_LIST_HEAD(&HEAD);
  // init_slab();
  return 0;
}


/* remove proc dir and file, freeall nodes on the listm destroy the memcache */                            
int __exit mp3_exit(void)
{
  // stop all the thread which operate on the linked list
  // free all the memory on the heap 
  printk(KERN_ALERT "MODULE UNLOADING...\n");
  proc_remove(fp);
  proc_remove(dir);
  printk("Proc file entry removed successfully!\n");
  

  printk("successfully clean up all memory...\n");
  return 0;
}


/* associate with module call function */    
module_init(mp3_init);
module_exit(mp3_exit);

