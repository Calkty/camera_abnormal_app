#include "config.h"

#include <stdlib.h>

static void trim(char *s)
{
    char *p = s;
    size_t n;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }
    n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' ||
                     s[n - 1] == '\r' || s[n - 1] == '\n')) {
        s[--n] = '\0';
    }
}

void config_defaults(AppConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->camera_id, sizeof(cfg->camera_id), "ptz_001");
    snprintf(cfg->rtsp_url, sizeof(cfg->rtsp_url),
             "rtsp://127.0.0.1:554/ISAPI/Streaming/channels/101");
    snprintf(cfg->upload_url, sizeof(cfg->upload_url),
             "http://127.0.0.1:8080/api/upload");
    snprintf(cfg->work_dir, sizeof(cfg->work_dir), "/tmp/camera_abnormal");
    cfg->hikflow_model_path[0] = '\0';
    cfg->abnormal_classes[0] = '\0';
    snprintf(cfg->human_alarm_ip, sizeof(cfg->human_alarm_ip), "10.184.142.15");
    cfg->human_alarm_port = 7200;
    cfg->app_chan = -1;
    cfg->pre_seconds = 5;
    cfg->post_seconds = 10;
    cfg->ring_seconds = 30;
    cfg->max_events = 4;
    cfg->upload_retry = 3;
    cfg->upload_retry_interval_ms = 3000;
    cfg->cooldown_seconds = 20;
    cfg->infer_interval_seconds = 0;
    cfg->codec = CODEC_UNKNOWN;
    cfg->fps = 25;
    cfg->debug_level = 0;
}

static CodecType parse_codec(const char *v)
{
    if (strcmp(v, "h264") == 0 || strcmp(v, "H264") == 0) {
        return CODEC_H264;
    }
    if (strcmp(v, "h265") == 0 || strcmp(v, "H265") == 0 ||
        strcmp(v, "hevc") == 0 || strcmp(v, "HEVC") == 0) {
        return CODEC_H265;
    }
    return CODEC_UNKNOWN;
}

int config_load(const char *path, AppConfig *cfg)
{
    FILE *fp;
    char line[1024];
    config_defaults(cfg);
    fp = fopen(path, "r");
    if (!fp) {
        ca_log("WARN", "config %s not found, using defaults", path);
        return CA_OK;
    }
    while (fgets(line, sizeof(line), fp)) {
        char *eq;
        trim(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        eq = strchr(line, '=');
        if (!eq) {
            continue;
        }
        *eq = '\0';
        trim(line);
        trim(eq + 1);
        if (strcmp(line, "camera_id") == 0) {
            snprintf(cfg->camera_id, sizeof(cfg->camera_id), "%s", eq + 1);
        } else if (strcmp(line, "rtsp_url") == 0) {
            snprintf(cfg->rtsp_url, sizeof(cfg->rtsp_url), "%s", eq + 1);
        } else if (strcmp(line, "upload_url") == 0) {
            snprintf(cfg->upload_url, sizeof(cfg->upload_url), "%s", eq + 1);
        } else if (strcmp(line, "work_dir") == 0) {
            snprintf(cfg->work_dir, sizeof(cfg->work_dir), "%s", eq + 1);
        } else if (strcmp(line, "hikflow_model_path") == 0) {
            snprintf(cfg->hikflow_model_path, sizeof(cfg->hikflow_model_path), "%s", eq + 1);
        } else if (strcmp(line, "abnormal_classes") == 0) {
            snprintf(cfg->abnormal_classes, sizeof(cfg->abnormal_classes), "%s", eq + 1);
        } else if (strcmp(line, "human_alarm_ip") == 0) {
            snprintf(cfg->human_alarm_ip, sizeof(cfg->human_alarm_ip), "%s", eq + 1);
        } else if (strcmp(line, "human_alarm_port") == 0) {
            cfg->human_alarm_port = atoi(eq + 1);
        } else if (strcmp(line, "listen_port") == 0) {
            cfg->listen_port = atoi(eq + 1);
        } else if (strcmp(line, "app_chan") == 0) {
            cfg->app_chan = atoi(eq + 1);
        } else if (strcmp(line, "pre_seconds") == 0) {
            cfg->pre_seconds = atoi(eq + 1);
        } else if (strcmp(line, "post_seconds") == 0) {
            cfg->post_seconds = atoi(eq + 1);
        } else if (strcmp(line, "ring_seconds") == 0) {
            cfg->ring_seconds = atoi(eq + 1);
        } else if (strcmp(line, "max_events") == 0) {
            cfg->max_events = atoi(eq + 1);
        } else if (strcmp(line, "upload_retry") == 0) {
            cfg->upload_retry = atoi(eq + 1);
        } else if (strcmp(line, "upload_retry_interval_ms") == 0) {
            cfg->upload_retry_interval_ms = atoi(eq + 1);
        } else if (strcmp(line, "cooldown_seconds") == 0) {
            cfg->cooldown_seconds = atoi(eq + 1);
        } else if (strcmp(line, "infer_interval_seconds") == 0) {
            cfg->infer_interval_seconds = atoi(eq + 1);
        } else if (strcmp(line, "codec") == 0) {
            cfg->codec = parse_codec(eq + 1);
        } else if (strcmp(line, "fps") == 0) {
            cfg->fps = atoi(eq + 1);
        } else if (strcmp(line, "debug_level") == 0) {
            cfg->debug_level = atoi(eq + 1);
        } else if (strcmp(line, "test_event_interval_seconds") == 0) {
            cfg->test_event_interval_seconds = atoi(eq + 1);
        }
    }
    fclose(fp);
    if (cfg->ring_seconds < cfg->pre_seconds + 5) {
        cfg->ring_seconds = cfg->pre_seconds + 5;
    }
    if (cfg->max_events < 1) {
        cfg->max_events = 1;
    }
    if (cfg->fps <= 0) {
        cfg->fps = 25;
    }
    if (cfg->human_alarm_port <= 0) {
        cfg->human_alarm_port = 7200;
    }
    if (cfg->infer_interval_seconds < 0) {
        cfg->infer_interval_seconds = 0;
    }
    if (cfg->upload_retry <= 0) {
        cfg->upload_retry = 3;
    }
    if (cfg->upload_retry_interval_ms <= 0) {
        cfg->upload_retry_interval_ms = 3000;
    }
    if (cfg->debug_level < 0) {
        cfg->debug_level = 0;
    }
    return CA_OK;
}
