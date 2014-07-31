#ifndef GRAPHICS_CONTEXT_H
#define GRAPHICS_CONTEXT_H

extern "C" {
#ifdef USE_GL
#include <SDL2/SDL.h>
#endif

#ifdef USE_GLES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif
}



class GameWindow;



///
/// Used to unambiguously distinguish between GL contexts.
///
/// The implementation is simply a wrapper around GameWindow which hides
/// the user from the internals of graphics management.
///
class GraphicsContext {
private:
    friend class GameWindow;
    
    ///
    /// The window with the one-to-one mapping with the GL context.
    ///
    GameWindow* window;
    
    GraphicsContext(GameWindow* window);
public:
    ///
    /// Return true if the contexts use the same GL context.
    ///
    bool operator==(GraphicsContext other);

    ///
    /// Start using this GL context.
    ///
    void use();
};



#endif
