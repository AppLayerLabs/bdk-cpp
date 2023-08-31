/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

int main() {

  std::string deployerScriptPath = std::filesystem::current_path().string() +
                                   std::string("/scripts/AIO-setup.sh");

  std::string scriptCommand = deployerScriptPath + " --only-deploy";

  const char *cmd = scriptCommand.c_str();

  int result = system(cmd);
  if (result != 0) {
    std::cerr << "Script execution failed with error code " << result
              << std::endl;
  }
  return 0;
}
