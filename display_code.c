#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/mutex.h>

#include <linux/uaccess.h> // copy_from/to_user
#include <asm/uaccess.h> // ^same
#include <linux/sched.h> // for timers
#include <linux/jiffies.h> // for jiffies global variable
#include <linux/string.h> // for string manipulation functions
#include <linux/ctype.h> // for isdigit

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Draw a rectangle");

#define CYG_FB_DEFAULT_PALETTE_BLUE         0x01
#define CYG_FB_DEFAULT_PALETTE_RED          0x04
#define CYG_FB_DEFAULT_PALETTE_WHITE        0x0F
#define CYG_FB_DEFAULT_PALETTE_LIGHTBLUE    0x09
#define CYG_FB_DEFAULT_PALETTE_BLACK        0x00

// GRAPHICS VARIABLES
struct fb_info info;
struct fb_fillrectblank;

// Helper function borrowed from drivers/video/fbdev/core/fbmem.c /
// Simple bitmap for numeric characters 0-9, each character is 8x12 pixels.
static const uint8_t font_data[10][12] = {
    {0x7E, 0x81, 0x81, 0x81, 0x7E, 0x00},  // 0
    {0x00, 0x82, 0xFF, 0x80, 0x00, 0x00},  // 1
    {0x82, 0xC1, 0xA1, 0x91, 0x8E, 0x00},  // 2
    {0x42, 0x89, 0x89, 0x89, 0x76, 0x00},  // 3
    {0x1F, 0x10, 0x10, 0xFF, 0x10, 0x00},  // 4
    {0x4F, 0x89, 0x89, 0x89, 0x71, 0x00},  // 5
    {0x7E, 0x89, 0x89, 0x89, 0x72, 0x00},  // 6
    {0x01, 0xF1, 0x09, 0x05, 0x03, 0x00},  // 7
    {0x76, 0x89, 0x89, 0x89, 0x76, 0x00},  // 8
    {0x4E, 0x91, 0x91, 0x91, 0x7E, 0x00}   // 9
};

// Draw a character on the framebuffer
void draw_char(struct fb_info *info, uint8_t character, int x, int y, uint32_t color) {
    if (character < '0' || character > '9') return;  // Check if the character is a number
    uint8_t *bitmap = font_data[character - '0'];
    for (int cy = 0; cy < 12; cy++) {
        for (int cx = 0; cx < 8; cx++) {
            if (bitmap[cy] & (0x80 >> cx)) {
                uint32_t *pixel = (uint32_t *)(info->screen_base + (x + cx + (y + cy) * info->var.xres) * 4);
                *pixel = color;
            }
        }
    }
}
// Draw a string of characters on the framebuffer
void draw_string(struct fb_info *info, char *str, int x, int y, uint32_t color) {
    while (*str) {
        draw_char(info, *str, x, y, color);
        x += 8;  // Move to the right for the next character
        str++;
    }
}

static ssize_t show_counts(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", global_counts);
}

static ssize_t store_counts(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    strncpy(global_counts, buf, min(count, sizeof(global_counts)-1));
    global_counts[count] = '\0';
    return count;
}


static struct fb_infoget_fb_info(unsigned int idx)
{
    struct fb_info *fb_info;

    if (idx >= FB_MAX)
        return ERR_PTR(-ENODEV);

    fb_info = registered_fb[idx];
    if (fb_info)
        atomic_inc(&fb_info->count);

    return fb_info;
}
static struct kobject *kobj;

static int __init display_init(void) {
    int ret;

    // Create a kobject linked to the kernel's kobj, this will appear in /sys/kernel/
    kobj = kobject_create_and_add("display", kernel_kobj);
    if (!kobj)
        return -ENOMEM;

    // Create the sysfs file
    ret = sysfs_create_file(kobj, &counts_attribute.attr);
    if (ret) {
        kobject_put(kobj);
        return ret;
    }

    return 0;
}

static void __exit display_exit(void) {
    // Remove the sysfs file and kobject during cleanup
    kobject_put(kobj);
}

module_init(display_init);
module_exit(display_exit);

/*static int init hellofb_init(void)
{
    printk(KERN_INFO "Hello framebuffer!\n");

    // Draw a rectagle
        blank = kmalloc(sizeof(struct fb_fillrect), GFP_KERNEL);
    blank->dx = 0;
    blank->dy = 0;
    blank->width = 40;
    blank->height = 100;
    blank->color = CYG_FB_DEFAULT_PALETTE_RED;
    blank->rop = ROP_COPY;
    info = get_fb_info(0);
    lock_fb_info(info);
    sys_fillrect(info, blank);
    unlock_fb_info(info);

    return 0;
}

static void exit hellofb_exit(void) {

    // Cleanup framebuffer graphics
    kfree(blank);

    printk(KERN_INFO "Goodbye framebuffer!\n");
    printk(KERN_INFO "Module exiting\n");
}

module_init(hellofb_init);
module_exit(hellofb_exit);*/