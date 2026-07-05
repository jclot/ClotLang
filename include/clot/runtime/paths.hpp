#pragma once

#include <filesystem>
#include <vector>

namespace clot::runtime {

// Absolute path to the running executable, or an empty path if it cannot be
// determined on this platform.
std::filesystem::path ExecutablePath();

// Directories that may contain the bundled standard library (a `clot/`
// subfolder). Derived from the `CLOT_HOME` environment variable and from the
// location of the running executable, so the stdlib resolves regardless of the
// current working directory once Clot is installed.
std::vector<std::filesystem::path> StdlibSearchRoots();

}  // namespace clot::runtime
