#define LINUX

#include <linux/kernel.h>
#include <linux/module.h>
#include "mp2_given.h"

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h> // slab allocator
#include <linux/list.h>  // list of mp2_struct
#include <linux/timer.h>
#include <linux/kthread.h>

#define DEBUG		1
#define READY		1
#define RUNNING 	2
#define SLEEPING	0

#define CACHE_SIZE 20

MODULE_AUTHOR("GROUP_ID");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS 423 MP2");

const char * proc_dir;
const char * proc_filename;	
struct proc_dir_entry * fp;
struct proc_dir_entry * dir;
struct list_head HEAD;
struct task_struct * dispatch_kth;
struct mp2_task_struct{
	struct task_struct * tsk;
	struct list_head node;
	struct timer_list tm;
	int period;
	int computation;
	int pid;
	int state;
};

struct mp2_task_struct * curr_tsk;
struct kmem_cache * mycache; // cache for the mp2_task_Struct 

/* read callback, handling scheduling new periodical task */
static ssize_t myread(struct file * fp, char __user * userbuff, size_t len, loff_t * offset)
{
	printk(KERN_DEBUG "[Read Callback] triggered");
	// printk(KERN_DEBUG "OFFSET: %d, LEN: %d\n", *offset, len);
	if (*offset > 0) return 0;
	
	int buffsize = 2048;
	char * buf =(char *) kmalloc(buffsize, GFP_KERNEL);
	// TODO output all registered pid and their period and computation
	buf[0] = 'a';
	buf[1] = '0';
	int shift = strlen(buf);	
	copy_to_user(userbuff, buf, shift+1);
	kfree(buf);
	*offset += (shift);
	return shift; // if return 1, then one element will actually be returned to the stdout, len is a alrger number
}

/* when timer is expired, set the state of task to READY and wake up the dispatch thread */
void tm_callback(unsigned long data)
{
	struct mp2_task_struct * tsk = (struct mp2_task_struct *) data;
	mod_timer(&tsk->tm, jiffies + msecs_to_jiffies(tsk->period)); //TODO precision
	if (tsk->state == SLEEPING) tsk->state = READY;	
	wake_up_process(dispatch_kth);
}

/* entry for write callback function to register the periodic program */ 
int reg_entry(int pid, int period, int computation){
	printk(KERN_DEBUG "Register section enterd... params: %d, %d, %d\n", pid, period, computation);
	
	struct mp2_task_struct * tsk = kmem_cache_alloc(mycache, GFP_KERNEL); // TODO not sure which flag we should use
	list_add_tail(&tsk->node, &HEAD);
	tsk->pid = pid; 
	tsk->period = period; 
	tsk->computation = computation;
	setup_timer(&tsk->tm, tm_callback, (unsigned long) tsk); // args
	mod_timer(&tsk->tm, jiffies + msecs_to_jiffies(period));
	tsk->tsk = find_task_by_pid(pid);
	tsk->state = SLEEPING; //TODO firstly add the pid to the list, the state is default to be sleeping, so the thread will be wait for at least one time round to be invoked		
	return 0;
}

/* entry for write callback to yield the function at user's will */
int yield_entry(int pid){
	printk(KERN_DEBUG "Yield section enterd... params: %d\n",pid);
	return 0;
}

/* deregister the process from the list of task_struct */
int dereg_entry(int pid){
	printk(KERN_DEBUG "De-register section enterd... params: %d\n", pid);
	return 0;
}

/* helper function to find target char in a string starting from start index */
int findchar(char * s, char c, int start)
{	
	int i;
	for (i = start; i < strlen(s); ++i) 
		if (*(s + i) == c ) return i;
	return -1;
}

/* write callback, handling the request for register my p_task to the kernel */
static ssize_t mywrite(struct file * fp, const char __user * userbuff, size_t len, loff_t * offset)
{	
	printk(KERN_DEBUG "[Write Callback] triggered");
	char buff[len+1];
	buff[len] = 0;
	int copied = copy_from_user(buff, userbuff, len);
	if ((buff[0] != 'Y' && buff[0] != 'R' && buff[0] != 'D') || strlen(buff) < 3) 
			printk(KERN_ALERT "The operation is not supported. Please choose either R, Y, D for input...\n");
	int first = 2;
	int second = findchar(buff, ',', 2);
	int third = findchar(buff, ',', second + 1);
	int pid = 0, period = 0, computation = 0;
	if (second == -1) 
	{	buff[strlen(buff)] = 0;
		kstrtoint(buff+2, 10, &pid);
	}
	if (second != -1 && third != -1 && strlen(buff) > third + 2)
	{	buff[second] = '\0';
		buff[third] = '\0';
		kstrtoint(buff+2, 10, &pid);
		kstrtoint(buff+second+1, 10, &period);	
		kstrtoint(buff+third+1,10, &computation);
		buff[second] = ',';
		buff[third] = ',';
	}
	switch (buff[0]){
		case 'R':
			reg_entry(pid,period,computation);
			break;
		case 'Y':
			yield_entry(pid);
			break;
		case 'D':
			dereg_entry(pid);
			break;
	}
	printk(KERN_DEBUG "The input is %s \n", buff);
	return len; // mywrite will finish normally without return 0
}


static const struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.read = myread,
	.write = mywrite
};

/* set READY task with highest priority to RUNNABLE and schedule with 99 priority in FIFOS */
static void set_new_task(struct mp2_task_struct * tsk)
{
	struct sched_param sparam;
	wake_up_process(tsk->tsk);
	sparam.sched_priority = 99;
	sched_setscheduler(tsk->tsk, SCHED_FIFO, &sparam);
	curr_tsk = tsk;  // replace the current_tsk with new chosen task
	return;
}

/* set current running task with priority 0, and schedule it into the SCHED_NORMAL */
static void set_old_task(struct mp2_task_struct *tsk)
{
	struct sched_param sparam;
	sparam.sched_priority = 0;
	sched_setscheduler(tsk->tsk, SCHED_NORMAL, &sparam);
	return;
}

/* dispatch thread, schedule new process and save the old process when is called */
static int dispatch_fn(void)
{
	// explicitly check if the kernel thread can be stop
	while (!kthread_should_stop())
	{
		// kthread should work until signed to be released
		int min_period = INT_MAX;
		struct mp2_task_struct * next;
		struct mp2_task_struct * tmp;
		struct mp2_task_struct * itr;
		if (!curr_tsk) min_period = curr_tsk->period;
		next = curr_tsk;
		list_for_each_entry_safe(itr, tmp, &HEAD, node)
		{
			if (itr->state == READY && itr->period < min_period) 
			{
				min_period = itr->period;
				next = itr; 
			}
		}
		if (curr_tsk && curr_tsk != next) set_old_task(curr_tsk); 
		if (next != NULL) set_new_task(next);
		// set kernel thread itself to sleep, UNINTERRUPTIBLE
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule();
	};
	
	return 0;
	// do_exit();
	// finish the dispatch thread
}

/* used at the start of the program, wake it up at the start */
static void init_mykernel(void)
{
	dispatch_kth = kthread_create(dispatch_fn, NULL, "dispatch_kernel"); // func, data, name, args
	if (!dispatch_kth) 
		printk(KERN_ALERT "Kernel thread create failed...");
	else
		wake_up_process(dispatch_kth);
	return;
}

static void init_slab(void)
{
	char * cache_name = "slab";
	int size = sizeof(struct mp2_task_struct);
	mycache = kmem_cache_create(cache_name, size, 0,  SLAB_HWCACHE_ALIGN, NULL); // construction and decons
	return;
}


/* init list_head, kmemcache, and kernel */
int __init mp2_init(void)
{
	// set up proc file entry
	// kernel list_head init
	// callback _ file operation
	printk(KERN_ALERT "MODULE LOADING... \n");
	proc_dir = "mp2";
	proc_filename = "status";
	dir = proc_mkdir(proc_dir, NULL);
	fp = proc_create(proc_filename, 0666, dir, &f_ops);
	if (!fp){
		printk(KERN_ALERT "File entry creation failed...\n");
		proc_remove(dir);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&HEAD);
	init_slab();
	init_mykernel();
	return 0;
}

/* free: timer, list_head, mp2_task_struct for each node on the list */
int freeall(void)
{
	printk(KERN_ALERT "Starting free the memory...");
	curr_tsk = NULL;
	struct mp2_task_struct * tmp;
	struct mp2_task_struct * itr;
	if (!list_empty(&HEAD)) 
	{
		list_for_each_entry_safe(itr, tmp, &HEAD, node)
		{
			// delete list_head, timer_list
			// task_struct
			del_timer(&itr->tm);
			list_del(&itr->node);
			kmem_cache_free(mycache, itr);
		}
	}
	
	return 0;
}

/* remove proc dir and file, freeall nodes on the listm destroy the memcache */
int __exit mp2_exit(void)
{
	// stop all the thread which operate on the linked list
	// free all the memory on the heap 
	printk(KERN_ALERT "MODULE UNLOADING...\n");
	proc_remove(fp);
	proc_remove(dir);
	printk("Proc file entry removed successfully!\n");

	freeall();
	
	printk(KERN_DEBUG "Start destroy the mycache...");
	if (mycache) kmem_cache_destroy(mycache);
	return 0;
}


module_init(mp2_init);
module_exit(mp2_exit);


