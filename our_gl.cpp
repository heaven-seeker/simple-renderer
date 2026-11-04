#include "our_gl.h"

#include <algorithm>

Pipeline::Pipeline(int w, int h) : width(w), height(h) {
    clear_zbuffer();
    ModelView = geom::matrix<4,4>::identity();
    Viewport = geom::matrix<4,4>::identity();
    Perspective = geom::matrix<4,4>::identity();
}

void Pipeline::clear_zbuffer() {
    zbuffer.assign(width * height, -std::numeric_limits<double>::max());
}

void Pipeline::init_viewport(const int x, const int y, const int w, const int h) {
    Viewport = {{{w/2., 0, 0, x + w/2.}, {0, h/2., 0, y + h/2.}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
}

void Pipeline::init_perspective(const double f) {
    Perspective = {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1/f,1}}};
}

void Pipeline::lookat(geom::vec3 eye, geom::vec3 center, geom::vec3 up) {
    geom::vec<3> n = normalize((eye - center));
    geom::vec<3> l = normalize(cross(up, n));
    geom::vec<3> m = normalize(cross(n, l));
    geom::matrix<4, 4> Rotation = {{{l.x, l.y, l.z, 0}, {m.x, m.y, m.z, 0}, {n.x, n.y, n.z, 0}, {0, 0, 0, 1}}};
    geom::matrix<4, 4> Translation = {{{1, 0, 0, -eye.x}, {0, 1, 0, -eye.y}, {0, 0, 1, -eye.z}, {0, 0, 0, 1}}};

    ModelView = Rotation * Translation;
}

void Pipeline::save_zbuffer(const std::string& filename) {
    TGAImage zbuffer_image(width, height, TGAImage::GRAYSCALE);

    double min_z = std::numeric_limits<double>::max();
    double max_z = -std::numeric_limits<double>::max();

    for (int i = 0; i < width * height; i++) {
        if (zbuffer[i] != -std::numeric_limits<double>::max()) {
            if (zbuffer[i] < min_z) min_z = zbuffer[i];
            if (zbuffer[i] > max_z) max_z = zbuffer[i];
        }
    }

    if (max_z == min_z) max_z = min_z + 1.0;

    for (int i = 0; i < width * height; i++) {
        if (zbuffer[i] != -std::numeric_limits<double>::max()) {
            double normalized_z = (zbuffer[i] - min_z) / (max_z - min_z);

            int color = normalized_z * 255.0;
            color = std::max(0, std::min(255, color));

            zbuffer_image.set(i % width, i / width, {static_cast<unsigned char>(color)});
        }
    }

    zbuffer_image.write_tga_file(filename);
}