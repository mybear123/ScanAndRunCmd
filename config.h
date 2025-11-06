#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <wchar.h>

typedef struct {
    int start_hour;
    int end_hour;
} TimeRange;

typedef struct {
    int min_time;       // 随机最小时间（秒）
    int max_time;       // 随机最大时间（秒）
    int scan_period;    // 检测周期（秒）
    int proc_count;
    wchar_t** proc_list;
    int time_count;
    TimeRange times[16];
    int cmd_count;      // Run {} 中命令数量
    wchar_t** cmds;     // Run {} 中每一行命令
} Config;

Config* loadConfig(const wchar_t* filename);
void freeConfig(Config* cfg);

#endif