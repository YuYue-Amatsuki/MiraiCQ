#pragma once

#include <string>

class EmojiTool
{
public:
	/*
	* ���������ַ����е� emoji תΪ cq emoji ��
	* ����`str`��Ҫת�����ַ���
	* ����ֵ��ת������ַ���
	*/
	static std::string escape_cq_emoji(const std::string& str);
	/*
	* ���������ַ����е� cq emoji �� תΪ emoji 
	* ����`str`��Ҫת�����ַ���
	* ����ֵ��ת������ַ���
	*/
	static std::string unescape_cq_emoji(const std::string& str);
	/*
	* ����������utf-8�ַ��ĳ���
	* ����`str`��Ҫ������ַ���
	* ����`offset`���ַ����ڵ�λ��
	* ����ֵ��utf-8�ַ��ĳ���
	*/
	static int utf8_next_len(const std::string& str, int offset);

private:
	EmojiTool();
};

