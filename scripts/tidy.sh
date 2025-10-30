#!/bin/bash
# scripts/tidy.sh

echo "Running clang-tidy..."
echo "=============================================="

clang-tidy -p build/ \
	-header-filter='^src/.*' \
	$(find src/ include/ -name "*.c" -o -name "*.h")

echo "=============================================="
echo "✅ Clang-tidy complete"
