#include "MiraiPlus.h"

#include <Windows.h>
#include <mutex>
#include <fstream>
#include <algorithm>

#include "../log/MiraiLog.h"
#include "../tool/PathTool.h"
#include "../tool/StrTool.h"
#include "../tool/TimeTool.h"
#include "../tool/ThreadTool.h"
#include "../tool/IPCTool.h"
#include "../config/config.h"

#include <jsoncpp/json.h>
#include <assert.h>
#include <websocketpp/base64/base64.hpp>


using namespace std;
MiraiPlus::MiraiPlus()
{
}

Json::Value MiraiPlus::read_plus_json(const std::string& json_path, std::string& err_msg)
{
	std::string json_file;
	try {
		json_file = PathTool::read_biniary_file(json_path);
		if (StrTool::is_utf8(json_file)) {
			json_file = StrTool::to_ansi(json_file);
		}
	}
	catch (...) {
		err_msg = "ģ��json�ļ���ȡʧ��";
		return Json::Value();
	}
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(json_file, root,true))
	{
		err_msg = "ģ��json�ļ�����ʧ��";
		return Json::Value();
	}
	return root;
}

MiraiPlus::~MiraiPlus()
{
}


bool MiraiPlus::load_dll_plus(const std::string& plus_name, std::string& err_msg)
{
	err_msg.clear();

	/* ����ͼƬĿ¼ */
	PathTool::create_dir(PathTool::get_exe_dir() + "data\\");
	PathTool::create_dir(PathTool::get_exe_dir() + "data\\image");

	std::string dll_path = PathTool::get_exe_dir() + "app\\" + plus_name + ".dll";
	std::string json_path = PathTool::get_exe_dir() + "app\\" + plus_name + ".json";
	if (!PathTool::is_file_exist(dll_path))
	{
		err_msg = "ģ��dll�ļ�������";
		return false;
	}
	if (!PathTool::is_file_exist(json_path))
	{
		err_msg = "ģ��json�ļ�������";
		return false;
	}

	Json::Value root = read_plus_json(json_path, err_msg);
	if (err_msg != "") {
		return false;
	}

	std::shared_ptr<PlusDef> plus_def(new PlusDef);

	plus_def->filename = plus_name;

	Json::Value def_str = "";
	{
		auto name_json = root.get("name", def_str);
		if (name_json.isString() && name_json.asString() != "")
		{
			plus_def->name = name_json.asString();
		}
		else
		{
			err_msg = "ģ��json�ļ�����ʧ��,ȱ��name";
			return false;
		}
	}

	{
		auto version_json = root.get("version", def_str);
		if (version_json.isString() && version_json.asString() != "")
		{
			plus_def->version = version_json.asString();
		}
		else
		{
			err_msg = "ģ��json�ļ�����ʧ��,ȱ��version";
			return false;
		}
	}

	// �Ƿ���ն����event
	plus_def->recive_ex_event = StrTool::get_bool_from_json(root, "recive_ex_event", false);

	// �Ƿ���ն����poke event
	plus_def->recive_poke_event = StrTool::get_bool_from_json(root, "recive_poke_event", false);

	{
		auto author_json = root.get("author", def_str);
		if (author_json.isString() && author_json.asString() != "")
		{
			plus_def->author = author_json.asString();
		}
		else
		{
			err_msg = "ģ��json�ļ�����ʧ��,ȱ��author";
			return false;
		}
	}

	{
		auto description_json = root.get("description", def_str);
		if (description_json.isString())
		{
			plus_def->description = description_json.asString();
		}
		else
		{
			err_msg = "ģ��json�ļ�����ʧ��,ȱ��description";
			return false;
		}
	}

	auto event_json_arr = root.get("event", "");
	if (event_json_arr.isArray())
	{
		for (auto& node : event_json_arr)
		{
			auto event = make_shared<PlusDef::Event>();
			{
				auto fun_name_json = node.get("function", def_str);
				if (fun_name_json.isString() && fun_name_json.asString() != "")
				{
					event->fun_name = fun_name_json.asString();
				}
				else
				{
					err_msg = "ģ��json�ļ�����ʧ��,ȱ��function";
					return false;
				}
			}
			{
				auto type_json = node.get("type", def_str);
				if (type_json.isInt())
				{
					event->type = type_json.asInt();
				}
				else
				{
					err_msg = "ģ��json�ļ�����ʧ��,event����`" + event->fun_name + "`ȱ��type";
					return false;
				}
			}
			{
				auto priority_json = node.get("priority", def_str);
				if (priority_json.isInt())
				{
					event->priority = priority_json.asInt();
				}
				else
				{
					event->priority = 30000;
					std::string msg = "����`" + event->fun_name + "`ȱ��priority,ʹ��Ĭ��ֵ30000";
					MiraiLog::get_instance()->add_debug_log("load_plus", msg);
				}
			}

			plus_def->event_vec.push_back(event);
		}
	}
	auto meun_json_arr = root.get("menu", "");
	if (meun_json_arr.isArray())
	{
		for (auto& node : meun_json_arr)
		{
			auto menu = make_shared<PlusDef::Menu>();
			{
				auto fun_name_json = node.get("function", def_str);
				if (fun_name_json.isString() && fun_name_json.asString() != "")
				{
					menu->fun_name = fun_name_json.asString();
				}
				else
				{
					err_msg = "ģ��json�ļ�����ʧ��,ȱ��function";
					return false;
				}
				auto name_json = node.get("name", def_str);
				if (name_json.isString() && name_json.asString() != "")
				{
					menu->name = name_json.asString();
				}
				else
				{
					err_msg = "ģ��json�ļ�����ʧ��,ȱ��name";
					return false;
				}
			}
			plus_def->menu_vec.push_back(menu);
		}
	}
	plus_def->dll_name = plus_name;
	plus_def->ac = StrTool::gen_ac();
	plus_def->plus_type = "dll";

	{
		unique_lock<shared_mutex> lock(this->mx_plus_map);
		plus_map[plus_def->ac] = plus_def;
	}

	return true;
}

bool MiraiPlus::load_py_plus(const std::string& plus_name, std::string& err_msg)
{
	err_msg.clear();
	std::string main_path = PathTool::get_exe_dir() + "pyapp\\" + plus_name + "\\main.py";
	std::string json_path = PathTool::get_exe_dir() + "pyapp\\" + plus_name + "\\main.json";
	if (!PathTool::is_file_exist(main_path))
	{
		err_msg = main_path + " �ļ�������";
		return false;
	}
	if (!PathTool::is_file_exist(json_path))
	{
		err_msg = json_path + " �ļ�������";
		return false;
	}

	Json::Value root = read_plus_json(json_path, err_msg);
	if (err_msg != "") {
		return false;
	}

	std::shared_ptr<PlusDef> plus_def(new PlusDef);

	plus_def->filename = plus_name;
	plus_def->dll_name = plus_name;
	plus_def->author = StrTool::get_str_from_json(root, "author", "δ֪");
	plus_def->description = StrTool::get_str_from_json(root, "description", "��");
	plus_def->name = StrTool::get_str_from_json(root, "name", "����");
	plus_def->version = StrTool::get_str_from_json(root, "version", "δ֪");
	plus_def->plus_type = "py";
	plus_def->ac = StrTool::gen_ac();
	{
		unique_lock<shared_mutex> lock(this->mx_plus_map);
		plus_map[plus_def->ac] = plus_def;
	}

	return true;

}

bool MiraiPlus::load_plus(const std::string& plus_name, std::string plus_type ,std::string & err_msg) 
{
	err_msg.clear();
	if (plus_type == "dll") {
		return load_dll_plus(plus_name, err_msg);
	}else {
		return load_py_plus(plus_name, err_msg);
	}
}

bool MiraiPlus::enable_plus(int ac, std::string & err_msg) 
{
	err_msg.clear();
	std::shared_ptr<PlusDef> plus = get_plus(ac);
	if (!plus) 
	{
		err_msg = "��Ч��ac";
		return false;
	}
	
	{
		// ������̴��ڣ�˵������Ѿ�����
		shared_lock<shared_mutex> lock(plus->mx_plus_def);
		if (plus->process) {
			return true;
		}
	}

	// ����UUID,������uuid���������̣������ò��ʼ����˳������API
	std::string uuid = StrTool::gen_uuid();
	plus->set_uuid(uuid);
	assert(uuid != "");

	/* ����������� */
	std::shared_ptr<MiraiPlus::PlusDef::Process> proc = std::make_shared<MiraiPlus::PlusDef::Process>(plus->dll_name,uuid, plus->plus_type);
	
	{
		unique_lock<shared_mutex> lock(plus->mx_plus_def);
		plus->process = proc;
	}

	if (plus->plus_type == "dll") {
		// �ȴ�������̼���
		Json::Value to_send;
		to_send["action"] = "is_load";
		bool is_load = false;
		for (int i = 0; i < 100; ++i)
		{
			const char* ret = IPC_ApiSend(uuid.c_str(), Json::FastWriter().write(to_send).c_str(), 100);
			if (strcmp(ret, "OK") == 0)
			{
				is_load = true;
				break;
			}
		}
		if (!is_load)
		{
			err_msg = "�������Ӧ";
			plus->set_uuid("");
			{
				unique_lock<shared_mutex> lock(plus->mx_plus_def);
				plus->process = nullptr;
			}
			return false;
		}
	}
	else {
		// py�����do nothing
	}

	return true;
}

void MiraiPlus::disable_plus(int ac) 
{
	std::shared_ptr<PlusDef> plus = get_plus(ac);
	
	if (!plus) {
		// �����AC
		return ;
	}

	// ���δ����
	if (!plus->is_enable()) {
		return ;
	}

	if (plus->plus_type == "dll") {
		Json::Value to_send;
		to_send["event_type"] = "exit";
		// �����˳��¼�
		IPC_SendEvent(plus->get_uuid().c_str(), to_send.toStyledString().c_str());
		// �ȴ������˳�(5s)
		{
			shared_lock<shared_mutex> lock(mx_plus_map);
			plus->process->wait_process_quit(5000);
		}
	}
	{
		unique_lock<shared_mutex> lock(mx_plus_map);
		plus->process = nullptr;
	}
	plus->set_uuid("");

}

bool MiraiPlus::is_enable(int ac) 
{
	std::shared_ptr<PlusDef> plus = get_plus(ac);
	if (!plus) {
		return false;
	}
	return plus->is_enable();
}

std::shared_ptr<MiraiPlus::PlusDef> MiraiPlus::get_plus(int ac) 
{
	shared_lock<shared_mutex> lock(mx_plus_map);
	auto iter = plus_map.find(ac);
	if (iter == plus_map.end())
	{
		return nullptr;
	}
	return iter->second;
	
}

std::map<int, std::shared_ptr<MiraiPlus::PlusDef>> MiraiPlus::get_all_plus() 
{
	shared_lock<shared_mutex> lock(mx_plus_map);
	return plus_map;
}

std::vector<std::pair<int,std::weak_ptr<MiraiPlus::PlusDef>>> MiraiPlus::get_all_ac() 
{
	shared_lock<shared_mutex> lock(mx_plus_map);
	std::vector<std::pair<int, std::weak_ptr<MiraiPlus::PlusDef>>> ret_vec;
	for (auto& it : plus_map)
	{
		ret_vec.push_back({ it.first,it.second });
	}
	return ret_vec;
}

MiraiPlus* MiraiPlus::get_instance() 
{
	static MiraiPlus mirai_plus;
	return &mirai_plus;
}

MiraiPlus::PlusDef::~PlusDef()
{

}

const std::shared_ptr<const MiraiPlus::PlusDef::Event> MiraiPlus::PlusDef::get_event_fun(int type) 
{
	for (const auto& fun : event_vec)
	{
		if (fun->type == type)
		{
			return fun;
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<const MiraiPlus::PlusDef::Menu>> MiraiPlus::PlusDef::get_menu_vec() 
{
	return menu_vec;
}

std::string MiraiPlus::PlusDef::get_name() 
{
	return this->name;
}

std::string MiraiPlus::PlusDef::get_filename() 
{
	return this->filename;
}

std::string MiraiPlus::PlusDef::get_uuid()
{
	std::shared_lock<std::shared_mutex> lk(this->mx_plus_def);
	return this->uuid;
}

void MiraiPlus::PlusDef::set_uuid(const std::string & uuid)
{
	std::unique_lock<std::shared_mutex> lk(this->mx_plus_def);
	this->uuid = uuid;
}

bool MiraiPlus::PlusDef::is_enable()
{
	shared_lock<shared_mutex> lock(mx_plus_def);
	return (process != nullptr);
}



bool MiraiPlus::PlusDef::is_process_exist()
{
	shared_lock<shared_mutex> lock(mx_plus_def);
	if (process && process->is_exist()) {
		return true;
	}
	return false;
}

int MiraiPlus::PlusDef::get_process_id()
{
	shared_lock<shared_mutex> lock(mx_plus_def);
	if (process) {
		return process->get_pid();
	}
	return -1;
}

bool MiraiPlus::PlusDef::is_recive_ex_event()
{
	shared_lock<shared_mutex> lock(mx_plus_def);
	return this->recive_ex_event;
}

bool MiraiPlus::PlusDef::is_recive_poke_event()
{
	shared_lock<shared_mutex> lock(mx_plus_def);
	return this->recive_poke_event;
}

MiraiPlus::PlusDef::Process::Process(const std::string& dll_name,const std::string & uuid,std::string plus_type)
{

	std::string cmd = "";
	PROCESS_INFORMATION pi;
	if (plus_type == "dll") {
		char path_str[MAX_PATH + 1] = { 0 };
		if (GetModuleFileNameA(NULL, path_str, MAX_PATH) == 0)
		{
			MiraiLog::get_instance()->add_fatal_log("PLUSLOAD", "��õ�ǰexe����ʧ��");
			exit(-1);
		}
		cmd = "\"" + string(path_str) + "\"";
		cmd += " ";
		cmd += IPC_GetFlag();
		cmd += " ";
		cmd += uuid;
		cmd += " ";
		cmd += websocketpp::base64_encode(dll_name);
		STARTUPINFO si = { sizeof(si) };
		MiraiLog::get_instance()->add_debug_log("PLUSLOAD", cmd);
		BOOL bRet = CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, DETACHED_PROCESS | CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
		if (bRet != TRUE) {
			bRet = CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi);
			if (bRet != TRUE) {
				MiraiLog::get_instance()->add_fatal_log("PLUSLOAD", "�����������ʧ��");
				exit(-1);
			}
		}
	}
	else {
		Config::get_instance()->get_access_token();
		Json::Value root;
		root["ws_url"] = Config::get_instance()->get_ws_url();
		root["access_token"] = Config::get_instance()->get_access_token();
		std::string env = websocketpp::base64_encode(Json::FastWriter().write(root));
		std::string exe_dir = PathTool::get_exe_dir();
		cmd = "\"" + exe_dir + "bin\\Python38-32\\python.exe\" \"" + exe_dir + "pyapp\\" + dll_name + "\\main.py\" " + env;
		std::string work_path = exe_dir + "pyapp\\" + dll_name;
		STARTUPINFO si = { sizeof(si) };
		MiraiLog::get_instance()->add_debug_log("PLUSLOAD", cmd);
		BOOL bRet = CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_BREAKAWAY_FROM_JOB, NULL, work_path.c_str(), &si, &pi);
		if (bRet != TRUE) {
			bRet = CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, work_path.c_str(), &si, &pi);
			if (bRet != TRUE) {
				MiraiLog::get_instance()->add_fatal_log("PLUSLOAD", "�����������ʧ��");
				exit(-1);
			}
		}
	}
	
	CloseHandle(pi.hThread);
	this->process_handle = pi.hProcess;
	//�������̣�ȷ�������̽������ӽ�����ǿ���˳�
	HANDLE job = CreateJobObjectA(NULL, NULL);
	if (job == NULL) {
		MiraiLog::get_instance()->add_fatal_log("PLUSLOAD", "CreateJobObjectAʧ��");
		exit(-1);
	}
	this->job_handle = job;
	BOOL ret = AssignProcessToJobObject(job, this->process_handle);
	if (ret != TRUE) {
		MiraiLog::get_instance()->add_fatal_log("PLUSLOAD", "���ӽ��̸��ӵ�������ʧ��1");
		exit(-1);
	}
	assert(ret == TRUE);
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_info = {0};
	limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	ret = SetInformationJobObject(
		job,
		JobObjectExtendedLimitInformation,
		&limit_info,
		sizeof(limit_info)
	);
	if (ret != TRUE) {
		MiraiLog::get_instance()->add_fatal_log("PLUSLOAD", "�����ӽ����Զ�����ʧ��");
		exit(-1);
	}
}

bool MiraiPlus::PlusDef::Process::is_exist()
{
	assert(process_handle != NULL);
	DWORD dw = WaitForSingleObject(process_handle, 1);
	if (dw == WAIT_OBJECT_0 || dw == WAIT_FAILED) {
		return false;
	}
	return true;
}

int MiraiPlus::PlusDef::Process::get_pid()
{
	assert(process_handle != NULL);
	if (is_exist()) {
		return GetProcessId(process_handle);
	}
	return -1;
}

void MiraiPlus::PlusDef::Process::wait_process_quit(int timeout)
{
	DWORD dw = WaitForSingleObject(process_handle, timeout);
}

MiraiPlus::PlusDef::Process::~Process()
{
	//TerminateProcess((HANDLE)process_handle, 0);
	CloseHandle(process_handle);
	CloseHandle(job_handle);
}
