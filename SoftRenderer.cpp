#include "SoftRenderer.h"
#include <algorithm>
#include <ostream>
#include <stdexcept>

SoftRenderer::~SoftRenderer() {
  delete zbuffer;
  delete image;
}

void SoftRenderer::append_model(Model *model) { scene.push_back(model); }

void SoftRenderer::init() {
  if (!inited) {
    camera = eye - center;
    modelView = lookat(eye, center, up);
    projection = project(-1.f / norm(camera - center));
    viewport = view(corner.x, corner.y, size.x, size.y, size.z);
    zbuffer = new TGAImage(imageSize.x, imageSize.y, TGAImage::GRAYSCALE);
    image = new TGAImage(imageSize.x, imageSize.y, TGAImage::RGB);
    light = normalized(light);
    inited = true;
  } else {
    throw std::runtime_error("init twice!");
  }
}

void SoftRenderer::render(IShader &shader) {
  if (!inited) {
    throw std::runtime_error("not inited!");
  } else {
    for (auto model : scene) {
      for (int i = 0; i < model->nfaces(); i++) {
        vec3 screen_coords[3];

        for (int j = 0; j < 3; j++) {
          vec3 v = model->vert(i, j);
          shader.varying_intensity[j] =
              std::max(0., model->normal(i, j) * light);
          vec4 vl{v.x, v.y, v.z, 1};
          vl = viewport * projection * modelView * vl;
          screen_coords[j].x = int(vl.x / vl.w);
          screen_coords[j].y = int(vl.y / vl.w);
          screen_coords[j].z = int(vl.z / vl.w);
        }
        triangle(screen_coords, shader, image, zbuffer);
      }
    }
  }
}

void SoftRenderer::write_image(const char *path) {
  if (!inited) {
    throw std::runtime_error("not inited!");
  } else {
    zbuffer->write_tga_file(path);
  }
}
void SoftRenderer::write_Zimage(const char *path) {
  if (!inited) {
    throw std::runtime_error("not inited!");
  } else {
    zbuffer->write_tga_file(path);
  }
}