#include "SoftRenderer.h"
#include "geometry.h"
#include "tgaimage.h"
#include <algorithm>
#include <chrono>
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

struct DepthShader : public IShader {
  bool fragment(const vec3 bar, TGAColor &color) const { return false; }
};

struct Shader : public IShader {
  bool fragment(const vec3 bar, TGAColor &color) const {
    auto uv = renderData.varying_uv * bar;
    auto nrm = normalized(renderData.varying_normal.transpose() * bar);

    vec3 p = renderData.ndc_tri.transpose() * bar;
    vec4 pS = renderData.shadowMat * vec4{p.x, p.y, p.z, 1};
    float shadow =
        0.3 +
        0.7 *
            (renderData.shadow->get(
                 std::max(
                     0, std::min(int(pS.x / pS.w), renderData.shadow->width()-1)),
                 std::max(0, std::min(int(pS.y / pS.w),
                                      renderData.shadow->height()-1)))[0] < pS.z+40 );

    vec3 e1 = renderData.ndc_tri[1] - renderData.ndc_tri[0];
    vec3 e2 = renderData.ndc_tri[2] - renderData.ndc_tri[0];

    vec2 dUV1 = vec2{renderData.varying_uv[0][1] - renderData.varying_uv[0][0],
                     renderData.varying_uv[1][1] - renderData.varying_uv[1][0]};
    vec2 dUV2 = vec2{renderData.varying_uv[0][2] - renderData.varying_uv[0][0],
                     renderData.varying_uv[1][2] - renderData.varying_uv[1][0]};

    float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
    auto tangent = normalized(f * (dUV2.y * e1 - dUV1.y * e2));
    auto bitangent = normalized(f * (-dUV2.x * e1 + dUV1.x * e2));

    mat<3, 3> B;
    B[0] = tangent;
    B[1] = bitangent;
    B[2] = nrm;
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
                          uint8_t(5 + color[i] * shadow * (diff + .6 * spec)));
    }

    return false;
  }
};

int main(int, char **) {
  auto renderer = new SoftRenderer();

  auto model = new Model("obj/african_head/african_head.obj");
  renderer->append_model(model);
  model = new Model("obj/african_head/african_head_eye_inner.obj");
  renderer->append_model(model);

  renderer->center = {0, 0, 0};
  renderer->light = {0, 1, 1};
  renderer->eye = renderer->light;
  renderer->corner = {100, 100};
  renderer->size = {600, 600, 255};
  renderer->imageSize = {800, 800};
  renderer->init(false);

  GouraudShader shader1;
  Shader shader2;
  DepthShader depthShader;

  // shadowbuffer
  auto start = std::chrono::high_resolution_clock::now();
  renderer->render(depthShader);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  std::cout << "渲染时间: " << duration.count() << " 秒" << std::endl;
  renderer->write_zimage("depth.tga");

  // // render
  renderer->eye = {2, 1, 4};
  auto curMat = renderer->getCurCompoundMatrix();
  auto zimage = renderer->get_zImage();
  renderer->setShadowInfo(&zimage, curMat);
  renderer->init(true);
  start = std::chrono::high_resolution_clock::now();
  renderer->render(shader2);
  end = std::chrono::high_resolution_clock::now();
  duration = end - start;
  std::cout << "渲染时间: " << duration.count() << " 秒" << std::endl;
  renderer->write_image("output.tga");
  delete renderer;
}
