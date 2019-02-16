#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp1_given.h"
// proc related reference
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/list.h> /*klist datastructure */
#include <linux/string.h> /* use memcpy*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

struct proc_dir_entry *entry; /* mp1/status */
static char *entry_name = "status";

typedef struct{
	/* store the */
	struct list_head ptr;
	int cpu_usage;
	int pid;
}proc_cpu;

struct proc_cpu list;
static int size;
static const struct file_operations proc_file_ops = {
	.owner = THIS_MODULE,
//	.open = single_open,
	.read = myread,
	.write = mywrite
//	.release = single_release,
	// all the configuration for a file 
};

/* myread and mywrite callback function */
/* read function, return the pid and related cpu_usage*/
static ssize_t myread(struct file * fp, 
		char __user * buff, 
		size_t len, 
		loff_t * offset)
{
	// TODO: modify the linkedlist, register the pid of requesting process
	
	struct list_head * i;
	// iterate through
	int buffsize = sizeof(int) * size * 2;
	int * kbuf = kmalloc(buffsize, GFP_NOWAIT); /* kbuf for temporary storing all the pid-cputime*/

	if (!list_empty(list.ptr))
	{
		// lockhere? 
		foreach(i, &list.ptr)
		{
			struct proc_cpu * curr = (struct proc_cpu *) i;
			memcpy(kbuf, &(curr->pid), sizeof(int));
			kbuf++;
			memcpy(kbuf, &(curr->cpu_usage), sizeof(int));	
			kbuf++;
			// finish the copy of pid and cpu usage
		}

	};
	// copy to user
	copy_to_user(buff, kbuf, buffsize);
	
	return 0;

}

/* write callback function, input should be the requested pid, return written bytes to kernel. */
static ssize_t mywrite(struct file * fp, 
		const char __user * buff, 
		size_t len, 
		loff_t * offset)
{
	// lock for the data structure

	// TODO: iterate through the Linkedlist and return the corresponding cpu use time
	char kbuf[len];
	copy_from_user(kbuf, buff, len);
	int pid = atoi(kbuf);
	
	// insert into the linkedlist 
	
	struct proc_cpu * newnode = new kmalloc(sizeof(struct proc_cpu), GPF_NOWAIT); /*GFP: get free pages*/
	
	list_add_tail(newnode->ptr, list.ptr);
	
	// add tail then add size by 1
	size++;

	return 0;
	
}

// list the linkedlist iteratively
static void freell(struct list_head * curr_head)
{
	while (!list_empty())
	{
		// ptr of the proc_cpu
		struct list_head * curr = curr_head->next; 
		list_del_init(curr);
		// direct cast the struct start address to struct list 
		free((struct proc_cpu * )  (*curr));
	
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
   
   // linkedlist initializatino 
   LIST_HEAD_INIT(&list.ptr);
   list.pid = 0;
   list.cpu_usage = 0;

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
  
   // release the memory of linkedlist
   freell(list.ptr);



   // 1 remove proc entry  
   proc_remove(entry);
   

   
   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
