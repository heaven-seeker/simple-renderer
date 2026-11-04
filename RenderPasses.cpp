#include "RenderPasses.h"
#include <random>
#include <limits>
#include <cmath>
#include <iostream>

#include "tgaimage.h"
#include "Model.h"
#include "our_gl.h"
#include "Shaders.h"
#include "color.h"

// Global camera and light settings
const geom::vec3 eye(1, 1, 3);
const geom::vec3 center(0, 0, 0);
const geom::vec3 up(0, 1, 0);


std::vector<double> compute_AO_bruteforce(int width, int height, const std::vector<Model*>& models, Model& floor, Pipeline& pipeline) {
    std::vector<double> ao_buffer(width * height, 0.0);
    TGAImage dummy_fb(width, height, TGAImage::GRAYSCALE);

    // Generate 1000 random light directions for brute-force raycasting
    std::vector<geom::vec3> light_dirs(1000);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> distribution(-1.0, 1.0);

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 3; j++) {
            auto value = distribution(gen);
            light_dirs[i][j] = value;
        }
        if (norm(light_dirs[i]) > 1.0) {
            i--; // Discard vectors outside the unit sphere
        } else {
            // Keep vectors pointing upwards (hemisphere)
            light_dirs[i].y = light_dirs[i].y < 0. ? light_dirs[i].y * -1 : light_dirs[i].y;
            light_dirs[i] = normalize(light_dirs[i]);
        }
    }

    // Initialize camera perspective
    pipeline.lookat(eye, center, up);
    pipeline.init_perspective(norm(eye - center));
    pipeline.init_viewport(0, 0, width, height);
    pipeline.clear_zbuffer();

    // Render depth map from the camera's point of view for ALL models
    for (Model* m : models) {
        DepthShader depth_shader_model(*m, pipeline.ModelView, pipeline.Perspective);
        m->draw_model(dummy_fb, depth_shader_model, pipeline);
    }

    // Render the floor depth
    DepthShader depth_shader_floor(floor, pipeline.ModelView, pipeline.Perspective);
    floor.draw_model(dummy_fb, depth_shader_floor, pipeline);

    // Save camera Z-buffer to compare against shadow maps later
    std::vector<double> camera_zbuffer = pipeline.zbuffer;
    geom::matrix<4, 4> M_camera_full = pipeline.Viewport * pipeline.Perspective * pipeline.ModelView;
    geom::matrix<4, 4> M_camera_inv = M_camera_full.inverse();

    for (int i = 0; i < 1000; i++ ) {
        geom::vec3 light_pos = light_dirs[i] * 3.0;

        // Render scene from the light's perspective
        pipeline.lookat(light_pos, center, up);
        pipeline.init_perspective(norm(light_pos - center));
        pipeline.init_viewport(0, 0, width, height);
        pipeline.clear_zbuffer();

        // Draw all models to the shadow map
        for (Model* m : models) {
            DepthShader depth_shader_model(*m, pipeline.ModelView, pipeline.Perspective);
            m->draw_model(dummy_fb, depth_shader_model, pipeline);
        }
        floor.draw_model(dummy_fb, depth_shader_floor, pipeline);

        geom::matrix<4, 4> M_light_full = pipeline.Viewport * pipeline.Perspective * pipeline.ModelView;
        geom::matrix<4, 4> M_screen_to_light = M_light_full * M_camera_inv;

        // Check each pixel against the shadow map to accumulate AO
        #pragma omp parallel for
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                double z = camera_zbuffer[x + y * width];
                if (z == -std::numeric_limits<double>::max()) continue;

                geom::vec4 frag_screen(x, y, z, 1.0);
                geom::vec4 frag_light = M_screen_to_light * frag_screen;
                frag_light.x = frag_light.x / frag_light.w;
                frag_light.y = frag_light.y / frag_light.w;
                frag_light.z = frag_light.z / frag_light.w;

                if (frag_light.x < 0 || frag_light.x >= width || frag_light.y < 0 || frag_light.y >= height) continue;

                int light_idx = (int)frag_light.x + (int)frag_light.y * width;
                double shadow_z = pipeline.zbuffer[light_idx];

                if (frag_light.z > shadow_z - 0.01) {
                    // Use atomic to prevent race conditions during parallel execution
                    #pragma omp atomic
                    ao_buffer[x + y * width] += 1.0;
                }
            }
        }
    }

    // Normalize AO buffer and output image
    for (size_t i = 0; i < ao_buffer.size(); i++) {
        ao_buffer[i] /= 1000.0;
    }

    TGAImage ao(width, height, TGAImage::GRAYSCALE);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            ao.set(x, y, {static_cast<unsigned char>(ao_buffer[x + y * width] * 255.0)});
        }
    }
    ao.write_tga_file("ao_buffer.tga");

    return ao_buffer;
}

void visualize_ssao_kernel(const std::vector<geom::vec3>& kernel) {
    int size = 512;
    TGAImage viz(size, size, TGAImage::RGB);

    for(int i=0; i<size; i++) {
        viz.set(i, size/2, TGAColor{100, 100, 100, 255});
        viz.set(size/2, i, TGAColor{100, 100, 100, 255});
    }

    for (const auto& sample : kernel) {
        int x = (sample.x + 1.0) * (size / 2);
        int z = (sample.z + 0.0) * (size / 1.0);

        for(int dx=-2; dx<=2; dx++) {
            for(int dz=-2; dz<=2; dz++) {
                if (x+dx >= 0 && x+dx < size && (size-z)+dz >= 0 && size-z+dz < size)
                    viz.set(x + dx, (size - z) + dz, TGAColor{255, 255, 255, 255});
            }
        }
    }
    viz.write_tga_file("kernel_viz.tga");
}

std::vector<double> compute_SSAO(int width, int height, const std::vector<Model*>& models, Model& floor, Pipeline& pipeline) {
    std::vector<geom::vec3> ssao_kernel;
    ssao_kernel.reserve(128);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> distribution(0, 1.0);
    auto lerp = [](double a, double b, double f) { return a + f * (b - a); };

    // Generate hemisphere kernel samples
    for (int i = 0; i < 128; i++) {
        geom::vec3 sample(distribution(gen) * 2.0 - 1.0, distribution(gen) * 2.0 - 1.0, distribution(gen));
        sample = normalize(sample);
        sample = sample * distribution(gen);
        double scale = double(i) / 128.0;
        scale = lerp(0.1, 1.0, scale * scale);
        sample = sample * scale;
        ssao_kernel.push_back(sample);
    }

    visualize_ssao_kernel(ssao_kernel);

    // Generate random noise to rotate the kernel
    std::vector<geom::vec3> ssao_noise;
    ssao_noise.reserve(16);
    for (int i = 0; i < 16; i++) {
        geom::vec3 noise(distribution(gen) * 2.0 - 1.0, distribution(gen) * 2.0 - 1.0, 0.0);
        ssao_noise.push_back(noise);
    }

    // Setup camera
    pipeline.lookat(eye, center, up);
    pipeline.init_perspective(norm(eye - center));
    pipeline.init_viewport(0, 0, width, height);
    pipeline.clear_zbuffer();

    TGAImage normal_fb(width, height, TGAImage::RGB);

    // Render floor normals
    GeometryShader geom_shader_floor(floor, pipeline.ModelView, pipeline.Perspective);
    floor.draw_model(normal_fb, geom_shader_floor, pipeline);

    // Render normals for ALL models in the scene
    for (Model* m : models) {
        GeometryShader geom_shader_model(*m, pipeline.ModelView, pipeline.Perspective);
        m->draw_model(normal_fb, geom_shader_model, pipeline);
    }

    std::vector<double> ao_buffer(width * height, 0.0);
    double radius = 1.5;
    geom::matrix<4, 4> M_inv = (pipeline.Viewport * pipeline.Perspective).inverse();

    // Calculate SSAO per pixel using OpenMP for performance
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = x + y * width;
            double z_screen = pipeline.zbuffer[idx];

            if (z_screen == -std::numeric_limits<double>::max()) {
                ao_buffer[idx] = 1.0;
                continue;
            }

            // Unproject from screen space to view space
            geom::vec4 screen_coord(x, y, z_screen, 1.0);
            geom::vec4 view_coord = M_inv * screen_coord;
            geom::vec3 frag_pos(
                view_coord.x / view_coord.w,
                view_coord.y / view_coord.w,
                view_coord.z / view_coord.w
            );

            TGAColor c = normal_fb.get(x, y);

            // Unpack normal from color
            geom::vec3 normal(
                c.bgra[2] / 127.5 - 1.0,
                c.bgra[1] / 127.5 - 1.0,
                c.bgra[0] / 127.5 - 1.0
            );

            normal = geom::normalize(normal);

            if (norm(normal) < 0.1) {
                ao_buffer[idx] = 1.0;
                continue;
            }

            // Create TBN matrix for orienting the kernel
            int noise_x = x % 4;
            int noise_y = y % 4;
            geom::vec3 random_vec = ssao_noise[noise_x + noise_y * 4];

            geom::vec3 t = geom::normalize(random_vec - normal * dot(random_vec, normal));
            geom::vec3 b = cross(normal, t);
            geom::matrix<3, 3> TBN = {{{t.x, b.x, normal.x}, {t.y, b.y, normal.y}, {t.z, b.z, normal.z}}};

            double occlusion = 0.0;

            for (int i = 0; i < 128; i++) {
                geom::vec3 ssao_vec = TBN * ssao_kernel[i];
                geom::vec3 sample_pos = frag_pos + ssao_vec * radius;

                // Project sample back to screen space
                geom::vec4 clip = pipeline.Perspective * geom::vec4{sample_pos.x, sample_pos.y, sample_pos.z, 1.0};
                geom::vec4 ndc = geom::vec4(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w, 1.0);

                geom::vec4 screen = pipeline.Viewport * ndc;
                int sx = (int)screen.x;
                int sy = (int)screen.y;

                if (sx < 0 || sx >= width || sy < 0 || sy >= height) continue;

                double shadow_z_screen = pipeline.zbuffer[sx + sy * width];
                if (shadow_z_screen == -std::numeric_limits<double>::max()) continue;

                geom::vec4 shadow_screen_coord(sx, sy, shadow_z_screen, 1.0);
                geom::vec4 shadow_view_coord = M_inv * shadow_screen_coord;
                double z_pos = shadow_view_coord.z / shadow_view_coord.w;

                // Accumulate occlusion if the sample is behind geometry
                if (sample_pos.z < z_pos - 0.01 && std::abs(frag_pos.z - z_pos) < radius) {
                    occlusion += 1.0;
                }
            }

            occlusion = 1.0 - (occlusion / 128.0);
            ao_buffer[idx] = occlusion;
        }
    }

    // Dump raw SSAO buffer for debugging
    TGAImage ao_image(width, height, TGAImage::GRAYSCALE);

    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int color_val = std::max(0, std::min(255, (int)(ao_buffer[x + y * width] * 255.0)));
            ao_image.set(x, y, TGAColor({static_cast<unsigned char>(color_val)}));
        }
    }
    ao_image.write_tga_file("ssao_raw.tga");

    return ao_buffer;
}

std::vector<double> apply_ssao_blur(const std::vector<double>& ao_buffer, int width, int height) {
    std::vector<double> blurred_ao(width * height, 0.0);

    // Apply a simple 4x4 box blur to smooth out the noise
    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double result = 0.0;
            int count = 0;

            for (int dy = -2; dy < 2; dy++) {
                for (int dx = -2; dx < 2; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        result += ao_buffer[nx + ny * width];
                        count++;
                    }
                }
            }
            blurred_ao[x + y * width] = result / (double)count;
        }
    }
    return blurred_ao;
}

// Applies Sobel edge detection (if enabled) and blends the SSAO shadows into the framebuffer.
void apply_post_processing(TGAImage& framebuffer, const Pipeline& pipeline, const std::vector<double>& blurred_ao, bool enable_outline) {
    int width = framebuffer.width();
    int height = framebuffer.height();
    TGAColor shadow_tint{150, 0, 100, 255}; // Deep purple tint for shadows

    #pragma omp parallel for
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int idx = x + y * width;

            if (pipeline.zbuffer[idx] == -std::numeric_limits<double>::max()) {
                continue;
            }

            TGAColor color = framebuffer.get(x, y);

            // Sobel edge detection using the depth buffer
            if (enable_outline) {
                double tl = pipeline.zbuffer[(x - 1) + (y - 1) * width];
                double tc = pipeline.zbuffer[x       + (y - 1) * width];
                double tr = pipeline.zbuffer[(x + 1) + (y - 1) * width];

                double ml = pipeline.zbuffer[(x - 1) + y * width];
                double mc = pipeline.zbuffer[x       + y * width];
                double mr = pipeline.zbuffer[(x + 1) + y * width];

                double bl = pipeline.zbuffer[(x - 1) + (y + 1) * width];
                double bc = pipeline.zbuffer[x       + (y + 1) * width];
                double br = pipeline.zbuffer[(x + 1) + (y + 1) * width];

                double gx = (tr - tl) + 2.0 * (mr - ml) + (br - bl);
                double gy = (bl - tl) + 2.0 * (bc - tc) + (br - tr);
                double edge = std::sqrt(gx * gx + gy * gy);

                // Draw outline
                if (edge > 0.05) {
                    framebuffer.set(x, y, black);
                    continue;
                }
            }

            // Blend SSAO shadows into the final color
            double light_factor = 0.4 + 0.6 * blurred_ao[x + y * width];
            double shadow_factor = 1.0 - light_factor;
            color = color * light_factor + shadow_tint * shadow_factor;

            framebuffer.set(x, y, color);
        }
    }
}

void render_scene(TGAImage& framebuffer, Pipeline& pipeline, const std::vector<Model*>& models, Model& floor, const std::string& shader_type,
    const std::vector<TGAColor>& model_colors, TGAColor floor_color, geom::vec3 light_dir) {
    int width = framebuffer.width();
    int height = framebuffer.height();

    const geom::vec3 light_dir_world = light_dir;

    // Clear screen to background color
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            framebuffer.set(i, j, black);
        }
    }

    pipeline.clear_zbuffer();
    pipeline.lookat(eye, center, up);
    pipeline.init_perspective(norm(eye - center));
    pipeline.init_viewport(0, 0, width, height);

    // Helper lambda to render a model with the selected shader
    // We use a lambda to avoid duplicating the massive if-else block for the floor and scene models
    auto draw_with_shader = [&](Model& m, TGAColor color) {
        if (shader_type == "phong") {
            PhongShading shader(light_dir_world, m, color, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "gouraud") {
            GouraudShader shader(m, light_dir_world, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "tangentspace") {
            std::vector<double> dummy_ao;
            NormalTangentSpace shader(light_dir_world, m, dummy_ao, width, pipeline.ModelView, pipeline.Perspective, pipeline.Viewport);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "simple") {
            MySimpleShader shader(color, m, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "wireframe") {
            WireframeShader shader(m, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "solid") {
            SolidShader shader(m, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "random") {
            RandomShader shader(m, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "normalmap") {
            NormalMappingShader shader(light_dir_world, m, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else if (shader_type == "geometry") {
            GeometryShader shader(m, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        } else {
            // Default fallback is Toon Shader
            ToonShader shader(m, light_dir_world, color, pipeline.ModelView, pipeline.Perspective);
            m.draw_model(framebuffer, shader, pipeline);
        }
    };

    // Render floor geometry
    draw_with_shader(floor, floor_color);

    // Render all scene models
    for (size_t i = 0; i < models.size(); i++) {
        TGAColor model_color = (i < model_colors.size()) ? model_colors[i] : orange;
        draw_with_shader(*models[i], model_color);
    }
}