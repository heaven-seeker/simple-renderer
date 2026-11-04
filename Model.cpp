#include "Model.h"

#include <algorithm>
#include <filesystem>
#include <random>
#include <fstream>
#include <string>
#include <sstream>
#include "color.h"
#include "Geometry.h"
#include "rasterizer.h"
#include "our_gl.h"

namespace fs = std::filesystem;

constexpr size_t POINT_SIZE = 1;

std::ostream& operator<<(std::ostream& s, const geom::vec<3>& v) {
    s << "(" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
    return s;
}

std::ostream& operator<<(std::ostream& s, const Face& face) {

    for (int i = 0; i < 3; i++) {
        s << face.corners[i].v << "/" << face.corners[i].t << "/" << face.corners[i].n;
        if (i != 2) {
            s << " ";
        }
    }
    return s;
}

Model::Model(const std::string& fileName) {
    std::string line;
    std::string digits = "-0123456789";

    std::string nm_filename = fileName.substr(0, fileName.find_last_of('.')) + "_nm.tga";
    std::string diffuse_filename = fileName.substr(0, fileName.find_last_of('.')) + "_diffuse.tga";
    std::string specular_filename = fileName.substr(0, fileName.find_last_of('.')) + "_spec.tga";
    std::string glow_filename = fileName.substr(0, fileName.find_last_of('.')) + "_glow.tga";
    std::string normaltangentmap_filename = fileName.substr(0, fileName.find_last_of('.')) + "_nm_tangent.tga";

    if (fs::exists(diffuse_filename)) {
        diffusemap.read_tga_file(diffuse_filename);
        diffusemap.flip_vertically();
    }

    if (fs::exists(nm_filename)) {
        normalmap.read_tga_file(nm_filename);
        normalmap.flip_vertically();
    }

    if (fs::exists(specular_filename)) {
        specularmap.read_tga_file(specular_filename);
        specularmap.flip_vertically();
    }

    if (fs::exists(normaltangentmap_filename)) {
        normaltangentmap.read_tga_file(normaltangentmap_filename);
        normaltangentmap.flip_vertically();
    }

    if (fs::exists(glow_filename)) {
        glowmap.read_tga_file(glow_filename);
        glowmap.flip_vertically();
    }

    std::ifstream in(fileName);

    if (in.is_open())
    {
        while (std::getline(in, line)) {
            if (line[0] == '#') {
                continue;
            }
            if (line[0] == 'v' && line[1] == ' ') {
                long unsigned int index = line.find_first_of(digits);

                if (index != std::string::npos) {
                    line = line.substr(index);
                }
                std::istringstream coords(line);

                float xt, yt, zt;
                coords >> xt >> yt >> zt;

                verts.push_back(geom::vec<3>({xt, yt, zt}));
            } else if (line[0] == 'f' && line[1] == ' ') {
                long unsigned int index = line.find_first_of(digits);

                if (index != std::string::npos) {
                    line = line.substr(index);
                }

                std::istringstream iss(line);
                std::string str;

                Face face = Face();
                int i = 0;
                while (iss >> str && i < 3) {

                    int v, t, n;

                    int result = sscanf(str.c_str(), "%d/%d/%d", &v, &t, &n);

                    if (result == 3) {
                        face.corners[i].v = v - 1;
                        face.corners[i].t = t - 1;
                        face.corners[i].n = n - 1;
                    }
                    
                    i++;
                }

                faces.push_back(face);
                
            } else if (line.substr(0, 2) == "vt") {

                long unsigned int index = line.find_first_of(digits);

                if (index != std::string::npos) {
                    line = line.substr(index);
                }

                std::stringstream coords(line);
                float x, y, z;
                coords >> x >> y >> z;

                tex_coords.push_back(geom::vec<3>({x, y, z}));
            } else if (line.substr(0, 2) == "vn") {
                long unsigned int index = line.find_first_of(digits);

                if (index != std::string::npos) {
                    line = line.substr(index);
                }

                std::stringstream coords(line);
                float x, y, z;
                coords >> x >> y >> z;

                normals.push_back(geom::vec<3>({x, y, z}));
            }

        }
    }
    in.close();
}

void Model::draw_model(TGAImage& framebuffer, IShader& shader, Pipeline& pipeline) const {
    for (size_t i = 0; i < faces.size(); i++) {
        geom::vec4 clip_coords[3];
        geom::vec3 world_coords[3];

        for (int j = 0; j < 3; j++) {
            clip_coords[j] = shader.vertex(i, j);
            world_coords[j] = vert(i, j);
        }

        triangle_barycentric_bounding_box(clip_coords, framebuffer, shader, pipeline);
    }
}

void Model::draw_points(TGAImage& framebuffer, TGAColor color, Pipeline& pipeline) const {
    geom::matrix<4, 4> T = pipeline.Viewport * pipeline.Perspective * pipeline.ModelView;

    for (size_t i = 0; i < verts.size(); i++) {
        geom::vec<4> v({verts[i].x, verts[i].y, verts[i].z, 1.0});

        v = T * v;

        v.x /= v.w;
        v.y /= v.w;
        v.z /= v.w;

        draw_fat_point(v.x, v.y, framebuffer, color, POINT_SIZE);
    }
}

geom::vec3 Model::vert(int face, int vert) const {
    return verts[faces[face].corners[vert].v];
}

geom::vec3 Model::normal(int face, int vert) const {
    return normals[faces[face].corners[vert].n];
}

geom::vec2 Model::uv(int face, int vert) const {
    geom::vec3 temp = tex_coords[faces[face].corners[vert].t];
    geom::vec2 uv = {temp.x, temp.y};
    return uv;
}

geom::vec3 Model::normal_from_map(geom::vec2 uv) const {
    if (normalmap.width() == 0) return geom::vec3(0, 0, 1);

    int x = std::max(0, std::min((int)(uv.x * normalmap.width()),  normalmap.width() - 1));
    int y = std::max(0, std::min((int)(uv.y * normalmap.height()), normalmap.height() - 1));

    TGAColor c = normalmap.get(x, y);
    geom::vec3 res;
    res[0] = (double)c.bgra[2] / 255.0 * 2.0 - 1.0;
    res[1] = (double)c.bgra[1] / 255.0 * 2.0 - 1.0;
    res[2] = (double)c.bgra[0] / 255.0 * 2.0 - 1.0;
    return normalize(res);
}

TGAColor Model::diffuse_from_map(geom::vec2 uv) const {
    if (diffusemap.width() == 0) return TGAColor{255, 255, 255, 255};

    int x = std::max(0, std::min((int)(uv.x * diffusemap.width()),  diffusemap.width() - 1));
    int y = std::max(0, std::min((int)(uv.y * diffusemap.height()), diffusemap.height() - 1));

    return diffusemap.get(x, y);
}

double Model::specular_from_map(geom::vec2 uv) const {
    if (specularmap.width() == 0) return 0.0;

    int x = std::max(0, std::min((int)(uv.x * specularmap.width()),  specularmap.width() - 1));
    int y = std::max(0, std::min((int)(uv.y * specularmap.height()), specularmap.height() - 1));

    return specularmap.get(x, y).bgra[0] / 255.0;
}

TGAColor Model::glow_from_map(geom::vec2 uv) const {
    if (glowmap.width() == 0) return TGAColor{0, 0, 0, 255};

    int x = std::max(0, std::min((int)(uv.x * glowmap.width()),  glowmap.width() - 1));
    int y = std::max(0, std::min((int)(uv.y * glowmap.height()), glowmap.height() - 1));

    return glowmap.get(x, y);
}

geom::vec3 Model::normal_from_tangent_map(geom::vec2 uv) const {
    if (normaltangentmap.width() == 0) return geom::vec3(0, 0, 1);

    int x = std::max(0, std::min((int)(uv.x * normaltangentmap.width()),  normaltangentmap.width() - 1));
    int y = std::max(0, std::min((int)(uv.y * normaltangentmap.height()), normaltangentmap.height() - 1));

    TGAColor c = normaltangentmap.get(x, y);
    geom::vec3 res;
    res[0] = (double)c.bgra[2] / 255.0 * 2.0 - 1.0;
    res[1] = (double)c.bgra[1] / 255.0 * 2.0 - 1.0;
    res[2] = (double)c.bgra[0] / 255.0 * 2.0 - 1.0;
    return normalize(res);
}