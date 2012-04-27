#ifndef MYRTTASKLIST_H_
#define MYRTTASKLIST_H_

#include <litmus/MyRTTask.h>
#include <linux/rwlock.h>

struct my_rt_dep_task_node {
	pid_t task_id;
	struct list_head* subtask_list;
	struct list_head* ref_subtask_list;
	struct list_head ptr;
};

extern struct list_head rt_dep_task_list;
extern rwlock_t rt_dep_task_list_lock;

void initializeDepTaskList();

int initializeTaskInDepTaskList(pid_t main_task_id);

int checkAndPrepareForNewIteration(struct my_rt_dep_task_node* task_node);

int prepareForNewIteration(pid_t task_id);

int addSubtaskToDepTaskList(pid_t subtask_pid, struct task_struct* sub_task);

int addParentToSubtaskInDepTaskList(struct task_struct* parent, struct task_struct* sub_task);

int addParentListToSubtaskInDepTaskList(struct list_head* parentList, struct task_struct* sub_task);

struct task_struct* FindSubtaskInMainTask(pid_t subtask_pid, pid_t main_task_pid);

void obtainSchedulableSubtaskList(struct list_head* subtask_list);

int independentSubTask(struct task_struct* subtask);

int schedulableSubTask(struct task_struct* subtask, struct list_head* sched_list);

void traverseSchedulableSubtaskList(struct list_head* subtask_list);

void traverseDepTaskList();

int deleteSubtaskFromDepTaskList(struct task_struct* sub_task);

int removeTaskFromDepTaskList(pid_t main_task_id);

void deleteDepTaskList();
#endif /* MYRTTASKLIST_H_ */
