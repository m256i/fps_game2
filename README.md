# Game engine thing (progress documentation for myself)
##   (Future) feature index:
- [x] Custom rendering backend/Win32 surface
- [x] Relatively advanced frame pacer (basically done)
- [ ] Fast SSE4.2/AVX2 accelerated math library (done soon)
- [ ] Fast OpenGL reference counting resource manager  
- [ ] Mesh and animation loading using [Assimp](https://github.com/assimp/assimp)
- [ ] Model system with: static render model, that can have LODS and can be instanced optionally

##   Win32 backend/Frame pacer:
- Vulkan backend provides exclusive fullscreen mode:
  - OpenGL draws to a shared vkImage that gets Blit'ted into the swapchain directly
  - Synchronization through external win32 semaphores
- We aim to provide minimal screen tearing and lowest input->photon latency:
  - By scheduling rendering and presenting so that the last render gets flipped on to the monitor virtually instantly
  
![image](https://github.com/user-attachments/assets/e40e4d52-fd81-49aa-87fc-5716dbe0b618)

(timings of the present calls compared to the next vsync (yellow line) on average)

![image](https://github.com/user-attachments/assets/4628c372-30cc-4694-a29f-a785454253d3)

(ipt->photon latency on my machine, measured (CurrentVBlankTimeStamp-LastRenderBeginTimestamp))
