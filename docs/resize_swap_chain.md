# Documentation: Swapchain Recreation System in Vulkan with Flecs

# Overview

This code manages the recreation of a Vulkan swapchain, which is necessary when the window is resized. The swapchain is a core Vulkan component responsible for presenting rendered images to the screen. The system uses Flecs to integrate this functionality into an ECS-based architecture, ensuring modularity and reusability.

# Key Components and Concepts

1. Swapchain: A Vulkan object (VkSwapchainKHR) that manages a queue of images to be presented to the display. It needs recreation when the window size changes.
2. Framebuffers (VkFramebuffer): Attachments that hold the render targets (e.g., swapchain images) for rendering.
3. Swapchain Image Views (VkImageView): Views into the swapchain images, allowing Vulkan to access them for rendering.
4. Swapchain Images (VkImage): The actual images managed by the swapchain, used as render targets.
5. VkSurfaceCapabilitiesKHR: Describes the properties of the surface (e.g., supported sizes) that the swapchain will present to.
6. VkSurfaceFormatKHR: Defines the pixel format and color space of the swapchain images.
7. VkPresentModeKHR: Specifies how images are presented to the screen (e.g., FIFO for V-Sync).
8. VkSwapchainCreateInfoKHR: A structure used to configure and create the swapchain.
9. VkImageViewCreateInfo: A structure to create image views for swapchain images.
10. VkFramebufferCreateInfo: A structure to create framebuffers tied to the swapchain image views.
11. needsSwapchainRecreation: A flag in WorldContext indicating that the swapchain must be recreated (e.g., after a resize event).
12. Flecs Integration: The SwapchainRecreationSystem is a Flecs system that monitors the needsSwapchainRecreation flag and triggers recreation.

---

Step-by-Step Explanation

1. cleanupSwapchain(WorldContext *ctx)

This function cleans up the existing swapchain and its associated resources before recreation.

- Steps:
    1. Wait for Device Idle: vkDeviceWaitIdle(ctx->device) ensures all GPU operations are complete before cleanup.
    2. Destroy Framebuffers: Loops through ctx->framebuffers and destroys each one using vkDestroyFramebuffer if it exists.
    3. Destroy Image Views: Loops through ctx->swapchainImageViews and destroys each one using vkDestroyImageView if it exists.
    4. Free Memory: Frees the dynamically allocated arrays ctx->framebuffers, ctx->swapchainImageViews, and ctx->swapchainImages.
    5. Destroy Swapchain: Destroys the swapchain itself with vkDestroySwapchainKHR.
    6. Reset Pointers: Sets framebuffers, swapchainImageViews, swapchainImages, and swapchain to NULL or VK_NULL_HANDLE.
- Purpose: Ensures no resources are leaked and prepares the context for a fresh swapchain.
    

---

2. recreateSwapchain(WorldContext *ctx)

This function recreates the swapchain and its dependencies based on the new window size.

- Steps:
    1. Early Exit Check: Returns if ctx->device or ctx->surface is invalid.
    2. Cleanup Old Swapchain: Calls cleanupSwapchain to destroy the old resources.
    3. Query Surface Capabilities: Uses vkGetPhysicalDeviceSurfaceCapabilitiesKHR to get the surface's supported properties (e.g., min/max extents).
        - If this fails, sets an error state (ctx->hasError and ctx->errorMessage).
    4. Update Swapchain Extent: Sets ctx->swapchainExtent to the new window size (ctx->width, ctx->height).
        - Clamps the extent to the min/max values from capabilities to ensure compatibility.
    5. Select Surface Format: Queries available formats with vkGetPhysicalDeviceSurfaceFormatsKHR and prefers VK_FORMAT_B8G8R8A8_SRGB with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR.
    6. Select Present Mode: Queries available present modes with vkGetPhysicalDeviceSurfacePresentModesKHR and selects VK_PRESENT_MODE_FIFO_KHR (V-Sync).
    7. Configure Swapchain: Fills VkSwapchainCreateInfoKHR with:
        - Surface, image count, format, extent, usage, sharing mode, transform, alpha, present mode, and clipping settings.
    8. Create Swapchain: Calls vkCreateSwapchainKHR to create the new swapchain.
        - If this fails, sets an error state.
    9. Get Swapchain Images: Retrieves the new images with vkGetSwapchainImagesKHR and allocates ctx->swapchainImages.
    10. Create Image Views: Loops through the images, creating a VkImageView for each with vkCreateImageView.
        - Uses VkImageViewCreateInfo with 2D view type, selected format, and color aspect.
        - If this fails, sets an error state.
    11. Create Framebuffers: Loops through the image views, creating a VkFramebuffer for each with vkCreateFramebuffer.
        - Uses VkFramebufferCreateInfo with the render pass, attachment (image view), extent, and layers.
        - If this fails, sets an error state.
    12. Update ImGui (if applicable): Updates the ImGui display size if initialized.
    13. Finalize: Clears the needsSwapchainRecreation flag and logs success.
- Purpose: Rebuilds the swapchain and its dependencies to match the new window size.
    

---

3. SwapchainRecreationSystem(ecs_iter_t *it)

This Flecs system integrates the swapchain recreation into the ECS framework.

- Steps:
    1. Get Context: Retrieves the WorldContext from the ECS world.
    2. Check State: Returns if ctx is null or an error has occurred.
    3. Trigger Recreation: If ctx->needsSwapchainRecreation is true, calls recreateSwapchain.
- Purpose: Monitors the need for swapchain recreation and executes it as part of the ECS update loop.

---

# Flowchart Diagram

Below is a textual representation of the flowchart. For a visual diagram, you can use tools like Mermaid.js or draw it manually based on this structure.

```text
[Start]
   |
   v
[SwapchainRecreationSystem]
   | ctx->needsSwapchainRecreation?
   | Yes                No
   v                   |
[recreateSwapchain]    |
   |                   |
   v                   |
[cleanupSwapchain]     |
   | Destroy framebuffers, image views, swapchain
   v                   |
[Query Surface Capabilities]
   | Success?          |
   | Yes              No
   v                  |
[Update Swapchain Extent]
   | Clamp to min/max |
   v                  |
[Select Format & Present Mode]
   v                  |
[Create Swapchain]     |
   | Success?          |
   | Yes              No
   v                  |
[Get Swapchain Images] |
   v                  |
[Create Image Views]   |
   | Success?          |
   | Yes              No
   v                  |
[Create Framebuffers]  |
   | Success?          |
   | Yes              No
   v                  |
[Update ImGui (if applicable)]
   v                  |
[Clear needsSwapchainRecreation]
   v                  |
[End]                 |
   |                  |
   v                  v
[Set Error State]---->[End]
```

---

# Detailed Explanation of Vulkan Components

1. Framebuffers: Represent a collection of attachments (e.g., swapchain images) used during rendering. Recreated here to match the new swapchain extent.
2. Swapchain Image Views: Provide a view into swapchain images, specifying how Vulkan interprets them (e.g., 2D, color).
3. Swapchain Images: The raw images managed by the swapchain, allocated by Vulkan and retrieved after swapchain creation.
4. VkSurfaceCapabilitiesKHR: Defines constraints like min/max image extent, ensuring the swapchain fits the surface.
5. VkSurfaceFormatKHR: Ensures the swapchain uses a compatible pixel format and color space (e.g., sRGB for accurate colors).
6. VkPresentModeKHR: Controls presentation timing (e.g., FIFO for smooth V-Sync).
7. VkSwapchainCreateInfoKHR: Configures all swapchain properties in one structure for creation.
8. VkImageViewCreateInfo: Configures how swapchain images are viewed (e.g., 2D, color aspect).
9. VkFramebufferCreateInfo: Ties image views to a render pass for rendering.
10. needsSwapchainRecreation: A flag set externally (e.g., by a window resize callback) to trigger this system.

---

# How It Fits Together
- Vulkan: Provides the low-level graphics API for swapchain management.
- Flecs: Wraps the recreation logic in a system, making it event-driven and modular.
- Window Resize: The external trigger (not shown) sets needsSwapchainRecreation, which the ECS system detects and handles.

This design is reusable: you can extend it by adding more systems (e.g., for pipeline recreation) or modifying WorldContext for additional state.