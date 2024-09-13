#if (defined(OCPN_GHC_FILESYSTEM) || \
     (defined(__clang_major__) && (__clang_major__ < 15)))
// MacOS 1.13
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#else
#include <filesystem>
#include <utility>
namespace fs = std::filesystem;
#endif

