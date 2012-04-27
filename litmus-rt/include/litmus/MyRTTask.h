#ifndef MYRTTASK_H_
#define MYRTTASK_H_

#include <litmus/MySubTaskParentList.h>

struct task_struct;

struct my_rt_subtask_node {
	pid_t subtask_pid;
	struct task_struct* subtask;
	struct list_head* parentList;
	struct list_head* ref_parentList;
	struct list_head ptr;
};

int addToSubtaskList(pid_t subtask_pid, struct task_struct* sub_task, struct list_head* subtask_list);

int addNodeToRefSubtaskList(struct my_rt_subtask_node* node,  struct list_head* ref_subtask_list);

int addParentToSubtask(struct task_struct* sub_task_parent, struct task_struct* sub_task, struct list_head* subtask_list);

int addParentListToSubtask(struct list_head* parent_list, struct task_struct* sub_task, struct list_head* subtask_list);

void traverseSubTaskList(struct list_head* subtask_list);

struct list_head* obtainParentListForSubtask(struct task_struct* sub_task, struct list_head* subtask_list);

struct list_head* obtainEmptyParentListSubtaskList(struct list_head* subtask_list);

void traverseEmptyParentSubtaskList(struct list_head* emptyParentSubtaskList);

void delSubtaskFromParentList(struct task_struct* sub_task, struct list_head* subtask_list);

struct my_rt_subtask_node* deleteFromSubtaskList(struct task_struct* subtask, struct list_head* subtask_list);

#endif /* MYRTTASK_H_ */
