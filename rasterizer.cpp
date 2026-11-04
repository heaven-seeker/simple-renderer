#include "rasterizer.h"
#include "filesystem"
#include <algorithm>
#include <cmath>
#include "Geometry.h"
#include "color.h"
#include "our_gl.h"

namespace fs = std::filesystem;

double signed_triangle_area(int x1, int y1, int x2, int y2, int x3, int y3) {
    return 0.5 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
}

void line(int ax, int ay, int bx, int by, TGAImage& framebuffer, TGAColor color)
{
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    if (steep)
    {
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx)
    {
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    int y = ay;
    int ierror = 0;
    int slope = std::abs(by - ay) * 2;
    for (int x = ax; x <= bx; x++)
    {
        if (steep)
        {
            framebuffer.set(y, x, color);
        } else
        {
            framebuffer.set(x, y, color);
        }

        ierror += slope;
        if (ierror > bx - ax)
        {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx - ax);
        }
    }
}

void triangle_barycentric_bounding_box(geom::vec4* clip_coords, TGAImage& framebuffer, const IShader& shader, Pipeline& pipeline) {

    geom::vec3 points[3];
    for (int i = 0; i < 3; i++) {
        points[i] = geom::vec3(clip_coords[i].x / clip_coords[i].w, clip_coords[i].y / clip_coords[i].w, clip_coords[i].z / clip_coords[i].w);
    }

    geom::vec3 s_pts[3];
    for (int i = 0; i < 3; i++) {
        geom::vec4 translation = pipeline.Viewport * geom::vec4(points[i].x, points[i].y, points[i].z, 1.0);
        s_pts[i] = geom::vec3({translation.x, translation.y, translation.z});
    }

    int bbminx = std::max(0, (int)std::min({s_pts[0].x, s_pts[1].x, s_pts[2].x}));
    int bbminy = std::max(0, (int)std::min({s_pts[0].y, s_pts[1].y, s_pts[2].y}));
    int bbmaxx = std::min(framebuffer.width() - 1,  (int)std::max({s_pts[0].x, s_pts[1].x, s_pts[2].x}));
    int bbmaxy = std::min(framebuffer.height() - 1, (int)std::max({s_pts[0].y, s_pts[1].y, s_pts[2].y}));

    double total_area = signed_triangle_area(s_pts[0].x, s_pts[0].y, s_pts[1].x, s_pts[1].y, s_pts[2].x, s_pts[2].y);

    if (std::abs(total_area) < 1) return;

    #pragma omp parallel for

    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, s_pts[1].x, s_pts[1].y, s_pts[2].x, s_pts[2].y) / total_area;
            double beta = signed_triangle_area(x, y, s_pts[2].x, s_pts[2].y, s_pts[0].x, s_pts[0].y) / total_area;
            double gamma = signed_triangle_area(x, y, s_pts[0].x, s_pts[0].y, s_pts[1].x, s_pts[1].y) / total_area;

            if (alpha < 0 || beta < 0 || gamma < 0) continue;
            double z = alpha * s_pts[0].z + beta * s_pts[1].z + gamma * s_pts[2].z;
            int idx = x + y * framebuffer.width();
            geom::vec<3> bc = {static_cast<float>(alpha), static_cast<float>(beta), static_cast<float>(gamma)};

            if (pipeline.zbuffer[idx] < z) {
                auto [discard, color] = shader.fragment(bc);

                if (!discard) {
                    pipeline.zbuffer[idx] = z;
                    framebuffer.set(x, y, color);
                }
            }
        }
    }
}

void draw_fat_point(int x, int y, TGAImage& framebuffer, TGAColor color, int size)
{
    if (size == 0) return;
    for (int i = 0; i <= size; i++)
    {
        for (int j = -size; j <= size; j++)
        {
            int px = x + i;
            int py = y + j;

            if (px >= 0 && px < framebuffer.width() && py >= 0 && py < framebuffer.height())
            {
                framebuffer.set(px, py, color);
            }

        }
    }
}