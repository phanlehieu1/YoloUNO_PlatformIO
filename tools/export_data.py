import argparse
import csv
import json
from pathlib import Path

FIELDS = [
    "timestamp",
    "temperature",
    "humidity",
    "anomaly_score",
    "is_anomaly",
    "model_version",
    "device_id",
]


def first_present(mapping, keys, default=""):
    for key in keys:
        if key in mapping and mapping[key] not in (None, ""):
            return mapping[key]
    return default


def normalize_row(row):
    return {
        "timestamp": first_present(row, ["timestamp", "ts", "time"]),
        "temperature": first_present(row, ["temperature", "temp"]),
        "humidity": first_present(row, ["humidity", "hum"]),
        "anomaly_score": first_present(row, ["anomaly_score", "score"]),
        "is_anomaly": first_present(row, ["is_anomaly", "label", "anomaly"]),
        "model_version": first_present(row, ["model_version", "model"]),
        "device_id": first_present(row, ["device_id", "device", "macAddress", "mac"]),
    }


def load_json_records(path):
    payload = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(payload, list):
        return payload
    if isinstance(payload, dict):
        for key in ("data", "rows", "items", "telemetry"):
            value = payload.get(key)
            if isinstance(value, list):
                return value
        return [payload]
    raise ValueError("Unsupported JSON structure")


def load_csv_records(path):
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def main():
    parser = argparse.ArgumentParser(description="Normalize CoreIOT/ThingsBoard export into retrain CSV")
    parser.add_argument("input", help="Path to source JSON or CSV export")
    parser.add_argument("output", help="Path to normalized CSV output")
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)

    if input_path.suffix.lower() == ".json":
        records = load_json_records(input_path)
    elif input_path.suffix.lower() == ".csv":
        records = load_csv_records(input_path)
    else:
        raise ValueError("Input must be .json or .csv")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS)
        writer.writeheader()
        for record in records:
            writer.writerow(normalize_row(record))

    print(f"Wrote {len(records)} normalized rows to {output_path}")


if __name__ == "__main__":
    main()
