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
// #include <linux/jiffies.h> 

#define DEBUG		1
#define READY		1
#define RUNNING 	2
#define SLEEPING	0

MODULE_AUTHOR("GROUP_ID");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS 423 MP2");

const char * proc_dir;
const char * proc_filename;	
struct proc_dir_entry * fp;
struct proc_dir_entry * dir;
struct list_head HEAD;
struct task_struct * dispatch_kth;
unsigned long denominator;
unsigned long numerator;


struct mp2_task_struct{
	struct task_struct * tsk;
	struct list_head node;
	struct timer_list tm;
	unsigned int period;
	unsigned int computation;
	unsigned int pid;
	unsigned int state;
	uint64_t next_period;
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
	char * tmpbuf = (char *) kmalloc(buffsize, GFP_KERNEL);
	char * buf =(char *) kmalloc(buffsize, GFP_KERNEL);
	int off = 0;
	int size = 0; 
	struct mp2_task_struct * tmp = NULL;
	if (!list_empty(&HEAD))
	{
		list_for_each_entry(tmp, &HEAD, node)
		{
			// output format: tmp->pid\n, output all registered pid
			++size;
			sprintf(tmpbuf+off, "%d\n\0", tmp->pid);
			off = strlen(tmpbuf);
		}
	}
	sprintf(buf, "%d\n\0", size);
	int total_len = strlen(buf) + off;
	memcpy(buf+strlen(buf), tmpbuf, off+1);
	copy_to_user(userbuff, buf, total_len + 1);
	kfree(buf);
	kfree(tmpbuf);
	*offset += (total_len + 1);
	return total_len + 1; // if return 1, then one element will actually be returned to the stdout, len is a alrger number
}

/* when timer is expired, set the state of task to READY and wake up the dispatch thread */
void tm_callback(unsigned long data)
{
	struct mp2_task_struct * tsk = (struct mp2_task_struct *) data;
	#ifdef DEBUG
	printk(KERN_ALERT "Timer triggered for pid:%d ...", tsk->pid);
	#endif
	if (tsk->state == SLEEPING) tsk->state = READY;	
	wake_up_process(dispatch_kth);
	return;
}

/* free signle mp2_struct node */
int freeone(struct mp2_task_struct * itr)
{
	#ifdef DEBUG
	printk(KERN_DEBUG "Starting freeone for pid: %d ...", itr->pid);
	#endif
	del_timer_sync(&itr->tm);
	list_del(&itr->node);
	kmem_cache_free(mycache, itr);
	// what to deal with task_struct itself? 
	return 0;
}

/* determine if the entering task will still make the RTS possible to meet all deadlines, return 1 for possible, return 0 for impossible */
int admission_control(unsigned int period, unsigned int computation, unsigned int pid, int flag)
{
	int ret = 1;
	if (flag == 1){ 
		printk(KERN_DEBUG "remove C/P from current admission control: %lu, %lu\n", numerator, denominator);
		numerator = numerator * period  - computation * denominator;
		if (numerator == 0) denominator = 0;
		else denominator *= period;
	}
	else{
	  printk(KERN_DEBUG "admission control, current C, P: %lu, %lu\n", numerator, denominator);
	  if (numerator == 0) 
	  {
		  if (computation * 1000 <= 693 * period) {
			ret = 0;
			denominator = period;
			numerator = computation;
		  }
	  }else {
		  unsigned long curr_den = denominator * period;
		  unsigned long curr_n = numerator * period + computation * denominator;
		  if (curr_n * 1000 <= 693 * curr_den){
			ret = 0;
			numerator = curr_n;
			denominator = curr_den;
		  }
	  }
	}
	return ret;
}

/* entry for write callback function to register the periodic program */ 
int reg_entry(int pid, unsigned int period, unsigned int computation){
	printk(KERN_DEBUG "Register section enterd... params: %d, %d, %d\n", pid, period, computation);
	
	if (admission_control(period, computation, pid, 0)) 
	{	
		printk(KERN_ALERT "This task is not qualified for current task sets to meet RTS requirement...");
		return 1;
	}
	struct mp2_task_struct * tsk = kmem_cache_alloc(mycache, GFP_KERNEL);
	list_add_tail(&tsk->node, &HEAD);
	tsk->pid = pid; 
	tsk->period = period; // jiffies unsigned long
	tsk->computation = computation;
	tsk->tsk = find_task_by_pid(pid);
	tsk->next_period = 0; 
	tsk->state = SLEEPING; //Firstly add the pid to the list, the state is default to be sleeping, so the thread will be wait for at least one time round to be invoked		
	setup_timer(&tsk->tm, tm_callback, (unsigned long) tsk); // timer init in first yield function 

	#ifdef DEBUG
	printk(KERN_DEBUG "PID % is finishing register; period %d; computation %d.", tsk->pid, tsk->period, tsk->computation);
	#endif 	
	return 0;
}

/* iterate through the list to find the mp2 struct with identical pid */
struct mp2_task_struct * find_by_pid(int pid)
{
	struct mp2_task_struct * itr;
	if (!list_empty(&HEAD))
	{ // TODO add lock
		list_for_each_entry(itr, &HEAD, node)
		{
			if (itr->pid == pid) return itr;
		}
	}
	return NULL;
}

/* set task to sleep till next period, return 1 for successfully sleep, return 0 for no task found */
int set_task_sleep(int pid)
{
	struct mp2_task_struct * tmp = find_by_pid(pid);
	if (!tmp) return 0;
	if (tmp == curr_tsk) curr_tsk = NULL; 
	#ifdef DEBUG
	printk(KERN_DEBUG "task with pid %d is set to sleep...\n", tmp->pid);
	#endif
	tmp->state = SLEEPING; // TODO, possiblely not sleeping, yield to set old sleep 
	set_task_state(tmp->tsk, TASK_UNINTERRUPTIBLE); // decide if the yield task should be set to Uninterrupt
	// schedule not needed since same process stack may not need this
	return 1;
}


/* entry for write callback to yield the function at user's will */
void yield_entry(int pid){
	#ifdef DEBUG
	if (curr_tsk) printk(KERN_DEBUG "current task pid is %d,and yield input: %d \n", curr_tsk->pid, pid);
	else printk(KERN_DEBUG "there is no current task running...");
	#endif
	printk(KERN_DEBUG "Yield section enterd... params: %d\n",pid);
	int ret = 1;

	struct mp2_task_struct * yield_tsk = find_by_pid(pid);

	// ACTIVATION: to activate the first timer, then wakeup the dispatch
	if (yield_tsk->next_period == 0) 
	{
		printk(KERN_DEBUG "Task activcation: yield first called for this process, current time: %lu\n", jiffies);
		yield_tsk->next_period = jiffies + jiffies_to_msecs(yield_tsk->period);
		yield_tsk->state = READY; //TODO: current process,should be put to NORMAL queue if not chosen
		// set_task_state(yield_tsk, TASK_INTERRUPTIBLE); // set_task_state doesn't need the schedule
		wake_up_process(dispatch_kth); 
		return;
	}

	printk(KERN_DEBUG "Yield at jiffies %lu\n", jiffies);	
	// if task's next period has not begun
	if (jiffies < yield_tsk->next_period) 
	{
	  	ret = set_task_sleep(pid);
	  	mod_timer(&yield_tsk->tm, yield_tsk->next_period);
		printk(KERN_DEBUG "Yield set the timernext jiffies %lu; task's period: %lu\n", yield_tsk->next_period, jiffies_to_msecs(yield_tsk->period));
	}
	yield_tsk->next_period += jiffies_to_msecs((unsigned long) yield_tsk->period);
	// if (!ret) return 0; // if the pid doesn't exist, then there is no need to return. 
	wake_up_process(dispatch_kth);
	schedule(); // TODO schedule() not enabled 
} //

/* deregister the process from the list of task_struct */
int dereg_entry(int pid){
	printk(KERN_DEBUG "De-register section enterd... params: %d\n", pid);
	
	struct mp2_task_struct * tsk = find_by_pid(pid);
	if (!tsk) 
	{	printk(KERN_DEBUG "No task struct found with this pid: %d ...", pid);
		return 1;
	}
	if (tsk == curr_tsk) curr_tsk = NULL;
	admission_control(tsk->pid, tsk->period, tsk->computation, 1);
	freeone(tsk);
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
	tsk->state = RUNNING;
	sched_setscheduler(tsk->tsk, SCHED_FIFO, &sparam);
	curr_tsk = tsk;  // bug possible, if process is preempted, then curr_tsk is not switched out
	return;
}

/* set current running task with priority 0, and schedule it into the SCHED_NORMAL */
static void set_old_task(struct mp2_task_struct *tsk)
{
	struct sched_param sparam;
	sparam.sched_priority = 0;
	if (tsk->state == RUNNING) tsk->state = READY; // if current state is SLEEPING, then we should not make it ready now
	sched_setscheduler(tsk->tsk, SCHED_NORMAL, &sparam);
	return;
}

/* dispatch kernel thread, schedule new process and save the old process when is called */
static int dispatch_fn(void)
{
	printk(KERN_ALERT "Kernel thread is running...");
	// explicitly check if the kernel thread can be stop, kernel will signaled if there is no need to poll
	while (!kthread_should_stop()) 
	{
		// kthread should work until the context switching is finished
		unsigned int min_period = INT_MAX; 
		struct mp2_task_struct * next = NULL;
		struct mp2_task_struct * tmp = NULL;
		struct mp2_task_struct * itr = NULL;
		// if (curr_tsk) 
		// DEFAULT SHOULD BE NULL
		#ifdef DEBUG 
		printk(KERN_DEBUG "Start iterate the list to choose process to switch...");
		#endif
		if (!list_empty(&HEAD))
		{
		    list_for_each_entry_safe(itr, tmp, &HEAD, node)
		    {
			    // decide which task to activate, should take current time into account
			    if (itr->state != SLEEPING && itr->period < min_period)  // itr->state == READY | RUNNING
			    {
				    min_period = itr->period;
				    next = itr;
			    }
		    }
		}
		// TODO check running task's period, to decide the next task
		// TODO situation: dispatch is interrupted by another timer 
		#ifdef DEBUG
		if (next) printk(KERN_DEBUG "pid of the chosen proc: %d\n", next->pid);
		printk(KERN_DEBUG "This is the choosen pid's mp_struct pointer: next %d, curr_tsk %d ", next, curr_tsk);
		#endif
		if (next && curr_tsk != next) 
		{	
			if (curr_tsk) set_old_task(curr_tsk); 
			set_new_task(next); // if the curr == next, there is not need to switch
		}
		
	
		// set kernel thread itself to sleep, INTERRUPTIBLE
		printk(KERN_DEBUG "kernel thread is going to sleep...");
		set_current_state(TASK_INTERRUPTIBLE);  // kernel thread should be wakable by the wake_up_process
		schedule();
	};
	
	// should release resources owned by kernel thread
	// do_exit(); there is no need to call exit, other that we should return the val 
	return 0;
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
	printk(KERN_DEBUG "Slab allocator initializing... ");
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
	curr_tsk = NULL;
	denominator = 0;
	numerator = 0;
	printk(KERN_DEBUG "Start init list, slab, kernel thread...");
	INIT_LIST_HEAD(&HEAD);
	init_slab();
	#ifdef DEBUG
	printk(KERN_DEBUG "Init mykernel started... ");	
	#endif
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
			// delete list_head, timer_list, task_struct
			freeone(itr);
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
	int ret = kthread_stop(dispatch_kth); // kill the kernel code, when the task is chosen, it will wake up the process to wait for its completion
	printk(KERN_DEBUG "Start destroy the mycache...");
	if (mycache) kmem_cache_destroy(mycache);
	return 0;
}


module_init(mp2_init);
module_exit(mp2_exit);
