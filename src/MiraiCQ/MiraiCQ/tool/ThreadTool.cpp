#include "ThreadTool.h"
#include "TimeTool.h"
#include <thread>
#include "../log/MiraiLog.h"
#include <spdlog/fmt/fmt.h>



bool ThreadTool::submit(const std::function<void()> & task)
{
	/* ��ǰ������࣬�ܾ��ύ������ */
	if (get_task_list_nums() > max_task_nums)
	{
		MiraiLog::get_instance()->add_debug_log("ThreadTool","�ύ������ʧ�ܣ��ۻ�������������");
		return false;
	}
	// ����������������
	{
		std::unique_lock<std::shared_mutex> lock(mx_task_list);
		task_list.push_back(task);
	}
	// û�п����̣߳�������һ���̣߳���֤������˳������
	if (unused_thread_nums == 0)
	{
		add_new_thread();
	}
	return true;
}

ThreadTool* ThreadTool::get_instance()
{
	static ThreadTool t;
	return &t;
}

int ThreadTool::get_cur_thread_nums() const
{
	return cur_thread_nums;
}

int ThreadTool::get_unused_thread_nums() const
{
	return unused_thread_nums;
}

size_t ThreadTool::get_task_list_nums()
{
	std::shared_lock<std::shared_mutex> lock(mx_task_list);
	return task_list.size();
}

ThreadTool::ThreadTool()
{
	// ����һ���ػ��߳�
	++cur_thread_nums;
	std::thread([&]() {
		int i = 0;
		while (true) {
			// û�л�ȡ������˯��һ���
			TimeTool::sleep(100);
			// ���û��δʹ�õ��߳�,����������������һ���߳�
			if (unused_thread_nums == 0 && (get_task_list_nums() > 0))
			{
				add_new_thread();
			}
		}
		// �߳��˳��ˣ�ʵ���ϲ���ִ�е�����
		// --cur_thread_nums;
	}).detach();
}


void ThreadTool::add_new_thread()
{
	// �����ǰ�߳��������࣬��ܾ������µ��߳�
	if (cur_thread_nums > max_thread_nums)
	{
		return;
	}
	++cur_thread_nums;
	std::thread([&]() {
		// ���ڱ���Ƿ����
		bool is_unused = false;
		while (true)
		{
			std::function<void()> task = nullptr;
			{
				// �����������һ������
				std::unique_lock<std::shared_mutex> lock(mx_task_list);
				if (task_list.size() > 0) {
					task = (*task_list.begin());
					task_list.pop_front();
				}
			}
			if (task) {
				// ����õ��ˣ���ִ��
				if (is_unused)
				{
					// ɾ�����б��
					is_unused = false;
					--unused_thread_nums;
				}
				task();
			}
			else {
				// ���û�õ���˵���������Ϊ�գ�
				if (is_unused){
					// ����Ѿ������Ϊ���У�˵������û�õ�task�����˳�
					break;
				}
				TimeTool::sleep(100);
				// �����̱߳��Ϊ����,Ȼ�����ѭ��
				++unused_thread_nums;
				is_unused = true;
			}

		}
		// ɾ�����б��
		if (is_unused) {
			--unused_thread_nums;
		}
		// �߳��˳���
		--cur_thread_nums;
	}).detach();
}
