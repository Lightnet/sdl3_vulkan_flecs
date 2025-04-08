

# Main test

```c
  while (!ctx->shouldQuit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (ctx->isImGuiInitialized) {
        //ecs_print(1,"imgui input");
        ImGui_ImplSDL3_ProcessEvent(&event);
      }
      if (event.type == SDL_EVENT_QUIT) {
        ecs_print(1, "Quit event received");
        ctx->shouldQuit = true;
      }
    }
    ecs_progress(world, 0);
  }
```


 * https://www.flecs.dev/flecs/group__c__addons__pipeline.html#ga46f3f33d48f252fcc510450aebe8c187
 
```c
#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Delta Time Example", 
                                          800, 600, 0);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    Uint64 previousTime = SDL_GetTicks(); // Time at the start
    SDL_bool running = SDL_TRUE;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = SDL_FALSE;
            }
        }

        // Get current time and calculate delta time
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - previousTime) / 1000.0f; // Convert ms to seconds
        previousTime = currentTime; // Update previous time for next frame

        // Print delta time
        printf("Delta Time: %f seconds\n", deltaTime);

        // Optional: Cap the frame rate to avoid flooding the console
        SDL_Delay(16); // Roughly 60 FPS (1000ms / 60 ≈ 16ms)
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

 * Precision: SDL_GetTicks() has millisecond precision. For higher precision, consider SDL_GetPerformanceCounter() and SDL_GetPerformanceFrequency() in SDL3, which use a high-resolution timer:
```c
Uint64 perfCounter = SDL_GetPerformanceCounter();
float deltaTime = (float)(perfCounter - previousPerfCounter) / SDL_GetPerformanceFrequency();
```