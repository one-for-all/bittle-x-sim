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
