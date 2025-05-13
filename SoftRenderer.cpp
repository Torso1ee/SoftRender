#include "SoftRenderer.h"
#include <algorithm>
#include <ostream>
#include <stdexcept>

SoftRenderer::~SoftRenderer() {
  delete zbuffer;
  delete image;
  for (auto model : scene) {
    delete model;
  }
}

void SoftRenderer::append_model(Model *model) { scene.push_back(model); }

void SoftRenderer::setShadowInfo(TGAImage *map, mat<4, 4> shadowM) {
  shadowImage = map;
  shadow = shadowM;
}

void SoftRenderer::init(bool proj) {
  if (!inited) {
    camera = eye - center;
    modelView = lookat(eye, center, up);
    projection = project(proj ? -1.f / norm(camera - center) : 0);
    viewport = view(corner.x, corner.y, size.x, size.y, size.z);
    zbuffer = new TGAImage(imageSize.x, imageSize.y, TGAImage::GRAYSCALE);
    image = new TGAImage(imageSize.x, imageSize.y, TGAImage::RGB);
    light = normalized(light);
    shadow = shadow * getCurCompoundMatrix().invert();
    inited = true;
  } else {
    throw std::runtime_error("init twice!");
  }
}

void SoftRenderer::render(IShader &shader) {
  if (!inited) {
    throw std::runtime_error("not inited!");
  } else {
    auto cmat = getCurCompoundMatrix();
    shader.renderData.depth = size.z;
    shader.renderData.viewport = viewport;
    shader.renderData.projection = projection;
    shader.renderData.modelView = modelView;
    shader.renderData.light = light;
    shader.renderData.shadow = shadowImage;
    shader.renderData.normalMat = modelView.invert_transpose();
    shader.renderData.viewDir = normalized(camera);
    for (auto model : scene) {
      shader.renderData.model = model;
      for (int i = 0; i < model->nfaces(); i++) {
        vec3 screen_coords[3];
        vec2 uv;
        for (int j = 0; j < 3; j++) {
          vec3 v = model->vert(i, j);
          vec3 nrm = model->normal(i, j);
          shader.renderData.varying_normal[j] = nrm;
          shader.renderData.varying_intensity[j] =
              std::max(0., model->normal(i, j) * light);
          uv = model->uv(i, j);
          shader.renderData.varying_uv[0][j] = uv.x;
          shader.renderData.varying_uv[1][j] = uv.y;
          vec4 vl{v.x, v.y, v.z, 1};
          vl = cmat * vl;
          screen_coords[j].x = int(vl.x / vl.w);
          screen_coords[j].y = int(vl.y / vl.w);
          screen_coords[j].z = int(vl.z / vl.w);
          shader.renderData.ndc_tri[j] = screen_coords[j];
        }
        triangle(screen_coords, shader, image, zbuffer);
      }
    }
  }
}

mat<4, 4> SoftRenderer::getCurCompoundMatrix() {
  viewport *projection *modelView;
}

void SoftRenderer::write_image(const char *path) {
  if (!inited) {
    throw std::runtime_error("not inited!");
  } else {
    image->write_tga_file(path);
  }
}

TGAImage SoftRenderer::get_zImage() { return *zbuffer; }

void SoftRenderer::write_zimage(const char *path) {
  if (!inited) {
    throw std::runtime_error("not inited!");
  } else {
    zbuffer->write_tga_file(path);
  }
}