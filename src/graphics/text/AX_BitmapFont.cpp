#include "axle/graphics/text/AX_BitmapFont.hpp"

#include <iostream>

using namespace axle::utils;

namespace axle::gfx
{

// class BitmapFont {
// private:
//     utils::MagicPool<CharacterAtlas> m_Pages{};

//     std::map<wchar_t, CharGlyph> m_Glyphs{};
//     bool m_Transformed{false};
// public:

namespace FT {
    ExResult<FT_Library> Get() {
        static FT_Library local{};
        static bool ready{false};

        if (ready) return local;
        FT_Error err = FT_Init_FreeType(&local);
        if (err) {
            return {"Could not Initialize FreeType Library, Error: " + std::to_string(err)};
        } else {
            ready = true;
            return local;
        };
    }
};

BitmapFont::~BitmapFont() {
    FT_Done_Face(m_FTFace);
}

ExError BitmapFont::LoadFont(const std::filesystem::path& filename) {
    if (!m_FTFaceInit) return {"an FT_Face already loaded"};
    FT_Error err = FT_New_Face(m_FTLib, filename.string().c_str(), 0, &m_FTFace);
    if (err) {
        return {"Failed to load font, Error: " + std::to_string(err)};
    } else {
        m_FTFaceInit = true;
        return ExError::NoError();
    };
}

ExError BitmapFont::SetGlyphSize(uint32_t fWidth, uint32_t fHeight) {
    if (!m_FTFaceInit) return {"No FT_Face (Font) loaded yet"};
    FT_Set_Pixel_Sizes(m_FTFace, fWidth, fHeight);
    return ExError::NoError();
}

const ExResult<CharacterAtlas>& BitmapFont::GetPage(wchar_t _char) {
    for (auto& [_char, idx] : m_PageIndexByGlyph) {
        std::wcout << "[" << _char << ", " << idx << "]\n";
    }
    auto it = m_PageIndexByGlyph.find(_char);
    if (it == m_PageIndexByGlyph.end()) {
        std::cout << "Page not found\n";
        return ExError{"Page not found for char=" + std::to_string(_char)};
    }

    auto atlasIdx = it->second;

    return m_Pages.GetRaw(atlasIdx);
}

ExError BitmapFont::GenerateGlyph(wchar_t _char) {
    if (!m_FTFaceInit) return {"No FT_Face (Font) loaded yet"};
    if (m_Glyphs.find(_char) != m_Glyphs.end()) return {"This glyph already exists"};

    FT_Error err = FT_Load_Char(m_FTFace, _char, FT_LOAD_RENDER);
    if (err) return {"Failed to load Glyph for character '" + std::to_string(_char) + "', Error: " + std::to_string(err)};

    CharGlyphMetrics metrics {
        .advance = {m_FTFace->glyph->advance.x,   m_FTFace->glyph->advance.y},
        .bearing = {m_FTFace->glyph->bitmap_left, m_FTFace->glyph->bitmap_top}
    };

    glm::ivec2 size = {m_FTFace->glyph->bitmap.width, m_FTFace->glyph->bitmap.rows};

    std::vector<uint8_t> bitmap = std::vector<uint8_t>(size.x * size.y);
    std::memcpy(bitmap.data(), m_FTFace->glyph->bitmap.buffer, bitmap.size());

    CharGlyph glyph {
        .size = size,
        .bitmap = std::move(bitmap),
        .metrics = metrics
    };

    m_Glyphs.insert({_char, glyph});
    return ExError::NoError();
}

ExError BitmapFont::GenerateGlyphs(const Range<wchar_t>& charsRange) {
    if (!m_FTFaceInit) return {"No FT_Face (Font) loaded yet"};

    auto from = charsRange.From();
    auto to = charsRange.To();

    bool hasErrors{false};
    std::stringstream errStream;
    errStream << "Failed to Load Glyphs:\n";

    for (wchar_t _char = from; _char <= to; _char++) {
        auto err = BitmapFont::GenerateGlyph(_char);
        if (err.IsValid()) {
            hasErrors = true;
            errStream << err.GetMessage() << std::endl;
        }
    }

    if (hasErrors) {
        return {errStream.str()};
    } else {
        return ExError::NoError();
    }
}

bool BitmapFont::PackIntoBestFitAtlas(
    const std::vector<stbrp_rect>& inRects,
    uint32_t& outW, uint32_t& outH,
    std::vector<stbrp_rect>& outRects
) {
    outRects = inRects;

    int w = 64;
    int h = 64;

    while (true) {
        std::vector<stbrp_rect> attempt = inRects;

        std::vector<stbrp_node> nodes(w);
        stbrp_context ctx;

        stbrp_init_target(&ctx, w, h, nodes.data(), nodes.size());
        stbrp_pack_rects(&ctx, attempt.data(), attempt.size());

        bool allPacked = true;
        for (auto& r : attempt) {
            if (!r.was_packed) { allPacked = false; break; }
        }

        if (allPacked) {
            outW = w;
            outH = h;
            outRects = std::move(attempt);
            return true;
        }

        if (w < h)  w *= 2;
        else        h *= 2;

        if (w > 8192 || h > 8192)
            return false;
    }
}

ExError BitmapFont::TransformToPages() {
    if (m_Transformed) return {"Already transformed, use another instance of BitmapFont"};

    std::vector<std::vector<std::pair<wchar_t, CharGlyph>>> blocks;

    std::vector<std::pair<wchar_t, CharGlyph>> current;
    current.reserve(512);

    for (auto &p : m_Glyphs) {
        if (current.size() == 512) {
            blocks.push_back(std::move(current));
            current = {};
            current.reserve(512);
        }
        current.push_back(p);
    }

    if (!current.empty())
        blocks.push_back(std::move(current));
    
    for (auto& block : blocks) {
        uint32_t atlasW = 2048;
        uint32_t atlasH = 2048;

        stbrp_context ctx;
        std::vector<stbrp_node> nodes(atlasW);

        stbrp_init_target(&ctx, atlasW, atlasH, nodes.data(), nodes.size());
        
        std::vector<stbrp_rect> rects, packed;
        rects.reserve(block.size());

        for (auto& [_char, glyph] : m_Glyphs) {
            rects.push_back({
                .id = _char,
                .w = glyph.size.x,
                .h = glyph.size.y
            });   
        }

        stbrp_pack_rects(&ctx, rects.data(), rects.size());

        if (!PackIntoBestFitAtlas(rects, atlasW, atlasH, packed)) {
            return {"Packing failed: atlas too large"};
        }

        auto& page = *m_Pages.Reserve();

        page.width = atlasW;
        page.height = atlasH;

        std::vector<uint8_t> atlasPixels(atlasW * atlasH, 0);

        for (auto &rect : packed) {
            auto &glyph = m_Glyphs[rect.id];

            for (int row = 0; row < glyph.size.y; row++) {
                memcpy(&atlasPixels[(rect.y + row) * atlasW + rect.x],
                    &glyph.bitmap[row * glyph.size.x],
                    glyph.size.x);
            }
            auto& original = m_Glyphs[rect.id];

            m_PageIndexByGlyph.insert({(wchar_t)rect.id, page.index});

            page.buffer = std::move(atlasPixels);
            page.glyphs.insert({rect.id,
                AtlasGlyph {
                    .codepoint = (wchar_t)rect.id,
                    .pos = {rect.x, rect.y},
                    .size = original.size,
                    .metrics = original.metrics
                }
            });
        }
    }

    m_Transformed = true;
    return ExError::NoError();
}

// };

}