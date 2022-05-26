#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <asm/current.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Rusin");
MODULE_DESCRIPTION("catalog chardev");

const char* DEVICE_NAME = "my_queue";
const char* CLASS_NAME  = "my_queue_class";
#define BUFF_SIZE 255

#define MAGIC 17171717

int             init_module(void);
void            exit_module(void);
static int      device_open(struct inode*, struct file*);
static int      device_release(struct inode*, struct file*);
static ssize_t  device_read(struct file*, char*, size_t, loff_t*);
static ssize_t  device_write(struct file*, const char*, size_t, loff_t*);

static int  major                   = 0;
static int  dev_open                = 0;
static char message[BUFF_SIZE + 1]  = "INIT Queue\n";
static struct class* char_class     = NULL;
static struct device* char_dev      = NULL;
static char* str = NULL;

static int ticket_counter = 1;

struct list_head tickets_head;
struct ticket_node {
    struct list_head tickets_head;
    pid_t  pid;
    int    ticket;
};

static void set_new_ticket(pid_t pid, int new_ticket) {
    struct list_head* ptr;
    struct ticket_node* node;

    list_for_each(ptr, &tickets_head) {
        node = list_entry(ptr, struct ticket_node, tickets_head);
        if (node->pid == pid) {
            node->ticket = new_ticket;
            return;
        }
    }

    struct ticket_node* new_node = kmalloc(sizeof(struct ticket_node), GFP_KERNEL);
    new_node->pid = pid;
    new_node->ticket = new_ticket;

    list_add(&new_node->tickets_head, &tickets_head);
}

static int get_ticket(pid_t pid) {
    struct list_head*   ptr;
    struct ticket_node* node;

    list_for_each(ptr, &tickets_head) {
        node = list_entry(ptr, struct ticket_node, tickets_head);
        if (node->pid == pid) {
            return node->ticket;
        }
    }
    return -1;
}

struct list_head strs_head;
struct str_node {
    struct list_head strs_head;
    char*  str;
    int    ticket;
};

static void insert_new_str(int ticket, char* str) {
    printk("insert new string: %s\n", str);

    struct str_node *new_node = kmalloc(sizeof(struct str_node), GFP_KERNEL);
    new_node->ticket = ticket;
    new_node->str = str;

    struct list_head* ptr;
    struct str_node*  node;

    list_for_each(ptr, &strs_head) {
        node = list_entry(ptr, struct str_node, strs_head);
        if (node->ticket > ticket) {
            list_add_tail(&new_node->strs_head, &node->strs_head);
            return;
        }
    }   

    printk(KERN_ALERT "unreachable: insert_new_str()");
}

static char* get_str(void) {
    if (((struct str_node*)strs_head.next)->ticket == MAGIC) {
        printk("empty!");
        return NULL;
    }

    char* str = ((struct str_node*)strs_head.next)->str;
    __list_del_entry(strs_head.next);
    printk("get_str: %s\n", str);
    return str;
}

static struct file_operations fops = {
    .read    = device_read,
    .write   = device_write,
    .open    = device_open,
    .release = device_release
};

int init_module(void) {
    INIT_LIST_HEAD(&tickets_head);
    INIT_LIST_HEAD(&strs_head);
    
    {
        struct str_node* new_node = kmalloc(sizeof(struct str_node), GFP_KERNEL);
        new_node->str = "FAKE";
        new_node->ticket = MAGIC;

        list_add(&new_node->strs_head, &strs_head);
    }

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

    str = kmalloc(19, GFP_KERNEL);
    memcpy(str, "tello->dyn->string", 19);

    return 0;
}

void exit_module(void) {
    device_destroy(char_class, MKDEV(major, 0));
    class_unregister(char_class);
    class_destroy(char_class);
    unregister_chrdev(major, DEVICE_NAME);

    printk(KERN_ALERT "Shutdown of chardev module\n");

    kfree(str);
    str = NULL;
}

static int device_open(struct inode* inode, struct file* filp) {

    if (dev_open > 10) {
        return -EBUSY;
    }

    dev_open++;
    printk(KERN_INFO "Device has been opened %d time(s)\n", dev_open);

    set_new_ticket(current->pid, ticket_counter++);
    
    return 0;
}

static int device_release(struct inode* inode, struct file* filp) {

    dev_open--;
    printk(KERN_INFO "Device successfully closed (now open %d)\n", dev_open);

    return 0;
}

static ssize_t device_read(struct file* filp, char* buffer, size_t length, loff_t* offset) {

    loff_t pos = *offset;
	size_t ret;

    if (pos > 0) {
        return 0;
    }

    char* str = get_str();
    if (str == NULL) {
        printk(KERN_INFO "Queue is empty!");
        return 0;
    }

	if (pos < 0) {
		return -EINVAL;
    }

	if (pos >= BUFF_SIZE || length == 0) {
		return 0;
    }

	if (length > BUFF_SIZE - pos) {
		length = BUFF_SIZE - pos;
    }

    size_t str_len = strlen(str);
    if (pos >= str_len) {
        return 0;
    }
    
    if (str_len < length) {
        length = str_len;
    }

	ret = copy_to_user(buffer, str + pos, length);
    if (ret != 0) {
        return -EFAULT;
    }

    printk(KERN_INFO "form pos: %d, len: %ld\n", pos, length);
    printk(KERN_INFO "!!: %s\n", str);

    kfree(str);

	*offset = pos + length;
	return length;
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

    printk(KERN_INFO "read: length: %zu\n", length);
    printk(KERN_INFO "pos: length: %llu\n", pos);

    printk(KERN_INFO "WRITTEN: %s\n", message + pos);

    // parse request
    // parse_request();
    char* str = (char*)kmalloc(length, GFP_KERNEL);
    strncpy(str, message + pos, length + 1);
    insert_new_str(get_ticket(current->pid), str);

    memset(message, 0, BUFF_SIZE);

	*offset = pos + length;
	return length;
}

// module_init(init_module);
module_exit(exit_module);
