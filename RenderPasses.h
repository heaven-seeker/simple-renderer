#pragma once

#include <vector>
#include "Geometry.h"

struct TGAColor;
class Model;
class Pipeline;
struct TGAImage;

// Computes Ambient Occlusion using a brute-force raycasting approach for multiple models.
// Extremely slow, used mainly as a reference for ground-truth AO.
std::vector<double> compute_AO_bruteforce(int width, int height, const std::vector<Model*>& models, Model& floor, Pipeline& pipeline);

// Renders the generated SSAO sample kernel to an image for debugging purposes.
void visualize_ssao_kernel(const std::vector<geom::vec3>& kernel);

// Computes Screen Space Ambient Occlusion (SSAO) using the depth buffer for multiple models.
// Generates a hemisphere of samples and random rotation noise to estimate lighting occlusion.
std::vector<double> compute_SSAO(int width, int height, const std::vector<Model*>& models, Model& floor, Pipeline& pipeline);

// Applies a 4x4 box blur to the raw SSAO buffer to eliminate noise artifacts and grain.
std::vector<double> apply_ssao_blur(const std::vector<double>& ao_buffer, int width, int height);

// Applies Sobel edge detection for a comic-book outline effect and blends the SSAO shadows into the framebuffer.
void apply_post_processing(TGAImage& framebuffer, const Pipeline& pipeline, const std::vector<double>& blurred_ao, bool enable_outline = true);

// Clears the depth buffer and renders the main scene geometry using the selected shader.
void render_scene(TGAImage& framebuffer, Pipeline& pipeline, const std::vector<Model*>& models, Model& floor,
    const std::string& shader_type, const std::vector<TGAColor>& model_colors, TGAColor floor_color, geom::vec3 light_dir);