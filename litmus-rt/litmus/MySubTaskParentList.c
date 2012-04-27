/*
 * MySubTaskParentList.h
 *
 *  Created on: 02-Feb-2012
 *      Author: ashish
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <litmus/MySubTaskParentList.h>

int addToParentList(struct task_struct* sub_task, struct list_head* list) {
	int flag = 0;
	struct task_struct_node* node = kmalloc(sizeof(struct task_struct_node), GFP_KERNEL);

	if (node) {
		node->subtask = sub_task;
		INIT_LIST_HEAD(&node->ptr);
		list_add_tail(&(node->ptr), list);
		flag = 1;
	}

	return flag;
}

void traverseParentList(struct list_head* list) {

	struct task_struct_node* node;
	list_for_each_entry(node, list, ptr){
		printk("%d, ", node->subtask->pid);
	}
}

int deleteFromParentList(struct task_struct* sub_task, struct list_head* list) {
	int flag = 0;

	struct task_struct_node* node = NULL;
	list_for_each_entry(node, list, ptr) {
		if(node->subtask == sub_task) {
			list_del(&node->ptr);
			flag = 1;
			break;
		}
	}

	return flag;
}
