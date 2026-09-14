#ifndef CAMERA_ABNORMAL_CONFIG_H
#define CAMERA_ABNORMAL_CONFIG_H

#include "common.h"

typedef struct {
    char camera_id[CA_MAX_ID];
    char rtsp_url[CA_MAX_URL];
    char upload_url[CA_MAX_URL];
    char work_dir[CA_MAX_PATH];
    char hikflow_model_path[CA_MAX_FULL_PATH];
    char abnormal_classes[128];
    char human_alarm_ip[64];
    int human_alarm_port;
    int listen_port;
    int app_chan;
    int pre_seconds;
    int post_seconds;
    int ring_seconds;
    int max_events;
    int upload_retry;
    int upload_retry_interval_ms;
    int cooldown_seconds;
    int infer_interval_seconds;
    CodecType codec;
    int fps;
    int debug_level;
    int test_event_interval_seconds;
} AppConfig;

void config_defaults(AppConfig *cfg);
int config_load(const char *path, AppConfig *cfg);

#endif
