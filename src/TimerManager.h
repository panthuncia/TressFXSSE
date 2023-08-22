#pragma once

class TimerManager
{
	~TimerManager();

	static TimerManager* GetSingleton()
	{
		static TimerManager manager;
		return &manager;
	}
};
