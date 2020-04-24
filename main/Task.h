#ifndef __TASK_H
#define __TASK_H

// inherited by classes that will supply callback methods for tasks

#include "Vector.h"
#include "Pointer.h"

class TaskManager;

class Task {
public:
	Task(const char *name, unsigned long int offset);
	virtual ~Task();
	unsigned long int next_run() const;
	const char *get_name() const;

protected:
	// overridden by concrete task subclasses
	virtual unsigned long int run2(unsigned long int now) = 0;

private:
	bool run(unsigned long int now);
	void set_timebase(unsigned long int timebase);
	bool should_run(unsigned long int now) const;
	bool cancelled() const;

	const char *name;
	unsigned long int offset;
	unsigned long int timebase;

	// Tasks must be manipulated through (smart) pointers,
	// the pointer is the ID, no copies allowed
	Task() = delete;
	Task(const Task&) = delete;
	Task(const Task&&) = delete;
	Task& operator=(const Task&) = delete;
	Task& operator=(Task&&) = delete;

	friend class TaskManager;
};

class TaskManager {
public:
	TaskManager();
	~TaskManager();
	void run(unsigned long int);
	void schedule(Ptr<Task> task);
	void cancel(const Task* task);
	// for testing purposes
	Ptr<Task> next_task() const;
private:
	Vector< Ptr<Task> > tasks;

	TaskManager(const TaskManager&) = delete;
	TaskManager(const TaskManager&&) = delete;
	TaskManager& operator=(const TaskManager&) = delete;
	TaskManager& operator=(TaskManager&&) = delete;
};

#endif
