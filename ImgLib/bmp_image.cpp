#include "bmp_image.h"
#include "pack_defines.h"
#include "img_lib.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} PACKED_STRUCT_END

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    if (!image) {
        return false;
    }

    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    const int width = image.GetWidth();
    const int height = image.GetHeight();
    const int stride = GetBMPStride(width);

    BitmapFileHeader file_header{};
    file_header.bfType = 0x4D42; // 'BM'
    file_header.bfSize = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + stride * height;
    file_header.bfOffBits = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;

    BitmapInfoHeader info_header{};
    info_header.biSize = sizeof(BitmapInfoHeader);
    info_header.biWidth = width;
    info_header.biHeight = height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = 0;
    info_header.biSizeImage = stride * height;
    info_header.biXPelsPerMeter = 11811;
    info_header.biYPelsPerMeter = 11811;
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0x1000000;

    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    vector<char> buffer(stride);
    for (int y = height - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < width; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buffer.data(), stride);
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) {
        return {};
    }

    BitmapFileHeader file_header;
    in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (in.gcount() != sizeof(file_header) || file_header.bfType != 0x4D42) {
        return {};
    }

    BitmapInfoHeader info_header;
    in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    if (in.gcount() != sizeof(info_header) || info_header.biSize != sizeof(BitmapInfoHeader)) {
        return {};
    }

    if (info_header.biBitCount != 24 || info_header.biCompression != 0) {
        return {};
    }

    const int width = info_header.biWidth;
    const int height = abs(info_header.biHeight);
    const int stride = GetBMPStride(width);

    in.seekg(file_header.bfOffBits, ios::beg);

    Image result(width, height, Color::Black());
    vector<char> buffer(stride);

    for (int y = height - 1; y >= 0; --y) {
        in.read(buffer.data(), stride);
        if (in.gcount() != stride) {
            return {};
        }

        Color* line = result.GetLine(y);
        for (int x = 0; x < width; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
            line[x].a = static_cast<byte>(255);
        }
    }

    return result;
}

}  // namespace img_lib