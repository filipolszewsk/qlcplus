#!/bin/bash
# QLC+ with DimmerWave EFX - Launch Script
# This runs the development version without affecting your installed QLC+

cd "$(dirname "$0")/main"
./qlcplus "$@"

