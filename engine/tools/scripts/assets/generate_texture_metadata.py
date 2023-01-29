import os

directory = "Z:\\workshop\\engine\\assets\\models\\test_scenes\\sponza\\textures"
virtual_output_path = "data:models/test_scenes/sponza/textures"
metadata_template = """# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
type: texture
version: 1

group: world

usage: {{usage}}

faces:
    - {{path}}
"""

print("Generating metadata for files in: " + directory)
for file in os.listdir(directory):
    filename = os.fsdecode(file)
    if filename.endswith("png"):
        metadata_file = filename.replace(".png", ".yaml")
        metadata_path = os.path.join(directory, metadata_file)
        if not os.path.exists(metadata_path):

            usage = "color"
            if "roughness" in filename.lower():
                usage = "roughness"
            elif "normal" in filename.lower():
                usage = "normal"
            elif "metalness" in filename.lower():
                usage = "metallic"
            
            virtual_path = virtual_output_path + "/" + filename

            metadata = metadata_template
            metadata = metadata.replace("{{usage}}", usage)
            metadata = metadata.replace("{{path}}", virtual_path)

            print("Generating metadata for: " + metadata_path)

            handle = open(metadata_path, "wt")
            handle.write(metadata)
            handle.close()