#include "ofxNDILoader.h"

#if defined(_WIN32)
#include <windows.h>
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
    HMODULE hLib = LoadLibraryA(libName);
    if (!hLib) {
        // Try executable directory
        char path[MAX_PATH];
        DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            // Find last backslash and replace filename with dll name
            for (int i = (int)len - 1; i >= 0; i--) {
                if (path[i] == '\\' || path[i] == '/') {
                    path[i + 1] = '\0';
                    break;
                }
            }
            strncat(path, libName, MAX_PATH - strlen(path) - 1);
            hLib = LoadLibraryA(path);
        }
    }
    if (!hLib) return nullptr;

    typedef const NDIlib_v6* (*NDIlib_v6_load_t)(void);
    NDIlib_v6_load_t loadFn = (NDIlib_v6_load_t)GetProcAddress(hLib, "NDIlib_v6_load");
    if (!loadFn) return nullptr;

    return loadFn();

#elif defined(__APPLE__)
    void* hLib = dlopen("libndi.dylib", RTLD_LOCAL | RTLD_LAZY);
    if (!hLib) {
        // Try common install path
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
