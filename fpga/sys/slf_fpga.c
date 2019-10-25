/*
 * Copyright (c) 2019 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

# include  <linux/module.h>
# include  <linux/of_device.h>
# include  <linux/fs.h>
# include  <linux/interrupt.h>
# include  <linux/io.h>
# include  <linux/sched.h>
# include  <linux/sched/signal.h>
# include  <linux/uaccess.h>
# include  <linux/wait.h>

# include  "slf_fpga.h"

# define DRIVER_NAME "slf_fpga"

static int slf_fpga_major = -1;

typedef enum {
      ADDR_BUILD_ID  = 0x00,
      ADDR_LEDs      = 0x04,
      ADDR_UserIn    = 0x08,
      ADDR_UserInExp = 0x0c,
      ADDR_UserInIEN = 0x10
} slf_fpga_addr_t;

/*
 * The instance state for a device is represented by an instance of
 * this struct. The probe creates an instance and stashes device state
 * here, and the remove cleans up and deletes the instance.
 *
 * Also include here some convenience functions that use the device
 * instance to perform basic operations on the hardware, including
 * binding the instance to the opened file, reading/writing registers
 * in the mapped region.
 */
# define SLF_FPGA_DEVICE_MAX 1
static struct slf_fpga_instance {
      void __iomem * base;

      wait_queue_head_t userin_sync;

} slf_instance_table[SLF_FPGA_DEVICE_MAX] = {
      { .base = 0 }
};

static struct slf_fpga_instance*select_device(int minor)
{
      if (minor < 0) return 0;
      if (minor >= SLF_FPGA_DEVICE_MAX) return 0;
      return slf_instance_table + minor;
}

static inline void file_set_private_data(struct file*filp, struct slf_fpga_instance*xsp)
{
      filp->private_data = xsp;
}

static inline struct slf_fpga_instance*file_get_private_data(struct file*filp)
{
      struct slf_fpga_instance*xsp = (struct slf_fpga_instance*) (filp->private_data);
      return xsp;
}

inline uint32_t slf_fpga_read32(struct slf_fpga_instance*xsp, slf_fpga_addr_t offset)
{
      void __iomem*addr = xsp->base + offset;
      return ioread32(addr);
}

inline void slf_fpga_write32(struct slf_fpga_instance*xsp, slf_fpga_addr_t offset, uint32_t val)
{
      void __iomem*addr = xsp->base + offset;
      iowrite32(val, addr);
}

/*
 * These are the operations in the file_ops structure. These give the
 * device driver its behavior from the user-mode perspective.
 */
static int slf_fpga_open(struct inode*inode, struct file*filp)
{
      struct slf_fpga_instance*xsp = select_device(MINOR(inode->i_rdev));
      if (xsp == 0) return -ENODEV;
      file_set_private_data(filp, xsp);
      return 0;
}

/*
 * The release is called when the last threae closes the fd for the
 * device. Detatch the instance from the file pointer.
 */
static int slf_fpga_release(struct inode*inodep, struct file*filp)
{
      struct slf_fpga_instance*xsp = file_get_private_data(filp);

	/* Make sure interrupts are off. */
      slf_fpga_write32(xsp, ADDR_UserInIEN, 0x00);

      file_set_private_data(filp, 0);
      return 0;
}

/*
 * Write to the leds register.
 */
static long slf_fpga_leds_ioctl(struct slf_fpga_instance*xsp, unsigned long raw)
{
      struct slf_fpga_leds_s arg;
      if (copy_from_user(&arg, (void __user*)raw, sizeof arg) != 0)
	    return -EFAULT;

      slf_fpga_write32(xsp, ADDR_LEDs, arg.led_value);
      return 0;
}

/*
 * Read from the UserIn (buttons) register.
 */
static long slf_fpga_userin_ioctl(struct slf_fpga_instance*xsp, unsigned long raw)
{
      struct slf_fpga_UserIn_s arg;
      arg.user_in_value = slf_fpga_read32(xsp, ADDR_UserIn);
      if (copy_to_user((void __user*)raw, &arg, sizeof arg) != 0)
	    return -EFAULT;

      return 0;
}

static long slf_fpga_wait_ioctl(struct slf_fpga_instance*xsp, unsigned long raw)
{
      long rc = 0;
      struct slf_fpga_wait_s arg;
      if (copy_from_user(&arg, (void __user*)raw, sizeof arg) != 0)
	    return -EFAULT;

	/* Make sure interrupts are enabled.
	   NOTE 1: Currently, the hardware only has 8 buttons.
	   NOTE 2: The interrupts will be cleared on release, so we
	   don't need to turn them off again here. In fact, turning
	   interrupts off here may mess with parallel threads. */
      slf_fpga_write32(xsp, ADDR_UserInIEN, 0xff);

	/* Wait for the input value to be different from the
	   expected value. */
      struct wait_queue_entry wait_cell;
      init_waitqueue_entry(&wait_cell, current);
      add_wait_queue(&xsp->userin_sync, &wait_cell);
      for (;;) {
	    set_current_state(TASK_INTERRUPTIBLE);
	    rc = 0;

	      /* Get the current input value. If it differs from the
		 expected value, then we are done, break out of the
		 wait loop. */
	    arg.user_in_value = slf_fpga_read32(xsp, ADDR_UserIn);
	    if (arg.user_in_value != arg.user_in_exp)
		  break;

	    rc = -ERESTART;
	    if (signal_pending(current))
		break;

	    schedule();
      }
      set_current_state(TASK_RUNNING);
      remove_wait_queue(&xsp->userin_sync, &wait_cell);

      if (rc < 0)
	    return rc;

	/* Send the results back to the user. */
      if (copy_to_user((void __user*)raw, &arg, sizeof arg) != 0)
	    return -EFAULT;

      return 0;
}

static long slf_fpga_ioctl(struct file*filp, unsigned int cmd, unsigned long raw)
{
      struct slf_fpga_instance*xsp = file_get_private_data(filp);
      switch (cmd) {
	  case SLF_FPGA_LEDS:   return slf_fpga_leds_ioctl(xsp, raw);
	  case SLF_FPGA_UserIn: return slf_fpga_userin_ioctl(xsp, raw);
	  case SLF_FPGA_WAIT:   return slf_fpga_wait_ioctl(xsp, raw);
	  default:              return -ENOTTY;
      }
}

static irqreturn_t slf_fpga_isr(int irq, void*dev_id)
{
      struct slf_fpga_instance*xsp = (struct slf_fpga_instance*)dev_id;
      uint32_t user_in = slf_fpga_read32(xsp, ADDR_UserIn);
      slf_fpga_write32(xsp, ADDR_UserInExp, user_in);

	/* Wake up threads that may be waiting for button changes. */
      wake_up_interruptible(&xsp->userin_sync);

      return IRQ_HANDLED;
}

const struct file_operations slf_fpga_ops = {
      .open           = slf_fpga_open,
      .release        = slf_fpga_release,
      .unlocked_ioctl = slf_fpga_ioctl,
      .owner          = THIS_MODULE
};

/*
 * The driver calls the probe function for all the devices in the
 * device-tree that are compatible with this driver. We get the
 * information about the devices, and bind it to the instance.
 */
static int slf_fpga_probe(struct platform_device*dev)
{
      struct resource *res;
      struct slf_fpga_instance*xsp = slf_instance_table;
      if (xsp->base) {
	    printk(KERN_INFO "%s: Can only have one device instance!\n", DRIVER_NAME);
	    return -ENODEV;
      }

      init_waitqueue_head(&xsp->userin_sync);

      res = platform_get_resource(dev, IORESOURCE_MEM, 0);
      if (res == 0) {
	    printk(KERN_INFO DRIVER_NAME ": no memory resource.\n");
	    xsp->base = 0;
	    return -ENODEV;
      }
      printk(KERN_INFO DRIVER_NAME ": Memory at %pr\n", res);

	/* Map the registers. Return an error if the map fails. The
	   devm_* variant here arranges for the resources to be
	   automatically unmapped and released if the driver is
	   removed. */
      xsp->base = devm_ioremap_resource(&dev->dev, res);
      if (IS_ERR(xsp->base)) {
	    int rc = PTR_ERR(xsp->base);
	    xsp->base = 0;
	    return rc;
      }

	/* Make sure the device is in a ready, but quiet, state. */
      slf_fpga_write32(xsp, ADDR_UserInIEN, 0x00000000);

	/* Bind the interrupt request to the interrupt handler. */
      res = platform_get_resource(dev, IORESOURCE_IRQ, 0);
      if (res == 0) {
	    printk(KERN_INFO DRIVER_NAME ": no IRQ resource.\n");
      } else {
	    printk(KERN_INFO DRIVER_NAME ": IRQ at %pr\n", res);
	    unsigned int use_irq = res->start;
	    int rc = devm_request_irq(&dev->dev, use_irq, slf_fpga_isr,
				      IRQF_SHARED, DRIVER_NAME, xsp);
	    if (rc < 0) printk(KERN_INFO DRIVER_NAME ": IRQ request failed.\n");
      }

      
	/* This binds the instance pointer to the device pointer, so
	   that we can get at it later. */
      platform_set_drvdata(dev, xsp);

      u32 tmp = slf_fpga_read32(xsp, 0);
      printk(KERN_INFO DRIVER_NAME ": BUILD ID = %u\n", tmp);

      return 0;
}

/*
 * This is called when the driver is being removed from the
 * system. Make sure the device (if bound) is in a safe state, and
 * release the instance data.
 */
static int slf_fpga_remove(struct platform_device*dev)
{
      struct slf_fpga_instance*xsp = platform_get_drvdata(dev);
      if (xsp) {
	      /* Make sure device is in a safe state. */
	    slf_fpga_write32(xsp, ADDR_UserInIEN, 0x00000000);

	    xsp->base = 0;
	    xsp = 0;
      }

      return 0;
}

/*
 * The match table lists the devices that might be in a device tree
 * that this driver is compatible with. The platform_driver structure
 * points to this table, and the platform_driver_register() function
 * will look for devices in the tree that match entries in this table,
 * and will call the probe function for devices that match.
 */
static const struct of_device_id slf_fpga_match_table[] = {
      { .compatible = "xlnx,SLF-FPGA-1.0", .data = 0 },
      { }
};

/*
 * Describe the driver to the kernel. In particular, tell the kernel
 * the driver name that we use, point out the devices that the driver
 * can be matched to (compatible) and point to functions that are
 * invoked for compatible devices that are found and are to be bound
 * to the driver.
 */
static struct platform_driver slf_fpga_driver = {
      .driver = {
	    .name = DRIVER_NAME,
	    .of_match_table = slf_fpga_match_table,
      },
      .probe = slf_fpga_probe,
      .remove = slf_fpga_remove,
};

int init_module(void)
{
      int rc;

      rc = platform_driver_register(&slf_fpga_driver);
      if (rc < 0) return rc;

      slf_fpga_major = register_chrdev(0, DRIVER_NAME, &slf_fpga_ops);
      if (slf_fpga_major < 0) {
	    return slf_fpga_major;
      }

      return 0;
}

void cleanup_module(void)
{
      if (slf_fpga_major >= 0) {
	    unregister_chrdev(slf_fpga_major, DRIVER_NAME);
	    slf_fpga_major = -1;
      }
      platform_driver_unregister(&slf_fpga_driver);
}

MODULE_LICENSE("GPL");
