Import("env")
import os
import re
import json
import shutil

def after_build(source, target, env):
    print("\n=======================================")
    print("  POST-BUILD OTA SCRIPT (Auto-Generate)")
    print("=======================================")
    
    project_dir = env.get("PROJECT_DIR")
    config_path = os.path.join(project_dir, "include", "Config.h")
    output_dir = os.path.join(project_dir, "ota_output")
    
    # Target adalah array, index 0 adalah path absolute ke firmware.bin yang baru di-build
    target_bin = str(target[0]) 
    
    # 1. Parse Versi dari Config.h
    version = "unknown"
    if os.path.exists(config_path):
        with open(config_path, "r") as f:
            content = f.read()
            match = re.search(r'#define\s+FIRMWARE_VERSION\s+"([^"]+)"', content)
            if match:
                version = match.group(1)
                
    if version == "unknown":
        print("❌ ERROR: Tidak menemukan FIRMWARE_VERSION di Config.h")
        return

    # 2. Buat folder jika belum ada
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    bin_name = f"brangkas_v{version}.bin"
    output_bin_path = os.path.join(output_dir, bin_name)
    
    # 3. Salin file firmware.bin ke folder ota_output
    shutil.copyfile(target_bin, output_bin_path)
    
    # 4. Generate version.json
    base_url = "http://yourserver.com/brangkas" # Ganti dengan URL server Anda nantinya
    json_data = {
        "version": version,
        "url": f"{base_url}/{bin_name}"
    }
    
    json_path = os.path.join(output_dir, "version.json")
    with open(json_path, "w") as jf:
        json.dump(json_data, jf, indent=4)
        
    # Ambil ukuran file
    file_size_kb = os.path.getsize(output_bin_path) // 1024
    
    print(f"✅ Firmware berhasil disalin: ota_output/{bin_name} ({file_size_kb} KB)")
    print(f"✅ Metadata berhasil dibuat : ota_output/version.json")
    print("=======================================\n")

# Attach function agar dijalankan SETELAH proses link/build firmware.bin selesai
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
