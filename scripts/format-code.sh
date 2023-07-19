#!/bin/bash
for dir in src tests; do
  for file in `find $dir -name "*.cpp" -or -name "*.h"`; do
    echo "Formatting file: $file"
    clang-format -i $file
  done
done
