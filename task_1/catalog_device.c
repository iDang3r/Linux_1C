#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "catalog.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Rusin");
MODULE_DESCRIPTION("catalog chardev");

const char* DEVICE_NAME = "my_catalog";
const char* CLASS_NAME  = "my_catalog_class";
#define BUFF_SIZE 255

int             init_module(void);
void            exit_module(void);
static int      device_open(struct inode*, struct file*);
static int      device_release(struct inode*, struct file*);
static ssize_t  device_read(struct file*, char*, size_t, loff_t*);
static ssize_t  device_write(struct file*, const char*, size_t, loff_t*);

static int  major                   = 0;
static int  dev_open                = 0;
static char message[BUFF_SIZE + 1]  = "INIT Catalog\n";
static struct class* char_class     = NULL;
static struct device* char_dev      = NULL;

static struct file_operations fops = {
    .read    = device_read,
    .write   = device_write,
    .open    = device_open,
    .release = device_release
};

int init_module() {

    major = register_chrdev(0, DEVICE_NAME, &fops);

    if (major < 0) {
        printk(KERN_ALERT "'register_chrdev' failed with %d\n", major);
        return major;
    }
    printk(KERN_INFO "I was assigned major number %d.\n", major);

    char_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(char_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(char_class);
    }
    printk(KERN_INFO "Device class registered correctly\n");

    char_dev = device_create(char_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(char_dev)) {
        class_destroy(char_class);
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(char_dev);
    }
    printk(KERN_INFO "Device created correctly\n");

    return 0;
}

void exit_module() {

    device_destroy(char_class, MKDEV(major, 0));
    class_unregister(char_class);
    class_destroy(char_class);
    unregister_chrdev(major, DEVICE_NAME);

    printk(KERN_ALERT "Shutdown of chardev module\n");
}

static int device_open(struct inode* inode, struct file* filp) {

    if (dev_open) {
        return -EBUSY;
    }

    dev_open++;
    printk(KERN_INFO "Device has been opened %d time(s)\n", dev_open);
    
    return 0;
}

static int device_release(struct inode* inode, struct file* filp) {

    dev_open--;
    printk(KERN_INFO "Device successfully closed (now open %d)\n", dev_open);

    return 0;
}

static ssize_t device_read(struct file* filp, char* buffer, size_t length, loff_t* offset) {

    my_sayHello();

    loff_t pos = *offset;
	size_t ret;

	if (pos < 0) {
		return -EINVAL;
    }

	if (pos >= BUFF_SIZE || length == 0) {
		return 0;
    }

	if (length > BUFF_SIZE - pos) {
		length = BUFF_SIZE - pos;
    }

    size_t msg_len = strlen(message);
    if (pos >= msg_len) {
        return 0;
    }
    
    if (msg_len < length) {
        length = msg_len;
    }

	ret = copy_to_user(buffer, message + pos, length);
    if (ret != 0) {
        return -EFAULT;
    }

    printk(KERN_INFO "!!: %s\n", message);

	*offset = pos + length;
	return length;
}

void parse_request(void) {
    char option[8];
    sscanf(message, "%s", option);
    printk("GOT OPTION: %s", option);
    if (!strcmp(option, CATALOG_ADD)) {

        UserCatalog user;

        if (4 != sscanf(message + sizeof(CATALOG_ADD), "%s %s %u %s", user.name.str, user.surname.str, &user.age, user.phone.str)) {
            printk(KERN_ALERT "CATALOG_ADD failed: invalid arguments");
            return;
        }

        user.name.len    = strnlen(user.name.str,    CATALOG_MAX_LEN);
        user.surname.len = strnlen(user.surname.str, CATALOG_MAX_LEN);
        user.phone.len   = strnlen(user.phone.str,   CATALOG_MAX_LEN);

        if (-1 == catalog_add_user(&user)) {
            memcpy(message, "can't add user\n", 16);
            return;
        }

    } else if (!strcmp(option, CATALOG_DEL)) {

        char surname[CATALOG_MAX_LEN];
        int len = 0;
        if (1 != sscanf(message + sizeof(CATALOG_DEL), "%s%n", surname, &len)) {
            printk(KERN_ALERT "CATALOG_DEL failed: invalid arguments");
            return;
        }

        if (-1 == catalog_del_user(surname, len)) {
            memcpy(message, "no such user to delete\n", 24);
            return;
        }

    } else if (!strcmp(option, CATALOG_GET)) {

        char surname[CATALOG_MAX_LEN];
        int len = 0;
        if (1 != sscanf(message + sizeof(CATALOG_GET), "%s%n", surname, &len)) {
            printk(KERN_ALERT "CATALOG_GET failed: invalid arguments");
            return;
        }

        UserCatalog user;
        memset(&user, 0, sizeof(UserCatalog));
        if (-1 == catalog_get_user(surname, len, &user)) {
            memcpy(message, "can't find user\n", 17);
            return;
        }

        snprintf(message, BUFF_SIZE, "name: %s, surname: %s, age: %d, phone: %s\n", 
            user.name.str, user.surname.str, user.age, user.phone.str);

    } else {
        printk(KERN_ALERT "Unrecognized option in catalog_device (see catalog.h)");
        memcpy(message, "INCORRECT\n", 10);
    }
}

static ssize_t device_write(struct file* filp, const char* buffer, size_t length, loff_t* offset) {

    loff_t pos = *offset;
	size_t res;

	if (pos < 0) {
		return -EINVAL;
    }

	if (pos >= BUFF_SIZE || length == 0) {
		return 0;
    }

	if (length > BUFF_SIZE - pos) {
		length = BUFF_SIZE - pos;
    }

	res = copy_from_user(message + pos, buffer, length);
	if (res != 0) {
		return -EFAULT;
    }
    memset(message + pos + length, '\0', BUFF_SIZE - pos - length);

    printk(KERN_INFO "read: length: %zu\n", res);
    printk(KERN_INFO "pos: length: %llu\n", pos);

    // parse request
    parse_request();

	*offset = pos + length;
	return length;
}

// module_init(init_module);
module_exit(exit_module);
