#include "MiraiLog.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

#include <cstdio>
#include <atomic>

using namespace std;

extern bool g_is_plus;
extern std::string g_plus_name;

MiraiLog* MiraiLog::get_instance()
{
    static MiraiLog log;
    return &log;
}

void MiraiLog::add_debug_log(const string& category, const string& dat)
{
    add_log(Level::DEBUG, category, dat);
}

void MiraiLog::add_info_log(const string& category, const string& dat)
{
    add_log(Level::INFO, category, dat);
}

void MiraiLog::add_warning_log(const string& category, const string& dat)
{
    add_log(Level::WARNING, category, dat);
}

void MiraiLog::add_fatal_log(const string& category, const string& dat)
{
    add_log(Level::FATAL, category, dat);
}

void MiraiLog::add_front_sinks(front_sinks_funtype front_sinks, void* user_dat)
{
    unique_lock<shared_mutex> lock(add_log_mx);
    front_sinks_vec.push_back({ front_sinks, user_dat });
}

void MiraiLog::add_backend_sinks(backend_sinks_funtype backend_sinks, void* user_dat)
{
    unique_lock<shared_mutex> lock(add_log_mx);
    backend_sinks_vec.push_back({ backend_sinks, user_dat });
}
static string get_exe_dir()
{
    string ret_str;
    char path_str[MAX_PATH + 1] = { 0 };
    const DWORD n = GetModuleFileNameA(NULL, path_str, MAX_PATH);
    if (n == 0)
    {
        ret_str = "";
    }
    else
    {
        *(strrchr(path_str, '\\')) = '\0';
        ret_str = path_str + string("\\");
    }
    return ret_str;
}
MiraiLog::MiraiLog()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_every(std::chrono::seconds(3));
    if (!g_is_plus)
    {
        auto logger = spdlog::daily_logger_mt("MiraiLog", get_exe_dir() + "log/main_daily.txt", 2, 30);
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);
    }
    else {
        auto logger = spdlog::daily_logger_mt("MiraiLog", get_exe_dir() + "log/" + g_plus_name + "_plus_daily.txt", 2, 30);
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);
    }
    
}

/* ����һ����־ */
void MiraiLog::add_log(const Level& lv, const string& category, const string& dat)
{
    shared_lock<shared_mutex> lock(add_log_mx);
    /* ��������ǰ��sinks */
    pair<string, string> sinks_ret{ category ,dat };
    for (const auto& sinks : front_sinks_vec)
    {
        const auto slint_fun = sinks.first;
        const auto user_data = sinks.second;
        sinks_ret = slint_fun(lv, sinks_ret.first, sinks_ret.second, user_data);
    }

    /* �������к��sinks */
    for (const auto& sinks : backend_sinks_vec)
    {
        const auto slint_fun = sinks.first;
        const auto user_data = sinks.second;
        slint_fun(lv, sinks_ret.first, sinks_ret.second, user_data);
    }

    /* ���û���Զ���ĺ��sinks,�����Ĭ��sinks(��ӡ������̨��) */
    if (backend_sinks_vec.size() == 0)
    {
        default_backend_sinks(lv, sinks_ret.first, sinks_ret.second);
    }

}
/* Ĭ�ϵĺ��sinks */
void MiraiLog::default_backend_sinks(const Level& lv, const string& category, const string& dat)
{
    auto daily_logger = spdlog::get("MiraiLog");
    switch (lv)
    {
    case Level::DEBUG:
        daily_logger->debug("[{}]:{}", category, dat);
        spdlog::debug("[{}]:{}", category, dat);
        break;
    case Level::INFO:
        daily_logger->info("[{}]:{}", category, dat);
        spdlog::info("[{}]:{}", category, dat);
        break;
    case Level::WARNING:
        daily_logger->warn("[{}]:{}", category, dat);
        spdlog::warn("[{}]:{}", category, dat);
        break;
    case Level::FATAL:
        daily_logger->critical("[{}]:{}", category, dat);
        spdlog::critical("[{}]:{}", category, dat);
        break;
    default:
        break;
    }
    //���������ˢ�£������̲�ˢ��
    /*if (g_is_plus)
    {
        daily_logger->flush();
    }*/
}
    
