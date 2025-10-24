import os

# Hole Version von GitHub Actions oder lokal "dev"
version = os.getenv("GITHUB_RUN_NUMBER", "dev")
version_str = f"v{version}"

# Schreib in version.txt
with open("version.txt", "w") as f:
    f.write(version_str + "\n")

# Schreib in include/version.h
os.makedirs("include", exist_ok=True)
with open("include/version.h", "w") as f:
    f.write(f'#pragma once\n#define CURRENT_VERSION "{version_str}"\n')

print(f"// 🔧 Build Version: {version_str}")
