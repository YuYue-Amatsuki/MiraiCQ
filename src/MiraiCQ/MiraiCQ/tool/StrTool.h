#pragma once

#include <string>
#include <jsoncpp/json.h>

class StrTool
{
public:
	/*
	* ���������ַ����е�Сд��ĸתΪ��д
	* ����`str`��Ҫת�����ַ���
	* ����ֵ��ת������ַ���
	*/
	static std::string toupper(const std::string& str);
	/*
	* ���������ַ����еĴ�д��ĸתΪСд
	* ����`str`��Ҫת�����ַ���
	* ����ֵ��ת������ַ���
	*/
	static std::string tolower(const std::string& str);
	/*
	* ��������utf8�ַ���תΪansi
	* ����`utf8_str`��Ҫת�����ַ���
	* ����ֵ��ת������ַ���
	*/
	static std::string to_ansi(const std::string& utf8_str);
	/*
	* ��������ansi�ַ���תΪutf8
	* ����`ansi_str`��Ҫת�����ַ���
	* ����ֵ��ת������ַ���
	*/
	static std::string to_utf8(const std::string& ansi_str);
	/*
	* ����������uuid
	* ����ֵ���ɹ�����uuid��ʧ�ܷ���`""`
	*/
	static std::string gen_uuid() ;

	/*
	* ����������ac
	* ����ֵ������ac,����ʧ��,�����ظ�
	*/
	static int gen_ac() ;

	/*
	* �������ж��ַ����Ƿ���ĳ�ַ�����β
	* ����`str`��Ҫ�жϵ��ַ���
	* ����`end_str`����β�ַ���
	* ����ֵ����`str`��`end_str`��β���򷵻�`true`,���򷵻�`false`
	*/
	static bool end_with(const std::string& str, const std::string& end_str) ;

	/*
	* ������ȥ���ļ�����׺
	* ����`file_str`��Ҫȥ����׺���ļ���
	* ����ֵ��ȥ����׺����ļ��������޺�׺���򷵻��ļ���
	*/
	static std::string remove_suffix(const std::string& file_str) ;

	/*
	* ��������json�л�ȡ�ַ���
	* ����`json`��json
	* ����`key`����
	* ����`default_value`�����������󣬷��� `default_value`
	* ����ֵ��`key`����Ӧ��value
	*/
	static std::string get_str_from_json(const Json::Value & json,const std::string & key, const std::string& default_value) ;

	/*
	* ��������json�л�ȡint
	* ����`json`��json
	* ����`key`����
	* ����`default_value`�����������󣬷��� `default_value`
	* ����ֵ��`key`����Ӧ��value
	*/
	static int get_int_from_json(const Json::Value& json, const std::string& key, int default_value) ;
	
	/*
	* ��������json�л�ȡint64
	* ����`json`��json
	* ����`key`����
	* ����`default_value`�����������󣬷��� `default_value`
	* ����ֵ��`key`����Ӧ��value
	*/
	static int64_t get_int64_from_json(const Json::Value& json, const std::string& key, int64_t default_value) ;

	/*
	* ��������json�л�ȡbool
	* ����`json`��json
	* ����`key`����
	* ����`default_value`�����������󣬷��� `default_value`
	* ����ֵ��`key`����Ӧ��value
	*/
	static bool get_bool_from_json(const Json::Value& json, const std::string& key, bool default_value) ;


	/*
	* ��������cq��str��Ϣ��ʽת��Ϊjsonarr
	* ����`cq_str`��Ҫת����cq_str
	* ����`mode`��0:ֱ��ת�� 1:���˵��ǿ�Q��CQ��
	* ����ֵ���ɹ�����jsonarr��ʧ�ܷ���`Json::Value()`
	*/
	static Json::Value cq_str_to_jsonarr(const std::string & cq_str, int mode = 0) ;

	/*
	* ��������jsonarrת��Ϊcq��str��Ϣ��ʽ
	* ����`jsonarr`��Ҫת����jsonarr
	* ����`mode`��0:ֱ��ת�� 1:���˵��ǿ�Q��CQ��
	* ����ֵ���ɹ�����cqstr��ʧ�ܷ���`""`
	*/
	static std::string jsonarr_to_cq_str(const Json::Value& jsonarr, int mode = 0) ;

	/*
	* �������ַ����滻
	* ����`str`��Ҫ�滻���ַ���
	* ����`old_value`����ֵ
	* ����`new_value`����ֵ
	*/
	static void replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value) ;

	/*
	* ��������ini�ļ��ж�ȡ�ַ���
	* ����`file`��ini�ļ�·��
	* ����`section`����
	* ����`key`����
	* ����`key`������ȡʧ�ܣ����ص�Ĭ��ֵ
	* ����ֵ����ȡ���ַ���
	*/
	static std::string get_str_from_ini(const std::string& file, const std::string& section, const std::string& key, const std::string& default_value) ;

	/*
	* ������ ʹ��������ƥ��
	* ����`content`��Ҫƥ�������
	* ����`pattern`��������ʽ
	* ����ֵ��ƥ��õ�������
	*/
	static std::vector<std::string> match(const std::string& content, const std::string& pattern);

	/*
	* ������ʹ��������ƥ������
	* ����`content`��Ҫƥ�������
	* ����`pattern`��������ʽ
	* ����ֵ��ƥ��õ�������
	*/
	static std::vector<std::vector<std::string>> match_all(const std::string& content, const std::string& pattern);

	/*
	* �������ж��Ƿ���utf8����
	* ����`text`��Ҫ�жϵ�����
	* ����ֵ���Ƿ���utf8����
	*/
	static bool is_utf8(const std::string& text);

	/*
	* ������ȥ�����˿հ��ַ�
	* ����`text`����
	*/
	static void trim(std::string& text);



private:
	StrTool();
};

