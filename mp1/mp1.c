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



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

struct proc_dir_entry *entry; /* mp1/status */
static char *entry_name = "status";

struct proc_cpu{
	/* store the */
	struct list_head ptr;
	int cpu_usage;
	int pid;
};

// type correctness
static struct list_head list;
static int size;

/* myread and mywrite callback function */
/* read function, return the pid and related cpu_usage*/
static ssize_t myread(struct file * fp, 
		char __user * buff, 
		size_t len, 
		loff_t * offset)
{
	// TODO: modify the linkedlist, register the pid of requesting process
	if (* offset > 0) return 0;
	 
	printk(KERN_ALERT "READ FROM KERNEL, offset is: %d\n", *offset);	
	struct list_head * i;
	struct proc_cpu * j;
	int shift = 0;
	// iterate through
	// int buffsize = sizeof(int) * size * 2;
	// int kbuf[buffsize];
	int buffsize = 2048;	
	char * kbuf =(char *) kmalloc(buffsize, GFP_KERNEL);
	printk(KERN_DEBUG "Current node size: %d, The buff size is: %d\n", size, buffsize);

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


/*		list_for_each(i, &list)
		{
			proc_cpu * curr = (proc_cpu *) i;
			sprintf(kbuf+shift, "%d, %lu ms\n", curr->pid, curr->cpu_usage);
			shift = strlen(kbuf);
			printk(KERN_DEBUG "output string length, len(kbuf): %d\n", shift);
			// finish the copy of pid and cpu usage
		}
*/
	};
	// strlen excluding the null byte

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
	newnode->cpu_usage = curr_pid;
	// TODO cpu_time initalization
	list_add_tail(&(newnode->ptr), &list);
	
	printk(KERN_DEBUG "malloc success for new node \n");
	// add tail then add size by 1
	size++;
	printk(KERN_DEBUG "write success, node size: %d \n", size);	

	return len;
	
}

static const struct file_operations proc_file_ops = {
	.owner = THIS_MODULE,
//	.open = single_open,
	.read = myread,
	.write = mywrite
//	.release = single_release,
	// all the configuration for a file 
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


// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   // Insert your code here ...
   size = 0;

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

   // handler registration


   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   // Insert your code here ...
  
   printk(KERN_ALERT "Free linked list starting...\n");
   // release the memory of linkedlist
   freell(&list);


   // 1 remove proc entry  
   proc_remove(entry);
   

   
   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
   return;
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
