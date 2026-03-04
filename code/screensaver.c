#include "screensaver.h"

#include <time.h>
#include <errno.h>
#include <limits.h>

#include "config.h"

#include "libcutils/logger.h"

static struct timespec g_tp_last_reset = {.tv_sec = INT_MAX};

void screensaver_reset(void)
{

    if (g_config.screensaver_delay_min == 0) {
        return;
    }


    if (clock_gettime(CLOCK_MONOTONIC, &g_tp_last_reset) != 0) {
        log_error("deactivating screensaver, failed to get monotonic clock timepoint: %s\n", strerror(errno));
        g_config.screensaver_delay_min = 0;
    }
}

bool screensaver_active(void)
{
    if (g_config.screensaver_delay_min == 0) { return false;}


    struct timespec tp_current = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &tp_current) != 0) {
        log_error("deactivating screensaver, failed to get monotonic clock timepoint: %s\n", strerror(errno));
        g_config.screensaver_delay_min = 0;
        return false;
    }

    const time_t diff_sec = (tp_current.tv_sec-g_tp_last_reset.tv_sec);

    return (diff_sec >= g_config.screensaver_delay_min*60) ? true : false;
}
