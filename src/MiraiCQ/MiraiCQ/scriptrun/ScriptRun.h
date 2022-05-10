#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include <jsoncpp/json.h>

class ScriptRun
{
public:
	// ��ʼ���ű�/��װ�ű�
	void init();

	// ���ڹ���/�޸�����Center��onebot����
	bool onebot_event_filter(const char * dat);

	// ���ڹ���/�޸�����Plus��onebot����
	bool onebot_api_filter(const std::string & filename, const char* dat);

	static ScriptRun* get_instance();
private:
	void init_t();
	std::shared_mutex mx_lua_sta;
	void* lua_sta = 0;
	ScriptRun() {};
};

