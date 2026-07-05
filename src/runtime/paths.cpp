#include "clot/runtime/paths.hpp"

#include <cstdint>
#include <system_error>

#include "clot/runtime/env.hpp"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <vector>
#else
#include <unistd.h>
#endif

namespace clot::runtime {

std::filesystem::path ExecutablePath() {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length =
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            buffer.resize(length);
            return std::filesystem::path(buffer);
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    std::error_code ec;
    std::filesystem::path resolved = std::filesystem::weakly_canonical(buffer.data(), ec);
    return ec ? std::filesystem::path(buffer.data()) : resolved;
#else
    std::error_code ec;
    std::filesystem::path resolved = std::filesystem::read_symlink("/proc/self/exe", ec);
    return ec ? std::filesystem::path{} : resolved;
#endif
}

namespace {

void AddRoot(std::vector<std::filesystem::path>* roots, std::filesystem::path candidate) {
    if (roots == nullptr || candidate.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(candidate, ec);
    if (ec) {
        normalized = candidate.lexically_normal();
    }
    for (const auto& existing : *roots) {
        if (existing == normalized) {
            return;
        }
    }
    roots->push_back(std::move(normalized));
}

// Given a base directory, register the base and its conventional stdlib
// subdirectories (`lib`, `share`) as search roots.
void AddBaseWithSubdirs(std::vector<std::filesystem::path>* roots,
                        const std::filesystem::path& base) {
    if (base.empty()) {
        return;
    }
    AddRoot(roots, base);
    AddRoot(roots, base / "lib");
    AddRoot(roots, base / "share");
}

}  // namespace

std::vector<std::filesystem::path> StdlibSearchRoots() {
    std::vector<std::filesystem::path> roots;

    // Explicit override always wins.
    if (const auto home = GetEnvVar("CLOT_HOME"); home.has_value() && !home->empty()) {
        AddBaseWithSubdirs(&roots, std::filesystem::path(*home));
    }

    // Locate the stdlib relative to the installed binary. Supported layouts:
    //   <prefix>/bin/clot         + <prefix>/lib/clot     (script install)
    //   <pkg>/clot                + <pkg>/lib/clot        (portable archive)
    //   <dir>/clot                + <dir>/clot            (stdlib beside binary)
    const std::filesystem::path executable = ExecutablePath();
    if (!executable.empty()) {
        const std::filesystem::path bin_dir = executable.parent_path();
        AddBaseWithSubdirs(&roots, bin_dir);
        AddBaseWithSubdirs(&roots, bin_dir.parent_path());
    }

    return roots;
}

}  // namespace clot::runtime
