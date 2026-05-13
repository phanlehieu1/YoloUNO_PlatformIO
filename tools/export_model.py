import argparse
from pathlib import Path

import tensorflow as tf


SYMBOL_NAME = "dht_anomaly_model_tflite"
LINE_WIDTH = 12


def write_header(tflite_bytes, output_path):
    hex_rows = []
    for index in range(0, len(tflite_bytes), LINE_WIDTH):
        chunk = tflite_bytes[index:index + LINE_WIDTH]
        hex_rows.append(", ".join(f"0x{value:02x}" for value in chunk))

    body = ",\n  ".join(hex_rows)
    header = (
        f"const unsigned char {SYMBOL_NAME}[] = {{\n  {body}\n}};\n"
        f"const unsigned int {SYMBOL_NAME}_len = {len(tflite_bytes)};\n"
    )
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(header, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Export trained Keras model to TFLite header for ESP32 firmware")
    parser.add_argument("model_path", help="Saved Keras model path")
    parser.add_argument("output_header", help="Header file path, e.g. src/dht_anomaly_model.h")
    args = parser.parse_args()

    model = tf.keras.models.load_model(args.model_path)
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_model = converter.convert()

    output_path = Path(args.output_header)
    write_header(tflite_model, output_path)
    print(f"Wrote TFLite header to {output_path}")


if __name__ == "__main__":
    main()
