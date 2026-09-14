#include "infer_adapter.h"

#include "event_bridge.h"
#include "module_flags.h"

#include <pthread.h>

int audio_play_init(void);
void alarm_set_http_target(const char *ip, int port);
void alarm_server_task(void);
void *alarm_process(void *arg);
int target_detect_opdevsdk_init(char *app_name_chan);
int hikflow_demo_param_check(int argc, char *argv[]);
int hikflow_demo_init(int argc, char *argv[]);
void hikflow_demo_set_infer_interval_ms(int interval_ms);
void hikflow_demo_set_runtime_options(const char *model_path, const char *abnormal_classes);
int multi_pdc_config_init(void);

static EventQueue *g_human_event_queue = NULL;
static int32_t g_human_app_chan = -1;
static int g_human_inited = 0;

int32_t getAppRunningChan(int32_t *chan)
{
    if (g_human_app_chan >= 0 && chan) {
        *chan = g_human_app_chan;
        return 0;
    }
    return -1;
}

void camera_abnormal_on_human_alarm(int64_t event_wall_ms, float confidence)
{
    int64_t now_ms = ca_now_ms();
    ca_debug_log(1, "model event: timestamp=%lld confidence=%.3f clip_enabled=%d",
                 (long long)event_wall_ms, confidence, CA_ENABLE_CLIP);
    if (!g_human_event_queue) {
        return;
    }
    if (event_wall_ms < 946684800000LL || event_wall_ms > now_ms + 60000) {
        ca_debug_log(1, "human alarm timestamp adjusted: input=%lld now=%lld",
                     (long long)event_wall_ms, (long long)now_ms);
        event_wall_ms = now_ms;
    }
    ca_debug_log(1, "human alarm bridge: event_wall_ms=%lld confidence=%.3f",
                 (long long)event_wall_ms, confidence);
    abnormal_event_publish(g_human_event_queue, "human_abnormal", confidence, event_wall_ms);
}

int abnormal_event_publish(EventQueue *queue, const char *event_type, float confidence,
                           int64_t event_wall_ms)
{
    AbnormalEvent ev;
    if (!queue) return CA_ERR;
    memset(&ev, 0, sizeof(ev));
    ev.event_wall_ms = event_wall_ms;
    ev.detect_done_ms = ca_now_ms();
    ev.channel = 0;
    ev.confidence = confidence;
    snprintf(ev.event_type, sizeof(ev.event_type), "%s", event_type ? event_type : "abnormal");
    if (event_queue_push(queue, &ev, 0) != CA_OK) {
        ca_log("WARN", "abnormal event dropped: type=%s event_wall_ms=%lld",
               ev.event_type, (long long)ev.event_wall_ms);
        return CA_ERR;
    }
    return CA_OK;
}

static int start_human_detect_demo(const AppConfig *cfg, EventQueue *queue)
{
#if CA_ENABLE_LEGACY_ALARM
    pthread_t tid_alarm_server;
    pthread_t tid_alarm_process;
    pthread_attr_t attr;
#endif
    char app_name_with_chan[32];
    char *hf_argv[4];
    int hf_argc = 3;
    int ret;

    if (g_human_inited) {
        return CA_OK;
    }

    g_human_event_queue = queue;
    alarm_set_http_target(cfg->human_alarm_ip, cfg->human_alarm_port);
    hikflow_demo_set_infer_interval_ms(cfg->infer_interval_seconds * 1000);
    hikflow_demo_set_runtime_options(cfg->hikflow_model_path, cfg->abnormal_classes);
    ca_debug_log(1, "human detect config: alarm=%s:%d infer_interval_ms=%d app_chan=%d",
                 cfg->human_alarm_ip, cfg->human_alarm_port,
                 cfg->infer_interval_seconds * 1000, cfg->app_chan);
    ca_debug_log(1, "human detect model/class: model=%s abnormal_classes=%s",
                 cfg->hikflow_model_path[0] ? cfg->hikflow_model_path : "(hikflow_config.json)",
                 cfg->abnormal_classes[0] ? cfg->abnormal_classes : "(sel_class fallback)");
    snprintf(app_name_with_chan, sizeof(app_name_with_chan), "cameraAbnormal");

    if (cfg->app_chan >= 0) {
        g_human_app_chan = cfg->app_chan;
        snprintf(app_name_with_chan, sizeof(app_name_with_chan), "cameraAbnormal#%d", g_human_app_chan);
        hf_argc = 4;
    }

    hf_argv[0] = (char *)"camera_abnormal_app";
    hf_argv[1] = (char *)"./";
    hf_argv[2] = (char *)"CAMERA";
    hf_argv[3] = NULL;
    if (hf_argc == 4) {
        static char chan_arg[16];
        snprintf(chan_arg, sizeof(chan_arg), "%d", g_human_app_chan);
        hf_argv[3] = chan_arg;
    }

    ret = hikflow_demo_param_check(hf_argc, hf_argv);
    if (ret != 0) {
        ca_log("ERR", "hikflow_demo_param_check failed: %d", ret);
        return CA_ERR;
    }
    ca_debug_log(1, "hikflow_demo_param_check ok: argc=%d mode=%s chan_arg=%s",
                 hf_argc, hf_argv[2], hf_argc == 4 ? hf_argv[3] : "(auto)");

#if CA_ENABLE_LEGACY_ALARM
    ret = audio_play_init();
    if (ret != 0) {
        ca_log("WARN", "audio_play_init failed or test.wav already exists: 0x%x", ret);
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid_alarm_server, &attr, (void *(*)(void *))alarm_server_task, NULL) != 0 ||
        pthread_create(&tid_alarm_process, &attr, alarm_process, NULL) != 0) {
        pthread_attr_destroy(&attr);
        ca_log("ERR", "human alarm pthread_create failed");
        return CA_ERR;
    }
    pthread_attr_destroy(&attr);
    ca_debug_log(1, "legacy alarm threads started: alarm_server_task alarm_process");
#endif

    ret = target_detect_opdevsdk_init(app_name_with_chan);
    if (ret != 0) {
        ca_log("ERR", "target_detect_opdevsdk_init failed: 0x%x", ret);
        return CA_ERR;
    }

    ret = hikflow_demo_init(hf_argc, hf_argv);
    if (ret != 0) {
        ca_log("ERR", "hikflow_demo_init failed: 0x%x", ret);
        return CA_ERR;
    }

    ret = multi_pdc_config_init();
    if (ret != 0) {
        ca_log("ERR", "multi_pdc_config_init failed: 0x%x", ret);
        return CA_ERR;
    }

    g_human_inited = 1;
    ca_log("INFO", "human_detect_demo_v2 BSC/VIN + HIKFlow started");
    return CA_OK;
}

void *infer_thread(void *arg)
{
    InferContext *ctx = (InferContext *)arg;
    int64_t last_event_ms = 0;

    /*
     * The real human_detect_demo_v2 detection stack is started here. It runs
     * its own internal capture/inference threads and calls event_bridge when
     * hikflow_demo_proc_alarm() confirms an alarm.
     */
    ca_log("INFO", "infer thread started; infer_interval_seconds=%d test_event_interval_seconds=%d",
           ctx->cfg->infer_interval_seconds, ctx->cfg->test_event_interval_seconds);
    if (CA_ENABLE_INFER && start_human_detect_demo(ctx->cfg, ctx->event_queue) != CA_OK) {
        ca_log("ERR", "human detect stack failed to start");
        *ctx->running = 0;
        return NULL;
    }
    if (!CA_ENABLE_INFER && ctx->cfg->test_event_interval_seconds <= 0)
        ca_log("WARN", "inference disabled: set test_event_interval_seconds > 0 to trigger clips");
    while (*ctx->running) {
        if (ctx->event_queue && ctx->cfg->test_event_interval_seconds > 0) {
            int64_t now = ca_now_ms();
            if (last_event_ms == 0 ||
                now - last_event_ms >= (int64_t)ctx->cfg->test_event_interval_seconds * 1000) {
                if (abnormal_event_publish(ctx->event_queue, "test_abnormal", 0.990f, now) == CA_OK) {
                    ca_log("INFO", "test abnormal event published");
                }
                last_event_ms = now;
            }
        }
        ca_sleep_ms(50);
    }
    return NULL;
}
