#pragma once

#include <stdint.h>
#include <vector>
#include <string>

namespace vision
{
    enum PaddingMode
    {
        No,
        Black,
        White,
    };

    enum PatchesFormat
    {
        // patches-left-right-down channels-rgb pixels-left-right-down
        // [patch0, patch1, ...,    // row0: left --> right
        //  ....................    // row1: left --> right
        //  ....................]
        // patch   ::= [ch_r, ch_g, ch_b]
        // channel ::= [value0, value1, // row0 within a patch: left --> right
        //              ...]            // row1 ....
        PatchesLeftRightDown_ChannelsRGB_PixelsLeftRightDown,

        // patches-left-right-down pixels-left-right-down channels-rgb
        // [patch0, patch1, ...,    // row0: left --> right
        //  ....................    // row1: left --> right
        //  ....................]
        // patch   ::= [p0, p1, ...,    // row0 within a patch: left --> right
        //              ...]            // row1 ....
        // pixel   ::= [r, g, b]
        PatchesLeftRightDown_PixelsLeftRightDown_ChannelsRGB,

        // channels-rgb patches-left-right-down pixels-left-right-down
        // [ch_r, ch_g, ch_b]
        // channel ::= [patch0, patch1, ...,    // row0: left --> right
        //              ....................    // row1: left --> right
        //              ....................]
        // patch   ::= [v0, v1, ...,    // row0 within a patch: left --> right
        //              ...]            // row1 ....
        ChannelsRGB_PatchesLeftRightDown_PixelsLeftRightDown,

        // channels-rgb pixels-left-right-down
        // [ch_r, ch_g, ch_b]
        // channel   ::= [v0, v1, ...,    // row0 within the picture: left --> right
        //                ...]            // row1 ....
        ChannelsRGB_PixelsLeftRightDown,
    };

    class Resize
    {
    public:
        Resize(int width, int height);
        ~Resize(void);
    };

    class MergeKernel
    {
    public:
        MergeKernel(int size0, int size1);
        MergeKernel(const int *sizes);
        ~MergeKernel(void);
    };

    class MaxGridWidth
    {
    public:
        MaxGridWidth(int value);
        ~MaxGridWidth();
    };

    class MaxGridHeight
    {
    public:
        MaxGridHeight(int value);
        ~MaxGridHeight();
    };

    class MaxPatchNum
    {
    public:
        MaxPatchNum(int value);
        ~MaxPatchNum();
    };

    void image_dimension(const char *fn, int &width, int &height);
    void image_load(const char *fn, std::vector<uint8_t> &rgb_pixels, int &width, int &height, int patch_size, PaddingMode pad = PaddingMode::No);
    void image_rescale(const std::vector<uint8_t> &rgb_pixels, std::vector<float> &scaled_rgb_pixels, float scale_factor = 1/255.0f);
    void image_normalize(std::vector<float> &rgb_pixels, const float *mean, const float *std_d);

    // ASSUMPTION: already properly aligned to `patch_size`
    void image_arrange(const std::vector<float> &rgb_pixels, const int width, const int patch_size,
        std::vector<float> &arranged, const PatchesFormat fmt);

    void test(const char *fn);
}