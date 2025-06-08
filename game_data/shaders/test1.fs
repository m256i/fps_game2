#version 460 core
// #extension GL_ARB_bindless_texture : enable
// #extension GL_ARB_gpu_shader_fp64 : enable
// clang-format off

// uniform sampler2D tex;
// precision highp float;

// in vec2 u_mousePos;
// out vec4 FragColor;

in float camera_distance;
out vec4 FragColor;

const float FOG_DISTANCE = 1.5;

// vec3 hsv2rgb(in vec3 hsv) {
//     float h = hsv.x * 6.0;
//     float c = hsv.z * hsv.y;
//     float x = c * (1.0 - abs(mod(h, 2.0) - 1.0));
//     vec3 col;
//     if      (h < 1.0) col = vec3(c, x, 0.0);
//     else if (h < 2.0) col = vec3(x, c, 0.0);
//     else if (h < 3.0) col = vec3(0.0, c, x);
//     else if (h < 4.0) col = vec3(0.0, x, c);
//     else if (h < 5.0) col = vec3(x, 0.0, c);
//     else              col = vec3(c, 0.0, x);
//     float m = hsv.z - c;
//     return col + vec3(m);
// }

void main() {
  float fog_mask = clamp((1 / (camera_distance / FOG_DISTANCE)), 0.0, 1.0);
  FragColor = vec4(fog_mask, fog_mask, 0.0, 1.0);
}

// void main() {
//     const vec2 u_center = vec2(-0.7436433110005587, 0.13182515602251197);
//     //const vec2 u_center = vec2(0.27334, 0.00742);
//     const vec2 u_resolution = vec2(1920, 1080);

//     dvec2 uv = (gl_FragCoord.xy / u_resolution) * 2.0 - 1.0;
//     double aspect = u_resolution.x / u_resolution.y;
//     uv.x *= aspect;

//     double zoomExp = mix(0.0, 50.0, u_mousePos.x);
//     double zoom = double(exp2(float(zoomExp)));

//     double baseScale = 1.5;
//     double scale = baseScale / zoom;

//     dvec2 c = u_center + uv * scale;

//     double itMin = 150.0;
//     double itMax = 2000.0;
//     double itf = mix(itMin, itMax, clamp(u_mousePos.x, 0.0, 1.0));
//     int maxIter = int(itf);

//     dvec2 z = vec2(0.0);
//     double nu = 0.0;
//     int i;
//     for (i = 0; i < maxIter; i++) {
//         dvec2 z2 = dvec2(
//             z.x * z.x - z.y * z.y + c.x,
//             2.0 * z.x * z.y + c.y
//         );
//         z = z2;

//         double zr2 = dot(z, z);
//         if (zr2 > 4.0) {
//             double modz = sqrt(zr2);
//             nu = double(i) + 1.0 - double(log(log(float(modz)))) / double(log(2.0));
//             break;
//         }
//     }
//     if (i == maxIter) {
//         nu = double(maxIter);
//     }

//     double t = nu / double(maxIter);
//     float hue = mod(0.95 + 10.0 * float(t), 1.0);
//     float sat = 0.8;
//     float val = (i == maxIter) ? 0.0 : 1.0;
//     vec3 rgb = hsv2rgb(vec3(hue, sat * 0.85, val));
//     FragColor = vec4(rgb, 1.0);
// }