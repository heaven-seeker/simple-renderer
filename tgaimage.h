#pragma once
#include <cstdint>
#include <fstream>
#include <vector>

#pragma pack(push,1)
struct TGAHeader {
    std::uint8_t  idlength = 0;
    std::uint8_t  colormaptype = 0;
    std::uint8_t  datatypecode = 0;
    std::uint16_t colormaporigin = 0;
    std::uint16_t colormaplength = 0;
    std::uint8_t  colormapdepth = 0;
    std::uint16_t x_origin = 0;
    std::uint16_t y_origin = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t  bitsperpixel = 0;
    std::uint8_t  imagedescriptor = 0;
};
#pragma pack(pop)

struct TGAColor {
    std::uint8_t bgra[4] = {0,0,0,0};
    std::uint8_t bytespp = 4;
    std::uint8_t& operator[](const int i) { return bgra[i]; }

    TGAColor operator*(const double intensity) const {
        TGAColor newColor;
        for (int i = 0; i < 3; i++) {
            newColor.bgra[i] = std::min(static_cast<int>(bgra[i] * intensity), 255);
        }
        newColor.bgra[3] = bgra[3];
        return newColor;
    }
};

inline TGAColor operator+(const TGAColor &c1, const TGAColor &c2) {
    TGAColor res;
    res.bytespp = c1.bytespp;

    res.bgra[0] = std::min(255, int(c1.bgra[0]) + int(c2.bgra[0]));
    res.bgra[1] = std::min(255, int(c1.bgra[1]) + int(c2.bgra[1]));
    res.bgra[2] = std::min(255, int(c1.bgra[2]) + int(c2.bgra[2]));
    res.bgra[3] = c1.bgra[3];
    return res;
}

struct TGAImage {
    enum Format { GRAYSCALE=1, RGB=3, RGBA=4 };
    TGAImage() = default;
    TGAImage(const int w, const int h, const int bpp);
    bool  read_tga_file(const std::string filename);
    bool write_tga_file(const std::string filename, const bool vflip=true, const bool rle=true) const;
    void flip_horizontally();
    void flip_vertically();
    TGAColor get(const int x, const int y) const;
    void set(const int x, const int y, const TGAColor &c);
    int width()  const;
    int height() const;
private:
    bool   load_rle_data(std::ifstream &in);
    bool unload_rle_data(std::ofstream &out) const;
    int w = 0, h = 0;
    std::uint8_t bpp = 0;
    std::vector<std::uint8_t> data = {};
};
