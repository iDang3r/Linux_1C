cmd_/home/alex/1C/Linux_1C/task_2/modules.order := {   echo /home/alex/1C/Linux_1C/task_2/irq_module.ko; :; } | awk '!x[$$0]++' - > /home/alex/1C/Linux_1C/task_2/modules.order
