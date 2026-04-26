#include "ofxNDILoader.h"

#if defined(_WIN32)
#include <windows.h>
#include <cstring> // for strlen, strncat
#elif defined(__APPLE__)
#include <dlfcn.h>
#else // Linux
#include <dlfcn.h>
#endif

const NDIlib_v6* ofxNDILoad() {
#if defined(_WIN32)
#ifdef _WIN64
    const char* libName = "Processing.NDI.Lib.x64.dll";
#else
    const char* libName = "Processing.NDI.Lib.x86.dll";
#endif

    HMODULE hLib = nullptr;
    char path[MAX_PATH];

    // 1. Try executable directory with full path
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        for (int i = (int)len - 1; i >= 0; i--) {
            if (path[i] == '\\' || path[i] == '/') {
                path[i + 1] = '\0';
                break;
            }
        }
        strncat(path, libName, MAX_PATH - strlen(path) - 1);
        hLib = LoadLibraryA(path);
    }

    // 2. Try standard DLL search paths (executable dir, system dirs, PATH)
    if (!hLib) {
        hLib = LoadLibraryA(libName);
    }

    // 3. Try NDI runtime installation folder from environment variable
    if (!hLib) {
        char* envFolder = nullptr;
#if defined(_MSC_VER)
        size_t envLen = 0;
        _dupenv_s(&envFolder, &envLen, "NDILIB_REDIST_FOLDER");
#else
        envFolder = getenv("NDILIB_REDIST_FOLDER");
#endif
        if (envFolder) {
            snprintf(path, MAX_PATH, "%s\\%s", envFolder, libName);
            hLib = LoadLibraryA(path);
#if defined(_MSC_VER)
            free(envFolder);
#endif
        }
    }

    // 4. Try common NDI runtime install paths
    if (!hLib) {
        const char* x64Path = "C:\\Program Files\\NDI\\NDI 6 Runtime\\v6\\Processing.NDI.Lib.x64.dll";
        const char* x86Path = "C:\\Program Files (x86)\\NDI\\NDI 6 Runtime\\v6\\Processing.NDI.Lib.x86.dll";
        hLib = LoadLibraryA(_WIN64 ? x64Path : x86Path);
    }

    if (!hLib) return nullptr;

    typedef const NDIlib_v6* (__cdecl *NDIlib_v6_load_t)(void);
    NDIlib_v6_load_t loadFn = (NDIlib_v6_load_t)GetProcAddress(hLib, "NDIlib_v6_load");
    if (!loadFn) return nullptr;

    return loadFn();

#elif defined(__APPLE__)
    void* hLib = dlopen("libndi.dylib", RTLD_LOCAL | RTLD_LAZY);
    if (!hLib) {
        hLib = dlopen("/usr/local/lib/libndi.dylib", RTLD_LOCAL | RTLD_LAZY);
    }
    if (!hLib) return nullptr;

    typedef const NDIlib_v6* (*NDIlib_v6_load_t)(void);
    NDIlib_v6_load_t loadFn = (NDIlib_v6_load_t)dlsym(hLib, "NDIlib_v6_load");
    if (!loadFn) return nullptr;

    return loadFn();

#else // Linux
    void* hLib = dlopen("libndi.so.6", RTLD_LOCAL | RTLD_LAZY);
    if (!hLib) {
        hLib = dlopen("libndi.so", RTLD_LOCAL | RTLD_LAZY);
    }
    if (!hLib) return nullptr;

    typedef const NDIlib_v6* (*NDIlib_v6_load_t)(void);
    NDIlib_v6_load_t loadFn = (NDIlib_v6_load_t)dlsym(hLib, "NDIlib_v6_load");
    if (!loadFn) return nullptr;

    return loadFn();
#endif
}
