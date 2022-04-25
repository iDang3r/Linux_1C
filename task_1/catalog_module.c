#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <uapi/linux/string.h>
#include "catalog.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Rusin");
MODULE_DESCRIPTION("Cotalog module (storage)");
MODULE_VERSION("0.17");

struct list_head catalog_head;

struct catalog_node {
    struct list_head catalog_head;
    UserCatalog user;
};

static int __init catalog_module_init(void) {
    printk(KERN_INFO "Catalog module init!\n");
    INIT_LIST_HEAD(&catalog_head);
    return 0;
}
static void __exit catalog_module_exit(void) {
    printk(KERN_INFO "Catalog module exit!\n");
}

void my_sayHello(void) {
    printk(KERN_INFO "Hello, world from Catalog_module\n");
}
EXPORT_SYMBOL(my_sayHello);

long catalog_get_user(const char* surname, unsigned int len, UserCatalog* output_data) {
    struct list_head *ptr;
    struct catalog_node *node;

    if (list_empty(&catalog_head)) {
        return -1;
    }

    list_for_each(ptr, &catalog_head) {
        node = list_entry(ptr, struct catalog_node, catalog_head);
        if (strncmp(node->user.surname.str, surname, len) == 0) {
            memcpy(output_data, &node->user, sizeof(UserCatalog));
            return 0;
        }
    }

    return -1;
}
EXPORT_SYMBOL(catalog_get_user);

long catalog_add_user(UserCatalog* input_data) {

    struct catalog_node *new_node = kmalloc(sizeof(struct catalog_node), GFP_KERNEL);
    memcpy(&new_node->user, input_data, sizeof(UserCatalog));
    list_add(&new_node->catalog_head, &catalog_head);

    return 0;
}
EXPORT_SYMBOL(catalog_add_user);

long catalog_del_user(const char* surname, unsigned int len) {

    struct list_head *ptr;
    struct catalog_node *node;

    list_for_each(ptr, &catalog_head) {
        node = list_entry(ptr, struct catalog_node, catalog_head);
        if (strncmp(node->user.surname.str, surname, len) == 0) {
            list_del(&node->catalog_head);
            return 0;
        }
    }

    return -1;
}
EXPORT_SYMBOL(catalog_del_user);

module_init(catalog_module_init);
module_exit(catalog_module_exit);
