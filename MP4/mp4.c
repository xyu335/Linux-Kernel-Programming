#define pr_fmt(fmt) "cs423_mp4: " fmt

#include <linux/lsm_hooks.h>
#include <linux/security.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/cred.h>
#include <linux/dcache.h>
#include <linux/binfmts.h>
#include <linux/fs.h>  // for inode struct, /fs/xattr.c vfs_getxattr, // superblock enabled // file access control macros
#include <linux/dcache.h> // for dentry and dput
#include <linux/string.h> // strlen enabled
#include <linux/xattr.h> // 
#include <linux/errno.h> // errno/

#include <asm/uaccess.h> 

#include "mp4_given.h"

#ifdef CONFIG_SECURITY_MP4_LSM
void do_something(void) {pr_info("MP4 activated.");}
#else 
void do_something(void) {pr_info("nothing.");}
#endif

// typedef blobs struct mp4_security;	/* type macrp */

/**
 * get_inode_sid - Get the inode mp4 security label id
 *
 * @inode: the input inode
 *
 * @return the inode's security id if found.
 *
 */
static int get_inode_sid(struct inode *inode)
{
	// determine if the xattr support is enabled
	if (!inode->i_op->getxattr) 
	{
		// pr_err("current security module does not have xattr support.\n");
		return -ENOENT;
	}

	// get the xattr of the inode
	char * ctx = NULL;
	
	// find the parent directory dentry
	// struct super_block * i_sb = inode->i_sb;
	// de = i_sb->s_root;
	struct dentry * de = d_find_alias(inode);
	if (!de) {
		pr_err("the dentry is null..\n");
		return -ENOENT;
	}

	int len = strlen(XATTR_NAME_MP4);
	ctx = kmalloc(len + 1, GFP_KERNEL); // TODO, pitfall 
	if (!ctx) 
	{
		dput(de);
		pr_err("memory is not allocated..\n");
		return -ENOMEM;
	}
	ctx[len] = '\0';
	ssize_t ret = inode->i_op->getxattr(de, XATTR_NAME_MP4, ctx, len);
	//TODO  ret is -1 
	if (ret <= 0) 
	{
		if (printk_ratelimit()) pr_err("get extend attr failed\n");  // TODO remove this when bug is fixed
		dput(de);
		return -1;
	}
	// watch out for ERANGE error. 
	// convert the attr ctx to sid 
	int sid = __cred_ctx_to_sid(ctx);
	// if (printk_ratelimit()) pr_info("sid is generated for inode");
	dput(de);

	return sid;
}

/**
 * mp4_bprm_set_creds - Set the credentials for a new task
 *
 * @bprm: The linux binary preparation structure
 *
 * returns 0 on success.
 */
static int mp4_bprm_set_creds(struct linux_binprm *bprm)
{

	// READ THE context of the binary file that launch the program
	if (!bprm)
	{
		pr_err("bprm ptr null..\n");
		return -ENOENT;
	}
	struct inode * node = file_inode(bprm->file); // file_inode is inline function, return inode pointer other than error code
	if (!node) return -ENOENT;

	int sid = get_inode_sid(node);
	// speed up the process
	if (sid == MP4_NO_ACCESS) return -1; // TODO
	// rcu lock required, since only the task itself can modify the credentials of it
	struct cred * cred = current_cred();
	if (!cred) 
	{	
		if (printk_ratelimit()) pr_err("NO entrance in the bprm set cred hook");
		return -ENOENT; // the credential of the current process (binary program) has no credential
	}
	
	if (!cred->security) 
	{
		pr_err("SHOULD NOT HAPPEN: cred->sec allocation start in bprm hook");
		struct mp4_security * ptr = kzalloc(sizeof(struct mp4_security), GFP_KERNEL);
		if (!ptr)
		{	pr_err("no memory in allocating security label");
			return -ENOMEM;}
		cred->security = ptr;
	}
	struct mp4_security * ptr = (cred->security);
	ptr->mp4_flags = sid;
	return 0;
}

/**
 * mp4_cred_alloc_blank - Allocate a blank mp4 security label
 *
 * @cred: the new credentials
 * @gfp: the atomicity of the memory allocation
 *
 */
static int mp4_cred_alloc_blank(struct cred *cred, gfp_t gfp)
{
	
	if (!cred) 
	{
		return -ENOENT; 
	}
	struct mp4_security * ptr = kzalloc(sizeof(struct mp4_security), gfp);
	if (!ptr)
	{
		pr_err("memory allocation for blob failed..");
		return -ENOENT;
	}
	cred->security = ptr; 
	ptr->mp4_flags = MP4_NO_ACCESS;
	return 0;
}


/**
 * mp4_cred_free - Free a created security label
 *
 * @cred: the credentials struct
 *
 */
static void mp4_cred_free(struct cred *cred)
{
	if (!cred) 
	{
		if (printk_ratelimit()) pr_err("cred ptr is null when free..\n");
		return -ENOENT;
	}

	struct mp4_security * ptr = cred->security;
	// BUG_ON(cred->security && cred->security < PAGE_) 
	if (!cred->security) 
	{
		pr_err("security struct of cred is null...\n");
		return -ENOENT; 
	}
	cred->security = (void *) 0x7UL; // TODO ? what is this memory address, low address in userspace
	kfree(ptr);
	return;
}

/**
 * mp4_cred_prepare - Prepare new credentials for modification
 *
 * @new: the new credentials
 * @old: the old credentials
 * @gfp: the atomicity of the memory allocation
 *
 */
static int mp4_cred_prepare(struct cred *new, const struct cred *old,
			    gfp_t gfp)
{
	struct mp4_security * tsec = NULL;

	if (!new) 
	{
		if (printk_ratelimit()) pr_err("no credential when prepare.\n");
		return -ENOENT;// new could be null
	}
  	/*if (!old) 
  	{
    	pr_err("old cred is null ptr...");
   	 	return -ENOENT;
 	}
	*/
	if (!old->security) 
	{
    	new->security = kzalloc(sizeof(struct mp4_security), gfp);
    	if (!new->security) 
    	{
      		pr_err("new->security alloc blank failed..");
      		return -ENOMEM;
    	}
    	tsec = (new->security);
		tsec->mp4_flags = MP4_NO_ACCESS;
		return 0;
	}
	const struct mp4_security * old_tsec = old->security;
	tsec = kmemdup(old_tsec, sizeof(struct mp4_security), gfp);

	if (!tsec) 
	{
		pr_err("kmem dump failed...\n");
		return -ENOMEM;
	}
	
	new->security = tsec;
	return 0;
}


static inline void security_sid_to_context(int sid, char ** context, size_t * len)
{
	* context = "read-write";
	* len = strlen(*context);
}

/**
 * mp4_inode_init_security - Set the security attribute of a newly created inode
 *
 * @inode: the newly created inode
 * @dir: the containing directory
 * @qstr: unused
 * @name: where to put the attribute name
 * @value: where to put the attribute value
 * @len: where to put the length of the attribute
 *
 * returns 0 if all goes well, -ENOMEM if no memory, -EOPNOTSUPP to skip
 *
 */
static int mp4_inode_init_security(struct inode *inode, struct inode *dir,
				   const struct qstr *qstr,
				   const char **name, void **value, size_t *len)
{
	// init security for newly created inode
	// qstr, including the inode/dentry name? not sure what is it for. 
	/*
	struct mp4_security * new_tsec = kmalloc(sizeof(mp4_security), GFP_KERNEL);
	if (!new_tsec) return -ENOMEM; // no mem for new mp4_sec
	*/
	struct cred * cred = current_cred();
	if (!cred) return -ENOENT;
	
	int newsid = 0;
	size_t clen = 0;
	char * context = NULL;
	
	if (!cred->security) 
	{
		if (printk_ratelimit()) pr_err("there is no security label for the current process credential");
		// int ret = mp4_cred_alloc_blank(cred, GFP_KERNEL); 
		// TODO when creating the an inode, we should 
		// if (ret < 0) return ret;
		return -ENOENT; 
	}
	struct mp4_security * sec = cred->security;
	if (sec->mp4_flags == MP4_TARGET_SID)
	{
		if (name) *name = XATTR_NAME_MP4;
		if (value && len)
		{
			// default value to be read-write 
			security_sid_to_context(newsid, &context, &clen);
			*value = context;
			*len = clen;
		}
		else
		{
			*name = NULL;
			pr_err("input pointer for len and value...\n");
			return -ENOENT; // this should not happen for value and len eithre to be null ptr
		}
	}
	return 0;
}

/**
 * mp4_has_permission - Check if subject has permission to an object
 *
 * @ssid: the subject's security id
 * @osid: the object's security id
 * @mask: the operation mask
 *
 * returns 0 is access granter, -EACCES otherwise
 *
 */
static int mp4_has_permission(int ssid, int osid, int mask)
{
// #define NOACCESS (-1)
	// mask not enabled
	if (!mask) return 0;
	
	// mask enabled, and subject is target 
	if (osid == MP4_NO_ACCESS) 
	{
		// target no access, other is accessible
		if (mask == MAY_ACCESS)
		{
			if (ssid == MP4_TARGET_SID) return -EACCES;
			else return 0;
		} 
		else return -EACCES;
	}else if (osid == MP4_READ_WRITE) 
	{
		// read, write, append by target
		// read by ANYONE
		if (mask == MAY_READ) return 0;
		else if ((mask | MAY_WRITE| MAY_APPEND) == (MAY_READ | MAY_WRITE | MAY_APPEND))
		{
			if (ssid == MP4_TARGET_SID) return 0;
			else return -EACCES;
		}
		else return -EACCES;
	}else if (osid == MP4_READ_OBJ)
	{
		// read by ANYONE
		if (mask == MAY_READ) return 0;
		else return -EACCES;
	}else if (osid == MP4_EXEC_OBJ)
	{
		// read. exec by ANYONE 
		if ((mask | MAY_EXEC | MAY_READ)== MAY_EXEC | MAY_READ) return 0;
		else return -EACCES;
	}else if (osid == MP4_READ_DIR)
	{
		// read, exec, access by ANYONE
		if ((mask | MAY_EXEC | MAY_READ | MAY_ACCESS) == MAY_EXEC | MAY_READ | MAY_ACCESS) return 0;
		else return -EACCES;
	}else if (osid == MP4_RW_DIR)
	{
		// may be modified by target program
		if ((mask | MAY_WRITE | MAY_APPEND) == MAY_WRITE | MAY_APPEND)
		{
			if (osid == MP4_TARGET_SID) return 0;
			else return -EACCES;
		}else return -EACCES;
	}
	return -EACCES;
}

/**
 * mp4_inode_permission - Check permission for an inode being opened
 *
 * @inode: the inode in question
 * @mask: the access requested
 *
 * This is the important access check hook
 *
 * returns 0 if access is granted, -EACCES otherwise
 *
 */
static int mp4_inode_permission(struct inode *inode, int mask)
{
	/* hook for inode check */ 
	
	char path_buff[256] = {0};
	int length = 256;
	struct dentry * path_de = d_find_alias(inode);
	if (!path_de) return -EACCES;

	char * path_ret = dentry_path_raw(path_de, path_buff, length);
	if (!path_ret) 
	{
		// path_ret is null is the path is found without error
		if (path_de) dput(path_de);
		return -EACCES;
	}
	// path_buff is filled with path
	if (mp4_should_skip_path(path_buff))
	{
		if (printk_ratelimit()) pr_info("inode is SKIPABLE directory, path name: %s\n", path_buff);
		if (path_de) dput(path_de);
		return 0; 
	} 
	
	struct cred * cred = current_cred();	
	struct mp4_security * sec = cred->security;
	int ssid = sec->mp4_flags;
	
	// if non target want to access directory
	if (ssid != MP4_TARGET_SID)
	{
		if (S_ISDIR(inode->i_mode))
			if (path_de) dput(path_de);
			return 0; 
		//return -1; // error code not found for access control
	}

	int osid = get_inode_sid(inode); // TODO 
	if (osid < 0) {
		if (printk_ratelimit()) pr_err("the osid is an error code");
		if (path_de) dput(path_de);
		return osid;
	}
	
	
	if (printk_ratelimit()) pr_info("inode is no skipable, path name: %s\, ssid osid mask: %d %d %dn", path_buff, ssid, osid, mask);
	int ret = mp4_has_permission(ssid, osid, mask);
	// int ret_dir_rec = dir_look(de, ssid, mask);
	if (ret == -EACCES) 
		// log the failure attempt
		if (printk_ratelimit()) pr_err("The access is denied. ssid %d, osid %d, mask %d\n", ssid, osid ,mask);
	else
		if (printk_ratelimit()) pr_err(KERN_DEBUG "The access is granted, ssid %d, osid %d, mask %d", ssid, osid, mask);
	// final dput
	if (path_de) dput(path_de);
	return ret;
}


/*
 * This is the list of hooks that we will using for our security module.
 */
static struct security_hook_list mp4_hooks[] = {
	/*
	 * inode function to assign a label and to check permission
	 */
	LSM_HOOK_INIT(inode_init_security, mp4_inode_init_security),
	LSM_HOOK_INIT(inode_permission, mp4_inode_permission),

	/*
	 * setting the credentials subjective security label when laucnhing a
	 * binary
	 */
	LSM_HOOK_INIT(bprm_set_creds, mp4_bprm_set_creds),

	/* credentials handling and preparation */
	LSM_HOOK_INIT(cred_alloc_blank, mp4_cred_alloc_blank),
	LSM_HOOK_INIT(cred_free, mp4_cred_free),
	LSM_HOOK_INIT(cred_prepare, mp4_cred_prepare)
};

static __init int mp4_init(void)
{
	/*
	 * check if mp4 lsm is enabled with boot parameters
	 */
	if (!security_module_enable("mp4"))
		return 0;

	pr_info("mp4 LSM initializing..");
	// the attr key
	pr_info(XATTR_NAME_MP4);
	
	do_something();	
	/*
	 * Register the mp4 hooks with lsm
	 */
	security_add_hooks(mp4_hooks, ARRAY_SIZE(mp4_hooks));

	return 0;
}

/*
 * early registration with the kernel
 */
security_initcall(mp4_init);
