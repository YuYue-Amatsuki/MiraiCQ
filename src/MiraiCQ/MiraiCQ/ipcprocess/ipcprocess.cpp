#include "ipcprocess.h"


#include<thread>
#include <jsoncpp/json.h>
#include <Windows.h>
#include <assert.h>
#include <map>
#include <FL/Fl.H>

#include "../tool/IPCTool.h"
#include "../log/MiraiLog.h"
#include "../tool/TimeTool.h"
#include "../tool/StrTool.h"
#include "../tool/PathTool.h"
#include "../tool/ThreadTool.h"
#include "../center/center.h"

#include <tlhelp32.h>

#include "../resource.h"


extern std::string g_plus_name;
extern std::string g_main_flag;
static std::string g_dll_path;

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

/* ���ڲ�ѯ��������,����`""`����û�в�ѯ�� */
static std::string get_fun_name(int funtype)
{
	static std::map<int, std::string> mmap;
	static std::mutex mx;
	{
		std::lock_guard<std::mutex>lk(mx);
		if (mmap.find(funtype) != mmap.end()) {
			auto ret = mmap.at(funtype);
			if (ret == "?") { // `?` ����û�в�ѯ��
				return "";
			}
			else {
				return ret;
			}
		}
	}
	Json::Value to_send;
	std::string json_path = PathTool::get_exe_dir() + "app\\" + g_plus_name + ".json";
	if (!PathTool::is_file_exist(json_path))
	{
		std::lock_guard<std::mutex>lk(mx);
		mmap[funtype] = "";
		return "";
	}
	std::string json_file;
	try {
		json_file = PathTool::read_biniary_file(json_path);
		if (StrTool::is_utf8(json_file)) {
			json_file = StrTool::to_ansi(json_file);
		}
	}
	catch (...) {
		std::lock_guard<std::mutex>lk(mx);
		mmap[funtype] = "";
		return "";
	}
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(json_file, root))
	{
		std::lock_guard<std::mutex>lk(mx);
		mmap[funtype] = "";
		return "";
	}
	try {
		Json::Value event_json_arr = root.get("event", "");
		for (auto& it : event_json_arr) {
			if (it["type"].asInt64() == funtype) {
				std::string fun_name = it["function"].asString();
				std::lock_guard<std::mutex>lk(mx);
				mmap[funtype] = fun_name;
				return fun_name;
			}
		}
	}
	catch (...) {
		std::lock_guard<std::mutex>lk(mx);
		mmap[funtype] = "";
		return "";
	}
	std::lock_guard<std::mutex>lk(mx);
	mmap[funtype] = "";
	return "";
}

/* ���ڼ��ز��dll,�������þ�̬ȫ�ֱ���`g_dll_path` */
static void load_plus(const std::string& plus_name)
{
	/* ��ò������·����������׺ */
	std::string plus_path = PathTool::get_exe_dir() + "app\\" + plus_name;
	/* ���CQP��·�� */
	std::string cqp_path = "CQP.dll";
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
static void call_start(void* user_data)
{
	typedef __int32(__stdcall* fun_ptr_type_1)();
	int ret = ((fun_ptr_type_1)user_data)();
	if (ret != 0) {
		MiraiLog::get_instance()->add_fatal_log("START", "����ܾ�����, ǿ���˳�");
		exit(-1);
	}
}

/* ���ڵ��ò���˵� */
static void call_menu(void* user_data)
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
			IPC_ApiReply(sender_str.c_str(), flag_str.c_str(), "");
		}
		catch (const std::exception& e)
		{
			MiraiLog::get_instance()->add_warning_log("PLUS_API_FUN", "�׳��쳣" + std::string(e.what()));
		}

		}).detach();
}

/* ���ڴ��������̴������¼� */
static void do_event(const std::string & tp,const Json::Value & root) {
	std::string event_type = StrTool::get_str_from_json(root, "event_type", "");
	// MiraiLog::get_instance()->add_debug_log("PLUS","�յ������̵��¼����ͣ�"+ event_type);
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
	else {
		// MiraiLog::get_instance()->add_warning_log("EVENTRECV", "�յ�δ֪���¼�����:" + root.toStyledString());
	}
}

static void release_dll()
{
	std::string cqp1_dir = PathTool::get_exe_dir();
	std::string cqp1_file = cqp1_dir + "CQP.dll";
	std::string cqp2_dir = PathTool::get_exe_dir() + "bin\\";
	std::string cqp2_file = cqp2_dir + "CQP.dll";
	if (PathTool::is_file_exist(cqp1_file)) {
		PathTool::del_file(cqp1_file);
	}
	if (PathTool::is_file_exist(cqp2_file)) {
		PathTool::del_file(cqp2_file);
	}
	HRSRC hRes = FindResourceA(NULL, MAKEINTRESOURCEA(IDR_DLL_BIN1), "DLL_BIN");
	if (hRes == NULL) {
		MiraiLog::get_instance()->add_fatal_log("RELEASE_CQP.dll", "FindResourceA err");
		exit(-1);
	}
	HGLOBAL hMem = LoadResource(NULL, hRes);
	DWORD dwSize = SizeofResource(NULL, hRes);
	PathTool::create_dir(cqp2_dir);
	HANDLE hFile = CreateFileA(cqp2_file.c_str(), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MiraiLog::get_instance()->add_fatal_log("RELEASE_CQP.dll", "CreateFileA err");
		exit(-1);
	}
	DWORD dwWrite = 0;
	BOOL bRet = WriteFile(hFile, hMem, dwSize, &dwWrite, NULL);
	if (bRet == FALSE) {
		MiraiLog::get_instance()->add_fatal_log("RELEASE_CQP.dll", "WriteFile err");
		exit(-1);
	}
	CloseHandle(hFile);
}

void ipcprocess(const std::string& main_flag, const std::string& plus_flag, const std::string& plus_name)
{
	try
	{
		// ����dll����Ŀ¼
		std::string path_str = PathTool::get_exe_dir() + "bin\\";
		PathTool::create_dir(path_str);
		SetDllDirectoryA(path_str.c_str());


		/* ��ʼ��IPC */
		if (IPC_Init(plus_flag.c_str()) != 0)
		{
			MiraiLog::get_instance()->add_fatal_log("TESTPLUS", "IPC_Init ִ��ʧ��");
			exit(-1);
		}

		//�ͷ�CQP.dll
		release_dll();

		/* ���ز��������ʧ�ܻ�ǿ���˳����� */
		load_plus(plus_name);


		// CQ����������
		std::string start_fun_name = get_fun_name(1001);
		std::string enable_fun_name = get_fun_name(1003);
		void* fptr = get_fun_ptr(g_dll_path, start_fun_name);
		if (fptr) {
			call_start(fptr);
		}
		fptr = get_fun_ptr(g_dll_path, enable_fun_name);
		if (fptr) {
			call_start(fptr);
		}


		/* ���ڴ����������·����¼� */
		std::thread([plus_flag]() {
			while (true)
			{
				const char* evt = IPC_GetEvent(plus_flag.c_str());
				assert(evt);
				Json::Value root;
				Json::Reader reader;
				if (!reader.parse(evt, root))
				{
					MiraiLog::get_instance()->add_debug_log("IPC_PROCESS_EVENT", "�յ����淶��Json" + std::string(evt));
					continue;
				}
				
				//����Ƿ��ǲ���˳��¼�
				try {
					std::string event_type = StrTool::get_str_from_json(root, "event_type", "");
					if (event_type == "exit")
					{
						MiraiLog::get_instance()->add_info_log("PLUS", "���յ������̷��͵��˳��¼������ڰ�ȫ�˳�...");
						std::thread([]() {
							TimeTool::sleep(5000);
							/* ��ȫ�˳�(ָǿ�ƽ������� */
							exit(-1);
						}).detach();

						std::string fun_name = get_fun_name(1002);
						void* fun_ptr = get_fun_ptr(g_dll_path, fun_name);
						if (fun_ptr)
						{
							typedef int(__stdcall* exit_event)();
							((exit_event)fun_ptr)();
						}
						exit(0);
					}
				}
				catch (const std::exception& e) {
					MiraiLog::get_instance()->add_fatal_log("EVENTRECV", std::string("����˳�ʱ�����쳣��") + e.what());
					exit(-1);
				}

				try {
					if (StrTool::get_str_from_json(root, "post_type", "") == "message")
					{
						auto msg_json = root.get("message", Json::Value());
						/* �ڴ˽�message��Ϊ�����ʽ */
						if (msg_json.isString())
						{
							root["message"] = StrTool::cq_str_to_jsonarr(msg_json.asString());
						}
					}
				}
				catch (const std::exception& e) {
					MiraiLog::get_instance()->add_debug_log("EVENTRECV", "����Ϣ��ʽתΪ����ʧ��:" + std::string(e.what()));
					continue;
				}
				
				std::map<std::string,Json::Value> to_send;
				try {
					to_send = Center::get_instance()->deal_event(MiraiNet::NetStruct(new Json::Value(root)));
				}
				catch (const std::exception& e) {
					MiraiLog::get_instance()->add_debug_log("EVENTRECV", "Ԥ����Json��Ϣʧ��:" + std::string(e.what()));
					continue;
				}

				// ��������̳߳�
				ThreadTool::get_instance()->submit([to_send]() {
					try {
						for (auto& it : to_send) {
							if (!it.second.isNull()) {
								do_event(it.first,it.second);
							}
						}	
					}
					catch (const std::exception& e) {
						MiraiLog::get_instance()->add_fatal_log("EVENTRECV", std::string("do_event�����쳣��") + e.what());
						exit(-1);
					}
				});
			}
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