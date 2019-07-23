#include <stdlib.h>
#include <stdio.h>
#define STBI_MSC_SECURE_CRT
#define STBI_WRITE_NO_STDIO
#define STB_TRUETYPE_IMPLEMENTATION
#include "../gearoenix/sdk/stb/stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../gearoenix/sdk/stb/stb_image_write.h"

#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <cstdint>

class Font2D
{
private:
    stbtt_fontinfo font;
    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    int line_growth = 0;
    int fnt_height = 0;

    struct Aspect
    {
        int max_lsb = 0;
        int max_width = 1;
        int height = 1;

        void print() const noexcept
        {
            std::cout << "max_lsb: " << max_lsb << ", max_width: " << max_width << ", height: " << height << std::endl;
        }
    };

    Aspect compute_left_side_bearing(const std::wstring &text) const noexcept
    {
        bool start_of_line = true;
        Aspect a;
        int w = 1;
        a.height = line_growth + 1;
        const int txt_size_less = text.size() - 1;
        for (int i = 0; i < txt_size_less;)
        {
            const wchar_t c = text[i];
            const wchar_t next_c = text[++i];
            if (c == '\n')
            {
                a.height += line_growth;
                w = 1;
                continue;
            }
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
            lsb = w - lsb;
            if (a.max_lsb > lsb)
                a.max_lsb = lsb;
            w += advance;
            w += stbtt_GetCodepointKernAdvance(&font, c, next_c);
            if (a.max_width < w)
                a.max_width = w;
            std::cout << "advance: " << advance << ", lsb: " << lsb << std::endl;
        }
        {
            const wchar_t c = text[txt_size_less];
            if (c == '\n')
            {
                a.height += line_growth;
            }
            else
            {
                int advance, lsb;
                stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
                lsb = w - lsb;
                if (a.max_lsb > lsb)
                    a.max_lsb = lsb;
                w += advance;
                if (a.max_width < w)
                    a.max_width = w;
                std::cout << "advance: " << advance << ", lsb: " << lsb << std::endl;
            }
        }
        a.max_lsb = -a.max_lsb;
        a.max_width += a.max_lsb;
        return a;
    }

public:
    Font2D(const std::vector<unsigned char> &ttf_data) noexcept
    {
        stbtt_InitFont(&font, ttf_data.data(), 0);
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
        fnt_height = ascent - descent;
        line_growth = line_gap + fnt_height;
        std::cout << "ascent: " << ascent << std::endl;
        std::cout << "descent: " << descent << std::endl;
        std::cout << "line_gap: " << line_gap << std::endl;
        std::cout << "fnt_height: " << fnt_height << std::endl;
        std::cout << "line_growth: " << line_growth << std::endl;
    }
    /// Return an image with RGBA-8bit format
    std::vector<unsigned char> bake(const std::wstring &text, int img_width, int img_height, int img_margin, float &img_ratio) const noexcept
    {
        const Aspect a = compute_left_side_bearing(text);
        a.print();
        const float w_scale = float(img_width - (img_margin << 1)) / float(a.max_width);
        const float h_scale = float(img_height - (img_margin << 1)) / float(a.height);
        img_ratio = (w_scale * float(img_margin << 1) + float(a.max_width)) / (h_scale * float(img_margin << 1) + float(a.height));

        float base_line = float(ascent) * h_scale + float(img_margin);
        std::cout << "base_line: " << base_line << std::endl;
        std::vector<unsigned char> rnd_data(img_width * img_height);

        const float scaled_lg = float(line_growth) * h_scale;
        const float xpos_start = float(a.max_lsb) * w_scale + float(img_margin);
        const int txt_size_less = text.size() - 1;
        float xpos = xpos_start;

        const auto index = [&](const int x, const int y) {
            return y * img_width + x;
        };

        float y_shift = base_line - float(floor(base_line));

        for (int i = 0; i < txt_size_less;)
        {
            const wchar_t c = text[i];
            const wchar_t next_c = text[++i];
            if (c == '\n')
            {
                base_line += scaled_lg;
                xpos = xpos_start;
                y_shift = base_line - float(floor(base_line));
                continue;
            }
            int advance, lsb, x0, y0, x1, y1;
            const float x_shift = xpos - float(floor(xpos));
            stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
            stbtt_GetCodepointBitmapBoxSubpixel(&font, c, w_scale, h_scale, x_shift, y_shift, &x0, &y0, &x1, &y1);
            stbtt_MakeCodepointBitmapSubpixel(&font, &rnd_data[index(int(xpos) + x0, int(base_line) + y0)], x1 - x0, y1 - y0, img_width, w_scale, h_scale, x_shift, y_shift, c);
            xpos += w_scale * (advance + stbtt_GetCodepointKernAdvance(&font, c, next_c));
        }
        {
            const wchar_t c = text[txt_size_less];
            if (c != '\n')
            {
                int x0, y0, x1, y1;
                const float x_shift = xpos - float(floor(xpos));
                stbtt_GetCodepointBitmapBoxSubpixel(&font, c, w_scale, h_scale, x_shift, y_shift, &x0, &y0, &x1, &y1);
                stbtt_MakeCodepointBitmapSubpixel(&font, &rnd_data[index(int(xpos) + x0, int(base_line) + y0)], x1 - x0, y1 - y0, img_width, w_scale, h_scale, x_shift, y_shift, c);
            }
        }

        std::vector<unsigned char> img_pixels(img_width * img_height << 2);
        int img_index = 0;
        for (const unsigned char c : rnd_data)
        {
            if (c == 0)
            {
                img_pixels[img_index++] = 0;
                img_pixels[img_index++] = 0;
                img_pixels[img_index++] = 0;
                img_pixels[img_index++] = 0;
            }
            else
            {
                img_pixels[img_index++] = 255;
                img_pixels[img_index++] = 255;
                img_pixels[img_index++] = 255;
                img_pixels[img_index++] = c;
            }
        }
        return img_pixels;
    }
};

static void png_write_function(void *context, void *data, int size)
{
    std::ofstream *baked_file = reinterpret_cast<std::ofstream *>(context);
    baked_file->write(reinterpret_cast<const char *>(data), size);
}

unsigned char buffer[24 << 20];
unsigned char screen[20][100];

int main(int arg, char **argv)
{
    std::ifstream ttf_file("c:/windows/fonts/arialbd.ttf", std::ios::binary | std::ios::in);
    ttf_file.seekg(0, std::ios::end);
    std::vector<unsigned char> ttf_data(ttf_file.tellg());
    ttf_file.seekg(0);
    ttf_file.read(reinterpret_cast<char *>(ttf_data.data()), ttf_data.size());
    Font2D fnt(ttf_data);
    int img_height, img_width;
    float ratio = 0.0f;
    const auto img_data = fnt.bake(L"Hello World\nWhat a nice text you", 1000, 200, 10, ratio);
    //     for(int i = 0 ; i < 100; ++i)
    //     for(int j = 0 ; j < 20; ++j)
    //     screen[j][i] = 0;

    //    stbtt_fontinfo font;
    //    int i,j,ascent,baseline,ch=0;
    //    float scale, xpos=2; // leave a little padding in case the character extends left
    //    char *text = "Heljo World!"; // intentionally misspelled to show 'lj' brokenness

    //    fread(buffer, 1, 1000000, fopen("c:/windows/fonts/arialbd.ttf", "rb"));

    //    scale = stbtt_ScaleForPixelHeight(&font, 15);
    //    stbtt_GetFontVMetrics(&font, &ascent,0,0);
    //    baseline = (int) (ascent*scale);

    //    while (text[ch]) {
    //       int advance,lsb,x0,y0,x1,y1;
    //       float x_shift = xpos - (float) floor(xpos);
    //       stbtt_GetCodepointHMetrics(&font, text[ch], &advance, &lsb);
    //       stbtt_GetCodepointBitmapBoxSubpixel(&font, text[ch], scale,scale,x_shift,0, &x0,&y0,&x1,&y1);
    //       stbtt_MakeCodepointBitmapSubpixel(&font, &screen[baseline + y0][(int) xpos + x0], x1-x0,y1-y0, 100, scale, scale,x_shift,0, text[ch]);
    //       // note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
    //       // because this API is really for baking character bitmaps into textures. if you want to render
    //       // a sequence of characters, you really need to render each bitmap to a temp buffer, then
    //       // "alpha blend" that into the working buffer
    //       xpos += (advance * scale);
    //       if (text[ch+1])
    //          xpos += scale*stbtt_GetCodepointKernAdvance(&font, text[ch],text[ch+1]);
    //       ++ch;
    //    }

    //    std::uint32_t img_data[20][100];

    //    for (j=0; j < 20; ++j) {
    //       for (i=0; i < 100; ++i) {
    //          putchar(" .:ioVM@"[screen[j][i]>>5]);
    //          if(screen[j][i] != 0)
    //             img_data[j][i] = 0X00FFFFFF | ((std::uint32_t) screen[j][i]) << 24;
    //          else img_data[j][i] = 0;
    //       }
    //       putchar('\n');
    //    }

    std::ofstream baked_file("1.png", std::ios::binary | std::ios::out);
    stbi_write_png_to_func(
        png_write_function,
        &baked_file,
        1000, 200,
        4, img_data.data(), 0);

    return 0;
}