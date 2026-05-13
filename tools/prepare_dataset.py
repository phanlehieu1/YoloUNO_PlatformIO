import argparse
import csv
import random
from pathlib import Path

OUTPUT_FIELDS = ["temp", "humidity", "label", "model_version", "timestamp", "device_id"]


def parse_label(value):
    text = str(value).strip().lower()
    return 1 if text in {"1", "true", "yes", "anomaly"} else 0


def load_rows(path):
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        rows = []
        for row in reader:
            try:
                rows.append({
                    "temp": float(row["temperature"]),
                    "humidity": float(row["humidity"]),
                    "label": parse_label(row["is_anomaly"]),
                    "model_version": row.get("model_version", ""),
                    "timestamp": row.get("timestamp", ""),
                    "device_id": row.get("device_id", ""),
                })
            except (KeyError, TypeError, ValueError):
                continue
        return rows


def write_rows(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=OUTPUT_FIELDS)
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description="Prepare TinyML dataset splits from normalized telemetry")
    parser.add_argument("input", help="Normalized CSV from export_data.py")
    parser.add_argument("output_dir", help="Directory for train/val/test CSV files")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--train-ratio", type=float, default=0.7)
    parser.add_argument("--val-ratio", type=float, default=0.15)
    args = parser.parse_args()

    rows = load_rows(Path(args.input))
    if not rows:
        raise ValueError("No valid rows found in input dataset")

    random.Random(args.seed).shuffle(rows)

    train_end = int(len(rows) * args.train_ratio)
    val_end = train_end + int(len(rows) * args.val_ratio)

    output_dir = Path(args.output_dir)
    write_rows(output_dir / "train.csv", rows[:train_end])
    write_rows(output_dir / "val.csv", rows[train_end:val_end])
    write_rows(output_dir / "test.csv", rows[val_end:])

    print(f"Prepared dataset: train={train_end}, val={val_end - train_end}, test={len(rows) - val_end}")


if __name__ == "__main__":
    main()
