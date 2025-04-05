#include <SDL3/SDL.h>
#include <stdio.h>
#include "flecs.h"
#include "flecs_types.h"
#include "flecs_vulkan.h"

#define WIDTH 800
#define HEIGHT 600

int main(int argc, char* argv[]) {
    printf("init main!\n");
    ecs_world_t *world = ecs_init();

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3 Flecs", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize phases
    flecs_phases_init(world, &GlobalPhases);

    // Initialize WorldContext
    WorldContext world_ctx = { 
        .window = window,
        .imageCount = 0,
        .currentImageIndex = 0,
        .physicalDevice = VK_NULL_HANDLE,
        .device = VK_NULL_HANDLE,
        .instance = VK_NULL_HANDLE,
        .surface = VK_NULL_HANDLE,
        .swapchain = VK_NULL_HANDLE,
        .renderPass = VK_NULL_HANDLE,
        .commandPool = VK_NULL_HANDLE,
        .commandBuffer = VK_NULL_HANDLE
    };

    // Initialize Vulkan module
    flecs_vulkan_module_init(world, &world_ctx);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }
        ecs_progress(world, 1.0f);
    }

    printf("clean up\n");
    cleanup_vulkan(&world_ctx);
    ecs_fini(world);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}