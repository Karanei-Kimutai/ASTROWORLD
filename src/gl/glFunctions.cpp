/**
 * @file glFunctions.cpp
 * @brief Definitions of the OpenGL function pointers and the loader.
 */
#include "gl/glFunctions.h"

// Define every pointer, initially null: PFNGLCLEARPROC astroGl_glClear = nullptr; ...
#define ASTRO_GL_DEFINE(type, name) type astroGl_##name = nullptr;
ASTRO_GL_FUNCTIONS(ASTRO_GL_DEFINE)
#undef ASTRO_GL_DEFINE

bool loadOpenGlFunctions(GlProcLookup lookup, std::string& missingOut) {
    missingOut.clear();
    // Look up each function by its standard name; the #name stringifies it.
#define ASTRO_GL_LOAD(type, name)                                     \
    astroGl_##name = reinterpret_cast<type>(lookup(#name));          \
    if (astroGl_##name == nullptr) missingOut += std::string(#name) + " ";
    ASTRO_GL_FUNCTIONS(ASTRO_GL_LOAD)
#undef ASTRO_GL_LOAD
    return missingOut.empty();
}
