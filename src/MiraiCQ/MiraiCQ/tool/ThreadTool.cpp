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
		MiraiLog::get_instance()->add_fatal_log("ThreadTool","�ύ������ʧ�ܣ��ۻ�������������");
		return false;
	}
	// û�п����̣߳�������һ���̣߳���֤������˳������
	if (unused_thread_nums == 0)
	{
		add_new_thread();
	}
	// ����������������
	{
		std::lock_guard<std::mutex> lock(mx_task_list);
		task_list.push_back(task);
		cv.notify_one();
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
	std::lock_guard<std::mutex> lock(mx_task_list);
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
			TimeTool::sleep(200);
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
		MiraiLog::get_instance()->add_warning_log("ThreadTool", "�����̹߳��࣬�������߳�ʧ��");
		return;
	}
	++cur_thread_nums;
	std::thread([this]() {
		while (true)
		{
			std::function<void()> task = nullptr;
			{
				++unused_thread_nums;
				/* �ȴ�һ������ */
				std::unique_lock<std::mutex> lock(mx_task_list);
				bool is_get = cv.wait_for(lock, std::chrono::seconds(5), [this]() {
					return task_list.size() > 0;
				});
				--unused_thread_nums;
				if (is_get) {
					/* �ɹ��ȵ�һ������ */
					task = (*task_list.begin());
					task_list.pop_front();
				}
				else {
					/* û�гɹ��ȵ�һ������������߳� */
					break;
				}
			}
			/* ִ������ */
			try {
				task();
			}
			catch (const std::exception& e) {
				MiraiLog::get_instance()->add_fatal_log("ThreadTool", std::string("��ThreadTool�з���δ֪����:") + e.what());
				exit(-1);
			}
			
		}
		--cur_thread_nums;
	}).detach();
}
