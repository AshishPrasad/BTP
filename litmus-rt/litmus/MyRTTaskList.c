/*
 * MyRTTaskList.h
 *
 *  Created on: 02-Feb-2012
 *      Author: ashish
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <litmus/MyRTTaskList.h>

#define INDEPENEDENT_SUBTASK 0

struct list_head rt_dep_task_list;
rwlock_t rt_dep_task_list_lock;

void initializeDepTaskList() {
	INIT_LIST_HEAD(&rt_dep_task_list);
	rwlock_init(&rt_dep_task_list_lock);
	printk("@initializeDepTaskList\n");
}

int initializeTaskInDepTaskList(pid_t main_task_id){
	write_lock_irq(&rt_dep_task_list_lock);

	int flag = 0;
	printk("@initializeTaskInDepTaskList\n");
	struct my_rt_dep_task_node* task_node = kmalloc(sizeof(struct my_rt_dep_task_node), GFP_KERNEL);
	if(task_node) {
		task_node->task_id = main_task_id;

		if (!(task_node->subtask_list = kmalloc(sizeof(struct list_head), GFP_KERNEL))) {
			printk("Error in assigning memory to subtask_list.\n");
			goto out;
		}
		if (!(task_node->ref_subtask_list = kmalloc(sizeof(struct list_head), GFP_KERNEL))) {
			printk("Error in assigning memory to ref_subtask_list.\n");
			goto out;
		}

		INIT_LIST_HEAD(task_node->subtask_list);
		INIT_LIST_HEAD(task_node->ref_subtask_list);
		INIT_LIST_HEAD(&task_node->ptr);

		if (&rt_dep_task_list) {
			list_add_tail(&task_node->ptr, &rt_dep_task_list);
			printk("done @initializeTaskInDepTaskList\n");
			flag = 1;
		}
	}

		out:
	write_unlock_irq(&rt_dep_task_list_lock);
	return flag;
}

int checkAndPrepareForNewIteration(struct my_rt_dep_task_node* task_node) {
	int flag = 0;

	if (list_empty(task_node->subtask_list)) {
		list_replace_init(task_node->ref_subtask_list, task_node->subtask_list);
		flag = 1;
	}

	return flag;
}

int prepareForNewIteration(pid_t task_id) {
	write_lock_irq(&rt_dep_task_list_lock);

	int flag = 0;

	struct my_rt_dep_task_node* task_node;
	list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
		if (task_node->task_id == task_id) {
			flag = checkAndPrepareForNewIteration(task_node);
			break;
		}
	}

	write_unlock_irq(&rt_dep_task_list_lock);
	return flag;
}

int addSubtaskToDepTaskList(pid_t subtask_pid, struct task_struct* sub_task) {
	write_lock_irq(&rt_dep_task_list_lock);
	int flag = 0;
	struct my_rt_dep_task_node* task_node;
	list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
		if (task_node->task_id == sub_task->main_task_pid) {			
			if (addToSubtaskList(subtask_pid, sub_task, task_node->subtask_list)) {
				printk("Added subtask : (%d, %d) to DepTaskList.\n", subtask_pid, sub_task->pid);
				flag = 1;
			}
			break;
		}
	}

	write_unlock_irq(&rt_dep_task_list_lock);
	return flag;
}

int addParentToSubtaskInDepTaskList(struct task_struct* parent, struct task_struct* sub_task) {
	write_lock_irq(&rt_dep_task_list_lock);

	int flag = 0;

	if (sub_task->main_task_pid == parent->main_task_pid) {
		struct my_rt_dep_task_node* task_node;
		list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
			if (task_node->task_id == sub_task->main_task_pid) {				
				if (addParentToSubtask(parent, sub_task, task_node->subtask_list)) {
					printk("Added parent : %d to Subtask : %d.\n", parent->pid, sub_task->pid);
					flag = 1;
				}
				break;
			}
		}
	}

	write_unlock_irq(&rt_dep_task_list_lock);
	return flag;
}

int addParentListToSubtaskInDepTaskList(struct list_head* parentList, struct task_struct* sub_task) {
	write_lock_irq(&rt_dep_task_list_lock);

	int flag = 0;

	struct my_rt_dep_task_node* task_node;
	list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
		if (task_node->task_id == sub_task->main_task_pid) {
			if (addParentListToSubtask(parentList, sub_task, task_node->subtask_list))
				flag = 1;
			break;
		}
	}

	write_unlock_irq(&rt_dep_task_list_lock);
	return flag;

}

struct task_struct* FindSubtaskInMainTask(pid_t subtask_pid, pid_t main_task_pid) {
	write_lock_irq(&rt_dep_task_list_lock);

	struct my_rt_dep_task_node* task_node;
	struct my_rt_subtask_node* subtask_node;
	struct task_struct* target = NULL;

	list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
		if (task_node->task_id == main_task_pid) {
			list_for_each_entry(subtask_node, task_node->subtask_list, ptr) {
				if(subtask_node->subtask_pid == subtask_pid) {
					target = subtask_node->subtask;
					goto out_unlock;
				}
			}
		}
	}

		out_unlock:
	write_unlock_irq(&rt_dep_task_list_lock);
	return target;
}

int independentSubTask(struct task_struct* subtask) {
	int flag = 0;
	if (subtask->main_task_pid == INDEPENEDENT_SUBTASK) {
		flag = 1;
	}

	return flag;
}

void obtainSchedulableSubtaskList(struct list_head* subtask_list) {
	read_lock_irq(&rt_dep_task_list_lock);

	INIT_LIST_HEAD(subtask_list);

	struct my_rt_dep_task_node* task_node;
	list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
		list_splice(obtainEmptyParentListSubtaskList(task_node->subtask_list), subtask_list->prev);
	}

	read_unlock_irq(&rt_dep_task_list_lock);
}

int schedulableSubTask(struct task_struct* subtask, struct list_head* sched_list) {
	int flag = 0;

	/* Check whether the subtask is independent*/
	if (independentSubTask(subtask)) {
		flag = 1;
		goto out;
	}

	struct task_struct_node* subtask_node;
	list_for_each_entry(subtask_node, sched_list, ptr) {
		if (subtask_node->subtask == subtask) {
			flag = 1;
			break;
		}
	}

		out:
	return flag;
}

void traverseSchedulableSubtaskList(struct list_head* subtask_list) {
	traverseEmptyParentSubtaskList(subtask_list);
}

void traverseDepTaskList() {
	read_lock_irq(&rt_dep_task_list_lock);
	printk("Traversing the Task List.\n");

	struct my_rt_dep_task_node* cur;
	list_for_each_entry(cur, &rt_dep_task_list, ptr) {
		printk("Task_id: %d\n", cur->task_id);
		printk("Subtask List\n");
		traverseSubTaskList(cur->subtask_list);
		printk("Reference Subtask List\n");
		traverseSubTaskList(cur->ref_subtask_list);
		printk("\n");
	}

	read_unlock_irq(&rt_dep_task_list_lock);
}

// Need to add subtask to the reference subtask list to conserve the task structure
int deleteSubtaskFromDepTaskList(struct task_struct* sub_task) {
	write_lock_irq(&rt_dep_task_list_lock);

	int flag = 0;
	struct my_rt_subtask_node* subtask_node;
	struct my_rt_dep_task_node* task_node;
	list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
		if(task_node->task_id == sub_task->main_task_pid) {
			subtask_node = deleteFromSubtaskList(sub_task, task_node->subtask_list);
			if (subtask_node && addNodeToRefSubtaskList(subtask_node, task_node->ref_subtask_list)) {
//				checkAndPrepareForNewIteration(task_node);
				printk("Subtask: %d deleted @deleteSubtaskFromDepTaskList.\n", sub_task->pid);
				flag = 1;
			}
			break;
		}
	}

	write_unlock_irq(&rt_dep_task_list_lock);
	return flag;
}

int removeTaskFromDepTaskList(pid_t main_task_pid) {
	write_lock_irq(&rt_dep_task_list_lock);
	int flag = 0;

	printk("@removeTaskFromDepTaskList : %d.\n", main_task_pid);
	struct my_rt_dep_task_node* task_node;
	list_for_each_entry(task_node, &rt_dep_task_list, ptr) {
		if (task_node->task_id == main_task_pid) {
			printk("Found task: %d @removeTaskFromDepTaskList.\n", main_task_pid);
			list_del(&task_node->ptr);
			printk("Removed task: %d @removeTaskFromDepTaskList.\n", main_task_pid);
			flag = 1;
			break;
		}
	}

	write_unlock_irq(&rt_dep_task_list_lock);
	return flag;
}

void deleteDepTaskList() {
	write_lock_irq(&rt_dep_task_list_lock);

	printk("@deleteDepTaskList\n");
	list_del(&rt_dep_task_list);

	printk("DepTaskList deleted\n");
	write_unlock_irq(&rt_dep_task_list_lock);
}
