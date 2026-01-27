Import("env")
import os
import re

SCRIPT_VERSION = "2.18.2"
TARGET_TIMEOUT_MS = 3000  # From BOOTLOOP_STATUS_2026-01-18.md proven configuration
MIN_SAFE_TIMEOUT_MS = 3000


def _should_patch_legacy_timeout(content):
    for line in content.splitlines():
        if line.startswith('#define CONFIG_INT_WDT_TIMEOUT_MS '):
            if 'CONFIG_ESP_INT_WDT_TIMEOUT_MS' in line:
                return False
            return True
    return False


def patch_arduino_sdkconfig(env):
    print("=" * 70)
    print(f"🔧 ESP32-S3 Bootloop Fix (v{SCRIPT_VERSION})")
    print(f"Target timeout: {TARGET_TIMEOUT_MS} ms")
    print("=" * 70)

    platform_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    if not platform_dir:
        print("⚠️ Arduino framework not found")
        return

    sdk_dir = os.path.join(platform_dir, "tools", "sdk", "esp32s3")
    if not os.path.isdir(sdk_dir):
        print("⚠️ ESP32-S3 SDK directory not found")
        return

    sdkconfig_files = []
    for root, dirs, files in os.walk(sdk_dir):
        if "qio_opi" not in root:
            continue
        for file in files:
            if file == "sdkconfig.h":
                sdkconfig_files.append(os.path.join(root, file))

    if not sdkconfig_files:
        print("⚠️ No qio_opi sdkconfig.h found")
        return

    patched = 0
    skipped = 0

    for filepath in sdkconfig_files:
        variant = os.path.basename(os.path.dirname(os.path.dirname(filepath)))

        try:
            with open(filepath, "r") as f:
                content = f.read()

            match = re.search(
                r'^#define\s+CONFIG_ESP_INT_WDT_TIMEOUT_MS\s+(\d+)',
                content,
                re.MULTILINE
            )

            if not match:
                print(f"⚠️ {variant}: CONFIG_ESP_INT_WDT_TIMEOUT_MS not found")
                skipped += 1
                continue

            try:
                current = int(match.group(1))
            except ValueError:
                print(f"⚠️ {variant}: invalid timeout value")
                skipped += 1
                continue

            if current >= MIN_SAFE_TIMEOUT_MS:
                print(f"✅ {variant}: already safe ({current}ms)")
                continue

            new_content = re.sub(
                r'^#define\s+CONFIG_ESP_INT_WDT_TIMEOUT_MS\s+\d+',
                f'#define CONFIG_ESP_INT_WDT_TIMEOUT_MS {TARGET_TIMEOUT_MS}',
                content,
                flags=re.MULTILINE
            )

            if _should_patch_legacy_timeout(new_content):
                new_content = re.sub(
                    r'^#define\s+CONFIG_INT_WDT_TIMEOUT_MS\s+\d+',
                    f'#define CONFIG_INT_WDT_TIMEOUT_MS {TARGET_TIMEOUT_MS}',
                    new_content,
                    flags=re.MULTILINE
                )

            tmp = filepath + ".tmp"
            with open(tmp, "w") as f:
                f.write(new_content)
            os.replace(tmp, filepath)

            print(f"🔧 {variant}: patched {current} → {TARGET_TIMEOUT_MS}")
            patched += 1

        except Exception as e:
            print(f"❌ {variant}: {e}")
            skipped += 1

    print("")
    print(f"Patched: {patched}")
    print(f"Skipped: {skipped}")
    print("=" * 70)


patch_arduino_sdkconfig(env)
