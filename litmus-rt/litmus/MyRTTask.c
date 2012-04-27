/*
 * MyRTTask.h
 *
 *  Created on: 02-Feb-2012
 *      Author: ashish
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <litmus/MyRTTask.h>

int addToSubtaskList(pid_t subtask_pid, struct task_struct* sub_task, struct list_head* subtask_list) {
	int flag = 0;
	struct my_rt_subtask_node* node = kmalloc(sizeof(struct my_rt_subtask_node), GFP_KERNEL);

	if (node) {
		node->subtask_pid = subtask_pid;
		node->subtask = sub_task;

		node->parentList = kmalloc(sizeof(struct list_head), GFP_KERNEL);
		INIT_LIST_HEAD(node->parentList);

		node->ref_parentList = kmalloc(sizeof(struct list_head), GFP_KERNEL);
		INIT_LIST_HEAD(node->ref_parentList);

		INIT_LIST_HEAD(&node->ptr);
		list_add_tail(&node->ptr, subtask_list);

		flag = 1;
	}

	return flag;
}

// Need to swap the reference and working parent list and add the subtask node to the reference subtask list
// To be called only from deleteFromSubtaskList in MyRTTaskList.h
int addNodeToRefSubtaskList(struct my_rt_subtask_node* node,  struct list_head* ref_subtask_list) {
	int flag = 0;

	if (node) {
		// Replace parent list by reference parent list and initialize reference parent list
		list_replace_init(node->ref_parentList, node->parentList);
		list_add_tail(&node->ptr, ref_subtask_list);
		flag = 1;
	}

	return flag;
}

int addParentToSubtask(struct task_struct* sub_task_parent, struct task_struct* sub_task, struct list_head* subtask_list) {
	int flag = 0;

	struct my_rt_subtask_node* node;
	list_for_each_entry(node, subtask_list, ptr){
		if (node->subtask == sub_task) {
			if (addToParentList(sub_task_parent, node->parentList))
				flag = 1;
			break;
		}
	}

	return flag;
}

int addParentListToSubtask(struct list_head* parent_list, struct task_struct* sub_task, struct list_head* subtask_list) {
	int flag = 0;

	struct my_rt_subtask_node* node;
	list_for_each_entry(node, subtask_list, ptr){
		if (node->subtask == sub_task) {
			node->parentList = parent_list;
			flag = 1;
			break;
		}
	}

	return flag;
}

void traverseSubTaskList(struct list_head* subtask_list) {
	struct my_rt_subtask_node* node;
	list_for_each_entry(node, subtask_list, ptr) {
		printk("[subtask: %d] --> [parents: ", node->subtask_pid);
		traverseParentList(node->parentList);
		printk("] -> [ref_parents: ");
		traverseParentList(node->ref_parentList);
		printk("]\n");
	}
}

struct list_head* obtainParentListForSubtask(struct task_struct* sub_task, struct list_head* subtask_list) {
	struct list_head* parentList = NULL;

	struct my_rt_subtask_node* node;
	list_for_each_entry(node, subtask_list, ptr) {
		if (node->subtask == sub_task){
			parentList = node->parentList;
			break;
		}
	}

	return parentList;
}

struct list_head* obtainEmptyParentListSubtaskList(struct list_head* subtask_list) {
	struct list_head* subtaskList = kmalloc(sizeof(struct list_head), GFP_KERNEL);
	INIT_LIST_HEAD(subtaskList);

	struct my_rt_subtask_node* node;
	struct task_struct_node* sched_subtask_node;

	list_for_each_entry(node, subtask_list, ptr){
		if (list_empty(node->parentList)){
			sched_subtask_node = kmalloc(sizeof(struct task_struct_node), GFP_KERNEL);
			sched_subtask_node->subtask = node->subtask;
			INIT_LIST_HEAD(&sched_subtask_node->ptr);
			list_add_tail(&sched_subtask_node->ptr, subtaskList);
		}
	}

	return subtaskList;
}

void traverseEmptyParentSubtaskList(struct list_head* emptyParentSubtaskList){
	printk("Traversing subtasks with empty parent list...\n");

	struct task_struct_node* cur;
	list_for_each_entry(cur, emptyParentSubtaskList, ptr) {
		printk("[subtask -> %d , task_id: %d], ", cur->subtask->pid, cur->subtask->main_task_pid);
	}

	printk("\n");
}

// Need to insert the subtask_parent to reference parent list to preserve the precedence structure
void delSubtaskFromParentList(struct task_struct* sub_task, struct list_head* subtask_list)
{
	struct my_rt_subtask_node* node;
	struct task_struct_node* del_node;
	list_for_each_entry(node, subtask_list, ptr){
		if (deleteFromParentList(sub_task, node->parentList)) {
			printk("Deleted subtask: %d from Parent List of subtask: %d.\n", sub_task->pid, node->subtask_pid);
			addToParentList(sub_task, node->ref_parentList);
		}
	}
}

// Call only for subtask with empty parent list
struct my_rt_subtask_node* deleteFromSubtaskList(struct task_struct* subtask, struct list_head* subtask_list) {
	int flag = 0;

	struct my_rt_subtask_node* deleted_node = NULL;
	struct my_rt_subtask_node* node;

	list_for_each_entry(node, subtask_list, ptr) {
		if (node->subtask == subtask && list_empty(node->parentList)){
			list_del_init(&node->ptr);
			deleted_node = node;
			printk("Deleted Subtask: %d @deleteFromSubtaskList.\n", subtask->pid);
			break;
		}
	}

	if (deleted_node) {
		delSubtaskFromParentList(subtask, subtask_list);
	}

	return deleted_node;
}
