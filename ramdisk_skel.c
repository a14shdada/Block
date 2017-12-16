#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <asm/string.h>


#define DISK_NAME "sdb0"

static int hardsect_size	= 512;
static int nsector		= 1024;

static unsigned long size; /* Size of the Disk */
static u8 *data; /* array to store Disk data */
static struct request_queue *Queue; /* Request queue for all requests */
static int major_no = 0;
static struct gendisk *gd; /* Kenrnel Representation of device */

spinlock_t lock;

int ram_disk_open(struct block_device *bd, fmode_t pos)
{
	return 0;
}
static struct block_device_operations ram_disk_fops = {
	.owner	= THIS_MODULE,
	/* int (*open) (struct block_device *, fmode_t); */
	.open	= ram_disk_open,
};

/** 
 * Step 1	: Device Setup and Driver Initialization 
 * 1.1		: Setup Memory for RAM Disk
 * 1.2 		: Setup a request queue and assign a request routine
 * 1.3		: Assign Sector size to request queue 
 * 1.4		: Acquire Major number for registering driver with VFS. 
 * 1.5		: Allocate instance of gendisk
 * 1.6		: Initialize gendisk instance with driver details 
 */

void request_routine(struct request_queue *q)
{
	pr_info("%s: Request Routine called\n",__func__);
	
}
static int __init ram_init(void)
{
	
	pr_info("%s: RAM Disk Module Loading\n",__func__);
	/* 1.1 : Setup Memory for RAM Disk*/
	size	= nsector * hardsect_size; 
	/**
 	*	vmalloc  -  allocate virtually contiguous memory
 	*	@PARAM 1:		allocation size
 	*	Allocate enough pages to cover @size from the page level
 	*	allocator and map them into contiguous kernel virtual space.
 	*
	*	For tight control over page level allocator and protection flags
 	*	use __vmalloc() instead.
 	*/	
	data	= vmalloc(size); /* vzalloc -The memory allocated is set to zero. */
	if (data == NULL) {
		pr_err("%s: Out of Memory\n",__func__);
		return -ENOMEM;
	}
	pr_info("%s: Memory Allocated for RAM Disk\n",__func__);
	/* 1.2 : Setup a request queue and assign a request routine */

	/**
	* blk_init_queue  - prepare a request queue for use with a block device
	* @rfn:  The function to be called to process requests that have been
	*        placed on the queue.
	* @lock: Request queue spin lock
	*
	* Description:
	*    If a block device wishes to use the standard request handling procedures,
	*    which sorts requests and coalesces adjacent requests, then it must
	*    call blk_init_queue().  The function @rfn will be called when there
	*    are requests on the queue that need to be processed.  If the device
	*    supports plugging, then @rfn may not be called immediately when requests
	*    are available on the queue, but may be called at some time later instead.
	*    Plugged queues are generally unplugged when a buffer belonging to one
	*    of the requests on the queue is needed, or due to memory pressure.
	*
	*    @rfn is not required, or even expected, to remove all requests off the
	*    queue, but only as many as it can handle at a time.  If it does leave
	*    requests on the queue, it is responsible for arranging that the requests
	*    get dealt with eventually.
	*
	*    The queue spin lock must be held while manipulating the requests on the
	*    request queue; this lock will be taken also from interrupt context, so irq
	*    disabling is needed for it.
	*
	*    Function returns a pointer to the initialized request queue, or %NULL if
	*    it didn't succeed.
	*
	* Note:
	*    blk_init_queue() must be paired with a blk_cleanup_queue() call
	*    when the block device is deactivated (such as at module unload).
	**/
	/* struct request_queue *blk_init_queue(request_fn_proc *rfn, spinlock_t *lock) */
	/*  void (request_fn_proc) (struct request_queue *q); */
	Queue = blk_init_queue(request_routine, &lock);
	if (Queue == NULL) {
		pr_err("%s: Failed to setup request queue\n",__func__);
		goto free_mem;
	}
	pr_info("%s: Request Queue setup successfully\n",__func__);
	/*  1.3		: Assign Sector size to request queue */
	/**
 	* blk_queue_logical_block_size - set logical block size for the queue
 	* @q:  the request queue for the device
 	* @size:  the logical block size, in bytes
 	*
 	* Description:
 	*   This should be set to the lowest possible block size that the
 	*   storage device can address.  The default of 512 covers most
 	*   hardware.
 	**/
	/* void blk_queue_logical_block_size(struct request_queue *q, unsigned short size) */
	
	blk_queue_logical_block_size(Queue, hardsect_size);
	pr_info("%s: Sector size (%d) assigned to the Queue\n",__func__, hardsect_size);
		
 	/* 1.4		: Acquire major number for registering driver with VFS */
	/**
 	* register_blkdev - register a new block device
 	*
 	* @major: the requested major device number [1..255]. If @major = 0, try to
 	*         allocate any unused major number.
 	* @name: the name of the new block device as a zero terminated string
 	*
 	* The @name must be unique within the system.
 	*
 	* The return value depends on the @major input parameter:
 	*
 	*  - if a major device number was requested in range [1..255] then the
 	*    function returns zero on success, or a negative error code
 	*  - if any unused major number was requested with @major = 0 parameter
 	*    then the return value is the allocated major number in range
 	*    [1..255] or a negative error code otherwise
 	*/
	/*  int register_blkdev(unsigned int major, const char *name) */
	major_no = register_blkdev(major_no, DISK_NAME);
	if (major_no < 0) {
		pr_info("%s: Failed to get major number\n",__func__);
		goto cleanup_queue;
	}
	pr_info("%s: Got  major number: %d\n",__func__,major_no);	
	/**
	* Side Notes: Block driver register with two layer -> VFS and Block layer
	* VFS 		-> For application (user access)
	* Block Layer	-> For File IO operation (FS access)
	*/
	
 	/* 1.5		: Allocate instance of gendisk */
	/* struct gendisk *alloc_disk(int minors) */
	gd = alloc_disk(0);
	if ( gd == NULL) {
		pr_err("%s : Out of memory, failed to get instance of gendisk\n",__func__);
		goto unregister_block_device;
	}	
	pr_info("%s: Instance of gendisk allocated\n",__func__);
 	/* 1.6		: Initialize gendisk instance with driver details */
	/* major, first_minor and minors are input parameters only,
	 * don't use directly.  Use disk_devt() and disk_max_parts().
	 */
	gd->major 	= major_no; 	/* major number of driver */
	gd->first_minor = 0; 		/* First minor number of driver */
	gd->minors	= 1;		/* maximum number of minors, =1 for
					 * disks that can't be partitioned. */
	strcpy(gd->disk_name,DISK_NAME);			
	gd->fops	= &ram_disk_fops;
	/*  	void set_capacity(struct gendisk *disk, sector_t size)
		{
		disk->part0.nr_sects = size;
		} */
	set_capacity(gd,size);
	gd->queue	= Queue;
		
	/* void add_disk(struct gendisk *disk) */
	/* add_disk in turn calls device_add_disk which does following:
	 * device_add_disk(NULL, disk); 
	*/ 
	/**
 	* device_add_disk - add partitioning information to kernel list
 	* @parent: parent device for the disk
 	* @disk: per-device partitioning information
 	*
 	* This function registers the partitioning information in @disk
 	* with the kernel.
 	*
 	*/
	/* void device_add_disk(struct device *parent, struct gendisk *disk) */
	
	add_disk(gd);
	
	goto end;
	unregister_block_device:
		unregister_blkdev(major_no, DISK_NAME);
	cleanup_queue:
		blk_cleanup_queue(Queue);
	free_mem:
		vfree(data);
	end:
		return 0;
}

static void __exit ram_exit(void)
{
	/* void del_gendisk(struct gendisk *disk) */
	del_gendisk(gd);
	
	/* void put_disk(struct gendisk *disk) */
	put_disk(gd);
	
	/* void unregister_blkdev(unsigned int major, const char *name) */
	unregister_blkdev(major_no, DISK_NAME);

	/**
	* blk_cleanup_queue - shutdown a request queue
 	* @q: request queue to shutdown
 	*
 	* Mark @q DYING, drain all pending requests, mark @q DEAD, destroy and
 	* put it.  All future requests will be failed immediately with -ENODEV.
 	*/
	/* void blk_cleanup_queue(struct request_queue *q) */
	
	blk_cleanup_queue(Queue);

	
	/**
 	*	vfree  -  release memory allocated by vmalloc()
 	*	@PARAM 1:		memory base address
 	*
 	*	Free the virtually continuous memory area starting at @addr, as
 	*	obtained from vmalloc(), vmalloc_32() or __vmalloc(). If @addr is
 	*	NULL, no operation is performed.
 	*
 	*	Must not be called in NMI context (strictly speaking, only if we don't
 	*	have CONFIG_ARCH_HAVE_NMI_SAFE_CMPXCHG, but making the calling
 	*	conventions for vfree() arch-depenedent would be a really bad idea)
 	*
 	*	NOTE: assumes that the object at @addr has a size >= sizeof(llist_node)
 	*/
	vfree(data);
	pr_info("%s: RAM Disk Module Unloaded\n",__func__);
}

module_init(ram_init);
module_exit(ram_exit);

MODULE_AUTHOR("dada_ji@gmail.com");
MODULE_DESCRIPTION("Skelton Module for RAM Disk");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

