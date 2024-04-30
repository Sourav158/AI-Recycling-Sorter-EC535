
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>          // Required for the GPIO functions
#include <linux/interrupt.h>     // Required for the IRQ code
#include <linux/timer.h>
#include <linux/uaccess.h>       // Required for copy_to_user function

#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system_misc.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>

#define DEVICE_NAME "mysorter"
#define CLASS_NAME "sorter"

static int majorNumber = 61;
static struct class*  sorterClass  = NULL;
static struct device* sorterDevice = NULL;

// GPIO for button (assuming BCM numbering)
static int button_gpio = 26;
static unsigned int irq_number_btn;
static int button_flag = 0;
static int wait_flag = 0;       //while waiting for model

/* Buffer to store data */
static char *nibbler_buffer;

/* length of the current message */
static int nibbler_len;

// Prototypes for device functions
static int     dev_open(struct inode *inode, struct file *filp);
static int     dev_release(struct inode *inode, struct file *filp);
static ssize_t dev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t dev_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int __init mysorter_init(void);
static void __exit mysorter_exit(void);
//static void __exit mysorter_exit(void);

static struct file_operations fops =
{
    owner: THIS_MODULE,
    open: dev_open,
    read: dev_read,
    release: dev_release,
    write: dev_write,
};

static irq_handler_t  button_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
//static irq_handler_t  ped_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

module_init(mysorter_init);
module_exit(mysorter_exit);

static int __init mysorter_init(void) {
    // Try to dynamically allocate a major number for the device
    int result;
    result = register_chrdev(majorNumber, DEVICE_NAME, &fops);
    if (result < 0){
        printk(KERN_ALERT "MySorter failed to register a major number\n");
        return majorNumber;
    }
    
    // Register the device class
    sorterClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sorterClass)){
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(sorterClass);
    }
    printk(KERN_INFO "Mysorter: device class registered correctly\n");

    // Register the device driver
    sorterDevice = device_create(sorterClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(sorterDevice)){
        class_destroy(sorterClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(sorterDevice);
    }
    printk(KERN_INFO "Mysorter: device class created correctly\n");
    

    // Initialize the timer
    timer_setup(&my_timer, sorter_timer_callback, 0);

    gpio_request(button_gpio, "sorter_init");       // Set up the GPIOs for the button
    gpio_direction_input(button_gpio);        // Set the button GPIO to be an input
    gpio_set_debounce(button_gpio, 50);      // Debounce the button with a delay of 200ms

    gpio_export(button_gpio, false);       
    irq_number_btn = gpio_to_irq(button_gpio);    // Map GPIO to IRQ number
    request_irq(irq_number_btn, (irq_handler_t) button_irq_handler, IRQF_TRIGGER_RISING, "mysorter_button_handler", NULL);             

    nibbler_buffer = kmalloc(128, GFP_KERNEL); 
	if (!nibbler_buffer)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
	} 
	memset(nibbler_buffer, 0, 128);
	nibbler_len = 0;
    return 0;
}

static void __exit mysorter_exit(void){
    del_timer(&my_timer);                     // Ensure the timer is cancelled and deleted
    

    gpio_unexport(button_gpio);
    
    free_irq(irq_number_btn, NULL);               // Free the IRQ number, no *dev_id required in this case
    gpio_free(button_gpio);
    device_destroy(sorterClass, MKDEV(majorNumber, 0)); // remove the device
    class_unregister(sorterClass);                       // unregister the device class
    class_destroy(sorterClass);                          // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);          // unregister the major number
    if (nibbler_buffer)
	{
		kfree(nibbler_buffer);
	}
    printk(KERN_INFO "Mysorter: Goodbye from the LKM!\n");
}


static irq_handler_t button_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {
    button_flag = 1;
    return (irq_handler_t) IRQ_HANDLED; // Announce that the IRQ has been handled correctly
}

static int dev_open(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "Mysorter: Device has been opened\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buf, size_t count, loff_t *f_pos){
    char buffer[256];
    int len;

    if (button_flag == 1) {
        if (wait_flag == 1) {
            len = snprintf(buffer, sizeof(buffer), "0");
            button_flag = 0;
        }
        else {
            len = snprintf(buffer, sizeof(buffer), "1");    //telling script button pressed
            button_flag = 0;
            wait_flag = 1;
        }
    
    }
    else {
        len = snprintf(buffer, sizeof(buffer), "0");
    }

    if (*f_pos >= len)
        return 0;

    if (count > len - *f_pos)
        count = len - *f_pos;

    if (copy_to_user(buf, buffer + *f_pos, count))
        return -EFAULT;

    *f_pos += count;

    return count;
}

static ssize_t dev_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	int temp;
	char tbuf[256], *tbptr = tbuf;

	/* do not eat more than a bite */
	if (count > 128) count = 128;

	/* do not go over the end */
	if (count > 128 - *f_pos)
		count = 128 - *f_pos;

	if (copy_from_user(nibbler_buffer + *f_pos, buf, count))
	{
		return -EFAULT;
	}

	for (temp = *f_pos; temp < count + *f_pos; temp++) {					  
		tbptr += sprintf(tbptr, "%c", nibbler_buffer[temp]);	
        if (nibbler_buffer[temp] == "0") {   //cardboard
            
        }
        else if (nibbler_buffer[temp] == "1") {     //glass

        }
        else if (nibbler_buffer[temp] == "2") {     //metal

        }
        else if (nibbler_buffer[temp] == "3") {     //paper

        }
        else if (nibbler_buffer[temp] == "4") {     //plastic

        }
        else {      //trash

        }
    }
    wait_flag = 0;
    printk("RESULT IS: %c\n",nibbler_buffer[temp]);
    *f_pos += strlen(tbptr);
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "MySorter: Device successfully closed\n");
    return 0;
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jared Solis");
MODULE_DESCRIPTION("A simple Linux driver for a recyclable sorter");
MODULE_VERSION("0.1");