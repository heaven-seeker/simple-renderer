#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

#include "color.h"
#include "RenderPasses.h"
#include "Shaders.h"
#include "our_gl.h"

// Kept for single flags like -f, -w
std::string get_cmd_option(char **begin, char **end, const std::string &option, const std::string &default_value = "") {
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return std::string(*itr);
    }
    return default_value;
}

// Collects all arguments after a flag until another flag is encountered
std::vector<std::string> get_cmd_multiple_options(char **begin, char **end, const std::string &option) {
    std::vector<std::string> results;
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        while (itr != end && (*itr)[0] != '-') {
            results.push_back(std::string(*itr));
            ++itr;
        }
    }
    return results;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char** argv) {

    // 1. Check for help flag
    if (cmd_option_exists(argv, argv + argc, "-h") || cmd_option_exists(argv, argv + argc, "--help")) {
        std::cout << "Usage: " << argv[0] << " [options]\n"
                  << "Options:\n"
                  << "  -m <paths...>   Paths to one or MORE main models (.obj)\n"
                  << "  -c <colors...>  Colors for the main models (e.g., 'gold', 'red', 'white')\n"
                  << "  -f <path>       Path to the floor model (.obj)\n"
                  << "  -fc <color>     Color for the floor model (default: blue)\n"
                  << "  -l <x> <y> <z>  Light direction vector (default: -1.5 0.5 1.5)\n"
                  << "  -w <width>      Window width (default: 2048)\n"
                  << "  -s <height>     Window height (default: 2048)\n"
                  << "  -ao <type>      AO method: 'ssao', 'bruteforce', or 'none' (default: ssao)\n"
                  << "  -shader <type>  Shader type: 'toon', 'gouraud', 'phong', 'simple', 'wireframe',\n"
                  << "                  'solid', 'random', 'normalmap', 'geometry', 'tangentspace' (default: toon)\n";
        return 0;
    }

    // 2. Parse arguments
    // Parse main model paths (fallback to diablo3 if none provided)
    std::vector<std::string> model_paths = get_cmd_multiple_options(argv, argv + argc, "-m");
    if (model_paths.empty()) {
        model_paths.push_back("obj/diablo3_pose/diablo3_pose.obj");
    }

    // Parse floor path, AO method, and shader type
    std::string floor_path = get_cmd_option(argv, argv + argc, "-f", "obj/floor.obj");
    std::string ao_type    = get_cmd_option(argv, argv + argc, "-ao", "ssao");
    std::string shader_type = get_cmd_option(argv, argv + argc, "-shader", "toon");

    // Parse and convert colors for models and floor
    std::vector<std::string> color_names = get_cmd_multiple_options(argv, argv + argc, "-c");
    std::string floor_color_name = get_cmd_option(argv, argv + argc, "-fc", "blue");

    std::vector<TGAColor> model_colors;
    for (const auto& name : color_names) {
        model_colors.push_back(get_color_by_name(name));
    }

    if (model_colors.empty()) {
        model_colors.push_back(orange); // Default color
    }

    TGAColor floor_color = get_color_by_name(floor_color_name);

    // Parse light direction vector (x, y, z) and normalize it
    std::vector<std::string> light_args = get_cmd_multiple_options(argv, argv + argc, "-l");
    geom::vec3 light_dir(-1.5, 0.5, 1.5);
    if (light_args.size() >= 3) {
        try {
            light_dir.x = std::stod(light_args[0]);
            light_dir.y = std::stod(light_args[1]);
            light_dir.z = std::stod(light_args[2]);
        } catch (...) {
            std::cout << "Warning: Invalid light direction arguments, using default.\n";
            light_dir = geom::vec3(-1.5, 0.5, 1.5);
        }
    }
    light_dir = geom::normalize(light_dir);

    // Parse window dimensions
    int width = std::stoi(get_cmd_option(argv, argv + argc, "-w", "2048"));
    int height = std::stoi(get_cmd_option(argv, argv + argc, "-s", "2048"));

    std::cout << "Initializing renderer (" << width << "x" << height << ")...\n";

    // Initialize renderer, buffers, and load 3D models into memory
    Pipeline pipeline(width, height);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    Model floor(floor_path);

    std::vector<Model*> scene_models;
    for (const auto& path : model_paths) {
        std::cout << "Loading model: " << path << "\n";
        scene_models.push_back(new Model(path));
    }

    // 3. Ambient Occlusion Pass
    std::vector<double> final_ao_buffer;

    if (ao_type == "ssao") {
        std::cout << "Method: SSAO (Screen Space Ambient Occlusion)\n";
        std::vector<double> raw_ao = compute_SSAO(width, height, scene_models, floor, pipeline);
        std::cout << "Applying Blur to SSAO...\n";
        final_ao_buffer = apply_ssao_blur(raw_ao, width, height);
    }
    else if (ao_type == "bruteforce") {
        std::cout << "Method: Brute-force Raycasting (Reference AO)\n";
        final_ao_buffer = compute_AO_bruteforce(width, height, scene_models, floor, pipeline);
    }
    else {
        std::cout << "Method: None (AO disabled)\n";
        final_ao_buffer = std::vector<double>(width * height, 1.0);
    }

    // 4. Main Rendering Pass
    std::cout << "Rendering scene geometry with '" << shader_type << "' shader...\n";
    render_scene(framebuffer, pipeline, scene_models, floor, shader_type, model_colors, floor_color, light_dir);

    // 5. Post-Processing Pass
    bool enable_outline = (shader_type == "toon");
    std::cout << "Applying Sobel outlines and blending AO shadows...\n";
    apply_post_processing(framebuffer, pipeline, final_ao_buffer, enable_outline);

    // 6. Output
    std::cout << "Saving files...\n";
    pipeline.save_zbuffer("zbuffer_image.tga");
    framebuffer.write_tga_file("framebuffer.tga");

    for (Model* m : scene_models) {
        delete m;
    }

    std::cout << "Render complete!\n";
    return 0;
}