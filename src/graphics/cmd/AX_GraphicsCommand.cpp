#include "axle/graphics/cmd/AX_GraphicsCommand.hpp"

namespace axle::graphics {

GS_Handle GS_CreateHVal(GS_HandleEnum type, std::size_t raw) {
    GS_HandleVal&& val { .ptr = (int8_t*)raw };
    return {type, val};
}

GS_Handle GS_CreateHValNull() {
    return GS_CreateHVal(GSH_NullPtr, (size_t)nullptr);
}

GS_HandleVar GS_ReadHVal(GS_HandleEnum type, GS_HandleVal uni) {
    switch (type) {
        case GSH_Internal: return std::move(uni.ptr);
        case GSH_Int32: return std::move(uni.i32);
        case GSH_Int64: return std::move(uni.i64);
        case GSH_UInt32: return std::move(uni.f32);
        case GSH_UInt64: return std::move(uni.i64);
        case GSH_Float32: return std::move(uni.f32);
        case GSH_NullPtr: {
            int8_t* nll{nullptr};
            return nll;
        }
    }
}

GS_HandleVar GS_ReadH(GS_Handle pair) {
    return GS_ReadHVal(pair.first, pair.second);
}

}