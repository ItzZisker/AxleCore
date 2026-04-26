#pragma once

#include "axle/utils/AX_MagicPool.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Universal.hpp"
#include "axle/utils/AX_Span.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "glm/glm.hpp"

#include "stb_rect_pack.h"

#include <filesystem>
#include <vector>
#include <cstdint>
#include <unordered_map>

#define AX_FONT_UNIVERSAL_CHARS "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"

namespace axle::gfx
{

namespace FT { utils::ExResult<FT_Library> Get(); }

struct CharGlyphMetrics {
    glm::ivec2 advance;
    glm::ivec2 bearing;
};

struct CharGlyph {
    glm::ivec2 size;
    std::vector<uint8_t> bitmap;
    CharGlyphMetrics metrics;
};

struct CharacterAtlasTag {};
struct CharacterAtlasHandle : public utils::MagicHandleTagged<CharacterAtlasTag> {};

struct AtlasGlyph {
    wchar_t codepoint;
    glm::ivec2 pos;    // packed position
    glm::ivec2 size;   // width/height
    CharGlyphMetrics metrics;
};

struct CharacterAtlas : public utils::MagicInternal<CharacterAtlasHandle> {
    uint32_t width;
    uint32_t height;

    std::unordered_map<wchar_t, AtlasGlyph> glyphs;
    std::vector<uint8_t> buffer;
};

class BitmapFont {
private:
    utils::MagicPool<CharacterAtlas> m_Pages{};

    std::unordered_map<wchar_t, CharGlyph> m_Glyphs{};
    std::unordered_map<wchar_t, CharacterAtlasHandle> m_PageByGlyph{};

    // FT_Library m_FTLib{};
    FT_Face m_FTFace{};

    bool m_FTFaceInit{false};
    bool m_Transformed{false};

    bool PackIntoBestFitAtlas(
        const std::vector<stbrp_rect>& inRects,
        uint32_t& outW, uint32_t& outH,
        std::vector<stbrp_rect>& outRects
    );
public:
    ~BitmapFont();

    const std::vector<CharacterAtlas>& GetPages();
    CharacterAtlas* GetPage(wchar_t _char);

    utils::ExError LoadFont(const std::filesystem::path& filename);
    utils::ExError SetGlyphSize(uint32_t fWidth, uint32_t fHeight);

    utils::ExError GenerateGlyph(wchar_t _char);
    utils::ExError GenerateGlyphs(const utils::Range<wchar_t>& charsRange);
    utils::ExError GenerateGlyphs();

    utils::ExError TransformToPages(uint32_t pageSize = 512);
};

}