# RING 内存修复版 ring-fix-v2

本包已合并修复，无需覆盖其他文件。默认开启真实缓存；启动脚本未设置 CA_DIAG_RING_DISCARD=1。
这是完整源码项目，保留模型、运行库、网页和打包资源。需要在原 ARM SDK 环境编译。

## 默认选择

项目根目录 app.conf（make install 会把它复制到 APP/app.conf）：

```ini
ring_seconds=20
ring_max_mb=16
```

ring_max_mb 的单位是 MiB，即 1024*1024 字节。它约束环形缓存持有的视频 payload，不是整个进程 RSS。
默认选择 20 秒是为了给当前事件前 5 秒、后 10 秒的录像窗口及处理延迟留余量。不能保证排队很久的事件仍保留完整视频。
根据实测，20 秒约 14 MiB，叠加约 52 MiB 的基础 RSS，预期约 67～72 MiB，仅作为估算。malloc 分配器、SDK、码率变化会影响实际 RSS。
达到 16 MiB 上限时，内存保护优先，提前删除最旧视频，因此保留时长可能不足 20 秒。

## 已修复

- 每次入队前按 CLOCK_MONOTONIC 时间清理过期数据，系统校时不会干扰淘汰。原 recv_ms 保留用于事件匹配。
- 主线程每约 500 ms 清理一次，RTSP 中断后缓存也会到期释放（调度/锁竞争可能使执行略有延迟）。
- 申请新 payload 前先释放旧数据，使缓存持有的 payload 总量不超过 ring_max_mb。
- 固定 4096 个元数据槽，槽位用满也是淘汰条件，不再用 fps*12 推算保留时间。
- 单个 NAL 超过整个缓存预算时丢弃，记录 oversize。
- malloc 失败不会产生虚假的有效条目；记录 malloc_fail。失败前为腾空间淘汰的旧数据不会恢复。
- 辅助 VPS/SPS/PPS 副本每份最多 64 KiB；另有元数据、分配器开销，不计入 payload 上限。
- 空缓存快照返回成功和空数组。
- 保留 stderr INFO 诊断，不受 debug_level 或 ISFW 日志等级影响。

本次没有重写裁剪：CLIP 的全量快照与片段副本仍会额外占用内存，不受 ring_max_mb 限制。请先保持 CLIP=0 验证当前修复，不能把环形缓存上限当成裁剪峰值上限。
时间/容量淘汰以 NAL 为单位，不保证最旧条目是可解码关键帧。原裁剪逻辑会向关键帧对齐，可能缩短片段；高码率导致提前淘汰时同样如此。

## 第一轮：推荐设置 20 秒 / 16 MiB

```sh
sh makeapp.sh CC=aarch64-mix210-linux-gcc \
  ENABLE_INFER=1 ENABLE_RING=1 ENABLE_CLIP=0 \
  ENABLE_UPLOAD=0 ENABLE_LEGACY_ALARM=0
```

不要保留上一轮的 `export CA_DIAG_RING_DISCARD=1`；若手动从 shell 启动，请先 `unset CA_DIAG_RING_DISCARD`。
安装 output/ 下新生成的 .app。确认日志：

```text
RING limits: seconds=20 max_bytes=16777216 capacity=4096 discard=0
DIAG build=ring-fix-v2 discard=0 ring_seconds=20
```

保持同一码率、推理间隔及场景，运行至少 15 分钟。成功标准：

1. payload_bytes 和 peak_bytes 始终不超过 16777216。
2. 收流连续且系统时钟未跳变时，span_ms 应在约 20 秒以内；不会再随运行时长增长。
3. 大约 20 秒后 evict_time 持续增长。码率高时可能先出现 evict_bytes，这表示字节限制正在保护内存。
4. RSS 填充缓存后趋于稳定，记录实际峰值；不会因为 free 就必然立即下降（分配器可能保留内存）。
5. 正常情况下 oversize、malloc_fail 为 0。evict_count 若持续增加，说明元数据槽限制也在缩短保留窗口。

## 第二轮：更保守的 10 秒 / 8 MiB

修改项目根目录 app.conf 后，重新用相同命令打包：

```ini
ring_seconds=10
ring_max_mb=8
```

当前码率下预计视频 payload 约 7 MiB，基础 RSS 加缓存约 60 MiB 左右。日志 max_bytes 应为 8388608。
仍保持 CLIP=0。10 秒适合当前缓存稳定性测试，但不推荐直接用于原来的“前5秒、后10秒”裁剪流程：等待后段数据期间及事件排队时，所需数据可能已过期。
配置不会再静默把 10 秒改成其他数值。ring_seconds 允许 1～3600；ring_max_mb 允许 1～64，0、负数、非整数或越界值启动时报错。

## 验证记录

本机通过 C 单元测试直接执行修改后的 ring_buffer.c 和 config.c，使用 Windows 测试兼容头模拟时间与单线程锁；覆盖字节/时间/槽位淘汰、空闲到期、超大包拒绝、分配失败恢复、快照生命周期、系统时间跳变及 10/20 秒各 15 分钟的模拟入流。另做 10000 次持续字节上限压力测试。
main.c 已通过本机 C 编译器的编译检查。
这不是设备实测：没有验证 ARM SDK 全项目链接、真实 pthread 并发、视频解码或实际相机 RSS；这些需通过上述设备测试完成。
tests/compat 仅用于 Windows 单元测试，不在生产 Makefile 的 include 路径中，禁止加入固件构建。
