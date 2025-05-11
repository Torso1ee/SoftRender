#include "SoftRenderer.h"
#include "geometry.h"
#include "tgaimage.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>

struct GouraudShader : public IShader {
  bool fragment(const vec3 bar, TGAColor &color) const {
    float intensity = renderData.varying_intensity * bar;
    if (intensity > .8)
      intensity = .8;
    else if (intensity > .6)
      intensity = .6;
    else if (intensity > .4)
      intensity = .4;
    else if (intensity > .2)
      intensity = .2;
    else
      intensity = 0;
    color = TGAColor{uint8_t(255 * intensity), uint8_t(255 * intensity),
                     uint8_t(255 * intensity), 255};
    return false;
  }
};

struct Shader : public IShader {
  bool fragment(const vec3 bar, TGAColor &color) const {
    auto uv = renderData.varying_uv * bar;
    auto n = renderData.model->normal(uv);
    vec4 n4{n.x, n.y, n.z, 1};
    n4 = renderData.normalMat * n4;
    n = {n4.x / n4.w, n4.y / n4.w, n4.z / n4.w};
    normalized(n);
    float intensity = std::max(0., n * renderData.light);
    auto &diffuse = renderData.model->diffuse();
    color = diffuse.get(int(uv.x * (diffuse.width() - 1)),
                        int(uv.y * (diffuse.height() - 1)));
    color =
        TGAColor{uint8_t(color[0] * intensity), uint8_t(color[1] * intensity),
                 uint8_t(color[2] * intensity)};

    return false;
  }
};

int main(int, char **) {
  auto model = new Model("obj/african_head/african_head.obj");
  auto renderer = new SoftRenderer();
  renderer->append_model(model);

  renderer->center = {0, 0, 0};
  renderer->eye = {1, 1, 4};
  renderer->light = {0, 0, 1};
  renderer->corner = {100, 100};
  renderer->size = {600, 600, 255};
  renderer->imageSize = {800, 800};
  renderer->init();

  GouraudShader shader1;
  Shader shader2;

  renderer->render(shader2);
  renderer->write_image("output.tga");
  delete renderer;
}
