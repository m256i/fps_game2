// peak shitcode incoming
#include <assert.h>
#include <glad/glad.h>
#include <util/dbg/alloctrack.h>
#include <windowing/vulkan_present.h>
#include <renderer/gl_api.h>

#ifdef GAME_DEBUG

#define CHECK_VK(result, msg)                                                  \
  if (result != VK_SUCCESS) {                                                  \
    GAME_LOGF("%s (code %d)\n", msg, result);                                  \
    exit(1);                                                                   \
  }
#else
#define CHECK_VK(result, msg)                                                  \
  do {                                                                         \
    (u0) result;                                                               \
    (u0) msg;                                                                  \
  } while (0);
#endif

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

static VkSemaphore create_intenal_semaphore(VkDevice _device) {
  VkSemaphoreCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 
    .pNext = &(VkSemaphoreTypeCreateInfo){
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
      .initialValue = 0
    }
  };
  VkSemaphore semaphore;
  vkCreateSemaphore(_device, &info, NULL, &semaphore);
  return semaphore;
}

static u0 initialize_ringbuffer_sync(vk_context *ctx) {
  ctx->swapchain.current_index = 0;
  ctx->swapchain.vk_wait_semaphores     = TRACKED_MALLOC(sizeof(VkSemaphore) * ctx->swapchain.count_fbos);
  ctx->swapchain.vk_signal_semaphores   = TRACKED_MALLOC(sizeof(VkSemaphore) * ctx->swapchain.count_fbos);
  ctx->swapchain.vk_present_semaphores  = TRACKED_MALLOC(sizeof(VkSemaphore) * ctx->swapchain.count_fbos);
  ctx->swapchain.gl_wait_semaphores     = TRACKED_MALLOC(sizeof(GLuint) * ctx->swapchain.count_fbos);
  ctx->swapchain.gl_signal_semaphores   = TRACKED_MALLOC(sizeof(GLuint) * ctx->swapchain.count_fbos);
  ctx->swapchain.wait_handles           = TRACKED_MALLOC(sizeof(HANDLE) * ctx->swapchain.count_fbos);
  ctx->swapchain.sig_handles            = TRACKED_MALLOC(sizeof(HANDLE) * ctx->swapchain.count_fbos);

  memset(ctx->swapchain.vk_wait_semaphores, 0, sizeof(VkSemaphore) * ctx->swapchain.count_fbos);
  memset(ctx->swapchain.vk_signal_semaphores, 0, sizeof(VkSemaphore) * ctx->swapchain.count_fbos);

  memset(ctx->swapchain.gl_wait_semaphores, 0, sizeof(GLuint) * ctx->swapchain.count_fbos);
  memset(ctx->swapchain.gl_signal_semaphores, 0, sizeof(GLuint) * ctx->swapchain.count_fbos);

  GAME_LOGF("count_fbos: %zu", ctx->swapchain.count_fbos);

  GL_CALL(glGenSemaphoresEXT(ctx->swapchain.count_fbos, ctx->swapchain.gl_wait_semaphores));
  GL_CALL(glGenSemaphoresEXT(ctx->swapchain.count_fbos, ctx->swapchain.gl_signal_semaphores));

  const char* exts = (const char*)glGetString(GL_EXTENSIONS);
  GAME_ASSERT(strstr(exts, "GL_EXT_semaphore")           != NULL);
  GAME_ASSERT(strstr(exts, "GL_EXT_semaphore_win32")     != NULL);
  GAME_ASSERT(strstr(exts, "GL_EXT_memory_object")       != NULL);
  GAME_ASSERT(strstr(exts, "GL_EXT_memory_object_win32") != NULL);

  for (usize i = 0; i < ctx->swapchain.count_fbos; i++) {
    ctx->swapchain.vk_wait_semaphores[i] = create_exportable_semaphore(ctx->vk_device);
    ctx->swapchain.vk_signal_semaphores[i] = create_exportable_semaphore(ctx->vk_device);
    ctx->swapchain.vk_present_semaphores[i] = create_intenal_semaphore(ctx->vk_device);

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

    ctx->swapchain.wait_handles[i]  = wait_handle;
    ctx->swapchain.sig_handles[i]   = sig_handle;
    /*
    gl wait = vk signal and vice versa
    */
    GL_CALL(glImportSemaphoreWin32HandleEXT(ctx->swapchain.gl_wait_semaphores[i], GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, sig_handle));
    GL_CALL(glImportSemaphoreWin32HandleEXT(ctx->swapchain.gl_signal_semaphores[i], GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, wait_handle));
  }
}

static VkPresentModeKHR get_viable_present_mode(VkPhysicalDevice *_phys_dev, VkSurfaceKHR *_surface, VkPresentModeKHR  _prefered_mode) {
  u32 present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(*_phys_dev, *_surface, &present_mode_count, NULL);
  if (!present_mode_count) {
    GAME_LOGF("no available present modes!");
    exit(1);
  }
  VkPresentModeKHR *present_modes = TRACKED_MALLOC(sizeof(present_mode_count) * present_mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(*_phys_dev, *_surface, &present_mode_count, present_modes);
  for (u32 i = 0; i != present_mode_count; i++) {
    /* we prefer immediate (no waiting) */
    if (present_modes[i] == _prefered_mode) {
      TRACKED_FREE(present_modes);
      return _prefered_mode;
    }
  } // VK_PRESENT_MODE_IMMEDIATE_KHR
  GAME_LOGF("rendering mode not available: %i", _prefered_mode);
  exit(1);
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
  VkSurfaceFormatKHR *formats = TRACKED_MALLOC(sizeof(VkSurfaceFormatKHR) * format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(*_phys_dev, *_surface, &format_count, formats);

  for (u32 i = 0; i != format_count; i++) {
    if (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
      const VkSurfaceFormatKHR f= formats[i];
      TRACKED_FREE(formats);
      return f;
    }
    if (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
      const VkSurfaceFormatKHR f= formats[i];
      TRACKED_FREE(formats);
      return f;
    }
  }
  /* safe default */
  TRACKED_FREE(formats);
  GAME_LOGF("using fallback surface format mode!");
  exit(1);
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

u0 initialize_vulkan_context(vk_context *_context, HWND _window_handle, usize _screen_w, usize _screen_h, u32 _rmode) {
  if (_context->initialized) {
    GAME_WARNF("initialize_vulkan_context called on already initialized vk context");  
    return;
  }
  
  memset(_context, 0, sizeof *_context);
#ifdef GAME_DEBUG
  {
    u32 lc = 0;
    vkEnumerateInstanceLayerProperties(&lc, NULL);
    VkLayerProperties *available_layers = TRACKED_MALLOC(sizeof(VkLayerProperties) * lc);
    vkEnumerateInstanceLayerProperties(&lc, available_layers);
    bool av = false;
    for (u32 i = 0; i < lc; i++) {
      if (strcmp(available_layers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
        av = true;
        break;
      }
    }
    TRACKED_FREE(available_layers);
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
#endif

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = GAME_CLIENT_VER_STR,
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "None",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_1,
  };

#ifdef GAME_DEBUG
  const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
#endif
  const char *instance_extensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef GAME_DEBUG
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
      VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    };

  VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
#ifdef GAME_DEBUG
      .enabledExtensionCount = 4,
      .enabledLayerCount = 1,
      .ppEnabledLayerNames = layers,
      .pNext = &debug_create_info,
      #else
      .enabledExtensionCount = 3,
#endif
      .ppEnabledExtensionNames = instance_extensions,
  };

  VkInstance instance;
  CHECK_VK(vkCreateInstance(&instance_info, NULL, &instance), "failed to create Vulkan instance");

  u32 gpu_count = 0;
  vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
  if (gpu_count == 0) {
    GAME_LOGF("no vulkan-capable GPU found");
    return;
  }

#ifdef GAME_DEBUG
  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (vkCreateDebugUtilsMessengerEXT != NULL) {
    vkCreateDebugUtilsMessengerEXT(instance, &debug_create_info, NULL, &_context->debug_messenger);
  }
#endif

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
    VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME,
    VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
  };

  VkDeviceCreateInfo deviceInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queue_info,
      .enabledExtensionCount = 6,
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

  while (_screen_w == 0 || _screen_h == 0) {
    /* wait for pending WM_DISPLAYCHANGE fires when calling this right after resizing the window */
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &surface_caps),
    "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    _screen_w = max(surface_caps.minImageExtent.width, min(_screen_w, surface_caps.maxImageExtent.width));
    _screen_h = max(surface_caps.minImageExtent.height, min(_screen_h, surface_caps.maxImageExtent.height));
  }

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
  const VkPresentModeKHR present_mode = _rmode == RENDER_MODE_VSYNC ? get_viable_present_mode(&phys_dev, &surface, VK_PRESENT_MODE_FIFO_KHR) : get_viable_present_mode(&phys_dev, &surface, VK_PRESENT_MODE_IMMEDIATE_KHR);

  _context->surface_format = format;

  VkSurfaceFullScreenExclusiveWin32InfoEXT win32Info = {
      .sType    = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT,
      .pNext    = NULL,
      .hmonitor = MonitorFromWindow(_window_handle, MONITOR_DEFAULTTOPRIMARY)
  };

  GAME_ASSERT(win32Info.hmonitor != NULL);

  VkSurfaceFullScreenExclusiveInfoEXT fsInfo = {
      .sType               = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT,
      .pNext               = &win32Info,
      .fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT
  };

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
      .pNext = &fsInfo,
  };

  VkSwapchainKHR swapchain;
  CHECK_VK(vkCreateSwapchainKHR(device, &sci, NULL, &swapchain), "failed to create swapchain");

  PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT =
    (PFN_vkAcquireFullScreenExclusiveModeEXT)vkGetInstanceProcAddr(instance, "vkAcquireFullScreenExclusiveModeEXT");

  if (vkAcquireFullScreenExclusiveModeEXT == NULL) {
    GAME_LOGF("failed to load vkAcquireFullScreenExclusiveModeEXT");
    exit(1);
  }

  if (vkAcquireFullScreenExclusiveModeEXT(device, swapchain) != VK_SUCCESS) {
    GAME_LOGF("couldn't acquire fullscreen exclusive mode (you are propably on windows 11 +)");
  }

  _context->vk_surface = surface;
  _context->vk_swapchain = swapchain;

  u32 image_count = 0;
  vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
  _context->swapchain.count_fbos = image_count;
  _context->swapchain.shared_fbos = TRACKED_MALLOC(sizeof(shared_fbo) * image_count);

  _context->images = TRACKED_MALLOC(sizeof(VkImage) * image_count);
  _context->swapchain_images = TRACKED_MALLOC(sizeof(VkImage) * image_count);
  _context->image_memories = TRACKED_MALLOC(sizeof(VkDeviceMemory) * image_count);

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

  
    vkAllocateMemory(device, &alloc_info, NULL, &_context->image_memories[i]);
    vkBindImageMemory(device, _context->images[i], _context->image_memories[i], 0);

    VkMemoryGetWin32HandleInfoKHR handle_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
        .memory = _context->image_memories[i],
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
    };
    
    HANDLE win_handle;
    vkGetMemoryWin32HandleKHR(device, &handle_info, &win_handle);

    GLenum err = glGetError();

    shared_fbo sfbo = {0};
    glCreateMemoryObjectsEXT(1, &sfbo.memory_object);
    
    err = glGetError();
    if (err != 0) {
      GAME_CRITICALF("glCreateMemoryObjectsEXT failed with error %u", err);
      exit(1);
    }

    glImportMemoryWin32HandleEXT(sfbo.memory_object, (GLsizeiptr)req.size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, win_handle);
    err = glGetError();
    if (err != 0) {
      GAME_CRITICALF("glImportMemoryWin32HandleEXT failed with error %u", err);
      exit(1);
    }

    GLenum gl_format;
    switch (format.format) {
    case VK_FORMAT_B8G8R8A8_SRGB: gl_format = GL_BGRA_EXT; break;
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

  _context->cmd_buffers = TRACKED_MALLOC(sizeof(VkCommandBuffer) * image_count);
  _context->inflight_fences = TRACKED_MALLOC(sizeof(VkFence) * image_count);

  VkCommandBufferAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = _context->cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = image_count,
  };
  vkAllocateCommandBuffers(_context->vk_device, &allocInfo, _context->cmd_buffers);

  for (uint32_t i = 0; i < image_count; i++) {
    VkFenceCreateInfo fenceInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = 0};
    vkCreateFence(_context->vk_device, &fenceInfo, NULL, &_context->inflight_fences[i]);
  }

  initialize_ringbuffer_sync(_context);
  _context->initialized = true;

  GAME_LOGF("successfully initialized vulkan surface");
}

static u32 image_index = 0;

usize bind_vulkan_surface(vk_context *ctx) {
#ifdef GAME_DEBUG
  if (!ctx) {
    GAME_WARNF("bind_vulkan_surface call on uninitialized vk_context");
    return 0;
  }
  if (!ctx->initialized) {
    GAME_WARNF("bind_vulkan_surface call on uninitialized vk_context");
    return 0;
  }
#endif

  /*
  -- aqcuire next image with semphore
  -- make opengl wait for semahpore
  */
  vk_sc_ringbuf *rb = &ctx->swapchain;

  CHECK_VK(vkAcquireNextImageKHR(
    ctx->vk_device, 
    ctx->vk_swapchain, 
    UINT64_MAX, 
    rb->vk_signal_semaphores[rb->current_index], /* this is linked to opengls wait semaphore */
    VK_NULL_HANDLE, 
    &image_index
  ), "vkAcquireNextImageKHR failed");

  shared_fbo *sf = &rb->shared_fbos[image_index];

  /* bind the framebuffer texture for rendering */
  GL_CALL(glViewport(0, 0, ctx->texture_width, ctx->texture_height));
  GL_CALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER , sf->fbo_handle));

  return rb->current_index;
  /* now we can call opengl render calls */
}

static u0 vulkan_wait_for_opengl(vk_context *ctx) {
#ifdef GAME_DEBUG
  if (!ctx) {
    GAME_WARNF("vulkan_wait_for_opengl call on uninitialized vk_context");
    return;
  }
  if (!ctx->initialized) {
    GAME_WARNF("vulkan_wait_for_opengl call on uninitialized vk_context");
    return;
  }
#endif

  vk_sc_ringbuf *rb = &ctx->swapchain;
  /*
  signal vulkan that the render into the texture is done
  */
  GL_CALL(glSignalSemaphoreEXT(
    rb->gl_signal_semaphores[rb->current_index], 
    0, NULL, 1, 
    (GLuint[]){rb->shared_fbos[image_index].texture_handle}, 
    (GLenum[]){GL_LAYOUT_COLOR_ATTACHMENT_EXT}
  ));

  /*
  make vulkan wait for gl to write to the shared FBO/vkImage
  */
  const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; /* TODO: is this the right mask? */
  VkSubmitInfo submit = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &rb->vk_wait_semaphores[rb->current_index],
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 0,
      .pCommandBuffers = NULL,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = NULL
  };
  vkQueueSubmit(ctx->vk_queue, 1, &submit, VK_NULL_HANDLE);
}

u0 vulkan_present(vk_context *ctx) {
#ifdef GAME_DEBUG
  if (!ctx) {
    GAME_WARNF("vulkan_present call on uninitialized vk_context");
    return;
  }
  if (!ctx->initialized) {
    GAME_WARNF("vulkan_present call on uninitialized vk_context");
    return;
  }
#endif
  vulkan_wait_for_opengl(ctx);
  vk_sc_ringbuf *rb = &ctx->swapchain;
  usize source_index = image_index;
  u32 image_index = source_index;
  
  const VkCommandBuffer cmd = ctx->cmd_buffers[image_index];
  vkResetCommandBuffer(cmd, 0);
  VkCommandBufferBeginInfo bi = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
      NULL,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  vkBeginCommandBuffer(cmd, &bi);

  const VkImageMemoryBarrier pre_barrier = {
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

  VkImageMemoryBarrier mid_barrier = pre_barrier;
  {
    mid_barrier.image = ctx->images[source_index];
    mid_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    mid_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    mid_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    mid_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  }

  vkCmdPipelineBarrier(
    cmd, 
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
    VK_PIPELINE_STAGE_TRANSFER_BIT, 
    0, 0, NULL, 0, NULL, 1, 
    &mid_barrier
  );

  const VkImageBlit blit = {
      .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
      .srcOffsets = {{0, 0, 0}, {ctx->texture_width, ctx->texture_height, 1}},
      .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
      .dstOffsets = {{0,  ctx->texture_height, 0},  {ctx->texture_width, 0, 1}}
  };

  vkCmdBlitImage(
    cmd, 
    ctx->images[source_index], 
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
    ctx->swapchain_images[image_index], 
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
    1, 
    &blit, VK_FILTER_LINEAR
  );

  VkImageMemoryBarrier post_barrier = pre_barrier;
  {
    post_barrier.image = ctx->swapchain_images[image_index];
    post_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    post_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    post_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    post_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  }

  vkCmdPipelineBarrier(
    cmd, 
    VK_PIPELINE_STAGE_TRANSFER_BIT, 
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
    0, 0, NULL, 0, NULL, 
    1, &post_barrier
  );

  vkEndCommandBuffer(cmd);

  /* we dont need to wait for anything on render */
  const VkSubmitInfo submit = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &rb->vk_signal_semaphores[rb->current_index],
      .pWaitDstStageMask = 0,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1, /* we make the presentation engine wait for our "present_semaphore" */
      .pSignalSemaphores = &rb->vk_present_semaphores[rb->current_index]
  };

  vkQueueSubmit(ctx->vk_queue, 1, &submit, ctx->inflight_fences[rb->current_index]);

  const VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &rb->vk_present_semaphores[rb->current_index],
      .swapchainCount = 1,
      .pSwapchains = &ctx->vk_swapchain,
      .pImageIndices = &image_index
  };

  if (vkQueuePresentKHR(ctx->vk_queue, &presentInfo) == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT) {
    GAME_CRITICALF("fullscreen exclusive mode lost");
    PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT =
      (PFN_vkAcquireFullScreenExclusiveModeEXT)vkGetInstanceProcAddr(ctx->vk_instance, "vkAcquireFullScreenExclusiveModeEXT");
  
    if (vkAcquireFullScreenExclusiveModeEXT == NULL) {
      GAME_LOGF("failed to load vkAcquireFullScreenExclusiveModeEXT");
      exit(1);
    }
    if (vkAcquireFullScreenExclusiveModeEXT(ctx->vk_device, ctx->vk_swapchain) != VK_SUCCESS) {
      GAME_LOGF("couldn't acquire fullscreen exclusive mode (you are propably on windows 11 +)");
    }
    CHECK_VK(vkQueuePresentKHR(ctx->vk_queue, &presentInfo), "couldnt aqcuire fullscreen exclusive mode after refocusing");
  }

  /* wait CPU here for completion since we manually increment the frame index after */
  vkWaitForFences(ctx->vk_device, 1, &ctx->inflight_fences[rb->current_index], VK_TRUE, UINT64_MAX);
  vkResetFences(ctx->vk_device, 1, &ctx->inflight_fences[rb->current_index]);

  rb->current_index = (rb->current_index + 1) % rb->count_fbos;
}

u0 destroy_vulkan_context(vk_context *_context)
{
  if (!_context) return;
  if (!_context->initialized) {
    GAME_WARNF("destroy_vulkan_context called on un-initialized vk context");  
    return;
  }

  vkDeviceWaitIdle(_context->vk_device);

  GAME_LOGF("destroying vk_context");
  
  VkDevice device = _context->vk_device;
  
  vk_sc_ringbuf *rb = &_context->swapchain;
  if (rb->shared_fbos) {
    for (usize i = 0; i < rb->count_fbos; ++i) {
      shared_fbo *fbo = &rb->shared_fbos[i];
      if (fbo->fbo_handle) glDeleteFramebuffers(1, &fbo->fbo_handle);
      if (fbo->texture_handle) glDeleteTextures(1, &fbo->texture_handle);
      if (fbo->memory_object) glDeleteMemoryObjectsEXT(1, &fbo->memory_object);
    }
    TRACKED_FREE(rb->shared_fbos);
  }

  GAME_LOGF("destroyed shared fbos");
  
  if (rb->gl_wait_semaphores) {
    glDeleteSemaphoresEXT((GLsizei)rb->count_fbos, rb->gl_wait_semaphores);
    TRACKED_FREE(rb->gl_wait_semaphores);
  }
  
  if (rb->gl_signal_semaphores) {
    glDeleteSemaphoresEXT((GLsizei)rb->count_fbos, rb->gl_signal_semaphores);
    TRACKED_FREE(rb->gl_signal_semaphores);
  }

  GAME_LOGF("destroyed signal semaphores");
  
  if (rb->vk_wait_semaphores) {
    for (usize i = 0; i < rb->count_fbos; ++i) {
      if (rb->vk_wait_semaphores[i]) {
        GAME_LOGF("destroyed vulkan wait semaphore %d", i);
        vkDestroySemaphore(device, rb->vk_wait_semaphores[i], NULL);
      }
    }
    TRACKED_FREE(rb->vk_wait_semaphores);
  }

  GAME_LOGF("destroyed wait semaphores");

  if (rb->vk_signal_semaphores) {
    for (usize i = 0; i < rb->count_fbos; ++i) {
      if (rb->vk_signal_semaphores[i]) {
        GAME_LOGF("destroyed vulkan signal semaphore %d", i);
        vkDestroySemaphore(device, rb->vk_signal_semaphores[i], NULL);
      }
    }
    TRACKED_FREE(rb->vk_signal_semaphores);
  }

  GAME_LOGF("destroyed vk signal semaphores");

  if (rb->vk_present_semaphores) {
    for (usize i = 0; i < rb->count_fbos; ++i) {
      if (rb->vk_present_semaphores[i]) {
        GAME_LOGF("destroyed vulkan signal semaphore %d", i);
        vkDestroySemaphore(device, rb->vk_present_semaphores[i], NULL);
      }
    }
    TRACKED_FREE(rb->vk_present_semaphores);
  }

  GAME_LOGF("destroyed vk present semaphores");

  if (rb->wait_handles) {
    if (rb->wait_handles) {
      for (usize i = 0; i < rb->count_fbos; ++i) {
        CloseHandle(rb->wait_handles[i]);
      }
    }
    TRACKED_FREE(rb->wait_handles);
  }

  if (rb->sig_handles) {
    if (rb->sig_handles) {
      for (usize i = 0; i < rb->count_fbos; ++i) {
        CloseHandle(rb->sig_handles[i]);
      }
      TRACKED_FREE(rb->sig_handles);
    }
  }

  if (_context->inflight_fences) {
    for (usize i = 0; i < rb->count_fbos; ++i) {
      if (_context->inflight_fences[i]) {
        GAME_LOGF("destroyed vulkan fence %d", i);
        vkResetFences(_context->vk_device, 1, &_context->inflight_fences[i]);
        vkDestroyFence(device, _context->inflight_fences[i], NULL);
      }
    }
    TRACKED_FREE(_context->inflight_fences);
  }

  GAME_LOGF("destroyed vk fences semaphores");

  if (_context->cmd_buffers) {
    vkFreeCommandBuffers(device, _context->cmd_pool, (uint32_t)rb->count_fbos, _context->cmd_buffers);
    TRACKED_FREE(_context->cmd_buffers);
  }

  GAME_LOGF("destroyed vk cmd buffers");
  
  if (_context->cmd_pool) {
    vkDestroyCommandPool(device, _context->cmd_pool, NULL);
  }

  GAME_LOGF("destroyed vk cmd pool");
  
  if (_context->images) {
    for (usize i =0; i != _context->swapchain.count_fbos; i++) {
      vkDestroyImage(_context->vk_device, _context->images[i], NULL);
    }
    TRACKED_FREE(_context->images); // These are not created or destroyed manually, just array of handles
  }

  GAME_LOGF("destroyed vk images");
  
  if (_context->image_memories) {
    for (usize i =0; i != _context->swapchain.count_fbos; i++) {
      vkFreeMemory(_context->vk_device, _context->image_memories[i], NULL);
    }
    TRACKED_FREE(_context->image_memories);
  }

  GAME_LOGF("destroyed vk image memories");
  
  if (_context->swapchain_images) {
    TRACKED_FREE(_context->swapchain_images);
  }
  GAME_LOGF("destroyed vk swapchain images");
  
  if (_context->vk_swapchain) {
    vkDestroySwapchainKHR(device, _context->vk_swapchain, NULL);
  }

  GAME_LOGF("destroyed vk swapchain");
  
  if (_context->vk_surface) {
    vkDestroySurfaceKHR(_context->vk_instance, _context->vk_surface, NULL);
  }

  GAME_LOGF("destroyed vk surface");
  
  if (device) {
    vkDestroyDevice(device, NULL);
  }

  GAME_LOGF("destroyed vk device");

  if (_context->vk_instance) {
#ifdef GAME_DEBUG
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
    (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_context->vk_instance, "vkDestroyDebugUtilsMessengerEXT");
if (vkDestroyDebugUtilsMessengerEXT != NULL) {
  vkDestroyDebugUtilsMessengerEXT(_context->vk_instance,  _context->debug_messenger, NULL);
}
#endif
    vkDestroyInstance(_context->vk_instance, NULL);
  }

  GAME_LOGF("successfully destroyed vulkan surface");

  _context->initialized = false;
  memset(_context, 0, sizeof(*_context));
}

// clang-format on
