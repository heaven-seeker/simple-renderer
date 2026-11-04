#pragma once

#include <vector>
#include "Geometry.h"
#include "tgaimage.h"
#include "our_gl.h"

#include "color.h"
#include "Model.h"

// -----------------------------------------------------------------------------
// 1. MySimpleShader - Renders the model with a single flat color, no lighting.
// -----------------------------------------------------------------------------
struct MySimpleShader : public IShader {
    TGAColor color;
    const Model &model;
    geom::matrix<4, 4> MVP;

    MySimpleShader(TGAColor c, const Model &model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(const int face, const int vert) override;
    virtual std::pair<bool, TGAColor> fragment(const geom::vec<3> bar) const override;
};

// -----------------------------------------------------------------------------
// 2. WireframeShader - Renders only the edges of the triangles (wireframe mode).
// -----------------------------------------------------------------------------
struct WireframeShader : public IShader {
    const Model &model;
    geom::matrix<4, 4> MVP;

    WireframeShader(const Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(const int face, const int vert) override;
    virtual std::pair<bool, TGAColor> fragment(const geom::vec<3> bar) const override;
};

// -----------------------------------------------------------------------------
// 3. SolidShader - Renders the model with a solid white color (useful as a mask).
// -----------------------------------------------------------------------------
struct SolidShader : public IShader {
    const Model &model;
    geom::matrix<4, 4> MVP;

    SolidShader(Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(const int face, const int vert) override;
    virtual std::pair<bool, TGAColor> fragment(const geom::vec<3> bar) const override;
};

// -----------------------------------------------------------------------------
// 4. RandomShader - Renders faces with random colors (useful for debugging).
// -----------------------------------------------------------------------------
struct RandomShader : public IShader {
    const Model &model;
    TGAColor color = {};
    geom::vec3 tri[3];
    geom::matrix<4, 4> MV;
    geom::matrix<4, 4> P;

    RandomShader(const Model &m, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(const int face, const int vert) override;
    virtual std::pair<bool, TGAColor> fragment(const geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 5. GouraudShader - Calculates lighting per-vertex and interpolates the intensity.
// -----------------------------------------------------------------------------
struct GouraudShader : public IShader {
    const Model &model;
    geom::vec3 light_dir;
    double varying_intensity[3];
    geom::matrix<4, 4> MVP;

    GouraudShader(const Model &m, geom::vec3 light, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(int iface, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 6. PhongShading - Calculates lighting per-pixel for smooth specular highlights.
// -----------------------------------------------------------------------------
struct PhongShading : public IShader {
    const Model &model;
    TGAColor color;
    geom::vec3 light_dir;
    geom::vec3 varying[3];
    geom::vec3 tri[3];

    geom::matrix<4, 4> MV;
    geom::matrix<4, 4> P;
    geom::matrix<4, 4> normalMatrix;
    int exponent = 5;

    PhongShading(const geom::vec3& light, const Model& model, const TGAColor& color, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(int face, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 7. NormalMappingShader - Applies surface details using a basic normal map.
// -----------------------------------------------------------------------------
struct NormalMappingShader : public IShader {
    const Model &model;
    geom::vec3 light_dir;
    geom::matrix<4, 4> MV;
    geom::matrix<4, 4> P;
    geom::matrix<4, 4> normalMatrix;
    int exponent = 5;

    geom::vec2 varying_uv[3];
    geom::vec3 tri[3];

    NormalMappingShader(const geom::vec3& light, const Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(int face, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 8. NormalTangentSpace - Advanced normal mapping using Tangent Space (TBN matrix) with AO support.
// -----------------------------------------------------------------------------
struct NormalTangentSpace : public IShader {
    const Model &model;
    geom::vec3 light_dir;
    geom::matrix<4, 4> normalMatrix;
    geom::matrix<4, 4> MV;
    geom::matrix<4, 4> P;
    geom::matrix<4, 4> Viewport;
    double exponent = 50.0;

    std::vector<double>& ao_buffer;
    int width;

    geom::vec3 varying[3];
    geom::vec2 varying_uv[3];
    geom::vec3 tri[3];

    NormalTangentSpace(const geom::vec3& light, const Model& model, std::vector<double>& ao, int w,
                       const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P, const geom::matrix<4, 4>& Viewport);
    virtual geom::vec4 vertex(int face, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 9. DepthShader - Renders depth (Z-values) to the color buffer for shadow mapping or SSAO.
// -----------------------------------------------------------------------------
struct DepthShader : public IShader {
    const Model &model;
    geom::matrix<4, 4> MVP;

    DepthShader(const Model &model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(int iface, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 10. ShadowShader - Computes shadows using a pre-calculated shadow map.
// -----------------------------------------------------------------------------
struct ShadowShader : public IShader {
    const Model &model;
    geom::vec3 light_dir;
    const std::vector<double>& shadow_map;
    geom::matrix<4, 4> M_shadow;
    int width;

    double exponent = 50.0;
    geom::vec3 varying[3];
    geom::vec2 varying_uv[3];
    geom::vec3 tri[3];
    geom::matrix<4, 4> normalMatrix;
    geom::matrix<4, 4> MV;
    geom::matrix<4, 4> P;

    ShadowShader(const Model& model, const geom::vec3& light, const std::vector<double>& sm,
                 geom::matrix<4, 4>& ms, int w, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(int face, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 11. GeometryShader - Renders surface normals to the screen (used for SSAO passes).
// -----------------------------------------------------------------------------
struct GeometryShader : public IShader {
    const Model& model;
    geom::vec3 varying_normal[3];
    geom::matrix<4, 4> normalMatrix;
    geom::matrix<4, 4> MVP;

    GeometryShader(const Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(int face, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};

// -----------------------------------------------------------------------------
// 12. ToonShader - Cel-shading effect that quantizes lighting into distinct cartoon-style bands.
// -----------------------------------------------------------------------------
struct ToonShader : public IShader {
    const Model& model;
    geom::vec3 light_dir;
    TGAColor color;
    geom::vec3 varying_normal[3];
    geom::matrix<4, 4> normalMatrix;
    geom::matrix<4, 4> MVP;

    ToonShader(const Model& model, const geom::vec3& l, TGAColor c, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P);
    virtual geom::vec4 vertex(int face, int nthvert) override;
    virtual std::pair<bool, TGAColor> fragment(geom::vec3 bar) const override;
};