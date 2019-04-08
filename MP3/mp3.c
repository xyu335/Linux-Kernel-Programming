#include <linux/kernel.h>
#include <linux/module.h>

#include "mp3_given.h"
#include <linux/proc_fs.h> // fs include proc_create
#include <linux/fs.h>  // fs include file_operaion
#include <linux/list.h>  // list
#include <linux/slab.h> // kmem allocator
#include <linux/vmalloc.h> // vmalloc interface
#include <asm-generic/param.h> // support for USER_HZ to translate the jiffies with mcroseconds
#include <linux/sched.h> // for the task struct defination
#include <asm-generic/uaccess.h> // copy from user, to user 
#include <workqueue.h> 
#include <linux/mm_types.h>
#include <asm-generic/page.h> 
#include <linux/mm.h> // memory management

MODULE_AUTHOR("GROUP_ID");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS 423 MP2");

/* #define container_of(ptr, type, member) ({                      \
		        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
		        (type *)( (char *)__mptr - offsetof(type,member) );}) // for manuly use, refernce
*/ 

/* macro */
#define debug(msg) (printk(KERN_DEBUG msg);)  // for notification macro
#define alert(msg) (printk(KERN_ALERT msg);)  // for notification macro
#define VM_SIZE (512000)											// virtual memory size, bytes 4 * 128 KB = >  32 K sample * 16 Bytes of the four unsigned long data
#define VM_SAMPLE_SIZE (32000)								// sample size
#define SAMPLE_RATE 20

/* global variable */
char * proc_filename;
char * proc_dir;
char * chrdev_name;
unsigned int chrdev_major;
struct proc_dir_entry * fp;
struct proc_dir_entry * dir;
struct list_head HEAD;
struct workqueue_struct * wq;
struct delayed_work work;
static unsigned long * vm; // 
static unsigned long DELAYED; // for delayed queue work
static unsigned int USER_USE; // for counting char device open times
static unsigned int USER_CONCURRENT`_LIMIT = 1; // for limit access to the driver
static unsigned int offset; // how many (unsgined long ) has been written into the sample
static int BUFFER_FILLED_UP;

/* type macro */
typedef struct mp3_task_struct ts_mp3; 

/* mp3 struct */
struct mp3_task_struct{
  struct task_struct * tsk;
  struct list_head node;
  // struct timer_list tm;
  unsigned long utime; // user space cpu time
  unsigned long stime; // system space cpu time
  unsigned long minor_pf; // page fault
  unsigned long major_pf; // IO page fault
  pid_t pid;
};

/* iterate through list for ts_mp3 with target pid */
ts_mp3 * get_by_pid(int pid)
{
	if (list_empty(&HEAD)) return NULL;
	ts_mp3 * itr = NULL;
	ts_mp3 * tmp = NULL;
	list_for_each_entry_safe(itr, tmp, &HEAD, node)
	{
		if (itr->pid == pid) return itr;
	}
	return NULL;
}

/* work callback, update cpu information for registered process */
void workqueue_callback(work_struct * curr_work)
{
	debug("[work callback] entered...\n");
	if (BUFFER_FILLED_UP)
	{
		alert("Profiler buffer used up...\n");
		return;
	}

	ts_mp3 * itr = NULL;
	ts_mp3 * tmp = NULL;
	
	unsigned long tmps[4] = {};
	int index = 0;
	list_for_each_entry_safe(itr, tmp, &HEAD, node)
	{
		// update pf, utime, into the ts_mp3 struct
		// copy it to the shared memory
		get_cpu_use(itr->pid, &itr->minor_pf, &itr->major_pf, &itr->utime, &itr->stime);
		tmps[0]+=itr->minor_pf;
		tmps[1]+=itr->major_pf;
		tmps[2]+=itr->utime;
		tmps[3]+=itr->stime;	
		printk(KERN_DEBUG "update for pid: %d, minor: %ld, major: %ld\n", itr->pid, itr->minor_pf, itr->major_pf);
	}
	
	// print to the vm space 
	// sprintf(vm + offset, tmps)
	unsigned long * ptr = vm + offset;
	for (index; index < 4; ++index)
	{
		* (ptr++) = tmp[index];
	}
	
	offset += 4;	
	if (offset >= VM_SAMPLE_SIZE * 4) 
	{
		BUFFER_FILLED_UP = 1;
	}
	else * (ptr) = -1; // termination
	return; 	
}

/* chr device callback functions*/
int mymmap (struct file * inode, struct vm_area_struct * vma)
{
	// map get mmap request from process, and map the user address to the kernel vm 
	
	/* unsigned long vmalloc_to_pfn(const void * virtual_addr);
	 * int remap_pfn_range(struct vm_area_struct *, unsigned long addr,
	 * 			unsigned long pfn, unsigned long size, pgprot_t);
	 * pfn: physical memory address
	 * */
	unsigned long p_addr = vmalloc_to_pfn(vm);
	if (!p_addr) 
	{
		alert("mapping to physical memory failed...\n");
		return -1; // TODO return numer not sure
	}
	
	// int size = vma->vm_pgoff << PAGE_SHIFT;
	int size = vma->vm_end - vm_start; // little endian 
	if (remap_pfn_range(vma, vma->vm_start, p_addr, size, vma->vm_page_prot)) return -EAGAIN; // TODO error code declaration
	return 0;
}

/* operation function for char device, device open */
int myopen (struct inode *, struct file *)
{
	if (USER_USE >= USER_CONCURRENT_LIMIT) return -1;
	USER_USE++;
		
	try_module_get(THIS_MODULE);
	return 0;
}

/* operation function for char device, device close */
int myrelease (struct inode *, struct file *){
	USER_USE--;
	module_put(THIS_MODULE);
	return 0
};

/* */
int reg_entry(int pid)
{
  printk(KERN_DEBUG "[reg entry] entered for %d...\n", pid);
	
	ts_mp3 * new_struct = kmalloc(sizeof(ts_mp3), GFP_KERNEL); // TODO replace with the kmemecache
	new_struct->pid = pid;
	new_struct->tsk = find_task_by_pid(pid); // replace with get_cpu_use(task_struct * )
	if (!new_struct->tsk) return 1;
	
	list_add_tail(&node, &HEAD);

	// init workqueue job
	if (list_empty_careful(&HEAD))
	{
		// workqueue
		// delayed work
		INIT_DELAYED_WORK(work, workqueue_callback);
		if (work)
			if (queue_delayed_work(wq, work, DELAYED)) alert("failed with queue work...\n");
		else alert("failed with work init...\n");
	}
  return 0;
}


/**/
int unreg_entry(int pid)
{
  printk(KERN_DEBUG "[unreg entry] entered for %d...\n", pid);
  
	//remove entry from list
	ts_mp3 * task = find_by_pid(pid);
	if (!task) 
	{
		alert("unreg entry failed, task struct with pid not found...\n");
	}
	list_del(&task->node);

	if (list_empty(&HEAD)) 
	{	
		cancel_delayed_work_sync(work);
	}

	kfree(task);
	debug("unregister finished\n");
	return 0;
}

/* read callback, for all the samples */
static ssize_t myread(struct file *fp, char __user * userbuff, size_t len, loff_t * offset) 
{
	// read is not used in this mp
	return 0;
}


/* write callback, handle R and U requests */
static ssize_t mywrite(struct file * fp, const char __user * userbuff, size_t len, loff_t * offset)
{
  debug("[Write Callback] triggered");
  char buff[len+1];                                                                                     
  buff[len] = 0;
  int copied = copy_from_user(buff, userbuff, len);
  if ((buff[0] != 'R' && buff[0] != 'U') || strlen(buff) < 3) 
      alert("The operation is not supported. Please choose either R(register), U(unregister) + pid as input...\n");
  int pid = 0;
  buff[strlen(buff)] = 0;
  kstrtoint(buff+2, 10, &pid); // char *, int type, container 
  char ops = buff[0];

  if (pid == 0) 
  {
    alert("Failed with parse proc file input, please check your pid input...\n");
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
      alert("this should not happen.\n");
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

static struct file_operations f_ops_chrdev = {
	.owner=THIS_MODULE,
	.open=myopen,
	.release=myrelease, 
	.mmap=mmap_callback
};

/* init list_head, and kmemcache */
int __init mp3_init(void)
{
  // set up proc file entry
  // kernel list_head init
  // callback _ file operation
  alert("[MODULE LOADING]... \n");
  proc_dir = "mp3";
  proc_filename = "status";
  dir = proc_mkdir(proc_dir, NULL);
  fp = proc_create(proc_filename, 0666, dir, &f_ops);
  if (!fp){
    alert("File entry creation failed...\n");
    proc_remove(dir);
    return -ENOMEM;
  }
  printk(KERN_DEBUG "[init] list, slab, kernel thread\n current USER_HZ is %d...", USER_HZ);
  INIT_LIST_HEAD(&HEAD);
  // init_slab();
	// frequency => jiffies, 1/rate * 1000/100
	DELAYED = ((double) 1/SAMPLE_RATE) * (1000/100);
	printk(KERN_DEBUG "DELAYED set to be %ld\n", DELAYED);
	wq = create_workqueue("mp3");

	// vmalloc
	vm =(unsigned long *) vmalloc(VM_SIZE);
	
	// character device initialization
	if (register_chrdev(0, chrdev_name, f_ops_chrdev) < 0){
		alert("char device register fails...\n");	
	}

  return 0;
}


/* remove proc dir and file, freeall nodes on the listm destroy the memcache */                            
int __exit mp3_exit(void)
{
  // stop all the thread which operate on the linked list
  // free all the memory on the heap 
  alert("MODULE UNLOADING...\n");
  proc_remove(fp);
  proc_remove(dir);
  debug("Proc file entry removed successfully!\n");
	cancel_delayed_work_sync(work);
  destroy_workqueue(wq);
	debug("Work and Workqueue removed successfully\n");
	if (unregister(chrdev_major, chrdev_name) < 0) alert("char device unregistered unsuccessfully...\n");
	vfree(vm);	
	debug("VM freeed\n");

  debug("successfully clean up all memory...\n");
  return 0;
}


/* associate with module call function */    
module_init(mp3_init);
module_exit(mp3_exit);

