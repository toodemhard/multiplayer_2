import os
import subprocess
from pathlib import Path

from base_path import base_path


files = os.listdir("./shaders")

shader_out_dir = Path("data/shaders/")
shader_out_dir.mkdir(exist_ok=True, parents=True)
for src in files:
    subprocess.run(["glslc", f"./shaders/{src}", "-o", shader_out_dir / f"{src}.spv"])

header_file_path = base_path / "shaders.h"
header_file_path.parent.mkdir(exist_ok=True, parents=True)
with open(header_file_path, "w+") as header:
    header.write("""#pragma once

namespace shaders {
""")
    for src in files:
        var_name = src.replace(".", "_")
        header.write(f"    constexpr const char* {var_name} = \"shaders/{src}.spv\";\n")

    header.write("}")
