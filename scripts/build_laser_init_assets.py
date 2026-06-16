#!/usr/bin/env python3
import argparse
import os
import shutil

from build_default_assets import generate_config_json, generate_index_json, pack_assets_simple


def main():
    parser = argparse.ArgumentParser(description="Pack laser UI init frame bins into assets.bin")
    parser.add_argument("--input_dir", required=True, help="Directory containing init*.bin files")
    parser.add_argument("--output", required=True, help="Output assets.bin path")
    args = parser.parse_args()

    input_dir = os.path.abspath(args.input_dir)
    output_path = os.path.abspath(args.output)

    if not os.path.isdir(input_dir):
        raise SystemExit(f"Input directory not found: {input_dir}")

    files = sorted(
        name for name in os.listdir(input_dir)
        if name.lower().endswith(".bin") and os.path.isfile(os.path.join(input_dir, name))
    )
    if not files:
        raise SystemExit(f"No .bin files found in: {input_dir}")

    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    temp_dir = os.path.join(os.path.dirname(output_path), "laser_init_assets_tmp")
    if os.path.exists(temp_dir):
        shutil.rmtree(temp_dir)

    try:
        assets_dir = os.path.join(temp_dir, "assets")
        os.makedirs(assets_dir, exist_ok=True)

        for name in files:
            shutil.copy2(os.path.join(input_dir, name), os.path.join(assets_dir, name))

        generate_index_json(assets_dir, None, None, None, files, None)
        config_path = generate_config_json(temp_dir, assets_dir)

        with open(config_path, "r", encoding="utf-8") as f:
            import json
            config_data = json.load(f)

        pack_assets_simple(
            assets_dir,
            config_data["include_path"],
            config_data["image_file"],
            "assets",
            int(config_data["name_length"]),
        )
        shutil.copy2(config_data["image_file"], output_path)
        print(f"Generated laser init assets: {output_path}")
    finally:
        if os.path.exists(temp_dir):
            shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
