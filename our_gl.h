#pragma once
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

mat<4, 4> view(const int x, const int y, const int w, const int h, const int d);
mat<4, 4> project(const double coeff = 0); // coeff = -1/c
mat<4, 4> lookat(const vec3 eye, const vec3 center, const vec3 up);

struct RenderData {
  mat<4, 4> viewport;
  mat<4, 4> projection;
  mat<4, 4> modelView;
  mat<4, 4> normalMat;
  mat<4, 4> shadowMat;
  vec3 light;
  vec3 varying_intensity;
  mat<3, 3> varying_normal;
  mat<3, 3> ndc_tri;
  mat<3, 3> world_tri;
  int depth;

  mat<2, 3> varying_uv;
  Model *model;
  vec3 viewDir;
  TGAImage *shadow;
  TGAImage *occlImage;
};

struct IShader {
  RenderData renderData;
  static TGAColor sample2D(const TGAImage &img, const vec2 &uvf) {
    return img.get(uvf[0] * img.width(), uvf[1] * img.height());
  }
  virtual bool fragment(const vec3 bar, TGAColor &color) const = 0;
};

void rasterize(const vec4 clip_verts[3], const IShader &shader, TGAImage &image,
               std::vector<double> &zbuffer);

void triangle(vec3 *screen_coords, IShader &shader, TGAImage *image,
              TGAImage *zbuffer);