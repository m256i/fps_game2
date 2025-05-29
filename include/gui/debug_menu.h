#ifndef GUI_DEBUG_MENU_H_
#define GUI_DEBUG_MENU_H_

#include <RGFW/RGFW.h>

enum nk_glfw_init_state
{
    NK_GLFW3_DEFAULT = 0,
    NK_GLFW3_INSTALL_CALLBACKS
};

struct nk_context*   nk_glfw3_init(RGFW_window *win, enum nk_glfw_init_state, int max_vertex_buffer, int max_element_buffer);
void                 nk_glfw3_shutdown(void);
void                 nk_glfw3_font_stash_begin(void **atlas);
void                 nk_glfw3_font_stash_end(void);
void                 nk_glfw3_new_frame(void);
void                 nk_glfw3_render(void);

void                 nk_glfw3_device_destroy(void);
void                 nk_glfw3_device_create(void);

void                 nk_glfw3_key_callback(RGFW_window* win, u8 key, u8 keyChar, RGFW_keymod keyMod, RGFW_bool pressed);
void                 nk_gflw3_scroll_callback(RGFW_window *win, double xoff, double yoff);
void                 nk_glfw3_mouse_button_callback(RGFW_window* win, RGFW_mouseButton button, double scroll, RGFW_bool pressed);

unsigned int         nk_glfw3_create_texture(const void* image, int width, int height);
void                 nk_glfw3_destroy_texture(unsigned int tex_index);


#endif // GUI_DEBUG_MENU_H_
