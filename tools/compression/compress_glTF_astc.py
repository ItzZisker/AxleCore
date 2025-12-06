import json
import os
import subprocess
import sys

if len(sys.argv) < 2:
    print("Usage: python compress_glTF_astc.py <path_to_gltf>")
    sys.exit(1)

gltf_path = sys.argv[1]

if not os.path.exists(gltf_path):
    print(f"File not found: {gltf_path}")
    sys.exit(1)

with open(gltf_path, "r") as file:
    gltf = json.load(file)

for image in gltf.get("images", []):
    prev_uri = image["uri"]
    base, _ = os.path.splitext(prev_uri)
    astc_uri = base + ".astc"

    image["uri"] = astc_uri
    image["mimeType"] = "image/astc-6x6"

    try:
        print(f"Converting {prev_uri} -> {astc_uri}")
        subprocess.run(["astcenc.exe", "-cl", prev_uri, astc_uri, "6x6", "-medium"], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error converting {prev_uri}: {e}")
        continue

    if os.path.exists(prev_uri):
        os.remove(prev_uri)

with open(gltf_path, "w") as file:
    json.dump(gltf, file, indent=4)

print(f"Finished converting textures in {gltf_path}")
