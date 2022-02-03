#include "center.h"
#include <Windows.h>
#include "../log/MiraiLog.h"
#include "../tool/PathTool.h"
#include "../tool/StrTool.h"
#include "../tool/TimeTool.h"
#include "../config/config.h"
#include "../tool/ThreadTool.h"
#include "../tool/IPCTool.h"
#include <cassert>
#include <algorithm>

using namespace std;

Center::Center()
{
}

Center* Center::get_instance() 
{
	static Center center;
	return &center;
}

Center::~Center()
{

}

void Center::set_net(std::weak_ptr<MiraiNet> net) 
{
	unique_lock<shared_mutex> lk(mx_net);
	this->net = net;
}

int Center::load_all_plus() 
{
	auto plus = MiraiPlus::get_instance();
	assert(plus);
	std::string exe_dir = PathTool::get_exe_dir();
	std::string plus_dir = exe_dir + "app\\";
	PathTool::create_dir(plus_dir);
	std::vector<std::string> path_file = PathTool::get_path_file(plus_dir);
	int success_num = 0;
	for (const auto& file_str : path_file)
	{
		if (!StrTool::end_with(StrTool::tolower(file_str), ".dll"))
		{
			continue;
		}
		auto file_name = StrTool::remove_suffix(file_str);
		MiraiLog::get_instance()->add_debug_log("Center", "��ʼ���ز��`" + file_name + "`");
		std::string err;
		if (plus->load_plus(file_name, err))
		{
			++success_num;
			MiraiLog::get_instance()->add_info_log("Center", "���`" + file_name + "`���سɹ�");
		}
		else
		{
			MiraiLog::get_instance()->add_info_log("Center", "���`" + file_name + "`����ʧ�ܣ�"+ err);
		}

	}
	return success_num;
}

int Center::enable_all_plus() 
{
	auto plus = MiraiPlus::get_instance();
	assert(plus);
	auto plus_map = plus->get_all_plus();
	int success_num = 0;
	for (auto& p : plus_map)
	{
		std::string err;
		if (plus->enable_plus(p.first, err))
		{
			++success_num;
			MiraiLog::get_instance()->add_info_log("Center", "���`" + p.second->get_name() + "`�����ɹ�");
		}
		else
		{
			MiraiLog::get_instance()->add_info_log("Center", "���`" + p.second->get_name() + "`����ʧ�ܣ�" + err);
		}
	}
	return success_num;
}

static void IPC_SendEvent_T(const char* msg)
{
	auto plus = MiraiPlus::get_instance()->get_all_plus();
	for (auto p : plus) {
		IPC_SendEvent(p.second->uuid.c_str(), msg);
	}
}

int Center::del_all_plus() 
{
	//�Ƚ��ò������ֹ������յ��¼�
	auto plus = MiraiPlus::get_instance()->get_all_plus();
	for (auto p : plus) {
		p.second->is_enable = false;
	}
	// �����������˳��¼�
	Json::Value to_send;
	to_send["event_type"] = "exit";
	IPC_SendEvent_T(to_send.toStyledString().c_str());
	return 0;
}

bool Center::run() 
{
	auto plus = MiraiPlus::get_instance();
	assert(plus);
	{
		shared_lock<shared_mutex> lk(mx_net);
		if (!net.lock())
		{
			return false;
		}
	}
	std::thread([this]() {
		auto pool = ThreadTool::get_instance();
		MiraiLog::get_instance()->add_info_log("Center", "Center�Ѿ���ʼ����");
		while (true)
		{
			/* �����ﴦ������Net�¼����� */
			shared_ptr<MiraiNet> net;
			{
				shared_lock<shared_mutex>(mx_net);
				net = this->net.lock();
				if (!net)
				{
					TimeTool::sleep(0);
					continue;
				}
			}
			
			auto event_vec = net->get_event();
			/* ���û��ȡ���¼�����˯��һ��,����cpuѹ�� */
			if (event_vec.size() == 0)
			{
				TimeTool::sleep(10);
			}
			for (auto evt : event_vec)
			{
				if (!evt)
				{
					continue;
				}
				/* ���¼������̳߳� */
				pool->submit([this,evt]() {
					try
					{
						this->deal_event(evt);
					}
					catch (const std::exception& e)
					{
						MiraiLog::get_instance()->add_debug_log("Center", string("���¼������з���δ֪�쳣") + e.what());
					}
					
				});
			}
		}
	}).detach();
	return true;
}

std::vector<int> Center::get_ac_vec() 
{
	auto plus = MiraiPlus::get_instance();
	assert(plus);
	vector<int> ret_vec;
	auto plus_map = plus->get_all_plus();
	for (auto& it : plus_map)
	{
		ret_vec.push_back(it.first);
	}
	return ret_vec;
}

std::shared_ptr<Center::PlusInfo> Center::get_plus_by_ac(int ac) 
{
	auto plus = MiraiPlus::get_instance();
	assert(plus);
	auto pdf = plus->get_plus(ac);
	if (!pdf)
	{
		/* ac���� */
		return std::shared_ptr<Center::PlusInfo>();
	}
	std::shared_ptr<Center::PlusInfo> info(new Center::PlusInfo);
	info->ac = pdf->ac;
	info->description = pdf->description;
	info->is_enable = pdf->is_enable;
	info->is_load = !(pdf->is_first_enable);
	info->name = pdf->name;
	info->version = pdf->version;
	info->author = pdf->author;
	return info;
}

std::string Center::get_menu_name_by_ac(int ac, int pos) 
{
	auto plus = MiraiPlus::get_instance();
	assert(plus);
	auto pdf = plus->get_plus(ac);
	if (!pdf)
	{
		return "";
	}
	try
	{
		return pdf->menu_vec.at(pos)->name;
	}
	catch (const std::exception&)
	{
		return "";
	}
	
}

void Center::call_menu_fun_by_ac(int ac, int pos) 
{
	auto plus = MiraiPlus::get_instance();
	assert(plus);
	auto pdf = plus->get_plus(ac);
	if (!pdf)
	{
		return ;
	}
	Json::Value to_send;
	to_send["action"] = "call_menu";
	auto menu_vec = pdf->get_menu_vec();
	try {
		to_send["params"]["fun_name"] = menu_vec.at(pos)->fun_name;
		IPC_ApiSend(pdf->uuid.c_str(), Json::FastWriter().write(to_send).c_str(), 100);
	}
	catch (...) {

	}

}

