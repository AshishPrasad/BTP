/*
 * MySubTaskParentList.h
 *
 *  Created on: 02-Feb-2012
 *      Author: ashish
 */

#ifndef MYSUBTASKPARENTLIST_H_
#define MYSUBTASKPARENTLIST_H_

#include <linux/list.h>
#include <linux/sched.h>

struct task_struct_node {
	struct task_struct* subtask;
	struct list_head ptr;
};

int addToParentList(struct task_struct* sub_task, struct list_head* list);

void traverseParentList(struct list_head* list);

int deleteFromParentList(struct task_struct* sub_task, struct list_head* list);

#endif /* MYSUBTASKPARENTLIST_H_ */
