#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

struct DXTHeader {
    uint8_t magic[4];   // "DXT\0"
    uint16_t width;
    uint16_t height;
    uint8_t alpha;      // 0 = BC1/DXT1, 1 = BC3/DXT5
    uint8_t reserved;   // alignment padding
};

// Compress raw RGBA image to BC1 or BC3
std::vector<unsigned char> compressDXT(const unsigned char* rgba, int width, int height, bool alpha)
{
    int blockBytes = alpha ? 16 : 8; // BC3 = 16 bytes/block, BC1 = 8 bytes/block
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;

    std::vector<unsigned char> compressed(blocksX * blocksY * blockBytes);
    unsigned char rgbaBlock[4 * 4 * 4];
    unsigned char* dest = compressed.data();

    for (int by = 0; by < blocksY; ++by)
    {
        for (int bx = 0; bx < blocksX; ++bx)
        {
            // Fill 4×4 RGBA block
            for (int y = 0; y < 4; ++y)
            {
                for (int x = 0; x < 4; ++x)
                {
                    int srcX = bx * 4 + x;
                    int srcY = by * 4 + y;

                    unsigned char* dstPixel = rgbaBlock + 4 * (y * 4 + x);
                    if (srcX < width && srcY < height)
                    {
                        const unsigned char* srcPixel = rgba + 4 * (srcY * width + srcX);
                        memcpy(dstPixel, srcPixel, 4);
                    }
                    else
                    {
                        memset(dstPixel, 0, 4);
                    }
                }
            }

            // Compress block
            stb_compress_dxt_block(dest, rgbaBlock, alpha ? 1 : 0, STB_DXT_HIGHQUAL);
            dest += blockBytes;
        }
    }

    return compressed;
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <input> <output> <bc1|bc3|auto>\n";
        return 1;
    }

    const char* inputFile = argv[1];
    const char* outputFile = argv[2];
    std::string mode = argv[3];
    bool alpha = false;
    bool autoMode = false;

    if (mode == "bc1" || mode == "BC1" || mode == "dxt1" || mode == "DXT1")
        alpha = false;
    else if (mode == "bc3" || mode == "BC3" || mode == "dxt5" || mode == "DXT5")
        alpha = true;
    else if (mode == "auto" || mode == "AUTO")
        autoMode = true;
    else {
        std::cerr << "Invalid mode. Must be 'bc1', 'bc3', or 'auto'.\n";
        return 1;
    }

    int width, height, channels;
    unsigned char* data = stbi_load(inputFile, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load image: " << inputFile << "\n";
        return 1;
    }

    std::cout << "Loaded " << inputFile << " (" << width << "x" << height << ", " << channels << " channels)\n";

    if (autoMode)
    {
        if (channels == 4)
        {
            alpha = true;
            std::cout << "Auto-detected RGBA → using BC3/DXT5\n";
        }
        else
        {
            alpha = false;
            std::cout << "Auto-detected RGB/Gray → using BC1/DXT1\n";
        }
    }

    // Force convert to RGBA for uniform processing
    unsigned char* rgbaData = nullptr;
    if (channels != 4)
    {
        rgbaData = new unsigned char[width * height * 4];
        for (int i = 0; i < width * height; ++i)
        {
            if (channels >= 3)
            {
                rgbaData[i * 4 + 0] = data[i * channels + 0];
                rgbaData[i * 4 + 1] = data[i * channels + 1];
                rgbaData[i * 4 + 2] = data[i * channels + 2];
                rgbaData[i * 4 + 3] = 255;
            }
            else if (channels == 1)
            {
                rgbaData[i * 4 + 0] = data[i];
                rgbaData[i * 4 + 1] = data[i];
                rgbaData[i * 4 + 2] = data[i];
                rgbaData[i * 4 + 3] = 255;
            }
        }
    }
    else
    {
        rgbaData = data;
    }

    std::cout << "Compressing to " << (alpha ? "BC3/DXT5" : "BC1/DXT1") << "...\n";

    auto compressed = compressDXT(rgbaData, width, height, alpha);

    DXTHeader header = {};
    memcpy(header.magic, "DXT", 3);
    header.magic[3] = 0;
    header.width = static_cast<uint16_t>(width);
    header.height = static_cast<uint16_t>(height);
    header.alpha = alpha ? 1 : 0;

    std::ofstream out(outputFile, std::ios::binary);
    if (!out)
    {
        std::cerr << "Failed to open output file: " << outputFile << "\n";
        stbi_image_free(data);
        if (rgbaData != data) delete[] rgbaData;
        return 1;
    }

    out.write(reinterpret_cast<char*>(&header), sizeof(header));
    out.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    out.close();

    std::cout << "Saved " << outputFile << " (" << compressed.size() << " bytes)\n";

    if (rgbaData != data)
        delete[] rgbaData;
    stbi_image_free(data);

    return 0;
}
