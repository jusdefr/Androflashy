#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
case "${1:-release}" in
  debug) exec make debug ;;
  release) exec make release ;;
  clean) exec make clean ;;
  *) echo "Usage: $0 [debug|release|clean]" >&2; exit 2 ;;
esac
