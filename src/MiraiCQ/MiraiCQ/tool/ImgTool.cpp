#include "ImgTool.h"

#include <httplib/httplib.h>
#include "StrTool.h"
#include "Md5Tool.h"
#include <curl/curl.h>


static CURLcode res = curl_global_init(CURL_GLOBAL_ALL);

ImgTool::ImgInfo ImgTool::parse_img(const std::string& body)
{
    ImgTool::ImgInfo ret_info;
    if (body.size() >= 26 &&  (unsigned char)body[0] == 0x42 && (unsigned char)body[1] == 0x4d)
    {
        //bmp img
        ret_info.type = "bmp";

        ret_info.width += (unsigned char)body[0x12];
        ret_info.width += (unsigned char)body[0x13] * (unsigned int)256;
        ret_info.width += (unsigned char)body[0x14] * (unsigned int)256 * 256;
        ret_info.width += (unsigned char)body[0x15] * (unsigned int)256 * 256 * 256;

        ret_info.height += (unsigned char)body[0x16];
        ret_info.height += (unsigned char)body[0x17] * (unsigned int)256;
        ret_info.height += (unsigned char)body[0x18] * (unsigned int)256 * 256;
        ret_info.height += (unsigned char)body[0x19] * (unsigned int)256 * 256 * 256;
         
    }
    else if (body.size() >= 24 && !memcmp(&body[12], "IHDR", 4) && !memcmp(&body[1], "PNG", 3))
    {
        ret_info.type = "png";
        ret_info.width += (unsigned char)body[0x13];
        ret_info.width += (unsigned char)body[0x12] * (unsigned int)256;
        ret_info.width += (unsigned char)body[0x11] * (unsigned int)256 * 256;
        ret_info.width += (unsigned char)body[0x10] * (unsigned int)256 * 256 * 256;

        ret_info.height += (unsigned char)body[0x17];
        ret_info.height += (unsigned char)body[0x16] * (unsigned int)256;
        ret_info.height += (unsigned char)body[0x15] * (unsigned int)256 * 256;
        ret_info.height += (unsigned char)body[0x14] * (unsigned int)256 * 256 * 256;
    }
    else if (body.size() >= 16 && !memcmp(&body[1], "PNG", 3))
    {
        ret_info.type = "png";
        ret_info.width += (unsigned char)body[11];
        ret_info.width += (unsigned char)body[10] * (unsigned int)256;
        ret_info.width += (unsigned char)body[9] * (unsigned int)256 * 256;
        ret_info.width += (unsigned char)body[8] * (unsigned int)256 * 256 * 256;

        ret_info.height += (unsigned char)body[15];
        ret_info.height += (unsigned char)body[14] * (unsigned int)256;
        ret_info.height += (unsigned char)body[13] * (unsigned int)256 * 256;
        ret_info.height += (unsigned char)body[12] * (unsigned int)256 * 256 * 256;
    }
    else if (body.size() >= 3 && (unsigned char)body[0] == 0xff && (unsigned char)body[1] == 0xd8)
    {
        ret_info.type = "jpg";
        size_t i = 2;
        unsigned char b = (unsigned char)body[i];
        while (b != 0xDA)
        {
            while (b != 0xFF)
            {
                ++i;
                if (i >= body.size())
                {
                    return ret_info;
                }
                b = (unsigned char)body[i];
            }
            while (b == 0xFF)
            {
                ++i;
                if (i >= body.size())
                {
                    return ret_info;
                }
                b = (unsigned char)body[i];
            }
            if (b >= 0xC0 && b <= 0xC3)
            {
                if (i + 8 > body.size())
                {
                    return ret_info;
                }
                ret_info.height = (unsigned char)body[i + 4] * (unsigned int)256 + (unsigned char)body[i + 5];
                ret_info.width = (unsigned char)body[i + 6] * (unsigned int)256 + (unsigned char)body[i + 7];
                return ret_info;
            }
            else
            {
                if (i + 2 >= body.size())
                {
                    return ret_info;
                }
                int off = (unsigned char)body[i + 1] * (unsigned int)256 + (unsigned char)body[i + 2] - 2;
                i += 2;
                if (i + off > body.size())
                {
                    return ret_info;
                }
                i += off;
            }
        }
    }
    else if(body.size() >= 10 && (!memcmp(&body[0], "GIF87a", 6) || !memcmp(&body[0], "GIF89a", 6)))
    {
        ret_info.type = "gif";
        ret_info.width = (unsigned char)body[6] + (unsigned char)body[7] * (unsigned int)256;
        ret_info.height = (unsigned char)body[8] + (unsigned char)body[9] * (unsigned int)256;
    }

    return ret_info;
}

static size_t cb(void* data, size_t size, size_t nmemb, void* content)
{
    size_t realsize = size * nmemb;
    ((std::string*)content)->append(std::string((char*)data, realsize));
    return realsize;
}

bool ImgTool::download_img(const std::string& url,std::string& content) 
{
    char err_buf_temp[CURL_ERROR_SIZE] = { 0 };
    long retCode = 0;
    content.clear();
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&content);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_buf_temp);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        return false;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retCode);
    curl_easy_cleanup(curl);
    if (retCode != 200) {
        return false;
    }
    return  true;
}
