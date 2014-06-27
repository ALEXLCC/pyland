#include <iostream>
#include <map>

extern "C" {
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include <X11/Xlib.h>

#include <bcm_host.h>
}

#include "game_window.hpp"



#ifdef DEBUG
#define GAME_WINDOW_DEBUG
#endif



///
/// Mapping of SDL window IDs to GameWindows.
///
static std::map<Uint32,GameWindow*> windows = std::map<Uint32,GameWindow*>();



GameWindow::InitException::InitException(const char* message) {
    this->message = message;
}


const char* GameWindow::InitException::what() {
    return message;
}



GameWindow::GameWindow(int width, int height, bool fullscreen) {
    visible = false;
    close_requested = false;
    
    if (windows.size() == 0) {
        init_sdl(); // May throw InitException
    }

    // SDL already uses width,height = 0,0 for automatic
    // resolution. Sets maximized if not in fullscreen and given
    // width,height = 0,0.
    window = SDL_CreateWindow ("Project Zygote",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               width,
                               height,
                               (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE)
                               | ( (!fullscreen && width == 0 && height == 0) ?
                                   SDL_WINDOW_MAXIMIZED : 0 )
                               );
    if (window == NULL) {
#ifdef GAME_WINDOW_DEBUG
        std::cerr << "Failed to create SDL window." << std::endl;
#endif
        deinit_sdl();
        throw GameWindow::InitException("Failed to create SDL window");
    }

    SDL_GetWindowWMInfo(window, &wm_info);
    
    dispmanDisplay = vc_dispmanx_display_open(0); // (???)

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    
    try {
        init_gl();
    }
    catch (InitException e) {
        vc_dispmanx_display_close(dispmanDisplay); // (???)
        SDL_DestroyRenderer (renderer);
        SDL_DestroyWindow (window);
        if (windows.size() == 0) {
            deinit_sdl();
        }
        throw e;
    }

    windows[SDL_GetWindowID(window)] = this;
}

GameWindow::~GameWindow() {
    deinit_gl();
    
    vc_dispmanx_display_close(dispmanDisplay); // (???)
    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);
    
    // window_count--;
    windows.erase(SDL_GetWindowID(window));
    if (windows.size() == 0) {
        deinit_sdl();
    }
}


void GameWindow::init_sdl() {
    int result;
    
#ifdef GAME_WINDOW_DEBUG
    std::cerr << "Initializing SDL..." << std::endl;
#endif
    
    result = SDL_Init (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  
    if (result != 0) {
        throw GameWindow::InitException("Failed to initialize SDL");
    }

    SDL_VERSION(&wm_info.version);
    
#ifdef GAME_WINDOW_DEBUG
    std::cerr << "SDL initialized." << std::endl;
#endif
}


void GameWindow::deinit_sdl() {
#ifdef GAME_WINDOW_DEBUG
    std::cerr << "Deinitializing SDL..." << std::endl;
#endif
    
    // Should always work.
    SDL_Quit ();
    
#ifdef GAME_WINDOW_DEBUG
    std::cerr << "SDL deinitialized." << std::endl;
#endif
}


void GameWindow::init_gl() {
    EGLBoolean result;

    static const EGLint attribute_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, /* (???) */
        EGL_NONE
    };

    static const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    // Get an EGL display connection

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
#ifdef GAME_WINDOW_DEBUG
        std::cerr << "Error getting display." << std::endl;
#endif
        throw GameWindow::InitException("Error getting display");
    }

    // Initialize EGL display connection
    result = eglInitialize(display, NULL, NULL);
    if (result == EGL_FALSE) {
        eglTerminate(display);
#ifdef GAME_WINDOW_DEBUG
        std::cerr << "Error initializing display connection." << std::endl;
#endif
        throw GameWindow::InitException("Error initializing display connection");
    }

    // Get frame buffer configuration
    result = eglChooseConfig(display, attribute_list, &config, 1, &configCount);
    if (result == EGL_FALSE) {
        eglTerminate(display);
#ifdef GAME_WINDOW_DEBUG
        std::cerr << "Error getting frame buffer configuration." << std::endl;
#endif
        throw GameWindow::InitException("Error getting frame buffer configuration");
    }

    //Should I use eglBindAPI? It is auomatically ES anyway.

    // Create EGL rendering context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    if (context == EGL_NO_CONTEXT) {
    eglTerminate(display);
#ifdef GAME_WINDOW_DEBUG
        std::cerr << "Error creating rendering context." << std::endl;
#endif
        throw GameWindow::InitException("Error creating rendering context");
    }

    // Surface initialization is done here as it can be called multiple
    // times after  main initialization.
    init_surface();
}


void GameWindow::deinit_gl() {
    // Release EGL resources
    deinit_surface();
    eglDestroyContext(display, context);
    eglTerminate(display);
}


void GameWindow::init_surface() {
    int x, y, w, h;

    // It turns out that SDL's window position information is not good
    // enough, as it reports for the window border, not the rendering
    // area. For the time being, we shall be using LibX11 to query the
    // window's position.
    
    // child is just a place to put something. We don't need it.
    Window child;
    XTranslateCoordinates(wm_info.info.x11.display,
                          wm_info.info.x11.window,
                          XDefaultRootWindow(wm_info.info.x11.display),
                          0,
                          0,
                          &x,
                          &y,
                          &child);
    // SDL_GetWindowPosition(window, &x, &y);
    SDL_GetWindowSize(window, &w, &h);
    
    init_surface(x, y, w, h);
}


void GameWindow::init_surface(int x, int y, int w, int h) {
    EGLBoolean result;
  
    VC_RECT_T destination;
    VC_RECT_T source;
  
    static EGL_DISPMANX_WINDOW_T nativeWindow;

#ifdef GAME_WINDOW_DEBUG
    std::cerr << "Initializing window surface." << std::endl;
#endif
  
    // Create EGL window surface.

    destination.x = x;
    destination.y = y;
    destination.width = w;
    destination.height = h;
#ifdef GAME_WINDOW_DEBUG
    std::cerr << "New surface: " << w << "x" << h << " at (" << x << "," << y <<")." << std::endl;;
#endif
    source.x = 0;
    source.y = 0;
    source.width  = w << 16; // (???)
    source.height = h << 16; // (???)

    deinit_surface();

    DISPMANX_UPDATE_HANDLE_T dispmanUpdate;
    dispmanUpdate  = vc_dispmanx_update_start(0); // (???)

    dispmanElement = vc_dispmanx_element_add(dispmanUpdate, dispmanDisplay,
                                             0/*layer*/, &destination, 0/*src*/,
                                             &source, DISPMANX_PROTECTION_NONE,
                                             0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/); // (???)

    nativeWindow.element = dispmanElement;
    nativeWindow.width = w; // (???)
    nativeWindow.height = h; // (???)
    vc_dispmanx_update_submit_sync(dispmanUpdate); // (???)
    
    EGLSurface new_surface;
    new_surface = eglCreateWindowSurface(display, config, &nativeWindow, NULL);
    if (new_surface == EGL_NO_SURFACE) {
#ifdef GAME_WINDOW_DEBUG
        std::cerr << "Error creating window surface. " << eglGetError() << std::endl;
#endif
        throw GameWindow::InitException("Error creating window surface");
    }
    surface = new_surface;

    // Connect the context to the surface.
    result = eglMakeCurrent(display, surface, surface, context);
    if (result == EGL_FALSE) {
#ifdef GAME_WINDOW_DEBUG
        std::cerr << "Error connecting context to surface." << std::endl;
#endif
        throw GameWindow::InitException("Error connecting context to surface");
    }

    visible = true;
    change_surface = InitAction::DO_NOTHING;
    // Only set these if the init was successful.
    window_x = x;
    window_y = y;

    // Clean up any garbage in the SDL window.
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}


void GameWindow::deinit_surface() {
    if (visible) {
        int result;
        
        DISPMANX_UPDATE_HANDLE_T dispmanUpdate;
        dispmanUpdate  = vc_dispmanx_update_start(0); // (???)
        vc_dispmanx_element_remove (dispmanUpdate, dispmanElement);
        vc_dispmanx_update_submit_sync(dispmanUpdate); // (???)
        
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        result = eglDestroySurface(display, surface);
        if (result == EGL_FALSE) {
#ifdef GAME_WINDOW_DEBUG
            std::cerr << "Error destroying window surface." << std::endl;
#endif
            throw GameWindow::InitException("Error destroying window surface");
        }
    }
    visible = false;
    change_surface = InitAction::DO_NOTHING;
}


void GameWindow::update() {
    SDL_Event event;
    bool close_all = false;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: // Primarily used for killing when we become blind.
            close_all = true;
            break;
        case SDL_WINDOWEVENT:
            GameWindow* window = windows[event.window.windowID];

            // Instead of reinitialising on every event, do it ater we have
            // scanned the event queue in full.
            // Should focus events be included?
            switch (event.window.event) {
            case SDL_WINDOWEVENT_CLOSE:
                window->request_close();
                break;
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_MOVED:
            case SDL_WINDOWEVENT_RESTORED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                window->change_surface = InitAction::DO_INIT;
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
            case SDL_WINDOWEVENT_MINIMIZED:
            case SDL_WINDOWEVENT_HIDDEN:
                window->change_surface = InitAction::DO_DEINIT;
                break;
            }
            break;
        }
    }

    // For each open window, do any surface (re/de)initialisation.
    // Also set (if needed) the close_requests.
    for (auto pair : windows) {
        GameWindow* window = pair.second;
        
        // Hacky fix: The events don't quite chronologically work, so
        // check the window position to start any needed surface update.
        int x, y;
        Window child;
        XTranslateCoordinates(window->wm_info.info.x11.display,
                              window->wm_info.info.x11.window,
                              XDefaultRootWindow(window->wm_info.info.x11.display),
                              0,
                              0,
                              &x,
                              &y,
                              &child);
        if ((window->window_x != x || window->window_y != y) && window->visible) {
#ifdef GAME_WINDOW_DEBUG
            std::cerr << "Need surface reinit." << std::endl;
#endif
            window->change_surface = InitAction::DO_INIT;
        }
        
        if (close_all) {
            window->request_close();
        }
        
        switch (window->change_surface) {
        case InitAction::DO_INIT:
            try {
                window->init_surface();
            }
            catch (InitException e) {
                std::cerr << "Surface reinit failed: " << e.what() << std::endl;
            }
            break;
        case InitAction::DO_DEINIT:
            window->deinit_surface();
            break;
        case InitAction::DO_NOTHING:
            // Do nothing - hey, I don't like compiler warnings.
            break;
        }
    }
}


void GameWindow::request_close() {
    close_requested = true;
}


void GameWindow::cancel_close() {
    close_requested = false;
}


bool GameWindow::check_close() {
    return close_requested;
}


void GameWindow::use_context() {
    if (visible) {
        eglMakeCurrent(display, surface, surface, context);
    }
}


void GameWindow::swap_buffers() {
    eglSwapBuffers(display, surface);
}
