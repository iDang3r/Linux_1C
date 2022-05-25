#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/mm.h>
#include <linux/pgtable.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Rusin");
MODULE_DESCRIPTION("Mmaneg proc control");

#define PROC_NAME "mmaneg"
#define BUFF_SIZE 1024
static char module_buffer[BUFF_SIZE];
static unsigned long module_buffer_size = 0;
static char help_message[] = "\
    HELP for mmaneg: please, use:   \n\
        - listvma                   \n\
        - findpage addr             \n\
        - writeval addr val         \n\
";

static struct proc_dir_entry* entry;

static ssize_t procfile_read(struct file* file, char __user* buffer, size_t count, loff_t* offset) {
    if (*offset > 0 || count < BUFF_SIZE) {
        return 0;
    }
    if (copy_to_user(buffer, help_message, sizeof(help_message))) {
        return -EFAULT;
    }
    *offset = sizeof(help_message);
    return sizeof(help_message);
}

static ssize_t procfile_write(struct file* file, const char __user* buffer, size_t count, loff_t* f_pos){
    if (copy_from_user(module_buffer, buffer, count)) {
        return -EFAULT;
    }

    module_buffer_size = BUFF_SIZE;
    if (count < BUFF_SIZE) {
        module_buffer_size = count;
    }

    printk(KERN_INFO "READ: %s", module_buffer);
    printk(KERN_INFO "The process is \"%s\" (pid %i)\n",
                     current->comm, current->pid);

    if (!strncmp(module_buffer, "listvma", 7)) {
        printk(KERN_INFO "listvma command\n");

        struct vm_area_struct* vm_area = current->mm->mmap;
        while (vm_area != NULL) {
            printk(KERN_INFO "start_: %px, end_: %px\n", (void*)vm_area->vm_start, (void*)vm_area->vm_end);
            vm_area = vm_area->vm_next;
        }

    } else if (!strncmp(module_buffer, "findpage", 8)) {
        printk(KERN_INFO "findpage command\n");
        void* ptr = (void*)0;
        sscanf(module_buffer + 9, "%llx", &ptr);
        printk(KERN_INFO "<-ptr-> %px\n", ptr);
        
        pgd_t *pgd;
        p4d_t *p4d;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *ptep;

        void* address = ptr;
        pgd = pgd_offset(current->mm, address);
        if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd))) {
            goto out;
        }

        p4d = p4d_offset(pgd, address);
        if (p4d_none(*p4d) || unlikely(p4d_bad(*p4d))) {
            goto out;
        }

        pud = pud_offset(p4d, address);
        if (pud_none(*pud) || unlikely(pud_bad(*pud))) {
            goto out;
        }

        pmd = pmd_offset(pud, address);
        if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd))) {
		    goto out;
        }
        ptep = pte_offset_map(pmd, address);

        printk(KERN_INFO "ptep: %llx\n", *ptep);

        unsigned long vaddr = ptr;

        unsigned long page_addr = pte_val(*ptep) & PAGE_MASK;
        unsigned long page_offset = vaddr & ~PAGE_MASK;
        unsigned long paddr = page_addr | page_offset;
        printk("page_addr = %lx, page_offset = %lx\n", page_addr, page_offset);
        printk("vaddr = %lx, paddr = %lx\n", vaddr, paddr);

    } else if (!strncmp(module_buffer, "writeval", 8)) {
        printk(KERN_INFO "writeval command\n");

        // ?? change field in struct...
        // not useful 

    } else {
        printk(KERN_INFO "Unknown command, use `cat` to read help message\n");
    }
out:
    return module_buffer_size;
}

static int procfile_show(struct seq_file* m, void* v) {
    return 0;
}

static int procfile_open(struct inode* inode,struct file* file) {
    return single_open(file, procfile_show, NULL);
}

static struct proc_ops proc_fops = {
    .proc_lseek = seq_lseek,
    .proc_open = procfile_open,
    .proc_read = procfile_read,
    .proc_release = single_release,
    .proc_write = procfile_write,
};

static int __init my_proc_init(void) {
    entry = proc_create(PROC_NAME, 0777, NULL, &proc_fops);
    if (!entry) {
        return -1;
    }
    printk(KERN_INFO "%s: loaded!\n", PROC_NAME);
    return 0;
}

static void __exit my_proc_cleanup(void) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "%s: unloaded!\n", PROC_NAME);
}

module_init(my_proc_init);
module_exit(my_proc_cleanup);
