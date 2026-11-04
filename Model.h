#pragma once

#include <iostream>
#include <vector>
#include "Geometry.h"
#include "tgaimage.h"

class Pipeline;

struct Triangle {
    geom::vec<3> a, b, c;
    float z_mid;
    Triangle(geom::vec<3> a, geom::vec<3> b, geom::vec<3> c) : a(a), b(b), c(c) {
        z_mid = (a.z + b.z + c.z) / 3;
    }
};

// v - position, vt - texture coordinate, vn - normal
struct VertIndices {
    int v, t, n; // A single vertex of a triangle
};

struct Face {
    VertIndices corners[3]; // The three vertices that make up a face
};

class Model
{
    std::vector<geom::vec<3>> verts;
    std::vector<geom::vec<3>> tex_coords;
    std::vector<geom::vec<3>> normals;
    std::vector<Face> faces;
    TGAImage normalmap;
    TGAImage diffusemap;
    TGAImage specularmap;
    TGAImage glowmap;
    TGAImage normaltangentmap;

public:
    Model(const std::string& fileName);

    // Render methods
    void draw_model(struct TGAImage& framebuffer, struct IShader& shader, Pipeline& pipeline) const;
    void draw_points(struct TGAImage& framebuffer, struct TGAColor color, Pipeline& pipeline) const;

    // Getters for vertex data
    geom::vec3 vert(int face, int vert) const;
    geom::vec3 normal(int face, int vert) const;
    geom::vec2 uv(int face, int vert) const;

    // Material sampling methods
    geom::vec3 normal_from_map(geom::vec2 uv) const;
    TGAColor diffuse_from_map(geom::vec2 uv) const;
    double specular_from_map(geom::vec2 uv) const;
    TGAColor glow_from_map(geom::vec2 uv) const;
    geom::vec3 normal_from_tangent_map(geom::vec2 uv) const;
};