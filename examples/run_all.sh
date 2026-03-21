#!/usr/bin/env bash
set -e

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++26 -freflection -Wall -Wextra -O2"

PASS=0
FAIL=0

run_example() {
    local src="$1"
    local name
    name="$(basename "$src" .cpp)"

    printf "\n\033[1;36m══════════════════════════════════════════════════════════\033[0m\n"
    printf "\033[1;33m  %-40s\033[0m\n" "$name"
    printf "\033[1;36m══════════════════════════════════════════════════════════\033[0m\n"

    mkdir -p bin
    if $CXX $CXXFLAGS "$src" -o "bin/$name" 2>&1; then
        # Example 13 requires CLI args or it exits early
        if [[ "$name" == "13_clap_parser" ]]; then
            "bin/$name" --name World --count 2 || true
        else
            "bin/$name" || true
        fi
        PASS=$((PASS + 1))
    else
        echo "  COMPILE ERROR"
        FAIL=$((FAIL + 1))
    fi
}

cd "$(dirname "$0")"

echo ""
echo "  GCC : $($CXX --version | head -1)"
echo "  flags: -std=c++26 -freflection"
echo ""

for f in [0-9]*.cpp; do
    run_example "$f"
done

printf "\n\033[1;36m══════════════════════════════════════════════════════════\033[0m\n"
if [ "$FAIL" -eq 0 ]; then
    printf "\033[1;32m  All %d examples passed.\033[0m\n" "$PASS"
else
    printf "\033[1;31m  %d passed, %d failed.\033[0m\n" "$PASS" "$FAIL"
fi
printf "\033[1;36m══════════════════════════════════════════════════════════\033[0m\n\n"
