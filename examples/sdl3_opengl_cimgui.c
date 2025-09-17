// opengl
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cimgui.h>
#include <cimgui_impl.h>

#define igGetIO igGetIO_Nil

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "Failed to init video! %s\n", SDL_GetError());
        return 1;
    }

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_OPENGL;
    SDL_Window* window = SDL_CreateWindow("Dear cImGui SDL3+OpenGL3 example", (int)(1280 * main_scale), (int)(720 * main_scale), window_flags);
    if (!window) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Create OpenGL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_GL_MakeCurrent(window, gl_context);

    // Setup Dear ImGui context
    igCreateContext(NULL);
    ImGuiIO* io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    igStyleColorsDark(NULL);
    ImGuiStyle* style = igGetStyle();
    ImGuiStyle_ScaleAllSizes(style, main_scale);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = {0.45f, 0.55f, 0.60f, 1.00f};

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        // Demo window
        if (show_demo_window)
            igShowDemoWindow(&show_demo_window);

        // Simple window
        {
            static float f = 0.0f;
            static int counter = 0;
            igBegin("Hello, world!", NULL, 0);
            igText("This is some useful text.");
            igCheckbox("Demo Window", &show_demo_window);
            igCheckbox("Another Window", &show_another_window);
            igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
            igColorEdit4("clear color", (float*)&clear_color, 0);
            ImVec2 buttonSize = {0, 0};
            if (igButton("Button", buttonSize))
                counter++;
            igSameLine(0.0f, -1.0f);
            igText("counter = %d", counter);
            igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
            igEnd();
        }

        // Another window
        if (show_another_window) {
            igBegin("Another Window", &show_another_window, 0);
            igText("Hello from another window!");
            ImVec2 buttonSize = {0, 0};
            if (igButton("Close Me", buttonSize))
                show_another_window = false;
            igEnd();
        }

        // Rendering
        igRender();
        glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(NULL);
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}