#pragma once

#include <mutex>
#include <queue>

namespace EHKS
{
	class TaskQueue
	{
	public:
		void AddTask(std::function<void()> a_task);

		void ProcessTasks();

		static TaskQueue* GetSingleton();

		static void TaskQueue_Hook(void* a_unk);
		static void InstallHook();

	private:
		TaskQueue(){};
		~TaskQueue(){};
		TaskQueue(const TaskQueue&) = delete;
		TaskQueue& operator=(const TaskQueue&) = delete;

		std::queue<std::function<void()>> _taskQueue;
		std::mutex _mutex;

		static inline uintptr_t _originalFunc;
	};
}
