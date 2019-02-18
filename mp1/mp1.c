#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp1_given.h"
// proc related reference
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/list.h> /*klist datastructure */
#include <linux/string.h> /* use memcpy*/
#include <linux/slab.h> /*kmalloc*/
#include <linux/uaccess.h> /* copy_to_user */ 
#include <linux/timer.h> /*timer */
#include <linux/workqueue.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

struct proc_dir_entry *entry; /* mp1/status */
static char *entry_name = "status";

struct proc_cpu{
	/* store the */
	struct list_head ptr;
	unsigned long cpu_usage;
	int pid;
};

// type correctness
static struct list_head list;
static int size;
static struct timer_list tm;
static struct workqueue_struct * wq;
struct work_struct work;

spinlock_t lock;
/* myread and mywrite callback function */
/* read function, return the pid and related cpu_usage*/
static ssize_t myread(struct file * fp, 
		char __user * buff, 
		size_t len, 
		loff_t * offset)
{
	if(size == 0)
	{	
		char * errinfo = "No nodes in the linkedlist :(\n";
		printk(errinfo);
		copy_to_user(buff,errinfo, strlen(errinfo) + 1);
		*offset += (strlen(errinfo) + 1);  
	}
	if (* offset > 0) return 0;
	 
	printk(KERN_ALERT "READ FROM KERNEL, offset is: %d\n", *offset);	
	struct proc_cpu * j;
	int shift = 0;
	int buffsize = 2048;	
	char * kbuf =(char *) kmalloc(buffsize, GFP_KERNEL);
	printk(KERN_DEBUG "Current node size: %d, The buff size is: %d\n", size, buffsize);
	
	spin_lock(&lock);
	if (!list_empty(&list))
	{
		printk(KERN_ALERT "list is not empty\n");
		// lockhere? 
		list_for_each_entry(j, &list, ptr)
		{
			sprintf(kbuf+shift, "%d, %lu ms \n", j->pid, j->cpu_usage);
			shift = strlen(kbuf);
			printk(KERN_DEBUG "output string length, len(kbuf): %d\n", shift);		
		}		
	}
	spin_unlock(&lock);
	// copy to user
	copy_to_user(buff, kbuf, shift+1);
	kfree(kbuf);	
	*offset += shift;
	return shift+1;

}

/* write callback function, input should be the requested pid, return written bytes to kernel. */
static ssize_t mywrite(struct file * fp, 
		const char __user * buff, 
		size_t len, 
		loff_t * offset)
{
	// lock for the data structure
	
	char kbuf[len+1];
	kbuf[len] = '\x00';
	copy_from_user(kbuf, buff, len);
	int curr_pid;
	// kstrtoint
	int ret = kstrtoint(kbuf, 10, &curr_pid); /* kernel version of atoi()   could return long*/
	if (ret)
	{	int i = 0;
		while (kbuf[i] != '\0'){
			printk("ERRSTRTOI %x", *(kbuf+i));
			i++;
		}
		printk(KERN_ERR "error with kstrtoint %D\n", ret);		
		return ret;
	} 
	printk(KERN_DEBUG "The pid of requeted process: %d \n", curr_pid);

	// insert into the linkedlist 
	struct proc_cpu * newnode = (struct proc_cpu *) kmalloc(sizeof(struct proc_cpu), GFP_NOWAIT); /*GFP: get free pages*/
	newnode->pid = curr_pid;
	newnode->cpu_usage = 0;
	
	spin_lock(&lock);
	list_add_tail(&(newnode->ptr), &list);
	size++;
	spin_unlock(&lock);
	printk(KERN_DEBUG "write success, node size: %d \n", size);	
	return len;
	
}

static const struct file_operations proc_file_ops = {
	.owner = THIS_MODULE,
	.read = myread,
	.write = mywrite
};

// list the linkedlist iteratively
static void freell(struct list_head * curr_head)
{
	printk(KERN_ALERT "Free heap starting...");

	while (!list_empty(curr_head))
	{
		// ptr of the proc_cpu
		struct list_head * curr = curr_head->next; 
		printk(KERN_ALERT "start remove node for pid: %d \n", ((struct proc_cpu *) curr)->pid);
		list_del_init(curr);
		// direct cast the struct start address to struct list 
		kfree((struct roc_cpu * )  (curr));
	}
}

/* workqueue callback */
void wq_callback(struct work_struct * work)
{
	// update all the linklist node
	struct proc_cpu * curr;
	struct proc_cpu * tmp;
	// critical section begins
	spin_lock(&lock);
	if (list_empty(&list)) printk(KERN_ALERT "wqcallback, list is empty now. \n");
	list_for_each_entry_safe(curr, tmp, &list, ptr)
	{	
		printk("reach here pid&cpu: %d, %lu\n", curr->pid, curr->cpu_usage);
		if (!get_cpu_use(curr->pid, &curr->cpu_usage)) printk(KERN_ALERT "get cpu_time for %d: %lu\n", curr->pid, curr->cpu_usage);
		else
		{
		// remove the node form the node from the list
		list_del(curr);
		kfree(curr);
		size--;
		}
	}
	spin_unlock(&lock);
	// critical section ends
	printk(KERN_ALERT "work_Queue callback triggered\n ");
	
	// free the task instance
	kfree(work);
	return;
}

/* timer callback */
void tm_callback(unsigned long data)
{
	// add work to wq
	printk(KERN_ALERT "timer callback triggered\n");
	mod_timer(&tm, jiffies + msecs_to_jiffies(5000));
	
	struct work_struct * work = (struct work_struct * ) kmalloc(sizeof(struct work_struct), GFP_KERNEL);
	// add work to work queue
	if (work) 
	{
		INIT_WORK(work, wq_callback);
		if (queue_work(wq, work)) printk("queue_work success!\n");
	}
	printk(KERN_DEBUG "timer callback finished\n");
	
}

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   // Insert your code here ...
   size = 0;
   spin_lock_init(&lock);

   // create proc file system    
   entry = proc_mkdir("mp1", NULL);
   proc_create(entry_name, 0666, entry, &proc_file_ops);
   if (entry == NULL) 
   {
   	proc_remove(entry);
   	printk(KERN_DEBUG "proc entry creation failed");
   	return -ENOMEM;
   }

   printk(KERN_DEBUG "INIT_LIST_HEAD starting...\n");
   // linkedlist initializatino 
   INIT_LIST_HEAD(&list);

   // timer init
   setup_timer(&tm, tm_callback, 0);
   mod_timer(&tm, jiffies +msecs_to_jiffies(5000));
   
   // workqueue init
   wq = create_workqueue("mp1");
   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   // release the memory of linkedlist
   int ret_timer  = del_timer(&tm);
   if(ret_timer) printk("timer is still in use...\n");

   // 1 remove proc entry  
   proc_remove(entry);
   
   // timer workqueue delete
   flush_workqueue(&wq);
   destroy_workqueue(&wq);
   // TODO spinlock release
   freell(&list);
   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
   return;
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
