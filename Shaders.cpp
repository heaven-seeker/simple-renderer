#include "Shaders.h"

// --- MySimpleShader ---
MySimpleShader::MySimpleShader(TGAColor c, const Model &model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : color(c), model(model) { MVP = P * MV; }

geom::vec4 MySimpleShader::vertex(const int face, const int vert) {
    geom::vec3 v = model.vert(face, vert);
    return MVP * geom::vec4{v.x, v.y, v.z, 1.};
}

std::pair<bool, TGAColor> MySimpleShader::fragment(const geom::vec<3> bar) const {
    return {false, color};
}

// --- WireframeShader ---
WireframeShader::WireframeShader(const Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P)
    : model(model) { MVP = P * MV; }

geom::vec4 WireframeShader::vertex(const int face, const int vert) {
    geom::vec3 v = model.vert(face, vert);
    return MVP * geom::vec4{v.x, v.y, v.z, 1.};
}

std::pair<bool, TGAColor> WireframeShader::fragment(const geom::vec<3> bar) const {
    if (bar.x < 0.02f || bar.y < 0.02f || bar.z < 0.02f) return {false, black};
    return {true, TGAColor({0, 0, 0, 0})};
}

// --- SolidShader ---
SolidShader::SolidShader(Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model(model) { MVP = P * MV; }

geom::vec4 SolidShader::vertex(const int face, const int vert) {
    geom::vec3 v = model.vert(face, vert);
    return MVP * geom::vec4{v.x, v.y, v.z, 1.};
}

std::pair<bool, TGAColor> SolidShader::fragment(const geom::vec<3> bar) const {
    return {false, white};
}

// --- RandomShader ---
RandomShader::RandomShader(const Model &m, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model(m), MV(MV), P(P) {}

geom::vec4 RandomShader::vertex(const int face, const int vert) {
    if (vert == 0) {
        color = TGAColor({
            static_cast<uint8_t>(std::rand() % 255),
            static_cast<uint8_t>(std::rand() % 255),
            static_cast<uint8_t>(std::rand() % 255),
            255
        });
    }

    geom::vec3 v = model.vert(face, vert);
    geom::vec4 gl_Position = MV * geom::vec4{v.x, v.y, v.z, 1.};
    tri[vert] = gl_Position.xyz();
    return P * gl_Position;
}

std::pair<bool,TGAColor> RandomShader::fragment(const geom::vec3 bar) const {
    return {false, color};
}

// --- GouraudShader ---
GouraudShader::GouraudShader(const Model &m, geom::vec3 light, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model(m), light_dir(geom::normalize(light)) { MVP = P * MV; }

geom::vec4 GouraudShader::vertex(int iface, int nthvert) {
    geom::vec3 v = model.vert(iface, nthvert);
    geom::vec3 normal = model.normal(iface, nthvert);
    varying_intensity[nthvert] = std::max(0.0, dot(normal, light_dir));
    return MVP * geom::vec4(v.x, v.y, v.z, 1.0);
}

std::pair<bool, TGAColor> GouraudShader::fragment(geom::vec3 bar) const {
    double intensity = varying_intensity[0]*bar.x + varying_intensity[1]*bar.y + varying_intensity[2]*bar.z;
    TGAColor color = {
        static_cast<uint8_t>(white.bgra[0] * intensity),
        static_cast<uint8_t>(white.bgra[1] * intensity),
        static_cast<uint8_t>(white.bgra[2] * intensity), 255
    };
    return {false, color};
}

// --- PhongShading ---
PhongShading::PhongShading(const geom::vec3& light, const Model& model, const TGAColor& color, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model(model), color(color), MV(MV), P(P) {
    normalMatrix = transpose(MV.inverse());
    geom::vec4 l = MV * geom::vec4{light.x, light.y, light.z, 0};
    light_dir = normalize(l.xyz());
}

geom::vec4 PhongShading::vertex(int face, int nthvert) {
    geom::vec3 v = model.vert(face, nthvert);
    geom::vec3 normal = model.normal(face, nthvert);
    geom::vec4 n = normalMatrix * geom::vec4{normal.x, normal.y, normal.z, 0.};
    varying[nthvert] = normalize(n.xyz());
    geom::vec4 gl_Position = MV * geom::vec4{v.x, v.y, v.z, 1.};
    tri[nthvert] = gl_Position.xyz();
    return P * gl_Position;
}

std::pair<bool, TGAColor> PhongShading::fragment(geom::vec3 bar) const {
    geom::vec3 newNormal = normalize(bar.x * varying[0] + bar.y * varying[1] + bar.z * varying[2]);
    geom::vec3 frag_pos = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z; 
    geom::vec3 v = normalize(-1 * frag_pos);
    const double ambient = 0.1;
    double diffuse = std::max(0., dot(newNormal, light_dir));
    geom::vec3 r = normalize(2 * newNormal * dot(newNormal, light_dir) - light_dir);
    double specular = std::pow(std::max(0., dot(r, v)), exponent);
    double intensity = ambient + diffuse + specular;
    return {false, color * intensity};
}

// --- NormalMappingShader ---
NormalMappingShader::NormalMappingShader(const geom::vec3& light, const Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model(model), MV(MV), P(P) {
    normalMatrix = transpose(MV.inverse());
    geom::vec4 l = MV * geom::vec4{light.x, light.y, light.z, 0};
    light_dir = normalize(l.xyz());
}

geom::vec4 NormalMappingShader::vertex(int face, int nthvert) {
    geom::vec3 v = model.vert(face, nthvert);
    varying_uv[nthvert] = model.uv(face, nthvert);
    geom::vec4 gl_Position = MV * geom::vec4{v.x, v.y, v.z, 1.};
    tri[nthvert] = gl_Position.xyz();
    return P * gl_Position;
}

std::pair<bool, TGAColor> NormalMappingShader::fragment(geom::vec3 bar) const {
    geom::vec2 uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z;
    geom::vec3 n = model.normal_from_map(uv);
    geom::vec4 n_rotated = normalMatrix * geom::vec4{n.x, n.y, n.z, 0.};
    geom::vec3 newNormal = geom::normalize(n_rotated.xyz());
    geom::vec3 frag_pos = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
    geom::vec3 v = normalize(-1.0 * frag_pos);
    const double ambient = 0.25;
    double diffuse = std::max(0., dot(newNormal, light_dir));
    geom::vec3 r = normalize(2 * newNormal * dot(newNormal, light_dir) - light_dir);
    double specular = std::pow(std::max(0., dot(r, v)), exponent);
    TGAColor diff_color = model.diffuse_from_map(uv);
    double spec_power = model.specular_from_map(uv);
    diff_color = diff_color * (ambient + diffuse);
    TGAColor whitecolor = white * (specular * spec_power);
    return {false, diff_color + whitecolor};
}

// --- NormalTangentSpace ---
NormalTangentSpace::NormalTangentSpace(const geom::vec3& light, const Model& model, std::vector<double>& ao, int w, 
                                       const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P, const geom::matrix<4, 4>& Viewport) 
    : model(model), ao_buffer(ao), width(w), MV(MV), P(P), Viewport(Viewport) {
    normalMatrix = transpose(MV.inverse());
    geom::vec4 l = MV * geom::vec4{light.x, light.y, light.z, 0};
    light_dir = normalize(l.xyz());
}

geom::vec4 NormalTangentSpace::vertex(int face, int nthvert) {
    geom::vec3 v = model.vert(face, nthvert);
    geom::vec3 normal = model.normal(face, nthvert);
    geom::vec4 n = normalMatrix * geom::vec4{normal.x, normal.y, normal.z, 0.};
    varying[nthvert] = normalize(n.xyz());
    varying_uv[nthvert] = model.uv(face, nthvert);
    geom::vec4 gl_Position = MV * geom::vec4{v.x, v.y, v.z, 1.};
    tri[nthvert] = gl_Position.xyz();
    return P * gl_Position;
}

std::pair<bool, TGAColor> NormalTangentSpace::fragment(geom::vec3 bar) const {
    geom::vec2 uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z;
    geom::vec3 e0 = tri[1] - tri[0];
    geom::vec3 e1 = tri[2] - tri[0];
    geom::vec2 u0 = varying_uv[1] - varying_uv[0];
    geom::vec2 u1 = varying_uv[2] - varying_uv[0];
    geom::matrix<3, 2> E = {{{e0.x, e1.x}, {e0.y, e1.y}, {e0.z, e1.z}}};
    geom::matrix<2, 2> U = {{{u0.x, u1.x}, {u0.y, u1.y}}};
    geom::matrix<3, 2> tb =  E * U.inverse();
    geom::vec3 t = normalize(geom::vec3(tb[0][0], tb[1][0], tb[2][0]));
    geom::vec3 b = normalize(geom::vec3(tb[0][1], tb[1][1], tb[2][1]));
    geom::vec3 normal = normalize(varying[0] * bar.x + varying[1] * bar.y + varying[2] * bar.z);
    geom::matrix<3, 3> TBN = {{{t.x, b.x, normal.x},{t.y, b.y, normal.y},{t.z, b.z, normal.z}}};
    geom::vec3 n_tangent = model.normal_from_tangent_map(uv);
    geom::vec3 newNormal = normalize(TBN * n_tangent);
    geom::vec3 frag_pos = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
    geom::vec3 v = normalize(-1.0 * frag_pos);
    
    double ambient = 0.25;

    double diffuse = std::max(0., dot(newNormal, light_dir));
    geom::vec3 r = normalize(2 * newNormal * dot(newNormal, light_dir) - light_dir);
    double specular = std::pow(std::max(0., dot(r, v)), exponent);
    TGAColor diff_color = model.diffuse_from_map(uv);
    double spec_power = model.specular_from_map(uv);
    diff_color = diff_color * (ambient + diffuse);
    TGAColor whitecolor = white * (specular * spec_power);
    return {false, diff_color + whitecolor};
}

// --- DepthShader ---
DepthShader::DepthShader(const Model &model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model(model) { MVP = P * MV; }

geom::vec4 DepthShader::vertex(int iface, int nthvert) {
    geom::vec3 v = model.vert(iface, nthvert);
    return MVP * geom::vec4{v.x, v.y, v.z, 1.};
}

std::pair<bool, TGAColor> DepthShader::fragment(geom::vec3 bar) const {
    return {false, white};
}

// --- ShadowShader ---
ShadowShader::ShadowShader(const Model& model, const geom::vec3& light, const std::vector<double>& sm,
                           geom::matrix<4, 4>& ms, int w, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P)
    : model(model), shadow_map(sm), M_shadow(ms), width(w), MV(MV), P(P) {
    geom::vec4 l = MV * geom::vec4{light.x, light.y, light.z, 0};
    normalMatrix = transpose(MV.inverse());
    light_dir = normalize(l.xyz());
}

geom::vec4 ShadowShader::vertex(int face, int nthvert) {
    geom::vec3 v = model.vert(face, nthvert);
    geom::vec3 normal = model.normal(face, nthvert);
    geom::vec4 n = normalMatrix * geom::vec4{normal.x, normal.y, normal.z, 0.};
    varying[nthvert] = normalize(n.xyz());
    varying_uv[nthvert] = model.uv(face, nthvert);
    geom::vec4 gl_Position = MV * geom::vec4{v.x, v.y, v.z, 1.};
    tri[nthvert] = gl_Position.xyz();
    return P * gl_Position;
}

std::pair<bool, TGAColor> ShadowShader::fragment(geom::vec3 bar) const {
    geom::vec2 uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z;
    geom::vec3 e0 = tri[1] - tri[0];
    geom::vec3 e1 = tri[2] - tri[0];
    geom::vec2 u0 = varying_uv[1] - varying_uv[0];
    geom::vec2 u1 = varying_uv[2] - varying_uv[0];
    geom::matrix<3, 2> E = {{{e0.x, e1.x}, {e0.y, e1.y}, {e0.z, e1.z}}};
    geom::matrix<2, 2> U = {{{u0.x, u1.x}, {u0.y, u1.y}}};
    geom::matrix<3, 2> tb =  E * U.inverse();
    geom::vec3 t = normalize(geom::vec3(tb[0][0], tb[1][0], tb[2][0]));
    geom::vec3 b = normalize(geom::vec3(tb[0][1], tb[1][1], tb[2][1]));
    geom::vec3 normal = normalize(varying[0] * bar.x + varying[1] * bar.y + varying[2] * bar.z);
    geom::matrix<3, 3> TBN = {{{t.x, b.x, normal.x},{t.y, b.y, normal.y},{t.z, b.z, normal.z}}};
    geom::vec3 n_tangent = model.normal_from_tangent_map(uv);
    geom::vec3 newNormal = normalize(TBN * n_tangent);
    geom::vec3 frag_pos = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
    geom::vec4 frag_light_screen = M_shadow * geom::vec4(frag_pos.x, frag_pos.y, frag_pos.z, 1.);
    frag_light_screen = frag_light_screen / frag_light_screen.w;
    geom::vec3 v = normalize(-1.0 * frag_pos);
    const double ambient = 0.2;
    double diffuse = std::max(0., dot(newNormal, light_dir));
    geom::vec3 r = normalize(2 * newNormal * dot(newNormal, light_dir) - light_dir);
    double specular = std::pow(std::max(0., dot(r, v)), exponent);
    TGAColor diff_color = model.diffuse_from_map(uv);
    double spec_power = model.specular_from_map(uv);
    TGAColor whitecolor = white;

    int idx = int(frag_light_screen.x) + int(frag_light_screen.y) * width;
    if (idx < 0 || idx >= static_cast<int>(shadow_map.size())) return {false, diff_color + whitecolor};
    double shadow = 1.0;
    if (frag_light_screen.z < shadow_map[idx] - 0.01) shadow = 0.3;
    
    diff_color = diff_color * (ambient + diffuse * shadow);
    whitecolor = whitecolor * (specular * spec_power * shadow);
    return {false, diff_color + whitecolor};
}

// --- GeometryShader ---
GeometryShader::GeometryShader(const Model& model, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model{model} {
    normalMatrix = transpose(MV.inverse());
    MVP = P * MV;
}

geom::vec4 GeometryShader::vertex(int face, int nthvert) {
    geom::vec3 v = model.vert(face, nthvert);
    geom::vec3 n = model.normal(face, nthvert);
    geom::vec4 normal = normalMatrix * geom::vec4{n.x, n.y, n.z, 0.};
    varying_normal[nthvert] = normal.xyz();
    return MVP * geom::vec4{v.x, v.y, v.z, 1.};
}

std::pair<bool, TGAColor> GeometryShader::fragment(geom::vec3 bar) const {
    geom::vec3 normal = varying_normal[0] * bar.x + varying_normal[1] * bar.y + varying_normal[2] * bar.z;
    normal = geom::normalize(normal);
    TGAColor color;
    color.bgra[2] = std::max(0, std::min(255, (int)((normal.x + 1.0) * 127.5)));
    color.bgra[1] = std::max(0, std::min(255, (int)((normal.y + 1.0) * 127.5)));
    color.bgra[0] = std::max(0, std::min(255, (int)((normal.z + 1.0) * 127.5)));
    color.bgra[3] = 255;
    return {false, color};
}

// --- ToonShader ---
ToonShader::ToonShader(const Model& model, const geom::vec3& l, TGAColor c, const geom::matrix<4, 4>& MV, const geom::matrix<4, 4>& P) 
    : model(model), color(c) {
    normalMatrix = transpose(MV.inverse());
    light_dir = (MV * geom::vec4{l.x, l.y, l.z, 0.}).xyz();
    MVP = P * MV;
}

geom::vec4 ToonShader::vertex(int face, int nthvert) {
    geom::vec3 v = model.vert(face, nthvert);
    geom::vec3 n = model.normal(face, nthvert);
    geom::vec4 normal = normalMatrix * geom::vec4{n.x, n.y, n.z, 0.};
    varying_normal[nthvert] = normal.xyz();
    return MVP * geom::vec4{v.x, v.y, v.z, 1.};
}

std::pair<bool, TGAColor> ToonShader::fragment(geom::vec3 bar) const {
    geom::vec3 normal = varying_normal[0] * bar.x + varying_normal[1] * bar.y + varying_normal[2] * bar.z;
    normal = geom::normalize(normal);
    double intensity = std::max(0., dot(normal, light_dir));
    if (intensity > 0.85) intensity = 1.0;
    else if (intensity > 0.5) intensity = 0.7;
    else if (intensity > 0.2) intensity = 0.4;
    else intensity = 0.15;
    return {false, color * intensity};
}