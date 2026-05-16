import argparse
import json
import subprocess
import sys
from pathlib import Path


import shutil


def run_step(command, status, name, cwd=None, shell=False):
    status["status"] = "running"
    status["current_step"] = name
    write_status(status)
    subprocess.run(command, cwd=cwd, shell=shell, check=True)


def write_status(status):
    status_path = Path(status["status_path"])
    status_path.parent.mkdir(parents=True, exist_ok=True)
    status_path.write_text(json.dumps(status, indent=2), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Run full TinyML retrain pipeline")
    parser.add_argument("--raw-input", required=True, help="Source dataset or telemetry export path")
    parser.add_argument("--work-dir", default="artifacts/retrain", help="Directory for generated retrain artifacts")
    parser.add_argument("--model-version", default="tinyml_dht_v2", help="Suggested next model version label")
    parser.add_argument("--baseline-csv", action="store_true", help="Treat raw input as baseline CSV without header: temp,humidity,label")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    work_dir = (repo_root / args.work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    normalized_csv = work_dir / "normalized.csv"
    prepared_dir = work_dir / "dataset"
    model_path = work_dir / "tinyml_model.keras"
    metrics_path = work_dir / "metrics.json"
    header_path = work_dir / "dht_anomaly_model.h"
    status_path = work_dir / "job_status.json"

    python_exe = sys.executable
    raw_input = Path(args.raw_input).resolve()

    status = {
        "status": "queued",
        "current_step": "init",
        "raw_input": str(raw_input),
        "normalized_csv": str(normalized_csv),
        "prepared_dir": str(prepared_dir),
        "model_path": str(model_path),
        "metrics_path": str(metrics_path),
        "header_path": str(header_path),
        "suggested_model_version": args.model_version,
        "status_path": str(status_path),
    }
    write_status(status)

    try:
        if args.baseline_csv:
            status["status"] = "running"
            status["current_step"] = "normalize_baseline_csv"
            write_status(status)
            lines = raw_input.read_text(encoding="utf-8").splitlines()
            normalized_csv.write_text("timestamp,temperature,humidity,anomaly_score,is_anomaly,model_version,device_id\n", encoding="utf-8")
            with normalized_csv.open("a", encoding="utf-8") as handle:
                for index, line in enumerate(lines, start=1):
                    stripped = line.strip()
                    if not stripped:
                        continue
                    temperature, humidity, label = [part.strip() for part in stripped.split(",")[:3]]
                    handle.write(f"baseline_{index},{temperature},{humidity},,{label},baseline_teacher_csv,local_seed\n")
        else:
            run_step(
                [python_exe, str(repo_root / "tools" / "export_data.py"), str(raw_input), str(normalized_csv)],
                status,
                "export_data",
            )

        run_step(
            [python_exe, str(repo_root / "tools" / "prepare_dataset.py"), str(normalized_csv), str(prepared_dir)],
            status,
            "prepare_dataset",
        )
        run_step(
            [python_exe, str(repo_root / "tools" / "train_model.py"), str(prepared_dir), str(model_path)],
            status,
            "train_model",
        )
        run_step(
            [python_exe, str(repo_root / "tools" / "evaluate_model.py"), str(model_path), str(prepared_dir / "test.csv"), str(metrics_path)],
            status,
            "evaluate_model",
        )
        run_step(
            [python_exe, str(repo_root / "tools" / "export_model.py"), str(model_path), str(header_path)],
            status,
            "export_model",
        )

        # Copy the new model to the source directory
        status["status"] = "running"
        status["current_step"] = "copy_model_to_src"
        write_status(status)
        dest_header = repo_root / "src" / "dht_anomaly_model.h"
        shutil.copy2(header_path, dest_header)

        # Automatically build the PlatformIO project
        import os
        pio_cmd = "pio"
        if not shutil.which("pio"):
            user_home = Path.home()
            if os.name == 'nt':
                pio_path = user_home / ".platformio" / "penv" / "Scripts" / "pio.exe"
            else:
                pio_path = user_home / ".platformio" / "penv" / "bin" / "pio"
            if pio_path.exists():
                pio_cmd = str(pio_path)
            else:
                raise RuntimeError("PlatformIO 'pio' not found in PATH or standard directories.")
        
        run_step(
            [pio_cmd, "run", "-e", "yolo_uno_sensor"],
            status,
            "platformio_build",
            cwd=str(repo_root),
            shell=False
        )

        status["status"] = "succeeded"
        status["current_step"] = "done"
        write_status(status)
        print(json.dumps(status, indent=2))
    except Exception as exc:
        status["status"] = "failed"
        status["current_step"] = "error"
        status["error"] = str(exc)
        write_status(status)
        raise


if __name__ == "__main__":
    main()
