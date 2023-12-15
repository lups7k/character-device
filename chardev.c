#include <linux/kernel.h>
#include <linux/module.h>

#if CONFIG_MODVERSIONS == 1
#define MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/fs.h>
#include <linux/wrapper.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a, b, c) ((a) * 65536 + (b) * 256 + (c))
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 2, 0)
#include <asm/uaccess.h> // put_user
#endif

#define SUCCESS 0

#define DEVICE_NAME "char_dev"
#define BUF_LEN 80

static int Device_Open = 0;
static char Mesage[BUF_LEN];
static char *Message_Ptr;

static int device_open(struct inode *inode, struct file * file) {
	static int counter = 0;

#ifdef DEBUG
	printk("device_open: %p %p\n", inode, file);
#endif

	printk("device: %d.%d\n", inode -> i_rdev >> 8, inode -> i_rdev & 0xFF);

	if (Device_Open) return -EBUSY;
	Device_Open++;

	sprintf(Message, "If I told you once, I told you %d times - %s", counter++, "hello, world\n");

	Message_Ptr = Message;

	MOD_INC_USE_COUNT;

	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)
	static int device_release(struct inode *inode, struct file *file)
#else
	static void device_release(struct inode *inode, struct file *file)
#endif
{
#ifdef DEBUG
	printk("device_release: %p, %p\n", inode, file);
#endif
	Device_Open--;
	MOD_DEC_USE_COUNT;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)
	return 0;
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)
static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset)
#else
	static int device_read(struct inode *inode, struct file *file, char *buffer, int length)
#endif
{
	int bytes_read = 0;

	if (*Message_Ptr == 0) return 0;
	while (length && *Message_Ptr) {
		put_user(*(Message_Ptr++), buffer++);
		length--;
		bytes_read++;
	}
#ifdef DEBUG
	printk("Read %d bytes, %d left\n", bytes_read, length);
#endif
	return bytes_read;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)
static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
#else
	static int device_write(struct inode *inode, struct file *file, const char *buffer, int length)
#endif
{
	return -EINVAL;
}

static int Major;

struct file_operations Fops = {
	NULL,
	device_read,
	device_write,
	NULL,
	NULL,
	NULL,
	NULL,
	device_open,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)
	NULL,
#endif
	device_release
};
int init_module() {
	Major = module_register_chrdev(0, DEVICE_NAME, &Fops);
	if (Major < 0) {
		printk("%s device failed with %d\n Sorry, for registering the character", Major);
		return Major;
	}	

	printk("%s The major device number is %d. \n Registeration is a success.", Major);
	printk("If you want to talk to the device driver,\n you'll have to create a device file. \n We suggest you use");
	printk("mknod <name> c %d <minor>\n", Major);
	printk("You can try different minor numbers\n and see what happens.\n");
	return 0;
}
void cleanup_module() {
	int ret;
	ret = module_unregister_chrdev(Major, DEVICE_NAME);
	if (ret < 0) printk("Error in unregister_chrdev: %d\n", ret);
}
