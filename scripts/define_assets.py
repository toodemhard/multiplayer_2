import os
import subprocess
from pathlib import Path
import base_path

def write_asset_table(source_dir, file_name):
    asset_files = os.listdir(f"./data/{source_dir}")

    header_file_path = base_path.base_path / f"{file_name}.h"
    header_file_path.parent.mkdir(exist_ok=True, parents=True)

    with open(header_file_path, "w+") as header:
        header.write(f"""#pragma once

#define {file_name.upper()}(_)\\
""")

        for file in asset_files:
            # var_name = Path(file).stem
            var_name = file.replace(".", "_")
            var_name = var_name.replace(" ", "_")

            header.write(f"    _({var_name}, \"{source_dir}/{file}\")\\\n")

write_asset_table("images", "image_table");
write_asset_table("fonts", "font_table");
