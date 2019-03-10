#define LINUX

#include <linux/kernel.h>
#include <linux/module.h>
#include "mp2_given.h"

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <asm-generic/uaccess.h>
#include <linux/slab.h>

MODULE_AUTHOR("GROUP_ID");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS 423 MP2");


const char * proc_dir;
const char * proc_filename;	
struct proc_dir_entry * fp;
struct proc_dir_entry * dir;

/* read callback, handling scheduling new periodical task */
static ssize_t myread(struct file * fp, char __user * userbuff, size_t len, loff_t * offset)
{
	printk(KERN_DEBUG "[Read Callback] triggered");
	int buffsize = 2048;
	char * buf =(char *) kmalloc(buffsize, GFP_KERNEL);
	int i;
	for (i = 0; i < 256; ++i)
	{
		*(buf + i) = 'a';
	}
	// char buff[256] = {97};
	buf[255] = 0;
	int shift = strlen(buf);
	printk(KERN_DEBUG "shift length: %d\n", shift);
	
	// 
	copy_to_user(userbuff, buf, shift+1);
	kfree(buffsize);
	*offset += (shift);
	return shift+1;
}

int reg_entry(void){
	;
}
int yield_entry(void){
	;
}
int dereg_entry(void){
	;
}
/* write callback, handling the request for register my p_task to the kernel */
static ssize_t mywrite(struct file * fp, const char __user * userbuff, size_t len, loff_t * offset)
{	
	printk(KERN_DEBUG "[Write Callback] triggered");
	char buff[len+1];
	buff[len] = 0;
	int copied = copy_from_user(buff, userbuff, len);
	switch (buff[0]){
		case 'R':
			reg_entry();
		case 'Y':
			yield_entry();
		case 'D':
			dereg_entry();
		default:
			printk(KERN_ALERT "The operation is not supported. Please choose either R, Y, D for input...\n");
	}
	printk(KERN_DEBUG "The input is %s \n", buff);
	return len;
}


static const struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.read = myread,
	.write = mywrite
};

int __init mp2_init(void)
{
	// set up proc file entry
	// kernel list_head init
	// callback _ file operation
	printk(KERN_ALERT "MODULE LOADING... \n");
	proc_dir = "mp2";
	proc_filename = "status";
	dir = proc_mkdir(proc_dir, NULL);
	/*if (!dir){
		printk(KERN_ALERT "Directory creation failed...check if there already have an entry\n");
		return -ENOMEM;
	}*/
	fp = proc_create(proc_filename, 0666, dir, &f_ops);
	if (!fp){
		printk(KERN_ALERT "File entry creation failed...\n");
		proc_remove(dir);
		return -ENOMEM;
	}
	// test1
	

	return 0;
}

int __exit mp2_exit(void)
{
	// stop all the thread which operate on the linked list
	// free all the memory on the heap 
	printk(KERN_ALERT "MODULE UNLOADING...\n");
	proc_remove(fp);
	proc_remove(dir);	
	printk("proc file entry removed\n");

	return 0;
}


module_init(mp2_init);
module_exit(mp2_exit);


