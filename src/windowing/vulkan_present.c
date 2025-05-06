#include <windowing/vulkan_present.h>
#include <glad/glad.h>

#define CHECK_VK(result, msg)                                                                                          \
  if (result != VK_SUCCESS) {                                                                                          \
    GAME_LOGF("%s (code %d)\n", msg, result);                                                                          \
    exit(1);                                                                                                           \
  }

// clang-format off
static VkSemaphore create_exportable_semaphore(VkDevice _device) {
  VkSemaphoreCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 
    .pNext = &(VkSemaphoreTypeCreateInfo){
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
      .initialValue = 0,
      .pNext = &(VkExportSemaphoreCreateInfo){
        .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
      }
    }
  };
  VkSemaphore semaphore;
  vkCreateSemaphore(_device, &info, NULL, &semaphore);
  return semaphore;
}

static u0 initialize_ringbuffer_sync(vk_context *ctx) {
  ctx->swapchain.current_index = 0;
  ctx->swapchain.vk_wait_semaphores     = malloc(sizeof(VkSemaphore) * ctx->swapchain.count_fbos);
  ctx->swapchain.vk_signal_semaphores   = malloc(sizeof(VkSemaphore) * ctx->swapchain.count_fbos);
  ctx->swapchain.gl_wait_semaphores     = malloc(sizeof(GLuint) * ctx->swapchain.count_fbos);
  ctx->swapchain.gl_signal_semaphores   = malloc(sizeof(GLuint) * ctx->swapchain.count_fbos);

  glGenSemaphoresEXT(ctx->swapchain.count_fbos, ctx->swapchain.gl_wait_semaphores);
  glGenSemaphoresEXT(ctx->swapchain.count_fbos, ctx->swapchain.gl_signal_semaphores);

  for (usize i = 0; i < ctx->swapchain.count_fbos; i++) {
    ctx->swapchain.vk_wait_semaphores[i] = create_exportable_semaphore(ctx->vk_device);
    ctx->swapchain.vk_signal_semaphores[i] = create_exportable_semaphore(ctx->vk_device);

    VkSemaphoreGetWin32HandleInfoKHR handleInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
      .semaphore = ctx->swapchain.vk_wait_semaphores[i],
      .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
    };
    
    PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR =
    (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(ctx->vk_device, "vkGetSemaphoreWin32HandleKHR");
    
    if (!vkGetSemaphoreWin32HandleKHR) {
      GAME_LOGF("failed to load vkGetSemaphoreWin32HandleKHR");
      exit(1);
    } else {
      GAME_LOGF("successfully loaded vkGetSemaphoreWin32HandleKHR!");
    }
    
    HANDLE wait_handle, sig_handle;
    vkGetSemaphoreWin32HandleKHR(ctx->vk_device, &handleInfo, &wait_handle);
    handleInfo.semaphore = ctx->swapchain.vk_signal_semaphores[i];
    vkGetSemaphoreWin32HandleKHR(ctx->vk_device, &handleInfo, &sig_handle);

    glImportSemaphoreWin32HandleEXT(ctx->swapchain.gl_wait_semaphores[i], GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, wait_handle);
    glImportSemaphoreWin32HandleEXT(ctx->swapchain.gl_signal_semaphores[i], GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, sig_handle);
  }
}

static VkPresentModeKHR get_viable_present_mode(VkPhysicalDevice *_phys_dev, VkSurfaceKHR *_surface) {
  u32 present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(*_phys_dev, *_surface, &present_mode_count, NULL);
  if (!present_mode_count) {
    GAME_LOGF("no available present modes!");
    exit(1);
  }
  VkPresentModeKHR *present_modes = malloc(sizeof(present_mode_count) * present_mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(*_phys_dev, *_surface, &present_mode_count, present_modes);
  for (u32 i = 0; i != present_mode_count; i++) {
    /* we prefer immediate (no waiting) */
    if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      free(present_modes);
      return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
  }
  /* secondly check for mailbox as it is still better than FIFO */
  for (u32 i = 0; i != present_mode_count; i++) {
    /* we prefer immediate */
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      free(present_modes);
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }
  free(present_modes);
  GAME_LOGF("using fallback present mode!");
  /* fifo is a safe default */
  return VK_PRESENT_MODE_FIFO_KHR;
}

static u32 find_mem_type(u32 typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  GAME_LOGF("Failed to find suitable memory type!\n");
  exit(1);
}

static VkSurfaceFormatKHR get_viable_surface_format(VkPhysicalDevice *_phys_dev, VkSurfaceKHR *_surface) {
  u32 format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(*_phys_dev, *_surface, &format_count, NULL);
  if (!format_count) {
    GAME_LOGF("no available surface modes!");
    exit(1);
  }
  VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(*_phys_dev, *_surface, &format_count, formats);

  for (u32 i = 0; i != format_count; i++) {
    if (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
      free(formats);
      return formats[i];
    }
    if (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
      free(formats);
      return formats[i];
    }
  }
  /* safe default */
  free(formats);
  GAME_LOGF("using fallback surface format mode!");
  return formats[0];
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_printer(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT types,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData) {
  GAME_LOGF("Validation %s: %s\n\n",
            (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)     ? "ERROR"
            : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARNING"
                                                                           : "INFO",
            pCallbackData->pMessage);
  return VK_FALSE;
}

u0 initialize_vulkan_context(vk_context *_context, HWND _window_handle, usize _screen_w, usize _screen_h) {
  memset(_context, 0, sizeof *_context);

  {
    u32 lc = 0;
    vkEnumerateInstanceLayerProperties(&lc, NULL);
    VkLayerProperties *available_layers = malloc(sizeof(VkLayerProperties) * lc);
    vkEnumerateInstanceLayerProperties(&lc, available_layers);
    bool av = false;
    for (u32 i = 0; i < lc; i++) {
      if (strcmp(available_layers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
        av = true;
        break;
      }
    }
    free(available_layers);
    if (!av) {
      GAME_LOGF("Validation layer VK_LAYER_KHRONOS_validation not available");
      exit(1);
    }
  }

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = NULL,
      .messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_printer,
      .pUserData = NULL
  };

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "game_engine_client",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "None",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_1,
  };

  const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
  const char *instance_extensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
  };

  VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = 3,
      .enabledLayerCount = 1,
      .ppEnabledLayerNames = layers,
      .ppEnabledExtensionNames = instance_extensions,
      .pNext = &debug_create_info
  };

  VkInstance instance;
  CHECK_VK(vkCreateInstance(&instance_info, NULL, &instance), "failed to create Vulkan instance");

  u32 gpu_count = 0;
  vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
  if (gpu_count == 0) {
    GAME_LOGF("no vulkan-capable GPU found");
    return;
  }

  VkDebugUtilsMessengerEXT debug_messenger;
  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (vkCreateDebugUtilsMessengerEXT != NULL) {
    vkCreateDebugUtilsMessengerEXT(instance, &debug_create_info, NULL, &debug_messenger);
  }

  VkPhysicalDevice phys_dev;
  vkEnumeratePhysicalDevices(instance, &gpu_count, &phys_dev);

  f32 queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = 0,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
  };

  const char *device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
  };

  VkDeviceCreateInfo deviceInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queue_info,
      .enabledExtensionCount = 4,
      .ppEnabledExtensionNames = device_extensions,
  };

  VkDevice device;
  CHECK_VK(vkCreateDevice(phys_dev, &deviceInfo, NULL, &device), "failed to create logical device");

  vkGetDeviceQueue(device,
                   0, // no family TODO: is 0 a safe default here?
                   0, &_context->vk_queue);

  PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR = 
      (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");

  if (!vkGetMemoryWin32HandleKHR) {
    GAME_LOGF("failed to load vkGetMemoryWin32HandleKHR");
    exit(1);
  } else {
    GAME_LOGF("successfully loaded vkGetMemoryWin32HandleKHR!");
  }

  _context->vk_device = device;
  _context->vk_instance = instance;

  VkSurfaceKHR surface;
  {
    VkWin32SurfaceCreateInfoKHR surf_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(NULL),
        .hwnd = _window_handle,
    };
    CHECK_VK(vkCreateWin32SurfaceKHR(instance, &surf_info, NULL, &surface), "vkCreateWin32SurfaceKHR failed");
  }

  VkSurfaceCapabilitiesKHR surface_caps;
  CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &surface_caps),
           "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

  _screen_w = max(surface_caps.minImageExtent.width, min(_screen_w, surface_caps.maxImageExtent.width));
  _screen_h = max(surface_caps.minImageExtent.height, min(_screen_h, surface_caps.maxImageExtent.height));

  _context->texture_width = _screen_w;
  _context->texture_height = _screen_h;

  GAME_LOGF("new vulkan present screen size: (%d, %d)", _screen_w, _screen_h);

  if (!surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    GAME_LOGF("VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR not supported");
    exit(1);
  }

  GAME_LOGF("min image count: %d", surface_caps.minImageCount);
  GAME_LOGF("current transform: %d", surface_caps.currentTransform);

  const u32 swapchain_length = min(max(3, surface_caps.minImageCount), surface_caps.maxImageCount);

  GAME_LOGF("swapchain length: %d", swapchain_length);

  const VkSurfaceFormatKHR format = get_viable_surface_format(&phys_dev, &surface);
  const VkPresentModeKHR present_mode = get_viable_present_mode(&phys_dev, &surface);

  _context->surface_format = format;

  VkSwapchainCreateInfoKHR sci = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = swapchain_length,
      .imageFormat = format.format,
      .imageColorSpace = format.colorSpace,
      .imageExtent = {_screen_w, _screen_h},
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = present_mode,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
      .pNext = NULL,
  };

  VkSwapchainKHR swapchain;
  CHECK_VK(vkCreateSwapchainKHR(device, &sci, NULL, &swapchain), "failed to create swapchain");

  _context->vk_surface = surface;
  _context->vk_swapchain = swapchain;

  u32 image_count = 0;
  vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
  _context->swapchain.count_fbos = image_count;
  _context->swapchain.shared_fbos = malloc(sizeof(shared_fbo) * image_count);

  _context->images = malloc(sizeof(VkImage) * image_count);
  _context->swapchain_images = malloc(sizeof(VkImage) * image_count);
  vkGetSwapchainImagesKHR(device, swapchain, &image_count, _context->swapchain_images);

  for (u32 i = 0; i < image_count; i++) {
    VkExternalMemoryImageCreateInfo extImgInfo = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
    };
    VkImageCreateInfo img_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &extImgInfo,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format.format,
        .extent = {_screen_w, _screen_h, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    vkCreateImage(device, &img_info, NULL, &_context->images[i]);

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, _context->images[i], &req);
    VkExportMemoryAllocateInfo exportAlloc = {
        .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
    };
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &exportAlloc,
        .allocationSize = req.size,
        .memoryTypeIndex = find_mem_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, phys_dev),
    };

    VkDeviceMemory mem;
    vkAllocateMemory(device, &alloc_info, NULL, &mem);
    vkBindImageMemory(device, _context->images[i], mem, 0);

    VkMemoryGetWin32HandleInfoKHR handle_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
        .memory = mem,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
    };
    
    HANDLE win_handle;
    vkGetMemoryWin32HandleKHR(device, &handle_info, &win_handle);

    shared_fbo sfbo = {};
    glCreateMemoryObjectsEXT(1, &sfbo.memory_object);
    glImportMemoryWin32HandleEXT(sfbo.memory_object, (GLsizeiptr)req.size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, win_handle);

    GLenum gl_format;
    switch (format.format) {
    case VK_FORMAT_B8G8R8A8_SRGB: gl_format = GL_SRGB8_ALPHA8; break;
    case VK_FORMAT_R8G8B8A8_SRGB: gl_format = GL_RGBA8; break;
    default: GAME_LOGF("Unsupported format"); exit(1);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &sfbo.texture_handle);
    glTextureStorageMem2DEXT(sfbo.texture_handle, 1, gl_format, _screen_w, _screen_h, sfbo.memory_object, 0);
    glCreateFramebuffers(1, &sfbo.fbo_handle);
    glNamedFramebufferTexture(sfbo.fbo_handle, GL_COLOR_ATTACHMENT0, sfbo.texture_handle, 0);
    GLenum status = glCheckNamedFramebufferStatus(sfbo.fbo_handle, GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) { GAME_LOGF("FBO incomplete: %u", status); }
    _context->swapchain.shared_fbos[i] = sfbo;
  }

  VkCommandPoolCreateInfo poolInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = _context->vk_queue_family,
  };
  vkCreateCommandPool(_context->vk_device, &poolInfo, NULL, &_context->cmd_pool);

  _context->cmd_buffers = malloc(sizeof(VkCommandBuffer) * image_count);
  _context->inflight_fences = malloc(sizeof(VkFence) * image_count);

  VkCommandBufferAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = _context->cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = image_count,
  };
  vkAllocateCommandBuffers(_context->vk_device, &allocInfo, _context->cmd_buffers);

  for (uint32_t i = 0; i < image_count; i++) {
    VkFenceCreateInfo fenceInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    vkCreateFence(_context->vk_device, &fenceInfo, NULL, &_context->inflight_fences[i]);
  }

  initialize_ringbuffer_sync(_context);
  GAME_LOGF("successfully initialized vulkan surface");
}

u0 bind_vulkan_surface(vk_context *ctx) {
  vk_sc_ringbuf *rb = &ctx->swapchain;
  rb->current_index = (rb->current_index + 1) % rb->count_fbos;
  shared_fbo *sf = &rb->shared_fbos[rb->current_index];

  glWaitSemaphoreEXT(
    rb->gl_signal_semaphores[rb->current_index], 
    0, NULL, 1, 
    (GLuint[]){sf->texture_handle}, 
    (GLenum[]){GL_TEXTURE_2D}
  );

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sf->fbo_handle);
  glViewport(0, 0, ctx->texture_width, ctx->texture_height);
}

static u0 vulkan_wait_for_opengl(vk_context *ctx) {
  vk_sc_ringbuf *rb = &ctx->swapchain;
  u32 idx = rb->current_index;

  glSignalSemaphoreEXT(
    rb->gl_wait_semaphores[idx], 
    0, NULL, 1, 
    (GLuint[]){rb->shared_fbos[idx].texture_handle}, 
    (GLenum[]){GL_TEXTURE_2D}
  );

  VkSubmitInfo submit = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = NULL,
      .pWaitDstStageMask = NULL,
      .commandBufferCount = 0,
      .pCommandBuffers = NULL,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &rb->vk_wait_semaphores[idx]
  };
  vkQueueSubmit(ctx->vk_queue, 1, &submit, VK_NULL_HANDLE);
}

u0 vulkan_present(vk_context *ctx) {
  vulkan_wait_for_opengl(ctx);
  vk_sc_ringbuf *rb = &ctx->swapchain;
  u32 image_index;

  vkAcquireNextImageKHR(
    ctx->vk_device, 
    ctx->vk_swapchain, 
    UINT64_MAX, 
    rb->vk_wait_semaphores[rb->current_index],
    VK_NULL_HANDLE, 
    &image_index
  );

  rb->current_index = image_index;

  vkWaitForFences(ctx->vk_device, 1, &ctx->inflight_fences[image_index], VK_TRUE, UINT64_MAX);
  vkResetFences(ctx->vk_device, 1, &ctx->inflight_fences[image_index]);

  VkCommandBuffer cmd = ctx->cmd_buffers[image_index];
  vkResetCommandBuffer(cmd, 0);
  VkCommandBufferBeginInfo bi = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
      NULL,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  vkBeginCommandBuffer(cmd, &bi);

  VkImageMemoryBarrier pre_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = ctx->swapchain_images[image_index],
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
  };

  vkCmdPipelineBarrier(
    cmd, 
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
    VK_PIPELINE_STAGE_TRANSFER_BIT, 
    0, 0, NULL, 0, NULL, 1,
    &pre_barrier
  );

  VkImageBlit blit = {
    .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
    .srcOffsets = {{0, 0, 0}, {ctx->texture_width, ctx->texture_height, 1}},
    .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
    .dstOffsets = {{0, 0, 0}, {ctx->texture_width, ctx->texture_height, 1}}
  };

  VkImageMemoryBarrier midBarrier = pre_barrier;
  {
    midBarrier.image = ctx->images[image_index];
    midBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    midBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    midBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    midBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  }

  vkCmdPipelineBarrier(
    cmd, 
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
    VK_PIPELINE_STAGE_TRANSFER_BIT, 
    0, 0, NULL, 0, NULL, 1, 
    &midBarrier
  );

  vkCmdBlitImage(
    cmd, 
    ctx->images[image_index], 
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
    ctx->swapchain_images[image_index], 
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
    1, 
    &blit, VK_FILTER_LINEAR
  );

  VkImageMemoryBarrier postBarrier = pre_barrier;
  {
    postBarrier.image = ctx->swapchain_images[image_index];
    postBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    postBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    postBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    postBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  }

  vkCmdPipelineBarrier(
    cmd, 
    VK_PIPELINE_STAGE_TRANSFER_BIT, 
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
    0, 0, NULL, 0, NULL, 
    1, &postBarrier
  );

  vkEndCommandBuffer(cmd);

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
  VkSubmitInfo submit = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &rb->vk_wait_semaphores[image_index],
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &rb->vk_signal_semaphores[image_index]
  };
  vkQueueSubmit(ctx->vk_queue, 1, &submit, ctx->inflight_fences[image_index]);

  VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &rb->vk_signal_semaphores[image_index],
      .swapchainCount = 1,
      .pSwapchains = &ctx->vk_swapchain,
      .pImageIndices = &image_index
  };
  vkQueuePresentKHR(ctx->vk_queue, &presentInfo);
}

u0 destroy_vulkan_context(vk_context *_context)
{
  // TODO: implement
}

// clang-format on