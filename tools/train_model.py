import argparse
from pathlib import Path

import pandas as pd
import tensorflow as tf


def load_split(path):
    frame = pd.read_csv(path)
    x = frame[["temp", "humidity"]].astype("float32").values
    y = frame["label"].astype("float32").values
    return x, y


def build_model():
    return tf.keras.Sequential([
        tf.keras.layers.Input(shape=(2,)),
        tf.keras.layers.Dense(8, activation="relu"),
        tf.keras.layers.Dense(1, activation="sigmoid"),
    ])


def main():
    parser = argparse.ArgumentParser(description="Train TinyML anomaly model for ESP32")
    parser.add_argument("dataset_dir", help="Directory containing train.csv and val.csv")
    parser.add_argument("output_model", help="Path to save trained Keras model")
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--batch-size", type=int, default=16)
    args = parser.parse_args()

    dataset_dir = Path(args.dataset_dir)
    x_train, y_train = load_split(dataset_dir / "train.csv")
    x_val, y_val = load_split(dataset_dir / "val.csv")

    model = build_model()
    model.compile(loss="binary_crossentropy", optimizer="adam", metrics=["accuracy"])
    model.fit(
        x_train,
        y_train,
        epochs=args.epochs,
        batch_size=args.batch_size,
        validation_data=(x_val, y_val),
        verbose=2,
    )

    output_model = Path(args.output_model)
    output_model.parent.mkdir(parents=True, exist_ok=True)
    model.save(output_model)
    print(f"Saved Keras model to {output_model}")


if __name__ == "__main__":
    main()
