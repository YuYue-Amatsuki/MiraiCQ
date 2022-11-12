#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <shared_mutex>
#include <atomic>

class MiraiPlus
{
public:

	struct PlusDef
	{
		struct Event
		{
			int type;
			int priority = 30000;
			std::string fun_name;
		};
		struct Menu
		{
			std::string fun_name;
			std::string name;
		};
		struct Process
		{
			Process(const std::string& dll_name,const std::string & uuid);
			/* �жϽ����Ƿ���� */
			bool is_exist();
			/* �ȴ������˳�,���غ���ִ���������Ƿ��˳� */
			void wait_process_quit(int timeout);
			/* ��ý���id */
			int get_pid();
			~Process();
		private:
			void* process_handle = NULL;
			void* job_handle = NULL;
		};
		std::string name; /* ������� */
		std::string filename; /* ������ļ��� */
		std::string version; /* ����汾 */
		std::string author; /* ������� */
		std::string description; /* ������� */
		std::string dll_name;  /* ������ļ���(������׺) */
		int ac; /* ���ac */
		std::vector<std::shared_ptr<const Event>> event_vec;
		std::vector<std::shared_ptr<const Menu>> menu_vec;
		// std::set<int> auth_vec;
		std::shared_ptr<Process> process = nullptr; /* ����Ľ��� */
		std::shared_mutex mx_plus_def;
		bool recive_ex_event = false;

		~PlusDef();
		/*
		* �������Ӳ���л��һ��Event
		* ����`type`��������type��
		* ����ֵ���ɹ�����Event��ʧ�ܷ���null
		*/
		const std::shared_ptr<const Event> get_event_fun(int type);

		/*
		* �������Ӳ���л��Menu����
		* ����ֵ���ɹ�����Menu���飬����ʧ��
		*/
		std::vector<std::shared_ptr<const Menu>> get_menu_vec() ;

		/*
		* ��������ò������
		* ����ֵ�����ز������
		*/
		std::string get_name() ;

		/*
		* ��������ò������
		* ����ֵ�����ز������
		*/
		std::string get_filename() ;

		/*
		* ���������ز��uuid��uuid��������������IPCͨѶ�������δ���ã��򷵻�""
		* ����ֵ�����ز��uuid
		*/
		std::string get_uuid();

		/*
		* ���������ò����uuid
		*/
		void set_uuid(const std::string & uuid);

		/*
		* ���������ز���Ƿ�����,ע�⣬�����ǲ�����ã�������̾�һ�����ڣ���Ϊ��������쳣�˳�
		*/
		bool is_enable();

		/*
		*  ���������ز�������Ƿ���ڣ�ע�⣬�����û�����ã���������һ��������
		*/
		bool is_process_exist();

		/*
		*  ���������ز�����̵�pid����������̲����ڣ��򷵻�-1
		*/
		int get_process_id();

		bool is_recive_ex_event();
	private:
		std::string uuid;
	};

	~MiraiPlus();

	/*
	* ����������һ��dll���
	* ����`dll_name`:dll���֣�������׺��
	* ����`err_msg`,����ʧ��ʱ˵��ԭ��
	* ����ֵ���ɹ�����`true`,ʧ�ܷ���`false`
	*/
	bool load_plus(const std::string& dll_name,std::string & err_msg) ;

	/*
	* ����������һ�����
	* ����`ac`:������
	* ����`err_msg`,����ʧ��ʱ˵��ԭ��
	* ����ֵ�����غ���ִ�к����Ƿ�����
	*/
	bool enable_plus(int ac,std::string & err_msg) ;

	/*
	* ����������һ�����
	* ����`ac`:������
	*/
	void disable_plus(int ac) ;

	/*
	* �������ж�һ������Ƿ�����
	* ����`ac`:������
	* ����ֵ���Ѿ������÷���`true`��δ���û���ac���󷵻�false
	*/
	bool is_enable(int ac) ;

	/*
	* ������ͨ��AC�����һ�����
	* ����`ac`:������
	* ����ֵ���ɹ����ز��ָ�룬ac��Ч����null
	*/
	std::shared_ptr<PlusDef> get_plus(int ac) ;

	/*
	* ��������ò��map
	* ����ֵ�����ز��mapָ�룬�˺�������ʧ��
	*/
	std::map<int, std::shared_ptr<PlusDef>> get_all_plus() ;

	/*
	* ��������ò��ac
	* ����ֵ������ac���飬�˺�������ʧ��
	*/
	std::vector<std::pair<int, std::weak_ptr<MiraiPlus::PlusDef>>> get_all_ac() ;

	static MiraiPlus* get_instance() ;
private:
	MiraiPlus();
	std::map<int, std::shared_ptr<PlusDef>> plus_map;
	std::shared_mutex mx_plus_map;
};

