#include "SoftRenderer.h"
#include "geometry.h"
#include "tgaimage.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <ostream>
#define _USE_MATH_DEFINES
#include <math.h>
#define RENDER
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

struct AmShader : public IShader {
  bool fragment(const vec3 bar, TGAColor &color) const {
    vec2 uv = renderData.varying_uv * bar;
    vec3 p = renderData.ndc_tri * bar;
    if (abs(renderData.shadow->get(uv.x * (renderData.shadow->width() - 1),
                                   uv.y *
                                       (renderData.shadow->height() - 1))[0] -
            p.z) < 50) {
      renderData.occlImage->set(uv.x * (renderData.occlImage->width() - 1),
                                uv.y * (renderData.occlImage->height() - 1),
                                TGAColor{255});
    }
  }
};

struct OcShader : public IShader {
  bool fragment(const vec3 bar, TGAColor &color) const {
    vec2 uv = renderData.varying_uv * bar;
    auto gg = renderData.occlImage->get(
        uv.x * (renderData.occlImage->width() - 1),
        uv.y * (renderData.occlImage->height() - 1))[0];
    color = TGAColor{gg, gg, gg};

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
        0.7 * (renderData.shadow->get(
                   std::max(0, std::min(int(pS.x / pS.w),
                                        renderData.shadow->width() - 1)),
                   std::max(0, std::min(int(pS.y / pS.w),
                                        renderData.shadow->height() - 1)))[0] <
               pS.z + 40);

    vec3 e1 = renderData.world_tri[1] - renderData.world_tri[0];
    vec3 e2 = renderData.world_tri[2] - renderData.world_tri[0];

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

float max_elevation_angle(TGAImage *zbuffer, vec2 p, vec2 dir) {
  float maxangle = 0;
  for (float t = 0.; t < 1000.; t += 1.) {
    vec2 cur = p + dir * t;
    if (cur.x >= zbuffer->width() || cur.y >= zbuffer->height() || cur.x < 0 ||
        cur.y < 0)
      return maxangle;

    float distance = norm(p - cur);
    if (distance < 1.f)
      continue;
    float elevation = zbuffer->get(cur.x, cur.y)[0] - zbuffer->get(p.x, p.y)[0];
    maxangle = std::max(maxangle, atanf(elevation / distance));
  }
  return maxangle;
}

vec3 rand_point() {
  auto u = (float)rand() / (float)RAND_MAX;
  auto v = (float)rand() / (float)RAND_MAX;
  auto theta = 2.f * M_PI * u;
  auto phi = acos(2.f * v - 1.f);
  return vec3{sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)};
}

int main(int, char **) {
  auto renderer = new SoftRenderer();

  auto model = new Model("obj/african_head/african_head.obj");
  renderer->append_model(model);
  model = new Model("obj/african_head/african_head_eye_inner.obj");
  renderer->append_model(model);

  renderer->center = {0, 0, 0};
  renderer->light = {1, 0, 1};
  renderer->eye = renderer->light;
  renderer->corner = {100, 100};
  renderer->size = {600, 600, 255};
  renderer->imageSize = {800, 800};

  GouraudShader shader1;
  Shader shader2;
  DepthShader depthShader;
  AmShader amshader;

#ifdef RENDER
#pragma region
  renderer->init(false);
  auto start = std::chrono::high_resolution_clock::now();
  renderer->render(depthShader);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  std::cout << "渲染时间: " << duration.count() << " 秒" << std::endl;
  renderer->write_zimage("depth.tga");
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
#pragma endregion
#endif

#ifdef OCCLCAL
#pragma region
  auto occl = new TGAImage{1024, 1024, TGAImage::GRAYSCALE};
  auto total = new TGAImage{1024, 1024, TGAImage::GRAYSCALE};
  renderer->setOccl(occl, total);
  const int nrenders = 2000;
  for (int iter = 1; iter <= nrenders; iter++) {
    std::cout << iter << std::endl;
    for (int i = 0; i < 3; i++)
      renderer->up[i] = (float)rand() / (float)RAND_MAX;
    renderer->eye = rand_point();
    renderer->eye.y = abs(renderer->eye.y);
    renderer->init(false);
    renderer->render(depthShader);
    auto zimage = renderer->get_zImage();
    auto curMat = renderer->getCurCompoundMatrix();
    renderer->setShadowInfo(&zimage, curMat);
    renderer->init(false);
    renderer->render(amshader);
    for (int j = 0; j < total->width(); j++) {
      for (int k = 0; k < total->height(); k++) {
        auto tmp = total->get(j, k)[0];
        total->set(
            j, k,
            TGAColor{uint8_t(
                (tmp * (iter - 1) + occl->get(j, k)[0]) / float(iter) + .5f)});
      }
    }
  }
  total->flip_vertically();
  total->write_tga_file("occlusion.tga");
  occl->flip_vertically();
  occl->write_tga_file("occl.tga");
  delete occl;
  delete total;
#pragma endregion
#endif

#ifdef OCCLRENDER
#pragma region
  OcShader occ;
  auto newocl = new TGAImage();
  newocl->read_tga_file("occl.tga");
  renderer->setOccl(newocl, newocl);
  renderer->eye = {2, 1, 4};
  renderer->init(true);
  renderer->render(occ);
  renderer->write_image("output.tga");
#pragma endregion
#endif

#ifdef SIMPLEOCCLCAL
#pragma region simpleOc
  renderer->eye = {2, 1, 4};
  renderer->init(true);
  renderer->render(depthShader);
  auto zimage = renderer->get_zImage();
  TGAImage frame(800, 800, TGAImage::RGB);
  for (int j = 0; j < frame.width(); j++) {
    std::cout << j << std::endl;
    for (int k = 0; k < frame.height(); k++) {
      if (zimage.get(j, k)[0] < -1e5)
        continue;
      float total = 0;
      for (float a = 0; a < M_PI * 2 - 1e-4; a += M_PI / 4) {
        total +=
            M_PI / 2 - max_elevation_angle(&zimage, vec2{(double)j, (double)k},
                                           vec2{cos(a), sin(a)});
      }
      total /= (M_PI / 2) * 8;
      total = pow(total, 100.f);
      frame.set(j, k,
                TGAColor{uint8_t(total * 255), uint8_t(total * 255),
                         uint8_t(total * 255)});
    }
  }
  frame.flip_vertically();
  frame.write_tga_file("output.tga");
#pragma endregion
#endif

  delete renderer;
}
