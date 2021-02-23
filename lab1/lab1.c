#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#define DEFAULT_HISTORY_MAX_SIZE 16

MODULE_LICENSE("GPL");
MODULE_AUTHOR("P3401: Margarita Fedotova, Gleb Lebedenko");
MODULE_DESCRIPTION("IO Systems 2021 - lab 1, variant 4");
MODULE_VERSION("1.0");

static dev_t first;
static struct proc_dir_entry* entry;
static struct cdev c_dev;
static struct class *cl;

static int space_ctr;
static char* history;
static size_t size;
static size_t max_size;

static ssize_t proc_write(struct file *file, const char __user * ubuf, size_t count, loff_t* ppos)
{
  printk(KERN_DEBUG "/proc/var4: write()\n");
  return -1;
}

static ssize_t proc_read(struct file *file, char __user * ubuf, size_t count, loff_t* ppos)
{
  size_t len = size * sizeof(char);
  if (*ppos > 0 || count < len)
  {
    return 0;
  }
  if (copy_to_user(ubuf, history, len) != 0)
  {
    return -EFAULT;
  }
  *ppos = len;

  printk(KERN_INFO "/proc/var4: read()\n");
  return len;
}

static int dev_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "/dev/var4: open()\n");
  return 0;
}

static int dev_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "/dev/var4: close()\n");
  return 0;
}

static ssize_t dev_read(struct file *f, char __user *ubuf, size_t count, loff_t *off)
{
  size_t len = size * sizeof(char);
  if (*off > 0 || count < len)
  {
    return 0;
  }
  if (copy_to_user(ubuf, history, len) != 0)
  {
    return -EFAULT;
  }
  *off = len;

  printk(KERN_INFO "/dev/var4: read()\n");
  return len;
}

static ssize_t dev_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
  char c;
  size_t written_count;
  size_t i;

  for (i = 0; i < len; i++)
  {
    if (copy_from_user(&c, buf + i, 1) != 0)
    {
      return -EFAULT;
    }
    else
    {
      if (c == ' ')
      {
        space_ctr += 1;
      }
    }
  }

  written_count = sprintf(history + size, "%d\n", space_ctr);
  size += written_count;

  if (size + size > max_size)
  {
    max_size += max_size;
    history = krealloc(history, sizeof(char) * max_size, GFP_KERNEL);
  }

  printk(KERN_INFO "/dev/var4: write() [space_ctr = %d, written_count = %ld, size = %ld, max_size = %ld]\n", space_ctr, written_count, size, max_size);
  return len;
}

static struct file_operations mychdev_fops =
{
  .owner = THIS_MODULE,
  .open = dev_open,
  .release = dev_close,
  .read = dev_read,
  .write = dev_write
};

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = proc_read,
  .write = proc_write,
};


static int __init lab1_init(void)
{
  entry = proc_create("var4", 0444, NULL, &fops);
  space_ctr = 0;
  size = 0;
  max_size = DEFAULT_HISTORY_MAX_SIZE;
  history = (char*) kmalloc(sizeof(char) * max_size, GFP_KERNEL);

  if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
  {
    return -1;
  }

  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
    unregister_chrdev_region(first, 1);
    return -1;
  }

  if (device_create(cl, NULL, first, NULL, "var4") == NULL)
  {
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }

  cdev_init(&c_dev, &mychdev_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }

  printk(KERN_INFO "%s: initialization completed\n", THIS_MODULE->name);

  return 0;
}

static void __exit lab1_exit(void)
{
  cdev_del(&c_dev);
  device_destroy(cl, first);
  class_destroy(cl);
  unregister_chrdev_region(first, 1);

  kfree(history);

  proc_remove(entry);
  printk(KERN_INFO "%s: resources freed\n", THIS_MODULE->name);
}

module_init(lab1_init);
module_exit(lab1_exit);
