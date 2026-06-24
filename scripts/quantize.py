#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Unified quantization entrypoint for the Qt GUI.

Currently supports ONNX Runtime dynamic INT8 PTQ and delegates to quantize_onnx.py.
"""

import argparse
import runpy
import sys
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--backend", default="onnxruntime", choices=["onnxruntime"])
    parser.add_argument("--type", default="dynamic-int8", choices=["dynamic-int8"])
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    script = Path(__file__).with_name("quantize_onnx.py")
    sys.argv = [str(script), "--input", args.input, "--output", args.output]
    runpy.run_path(str(script), run_name="__main__")


if __name__ == "__main__":
    main()
