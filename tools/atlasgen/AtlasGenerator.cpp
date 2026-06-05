// AI-Generated Code GPT-5.2

// AtlasGenerator.cpp
// C++20
// Requires:
//   stb_image.h
//   stb_image_write.h
//   stb_rect_pack.h
//
// Example:
// AtlasGenerator
//   -input assets/ui
//   -output build/ui
//   -width 1024
//   -height 1024
//   -padding 2
//   -extrude 2
//   -rotate false
//
// NOTE:
// This is a starter implementation. Rotation packing flag is parsed but
// stb_rect_pack itself does not rotate rectangles. Add a pre-pass later.

#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

struct ImageEntry
{
    uint32_t Id;
    std::string Name;
    std::filesystem::path Path;

    int32_t Width;
    int32_t Height;
    int32_t Channels;

    uint8_t* Pixels;

    int32_t AtlasPage;
    int32_t AtlasX;
    int32_t AtlasY;
};

struct Options
{
    std::string InputDir;
    std::string OutputPrefix;

    int32_t Width = 1024;
    int32_t Height = 1024;

    int32_t Padding = 2;
    int32_t Extrude = 2;

    bool Rotate = false;
};

static bool ParseBool(const std::string& s)
{
    return s == "1" || s == "true" || s == "TRUE";
}

static bool ParseArgs(int argc, char** argv, Options* o)
{
    for(int i=1;i<argc;i++)
    {
        std::string a = argv[i];

        if(a == "-input" && i+1 < argc) o->InputDir = argv[++i];
        else if(a == "-output" && i+1 < argc) o->OutputPrefix = argv[++i];
        else if(a == "-width" && i+1 < argc) o->Width = std::stoi(argv[++i]);
        else if(a == "-height" && i+1 < argc) o->Height = std::stoi(argv[++i]);
        else if(a == "-padding" && i+1 < argc) o->Padding = std::stoi(argv[++i]);
        else if(a == "-extrude" && i+1 < argc) o->Extrude = std::stoi(argv[++i]);
        else if(a == "-rotate" && i+1 < argc) o->Rotate = ParseBool(argv[++i]);
    }

    return !o->InputDir.empty() && !o->OutputPrefix.empty();
}

static bool IsImage(const std::filesystem::path& p)
{
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".png" ||
           ext == ".jpg" ||
           ext == ".jpeg";
}

static bool LoadImages(const Options& opt, std::vector<ImageEntry>* images)
{
    uint32_t id = 0;

    for(auto& e : std::filesystem::directory_iterator(opt.InputDir))
    {
        if(!e.is_regular_file()) continue;
        if(!IsImage(e.path())) continue;

        ImageEntry img{};

        img.Id = id++;
        img.Name = e.path().stem().string();
        img.Path = e.path();

        img.Pixels = stbi_load(
            e.path().string().c_str(),
            &img.Width,
            &img.Height,
            &img.Channels,
            4);

        img.Channels = 4;

        if(!img.Pixels)
        {
            std::cerr << "Failed: " << e.path() << "\n";
            continue;
        }

        images->push_back(img);
    }

    std::sort(
        images->begin(),
        images->end(),
        [](const ImageEntry& a, const ImageEntry& b)
        {
            return (a.Width * a.Height) > (b.Width * b.Height);
        });

    return !images->empty();
}

static void CopyPixel(
    uint8_t* dst,
    int32_t atlasW,
    int32_t x,
    int32_t y,
    const uint8_t* src)
{
    uint8_t* p = dst + ((y * atlasW + x) * 4);
    p[0] = src[0];
    p[1] = src[1];
    p[2] = src[2];
    p[3] = src[3];
}

static bool PackPage(
    const Options& opt,
    std::vector<ImageEntry*>& pending,
    std::vector<ImageEntry*>& packed,
    int32_t& outW,
    int32_t& outH)
{
    outW = opt.Width;
    outH = opt.Height;

    while(true)
    {
        std::vector<stbrp_rect> rects(pending.size());
        std::vector<stbrp_node> nodes(outW);

        stbrp_context ctx;

        stbrp_init_target(
            &ctx,
            outW,
            outH,
            nodes.data(),
            (int)nodes.size());

        for(size_t i=0;i<pending.size();i++)
        {
            rects[i].id = (int)i;
            rects[i].w =
                pending[i]->Width +
                2 * (opt.Padding + opt.Extrude);

            rects[i].h =
                pending[i]->Height +
                2 * (opt.Padding + opt.Extrude);
        }

        stbrp_pack_rects(
            &ctx,
            rects.data(),
            (int)rects.size());

        packed.clear();

        for(auto& r : rects)
        {
            if(r.was_packed)
            {
                ImageEntry* img = pending[r.id];

                img->AtlasX =
                    r.x + opt.Padding + opt.Extrude;

                img->AtlasY =
                    r.y + opt.Padding + opt.Extrude;

                packed.push_back(img);
            }
        }

        if(!packed.empty())
            return true;

        outW = (int32_t)std::ceil(outW * 1.15);
        outH = (int32_t)std::ceil(outH * 1.15);

        if(outW > 16384 || outH > 16384)
            return false;
    }
}

static void WriteAtlasMetadata(
    const std::string& file,
    const std::vector<ImageEntry*>& page,
    int32_t atlasW,
    int32_t atlasH)
{
    std::ofstream f(file);

    for(auto* e : page)
    {
        float u0 = (float)e->AtlasX / atlasW;
        float v0 = (float)e->AtlasY / atlasH;

        float u1 =
            (float)(e->AtlasX + e->Width) / atlasW;

        float v1 =
            (float)(e->AtlasY + e->Height) / atlasH;

        f
        << e->Id << ";"
        << e->Name << ";"
        << e->AtlasPage << ";"
        << u0 << ";"
        << v0 << ";"
        << u1 << ";"
        << v1 << "\n";
    }
}

int main(int argc, char** argv)
{
    Options opt;

    if(!ParseArgs(argc, argv, &opt))
    {
        std::cout
            << "Usage: AtlasGenerator "
            << "-input DIR "
            << "-output PREFIX\n";

        return 1;
    }

    std::vector<ImageEntry> images;

    if(!LoadImages(opt, &images))
    {
        std::cerr << "No images loaded\n";
        return 1;
    }

    std::vector<ImageEntry*> remaining;

    for(auto& i : images)
        remaining.push_back(&i);

    int32_t pageIndex = 0;

    while(!remaining.empty())
    {
        std::vector<ImageEntry*> packed;

        int32_t atlasW;
        int32_t atlasH;

        if(!PackPage(
            opt,
            remaining,
            packed,
            atlasW,
            atlasH))
        {
            std::cerr << "Packing failed\n";
            return 1;
        }

        std::vector<uint8_t> atlas(
            (size_t)atlasW * atlasH * 4,
            0);

        for(auto* img : packed)
        {
            img->AtlasPage = pageIndex;

            for(int y=0;y<img->Height;y++)
            {
                for(int x=0;x<img->Width;x++)
                {
                    const uint8_t* src =
                        img->Pixels +
                        ((y * img->Width + x) * 4);

                    CopyPixel(
                        atlas.data(),
                        atlasW,
                        img->AtlasX + x,
                        img->AtlasY + y,
                        src);
                }
            }
        }

        std::string atlasName =
            opt.OutputPrefix +
            "_page_" +
            std::to_string(pageIndex) +
            ".png";

        stbi_write_png(
            atlasName.c_str(),
            atlasW,
            atlasH,
            4,
            atlas.data(),
            atlasW * 4);

        std::string metaName =
            opt.OutputPrefix +
            "_page_" +
            std::to_string(pageIndex) +
            ".txt";

        WriteAtlasMetadata(
            metaName,
            packed,
            atlasW,
            atlasH);

        remaining.erase(
            std::remove_if(
                remaining.begin(),
                remaining.end(),
                [&](ImageEntry* p)
                {
                    return std::find(
                        packed.begin(),
                        packed.end(),
                        p) != packed.end();
                }),
            remaining.end());

        pageIndex++;
    }

    for(auto& i : images)
    {
        if(i.Pixels)
            stbi_image_free(i.Pixels);
    }

    return 0;
}
