# 网页实时检测框叠加

在 HEOP 页面 `html/index.html` 的视频画面上叠加实时检测框。数据来自已运行的
HIKFlow 推理结果，不新增监听端口、不引入第三方依赖，也不改动现有检测、切片与上传链路。

## 链路

```text
VIN 原始帧 (NV21)
  └─ mscale ──► net_img ──► HIKFlow NPU (Model_P_NPU0.bin)
                              └─ hikflow_proc_alg_from_cam()  类别 + 归一化 bbox
                                    └─ hikflow_demo_publish_overlay()   ← 本项目新增
                                          └─ ca_detect_overlay_publish()  快照(带锁)
                                                └─ ISAPI GET .../cameraAbnormal/detections
                                                      └─ main.js: DetectOverlay 轮询 250ms
                                                            └─ #detectCanvas (2D 叠加层)
```

关键几何约定：`SuperRender_10.js` 用全屏四边形把视频**拉伸铺满**画布（无 letterbox），
因此归一化 bbox `[0,1]` 直接映射为 `x * 600, y * 450`，不需要任何比例补偿。
四个点是 `point[0]` 左上、`point[1]` 右上、`point[3]` 左下，取法与
`hikflow_demo_proc_jpegenc()` 完全一致，所以网页上的框与设备自己预览上的框一致。

## 端点

```text
GET /ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/detections?format=json[&chanID=N]
```

```json
{
  "cameraAbnormalDetections": {
    "seq": 1240,
    "ts": 1789620000000,
    "frameW": 1920,
    "frameH": 1080,
    "count": 1,
    "boxes": [
      { "x": 0.3120, "y": 0.2083, "w": 0.1458, "h": 0.4074,
        "cls": 0, "id": 1, "name": "person" }
    ]
  }
}
```

- `seq`：每次发布递增。前端只在新 `seq` 时重绘，用来丢弃乱序响应。
  **`seq` 为 0 表示设备还没发布过结果**，网页此时不画任何框。
- `x/y/w/h`：归一化到 `[0,1]`，坐标系与规则多边形一致。
- `count` 为 0 是正常响应，表示目标离开了，前端据此清空。
- 单次响应上限 `CA_DETECT_OVERLAY_MAX_BOXES`(16) 个框，类别名截断到 15 字符。
  上游数据在进入快照前已被 clamp 到 `[0,1]`，NaN 归 0，避免产生非法 JSON。

## 页面行为

| 行为 | 说明 |
| --- | --- |
| 轮询 | 250ms，`setTimeout` 串行自调度：上一请求结束才排下一次，设备慢时不会堆积请求 |
| 重绘 | 仅当 `seq` 变化时重绘，避免同一份数据反复擦画导致闪烁 |
| 失效清理 | 3 次连续请求失败，或超过 2000ms 没有成功响应，自动清空画布 |
| 开关 | 页面「显示检测框」复选框；关闭立即清空，视频与规则编辑不受影响 |
| 通道切换 | `getParam()` 里调用 `reset()` 清空旧通道的框，轮询继续（URL 每拍按当前通道重建） |
| 独立性 | 叠加层不依赖播放器，播放器初始化失败不影响检测框轮询 |
| 图层 | 新增 `#detectCanvas`，`pointer-events:none`，层级低于规则编辑用的 `#liveviewCanvas` |

## 画面上有两层框，别混淆

| 层 | 画在哪 | 由谁画 | 谁能看见 |
| --- | --- | --- | --- |
| 烧进码流那层 | 视频像素本身 | `hikflow_demo_proc_pos()` → `opdevsdk_pos_procTarget/procText` | 任何看这路码流的客户端（含切片录像、抓包） |
| 网页叠加层 | `#detectCanvas` | `main.js: DetectOverlay` | 只在不依赖播放器的网页上 |

两层文字现在**完全一致**：`<类别名> class:<模型类别号> confidence:<分数>`。

烧进码流那层曾经是 `<类别名> id:<每帧编号>`。那个 `id` 只是本帧过滤后的序号、**每帧重排**，
不代表同一个目标，所以不再显示（API 里仍保留 `id` 字段）。排查时若两层文字不一致，
先 Ctrl+F5 硬刷新，确认网页加载的是新的 `main.js`。

## 模块开关

| Make 参数 | C 宏 | 控制 |
| --- | --- | --- |
| ENABLE_DETECT_OVERLAY | CA_ENABLE_DETECT_OVERLAY | 检测结果快照的发布与上报（默认 1） |

关闭后推理照常运行、告警链路不受影响，只是快照永远为空（端点返回 `seq:0`/`count:0`），
网页不显示任何框。与其它模块一样，切换开关需要重新编译并重启进程。

## 观测

发布路径内建一条 **ERR 级常显日志**，用 `ca_log` 输出到 stderr，**不受 `debug_level`
或 hikflow 日志等级影响**，每秒最多一行，目标数变化时必然输出一行：

```text
[ERR] detect overlay: n=1 frame=1920x1080 ts=1789620000000 box0=(0.3120,0.2083,0.1458,0.4074) cls=0 id=1 name=person
[ERR] detect overlay: n=0 frame=1920x1080 ts=1789620001000
```

`n` 是尺寸上限内实际上报的框数，被截断时会出现 `n=16 truncated`。

## 参数与调优

| 参数 | 位置 | 影响 |
| --- | --- | --- |
| `infer_interval_seconds` | 根目录 `app.conf` | 检测间隔，决定框的刷新率。当前为 1，即框每秒更新一次、存在 ≤1s 量级的滞后（视频是实时的）。要「顺滑跟随」可临时设为 0，此时刷新率受 `hikflow_config.json` 的 `net.fps`(12.5) 约束 |
| `abnormal_classes` | 根目录 `app.conf` | 留空则回退 `hikflow_config.json` 的 `sel_class`（当前为 `-1`，即 6 类全不过滤）。**同时影响告警触发与网页可见框**（两者共用同一份过滤结果） |
| 规则多边形 | 页面「绘制规则」 | 目标框**中心点**必须落在多边形内才会进入快照，与告警口径一致 |
| `POLL_MS` / `STALE_MS` | `APP/html/script/main.js` 的 `DetectOverlay` | 轮询周期与失效阈值 |

> 改根目录 `app.conf` 后需 `make install`（会覆盖 `APP/app.conf`）；只改
> `APP/html/*` 时 `make install` 不会覆盖，但需要重新 `makeapp.sh` 打包。

## 验证

### 本机单元测试

`tests/compat` 只能用于 Windows 本机，禁止进入固件构建。

```sh
gcc -std=gnu99 -Wall -Wextra -Itests/compat -Isrc tests/test_detect_overlay.c -o tests/test_detect_overlay.exe
tests/test_detect_overlay.exe
```

覆盖：初始化前读取/发布、空列表发布、`seq` 单调、日志限流、坐标 clamp、
NaN 拒绝、类别名截断、超长列表截断、帧尺寸归一化。

### 设备端

```sh
# 端点连通性与数据（带页面同款 Cookie/SessionTag）
curl -s "http://127.0.0.1/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/detections?format=json&chanID=1"

# 人走动时 count / boxes[0].x 应随之变化；连续 100 次不应超时
for i in $(seq 100); do curl -s -o /dev/null -w "%{http_code} %{size_download}\n" \
  "http://127.0.0.1/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/detections?format=json"; done
```

浏览器侧（DevTools）：

1. Network 里 `detections` 请求稳定约 4Hz，无 4xx/5xx。
2. 框与人重合；**四角测试**：人站画面左/右/上/下边缘，框不偏移、不镜像。
3. 取消「显示检测框」→ 框立即消失，且规则多边形仍可正常拖拽、保存。
4. 切换通道 → 旧框立即消失、新通道的框随检测出现。
5. 刷新页面 → 自动恢复。

回归检查：告警→切片→`server.py` 收到事件仍正常；`DIAG ring:`、`DIAG memory: rss_kb=`
无异常增长；`makeapp.sh` 只生成一个 `.app`。

## 排障

| 现象 | 先看什么 |
| --- | --- |
| 网页完全没有框 | stderr 上的 `detect overlay:` 日志。`n=0` 说明后端就没出框（不是前端问题）；日志本身不出现说明 `CA_ENABLE_DETECT_OVERLAY=0` 或推理未启动 |
| 日志有 `n>0` 但页面无框 | 浏览器 Network 里看 `detections` 的状态码与响应体；确认「显示检测框」已勾选 |
| 框的位置整体偏移 | 确认没有开启播放器的裁剪/缩放（`SuperRender_10.js` 默认全屏拉伸，坐标是 1:1 的） |
| 框明显跟不上人 | 检测间隔所致，见「参数与调优」的 `infer_interval_seconds` |
| 端点返回 `seq:0` | 推理线程还没发布过结果，或应用刚启动尚未推理 |
| 响应被截断/非法 JSON | 框数上限降到 8 重试；正常异常类过滤下 count 通常 ≤5 |
