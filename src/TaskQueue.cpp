#include "TaskQueue.h"

namespace EHKS
{
	void TaskQueue::AddTask(std::function<void()> a_task)
	{
		std::lock_guard lk(_mutex);
		this->_taskQueue.push(std::move(a_task));
	}

	void TaskQueue::ProcessTasks()
	{
		std::queue<std::function<void()>> tasks;

		{
			std::lock_guard lk(_mutex);
			tasks.swap(_taskQueue);
		}

		while (!tasks.empty())
		{
			tasks.front()();
			tasks.pop();
		}
	}

	void TaskQueue::TaskQueue_Hook(void* a_unk)
	{
		reinterpret_cast<void (*)(void*)>(_originalFunc)(a_unk);

		TaskQueue* queue = TaskQueue::GetSingleton();
		queue->ProcessTasks();
	}

	void TaskQueue::InstallHook()
	{
		_originalFunc = SKSE::GetTrampoline().write_call<5>(REL::ID{ 36564 }.address() + 0x3E, (uintptr_t)TaskQueue::TaskQueue_Hook);
	}

	TaskQueue* TaskQueue::GetSingleton()
	{
		static TaskQueue singleton;
		return &singleton;
	}
}
