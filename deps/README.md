# BDK Dependencies

This is a custom-made dependency system for BDK, similar in concept (but not in practice) to Bitcoin's "depends" system.

## How it works

All dependencies (deps) are compiled from source. The system itself is structured like this:

* `scripts`: contains instructions for each self-contained dependency as its own script
* `deps.sh`: the main script, calls the other scripts

See "How to maintain" for more info on the internals.

## Requirements

Make sure that:

* The main project (bdk-cpp) and this folder are in a PATH that does ***NOT*** have spaces, accented or non-ASCII characters in it
  * e.g. "/home/user/Área de trabalho" - some compilation processes usually throw a fit under this specific condition
* The following programs/libs are installed: `cmake`, `gcc/g++`, `git`, `go`, `make`, `lz4`, `perl`, `python3`, `zlib`
  * Make sure you have the STATIC (.a) versions of libs installed, as BDK only links to those (distros like Arch may not ship static libs)

## How to use

Run `./deps.sh` as-is (without parameters) to print some help information. The script requires two parameters:

* One of the following commands: `fetch`, `config`, `build`, `install`, `clean` or `remove`
  * `fetch` downloads the dependency and/or applies patches to it if required
  * `config` calls the dependency's configuration command (e.g. `./configure` or `cmake`)
  * `build` calls the dependency's build command (e.g. `make` or `cmake --build`)
  * `install` calls the dependency's install command (e.g. `make install` or `cmake --install`)
  * `clean` calls the dependency's clean-build command (e.g. `make clean` or `cmake --build . --target clean`)
  * `remove` simply deletes the dependency's source code folder (useful for starting from scratch if needed)
* Either `all` (for all deps at once) or a specific module name (check the help information for valid names)

For example, `./deps.sh fetch all` will fetch all dependencies, `./deps.sh build boost` will build only Boost, etc..

If doing a clean build, we recommend running in order: `fetch all`, `config all`, `build all` and `install all`.

## How to add/maintain

Each dependency script must implement some variables and functions that will be called by `./deps.sh` (`xyz` is a placeholder name - also check some of the existing scripts for further reference):

* `XYZ_VERSION` - string for handling dependency versioning
* `XYZ_ROOT` - where the dependency source code will be downloaded to (usually `${DEPS_ROOT}/src/xyz`)
* `fetch_xyz()` - fetch the dependency's source code (e.g. `git clone --depth 1 --branch "${XYZ_VERSION}"`, adapt accordingly). Other kinds of repo initialization (e.g. `git submodule update`) and patching source code (if required) should be done here too
* `config_xyz()` - configure the dependency, if required (e.g. `cmake -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL} ..`)
* `build_xyz()` - build the dependency (e.g. `make` or `cmake --build .`)
* `install_xyz()` - install the dependency (e.g. `make install --prefix="${DEPS_INSTALL}"`)
* `clean_xyz()` - clean the dependency's build folder, if required (e.g. `make clean` or `cmake --build . --target clean`)
* `remove_xyz()` - delete the dependency's source code folder (useful for re-fetching)

If a dependency doesn't need one of the steps (e.g. no configuring), you can skip the respective function.

The following helper variables are defined in `deps.sh` and accessible to all scripts:

* `DEPS_ROOT` points to the root of this folder (where the `deps.sh` script is)
* `DEPS_SRC` points to the subfolder where dependencies will be downloaded (`${DEPS_ROOT}/src` by default)
* `DEPS_INSTALL` points to the subfolder where dependencies will be installed (`${DEPS_ROOT}/target` by default)

Then, add the necessary function calls to the main `deps.sh` script alongside the others.

