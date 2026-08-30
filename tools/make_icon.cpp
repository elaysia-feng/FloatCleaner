// make_icon: 生成 FloatCleaner 的多尺寸 .ico 图标
// 图案与程序运行时图标一致：薄荷绿圆底 + 白色向下箭头
// 用法: make_icon <输出.ico>
#include <windows.h>

#include <cstdio>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

struct Image {
    int size = 0;
    std::vector<uint8_t> dib; // BITMAPINFOHEADER + XOR(BGRA) + AND mask
};

COLORREF lerp(COLORREF a, COLORREF b, double t)
{
    return RGB(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t,
               GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t,
               GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
}

// 渲染一帧图标到 32bpp DIB（顶层向下像素缓冲）
void render(int size, uint8_t* px) // px: size*size*4, 顶层在下标 0
{
    HDC screen = GetDC(nullptr);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = size;
    bi.bmiHeader.biHeight = -size; // 顶层向下
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

    RECT rc{0, 0, size, size};
    HBRUSH clear = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(mem, &rc, clear);
    DeleteObject(clear);

    // 圆底：轻微纵向渐变（薄荷绿 -> 深一点的绿）
    const COLORREF top = RGB(255, 168, 206);
    const COLORREF bottom = RGB(155, 123, 232);
    for (int y = 0; y < size; ++y) {
        SetPixelV(mem, size / 2, y, lerp(top, bottom, (double)y / size));
    }
    // 用画笔做实心圆（GDI 无抗锯齿，16px 以下换实色更干净）
    HPEN ring = CreatePen(PS_SOLID, size >= 32 ? std::max(1, size / 32) : 1,
                          size >= 32 ? RGB(226, 105, 158) : RGB(255, 138, 178));
    HBRUSH bg = CreateSolidBrush(RGB(255, 138, 178));
    HBRUSH oldBrush = (HBRUSH)SelectObject(mem, bg);
    HPEN oldPen = (HPEN)SelectObject(mem, ring);
    Ellipse(mem, 1, 1, size - 1, size - 1);

    // 白色向下箭头
    const int cx = size / 2;
    const int stem = std::max(1, size / 7);
    POINT arrow[7] = {
        {cx - stem, size * 24 / 100},   {cx + stem, size * 24 / 100},
        {cx + stem, size * 50 / 100},   {cx + stem * 2, size * 50 / 100},
        {cx, size * 73 / 100},          {cx - stem * 2, size * 50 / 100},
        {cx - stem, size * 50 / 100},
    };
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    HPEN whitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    SelectObject(mem, white);
    SelectObject(mem, whitePen);
    Polygon(mem, arrow, 7);

    SelectObject(mem, oldBrush);
    SelectObject(mem, oldPen);
    DeleteObject(bg);
    DeleteObject(ring);
    DeleteObject(white);
    DeleteObject(whitePen);

    GdiFlush();
    memcpy(px, bits, (size_t)size * size * 4);

    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
}

Image makeImage(int size)
{
    Image img;
    img.size = size;

    std::vector<uint8_t> rgba((size_t)size * size * 4);
    render(size, rgba.data());

    BITMAPINFOHEADER h{};
    h.biSize = sizeof(h);
    h.biWidth = size;
    h.biHeight = size * 2; // XOR + AND
    h.biPlanes = 1;
    h.biBitCount = 32;
    h.biCompression = BI_RGB;
    h.biSizeImage = 0;

    const uint8_t* hdr = (const uint8_t*)&h;
    img.dib.insert(img.dib.end(), hdr, hdr + sizeof(h));

    // XOR：BGRA 自底向上（我们的渲染就是 BGRA 顺序，翻转行序）
    for (int y = size - 1; y >= 0; --y)
        img.dib.insert(img.dib.end(), rgba.begin() + (size_t)y * size * 4,
                       rgba.begin() + (size_t)(y + 1) * size * 4);

    // AND 掩码：1bpp，行按 32bit 对齐，自底向上；alpha>0 视为不透明
    const int stride = ((size + 31) / 32) * 4;
    std::vector<uint8_t> maskRow(stride, 0);
    for (int y = size - 1; y >= 0; --y) {
        std::vector<uint8_t> row(stride, 0);
        for (int x = 0; x < size; ++x) {
            const uint8_t alpha = rgba[((size_t)y * size + x) * 4 + 3];
            if (alpha > 8)
                row[x / 8] |= 0x80 >> (x % 8);
        }
        img.dib.insert(img.dib.end(), row.begin(), row.end());
    }
    return img;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: make_icon <output.ico>\n");
        return 1;
    }

    const int sizes[] = {256, 64, 48, 32, 24, 16};
    std::vector<Image> images;
    for (int s : sizes)
        images.push_back(makeImage(s));

    FILE* f = fopen(argv[1], "wb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }

    const uint16_t count = (uint16_t)images.size();
    uint16_t zero = 0, one = 1;
    fwrite(&zero, 2, 1, f); // reserved
    fwrite(&one, 2, 1, f);  // type: icon
    fwrite(&count, 2, 1, f);

    uint32_t offset = 6 + 16 * count;
    for (const Image& img : images) {
        uint8_t w = img.size == 256 ? 0 : (uint8_t)img.size;
        uint8_t colors = 0;
        uint16_t planes = 1, bpp = 32;
        uint32_t bytes = (uint32_t)img.dib.size();
        fwrite(&w, 1, 1, f);
        fwrite(&w, 1, 1, f);
        fwrite(&colors, 1, 1, f);
        fwrite(&zero /*byte*/, 1, 1, f);
        fwrite(&planes, 2, 1, f);
        fwrite(&bpp, 2, 1, f);
        fwrite(&bytes, 4, 1, f);
        fwrite(&offset, 4, 1, f);
        offset += bytes;
    }
    for (const Image& img : images)
        fwrite(img.dib.data(), 1, img.dib.size(), f);
    fclose(f);

    printf("[OK] %s written (%d sizes)\n", argv[1], (int)images.size());
    return 0;
}
