#define _CRT_SECURE_NO_WARNINGS


//检测system32.config。为什么要config文件名字要设置成system32？因为这样万一对方会检查startup，说不定也可以通过文件名蒙混过关。
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

static void trim(wchar_t* str) {
    if (!str) return;
    // left
    wchar_t* s = str;
    while (*s == L' ' || *s == L'\t') s++;
    if (s != str) memmove(str, s, (wcslen(s) + 1) * sizeof(wchar_t));
    // right
    wchar_t* e = str + wcslen(str) - 1;
    while (e >= str && (*e == L' ' || *e == L'\t')) { *e = L'\0'; e--; }
}

Config* loadConfig(const wchar_t* filename) {
    FILE* fp = _wfopen(filename, L"r, ccs=UTF-8");
    if (!fp) return NULL;

    Config* cfg = calloc(1, sizeof(Config));
    cfg->min_time = 60;
    cfg->max_time = 240;
    cfg->scan_period = 300;
    cfg->proc_count = 0;
    cfg->time_count = 0;
    cfg->cmd_count = 0;
    cfg->proc_list = malloc(sizeof(wchar_t*) * 128);
    cfg->cmds = malloc(sizeof(wchar_t*) * 128);

    wchar_t line[2048];
    int in_period = 0, in_time = 0, in_random = 0, in_scan = 0, in_run = 0;
    int pending_start = -1;

    while (fgetws(line, 2048, fp)) {
        // strip newline
        for (wchar_t* p = line; *p; ++p)
            if (*p == L'\r' || *p == L'\n') *p = L'\0';
        trim(line);
        if (wcslen(line) == 0) {
            // empty line: if in_run and want to preserve empty command? 我们忽略空行
            continue;
        }

        // If currently inside Run { ... } block, handle separately
        if (in_run) {
            // if line contains closing brace '}', end block (allow other chars on same line trimmed)
            wchar_t* close = wcschr(line, L'}');
            if (close) {
                // if before '}' has content, treat as command
                *close = L'\0';
                trim(line);
                if (wcslen(line) > 0) {
                    cfg->cmds[cfg->cmd_count++] = _wcsdup(line);
                }
                in_run = 0;
                continue;
            }
            else {
                // normal command line inside block
                cfg->cmds[cfg->cmd_count++] = _wcsdup(line);
                continue;
            }
        }

        // detect section headers
        if (_wcsicmp(line, L"ScanPeriod:") == 0) {
            in_period = 1; in_time = in_random = in_scan = in_run = 0; continue;
        }
        if (_wcsicmp(line, L"ScannerRunningTime:") == 0) {
            in_time = 1; in_period = in_random = in_scan = in_run = 0; continue;
        }
        if (_wcsicmp(line, L"RandomTime:") == 0) {
            in_random = 1; in_time = in_period = in_scan = in_run = 0; continue;
        }
        if (_wcsicmp(line, L"ScanRunning:") == 0) {
            in_scan = 1; in_time = in_random = in_period = in_run = 0; continue;
        }
        // Run can be "Run" or "Run{" on same line. Accept both.
        if (_wcsnicmp(line, L"Run", 3) == 0) {
            // If line contains '{' then start block now; maybe there's content after '{' on same line.
            wchar_t* open = wcschr(line, L'{');
            if (open) {
                wchar_t* after = open + 1;
                trim(after);
                if (wcslen(after) > 0) {
                    // there is command content on same line before maybe closing brace
                    // check if there's a closing brace on same line
                    wchar_t* close = wcschr(after, L'}');
                    if (close) {
                        // inline single-line block: {cmd}
                        *close = L'\0';
                        trim(after);
                        if (wcslen(after) > 0) cfg->cmds[cfg->cmd_count++] = _wcsdup(after);
                        in_run = 0;
                        continue;
                    }
                    else {
                        // start block and first command is after
                        cfg->cmds[cfg->cmd_count++] = _wcsdup(after);
                        in_run = 1;
                        continue;
                    }
                }
                else {
                    // just "Run{", start block
                    in_run = 1;
                    continue;
                }
            }
            else {
                // line is "Run" (or "Run   "), expect next line to be "{"
                in_run = 2; // 2 means expecting opening brace next
                continue;
            }
        }

        // If previously saw "Run" and now expect opening brace:
        if (in_run == 2) {
            // expecting a line that is "{" or starts with "{"
            wchar_t* open = wcschr(line, L'{');
            if (open) {
                wchar_t* after = open + 1;
                trim(after);
                if (wcslen(after) > 0) {
                    wchar_t* close = wcschr(after, L'}');
                    if (close) {
                        *close = L'\0';
                        trim(after);
                        if (wcslen(after) > 0) cfg->cmds[cfg->cmd_count++] = _wcsdup(after);
                        in_run = 0;
                    }
                    else {
                        cfg->cmds[cfg->cmd_count++] = _wcsdup(after);
                        in_run = 1;
                    }
                }
                else {
                    in_run = 1;
                }
            }
            else {
                // malformed: didn't find '{' where expected. Ignore and reset.
                in_run = 0;
            }
            continue;
        }

        // handle regular sections
        if (in_period) {
            if (wcsncmp(line, L"time=", 5) == 0) {
                cfg->scan_period = _wtoi(line + 5);
            }
        }
        else if (in_time) {
            // lines like time1s=20 or time1e=5
            if (wcsstr(line, L"s=")) {
                pending_start = _wtoi(wcsstr(line, L"s=") + 2);
            }
            else if (wcsstr(line, L"e=")) {
                int end = _wtoi(wcsstr(line, L"e=") + 2);
                if (pending_start >= 0) {
                    cfg->times[cfg->time_count].start_hour = pending_start;
                    cfg->times[cfg->time_count].end_hour = end;
                    cfg->time_count++;
                    pending_start = -1;
                }
                else {
                    // if no pending start, create with default start 0
                    cfg->times[cfg->time_count].start_hour = 0;
                    cfg->times[cfg->time_count].end_hour = end;
                    cfg->time_count++;
                }
            }
        }
        else if (in_random) {
            if (wcsncmp(line, L"min=", 4) == 0)
                cfg->min_time = _wtoi(line + 4);
            else if (wcsncmp(line, L"max=", 4) == 0)
                cfg->max_time = _wtoi(line + 4);
        }
        else if (in_scan) {
            wchar_t* context = NULL;
            wchar_t* token = wcstok(line, L",", &context);
            while (token) {
                trim(token);
                if (wcslen(token) > 0)
                    cfg->proc_list[cfg->proc_count++] = _wcsdup(token);
                token = wcstok(NULL, L",", &context);
            }
        }
    }

    fclose(fp);
    return cfg;
}

void freeConfig(Config* cfg) {
    if (!cfg) return;
    for (int i = 0; i < cfg->proc_count; i++) free(cfg->proc_list[i]);
    free(cfg->proc_list);
    for (int i = 0; i < cfg->cmd_count; i++) free(cfg->cmds[i]);
    free(cfg->cmds);
    free(cfg);
}