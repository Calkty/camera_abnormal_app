import json
import os
import shutil
import subprocess
import time
from pathlib import Path

from fastapi import FastAPI, File, UploadFile
from fastapi.responses import JSONResponse

BASE_DIR = Path(os.environ.get("CAMERA_UPLOAD_DIR", "uploads")).resolve()
BASE_DIR.mkdir(parents=True, exist_ok=True)

app = FastAPI()


def _print_detection(meta: dict, event_dir: Path) -> None:
    record = {
        "message": "收到检测上报",
        "received_at": time.strftime("%Y-%m-%d %H:%M:%S %z"),
        "metadata": meta,
        "saved_dir": str(event_dir),
    }
    if meta.get("event_type") == "human_abnormal":
        record["confidence_note"] = "当前摄像机上报的 confidence 固定为 1.0，并非真实模型分数"
    print("[DETECTION] " + json.dumps(record, ensure_ascii=False), flush=True)


def _codec_from_meta(meta: dict, video_name: str) -> str:
    codec = str(meta.get("codec", "")).lower()
    if codec in {"h264", "h265"}:
        return codec
    if video_name.endswith(".h265"):
        return "h265"
    return "h264"


def _convert_to_mp4(src: Path, dst: Path, codec: str, fps: int = 25) -> bool:
    input_format = "hevc" if codec == "h265" else "h264"
    cmd = [
        "ffmpeg",
        "-y",
        "-fflags",
        "+genpts",
        "-r",
        str(fps),
        "-f",
        input_format,
        "-i",
        str(src),
        "-c",
        "copy",
        str(dst),
    ]
    try:
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except Exception as exc:
        print(f"ffmpeg convert failed: {exc}", flush=True)
        return False


@app.post("/api/upload")
async def upload(metadata: UploadFile = File(...), video: UploadFile = File(...)):
    now_ms = int(time.time() * 1000)
    event_dir = BASE_DIR / str(now_ms)
    event_dir.mkdir(parents=True, exist_ok=True)

    meta_path = event_dir / "event.json"
    video_ext = ".h265" if video.filename and video.filename.endswith(".h265") else ".h264"
    raw_path = event_dir / f"event{video_ext}"
    mp4_path = event_dir / "event.mp4"

    with meta_path.open("wb") as f:
        shutil.copyfileobj(metadata.file, f)
    with raw_path.open("wb") as f:
        shutil.copyfileobj(video.file, f)

    try:
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
    except Exception as exc:
        print(f"[DETECTION] invalid metadata: {exc}", flush=True)
        meta = {}

    if not isinstance(meta, dict):
        print("[DETECTION] invalid metadata: expected a JSON object", flush=True)
        meta = {}
    _print_detection(meta, event_dir)

    codec = _codec_from_meta(meta, raw_path.name)
    fps = int(meta.get("fps", 25)) if isinstance(meta, dict) else 25
    converted = _convert_to_mp4(raw_path, mp4_path, codec, fps=fps)
    print("[UPLOAD] " + json.dumps({
        "camera_id": meta.get("camera_id"),
        "event_wall_ms": meta.get("event_wall_ms"),
        "raw_path": str(raw_path),
        "converted": converted,
        "mp4_path": str(mp4_path) if converted else None,
    }, ensure_ascii=False), flush=True)
    response = {
        "ok": True,
        "confirmed": True,
        "raw_path": str(raw_path),
        "metadata_path": str(meta_path),
        "mp4_path": str(mp4_path) if converted else None,
        "converted": converted,
    }
    return JSONResponse(response)
