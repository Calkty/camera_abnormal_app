# 个人模型替换说明（6 分类 v8）

把工程自带的 80 类 COCO demo 模型，换成个人训练的 6 分类模型。

## 现状

| 项 | 值 |
| --- | --- |
| 模型文件 | `vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin` |
| 来源 | `yolov8s/Sample/out/bin/v8_O_H9_NPU0.bin`（拷贝后 MD5 已核对一致，3,935,052 字节） |
| 转换配置 | `yolov8s/yolov8_onnx.json`：`net_name=v8`、输入 640x640 RGB、`weight_path=./Sample/user_data/best.onnx` |
| 量化校准集 | `Sample/ref_image_list.txt`（COCO 参考图） |
| 类别 | 6 类，取自 `best.onnx` 的 ONNX 元数据：`bird_nest / balloon / plastic_bag / smoke / kite / sky_lantern`，`imgsz=[640,640]`、`batch=1` |
| 自定义层 | `custom_layer/` 已整体换成 `Sample/out/tctool/` 的 `v8_custom_0*` 版本 |

配置里的文件名没变：新 bin 直接命名为 `Model_P_NPU0.bin`，所以 `hikflow_config.json` 的
`model_path`、`hikflow_attr.json` 的 `name`、以及 `makeapp.sh` 的 `test -s Model_P_NPU0.bin`
全部无需改动。

## ⚠️ 根因：新模型声明输入是 BGR，运行时喂的却是 NV21

**现象**：模型能加载、启动日志里没有任何报错，但**检测不到任何目标**。

**证据**：BIN 尾部 256 字节的 `dev_info` 块（`hikflow_proc_update_model_info()` 解析它），
用 `python check_model_bin.py` 即可读出：

| 字段 | 旧模型（能检出） | 新模型（检不出） |
| --- | --- | --- |
| `plat_type` | 7 | 7 |
| `dtype` | 1 (U08) | 1 (U08) |
| **`dformat`** | **2 = YVU420 / NV21** | **1 = BGR (packed)** |
| 输入 | 1×3×640×640 | 1×3×640×640 |

`hikflow_proc_update_net_info()` 把 `dformat` 映射成 `src_format`：

```
1 -> OPDEVSDK_HKA_BGR
2 -> OPDEVSDK_HKA_YVU420   (NV21)
4 -> OPDEVSDK_HKA_YUV420   (NV12)
```

而 `hikflow_proc_net()` 把 mscale 产出的 **NV21** 帧直接当输入：

```c
hkann_in.in_blob[0].src_format = ...;                  // 由 dformat 决定 -> 被声明成 BGR
hkann_in.in_blob[0].src_blob.data = pfrm;              // 实际是 NV21
hkann_in.in_blob[0].src_blob.shape = {1, 3, 640, 640}; // 按 BGR 算需要 1,228,800 字节
```

NV21 的 640×640 帧只有 `640*640*3/2 = 614,400` 字节。SDK 被告知"这是 BGR"，于是按 BGR
三平面解释这 614 KB 数据 → NPU 拿到的是垃圾张量 → 自定义层的 NMS 在 `conf_thresh=0.5`
下输出空 → **应用侧没有任何错误可报**。

**为什么应用不会报错**：`hikflow_proc_alg_from_cam()` 只检查类别和规则多边形；全仓库
`box_info->score` 只出现在两句 printf 里（`hikflow_proc_priv.c:678`、`:924`），从不参与判断。
所以"输入是垃圾"和"画面里没目标"在应用看来完全一样。

### 根因就是配置项 `input_type` 写成了 1

`dformat` 由转换配置 `yolov8_onnx.json` 的 `input_type` 决定。官方《HIKFlow 使用指南》
表 4-10 第 8 项：

> **`input_type`**：**推理计算时输入图像类型，必须与推理库使用的图像格式保持一致**
> `0` = feature map 输入；`1` = BGR 图像或灰度图输入；`2` = NV21 图像输入；`4` = NV12 图像输入

该指南第 24 页补充：

> `input_type` 设置为 1，表示**推理端**的输入图片为 BGR 格式；设置为 2，表示推理端的输入图片为 NV21 格式

本工程推理端**只能是 NV21** —— `hikflow_proc_net()` 把 mscale 产出的 YUV420SP 帧直接交给 SDK
（`src_blob.data = net_frame.yuvFrame.pVirAddr[0]`），所以正确值是 **`2`**，而配置里是 **`1`**。

**这不是移植时改错的**：转换工程的模板默认就是 `1`（`yolov8_onnx.json.orig` 里同样是 1，
当时只改了 `net_name` 与 `weight_path`）。demo 模型之所以记录 2，是因为海康出包时用的是 `input_type: 2`。

### 现场证据（2026-09-17 设备日志，已复核）

`F:\Download` 里 117 个 `192.168.1.64_*_cameraAbnormal_log.tar.gz` 排出的时间线：

| 时间段 | `detectnum` 最大值 | 设备上跑的模型 |
| --- | --- | --- |
| 9/11 ~ 9/17 **11:51** | **1 ~ 2**（几乎每个包都有） | 旧 COCO demo 模型，**能检出** |
| 9/17 **12:55** 起 | **0**（无一例外） | 个人 6 类模型，**检不出** |

`detect overlay:` 这行日志（本次新增）只出现在 12:55 之后的包里，也就是新代码首次上机的时间点。

**对照实验**——同一份程序、同一条喂图路径、同样 `1×3×640×640`：

| 模型 | 大小 | `dformat` | 实测 |
| --- | --- | --- | --- |
| `APP/Model_P_NPU0.bin`（旧 demo，COCO） | 12,276,663 | **2 (NV21)** | 能检出 person |
| `vendor/.../bin/Model_P_NPU0.bin`（个人 6 类） | 3,935,052 | **1 (BGR)** | 恒为 0 |

模型加载全程无报错，`Custom_Layer_*` 也没有失败 —— 这正符合"声明格式错、像素被误读"的表现：
不崩、不报错、就是永远没有框。

### 打包防呆：`makeapp.sh` 现在会拒绝 `dformat≠2`

`makeapp.sh` 在 `test -s Model_P_NPU0.bin` 之后新增一段检查，直接读
**`APP/Model_P_NPU0.bin` 尾部 `dev_info` 偏移 45** 的 int32：

- `2` → 打印 `model input format: 2 (NV21) ok`，继续打包
- `1` / `4` → 报错并 `exit 1`，提示重跑转换
- 读不出 → 报错并 `exit 1`

这样"改了配置但忘了重跑转换"不会再静默通过，省掉一轮刷机。
（脚本用纯 POSIX `wc`/`dd`/`od` 实现，容器里无需 python。）

### 容器内操作步骤（2026-09-17 已实测跑通）

容器 `heop-dev` 里的 `/home/hikcode/work/` 是**独立副本**：`camera_abnormal_app` 由 PC 同步进来，
但 `yolov8s` **只在容器内有**。**所以转换配置必须在容器里改，改 PC 那份不生效。**

非交互式 `docker exec` 没有开发环境（环境来自 `/etc/heop_devel_kit.rc/`，只有交互式 shell 会自动加载）。
按顺序 source 四个 rc 文件即可：

```sh
for f in 001_toolchain.rc academy.rc app.rc dsp.rc; do . /etc/heop_devel_kit.rc/$f; done
```

顺序不能乱：`app.rc` 依赖 `001_toolchain.rc` 设的 `CROSS_COMPILE_LIBPATH`。之后 `model_trans.sh`
（academy）与 `pack.sh`（`/heop/.tool/app/`）才在 PATH 上，`hikdsl` 才可导入。

**转换**（约 2 分钟）：

```sh
cd ~/work/yolov8s && ./run.sh ./yolov8_onnx.json
```

**构建 + 打包**：

```sh
cd ~/work/camera_abnormal_app
sh makeapp.sh CC=aarch64-mix210-linux-gcc
```

⚠️ **`CC=` 不能省**：漏掉会用宿主 `cc`，在 `audioplay.c:231` 的 `fread` 上被
`-Werror=unused-result` 打死（`makeapp.sh` 会把参数透传给 `make`）。

**本次实测结果**：

| 环节 | 修复前 | 修复后 |
| --- | --- | --- |
| `yolov8_onnx.json` → `input_type` | 1 | **2** |
| `Sample/out/hdnc/v8_convert_bin.json` → `format` | 1 | **2** |
| `Sample/out/bin/v8_O_H9_NPU0.bin` → `dformat` | 1 | **2** |
| BIN 大小 | 3,935,052 | 3,927,779 |

构建日志里会出现 `model input format: 2 (NV21) ok`（防呆放行），随后 `Pack Success`。

顺带确认：重新生成的 `tctool/*` 与旧的差异**只是变量名**（`model_22_Concat_4_output_0_1_transpose`
→ `..._0_3_transpose`），blob 索引 `ld->input_blobs[1]` 与参数顺序都没变。也就是说
**输入格式的修复本身只改 `dev_info`，自定义层不是必需的**；但既然模型重生成过，一起换更稳。


### 三种修法

**修法 A（推荐，源头修）**：把转换配置 `yolov8s/yolov8_onnx.json` 的
`"input_type": 1` 改成 `"input_type": 2`，重新跑 `run.sh` 转换，再按「改动清单」
替换 bin + `out/tctool/*`。

- 该值**已经改好**（`yolov8s/yolov8_onnx.json` 现在是 `"input_type": 2`），只差重跑转换
- 重转后先复核：`python check_model_bin.py <新 bin>` 应输出 `OK: dformat=2 (NV21)`
- 好处：声明格式 == 量化输入格式 == 运行时喂的格式，三方自洽；**应用代码零改动**

**修法 B（应急，应用侧）**：`hikflow_proc_update_net_info()` 里不再信任 BIN 的 `dformat`，
因为 mscale 永远产出 NV21：

```c
/* mscale always hands us YUV420SP, so the declared source format must be NV21
   regardless of how the model was calibrated. */
param_info_net->in_blob_param[0].src_format = OPDEVSDK_HKA_YVU420;
```
- 代价：改一行代码；将来若真要接 NV12 的模型需再调整

**修法 C（只用于验证诊断，1 个字节）**：把 BIN 的 `dformat` 从 1 改成 2。
**仅作一次性诊断实验，不要当长期方案** —— 手工改过的 BIN 与转换产物不一致，
以后没人说得清它为什么不一样。

诊断件已生成在本机临时目录（只改了尾部 `dformat` 那一个字节，其余 3,935,051 字节与转换产物逐字节相同）：

```sh
# 1) 换上诊断件（先备份，随时可回滚）
cp vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin \
   /tmp/Model_P_NPU0.bin.from-conversion
cp "$TEMP/Model_P_NPU0_dformat2.bin" \
   vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin

# 2) 复验：应输出 PASS
python check_model_bin.py vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin

# 3) 编译、打包、上设备。若开始出现检测框 -> 根因确认，转向修法 A 重新转换
#    （直接替换 APP/Model_P_NPU0.bin 也可以，不必重编）

# 4) 回滚
git checkout -- vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin
```

### 顺带发现的两处配置偏差（同一份《HIKFlow 使用指南》）

这两处**不会**导致"检不到目标"，但会影响量化效果。既然要重转，建议一并处理。

**① 量化图用的是通用 COCO 图**

指南第 22 页：

> 数量小于 100 张，则全部读取用于量化。**这里建议用户手动挑选图片，保证量化效果。**

第 25 页说明 `input_info.source`（即 `ref_image_list.txt`）就是**量化（调优）图像列表**。
现在 `Sample/ref_img_dir/` 里全是 COCO 的"人/车/街道"图，而目标是天空场景的
`bird_nest / balloon / plastic_bag / smoke / kite / sky_lantern` —— 分布完全不对应。
建议换成本场景实拍图（含 6 类目标、白天/夜晚/不同光照）。

**② 没有配置 `resize_image_w` / `resize_image_h`**

指南第 25 页：

> `resize_image_w` / `resize_image_h`：量化图片不失真缩放宽/高，**仅在模型转换量化时生效，
> 推理库推理时该参数不生效**。
> 1. …**保持图片的宽高比**…剩余部分填边补充
> 2. **如果不配置，那么就会将原图强制缩放至模型输入宽度和高度，导致图片失真，对量化效果有所影响。**
>    为了达到最好的量化效果，建议正确填写。

注意这两个参数**只影响转换时的量化**，不影响推理 —— 所以运行时直接拉伸是设计如此
（`hikflow_proc_net()` 里 `im_info[2] = 1.0` 的注释也印证了这一点），**不必去改推理代码**。


## 为什么必须连自定义层一起换

转换工程的 `net_name` 决定了模型内部自定义层的 `type` 字符串，而 `custom_callback.c`
是**按字符串分发**的：

```
旧（net_name=yolov8）  yolov8_custom_0 / yolov8_custom_0_sub_0 / yolov8_custom_0_sub_1
新（net_name=v8）      v8_custom_0     / v8_custom_0_sub_0     / v8_custom_0_sub_1
```

名字对不上，`Custom_Layer_GetModelMemsize()` 会走到 `else` 分支返回 `-1`，模型加载失败
（设备日志出现 `Custom_Layer_GetModelMemsize error!`）。

好消息：新旧 `custom_callback.h` 的 5 个 `Custom_Layer_*` 函数签名完全一致，所以
`hikflow_proc_priv.c` 等调用方**一行都不用改**。

## 改动清单

| 文件 | 动作 |
| --- | --- |
| `.../hikflow/bin/Model_P_NPU0.bin` | 换成新的 `v8_O_H9_NPU0.bin` |
| `.../hikflow/custom_layer/custom_callback.c/.h` | 换成 tctool 版本（分发 `v8_custom_0*`） |
| `.../hikflow/custom_layer/custom_v8_custom_0_layer.c/.h` | 新增 |
| `.../hikflow/custom_layer/custom_v8_custom_0_sub_0_layer.c/.h` | 新增 |
| `.../hikflow/custom_layer/custom_v8_custom_0_sub_1_layer.c/.h` | 新增 |
| `.../hikflow/custom_layer/v8_custom_0.h`、`v8_custom_0_forward.c`、`v8_custom_0_reshape.c` | 新增 |
| `.../hikflow/custom_layer/v8_custom_0_sub_0.h`、`_sub_0_forward.c`、`_sub_0_reshape.c` | 新增 |
| `.../hikflow/custom_layer/v8_custom_0_sub_1.h`、`_sub_1_forward.c`、`_sub_1_reshape.c` | 新增 |
| `.../hikflow/custom_layer/opc_runtime_arm.c/.h`、`dsl_*.h`、`opc_*.h` | 换成 tctool 版本 |
| `.../hikflow/custom_layer/custom_yolov8_*`、`yolov8_*` | 删除 |
| `.../hikflow/custom_layer/*.o` | 删除（构建产物，此前被误提交进仓库，建议一并提交删除） |
| `Makefile` 的 `HUMAN_SRCS` | 9 个文件名 `custom_yolov8_*`→`custom_v8_*`、`yolov8_*`→`v8_*`（只改文件名，逻辑零改动） |
| `hikflow_attr.json`（vendor + `APP/` 两份） | 类别表由 80 条 COCO 改为 6 条，见下 |

输入尺寸、`fps`、`vb_cnt` 均由 BIN 尾部的 `dev_info` 自动解析，不需要配置。

## 6 个类别名（已确认）

类别名直接取自 `best.onnx` 的 ONNX 元数据（在文件尾部搜索 `names` 即可看到）：

```
names: {0: 'bird_nest', 1: 'balloon', 2: 'plastic_bag', 3: 'smoke', 4: 'kite', 5: 'sky_lantern'}
imgsz: [640, 640]
batch: 1
```

`hikflow_attr.json` 已按此写好，vendor 与 `APP/` 两份已同步：

| index | name |
| --- | --- |
| 0 | bird_nest |
| 1 | balloon |
| 2 | plastic_bag |
| 3 | smoke |
| 4 | kite |
| 5 | sky_lantern |

`name` 只用于显示（设备预览的文字标注 + 网页上的框标签），想改成中文直接改
`hikflow_attr.json` 即可，不影响检测。**`index` 不要改** —— 它就是
`opdevsdk_hikflow_Process` 返回的类别号，也是 `sel_class` 过滤用的号。

## `sel_class`：已设为 `-1`（6 类全部检测）

`hikflow_config.json` 的 `alarm` 段现在是 `"sel_class": -1`，即**不做类别过滤**，
6 类目标全部参与告警与网页显示。

它同时决定**告警触发**和**网页上显示哪些框**（两者共用同一份过滤结果）。若以后只想保留
其中几类，有两种改法：

```jsonc
"sel_class": 3              // 只保留 3 号类（smoke）
// 或者更细粒度：在 app.conf 里写
abnormal_classes=3,5        // 只要 smoke 和 sky_lantern
```

> `sel_class = -1` 意味着 6 类都可能触发告警。`hikflow_config.json` 的
> `alarm_interval=1` 和 `app.conf` 的 `cooldown_seconds=20` 会限制事件频率；
> 若现场事件过多，优先收窄 `abnormal_classes`。

## 打包一致性：`APP/` 不能是"新模型 + 旧二进制"的混合

换模型要同时替换**两样东西**，它们必须成对：

| 组成 | 位置 | 谁生成 |
| --- | --- | --- |
| 模型 BIN | `vendor/.../hikflow/bin/Model_P_NPU0.bin` → `APP/Model_P_NPU0.bin` | 转换工具 |
| 自定义层分发器 | 编译进 `APP/camera_abnormal_app` | `make` + `make install` |

`APP/` 是**手工暂存目录**：`make install` 只覆盖它列出的文件，其余会留着上次构建的版本。
所以「只往 `APP/` 拷新 BIN」会得到**新模型 + 旧二进制** —— 旧二进制里只有 `yolov8_custom_0`
的分发分支，遇到新 BIN 的 `v8_custom_0` 会返回 -1，**模型根本加载不了**。

⚠️ 这与「dformat 不匹配」是**两种不同的故障**，别混：

| 故障 | 日志表现 | 原因 |
| --- | --- | --- |
| 二进制与模型不成对 | 有 `Custom_Layer_* error!`、`hikflow_demo_init failed` | `APP/` 是混合批次 |
| dformat 不匹配 | **完全没有任何报错** | BIN 声明 BGR，运行时喂 NV21 |

**务必用完整流程出包**，不要手工往 `APP/` 里挑文件：

```sh
make clean && make CC=aarch64-mix210-linux-gcc && make install   # 或直接 ./makeapp.sh
```

**检查手上的 `APP/` 那对是否匹配**：

```powershell
python check_model_bin.py APP/Model_P_NPU0.bin      # 期望 dformat=2
$s = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes('APP/camera_abnormal_app'))
"yolov8_custom_0 = " + ([regex]::Matches($s,'yolov8_custom_0').Count)
"v8_custom_0     = " + ([regex]::Matches($s,'(?<!yolo)v8_custom_0').Count)   # 期望 > 0
```

两者都对了再上设备，否则你看到的失败可能来自"配错了对"，而不是模型本身。

> 2026-09-17 实测：本工作区的 `APP/` 是**混合批次** —— `camera_abnormal_app` 与
> `Model_P_NPU0.bin` 是 9/11 的旧批次（二进制里 `v8_custom_0` 计数为 0、模型仍是 12,276,663
> 字节的旧模型），而 `hikflow_attr.json`/`hikflow_config.json` 已是 9/17 的新配置。
> **这一版 `APP/` 不能打包**，必须先跑完整的 `make install`。

## 验收方案

分 6 关，**每关有明确的通过判据和"失败→原因"映射**。任何一关不过就别往下走，否则后面
的现象会混在一起分不清是模型问题还是集成问题。

### S0 编译前静态校验

```sh
# 模型的输入格式声明必须与运行时喂进去的 NV21 一致，否则会静默检不到目标
python check_model_bin.py vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin
# 也可以拿旧模型做参照物对比：
python check_model_bin.py "git:HEAD:vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin" \
                          vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin
```

| 检查 | 结果 |
| --- | --- |
| **BIN 的 `dformat` 是否为 2 (NV21)** | ❌ **当前是 1 (BGR) —— 见上面的「根因」章节** |
| 新 bin 与转换产物 MD5 | ✅ 一致（`FD57356B0C6295A2C3B03591A7A15F5B`，3,935,052 字节） |
| 全仓库 `yolov8` 残留引用 | ✅ 零残留 |
| `Makefile` 的 `HUMAN_SRCS`(24) 与核心 `SRCS`(12) 文件 | ✅ 全部存在 |
| 新自定义层的本地 `#include` | ✅ 全部可解析 |
| `hikflow_attr.json` / `hikflow_config.json` | ✅ 合法 JSON，6 类、`sel_class=-1` |

> S0 目前**不通过**：模型声明的输入格式与运行时不一致。修好之前 S1～S6 都不必跑 ——
> 这是"能加载、无报错、检不到目标"的唯一已知原因。

### S1 交叉编译

```sh
make clean
make CC=aarch64-mix210-linux-gcc
make install
```

**通过判据**：编译过程出现 `custom_v8_custom_0_layer.c` 等新文件；`APP/Model_P_NPU0.bin`
被更新。

**失败→原因**：
- `No rule to make target ...custom_v8_...` → custom_layer 里的文件没替换成功
- 报 `-Werror` 告警 → 生成代码在新工具链下有告警。**先看告警内容**；确认只是告警而非
  逻辑问题时，才考虑给 `HUMAN_CFLAGS` 加对应的 `-Wno-xxx`（不要直接去掉 `-Werror`）

### S2 模型加载（最容易失败的一关）

启动应用，然后：

```sh
./hf_dbg <pid> hf_stat          # 看 net_input / cmm mem
logread | grep -i custom_layer  # 期望无输出
```

**通过判据**：
1. 日志中**没有** `Custom_Layer_GetModelMemsize error!` / `CreateModel error!` / `GetMemsize error!`
2. `hf_stat` 的 `net_input` 是 **640x640**
3. 进程不退出，`net_suc_times` 持续增长

**失败→原因**：出现 `Custom_Layer_* error!` 就是层名不匹配 —— 检查
`custom_layer/custom_callback.c` 里是不是 `strcmp(ld->type, "v8_custom_0")`，以及
`bin/Model_P_NPU0.bin` 是不是新拷的那个文件。

### S3 推理真的在跑（需要能复现的目标）

6 类里**能现场复现的**只有三个：`balloon`(气球)、`plastic_bag`(塑料袋)、`kite`(风筝)。
`bird_nest`(鸟巢)、`smoke`(烟)、`sky_lantern`(孔明灯) 不好造 —— 验收至少准备
**气球 + 塑料袋**两样。

**通过判据**：

```sh
# 未过滤的原始目标数（syslog）
logread | grep detectnum
# 已上报的框（stderr，ERR 级常显）
#   detect overlay: n=1 ... name=balloon
```

| `detectnum`（未过滤） | `detect overlay: n=`（已过滤） | 结论 |
| --- | --- | --- |
| >0 | >0，且 `name` 正确 | ✅ S3 通过 |
| >0 | 0 | `sel_class=-1` 下不该发生 → 查 `hf_stat` 的 `point_num`（规则多边形是否退化为 0） |
| 0 | 0 | 模型没检出 → 见下 |

**`detectnum=0` 时的排查顺序**（先排除已确认的格式问题，再怀疑精度）：

1. **先查输入格式**：`python check_model_bin.py <bin>` 是否 PASS。`dformat` 不是 2 就一定出不来框，
   与模型精度无关（见上面的「根因」章节）。
2. 再看是否量化掉点、目标太小、光照不合适。可先在 PC 上用转换工程自带的仿真入口确认模型本身
   能出框（`Sample/out/tctool/v8_custom_0.py` 会 `LoadLibrary` `opc/v8_custom_0_pc.so`，
   里面的绝对路径要改成本机路径）。PC 上能出框而设备上不出，才是集成/预处理问题。
3. 若 `dformat` 正确但仍无框，再回到「标定数据与 `conf_thresh`」这条路 ——
   减小 `conf_thresh` 重转一版是最省事的判别实验。

### S4 网页显示（本次需求的最终目标）

打开 `html/index.html`，勾选「显示检测框」。

**通过判据**：
1. 气球/塑料袋在画面里 → 出现绿框，**位置与目标重合**
2. **四角对齐测试**：目标移到画面左/右/上/下边缘，框不偏移、不镜像
3. 标签显示**真实类别名**（`balloon id:1`），不是 `person`、也不是 `class1`
4. 至少验证 **2 个不同类别**，标签各自正确
5. 取消勾选 → 框立即消失，规则多边形仍可正常拖拽/保存
6. 切换通道 → 旧框消失，新框随检测出现

**失败→原因**：
- 标签仍显示 `person` 等旧名 → `APP/hikflow_attr.json` 没生效（它是按 `model_path` 的
  名字匹配 `"name":"Model_P_NPU0.bin"` 的），或只改了 vendor 那份没同步 APP
- 框有系统性偏移 → 检查是否有人改过 `SuperRender_10.js` 的渲染/裁剪

### S5 告警链路回归（`sel_class=-1` 带来的新风险）

现在 6 类都会告警，事件量可能上升。

**通过判据**：
1. 一次气球出现只产生**一次**事件（`alarm_interval=1` + `cooldown_seconds=20` 生效）
2. 切片生成 → 上传到 `server.py` 成功（看 `[UPLOAD]` 与返回 2xx），MP4 转封装正常
3. 连续观察 10 分钟，事件数不失控

**失败→原因**：事件刷屏 → 收窄 `abnormal_classes`（如只留 `3,5`）。

### S6 稳定性

连续运行 30～60 分钟。

**通过判据**：`DIAG memory: rss_kb=` 稳定不涨；`mscale_get_lost_times`、`net_lost_times`、
`jpegenc_lost_times` 不持续增长；`detect overlay: n=` 持续输出；无 `Custom_Layer_*` 错误。

### 验收门槛（全部满足才算通过）

1. S1 交叉编译零告警通过
2. S2 无 `Custom_Layer_*` 错误，`net_input=640x640`
3. S3 至少 2 个类别被真实检出，`detect overlay:` 的 `name` 正确
4. S4 网页上框位置正确、标签是 6 类真名、开关与通道切换正常
5. S5 告警→切片→上传正常且事件频率可控
6. S6 30 分钟无内存增长、无丢失计数增长

## 回滚

模型与自定义层是成对的，回滚要一起回：

```sh
git checkout -- vendor/human_detect_demo_v2/src/hikflow/bin/Model_P_NPU0.bin \
                vendor/human_detect_demo_v2/src/hikflow/custom_layer Makefile
git clean -f vendor/human_detect_demo_v2/src/hikflow/custom_layer
```
