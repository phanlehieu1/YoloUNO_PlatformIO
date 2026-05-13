import argparse
import json
from pathlib import Path

import pandas as pd
import tensorflow as tf


def load_split(path):
    frame = pd.read_csv(path)
    x = frame[["temp", "humidity"]].astype("float32").values
    y = frame["label"].astype("float32").values
    return x, y


def main():
    parser = argparse.ArgumentParser(description="Evaluate TinyML model on held-out test split")
    parser.add_argument("model_path", help="Saved Keras model path")
    parser.add_argument("test_csv", help="Prepared test split CSV path")
    parser.add_argument("output_json", help="Path to write metrics JSON")
    args = parser.parse_args()

    model = tf.keras.models.load_model(args.model_path)
    x_test, y_test = load_split(Path(args.test_csv))
    loss, accuracy = model.evaluate(x_test, y_test, verbose=0)

    predictions = model.predict(x_test, verbose=0).reshape(-1)
    predicted = (predictions >= 0.5).astype(int)
    tp = int(((predicted == 1) & (y_test == 1)).sum())
    tn = int(((predicted == 0) & (y_test == 0)).sum())
    fp = int(((predicted == 1) & (y_test == 0)).sum())
    fn = int(((predicted == 0) & (y_test == 1)).sum())

    metrics = {
        "loss": float(loss),
        "accuracy": float(accuracy),
        "threshold": 0.5,
        "samples": int(len(y_test)),
        "true_positive": tp,
        "true_negative": tn,
        "false_positive": fp,
        "false_negative": fn,
    }

    output_path = Path(args.output_json)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    print(json.dumps(metrics, indent=2))


if __name__ == "__main__":
    main()
