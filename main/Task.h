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
#include "Pointer.h"

class TaskManager;

class Task {
public:
	Task(const char *name, uint32_t offset);
	virtual ~Task();
	uint32_t next_run() const;
	const char *get_name() const;

protected:
	// overridden by concrete task subclasses
	virtual uint32_t run2(uint32_t now) = 0;

private:
	bool run(uint32_t now);
	void set_timebase(uint32_t timebase);
	bool should_run(uint32_t now) const;
	bool cancelled() const;

	const char *name;
	uint32_t offset;
	uint32_t timebase;

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
	void run(uint32_t);
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
