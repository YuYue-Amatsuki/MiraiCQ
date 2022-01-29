#include "plusprocess.h"


#include<thread>
#include <jsoncpp/json.h>
#include <Windows.h>
#include <assert.h>
#include <FL/Fl.H>

#include "../../IPC/ipc.h"
#include "../log/MiraiLog.h"
#include "../tool/TimeTool.h"
#include "../tool/StrTool.h"
#include "../tool/PathTool.h"
#include "../tool/ThreadTool.h"


extern std::string g_main_flag;
static std::string g_dll_path;

static std::atomic_bool g_close_heartbeat = false;

/* ���ڴӲ��dll�л�ȡ������ַ */
static void* get_fun_ptr(const std::string& dll_path, const std::string& fun_name)
{
	HMODULE hand = GetModuleHandleA(dll_path.c_str());
	if (hand == NULL)
	{
		hand = LoadLibraryA(dll_path.c_str());
	}
	if (hand == NULL)
	{
		return NULL;
	}
	return GetProcAddress(hand, fun_name.c_str());
}

/* �����������̲�ѯ�������� */
static std::string get_fun_name(int funtype)
{
	static std::map<int, std::string> mmap;
	static std::mutex mx;
	{
		std::lock_guard<std::mutex>lk(mx);
		if (mmap.find(funtype) != mmap.end()) {
			return mmap.at(funtype);
		}
	}
	
	Json::Value to_send;
	to_send["action"] = "get_fun_name";
	to_send["params"] = funtype;
	const char* ret = IPC_ApiSend(g_main_flag.c_str(), Json::FastWriter().write(to_send).c_str(), 3000);
	if (strcmp(ret, "") != 0) {
		std::lock_guard<std::mutex>lk(mx);
		mmap[funtype] = ret;
	}
	return ret;
}

/* �����ж��������Ƿ񻹴��ڣ��������ڣ��������ǰ���� */
static void do_heartbeat(const std::string& main_flag)
{
	Json::Value to_send;
	to_send["action"] = "heartbeat";
	while (true)
	{
		const char* ret = IPC_ApiSend(main_flag.c_str(), Json::FastWriter().write(to_send).c_str(), 3000);
		if (strcmp(ret, "") == 0)
		{
			if (g_close_heartbeat){
				break;
			}
			MiraiLog::get_instance()->add_warning_log("do_heartbeat", "��⵽����������Ӧ�����Բ������ǿ���˳�");
			exit(-1);
		}
		TimeTool::sleep(5000);
	}
}


/* ���ڼ��ز��dll,�������þ�̬ȫ�ֱ���`g_dll_path` */
static void load_plus(const std::string& plus_name)
{
	/* ��ò������·����������׺ */
	std::string plus_path = PathTool::get_exe_dir() + "app\\" + plus_name;
	/* ���CQP�ľ���·�� */
	std::string cqp_path = PathTool::get_exe_dir() + "CQP.dll";
	/* ����CQP.dll */
	if (LoadLibraryA(cqp_path.c_str()) == NULL)
	{
		MiraiLog::get_instance()->add_fatal_log("LOADPLUS", "CQP.dll����ʧ��");
		exit(-1);
	}
	/* ���ز��dll */
	std::string plus_dll_path = plus_path + ".dll";
	/* ȫ�ֱ�����ֵ */
	g_dll_path = plus_dll_path;
	HMODULE hand = LoadLibraryA(plus_dll_path.c_str());
	if (hand == NULL)
	{
		MiraiLog::get_instance()->add_fatal_log("LOADPLUS", plus_dll_path + "����ʧ��");
		exit(-1);
	}
	/* ���ò����Initialize���� */
	typedef __int32(__stdcall* fun_ptr_type_1)(__int32);
	fun_ptr_type_1 fun_ptr1 = (fun_ptr_type_1)GetProcAddress(hand, "Initialize");
	if (!fun_ptr1)
	{
		MiraiLog::get_instance()->add_fatal_log("LOADPLUS", plus_dll_path + ":����Initialize��ȡʧ��");
		exit(-1);
	}
	fun_ptr1(1); //����acֱ�Ӵ�1
}

/* ���ڵ��ò����start��enable���� */
void call_start(void* user_data)
{
	typedef __int32(__stdcall* fun_ptr_type_1)();
	int ret = ((fun_ptr_type_1)user_data)();
	if (ret != 0) {
		MiraiLog::get_instance()->add_fatal_log("START","����ܾ�����, ǿ���˳�");
		exit(-1);
	}
}

/* ���ڵ��ò���˵� */
void call_menu(void* user_data)
{
	typedef __int32(__stdcall* fun_ptr_type_1)();
	((fun_ptr_type_1)user_data)();
}

/* IPC_ApiRecv�Ļص����������ڽ��������̵�ָ�� */
static void fun(const char* sender, const char* flag, const char* msg)
{
	assert(msg);
	assert(sender);
	assert(flag);
	std::string msg_str = msg;
	std::string flag_str = flag;
	std::string sender_str = sender;
	/* ���µ��߳����洦��API���� */
	std::thread([msg_str, flag_str, sender_str]() {
		try
		{
			Json::Value root;
			Json::Reader reader;
			if (!reader.parse(msg_str, root))
			{
				MiraiLog::get_instance()->add_debug_log("PLUS_API_FUN", "�յ����淶��Json" + std::string(msg_str));
				IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "");
				return;
			}
			std::string action = StrTool::get_str_from_json(root, "action", "");
			if (action == "is_load")
			{
				// �����������жϲ�������Ƿ���������
				IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "OK");
				return;
			}
			else if (action == "start")
			{
				// CQ����������
				void* fptr = get_fun_ptr(g_dll_path, root["params"]["fun_name"].asString());
				if (!fptr) {
					IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "");
					return;
				}
				IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "OK");
				Fl::awake(call_start, fptr);
				return;
			}
			else if (action == "enable")
			{
				// ������ú���
				void* fptr = get_fun_ptr(g_dll_path, root["params"]["fun_name"].asString());
				if (!fptr) {
					IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "");
					return;
				}
				IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "OK");
				Fl::awake(call_start, fptr);
				return;
			}
			else if (action == "call_menu") 
			{
				// ����˵�
				void* fptr = get_fun_ptr(g_dll_path, root["params"]["fun_name"].asString());
				if (!fptr) {
					IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "");
					return;
				}
				IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "OK");
				Fl::awake(call_menu, fptr);
				return;
			}
			else if (action == "heartbeat")
			{
				// �����������������жϲ�������Ƿ�����������
				IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "OK");
				return;
			}
			IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "");
		}
		catch (const std::exception& e)
		{
			MiraiLog::get_instance()->add_warning_log("PLUS_API_FUN", "�׳��쳣" + std::string(e.what()));
		}
		
	}).detach();
}

/* ���ڴ��������̴������¼� */
static void do_event(Json::Value & root) {
	std::string event_type = StrTool::get_str_from_json(root, "event_type", "");
	MiraiLog::get_instance()->add_debug_log("PLUS","�յ������̵��¼����ͣ�"+ event_type);
	if (event_type == "cq_event_group_message")
	{
		std::string fun_name = get_fun_name(2);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_group_message)(int sub_type, int msg_id, int64_t from_group, int64_t from_qq, const char* anonymous, const char* msg, int font);
			((cq_event_group_message)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["msg_id"].asInt(),
				root["data"]["from_group"].asInt64(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["anonymous"].asCString(),
				root["data"]["msg"].asCString(),
				root["data"]["font"].asInt()
				);
		}
	}
	else if (event_type == "cq_1207_event")
	{
		std::string fun_name = get_fun_name(1207);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_1207_event)(const char* msg);
			((cq_1207_event)fun_ptr)(
				root["data"]["msg"].asCString()
				);
		}
	}
	else if (event_type == "cq_event_private_message")
	{
		std::string fun_name = get_fun_name(21);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_private_message)(int sub_type, int msg_id, int64_t from_qq, const char* msg, int font);
			((cq_event_private_message)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["msg_id"].asInt(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["msg"].asCString(),
				root["data"]["font"].asInt()
				);
		}
	}
	else if (event_type == "cq_event_group_request")
	{
		std::string fun_name = get_fun_name(302);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_group_request)(__int32 sub_type, __int32 send_time, __int64 from_group, __int64 from_qq, const char* msg, const char* response_flag);
			((cq_event_group_request)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_group"].asInt64(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["msg"].asCString(),
				root["data"]["response_flag"].asCString()
				);
		}
	}
	else if (event_type == "cq_event_friend_request")
	{
		std::string fun_name = get_fun_name(301);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_friend_request)(__int32 sub_type, __int32 send_time, __int64 from_qq, const char* msg, const char* response_flag);
			((cq_event_friend_request)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["msg"].asCString(),
				root["data"]["response_flag"].asCString()
				);
		}
	}
	else if (event_type == "cq_event_friend_add")
	{
		std::string fun_name = get_fun_name(201);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_friend_add)(__int32 sub_type, __int32 send_time, __int64 from_qq);
			((cq_event_friend_add)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_qq"].asInt64()
				);
		}
	}
	else if (event_type == "cq_event_group_ban")
	{
		std::string fun_name = get_fun_name(104);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_group_ban)(__int32 sub_type, __int32 send_time, __int64 from_group, __int64 from_qq, __int64 being_operate_qq, __int64 duration);
			((cq_event_group_ban)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_group"].asInt64(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["being_operate_qq"].asInt64(),
				root["data"]["duration"].asInt64()
				);
		}
	}
	else if (event_type == "cq_event_group_member_increase")
	{
		std::string fun_name = get_fun_name(103);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_group_member_increase)(__int32 sub_type, __int32 send_time, __int64 from_group, __int64 from_qq, __int64 being_operate_qq);
			((cq_event_group_member_increase)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_group"].asInt64(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["being_operate_qq"].asInt64()
				);
		}
	}
	else if (event_type == "cq_event_group_member_decrease")
	{
		std::string fun_name = get_fun_name(102);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_group_member_decrease)(__int32 sub_type, __int32 send_time, __int64 from_group, __int64 from_qq, __int64 being_operate_qq);
			((cq_event_group_member_decrease)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_group"].asInt64(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["being_operate_qq"].asInt64()
				);
		}
	}
	else if (event_type == "cq_event_group_admin")
	{
		std::string fun_name = get_fun_name(101);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_group_admin)(__int32 sub_type, __int32 send_time, __int64 from_group, __int64 being_operate_qq);
			((cq_event_group_admin)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_group"].asInt64(),
				root["data"]["being_operate_qq"].asInt64()
				);
		}
	}
	else if (event_type == "cq_event_group_upload")
	{
		std::string fun_name = get_fun_name(11);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* cq_event_group_upload)(__int32 sub_type, __int32 send_time, __int64 from_group, __int64 from_qq, const char* file_base64);
			((cq_event_group_upload)fun_ptr)(
				root["data"]["sub_type"].asInt(),
				root["data"]["send_time"].asInt(),
				root["data"]["from_group"].asInt64(),
				root["data"]["from_qq"].asInt64(),
				root["data"]["file_base64"].asCString()
				);
		}
	}
	else if (event_type == "exit")
	{
		g_close_heartbeat = true;
		MiraiLog::get_instance()->add_info_log("PLUS", "���յ������̷��͵��˳��¼������ڰ�ȫ�˳�...");
		std::thread([]() {
			TimeTool::sleep(5000);
			/* ��ȫ�˳�(ָǿ�ƽ������� */
			exit(-1);
		}).detach();
		std::string fun_name = get_fun_name(1004);
		void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
		if (fun_ptr)
		{
			typedef int(__stdcall* exit_event)();
			((exit_event)fun_ptr)();
		}
		exit(-1);
	}
	else { 
		MiraiLog::get_instance()->add_warning_log("EVENTRECV", "�յ�δ֪���¼�����:" + root.toStyledString());
	}
}
void plusprocess(const std::string& main_flag, const std::string& plus_flag, const std::string& plus_name)
{
	try
	{
		/* ��ʼ��IPC */
		if (IPC_Init(plus_flag.c_str()) != 0)
		{
			MiraiLog::get_instance()->add_fatal_log("TESTPLUS", "IPC_Init ִ��ʧ��");
			exit(-1);
		}

		/* ���ز��������ʧ�ܻ�ǿ���˳����� */
		load_plus(plus_name);

		/* ���ڴ����������·����¼� */
		std::thread([plus_flag]() {
			while (true)
			{
				const char* evt = IPC_GetEvent(plus_flag.c_str());
				
				Json::Value root;
				Json::Reader reader;
				if (!reader.parse(evt, root))
				{
					/* Json����ʧ�� */
					MiraiLog::get_instance()->add_warning_log("EVENTRECV", "�յ����淶��Json" + std::string(evt));
					continue;
				}
				try {
					ThreadTool::get_instance()->submit([=]() {
						Json::Value root_ = root;
						do_event(root_);
					});
					// Ŀǰ�¼��Ȳ������̣߳���ֹĪЩ���������ȷ����
					// do_event(root);
				}
				catch (const std::exception& e) {
					MiraiLog::get_instance()->add_warning_log("EVENTRECV", std::string("do_event�����쳣��") + e.what());
				}
			}
		}).detach();

		/* �����ж��������Ƿ�������������Լ� */
		std::thread([main_flag]() {
			do_heartbeat(main_flag);
		}).detach();

		/* ���������̵�API���� */
		std::thread([&]() {
			while (true) {
				IPC_ApiRecv(fun);
			}
		}).detach();

		/* ����ѭ�� */
		while (true) {
			TimeTool::sleep(20);
			Fl::wait(1e20);
		}
	}
	catch (const std::exception& e)
	{
		MiraiLog::get_instance()->add_fatal_log("PLUS", e.what());
	}


}