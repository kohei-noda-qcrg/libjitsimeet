#pragma once
#include "array-util.hpp"

enum class CodecType {
    Opus,
    H264,
    Vp8,
    Vp9,
};

inline const auto codec_type_str = make_str_table<CodecType>({
    {CodecType::Opus, "opus"},
    {CodecType::H264, "H264"},
    {CodecType::Vp8, "VP8"},
    {CodecType::Vp9, "VP9"},
});

