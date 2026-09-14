# camera_abnormal_app 服务端部署文档

本文档说明 `camera_abnormal_app/server` 如何部署到球机外部服务器上。服务端的职责是接收球机端上传的异常视频片段和 JSON 元数据，并把 `.h264/.h265` 原始码流转成 `event.mp4`。

## 一、服务端功能

项目目录：

```text
C:\Users\Administrator\Documents\Codex\2026-08-08\zhe\outputs\camera_abnormal_app\server
```

关键文件：

```text
server.py            FastAPI 服务端程序
requirements.txt    Python 依赖
Dockerfile          Linux Docker 镜像构建文件
docker-compose.yml  Docker Compose 启动文件
uploads/            默认上传保存目录
```

服务端接口：

```text
POST /api/upload
```

球机端会通过 `multipart/form-data` 上传两个字段：

```text
metadata  event.json
video     event.h264 或 event.h265
```

服务端收到后会保存为：

```text
uploads/<服务器时间戳>/
  event.json
  event.h264 或 event.h265
  event.mp4
```

`event.mp4` 由 `ffmpeg` 从原始 H264/H265 码流转换得到。

## 二、球机端需要配置的地址

球机端配置文件：

```text
C:\Users\Administrator\Documents\Codex\2026-08-08\zhe\outputs\camera_abnormal_app\app.conf
```

把里面的 `upload_url` 改成外部服务器地址：

```ini
upload_url=http://服务器IP:8080/api/upload
```

例如外部服务器 IP 是 `192.168.1.100`：

```ini
upload_url=http://192.168.1.100:8080/api/upload
```

注意：当前球机端上传代码只支持普通 HTTP，不支持 HTTPS；URL 建议使用 IPv4 地址。

## 三、Linux 服务器部署方式一：Docker Compose 推荐

适合已经安装 Docker 的 Linux 服务器。

### 1. 上传 server 目录

把整个 `server` 目录复制到 Linux 服务器，例如：

```bash
/opt/camera_abnormal_app/server
```

进入目录：

```bash
cd /opt/camera_abnormal_app/server
```

### 2. 确认文件

```bash
ls
```

应能看到：

```text
Dockerfile
docker-compose.yml
requirements.txt
server.py
```

### 3. 启动服务

```bash
docker compose up -d --build
```

如果系统使用旧版 compose 命令：

```bash
docker-compose up -d --build
```

### 4. 查看状态

```bash
docker compose ps
docker compose logs -f
```

正常情况下服务监听：

```text
0.0.0.0:8080
```

上传文件会保存到 Linux 当前目录下的：

```text
./uploads/
```

### 5. 放行防火墙

Ubuntu/Debian 如果启用了 `ufw`：

```bash
sudo ufw allow 8080/tcp
sudo ufw status
```

CentOS/RHEL 如果启用了 `firewalld`：

```bash
sudo firewall-cmd --add-port=8080/tcp --permanent
sudo firewall-cmd --reload
```

### 6. 停止服务

```bash
docker compose down
```

## 四、Linux 服务器部署方式二：原生 Python

适合不想用 Docker，或者 Docker 环境不可用的情况。

### 1. 安装系统依赖

Ubuntu/Debian：

```bash
sudo apt-get update
sudo apt-get install -y python3 python3-venv python3-pip ffmpeg
```

CentOS/RHEL：

```bash
sudo yum install -y python3 python3-pip ffmpeg
```

如果 `yum` 源没有 `ffmpeg`，需要先配置 EPEL/RPM Fusion 或使用服务器已有的 FFmpeg 安装方式。

### 2. 创建虚拟环境

```bash
cd /opt/camera_abnormal_app/server
python3 -m venv .venv
source .venv/bin/activate
```

### 3. 安装 Python 依赖

```bash
pip install -r requirements.txt
```

### 4. 启动服务

```bash
export CAMERA_UPLOAD_DIR=uploads
uvicorn server:app --host 0.0.0.0 --port 8080
```

后台运行可用：

```bash
nohup uvicorn server:app --host 0.0.0.0 --port 8080 > server.log 2>&1 &
```

### 5. systemd 常驻服务可选

新建：

```bash
sudo nano /etc/systemd/system/camera-upload-server.service
```

写入：

```ini
[Unit]
Description=Camera Abnormal Upload Server
After=network.target

[Service]
Type=simple
WorkingDirectory=/opt/camera_abnormal_app/server
Environment=CAMERA_UPLOAD_DIR=/opt/camera_abnormal_app/server/uploads
ExecStart=/opt/camera_abnormal_app/server/.venv/bin/uvicorn server:app --host 0.0.0.0 --port 8080
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```

启动：

```bash
sudo systemctl daemon-reload
sudo systemctl enable camera-upload-server
sudo systemctl start camera-upload-server
sudo systemctl status camera-upload-server
```

查看日志：

```bash
journalctl -u camera-upload-server -f
```

## 五、Windows 服务器部署方式：原生 Python

适合不安装 WSL、不使用 Docker Desktop 的 Windows 主机。

### 1. 安装 Python

安装 Python 3.10 或更高版本。安装时建议勾选：

```text
Add python.exe to PATH
```

安装后打开 PowerShell，检查：

```powershell
python --version
pip --version
```

### 2. 安装 FFmpeg

方式一：如果使用 winget：

```powershell
winget install Gyan.FFmpeg
```

方式二：手动下载 FFmpeg，把 `ffmpeg.exe` 所在目录加入系统 `PATH`。

检查：

```powershell
ffmpeg -version
```

### 3. 进入 server 目录

```powershell
cd "C:\Users\Administrator\Documents\Codex\2026-08-08\zhe\outputs\camera_abnormal_app\server"
```

### 4. 创建虚拟环境

```powershell
python -m venv .venv
```

激活虚拟环境：

```powershell
.\.venv\Scripts\Activate.ps1
```

如果 PowerShell 阻止脚本执行，可以临时允许当前窗口运行：

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\.venv\Scripts\Activate.ps1
```

### 5. 安装依赖

```powershell
pip install -r requirements.txt
```

### 6. 设置上传目录

PowerShell 写法：

```powershell
$env:CAMERA_UPLOAD_DIR="uploads"
```

CMD 写法：

```cmd
set CAMERA_UPLOAD_DIR=uploads
```

注意：`$env:CAMERA_UPLOAD_DIR="uploads"` 只能在 PowerShell 里执行。如果在 CMD 或 Git Bash 里执行，会报“文件名、目录名或卷标语法不正确”。

### 7. 启动服务

PowerShell：

```powershell
uvicorn server:app --host 0.0.0.0 --port 8080
```

启动后不要关闭这个窗口。看到类似下面内容说明服务已启动：

```text
Uvicorn running on http://0.0.0.0:8080
```

### 8. 放行 Windows 防火墙

以管理员身份打开 PowerShell：

```powershell
New-NetFirewallRule -DisplayName "Camera Upload Server 8080" -Direction Inbound -Protocol TCP -LocalPort 8080 -Action Allow
```

### 9. 查看本机 IP

```powershell
ipconfig
```

找到和球机同网段的 IPv4 地址，例如：

```text
192.168.1.100
```

然后球机端 `app.conf` 配置为：

```ini
upload_url=http://192.168.1.100:8080/api/upload
```

## 六、连通性测试

### 1. 从服务器本机测试服务是否启动

Linux：

```bash
curl http://127.0.0.1:8080/docs
```

Windows PowerShell：

```powershell
curl.exe http://127.0.0.1:8080/docs
```

能返回 HTML 内容，说明 FastAPI 服务可访问。

### 2. 从球机所在网络测试

在和球机同网段的电脑上测试：

```bash
curl http://服务器IP:8080/docs
```

如果访问不到，优先检查：

```text
服务器 IP 是否正确
防火墙是否放行 8080
服务器和球机是否在同一网络或路由可达
服务是否用 --host 0.0.0.0 启动
```

### 3. 手动模拟上传

准备两个测试文件：

```text
event.json
event.h264 或 event.h265
```

Linux：

```bash
curl -F "metadata=@event.json" -F "video=@event.h264" http://127.0.0.1:8080/api/upload
```

Windows PowerShell：

```powershell
curl.exe -F "metadata=@event.json" -F "video=@event.h264" http://127.0.0.1:8080/api/upload
```

正常返回类似：

```json
{
  "ok": true,
  "confirmed": true,
  "raw_path": "...event.h264",
  "metadata_path": "...event.json",
  "mp4_path": "...event.mp4",
  "converted": true
}
```

如果 `"converted": false`，说明文件保存成功，但 FFmpeg 转 MP4 失败。常见原因是：

```text
ffmpeg 没安装或不在 PATH
上传的原始码流不是有效 H264/H265
码流缺少 SPS/PPS/VPS 或不是从关键帧开始
metadata 里的 codec/fps 信息不正确
```

## 七、常见问题

### 1. Docker 提示 Cannot connect to the Docker daemon

说明当前环境没有运行 Docker 服务，或者是在普通容器内部执行了 `docker compose`。处理方式：

```text
在真正的 Linux 宿主机上安装并启动 Docker
或者不用 Docker，改用本文的原生 Python 部署方式
```

### 2. Windows 下设置 CAMERA_UPLOAD_DIR 报语法错误

要区分 shell：

PowerShell：

```powershell
$env:CAMERA_UPLOAD_DIR="uploads"
```

CMD：

```cmd
set CAMERA_UPLOAD_DIR=uploads
```

Git Bash：

```bash
export CAMERA_UPLOAD_DIR=uploads
```

### 3. 球机上传失败

检查球机端 `app.conf`：

```ini
upload_url=http://服务器IP:8080/api/upload
upload_retry=3
upload_retry_interval_ms=3000
```

再检查服务器：

```text
服务是否启动
8080 端口是否开放
球机是否能 ping 通服务器
服务器 uploads 目录是否有写权限
```

### 4. 收到 h264/h265 但没有 mp4

优先检查 FFmpeg：

```bash
ffmpeg -version
```

然后检查原始码流是否从关键帧开始，是否带 SPS/PPS/VPS。项目第二阶段已经考虑“关键帧对齐”和“SPS/PPS/VPS 注入”，这两个点会直接影响服务器能否稳定转 MP4。

## 八、推荐部署组合

正式部署建议：

```text
Linux 服务器：Docker Compose 或 systemd 原生 Python
Windows 服务器：原生 Python + FFmpeg + Windows 防火墙放行 8080
球机端：upload_url 指向外部服务器 /api/upload
```

如果现场环境允许，优先选 Linux + Docker Compose，部署和迁移最省心；如果现场只有 Windows 主机，就用 Windows 原生 Python 方式即可。
