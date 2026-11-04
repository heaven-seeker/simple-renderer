#pragma once

#include <vector>
#include "Geometry.h"

struct TGAImage;
struct TGAColor;
class Pipeline;

double signed_triangle_area(int x1, int y1, int x2, int y2, int x3, int y3);
void line(int ax, int ay, int bx, int by, TGAImage& framebuffer, struct TGAColor color);
void draw_fat_point(int x, int y, TGAImage& framebuffer, TGAColor color, int size = 1);

void triangle_barycentric_bounding_box(geom::vec4* clip_coords, TGAImage& framebuffer, const struct IShader& shader, Pipeline& pipeline);