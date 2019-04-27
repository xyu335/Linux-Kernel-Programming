#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp1_given.h"
// proc related reference
#include <linux/pid.h> // 
#include "mp4_given.h"
#include <linux/cred.h>
#include <linux/dcache.h>
#include <linux/fs.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");


// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   
   printk(KERN_ALERT "MP1 MODULE LOADED\n");
	
	struct mp4_security * sec = NULL;
	int flags = -1;
	rcu_read_lock(); // lock for find by pid
	struct task_struct * task = find_task_by_pid(1);
	if (task != NULL) 
	{
		struct cred * cd = task->cred;
		if (cd == NULL) 
		{
			printk(KERN_ALERT "cd is null.\n");
			rcu_read_unlock();
			return 0;
		}else 
		{
			sec = cd->security;
			flags = sec->mp4_flags;
			printk(KERN_ALERT "security addr: %p, flags: %d\n", sec, flags);
		}
	}
	rcu_read_unlock();
	if (!sec) printk(KERN_ALERT "security addr: %p, flags: %d\n", sec, flags);
	else printk(KERN_ALERT "false sec...");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
   return;
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
