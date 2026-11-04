#pragma once
#include "tgaimage.h"
#include "Geometry.h"
#include <vector>
#include <string>

class Pipeline {
public:
    geom::matrix<4,4> ModelView;
    geom::matrix<4,4> Viewport;
    geom::matrix<4,4> Perspective;
    std::vector<double> zbuffer;
    int width, height;

    Pipeline(int w, int h);

    void lookat(geom::vec3 eye, geom::vec3 center, geom::vec3 up);
    void init_perspective(const double f);
    void init_viewport(const int x, const int y, const int w, const int h);
    void clear_zbuffer();

    void save_zbuffer(const std::string& filename);
};

struct IShader {
    virtual ~IShader() = default;
    virtual geom::vec4 vertex(int iface, int nthvert) = 0;
    virtual std::pair<bool, TGAColor> fragment(const geom::vec3 bar) const = 0;
};