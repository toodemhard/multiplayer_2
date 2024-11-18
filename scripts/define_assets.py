import os
import subprocess
from pathlib import Path
import base_path

asset_files = os.listdir("./data/assets")

header_file_path = base_path.base_path / "assets.h"
header_file_path.parent.mkdir(exist_ok=True, parents=True)
with open(header_file_path, "w+") as header:
    header.write("""#pragma once

namespace assets {
""")

    for file in asset_files:
        # var_name = Path(file).stem
        var_name = file.replace(".", "_")
        var_name = var_name.replace(" ", "_")

        header.write(f"    constexpr const char* {var_name} = \"assets/{file}\";\n")

    header.write("}")
