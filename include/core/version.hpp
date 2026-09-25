#pragma once

// Generated from git at build time - see cmake/GitVersion.cmake.
// kVersion is the last tag reachable from HEAD, so tagging a release is all it
// takes to update the version the client reports.

namespace wowee {
namespace core {

static constexpr const char* kVersion = "3.1.38";
static constexpr const char* kBuildDate = "2026-09-24";

// "v2.0.3-preview (built 2026-07-12)" - what the login screen and settings show.
inline constexpr const char* kVersionString =
    "@WOWEE_GIT_VERSION@ (built @WOWEE_BUILD_DATE@)";

} // namespace core
} // namespace wowee
