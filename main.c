#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

#include "config.h"
#include "ScanTheRunning.h" 

#define CONFIG_FILE L"system32.config"

static int isWithinTimeRange(Config* cfg) {
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    int hour = lt->tm_hour;
    for (int i = 0; i < cfg->time_count; i++) {
        int s = cfg->times[i].start_hour;
        int e = cfg->times[i].end_hour;
        if (s < 0 || e < 0 || s > 23 || e > 23) continue;
        if (s <= e) {
            if (hour >= s && hour < e) return 1;
        }
        else {
            if (hour >= s || hour < e) return 1;
        }
    }
    return 0;
}

int main(void) {
    Config* cfg = loadConfig(CONFIG_FILE);
    if (!cfg) {
        wprintf(L"无法加载配置文件 %s\n", CONFIG_FILE);
        return 0;
    }
    if (cfg->proc_count == 0) {
        wprintf(L"配置文件中未指定要检测的程序（ScanRunning）。\n");
        freeConfig(cfg);
        return 0;
    }
    if (cfg->time_count == 0) {
        wprintf(L"配置文件中未指定任何运行时段（ScannerRunningTime）。\n");
        freeConfig(cfg);
        return 0;
    }

    // 隐藏控制台窗口（需要查看日志则注释掉）
    HWND hwnd = GetConsoleWindow();
    //ShowWindow(hwnd, SW_HIDE);

    // 随机种子
    srand((unsigned int)(time(NULL) ^ GetTickCount()));

    wprintf(L"配置加载完成：检测周期=%d 秒，min=%d, max=%d, 目标进程=%d, 命令行=%d\n",
        cfg->scan_period, cfg->min_time, cfg->max_time, cfg->proc_count, cfg->cmd_count);

    while (1) {
        // 如果当前不在运行时段，休眠 60 秒再检查
        if (!isWithinTimeRange(cfg)) {
            Sleep(60 * 1000);
            continue;
        }

        // 到达检测时刻：每次检测前不会立刻进入随机断网，只有检测到进程才进入
        // 检测所有目标进程
        int found = 0;
        for (int i = 0; i < cfg->proc_count; i++) {
            if (ScanTheRunning(cfg->proc_list[i]) == 0) {
                found = 1;
                // 执行配置中 Run { ... } 中的每一行命令（若没有则默认断网）
                if (cfg->cmd_count > 0) {
                    for (int c = 0; c < cfg->cmd_count; c++) {
                        wchar_t* cmd = cfg->cmds[c];
                        if (cmd && wcslen(cmd) > 0) {
                            // 使用宽字符版本的 system
                            _wsystem(cmd);
                        }
                    }
                }
                else {
                    // 保持向后兼容：若未配置 Run 块，默认执行 netsh wlan disconnect
                    _wsystem(L"netsh wlan disconnect");
                }
                break; // 只要检测到任意一个配置进程就执行一次命令序列
            }
        }

        if (found) {
            // 进入随机断网等待期间：在此期间不检测（仅等待）
            int min = cfg->min_time;
            int max = cfg->max_time;
            if (max < min) max = min;
            int delay = (rand() % (max - min + 1)) + min;
            Sleep(delay * 1000);

            // 随机等待结束后，再等待 scan_period 秒，再进入下一次检测
            Sleep(cfg->scan_period * 1000);
        }
        else {
            // 未检测到，直接等待 scan_period 秒后再检测
            Sleep(cfg->scan_period * 1000);
        }
    }

    // unreachable, but keep cleanup
    freeConfig(cfg);
    return 0;
}