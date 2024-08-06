#pragma once

#include <string>

class ImgTool
{
public:
	struct ImgInfo
	{
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int size = 0;
		std::string type = "";
		std::string md5_str = "";
	};

	/*
	* ����������ͼƬ
	* ����`url`��ͼƬ��url
	* ����`content`��������ɵ�ͼƬ�ֽڼ�
	* ����ֵ���ɹ�����`true`��ʧ�ܷ���`false`
	*/
	static bool download_img(const std::string& url, std::string& content) ;

	/*
	* ���������ͼƬ��Ϣ
	* ����`body`��ͼƬ��body(����ֻ��һ����ͼƬ)
	* ����ֵ��ͼƬ��Ϣ(���ܲ�����)
	*/
	static ImgInfo parse_img(const std::string& body);
	

private:

};

