/**
 * @file glFunctions.h
 * @brief Our own minimal OpenGL 3.3 core function loader (what libraries like GLAD do).
 *
 * WHY A LOADER IS NEEDED: on most platforms the system OpenGL library only
 * exports the ancient OpenGL 1.1 functions. Every modern function
 * (glCreateShader, glGenVertexArrays, ...) lives inside the graphics DRIVER
 * and must be looked up by name at runtime, AFTER a context exists. GLFW
 * provides glfwGetProcAddress for that lookup.
 *
 * HOW IT WORKS:
 *   - The official Khronos header <GL/glcorearb.h> supplies the constants
 *     (GL_TRIANGLES, ...) and the function-pointer TYPES (PFNGLCLEARPROC, ...).
 *   - ASTRO_GL_FUNCTIONS lists every function this project uses (an "X-macro").
 *   - For each entry we declare a pointer named astroGl_<name>, plus a macro so
 *     the code can keep writing the standard name (glClear -> astroGl_glClear).
 *     The prefix avoids clashing with symbols that libGL itself exports.
 *   - loadOpenGlFunctions() fills every pointer and reports any that are missing.
 *
 * Include this header INSTEAD of <GL/gl.h>, and define GLFW_INCLUDE_NONE before
 * including GLFW so GLFW does not pull in the old header.
 */
#pragma once

#include <GL/glcorearb.h>

#include <string>

/** @brief Every OpenGL function the project calls: X(function pointer type, name). */
#define ASTRO_GL_FUNCTIONS(X)                                              \
    /* state and frame */                                                  \
    X(PFNGLCLEARPROC, glClear)                                             \
    X(PFNGLFINISHPROC, glFinish)                                           \
    X(PFNGLCLEARCOLORPROC, glClearColor)                                   \
    X(PFNGLENABLEPROC, glEnable)                                           \
    X(PFNGLDISABLEPROC, glDisable)                                         \
    X(PFNGLVIEWPORTPROC, glViewport)                                       \
    X(PFNGLDEPTHFUNCPROC, glDepthFunc)                                     \
    X(PFNGLDEPTHMASKPROC, glDepthMask)                                     \
    X(PFNGLBLENDFUNCPROC, glBlendFunc)                                     \
    X(PFNGLCULLFACEPROC, glCullFace)                                       \
    X(PFNGLLINEWIDTHPROC, glLineWidth)                                     \
    X(PFNGLPOLYGONMODEPROC, glPolygonMode)                                 \
    X(PFNGLPOLYGONOFFSETPROC, glPolygonOffset)                             \
    X(PFNGLGETSTRINGPROC, glGetString)                                     \
    X(PFNGLGETERRORPROC, glGetError)                                       \
    X(PFNGLGETINTEGERVPROC, glGetIntegerv)                                 \
    X(PFNGLREADPIXELSPROC, glReadPixels)                                   \
    X(PFNGLPIXELSTOREIPROC, glPixelStorei)                                 \
    X(PFNGLDRAWBUFFERPROC, glDrawBuffer)                                   \
    X(PFNGLREADBUFFERPROC, glReadBuffer)                                   \
    /* shaders and programs */                                             \
    X(PFNGLCREATESHADERPROC, glCreateShader)                               \
    X(PFNGLSHADERSOURCEPROC, glShaderSource)                               \
    X(PFNGLCOMPILESHADERPROC, glCompileShader)                             \
    X(PFNGLGETSHADERIVPROC, glGetShaderiv)                                 \
    X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)                       \
    X(PFNGLDELETESHADERPROC, glDeleteShader)                               \
    X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                             \
    X(PFNGLATTACHSHADERPROC, glAttachShader)                               \
    X(PFNGLLINKPROGRAMPROC, glLinkProgram)                                 \
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                               \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                     \
    X(PFNGLUSEPROGRAMPROC, glUseProgram)                                   \
    X(PFNGLDELETEPROGRAMPROC, glDeleteProgram)                             \
    X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)                   \
    X(PFNGLUNIFORM1IPROC, glUniform1i)                                     \
    X(PFNGLUNIFORM1FPROC, glUniform1f)                                     \
    X(PFNGLUNIFORM3FPROC, glUniform3f)                                     \
    X(PFNGLUNIFORM2FPROC, glUniform2f)                                     \
    X(PFNGLUNIFORM3FVPROC, glUniform3fv)                                   \
    X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)                       \
    /* vertex data */                                                      \
    X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                         \
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                         \
    X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)                   \
    X(PFNGLGENBUFFERSPROC, glGenBuffers)                                   \
    X(PFNGLBINDBUFFERPROC, glBindBuffer)                                   \
    X(PFNGLBUFFERDATAPROC, glBufferData)                                   \
    X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                             \
    X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)                 \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)         \
    X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor)                 \
    X(PFNGLDRAWARRAYSPROC, glDrawArrays)                                   \
    X(PFNGLDRAWELEMENTSPROC, glDrawElements)                               \
    X(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced)             \
    /* textures */                                                         \
    X(PFNGLGENTEXTURESPROC, glGenTextures)                                 \
    X(PFNGLBINDTEXTUREPROC, glBindTexture)                                 \
    X(PFNGLTEXIMAGE2DPROC, glTexImage2D)                                   \
    X(PFNGLTEXPARAMETERIPROC, glTexParameteri)                             \
    X(PFNGLACTIVETEXTUREPROC, glActiveTexture)                             \
    X(PFNGLDELETETEXTURESPROC, glDeleteTextures)                           \
    X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap)                           \
    /* off-screen frame buffers */                                         \
    X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)                         \
    X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer)                         \
    X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)                   \
    X(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer)         \
    X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D)               \
    X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus)           \
    X(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers)                       \
    X(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer)                       \
    X(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers)                 \
    X(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glRenderbufferStorageMultisample) \
    X(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer)

// Declare one pointer per function: extern PFNGLCLEARPROC astroGl_glClear; ...
#define ASTRO_GL_DECLARE(type, name) extern type astroGl_##name;
ASTRO_GL_FUNCTIONS(ASTRO_GL_DECLARE)
#undef ASTRO_GL_DECLARE

// Standard names map onto our pointers.
#define glClear astroGl_glClear
#define glFinish astroGl_glFinish
#define glVertexAttribDivisor astroGl_glVertexAttribDivisor
#define glDrawElementsInstanced astroGl_glDrawElementsInstanced
#define glDrawBuffer astroGl_glDrawBuffer
#define glPolygonOffset astroGl_glPolygonOffset
#define glUniform2f astroGl_glUniform2f
#define glReadBuffer astroGl_glReadBuffer
#define glUniform3fv astroGl_glUniform3fv
#define glFramebufferTexture2D astroGl_glFramebufferTexture2D
#define glClearColor astroGl_glClearColor
#define glEnable astroGl_glEnable
#define glDisable astroGl_glDisable
#define glViewport astroGl_glViewport
#define glDepthFunc astroGl_glDepthFunc
#define glDepthMask astroGl_glDepthMask
#define glBlendFunc astroGl_glBlendFunc
#define glCullFace astroGl_glCullFace
#define glLineWidth astroGl_glLineWidth
#define glPolygonMode astroGl_glPolygonMode
#define glGetString astroGl_glGetString
#define glGetError astroGl_glGetError
#define glGetIntegerv astroGl_glGetIntegerv
#define glReadPixels astroGl_glReadPixels
#define glPixelStorei astroGl_glPixelStorei
#define glCreateShader astroGl_glCreateShader
#define glShaderSource astroGl_glShaderSource
#define glCompileShader astroGl_glCompileShader
#define glGetShaderiv astroGl_glGetShaderiv
#define glGetShaderInfoLog astroGl_glGetShaderInfoLog
#define glDeleteShader astroGl_glDeleteShader
#define glCreateProgram astroGl_glCreateProgram
#define glAttachShader astroGl_glAttachShader
#define glLinkProgram astroGl_glLinkProgram
#define glGetProgramiv astroGl_glGetProgramiv
#define glGetProgramInfoLog astroGl_glGetProgramInfoLog
#define glUseProgram astroGl_glUseProgram
#define glDeleteProgram astroGl_glDeleteProgram
#define glGetUniformLocation astroGl_glGetUniformLocation
#define glUniform1i astroGl_glUniform1i
#define glUniform1f astroGl_glUniform1f
#define glUniform3f astroGl_glUniform3f
#define glUniformMatrix4fv astroGl_glUniformMatrix4fv
#define glGenVertexArrays astroGl_glGenVertexArrays
#define glBindVertexArray astroGl_glBindVertexArray
#define glDeleteVertexArrays astroGl_glDeleteVertexArrays
#define glGenBuffers astroGl_glGenBuffers
#define glBindBuffer astroGl_glBindBuffer
#define glBufferData astroGl_glBufferData
#define glDeleteBuffers astroGl_glDeleteBuffers
#define glVertexAttribPointer astroGl_glVertexAttribPointer
#define glEnableVertexAttribArray astroGl_glEnableVertexAttribArray
#define glDrawArrays astroGl_glDrawArrays
#define glDrawElements astroGl_glDrawElements
#define glGenTextures astroGl_glGenTextures
#define glBindTexture astroGl_glBindTexture
#define glTexImage2D astroGl_glTexImage2D
#define glTexParameteri astroGl_glTexParameteri
#define glActiveTexture astroGl_glActiveTexture
#define glDeleteTextures astroGl_glDeleteTextures
#define glGenerateMipmap astroGl_glGenerateMipmap
#define glGenFramebuffers astroGl_glGenFramebuffers
#define glBindFramebuffer astroGl_glBindFramebuffer
#define glDeleteFramebuffers astroGl_glDeleteFramebuffers
#define glFramebufferRenderbuffer astroGl_glFramebufferRenderbuffer
#define glCheckFramebufferStatus astroGl_glCheckFramebufferStatus
#define glGenRenderbuffers astroGl_glGenRenderbuffers
#define glBindRenderbuffer astroGl_glBindRenderbuffer
#define glDeleteRenderbuffers astroGl_glDeleteRenderbuffers
#define glRenderbufferStorageMultisample astroGl_glRenderbufferStorageMultisample
#define glBlitFramebuffer astroGl_glBlitFramebuffer

/** @brief Signature of a "look up a function by name" routine, e.g. glfwGetProcAddress. */
using GlProcLookup = void* (*)(const char* name);

/**
 * @brief Fill every function pointer using the given lookup. Call once, after
 *        the OpenGL context has been made current.
 * @param missingOut Receives the names of any functions the driver did not provide.
 * @return true if every function was found.
 */
bool loadOpenGlFunctions(GlProcLookup lookup, std::string& missingOut);
