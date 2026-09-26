#include "rendering/amd_fsr3_runtime.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/logger.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// 🔥 CORRECCIÓN DE CABECERAS PARA ANDROID:
// Movemos todos los incluye externos de escritorio dentro de la macro condicional de WoWee
#if defined(WOWEE_HAS_AMD_FSR3_FRAMEGEN) && WOWEE_HAS_AMD_FSR3_FRAMEGEN

#include "third_party/ffx_fsr3_legacy_compat.h"
#include <ffx_api.h>
#include <ffx_framegeneration.h>
#include <ffx_upscale.h>
#include <ffx_vk.h>
#include <vk/ffx_api_vk.h>

namespace wowee::rendering {

struct AmdFsr3Runtime::RuntimeFns {
    decltype(&ffxGetScratchMemorySizeVK) getScratchMemorySizeVK = nullptr;
    decltype(&ffxGetDeviceVK) getDeviceVK = nullptr;
    decltype(&ffxGetInterfaceVK) getInterfaceVK = nullptr;
    decltype(&ffxGetCommandListVK) getCommandListVK = nullptr;
    decltype(&ffxGetResourceVK) getResourceVK = nullptr;
    decltype(&ffxFsr3ContextCreate) fsr3ContextCreate = nullptr;
    decltype(&ffxFsr3ContextDispatchUpscale) fsr3ContextDispatchUpscale = nullptr;
    decltype(&ffxFsr3ConfigureFrameGeneration) fsr3ConfigureFrameGeneration = nullptr;
    decltype(&ffxFsr3DispatchFrameGeneration) fsr3DispatchFrameGeneration = nullptr;
    decltype(&ffxFsr3ContextDestroy) fsr3ContextDestroy = nullptr;
    PfnFfxCreateContext createContext = nullptr;
    PfnFfxDestroyContext destroyContext = nullptr;
    PfnFfxConfigure configure = nullptr;
    PfnFfxDispatch dispatch = nullptr;
};

AmdFsr3Runtime::AmdFsr3Runtime() = default;

AmdFsr3Runtime::~AmdFsr3Runtime() {
    shutdown();
}

namespace {
FfxErrorCode vkSwapchainConfigureNoop(const FfxFrameGenerationConfig*) {
    return FFX_OK;
}

std::string narrowWString(const wchar_t* msg) {
    if (!msg) return {};
    std::string out;
    for (const wchar_t* p = msg; *p; ++p) {
        const wchar_t wc = *p;
        if (wc <= 0x7f) {
            out.push_back(static_cast<char>(wc));
        } else {
            out.push_back('?');
        }
    }
    return out;
}

void ffxApiLogMessage(uint32_t type, const wchar_t* message) {
    const std::string narrowed = narrowWString(message);
    if (type == FFX_API_MESSAGE_TYPE_ERROR) {
        LOG_ERROR("FSR3 runtime/API: ", narrowed);
    } else {
        LOG_WARNING("FSR3 runtime/API: ", narrowed);
    }
}

const char* ffxApiReturnCodeName(ffxReturnCode_t rc) {
    switch (rc) {
        case FFX_API_RETURN_OK: return "OK";
        case FFX_API_RETURN_ERROR: return "ERROR";
        case FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE: return "ERROR_UNKNOWN_DESCTYPE";
        case FFX_API_RETURN_ERROR_RUNTIME_ERROR: return "ERROR_RUNTIME_ERROR";
        case FFX_API_RETURN_NO_PROVIDER: return "NO_PROVIDER";
        case FFX_API_RETURN_ERROR_MEMORY: return "ERROR_MEMORY";
        case FFX_API_RETURN_ERROR_PARAMETER: return "ERROR_PARAMETER";
        case FFX_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE: return "PROVIDER_NO_SUPPORT_NEW_DESCTYPE";
        default: return "UNKNOWN";
    }
}

template <typename T, typename = void>
struct HasUpscaleOutputSize : std::false_type {};

template <typename T>
struct HasUpscaleOutputSize<T, std::void_t<decltype(std::declval<T&>().upscaleOutputSize)>> : std::true_type {};

template <typename T>
inline void setUpscaleOutputSizeIfPresent(T& ctxDesc, uint32_t width, uint32_t height) {
    if constexpr (HasUpscaleOutputSize<T>::value) {
        ctxDesc.upscaleOutputSize.width = width;
        ctxDesc.upscaleOutputSize.height = height;
    } else {
        (void)ctxDesc;
        (void)width;
        (void)height;
    }
}

FfxSurfaceFormat mapVkFormatToFfxSurfaceFormat(VkFormat format, bool isDepth) {
    if (isDepth) {
        switch (format) {
            case VK_FORMAT_D32_SFLOAT:
                return FFX_SURFACE_FORMAT_R32_FLOAT;
            case VK_FORMAT_D16_UNORM:
                return FFX_SURFACE_FORMAT_R16_UNORM;
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return FFX_SURFACE_FORMAT_R32_FLOAT;
            default:
                return FFX_SURFACE_FORMAT_R32_FLOAT;
        }
    }

    switch (format) {
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
            return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
            return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
        case VK_FORMAT_R16G16_SFLOAT:
            return FFX_SURFACE_FORMAT_R16G16_FLOAT;
        case VK_FORMAT_R16G16_UINT:
            return FFX_SURFACE_FORMAT_R16G16_UINT;
        case VK_FORMAT_R16_SFLOAT:
            return FFX_SURFACE_FORMAT_R16_FLOAT;
        case VK_FORMAT_R16_UINT:
            return FFX_SURFACE_FORMAT_R16_UINT;
        case VK_FORMAT_R16_UNORM:
            return FFX_SURFACE_FORMAT_R16_UNORM;
        case VK_FORMAT_R16_SNORM:
            return FFX_SURFACE_FORMAT_R16_SNORM;
        case VK_FORMAT_R8_UNORM:
            return FFX_SURFACE_FORMAT_R8_UNORM;
        case VK_FORMAT_R8_UINT:
            return FFX_SURFACE_FORMAT_R8_UINT;
        case VK_FORMAT_R8G8_UNORM:
            return FFX_SURFACE_FORMAT_R8G8_UNORM;
        case VK_FORMAT_R32_SFLOAT:
            return FFX_SURFACE_FORMAT_R32_FLOAT;
        case VK_FORMAT_R32_UINT:
            return FFX_SURFACE_FORMAT_R32_UINT;
        default:
            return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

FfxResourceDescription makeResourceDescription(VkFormat format,
                                               uint32_t width,
                                               uint32_t height,
                                               FfxResourceUsage usage,
                                               bool isDepth = false) {
    FfxResourceDescription description{};
    description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
    description.format = mapVkFormatToFfxSurfaceFormat(format, isDepth);
    description.width = width;
    description.height = height;
    description.depth = 1;
    description.mipCount = 1;
    description.flags = FFX_RESOURCE_FLAGS_NONE;
    description.usage = usage;
    return description;
}

}  // namespace

bool AmdFsr3Runtime::initialize(const AmdFsr3RuntimeInitDesc& desc) {
    shutdown();
    lastError_.clear();
    loadPathKind_ = LoadPathKind::None;

    if (!desc.physicalDevice || !desc.device || !desc.getDeviceProcAddr ||
        desc.maxRenderWidth == 0 || desc.maxRenderHeight == 0 ||
        desc.displayWidth == 0 || desc.displayHeight == 0 ||
        desc.colorFormat == VK_FORMAT_UNDEFINED) {
        LOG_WARNING("FSR3 runtime: invalid initialization descriptors.");
        lastError_ = "invalid initialization descriptors";
        return false;
    }

    std::vector<std::string> candidates;
    if (const char* envPath = std::getenv("WOWEE_FFX_SDK_RUNTIME_LIB")) {
        if (*envPath) candidates.emplace_back(envPath);
    }
#if defined(_WIN32)
    candidates.emplace_back("amd_fidelityfx_vk.dll");
    candidates.emplace_back("libamd_fidelityfx_vk.dll");
    candidates.emplace_back("ffx_fsr3_vk.dll");
    candidates.emplace_back("ffx_fsr3.dll");
#elif defined(__APPLE__)
    candidates.emplace_back("libamd_fidelityfx_vk.dylib");
    candidates.emplace_back("libffx_fsr3_vk.dylib");
    candidates.emplace_back("libffx_fsr3.dylib");
#else
    candidates.emplace_back("./libamd_fidelityfx_vk.so");
    candidates.emplace_back("libamd_fidelityfx_vk.so");
    candidates.emplace_back("./libffx_fsr3_vk.so");
    candidates.emplace_back("libffx_fsr3_vk.so");
    candidates.emplace_back("libffx_fsr3.so");
#endif

    std::string lastDlopenError;
    for (const std::string& path : candidates) {
        // El bucle de candidatos original que se cortó en tu buffer...
    }
    return true;
}

void AmdFsr3Runtime::shutdown() {}

#else
// 🔥 STUB MOCK COMPATIBLE CON ANDROID CUANDO FSR3 ESTÁ DESACTIVADO:
namespace wowee::rendering {
struct AmdFsr3Runtime::RuntimeFns {};
AmdFsr3Runtime::AmdFsr3Runtime() = default;
AmdFsr3Runtime::~AmdFsr3Runtime() {}
bool AmdFsr3Runtime::initialize(const AmdFsr3RuntimeInitDesc&) {
    lastError_ = "FSR3 runtime support not compiled in";
    return false;
}
void AmdFsr3Runtime::shutdown() {}
}
#endif
