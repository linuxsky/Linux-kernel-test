/*************************************************************************
    > File Name: kfifo_test.c
    > Author: baiy
    > Mail: baiyang0223@163.com 
    > Created Time: 2020-12-04-17:19:54
    > Func:  测试内核KFIFO
 ************************************************************************/
#define pr_fmt(fmt) "[%s:%d:]" fmt, __func__, __LINE__
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h>
#endif

#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>


#define MAX_FIFO_COUNT (PAGE_SIZE)
extern void get_random_bytes(void* buf,int nbytes);
static struct task_struct *push_thread = NULL;
static struct task_struct *pop_thread = NULL;

/* fifo size in elements (bytes) */
#define FIFO_SIZE   32
#ifdef DYNAMIC
static struct kfifo test_fifo;
#else
static DECLARE_KFIFO(test_fifo, unsigned char, FIFO_SIZE);
#endif


static int make_kfifo_push_thread(void *data)
{
     int i = 5;
	 int size = 0;
     struct completion cmpl;
     unsigned char val = 0;
     printk("[KFIFO PUSH]thread starting...\n");
     while((i--) > 0){
            init_completion(&cmpl);
            wait_for_completion_timeout(&cmpl, 3 * HZ);
			size = kfifo_len(&test_fifo);
			if(size > MAX_FIFO_COUNT) {
            	printk("[KFIFO PUSH]: size %d,skip push \n", size);
				continue;
			}
            get_random_bytes(&val,sizeof(val));  //生成一个内核随机数
			kfifo_put(&test_fifo, val);
            printk("[KFIFO PUSH]: %u, size %d \n",val, size);
     }
     printk("[KFIFO PUSH] ended!\n");
     return 0;
}
static int make_kfifo_pop_thread(void *data)
{
     int i = 100;
	 int size = 0;
     struct completion cmpl;
     unsigned char val = 0;
     printk("[KFIFO POP]thread starting...\n");
     while((i--) > 0){
            init_completion(&cmpl);
            wait_for_completion_timeout(&cmpl, 1 * HZ);
			size = kfifo_len(&test_fifo);
			if(size <= 0) {
            	printk("[KFIFO POP]: size %d,skip pop \n", size);
				continue;
			}
			kfifo_get(&test_fifo, &val);
            printk("[KFIFO POP]: %u, size %d \n",val, size);
     }
     printk("[KFIFO POP] ended!\n");
     return 0;
}


static int __init kfifo_dev_init(void)
{
#ifdef DYNAMIC
	int ret;

	ret = kfifo_alloc(&test_fifo, FIFO_SIZE, GFP_KERNEL);
	if (ret) {
		printk(KERN_ERR "error kfifo_alloc\n");
		return ret;
	}
#else
	INIT_KFIFO(test_fifo);
#endif
	pr_info("kfifo init");
	push_thread = kthread_run(make_kfifo_push_thread,NULL,"kfifo push");
	pop_thread = kthread_run(make_kfifo_pop_thread,NULL,"kfifo pop");
    return 0;
}

static void __exit kfifo_dev_exit(void)
{
#ifdef DYNAMIC
	kfifo_free(&test_fifo);
#endif
	pr_info("kfifo exit");
}


module_init(kfifo_dev_init);
module_exit(kfifo_dev_exit);

MODULE_LICENSE("GPL v2");
MODULE_INFO(supported, "Test driver that simulate serial port over PCI");
