#include "our_gl.h"
#include <algorithm>
#include <complex>
#include <cstdint>
#include <iostream>
#include <ostream>
mat<4, 4> view(const int x, const int y, const int w, const int h,
               const int d) {
  auto m = identity<4>();
  m[0][3] = x + w / 2.f;
  m[1][3] = y + h / 2.f;
  m[2][3] = d / 2.f;

  m[0][0] = w / 2.f;
  m[1][1] = h / 2.f;
  m[2][2] = d / 2.f;
  return m;
}

mat<4, 4> project(const double coeff) {
  auto m = identity<4>();
  m[3][2] = coeff;
  return m;
}

vec3 barycentric(vec3 A, vec3 B, vec3 C, vec3 P) {
  vec3 s[2];
  for (int i = 2; i--;) {
    s[i][0] = C[i] - A[i];
    s[i][1] = B[i] - A[i];
    s[i][2] = A[i] - P[i];
  }
  vec3 u = cross(s[0], s[1]);
  if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero
                             // then triangle ABC is degenerate
    return vec3{1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z};
  return vec3{-1, 1, 1}; // in this case generate negative coordinates, it will
                         // be thrown away by the rasterizator
}

mat<4, 4> lookat(const vec3 eye, const vec3 center, const vec3 up) {
  auto z = normalized(eye - center);
  auto x = normalized(cross(up, z));
  auto y = normalized(cross(z, x));
  auto minV = identity<4>();
  auto tr = identity<4>();
  for (int i = 0; i < 3; i++) {
    minV[0][i] = x[i];
    minV[1][i] = y[i];
    minV[2][i] = z[i];
    tr[i][3] = -center[i];
  }
  return minV * tr;
}

void triangle(vec3 *screen_coords, IShader &shader, TGAImage *image,
              TGAImage *zbuffer) {
  vec2 bbmin = {std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max()};
  vec2 bbmax = {-std::numeric_limits<double>::max(),
                -std::numeric_limits<double>::max()};
  vec2 clamp = {(double)image->width() - 1, (double)image->height() - 1};
  for (int i = 0; i < 3; i++) {
    bbmin[0] = std::max(0.0, std::min(screen_coords[i].x, bbmin[0]));
    bbmin[1] = std::max(0.0, std::min(screen_coords[i].y, bbmin[1]));
    bbmax[0] = std::min(clamp[0], std::max(screen_coords[i].x, bbmax[0]));
    bbmax[1] = std::min(clamp[1], std::max(screen_coords[i].y, bbmax[1]));
  }
  vec3 p;
  TGAColor color;
  for (p.x = bbmin[0]; p.x <= bbmax[0]; p.x++) {
    for (p.y = bbmin[1]; p.y <= bbmax[1]; p.y++) {
      auto bc =
          barycentric(screen_coords[0], screen_coords[1], screen_coords[2], p);
      if (bc.x < 0 || bc.y < 0 || bc.z < 0)
        continue;
      p.z = 0;
      for (int k = 0; k < 3; k++) {
        p.z += screen_coords[k].z * bc[k];
      }
      int frag_depth = std::max(0, std::min(255, int(p.z + .5)));
      if (frag_depth > zbuffer->get(p.x, p.y)[0]) {
        bool discard = shader.fragment(bc, color);
        if (!discard) {
          zbuffer->set(p.x, p.y, TGAColor{uint8_t(frag_depth)});
          image->set(p.x, p.y, color);
        }
      }
    }
  }
}