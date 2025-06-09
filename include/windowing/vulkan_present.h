#ifndef WINDOWING_VULKAN_PRESENT_H_
#define WINDOWING_VULKAN_PRESENT_H_

#ifdef _WIN64
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>
#include <glad/glad.h>
#include <common.h>

typedef enum {
  /* renders and presents ASAP works best with gsync and high fps */
  RENDER_MODE_IMMEDIATE,
  /* renders and waits for vsync -> works no matter what but high latency */
  RENDER_MODE_VSYNC,
  /* renders and presents with accurate timing */
  RENDER_MODE_FRAME_PACE_EXP
  /* (experimental might not work but should result in lowest latency when
     working)  */
} render_mode;
/* the fbos that wrap the textures that vulkan owns */
typedef struct {
  GLuint memory_object;
  GLuint texture_handle;
  GLuint fbo_handle;
  GLuint depth_rbo_handle;
} shared_fbo;
/* a "ringbuffer of shared textures/fbos" */
typedef struct {
  shared_fbo  *shared_fbos;
  usize        count_fbos;
  usize        current_index;
  /* vulkan internal semaphores */
  // signaled by GL, waited by Vk  [image_count] malloc
  VkSemaphore *vk_wait_semaphores;
  // signaled by Vk, waited by GL  [image_count] malloc
  VkSemaphore *vk_signal_semaphores;
  // signaled by Vk, waited by Preseting engine  [image_count] malloc
  VkSemaphore *vk_present_semaphores;
  //
  VkSemaphore *vk_acquire_semaphores;
  /* dummy gl names for the shared semaphores that reference the shared ones */
  GLuint      *gl_wait_semaphores;   // [image_count] malloc
  GLuint      *gl_signal_semaphores; // [image_count] malloc
  /* external win32 handles for the shared state */
  HANDLE      *wait_handles;
  HANDLE      *sig_handles;
} vk_sc_ringbuf;
/*
vulkan data for the main client window
this context is bound to a specific framebuffer resolution.
when the resolution changes or the window gets resized
this needs to be initialized from scratch again
*/
typedef struct {
  /* a ringbuffer of image_buffer handles that OpenGL can write to */
  vk_sc_ringbuf      swapchain;
  /* vulkan data */
  VkInstance         vk_instance;
  VkDevice           vk_device;
  VkSurfaceKHR       vk_surface;
  VkSwapchainKHR     vk_swapchain;
  VkQueue            vk_queue;
  u32                vk_queue_family;
  /* more vulkan data */
  VkCommandPool      cmd_pool;
  VkCommandBuffer   *cmd_buffers;      // [image_count] malloc
  VkFence           *inflight_fences;  // [image_count] malloc
  VkImage           *images;           // [image_count] malloc
  VkImage           *swapchain_images; // [image_count] malloc
  VkDeviceMemory    *image_memories;   // [image_count] malloc
  /* the properties of the vulkan surface/images and opengl fbos/textures */
  usize              texture_width, texture_height;
  VkSurfaceFormatKHR surface_format;
  bool               initialized;
#ifdef GAME_DEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vk_context;

// clang-format off

u0      initialize_vulkan_context   (vk_context *_context, HWND _window_handle, usize _screen_w, usize _screen_h, u32 _rmode);
usize   bind_vulkan_surface         (vk_context *ctx);
u0      vulkan_present              (vk_context *ctx);
u0      destroy_vulkan_context      (vk_context *_context);

#else
typedef enum {
  /* renders and presents ASAP works best with gsync and high fps */
  RENDER_MODE_IMMEDIATE,
  /* renders and waits for vsync -> works no matter what but high latency */
  RENDER_MODE_VSYNC,
  /* renders and presents with accurate timing */
  RENDER_MODE_FRAME_PACE_EXP
  /* (experimental might not work but should result in lowest latency when
     working)  */
} render_mode;
#endif
// clang-format on
#endif // WINDOWING_VULKAN_PRESENT_H_
