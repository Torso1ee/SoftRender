#pragma once
#include "model.h"
#include "our_gl.h"
#include "tgaimage.h"
#include <vector>

class SoftRenderer {

public:
  ~SoftRenderer();

  vec3 center;
  vec3 eye;
  vec3 up = {0, 1, 0};
  vec3 light;
  vec2 corner;
  vec3 size;
  vec2 imageSize;

  void append_model(Model *model);
  void init();

  void render(IShader &shader);
  void write_image(const char *path);
  void write_Zimage(const char *path);


private:
  std::vector<Model *> scene;
  vec3 camera;
  mat<4, 4> projection;
  mat<4, 4> modelView;
  mat<4, 4> viewport;
  TGAImage *zbuffer;
  TGAImage *image;
  bool inited;
};