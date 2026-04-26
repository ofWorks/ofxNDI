#pragma once

#include <cstddef> // for NULL — NDI headers use it without defining
#include "Processing.NDI.Lib.h"

// Cross-platform dynamic loader for the NDI library.
// Avoids needing import libraries (.lib) on Windows or system installs on macOS/Linux.
// Returns a pointer to the loaded NDIlib_v6 interface, or nullptr on failure.
const NDIlib_v6* ofxNDILoad();
