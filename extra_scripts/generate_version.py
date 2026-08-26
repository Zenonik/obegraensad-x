import os

version = os.getenv("GITHUB_RUN_NUMBER", "dev")
version_str = f"v{version}"

try:
    with open("version.txt", "w") as f:
        f.write(version_str + "\n")
except OSError:
    pass

try:
    os.makedirs("include", exist_ok=True)
    with open("include/version.h", "w") as f:
        f.write(f'#pragma once\n#define CURRENT_VERSION "{version_str}"\n')
except OSError:
    pass

print(f"// Build Version: {version_str}")
