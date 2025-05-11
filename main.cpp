#include "SoftRenderer.h"
#include "geometry.h"
#include "tgaimage.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ostream>

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
    auto nrm = normalized(renderData.varying_normal.transpose() * bar);

    auto nrms = renderData.varying_normal;
    mat<3, 3> A;
    A[0] = renderData.ndc_tri[1] - renderData.ndc_tri[0];
    A[1] = renderData.ndc_tri[2] - renderData.ndc_tri[0];
    A[2] = nrm;

    auto AI = A.invert();

    vec3 i =
        AI * vec3{renderData.varying_uv[0][1] - renderData.varying_uv[0][0],
                  renderData.varying_uv[0][2] - renderData.varying_uv[0][0], 0};
    vec3 j =
        AI * vec3{renderData.varying_uv[1][1] - renderData.varying_uv[1][0],
                  renderData.varying_uv[1][2] - renderData.varying_uv[1][0], 0};

    mat<3, 3> B;
    B.rows[0] = i;
    B.rows[1] = j;
    B.rows[2] = nrm;
    B = B.transpose();

    auto n = normalized(B * renderData.model->normal(uv));

    auto diff = std::max(0., n * renderData.light);

    auto &diffuse = renderData.model->diffuse();
    auto &specular = renderData.model->specular();

    auto r = normalized(2 * (n * renderData.light) * n - renderData.light);
    auto spec = pow(std::max(r * renderData.viewDir, 0.),
                    specular.get(int(uv.x * (diffuse.width() - 1)),
                                 int(uv.y * (diffuse.height() - 1)))[0]);
    color = diffuse.get(int(uv.x * (diffuse.width() - 1)),
                        int(uv.y * (diffuse.height() - 1)));
    for (int i = 0; i < 3; i++) {
      color[i] = std::min(uint8_t(255),
                          uint8_t(5 + color[i] * (diff + .6 * spec)));
    }

    return false;
  }
};

int main(int, char **) {
  auto model = new Model("obj/african_head/african_head.obj");
  auto renderer = new SoftRenderer();
  renderer->append_model(model);

  renderer->center = {0, 0, 0};
  renderer->eye = {2, 2, 4};
  renderer->light = {0, 0.5, 1};
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
