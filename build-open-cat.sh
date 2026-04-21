#!/usr/bin/env bash
set -e # immediately exit if any command returns non-zero exit code

arduino-cli compile --fqbn esp32:esp32:esp32 --board-options "CPUFreq=80,DebugLevel=verbose,PartitionScheme=partitions_factory_only" OpenCatEsp32 --output-dir ./OpenCatEsp32/build

# --build-property "build.extra_flags=-DCORE_DEBUG_LEVEL=5"
#  --build-property "build.partitions=huge_app"

# Added the partitions_factory_only.csv to /Users/jay/Library/Arduino15/packages/esp32/hardware/esp32/2.0.12/tools/partitions/

# Added the following lines in /Users/jay/Library/Arduino15/packages/esp32/hardware/esp32/2.0.12/boards.txt:
# esp32.menu.PartitionScheme.partitions_factory_only =partitions_factory_only
# esp32.menu.PartitionScheme.partitions_factory_only.build.partitions=partitions_factory_only
# esp32.menu.PartitionScheme.partitions_factory_only.upload.maximum_size=3145728

xtensa-esp32-elf-nm -n OpenCatEsp32/build/OpenCatEsp32.ino.elf > OpenCatEsp32/build/symbols.txt

# Initial build files
mkdir -p www/static/OpenCatEsp32/build/
cp OpenCatEsp32/build/OpenCatEsp32.ino.bootloader.bin www/static/OpenCatEsp32/build/
cp OpenCatEsp32/build/OpenCatEsp32.ino.partitions.bin www/static/OpenCatEsp32/build/
cp OpenCatEsp32/build/OpenCatEsp32.ino.bin www/static/OpenCatEsp32/build/
cp OpenCatEsp32/build/symbols.txt www/static/OpenCatEsp32/build/
cp OpenCatEsp32/bootloader_symbols.txt www/static/OpenCatEsp32
cp -r rom www/static/rom

# Display files
cp OpenCatEsp32/OpenCatEsp32.ino www/src/assets/
cp -r OpenCatEsp32/src www/src/assets/

# default compiled binary and symbol for when user hits reset
cp OpenCatEsp32/build/OpenCatEsp32.ino.bin www/src/assets/
cp OpenCatEsp32/build/symbols.txt www/src/assets/
