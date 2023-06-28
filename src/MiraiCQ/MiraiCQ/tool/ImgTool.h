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
	* ���������ͼƬ��Ϣ
	* ����`url`��ͼƬ��url
	* ����`info`�����ͼƬ��Ϣ
	* ����ֵ���ɹ�����`true`��ʧ�ܷ���`false`
	*/
	static bool get_info(const std::string& url, ImgInfo & info, bool isqq);

	/*
	* ����������ͼƬ
	* ����`url`��ͼƬ��url
	* ����`save_path`��ͼƬ����·��
	* ����ֵ���ɹ�����`true`��ʧ�ܷ���`false`
	*/
	static bool download_img(const std::string& url, const std::string& save_path) ;

	/*
	* ���������ͼƬ��Ϣ
	* ����`body`��ͼƬ��body(����ֻ��һ����ͼƬ)
	* ����ֵ��ͼƬ��Ϣ(���ܲ�����)
	*/
	static ImgInfo parse_img(const std::string& body);
	

private:

};

