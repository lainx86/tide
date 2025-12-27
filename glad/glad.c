/*
 * GLAD - OpenGL Loader (stub implementation)
 *
 * See glad.h for notes on generating the real GLAD loader.
 */

#include "glad/glad.h"

int gladLoadGLLoader(GLADloadproc load) {
  (void)load; // Unused in stub

  // The stub relies on GL_GLEXT_PROTOTYPES being defined,
  // which makes GL functions available directly.
  // Real GLAD would load function pointers here.

  return 1; // Success
}
