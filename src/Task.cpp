/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

/* Abstract class for Task classes: delayed code execution
 *
 * The scheme assumes the existence of a monotonic clock like
 * Arduino's millis_nw(). Each task is associated with a future
 * clock value (timebase + offset) All tasks whose value is
 * less than millis_nw() * are run.
 *
 * Concrete Task classes implement run2(). This method may
 * return 0 (meaning the task is finished) or a positive offset
 * (meaning the task should be rescheduled to now + offset).
 *
 * The task manager lives inside the Network class, so the
 * main loop calls a Network method periodically, which
 * calls the task manager.
 */

#include "Task.h"
#include "ArduinoBridge.h"
#include "Timestamp.h"

Task::Task(const char *name, int64_t offset):
	name(name), offset(offset), timebase(0)
{
}

Task::~Task()
{
	this->timebase = 0;
}

void Task::set_timebase(int64_t now)
{
	this->timebase = now;
}

bool Task::cancelled() const
{
	return this->timebase == 0;
}

int64_t Task::next_run() const
{
	// logi("Timebase", this->timebase);
	// logi("Offset", this->offset);
	return this->timebase + this->offset;
}

bool Task::should_run(int64_t now) const
{
	return ! this->cancelled() && this->next_run() < now;
}

bool Task::run(int64_t now)
{
	// concrete routine returns new timeout (which could be random)
	this->offset = this->run2(now);
	// task cancelled by default, rebased by task mgr if necessary
	this->timebase = 0;
	return this->offset > 0;
}

/*
Buffer Task::get_name() const
{
	return name;
}
*/

TaskManager::TaskManager() {}

TaskManager::~TaskManager()
{
	stop();
}

void TaskManager::stop()
{
	tasks.clear();
}

void TaskManager::schedule(Ptr<Task> task)
{
	Ptr<Task> etask = task;
	tasks.push_back(etask);
	etask->set_timebase(sys_timestamp());
}

Ptr<Task> TaskManager::next_task() const
{
	Ptr<Task> ret(0);
	int64_t task_time = sys_timestamp() + 60 * 1000;
	for (size_t i = 0 ; i < tasks.count(); ++i) {
		Ptr<Task> t = tasks[i];
		if (! t->cancelled()) {
			int64_t nr = t->next_run();
			if (nr < task_time) {
				task_time = nr;
				ret = t;
			}
		}
	}
	return ret;
}

/*
void TaskManager::cancel(const Task* task)
{
	for (size_t i = 0 ; i < tasks.count(); ++i) {
		if (tasks[i].id() == task) {
			tasks.remov(i);
			break;
		}
	}
}
*/

void TaskManager::run(int64_t now)
{
	bool dirty = false;

	for (size_t i = 0 ; i < tasks.count(); ++i) {
		Ptr<Task> t = tasks[i];
		if (t->should_run(now)) {
			bool stay = t->run(now);
			if (stay) {
				// reschedule
				t->set_timebase(sys_timestamp());
			} else {
				// task list must be pruned
				dirty = true;
			}
		}
	}

	while (dirty) {
		dirty = false;
		for (size_t i = 0 ; i < tasks.count(); ++i) {
			Ptr<Task> t = tasks[i];
			if (t->cancelled()) {
				tasks.remov(i);
				dirty = true;
				break;
			}
		}
	}
}
