/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Abstract class for Task classes: delayed code execution

#ifndef __TASK_H
#define __TASK_H

#include <cstddef>
#include <cstdint>
#include "Vector.h"
#include "Buffer.h"
#include "Pointer.h"

class TaskManager;

class Task {
public:
	Task(const char *name, int64_t offset);
	virtual ~Task();
	int64_t next_run() const;
	Buffer get_name() const;

protected:
	// overridden by concrete task subclasses
	virtual int64_t run2(int64_t now) = 0;

private:
	bool run(int64_t now);
	void set_timebase(int64_t timebase);
	bool should_run(int64_t now) const;
	bool cancelled() const;

	Buffer name;
	int64_t offset;
	int64_t timebase;

	// Tasks must be manipulated through (smart) pointers,
	// the pointer is the ID, no copies allowed
	Task() = delete;
	Task(const Task&) = delete;
	Task(Task&&) = delete;
	Task& operator=(const Task&) = delete;
	Task& operator=(Task&&) = delete;

	friend class TaskManager;
};

class TaskManager {
public:
	TaskManager();
	~TaskManager();
	void stop();
	void run(int64_t);
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
