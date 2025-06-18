#version 460 core
#extension GL_ARB_bindless_texture : enable

struct SSBODataCell {
  float screen_pos_x; // pixel pos / screen_width
  float screen_pos_y; // pixel pos / screen_height
  float umin;
  float vmin;
  float umax;
  float vmax;
};

layout(std430, binding = 1) buffer GlyphData { SSBODataCell cells[]; };
layout(location = 0) in vec2 aPos;
layout(
  location = 1
) in vec2 aTextureCoords; // texture coords on quad (0,0) -> (1,1)
out vec2 oTextureCoords;  // texture coords of quad into atlas

void main() {
  SSBODataCell cell = cells[gl_InstanceID];
  vec2         baseNDC =
    vec2(cell.screen_pos_x * 2.0 - 1.0, (1.0 - cell.screen_pos_y) * 2.0 - 1.0);

  const float glyphW = 8.0 / 1920.0; // 8px in normalized‐screen coords
  const float glyphH = 8.0 / 1080.0;
  vec2        scale  = vec2(glyphW, glyphH);

  // 3) Map aPos (−1..1)→ offset (−scale..+scale), then move it
  gl_Position = vec4(aPos * scale + baseNDC, 0.0, 1.0);

  vec2 uvMin     = vec2(cell.umin, cell.vmin);
  vec2 uvMax     = vec2(cell.umax, cell.vmax);
  oTextureCoords = mix(uvMin, uvMax, aTextureCoords);
}