#include "SoftRenderer.h"
#include "geometry.h"
#include "tgaimage.h"
#include <algorithm>
#include <cstdint>

struct GouraudShader : public IShader {
  bool fragment(const vec3 bar, TGAColor &color) const {
    float intensity = varying_intensity * bar;
    color = TGAColor{uint8_t(255 * intensity), uint8_t(255 * intensity),
                     uint8_t(255 * intensity), 255};
    return false;
  }
};

int main(int, char **) {
  auto model = new Model("obj/african_head/african_head.obj");
  auto renderer = new SoftRenderer();
  renderer->append_model(model);

  renderer->center = {0, 0, 0};
  renderer->eye = {4, 2, 4};
  renderer->light = {0, 1, 1};
  renderer->corner = {100, 100};
  renderer->size = {600, 600,255};
  renderer->imageSize = {800, 800};
  renderer->init();

  GouraudShader shader;

  renderer->render(shader);
  renderer->write_image("output.tga");
  delete renderer;
}
