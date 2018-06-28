// The IYFEngine
//
// Copyright (C) 2015-2018, Manvydas Šliamka
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
// conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
// of conditions and the following disclaimer in the documentation and/or other materials
// provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
// used to endorse or promote products derived from this software without specific prior
// written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
// WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GRAPHICSAPIENUM_HPP
#define GRAPHICSAPIENUM_HPP

#include "utilities/Flags.hpp"
// TODO propper template based flags
#include "utilities/enum_flag_ops.hpp"

#include <string>

namespace iyf {
namespace renderingConstants {
    const auto ExternalSubpass = ~0U;
}

enum class AccessFlagBits : std::uint32_t {
    IndirectCommandRead         = 0x00001,
    IndexRead                   = 0x00002,
    VertexAttributeRead         = 0x00004,
    UniformRead                 = 0x00008,
    InputAttachmentRead         = 0x00010,
    ShaderRead                  = 0x00020,
    ShaderWrite                 = 0x00040,
    ColorAttachmentRead         = 0x00080,
    ColorAttachmentWrite        = 0x00100,
    DepthStencilAttachmentRead  = 0x00200,
    DepthStencilAttachmentWrite = 0x00400,
    TransferRead                = 0x00800,
    TransferWrite               = 0x01000,
    HostRead                    = 0x02000,
    HostWrite                   = 0x04000,
    MemoryRead                  = 0x08000,
    MemoryWrite                 = 0x10000
};
using AccessFlags = Flags<AccessFlagBits>;
DEFINE_ENUM_FLAG_OPS(AccessFlagBits)

enum class AttachmentDescriptionFlagBits : std::uint32_t {
    MayAlias = 0x1
};
using AttachmentDescriptionFlags = Flags<AttachmentDescriptionFlagBits>;
DEFINE_ENUM_FLAG_OPS(AttachmentDescriptionFlagBits)

enum class AttachmentLoadOp {
    Load,
    Clear,
    DoNotCare,
    COUNT
};

enum class AttachmentStoreOp {
    Store,
    DoNotCare,
    COUNT
};

enum class BackendType {
    Vulkan
};

enum class BlendFactor {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha,
    COUNT
};

enum class BlendOp {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
    COUNT
};

enum class BorderColor {
    FloatTransparentBlack,
    IntTransparentBlack,
    FloatOpaqueBlack,
    IntOpaqueBlack,
    FloatOpaqueWhite,
    IntOpaqueWhite,
    COUNT
};

enum class BufferLevel {
    Primary,
    Secondary
};

enum class BufferUpdateFrequency {
    Never,
    OncePerFrame,
    ManyTimesPerFrame//TODO remove
};

enum class BufferUsageFlagBits : std::uint32_t {
    TransferSource      = 0x001,
    TransferDestination = 0x002,
    UniformTexelBuffer  = 0x004,
    StorageTexelBuffer  = 0x008,
    UniformBuffer       = 0x010,
    StorageBuffer       = 0x020,
    IndexBuffer         = 0x040,
    VertexBuffer        = 0x080,
    IndirectBuffer      = 0x100
};

using BufferUsageFlags = Flags<BufferUsageFlagBits>;
DEFINE_ENUM_FLAG_OPS(BufferUsageFlagBits)

enum class CommandBufferUsageFlagBits : std::uint32_t  {
    OneTimeSubmit      = 0x01,
    RenderPassContinue = 0x02,
    SimultaneousUse    = 0x04
};
using CommandBufferUsageFlags = Flags<CommandBufferUsageFlagBits>;
DEFINE_ENUM_FLAG_OPS(CommandBufferUsageFlagBits)

enum class CompareOp {                                // X11 defines 'Always', Vulkan pulls X11, things go boom, TODO #undef?
    Never,
    Less,
    LessEqual,
    Equal,
    Greater,
    GreaterEqual,
    Always,
    NotEqual,
    COUNT
};

enum class ComponentSwizzle {
    Identity,
    Zero,
    One,
    R,
    G,
    B,
    A,
    COUNT
};

enum class ColorWriteMaskFlagBits : std::uint32_t {
    Red   = 0x01,
    Green = 0x02,
    Blue  = 0x04,
    Alpha = 0x08
};
using ColorWriteMaskFlags = Flags<ColorWriteMaskFlagBits>;
DEFINE_ENUM_FLAG_OPS(ColorWriteMaskFlagBits)

enum class CullModeFlagBits : std::uint32_t {
    Front        = 0x01,
    Back         = 0x02,
    COUNT        = 2,
    None         = 0x00,
    FrontAndBack = 0x03
};
using CullModeFlags = Flags<CullModeFlagBits>;
DEFINE_ENUM_FLAG_OPS(CullModeFlagBits)

enum class DependencyFlagBits : std::uint32_t {
    ByRegion = 0x1,
};
using DependencyFlags = Flags<DependencyFlagBits>;
DEFINE_ENUM_FLAG_OPS(DependencyFlagBits)

enum class DescriptorType {
    Sampler,
    CombinedImageSampler,
    SampledImage,
    StorageImage,
    UniformTexelBuffer,
    StorageTexelBuffer,
    UniformBuffer,
    StorageBuffer,
    UniformBufferDynamic,
    StorageBufferDynamic,
    InputAttachment
};

enum class DynamicState {
    Viewport,
    Scissor,
    LineWidth,
    DepthBias,
    BlendConstants,
    DepthBounds,
    StencilCompareMask,
    StencilWriteMask,
    StencilReference,
    COUNT
};

enum class Filter {
    Nearest,
    Linear//TODO?VK_FILTER_CUBIC_IMG
};

enum class Format {
    Undefined,
    R4_G4_uNorm_pack8,
    R4_G4_B4_A4_uNorm_pack16,
    B4_G4_R4_A4_uNorm_pack16,
    R5_G6_B5_uNorm_pack16,
    B5_G6_R5_uNorm_pack16,
    R5_G5_B5_A1_uNorm_pack16,
    B5_G5_R5_A1_uNorm_pack16,
    A1_R5_G5_B5_uNorm_pack16,
    R8_uNorm,
    R8_sNorm,
    R8_uScaled,
    R8_sScaled,
    R8_uInt,
    R8_sInt,
    R8_sRGB,
    R8_G8_uNorm,
    R8_G8_sNorm,
    R8_G8_uScaled,
    R8_G8_sScaled,
    R8_G8_uInt,
    R8_G8_sInt,
    R8_G8_sRGB,
    R8_G8_B8_uNorm,
    R8_G8_B8_sNorm,
    R8_G8_B8_uScaled,
    R8_G8_B8_sScaled,
    R8_G8_B8_uInt,
    R8_G8_B8_sInt,
    R8_G8_B8_sRGB,
    B8_G8_R8_uNorm,
    B8_G8_R8_sNorm,
    B8_G8_R8_uScaled,
    B8_G8_R8_sScaled,
    B8_G8_R8_uInt,
    B8_G8_R8_sInt,
    B8_G8_R8_sRGB,
    R8_G8_B8_A8_uNorm,
    R8_G8_B8_A8_sNorm,
    R8_G8_B8_A8_uScaled,
    R8_G8_B8_A8_sScaled,
    R8_G8_B8_A8_uInt,
    R8_G8_B8_A8_sInt,
    R8_G8_B8_A8_sRGB,
    B8_G8_R8_A8_uNorm,
    B8_G8_R8_A8_sNorm,
    B8_G8_R8_A8_uScaled,
    B8_G8_R8_A8_sScaled,
    B8_G8_R8_A8_uInt,
    B8_G8_R8_A8_sInt,
    B8_G8_R8_A8_sRGB,
    A8_B8_G8_R8_uNorm_pack32,
    A8_B8_G8_R8_sNorm_pack32,
    A8_B8_G8_R8_uScaled_pack32,
    A8_B8_G8_R8_sScaled_pack32,
    A8_B8_G8_R8_uInt_pack32,
    A8_B8_G8_R8_sInt_pack32,
    A8_B8_G8_R8_sRGB_pack32,
    A2_R10_G10_B10_uNorm_pack32,
    A2_R10_G10_B10_sNorm_pack32,
    A2_R10_G10_B10_uScaled_pack32,
    A2_R10_G10_B10_sScaled_pack32,
    A2_R10_G10_B10_uInt_pack32,
    A2_R10_G10_B10_sInt_pack32,
    A2_B10_G10_R10_uNorm_pack32,
    A2_B10_G10_R10_sNorm_pack32,
    A2_B10_G10_R10_uScaled_pack32,
    A2_B10_G10_R10_sScaled_pack32,
    A2_B10_G10_R10_uInt_pack32,
    A2_B10_G10_R10_sInt_pack32,
    R16_uNorm,
    R16_sNorm,
    R16_uScaled,
    R16_sScaled,
    R16_uInt,
    R16_sInt,
    R16_sFloat,
    R16_G16_uNorm,
    R16_G16_sNorm,
    R16_G16_uScaled,
    R16_G16_sScaled,
    R16_G16_uInt,
    R16_G16_sInt,
    R16_G16_sFloat,
    R16_G16_B16_uNorm,
    R16_G16_B16_sNorm,
    R16_G16_B16_uScaled,
    R16_G16_B16_sScaled,
    R16_G16_B16_uInt,
    R16_G16_B16_sInt,
    R16_G16_B16_sFloat,
    R16_G16_B16_A16_uNorm,
    R16_G16_B16_A16_sNorm,
    R16_G16_B16_A16_uScaled,
    R16_G16_B16_A16_sScaled,
    R16_G16_B16_A16_uInt,
    R16_G16_B16_A16_sInt,
    R16_G16_B16_A16_sFloat,
    R32_uInt,
    R32_sInt,
    R32_sFloat,
    R32_G32_uInt,
    R32_G32_sInt,
    R32_G32_sFloat,
    R32_G32_B32_uInt,
    R32_G32_B32_sInt,
    R32_G32_B32_sFloat,
    R32_G32_B32_A32_uInt,
    R32_G32_B32_A32_sInt,
    R32_G32_B32_A32_sFloat,
    R64_uInt,
    R64_sInt,
    R64_sFloat,
    R64_G64_uInt,
    R64_G64_sInt,
    R64_G64_sFloat,
    R64_G64_B64_uInt,
    R64_G64_B64_sInt,
    R64_G64_B64_sFloat,
    R64_G64_B64_A64_uInt,
    R64_G64_B64_A64_sInt,
    R64_G64_B64_A64_sFloat,
    B10_G11_R11_uFloat_pack32,
    E5_B9_G9_R9_uFloat_pack32,
    D16_uNorm,
    X8_D24_uNorm_pack32,
    D32_sFloat,
    S8_uInt,
    D16_uNorm_S8_uInt,
    D24_uNorm_S8_uInt,
    D32_sFloat_S8_uInt,
    BC1_RGB_uNorm_block,
    BC1_RGB_sRGB_block,
    BC1_RGBA_uNorm_block,
    BC1_RGBA_sRGB_block,
    BC2_uNorm_block,
    BC2_sRGB_block,
    BC3_uNorm_block,
    BC3_sRGB_block,
    BC4_uNorm_block,
    BC4_sNorm_block,
    BC5_uNorm_block,
    BC5_sNorm_block,
    BC6H_uFloat_block,
    BC6H_sFloat_block,
    BC7_uNorm_block,
    BC7_sRGB_block,
    ETC2_R8_G8_B8_uNorm_block,
    ETC2_R8_G8_B8_sRGB_block,
    ETC2_R8_G8_B8_A1_uNorm_block,
    ETC2_R8_G8_B8_A1_sRGB_block,
    ETC2_R8_G8_B8_A8_uNorm_block,
    ETC2_R8_G8_B8_A8_sRGB_block,
    EAC_R11_uNorm_block,
    EAC_R11_sNorm_block,
    EAC_R11_G11_uNorm_block,
    EAC_R11_G11_sNorm_block,
    ASTC_4x4_uNorm_block,
    ASTC_4x4_sRGB_block,
    ASTC_5x4_uNorm_block,
    ASTC_5x4_sRGB_block,
    ASTC_5x5_uNorm_block,
    ASTC_5x5_sRGB_block,
    ASTC_6x5_uNorm_block,
    ASTC_6x5_sRGB_block,
    ASTC_6x6_uNorm_block,
    ASTC_6x6_sRGB_block,
    ASTC_8x5_uNorm_block,
    ASTC_8x5_sRGB_block,
    ASTC_8x6_uNorm_block,
    ASTC_8x6_sRGB_block,
    ASTC_8x8_uNorm_block,
    ASTC_8x8_sRGB_block,
    ASTC_10x5_uNorm_block,
    ASTC_10x5_sRGB_block,
    ASTC_10x6_uNorm_block,
    ASTC_10x6_sRGB_block,
    ASTC_10x8_uNorm_block,
    ASTC_10x8_sRGB_block,
    ASTC_10x10_uNorm_block,
    ASTC_10x10_sRGB_block,
    ASTC_12x10_uNorm_block,
    ASTC_12x10_sRGB_block,
    ASTC_12x12_uNorm_block,
    ASTC_12x12_sRGB_block,
    PVRTC1_2BPP_uNorm_block,
    PVRTC1_4BPP_uNorm_block,
    PVRTC2_2BPP_uNorm_block,
    PVRTC2_4BPP_uNorm_block,
    PVRTC1_2BPP_sRGB_block,
    PVRTC1_4BPP_sRGB_block,
    PVRTC2_2BPP_sRGB_block,
    PVRTC2_4BPP_sRGB_block,
    COUNT,
};

enum class FrontFace {
    Clockwise,
    CounterClockwise,
    COUNT
};

enum class ImageAspectFlagBits : std::uint32_t {
    Color    = 0x01,
    Depth    = 0x02,
    Stencil  = 0x04,
    Metadata = 0x08
};
using ImageAspectFlags = Flags<ImageAspectFlagBits>;
DEFINE_ENUM_FLAG_OPS(ImageAspectFlagBits)

enum class ImageLayout {
    Undefined,
    General, 
    ColorAttachmentOptimal,
    DepthStencilAttachmentOptimal,
    DepthStencilReadOnlyOptimal,
    ShaderReadOnlyOptimal,
    TransferSrcOptimal,
    TransferDstOptimal,
    Preinitialized,
    PresentSource,
    COUNT
};

enum class ImageMemoryType {
    R32,
    RGB,
    RGBA,
    RGBAHalf,
    DepthStencilFloat
};

enum class ImageViewType {
    Im1D,
    Im2D,
    Im3D,
    ImCube,
    Im1DArray,
    Im2DArray,
    ImCubeArray,
    COUNT
};

enum class IndexType {
    UInt16,
    UInt32
};

enum class LogicOp {
    Clear,
    And,
    AndReverse,
    Copy,
    AndInverted,
    NoOp,
    Xor,
    Or,
    Nor,
    Equivalent,
    Invert,
    OrReverse,
    CopyInverted,
    OrInverted,
    Nand,
    Set,
    COUNT
};

enum class MemoryType {
    DeviceLocal,
    HostVisible
};

enum class MultithreadingSupport {
    None,
    Full
};

enum class PipelineBindPoint : std::uint32_t {
    Graphics,
    Compute
};

enum class PipelineStageFlagBits : std::uint32_t {
    TopOfPipe             = 0x00001, 
    DrawIndirect          = 0x00002,
    VertexInput           = 0x00004,
    VertexShader          = 0x00008,
    TessControlShader     = 0x00010,
    TessEvaluationShader  = 0x00020,
    GeometryShader        = 0x00040,
    FragmentShader        = 0x00080,
    EarlyFragmentTests    = 0x00100,
    LateFragmentTests     = 0x00200,
    ColorAttachmentOutput = 0x00400,
    ComputeShader         = 0x00800,
    Transfer              = 0x01000,
    BottomOfPipe          = 0x02000,
    Host                  = 0x04000,
    AllGraphics           = 0x08000,
    AllCommands           = 0x10000
};
using PipelineStageFlags = Flags<PipelineStageFlagBits>;
DEFINE_ENUM_FLAG_OPS(PipelineStageFlagBits)

enum class PolygonMode {
    Fill,
    Line,
    Point,
    COUNT
};

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineListWithAdjacency,
    LineStripWithAdjacency,
    TriangleListWithAdjacency,
    TriangleStripWithAdjacency,
    PatchList,
    COUNT
};

enum class QueueType {
    Graphics,
    Compute,
    Transfer
};

enum class SampleCountFlagBits : std::uint32_t {
    X1  = 0x01,
    X2  = 0x02,
    X4  = 0x04,
    X8  = 0x08,
    X16 = 0x10,
    X32 = 0x20,
    X64 = 0x40,
    COUNT = 7
};
using SampleCountFlags = Flags<SampleCountFlagBits>;
DEFINE_ENUM_FLAG_OPS(SampleCountFlagBits)

enum class SamplerAddressMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge
};

enum class SamplerMipmapMode {
    Nearest,
    Linear
};

enum class SamplerPreset {
    ImguiTexture,
    Default3DModel,
    SkyBox
};

enum class ShaderStageFlagBits : std::uint32_t {
    Vertex         = 0x01,
    TessControl    = 0x02,
    TessEvaluation = 0x04,
    Geometry       = 0x08,
    Fragment       = 0x10,
    Compute        = 0x20,
    COUNT          = 6
};
using ShaderStageFlags = Flags<ShaderStageFlagBits>;
DEFINE_ENUM_FLAG_OPS(ShaderStageFlagBits)

namespace con {
/// Returns appropriate ShaderStageFlags when provided an extension. Must be in lowercase and start with
/// a dot, e.g., ".vert"
///
/// \return ShaderStageFlags with appropriate ShaderStageFlagBits set or empty (0) ShaderStageFlags if 
/// the extension wasn't know or propperly formatted
extern ShaderStageFlags ExtensionToShaderStage(const std::string& extension);
}

enum class StencilOp {
    Keep,
    Zero,
    Replace,
    IncrementAndClamp,
    DecrementAndClamp,
    Invert,
    IncrementAndWrap,
    DecrementAndWrap,
    COUNT
};

enum class SubpassContents {
    Inline,
    SecondaryCommandBuffers
};

enum class VertexInputRate {
    Vertex,
    Instance
};

}

#endif /* GRAPHICSAPIENUM_HPP */

