#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Rusin");
MODULE_DESCRIPTION("Keyboard interrupt3r");
MODULE_VERSION("0.17");

#define TIME_TO_WAIT 5

static int irq = 1;
module_param(irq, int, S_IRUGO);

static int irq_id, irq_counter = 0;
static bool stop = false;

static irqreturn_t keyboard_interrupt(int irq, void* dev_id) {
    irq_counter++;
    return IRQ_NONE;
}

static void printer_handler(struct work_struct*);
static DECLARE_DELAYED_WORK(PrinterHandler, printer_handler);

void printer_handler(struct work_struct* not_used) {
    printk(KERN_INFO "timer interrupt: counter = %d\n", irq_counter);

    if (!stop) {
        schedule_delayed_work(&PrinterHandler, TIME_TO_WAIT * HZ);
    }
}

static int __init irq_module_init(void) {
    int ret = request_irq(irq, keyboard_interrupt, IRQF_SHARED, "keyboard_interrupt", &irq_id);
    if (ret) {
        printk(KERN_INFO "request_irq failed");
        return ret;
    }
    schedule_delayed_work(&PrinterHandler, TIME_TO_WAIT * HZ);

    printk(KERN_INFO "Set handler on IRQ: %d\n", irq );
    return 0;
}

static void __exit irq_module_exit(void) {
    synchronize_irq(irq);
    free_irq(irq, &irq_id);
    printk(KERN_INFO "Unset handler on IRQ: %d\n", irq );
    stop = true;
    cancel_delayed_work_sync(&PrinterHandler);
}

module_init(irq_module_init);
module_exit(irq_module_exit);

