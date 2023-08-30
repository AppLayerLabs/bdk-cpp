# Copyright (c) [2023] [Sparq Network]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

#!/bin/bash
for dir in src tests; do
  for file in `find $dir -name "*.cpp" -or -name "*.h"`; do
    echo "Formatting file: $file"
    clang-format -i $file
  done
done
