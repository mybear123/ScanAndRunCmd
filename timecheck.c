#include <time.h>
#include "timecheck.h"

//检查当前的时间

int isWithinTimeRange(Config* cfg) {
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    int hour = lt->tm_hour;

    for (int i = 0; i < cfg->time_count; i++) {
        int s = cfg->times[i].start_hour;
        int e = cfg->times[i].end_hour;
        if (s < 0 || e < 0) continue;

        if (s <= e) {
            if (hour >= s && hour < e) return 1;
        }
        else {
            if (hour >= s || hour < e) return 1;
        }
    }
    return 0;
}