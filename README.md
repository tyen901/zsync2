# zsync2

![CI](https://github.com/AppImage/zsync2/workflows/CI/badge.svg)

A rewrite of the advanced file download/sync tool zsync.

*Note: "zsync2" is a working title which might be changed in the future.*

The rewrite changes fundamental principles of how zsync works. For example,
the new code will be C++11. Furthermore, the entire functionality will be
bundled in a single library called libzsync2. The library will serve as a base
for the new zsync2 main binary, but can then also be linked by other projects
which seek to make use of the algorithms and functionality in it.

This project is intended to remain compatible to the original zsync, i.e.,
the zsync file format will stay the same etc. However, the zsync file format
should be extended to be able to serve additional purposes as well. Such
purposes could be e.g., adding meta information like version numbers, etc.


## Requirements

Although the library is written in C++11, it is intended that the headers
are compatible to older revisions of the C++ language. Therefore, a pattern
called Opaque Data Type is applied to all classes defined by the headers of
this library.

## Building

To build on Debian or Ubuntu style systems:

```
sudo apt-get -y install git cmake g++ libssl-dev libssh2-1-dev libcurl4-gnutls-dev zlib1g-dev libgcrypt20-dev
   
git submodule update --init
mkdir build
cd build
cmake .. -DUSE_SYSTEM_CURL=1 -DBUILD_CPR_TESTS=0
make -j$(nproc)
```

See `.travis.yml` for building on Travis CI.


### Windows

Below are two supported Windows workflows. The repository builds the bundled zlib by default; avoid installing `zlib` into your vcpkg instance unless you explicitly intend to replace the bundled copy (installing zlib alongside the bundled zlib can cause duplicate symbol linker errors).

Windows (recommended): Build with vcpkg (recommended for dependencies like cpr, gtest)

1. Install vcpkg (one-time):

```powershell
git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
Push-Location C:/vcpkg
.\bootstrap-vcpkg.bat
Pop-Location
```

2. Install the dependencies you want to supply from vcpkg (example x64):

```powershell
C:/vcpkg/vcpkg.exe install gtest:x64-windows
C:/vcpkg/vcpkg.exe install cpr:x64-windows
# Do NOT install zlib unless you understand symbol collision risks
# C:/vcpkg/vcpkg.exe install zlib:x64-windows
```

3. Configure and build (Visual Studio generator example):

```powershell
# from the repository root
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug --parallel 4
```

Alternate: configure for Ninja while still using the vcpkg toolchain (preferred for fast command-line builds):

```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug --parallel 4
```

4. Run tests:

```powershell
Push-Location build
ctest -C Debug --output-on-failure
Pop-Location
```

Windows (no vcpkg): Build with Visual Studio or Ninja and rely on bundled libraries

If you don't want to use vcpkg, the project will build the bundled copies of its vendored libraries (including zlib). Use a matching generator for your Visual Studio or Ninja setup:

```powershell
# Visual Studio generator (creates a .sln in build/)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4

# Or use Ninja (command-line build)
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
```

Notes and common pitfalls:

- Bundled zlib: the project builds and links the bundled zlib by default. Installing a conflicting system zlib (e.g., into vcpkg) while also building the bundled copy can produce duplicate symbol linker errors (inflate, zlibVersion, etc.). If you must use a system zlib, make sure you configure the project and toolchain consistently so the bundled zlib is not also built.
- CPR and args: you can provide system-installed `cpr` and `args` via vcpkg. If CMake cannot find a system `cpr` package, the project will fetch and build its bundled copy automatically.
- If CMake caches a failed find_package() result, delete `build/CMakeCache.txt` (or the whole `build/` folder) and re-run the configure command after changing vcpkg installs or CMake options.

## Functional description

zsync is a well known tool for downloading and updating local files from HTTP
servers using the well known algorithms rsync uses for diffing binary files.
Therefore, it becomes possible to synchronize modifications by exchanging the
changed blocks locally using `Range:` requests.

The system is based on meta files called `.zsync` files. They contain hash
sums for every block of data. The file is generated from and stored along
with the actual file it refers to. First, the client downloads the meta file.
Then, it lets the same algorithms used to generate the meta file hash the
local file. Then, just like with rsync, both lists of hash sums are compared.
Then, modified blocks are fetched from the server, and with the unmodified
blocks of binary data from the original files, a new file is put together,
which eventually replaces the original.

Due to how system works, nothing but a "dumb" HTTP server is required to make
use of zsync2. This makes it easy to integrate zsync2 into existing systems.


## Applications

A popular application scenario is software deployment. Here, the popular
"update channel" system can be used to describe the update model: For normal
updates (i.e., staying on the same channel), one should call zsync2 with a URL
to the `.zsync` file of the latest release of the application. Without
additional effort, zsync2 is then going to update the file accordingly. One
does not have to compare meta data to check for updates etc., and information
like version numbers for example become purely informational for the user, but
irrelevant for the actual update process. This makes setting up an update
infrastructure easier (one just has to set up a static URL to the latest
file).


## Licensing

The original source code has been released under the terms of the Artistic
License v2. Since this rewrite is a derivative work, it is published under the
same license. See COPYING for a copy of the license.


## Current State

Although zsync2 still shares a lot of the code with the original project, it is
as of now not functional. While debugging is ongoing, the API is somewhat ready
to be used by other projects. The project is therefore published in this state
to allow testing the integration into other projects. 
