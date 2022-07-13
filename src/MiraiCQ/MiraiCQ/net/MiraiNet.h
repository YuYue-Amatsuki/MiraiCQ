#pragma once

#include <memory>
#include <string>
#include <vector>
#include <shared_mutex>
#include <map>
#include <mutex>
#include <atomic>

#include <jsoncpp/json.h>

class MiraiNet
{
public:
	using NetStruct = std::shared_ptr<Json::Value>;
	virtual ~MiraiNet();
	/*
	* ����������net
	* ����ֵ�����ӳɹ�����true�����򷵻�false
	*/
	virtual bool connect() = 0;

	/*
	* �������ж�net�Ƿ�����
	* ����ֵ���Ѿ����ӳɹ�����true�����򷵻�false
	*/
	virtual bool is_connect() = 0;

	/*
	* ����������net����
	* ����`senddat`��net����
	* ����`timeout`����ʱ����λms
	* ����ֵ�����ص��ý��
	*/
	virtual NetStruct call_fun(NetStruct senddat,int timeout,bool in_new_net = false) = 0;
	
	/*
	* ����������config�����Ѿ������򸲸�
	* ����`key`����
	* ����`value`��ֵ
	*/
	void set_config(const std::string & key, const std::string& value);

	/*
	* ����������config�����Ѿ������򸲸�
	* ����`key`����
	* ����`value`��ֵ
	*/
	void set_all_config(const std::map<std::string, std::string> & all_config);

	/*
	* ���������config
	* ����`key`����
	* ����ֵ��ֵ�������������򷵻�`""`
	*/
	std::string get_config(const std::string& key);

	/*
	* �������������config
	* ����ֵ������config
	*/
	std::map<std::string, std::string> get_all_config() ;

	/*
	* ����������¼���������¼�����
	* ����ֵ���¼�����
	*/
	std::vector<NetStruct> get_event(int timeout = 5000);

	static std::shared_ptr<MiraiNet> get_instance(const std::string & type) ;
	
protected:
	MiraiNet();
	/*
	* ����������¼����¼�����
	* �������¼�
	*/
	void add_event(NetStruct event);

private:
	std::shared_mutex mx_config_map;
	std::map<std::string, std::string> config_map;
	std::mutex mx_event_vec;
	std::condition_variable cv_event_vec;
	std::vector<NetStruct> event_vec;
	/* ��¼event_vec������С������������Ͼɵ����ݻᱻ���� */
	std::atomic_size_t buf_size = 128;
};
