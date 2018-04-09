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

#ifdef BUILD_WITH_OPENGL
#include "graphics/gl/OpenGLBackend.hpp"

//#include <SDL_syswm.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>

#include "localization/Localization.hpp"

#include <gli/gli.hpp>
#include <glm/glm.hpp>
#include "utilities/util.hpp"

#include "stb_image.h"
// TODO react to config changes
namespace iyf {
// TODO veikia ne su visais kompiliatoriais
void OpenGLBackend::oglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return; //TODO iš parametro
    
	std::cerr << "GL-";
    switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		std::cerr << "error;               ";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cerr << "deprecated-behaviour;";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cerr << "undefined-behaviour; ";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::cerr << "portability;         ";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cerr << "performance;         ";
		break;
    case GL_DEBUG_TYPE_MARKER:
        std::cerr << "type marker;         ";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        std::cerr << "push group;          ";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        std::cerr << "pop group;           ";
        break;
	case GL_DEBUG_TYPE_OTHER:
		std::cerr << "other;               ";
		break;
	}
	
	std::cerr << "Priority: ";
	
	switch (severity) {
	case GL_DEBUG_SEVERITY_NOTIFICATION:
	    std::cerr << "INFO;  ";
	    break;
    case GL_DEBUG_SEVERITY_LOW:
	    std::cerr << "LOW;   ";
	    break;
    case GL_DEBUG_SEVERITY_MEDIUM:
	    std::cerr << "MEDIUM;";
	    break;
    case GL_DEBUG_SEVERITY_HIGH:
	    std::cerr << "HIGH;  ";
	    break;
	}
	
	std::cerr << "Source: ";
	switch (source) {
	case GL_DEBUG_SOURCE_API:
	    std::cerr << "OpenGL API;     ";
	    break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
	    std::cerr << "window system;  ";
	    break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
	    std::cerr << "shader compiler;";
	    break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
	    std::cerr << "third party;    ";
	    break;
	case GL_DEBUG_SOURCE_APPLICATION:
	    std::cerr << "application;    ";
	    break;
	case GL_DEBUG_SOURCE_OTHER:
	    std::cerr << "other;          ";
	    break;
	}
	
	std::cerr << "ID: " << id << "\n\t" << message;
	
	if (userParam != nullptr) {
        std::cerr << "; user data was attached.";
    }
    
    std::cerr << std::endl;
}

bool OpenGLBackend::initialize() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	
    if (isDebug) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG | SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    //SDL_GL_SetAttribute(SDL_GL_STEREO, 1);

    // TODO Anti-aliasing. Gražu, bet labai stabdo, gal įjungti?
    // Labiau simuliuotų realų atvejį.
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    openWindow();
	
	// Sukuriam kontekstą ir pririšam jį prie lango    
    context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

    //glewExperimental = GL_TRUE;//TODO įjungi, jei naudosiu GLEW_ARB_gl_spirv
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        throw std::runtime_error("GLEW failed to load extensions");
    }
    
    if (!GLEW_VERSION_4_5) {
        throw std::runtime_error("OpenGL 4.5 not supported");
    }
    
    //if (GLEW_ARB_gl_spirv) { // NAY be experimental
    //    std::cout << "YAY" << std::endl;
    //} else {
    //    std::cout << "NAY" << std::endl;
    //}
    
    std::cout << "GLEW:" << glewGetString(GLEW_VERSION) << std::endl;
    
    if (isDebug) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(oglDebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
    } else {
        glDisable(GL_DEBUG_OUTPUT);
        glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
    
    printWMInfo();
    
    //GLint mvab = 0;
    //glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &mvab);
    //std::cout << "Max vertex attribute bindings: " << mvab << std::endl;
    //GLint mubb = 0;
    //glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &mubb);
    //std::cout << "Max vertex attribute bindings: " << mubb << std::endl;

    // TODO Čia likusi Anti-aliasing nustatymų dalis.
    //glEnable(GL_MULTISAMPLE);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    //glEnable(GL_LINE_SMOOTH);
    //glEnable(GL_POLYGON_SMOOTH);
    
    // Pagal https://www.opengl.org/registry/specs/ARB/clip_control.txt žr. 14 klausimą apie tikslumo pagerinimą
    //glDepthRangef(1.0f, 0.0f); ar glDepthRange(1.0f, 0.0f); // TODO float gylio buferio tikslumo pagerinimui. Reikia tokio buferio, ir suprasti, kodėl "glDepthFunc(GL_LESS) should become glDepthRange(GL_GREATER)" nustoja viską piešti, pakeitus. Ten TURBŪT reikia invertuoti gylį shader'yje
    glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE); // Kaip DirectX ir Vulkan.
    
    GLint64 t;
    //glGetInteger64v(GL_MAX_VERTEX_ATTRIB_BINDINGS, &t);
    glGetInteger64v(GL_MIN_MAP_BUFFER_ALIGNMENT, &t);
    std::cout << t << std::endl;
    
    loadAuxiliaryMeshes();
    
    return true;
}

void OpenGLBackend::dispose() {
    destroyAuxiliaryMeshes();
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
}

//void OpenGLBackend::swapBuffers() {
//    SDL_GL_SwapWindow(window);
//}

bool OpenGLBackend::startFrame() {
    return true;
}

bool OpenGLBackend::endFrame() {
    SDL_GL_SwapWindow(window);
    return true;
}

GLbitfield OpenGLBackend::getGLShaderBitType(ShaderStageFlag type) {
    switch (type) {
    case ShaderStageFlag::Vertex:
        return GL_VERTEX_SHADER_BIT;
    case ShaderStageFlag::Fragment:
        return GL_FRAGMENT_SHADER_BIT;
    case ShaderStageFlag::Geometry:
        return GL_GEOMETRY_SHADER_BIT;
    case ShaderStageFlag::TessControl:
        return GL_TESS_CONTROL_SHADER_BIT;
    case ShaderStageFlag::TessEvaluation:
        return GL_TESS_EVALUATION_SHADER_BIT;
    case ShaderStageFlag::Compute:
        return GL_COMPUTE_SHADER_BIT;
    default:
        throw std::invalid_argument("Invalid shader bit enumerator value");
    }
}

GLenum OpenGLBackend::getGLShaderType(ShaderStageFlag type) {
    switch (type) {
    case ShaderStageFlag::Vertex:
        return GL_VERTEX_SHADER;
    case ShaderStageFlag::Fragment:
        return GL_FRAGMENT_SHADER;
    case ShaderStageFlag::Geometry:
        return GL_GEOMETRY_SHADER;
    case ShaderStageFlag::TessControl:
        return GL_TESS_CONTROL_SHADER;
    case ShaderStageFlag::TessEvaluation:
        return GL_TESS_EVALUATION_SHADER;
    case ShaderStageFlag::Compute:
        return GL_COMPUTE_SHADER;
    default:
        throw std::invalid_argument("Invalid shader type enumerator value");
    }
}

GLenum OpenGLBackend::getCompareOp(CompareOp type) {
    switch (type) {
    case CompareOp::Never:
        return GL_NEVER;
    case CompareOp::Less:
        return GL_LESS;
    case CompareOp::LessEqual:
        return GL_LEQUAL;
    case CompareOp::Equal:
        return GL_EQUAL;
    case CompareOp::Greater:
        return GL_GREATER;
    case CompareOp::GreaterEqual:
        return GL_GEQUAL;
    case CompareOp::Always:
        return GL_ALWAYS;
    case CompareOp::NotEqual:
        return GL_NOTEQUAL;
    default:
        throw std::invalid_argument("Invalid depth function enumerator value");
    }
}

GLenum OpenGLBackend::getBlendFactor(BlendFactor type) {
    switch (type) {
    case BlendFactor::Zero:
        return GL_ZERO;
    case BlendFactor::One:
        return GL_ONE;
    case BlendFactor::SrcColor:
        return GL_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case BlendFactor::DstColor:
        return GL_DST_COLOR;
    case BlendFactor::OneMinusDstColor:
        return GL_ONE_MINUS_DST_COLOR;
    case BlendFactor::SrcAlpha:
        return GL_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DstAlpha:
        return GL_DST_ALPHA;
    case BlendFactor::OneMinusDstAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case BlendFactor::ConstantColor:
        return GL_CONSTANT_COLOR;
    case BlendFactor::OneMinusConstantColor:
        return GL_ONE_MINUS_CONSTANT_COLOR;
    case BlendFactor::ConstantAlpha:
        return GL_CONSTANT_ALPHA;
    case BlendFactor::OneMinusConstantAlpha:
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    case BlendFactor::SrcAlphaSaturate:
        return GL_SRC_ALPHA_SATURATE;
    case BlendFactor::Src1Color:
        return GL_SRC1_COLOR;
    case BlendFactor::OneMinusSrc1Color:
        return GL_ONE_MINUS_SRC1_COLOR;
    case BlendFactor::Src1Alpha:
        return GL_SRC1_ALPHA;
    case BlendFactor::OneMinusSrc1Alpha:
        return GL_ONE_MINUS_SRC1_ALPHA;
    default:
        throw std::invalid_argument("Invalid blend factor enumerator value");
    }
}

GLenum OpenGLBackend::getBlendOp(BlendOp type) {
    switch (type) {
    case BlendOp::Add:
        return GL_FUNC_ADD;
    case BlendOp::Subtract:
        return GL_FUNC_SUBTRACT;
    case BlendOp::ReverseSubtract:
        return GL_FUNC_REVERSE_SUBTRACT;
    case BlendOp::Min:
        return GL_MIN;
    case BlendOp::Max:
        return GL_MAX;
    default:
        throw std::invalid_argument("Invalid blend operation enumerator value");
    }
}

GLenum OpenGLBackend::getStencilOp(StencilOp op) {
    switch (op) {
    case StencilOp::Keep:
        return GL_KEEP;
    case StencilOp::Zero:
        return GL_ZERO;
    case StencilOp::Replace:
        return GL_REPLACE;
    case StencilOp::IncrementAndClamp:
        return GL_INCR;
    case StencilOp::DecrementAndClamp:
        return GL_DECR;
    case StencilOp::Invert:
        return GL_INVERT;
    case StencilOp::IncrementAndWrap:
        return GL_INCR_WRAP;
    case StencilOp::DecrementAndWrap:
        return GL_DECR_WRAP;
    default:
        throw std::runtime_error("Invalid stencil operation enumerator value");
    }
}

GLenum OpenGLBackend::mapTopology(PrimitiveTopology topology) {
    switch (topology) {
    case PrimitiveTopology::PointList:
        return GL_POINTS;
    case PrimitiveTopology::LineList:
        return GL_LINES;
    case PrimitiveTopology::LineStrip:
        return GL_LINE_STRIP;
    case PrimitiveTopology::TriangleList:
        return GL_TRIANGLES;
    case PrimitiveTopology::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case PrimitiveTopology::TriangleFan:
        return GL_TRIANGLE_FAN;
    case PrimitiveTopology::LineListWithAdjacency:
        return GL_LINES_ADJACENCY;
    case PrimitiveTopology::LineStripWithAdjacency:
        return GL_LINE_STRIP_ADJACENCY;
    case PrimitiveTopology::TriangleListWithAdjacency:
        return GL_TRIANGLES_ADJACENCY;
    case PrimitiveTopology::TriangleStripWithAdjacency:
        return GL_TRIANGLE_STRIP_ADJACENCY;
    case PrimitiveTopology::PatchList:
        return GL_PATCHES;
    default:
        throw std::runtime_error("Invalid topology");
    }//GL_LINE_LOOP???
}

GLint OpenGLBackend::mapAttributeSize(Format format) {
    switch (format) {
    case Format::R32G32SFloat:
        return 2;
    case Format::R32G32B32SFloat:
        return 3;
    case Format::A2B10G10R10SNorm:
        return 4;
    case Format::R8G8SNorm:
        return 2;
    case Format::R8G8B8A8UNorm:
        return 4;
    case Format::R8G8B8A8SNorm:
        return 4;
    case Format::R8G8B8UInt:
        return 3;
    case Format::R8G8B8A8UInt:
        return 4;
    }
    
    throw std::runtime_error("Invalid format");
}

GLenum OpenGLBackend::mapAttributeFormat(Format format) {
    switch (format) {
    case Format::R32G32SFloat:
        return GL_FLOAT;
    case Format::R32G32B32SFloat:
        return GL_FLOAT;
    case Format::A2B10G10R10SNorm:
        return GL_INT_2_10_10_10_REV;
    case Format::R8G8SNorm:
        return GL_BYTE;
    case Format::R8G8B8UNorm:
        return GL_UNSIGNED_BYTE;
    case Format::R8G8B8A8UNorm:
        return GL_UNSIGNED_BYTE;
    case Format::R8G8B8A8SNorm:
        return GL_BYTE;
    case Format::R8G8B8UInt:
        return GL_UNSIGNED_BYTE;
    case Format::R8G8B8A8UInt:
        return GL_UNSIGNED_BYTE;
    }
    
    throw std::runtime_error("Invalid format");
}

GLboolean OpenGLBackend::mapAttributeNormalization(Format format) {
    switch (format) {
    case Format::R32G32SFloat:
        return GL_FALSE;
    case Format::R32G32B32SFloat:
        return GL_FALSE;
    case Format::A2B10G10R10SNorm:
        return GL_FALSE;
    case Format::R8G8SNorm:
        return GL_TRUE;
    case Format::R8G8B8UNorm:
        return GL_TRUE;
    case Format::R8G8B8A8UNorm:
        return GL_TRUE;
    case Format::R8G8B8A8SNorm:
        return GL_TRUE;
    case Format::R8G8B8UInt:
        return GL_FALSE;
    case Format::R8G8B8A8UInt:
        return GL_FALSE;
    }

    throw std::runtime_error("Invalid format");
}

GLenum OpenGLBackend::getPolygonMode(PolygonMode type) {
    switch (type) {
    case PolygonMode::Fill:
        return GL_FILL;
    case PolygonMode::Line:
        return GL_LINE;
    case PolygonMode::Point:
        return GL_POINT;
    default:
        throw std::invalid_argument("Invalid polygon mode");
    }
}

GLenum OpenGLBackend::mapLogicOp(LogicOp op) {
    switch (op) {
    case LogicOp::Clear:
        return GL_CLEAR;
    case LogicOp::And:
        return GL_AND;
    case LogicOp::AndReverse:
        return GL_AND_REVERSE;
    case LogicOp::Copy:
        return GL_COPY;
    case LogicOp::AndInverted:
        return GL_AND_INVERTED;
    case LogicOp::NoOp:
        return GL_NOOP;
    case LogicOp::Xor:
        return GL_XOR;
    case LogicOp::Or:
        return GL_OR;
    case LogicOp::Nor:
        return GL_NOR;
    case LogicOp::Equivalent:
        return GL_EQUIV;
    case LogicOp::Invert:
        return GL_INVERT;
    case LogicOp::OrReverse:
        return GL_OR_REVERSE;
    case LogicOp::CopyInverted:
        return GL_COPY_INVERTED;
    case LogicOp::OrInverted:
        return GL_OR_INVERTED;
    case LogicOp::Nand:
        return GL_NAND;
    case LogicOp::Set:
        return GL_SET;
    default:
        throw std::runtime_error("Invalid logic Op");
    }
}

GLenum OpenGLBackend::getCullMode(CullMode type) {
    switch (type) {
    case CullMode::None:
        return 0;
    case CullMode::Front:
        return GL_FRONT;
    case CullMode::Back:
        return GL_BACK;
    case CullMode::FrontAndBack:
        return GL_FRONT_AND_BACK;
    default:
        throw std::invalid_argument("Invalid cull mode");
    }
}

GLenum OpenGLBackend::getFrontFaceType(FrontFace type) {
    switch (type) {
    case FrontFace::Clockwise:
        return GL_CW;
    case FrontFace::CounterClockwise:
        return GL_CCW;
    default:
        throw std::invalid_argument("Invalid front face type");
    }
}

GLenum OpenGLBackend::getIndexType(IndexType type) {
    switch (type) {
    case IndexType::UInt16:
        return GL_UNSIGNED_SHORT;
    case IndexType::UInt32:
        return GL_UNSIGNED_INT;
    default:
        throw std::invalid_argument("Invalid index type");
    }
}

std::uint32_t OpenGLBackend::mapSampleCount(SampleCount count) {
    switch (count) {
    case SampleCount::X1:
        return 1;
    case SampleCount::X2:
        return 2;
    case SampleCount::X4:
        return 4;
    case SampleCount::X8:
        return 8;
    case SampleCount::X16:
        return 16;
    case SampleCount::X32:
        return 32;
    case SampleCount::X64:
        return 64;
    default:
        throw std::runtime_error(LOC("invalid_sample_count"));
    }
}

ShaderHnd OpenGLBackend::createShader(ShaderStageFlag shaderStageFlag, const std::string& path) {
    std::ifstream fs("data/shaders/" + path);
    if (!fs.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    
    fs.seekg(0, std::ios::end);
    size_t size = fs.tellg();
    std::string buff(size, ' ');
    fs.seekg(0);
    fs.read(&buff[0], size);// TODO C++17 fs.read(buff.data(), size);
    
    return createShaderFromSource(shaderStageFlag, buff);
}

ShaderHnd OpenGLBackend::createShaderFromSource(ShaderStageFlag shaderStageFlag, const std::string& source) {
    const char *s = source.c_str();
    GLuint name = glCreateShaderProgramv(getGLShaderType(shaderStageFlag), 1, &s);
    
    GLint result = GL_FALSE;
	int InfoLogLength;
    glGetProgramiv(name, GL_LINK_STATUS, &result);

	if (result == GL_FALSE) {
		glGetProgramiv(name, GL_INFO_LOG_LENGTH, &InfoLogLength);
		std::vector<char> error(InfoLogLength);
		glGetProgramInfoLog(name, InfoLogLength, NULL, &error[0]);
		std::cerr << "Program linking error\n\t " << error.data();
        throw std::runtime_error(error.data());
	}
    
    return name;
}
    
bool OpenGLBackend::destroyShader(ShaderHnd handle) {
    glDeleteProgram(static_cast<GLuint>(handle));
    
    return true;
}

GFXPipelineHnd OpenGLBackend::createGraphicsPipeline(const PipelineCreateInfo& info) {
    GFXPipelineHnd handle = getPipelineHandle();
    
    GLuint name = 0;
    glGenProgramPipelines(1, &name);
    
    for (auto &i : info.shaders) {
        glUseProgramStages(name, getGLShaderBitType(i.first), i.second);
    }
    
    GLuint VAOname = 0;
    glGenVertexArrays(1, &VAOname);
    
    glBindVertexArray(VAOname);
    
    for (auto& d : info.vertexInputState.vertexAttributeDescriptions) {
        glEnableVertexAttribArray(d.location);
        
        glVertexAttribFormat(d.location, mapAttributeSize(d.format), mapAttributeFormat(d.format), mapAttributeNormalization(d.format), d.offset);
        glVertexAttribBinding(d.location, static_cast<GLuint>(d.binding));
    }
    
    glBindVertexArray(0); //TODO? ar reikia atstatyti?

    GLRasterizationState rasterizationState {
        info.rasterizationState.depthClampEnable,
        info.rasterizationState.rasterizerDiscardEnable,
        getPolygonMode(info.rasterizationState.polygonMode),
        getCullMode(info.rasterizationState.cullMode),
        getFrontFaceType(info.rasterizationState.frontFace),
        info.rasterizationState.depthBiasEnable,
        info.rasterizationState.depthBiasConstantFactor,
        info.rasterizationState.depthBiasClamp,
        info.rasterizationState.depthBiasSlopeFactor,
        info.rasterizationState.lineWidth};
    
    GLDepthStencilState depthStencilState {
        info.depthStencilState.depthTestEnable,
        info.depthStencilState.depthWriteEnable,
        getCompareOp(info.depthStencilState.compareOp),
        info.depthStencilState.stencilTestEnable,
        info.depthStencilState.depthBoundsTestEnable,
        info.depthStencilState.minDepthBounds,
        info.depthStencilState.maxDepthBounds,
        
            {getStencilOp(info.depthStencilState.front.failOp),
            getStencilOp(info.depthStencilState.front.passOp),
            getStencilOp(info.depthStencilState.front.depthFailOp),
            getCompareOp(info.depthStencilState.front.compareOp),
            info.depthStencilState.front.compareMask,
            info.depthStencilState.front.writeMask,
            info.depthStencilState.front.reference},
            
            {getStencilOp(info.depthStencilState.back.failOp),
            getStencilOp(info.depthStencilState.back.passOp),
            getStencilOp(info.depthStencilState.back.depthFailOp),
            getCompareOp(info.depthStencilState.back.compareOp),
            info.depthStencilState.back.compareMask,
            info.depthStencilState.back.writeMask,
            info.depthStencilState.back.reference}
    };
    
    GLViewportState viewportState;
    viewportState.viewports.resize(info.viewportState.viewports.size() * 4);
    viewportState.depths.resize(info.viewportState.viewports.size() * 2);
    
    for (size_t i = 0; i < info.viewportState.viewports.size(); ++i) {
        auto &v = info.viewportState.viewports[i];
        
        viewportState.viewports[0 + 4 * i] = static_cast<GLfloat>(v.x);
        viewportState.viewports[1 + 4 * i] = static_cast<GLfloat>(v.y);
        viewportState.viewports[2 + 4 * i] = static_cast<GLfloat>(v.width);
        viewportState.viewports[3 + 4 * i] = static_cast<GLfloat>(v.height);
            
        viewportState.depths[0 + 2 * i] = static_cast<GLdouble>(v.minDepth);
        viewportState.depths[1 + 2 * i] = static_cast<GLdouble>(v.maxDepth);
    }
    
    viewportState.scissors.resize(info.viewportState.scissors.size() * 4);
    for (size_t i = 0; i < info.viewportState.scissors.size(); ++i) {
        auto &s = info.viewportState.scissors[i];
        
        viewportState.scissors[0 + 4 * i] = static_cast<GLint>(s.offset.x);
        viewportState.scissors[1 + 4 * i] = static_cast<GLint>(s.offset.y);
        viewportState.scissors[2 + 4 * i] = static_cast<GLint>(s.extent.x);
        viewportState.scissors[3 + 4 * i] = static_cast<GLint>(s.extent.y);
    }
    
    GLMultisampleState multisampleState {
        mapSampleCount(info.multisampleState.rasterizationSamples),
        info.multisampleState.sampleShadingEnable,
        info.multisampleState.minSampleShading,
        info.multisampleState.sampleMask,
        info.multisampleState.alphaToCoverageEnable,
        info.multisampleState.alphaToOneEnable
    };
    
    GLTessellationState tessellationState {
        info.tessellationState.patchControlPoints
    };
    
    std::vector<GLBlendStates> blendStates;
    
    for (auto &b : info.colorBlendState.attachments) {
        GLBlendStates bs {
            b.blendEnable, 
            getBlendFactor(b.srcColorBlendFactor),
            getBlendFactor(b.dstColorBlendFactor),
            getBlendFactor(b.srcAlphaBlendFactor),
            getBlendFactor(b.dstAlphaBlendFactor),
            getBlendOp(b.colorBlendOp),
            getBlendOp(b.alphaBlendOp),
            b.colorWriteMask};
            
        blendStates.emplace_back(bs);
    }
    
    GLColorBlendState blendState {
        info.colorBlendState.blendConstants,
        blendStates,
        info.colorBlendState.logicOpEnable,
        mapLogicOp(info.colorBlendState.logicOp)
    };
    
    GLInputAssemblyState assemblyState {
        mapTopology(info.inputAssemblyState.topology),
        info.inputAssemblyState.primitiveRestartEnable
    };
    
    pipelines[handle] = {name,
                         VAOname,
                         
                         blendState,
                         rasterizationState,
                         depthStencilState,
                         viewportState,
                         multisampleState,
                         tessellationState,
                         assemblyState,
                         info.vertexInputState};
    
    return handle;
}

void OpenGLBackend::bindGraphicsPipeline(GFXPipelineHnd handle) {
    currentlyBoundPipeline = handle;
    
    // TODO tikrinimas, ar egzistuoja, jei debug mode
    auto &pipeline = pipelines[handle];
    
    glBindProgramPipeline(static_cast<GLuint>(pipeline.pipelineName));
    glBindVertexArray(static_cast<GLuint>(pipeline.VAOName));
    
    //glDisable(GL_CULL_FACE);
    
    if (pipeline.depthStencilState.depthTestOn) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(pipeline.depthStencilState.depthFunction);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    if (pipeline.depthStencilState.depthWriteOn) {
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    
    // TODO patikrinti, ar sistemoje palaikomas plėtinys
    if (pipeline.depthStencilState.depthBoundsTestOn) {
        glEnable(GL_DEPTH_BOUNDS_TEST_EXT);
        glDepthBoundsEXT(pipeline.depthStencilState.depthBoundsMin, pipeline.depthStencilState.depthBoundsMax);
    } else {
        glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
    }
    
    if (pipeline.depthStencilState.stencilTestOn) {
        glEnable(GL_STENCIL_TEST);
        
        // Front
        glStencilMaskSeparate(GL_FRONT, pipeline.depthStencilState.front.writeMaskVal);
        glStencilFuncSeparate(GL_FRONT, pipeline.depthStencilState.front.compare, pipeline.depthStencilState.front.referenceVal, pipeline.depthStencilState.front.compareMaskVal);
        glStencilOpSeparate(GL_FRONT, pipeline.depthStencilState.front.fail, pipeline.depthStencilState.front.depthFail, pipeline.depthStencilState.front.pass);
        //Back
        glStencilMaskSeparate(GL_BACK, pipeline.depthStencilState.back.writeMaskVal);
        glStencilFuncSeparate(GL_BACK, pipeline.depthStencilState.back.compare, pipeline.depthStencilState.back.referenceVal, pipeline.depthStencilState.back.compareMaskVal);
        glStencilOpSeparate(GL_BACK, pipeline.depthStencilState.back.fail, pipeline.depthStencilState.back.depthFail, pipeline.depthStencilState.back.pass);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    
    glPatchParameteri(GL_PATCH_VERTICES, pipeline.tessellation.patchControlPoints);
    // TODO dynamic state
    // TODO kelių FBO palaikymas su glBlendFuncSeparatei ir glBlendEquationSeparatei ir glColorMaski
    if (pipeline.blendState.blendStates[0].enabled) {
        auto &bs = pipeline.blendState.blendStates[0];
        glEnable(GL_BLEND);
        
        glBlendFuncSeparate(bs.srcColBlendFac, bs.dstColBlendFac, bs.srcAlphaBlendFac, bs.dstAlphaBlendFac);
        glBlendEquationSeparate(bs.colBlendOp, bs.alphaBlendOp);
    } else {
        glDisable(GL_BLEND);
    }
    
    auto &bs = pipeline.blendState.blendStates[0];
    glColorMask((static_cast<bool>(bs.colorMask & ColorWriteMaskFlags::Red)) ? GL_TRUE : GL_FALSE,
                (static_cast<bool>(bs.colorMask & ColorWriteMaskFlags::Green)) ? GL_TRUE : GL_FALSE,
                (static_cast<bool>(bs.colorMask & ColorWriteMaskFlags::Blue)) ? GL_TRUE : GL_FALSE,
                (static_cast<bool>(bs.colorMask & ColorWriteMaskFlags::Alpha)) ? GL_TRUE : GL_FALSE);
                
    glBlendColor(pipeline.blendState.blendConst.r, pipeline.blendState.blendConst.g, pipeline.blendState.blendConst.b, pipeline.blendState.blendConst.a);
    
    if (pipeline.blendState.logicOpOn) {
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(pipeline.blendState.logicOpVal);
    } else {
        glDisable(GL_COLOR_LOGIC_OP);
    }
    
    if (pipeline.rasterizationState.isDepthClampEnabled) {
        glEnable(GL_DEPTH_CLAMP);
    } else {
        glDisable(GL_DEPTH_CLAMP);
    }
    
    if (pipeline.rasterizationState.isRasterizerDiscardEnabled) {
        glEnable(GL_RASTERIZER_DISCARD);
    } else {
        glDisable(GL_RASTERIZER_DISCARD);
    }
    
    glFrontFace(pipeline.rasterizationState.frontFaceVal);
    
    if (pipeline.rasterizationState.cullModeVal == 0) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(pipeline.rasterizationState.cullModeVal);
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, pipeline.rasterizationState.polygonModeVal);
    
    // TODO TEST 
    if (pipeline.multisampleState.sampleShadingOn) {
        glEnable(GL_SAMPLE_SHADING);
        glMinSampleShading(pipeline.multisampleState.minSampleShadingVal);
    } else {
        glDisable(GL_SAMPLE_SHADING);
    }
    
    if (pipeline.multisampleState.sampleMaskVal.size() > 0) {
        glEnable(GL_SAMPLE_MASK);
        
        for (size_t i = 0; i < pipeline.multisampleState.sampleMaskVal.size(); ++i) {
            glSampleMaski(i, pipeline.multisampleState.sampleMaskVal[i]);
        }
    } else {
        glDisable(GL_SAMPLE_MASK);
    }
    
    if (pipeline.multisampleState.alphaToCoverageOn) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    
    if (pipeline.multisampleState.alphaToOneOn) {
        glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    }
    
    glLineWidth(pipeline.rasterizationState.lineWidthVal);
    
    // TODO likę rassterizacijos komponentai//GAL JAU VISI?
    if (pipeline.assembly.primitiveRestartEnable) {
        glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    } else {
        glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }
    //primitiveMode = GL_TRIANGLES;
    
    // TODO atstatyti?
    bool dynamicViewport = true, dynamicScissor = true;
    if (!dynamicViewport) {
        glViewportArrayv(0, pipeline.viewportState.viewports.size() / 4, pipeline.viewportState.viewports.data());
        glDepthRangeArrayv(0, pipeline.viewportState.depths.size() / 2, pipeline.viewportState.depths.data());
    }
    
    if (!dynamicScissor) {
        glScissorArrayv(0, pipeline.viewportState.scissors.size() / 4, pipeline.viewportState.scissors.data());
    }
}

void OpenGLBackend::pushConstants(PipelineLayoutHnd handle, ShaderStageFlag flags, std::uint32_t offset, std::uint32_t size, const void* data) {

}

bool OpenGLBackend::destroyGraphicsPipeline(GFXPipelineHnd handle) {
    // TODO tikrinimas, ar egzistuoja, jei debug mode
    auto &pipeline = pipelines[handle];
    
    GLuint name = static_cast<GLuint>(pipeline.pipelineName);
    glDeleteProgramPipelines(1, &name);
    
    GLuint VAOname = static_cast<GLuint>(pipeline.VAOName);
    glDeleteVertexArrays(1, &VAOname);
    
    return true;
}

PipelineLayoutHnd OpenGLBackend::createPipelineLayout(const PipelineLayoutCreateInfo& info) {
    return 0;
}

bool OpenGLBackend::destroyPipelineLayout(PipelineLayoutHnd handle) {
    return true;
}


DescriptorSetLayoutHnd OpenGLBackend::createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info) {
    return 0;
}

bool OpenGLBackend::destroyDescriptorSetLayout(DescriptorSetLayoutHnd handle) {
    return true;
}

std::vector<DescriptorSetHnd> OpenGLBackend::allocateDescriptorSets(const DescriptorSetAllocateInfo& info) {
    return {0};
}

bool OpenGLBackend::updateDescriptorSets(const std::vector<WriteDescriptorSet>& set) {
    return true;
}

DescriptorPoolHnd OpenGLBackend::createDescriptorPool(const DescriptorPoolCreateInfo& info) {
    return 42;
}

bool OpenGLBackend::destroyDescriptorPool(DescriptorPoolHnd handle) {
    return true;
}

bool OpenGLBackend::bindDescriptorSets(PipelineBindPoints point, PipelineLayoutHnd layout, std::uint32_t firstSet, const std::vector<DescriptorSetHnd> descriptorSets, const std::vector<std::uint32_t> dynamicOffsets) {
    return true;
}

Image OpenGLBackend::createImage(const std::string& path) {
    if (util::endsWith(path, ".dds") || (util::endsWith(path, ".DDS"))) {
        return loadDDSImage(path);
    } else {
#ifdef ALLOW_NON_OPTIMAL_TEXTURE_FORMATS
        return loadImage(path);
#else
        throw std::runtime_error("Can only load DDS textures. If you wish to load non optimal formats, enable ALLOW_NON_OPTIMAL_TEXTURE_FORMATS compile flag.");
#endif        
    }
}

Image OpenGLBackend::create2DImageFromMemory(ImageMemoryType type, const glm::ivec2& dimesions, const void* data) {
    GLuint texture;
    glGenTextures(1, &texture);
    
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    Format format;
    if (type == ImageMemoryType::RGB) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dimesions.x, dimesions.y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        format = Format::R8G8B8UInt;
    } else if (type == ImageMemoryType::RGBA) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimesions.x, dimesions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        format = Format::R8G8B8A8UInt;
    } else {
        throw std::runtime_error("Invalid texture type.");
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return {texture, static_cast<std::uint64_t>(dimesions.x), static_cast<std::uint64_t>(dimesions.y), 1, 1, ImageViewType::Im1D, format};
}

//bool OpenGLBackend::activateTexture(const Texture& texture, TextureBnd binding) {
//    // TODO validate bindings, vulkan style descriptor pools?
//    glActiveTexture(GL_TEXTURE0 + binding);
//    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(handle));
//    
//    return true;
//}

bool OpenGLBackend::destroyImage(const Image& image) {
    GLuint name = static_cast<GLuint>(image.handle);
    glDeleteTextures(1, &name);
    
    return true;
}

SamplerHnd OpenGLBackend::createSampler(const SamplerCreateInfo& info) {
    return 0;
}

bool OpenGLBackend::destroySampler(SamplerHnd handle) {
    return true;
}

ImageViewHnd OpenGLBackend::createImageView(const ImageViewCreateInfo& info) {
    return 0;
}

bool OpenGLBackend::destroyImageView(ImageViewHnd handle) {
    return true;
}

UniformBufferSlice OpenGLBackend::createUniformBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
    return UniformBufferSlice(makeBuffer(BufferType::Uniform, size, flag, data), 0, size, flag);
}

//bool OpenGLBackend::bindUniformBuffer(UniformBufferBnd binding, const UniformBufferSlice& slice) {
//    glBindBufferRange(GL_UNIFORM_BUFFER, static_cast<GLuint>(binding), static_cast<GLuint>(slice.handle()), slice.offset(), slice.size());
//    
//    return true;
//}

bool OpenGLBackend::setUniformBufferData(const UniformBufferSlice& slice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return updateBuffer(BufferType::Uniform, slice.handle(), slice.offset(), slice.size(), data);
}

bool OpenGLBackend::updateUniformBufferData(const UniformBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return partialUpdateBuffer(BufferType::Uniform, slice.handle(), slice.offset(), subSlice.offset(), subSlice.size(), data);
}

bool OpenGLBackend::destroyUniformBuffer(const UniformBufferSlice& slice) {
    GLuint name = static_cast<GLuint>(slice.handle());
    glDeleteBuffers(1, &name);
    
    return true;
}

StorageBufferSlice OpenGLBackend::createStorageBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
    return StorageBufferSlice(makeBuffer(BufferType::Storage, size, flag, data), 0, size, flag);
}

bool OpenGLBackend::setStorageBufferData(const StorageBufferSlice& slice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return updateBuffer(BufferType::Storage, slice.handle(), slice.offset(), slice.size(), data);
}

bool OpenGLBackend::updateStorageBufferData(const StorageBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return partialUpdateBuffer(BufferType::Storage, slice.handle(), slice.offset(), subSlice.offset(), subSlice.size(), data);
}

bool OpenGLBackend::destroyStorageBuffer(const StorageBufferSlice& slice) {
    GLuint name = static_cast<GLuint>(slice.handle());
    glDeleteBuffers(1, &name);
    
    return true;
}

VertexBufferSlice OpenGLBackend::createVertexBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
    return VertexBufferSlice(makeBuffer(BufferType::Vertex, size, flag, data), 0, size, flag);
}

bool OpenGLBackend::setVertexBufferData(const VertexBufferSlice& slice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return updateBuffer(BufferType::Vertex, slice.handle(), slice.offset(), slice.size(), data);
}

bool OpenGLBackend::updateVertexBufferData(const VertexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return partialUpdateBuffer(BufferType::Vertex, slice.handle(), slice.offset(), subSlice.offset(), subSlice.size(), data);
}

bool OpenGLBackend::destroyVertexBuffer(const VertexBufferSlice& slice) {
    GLuint name = static_cast<GLuint>(slice.handle());
    glDeleteBuffers(1, &name);
    
    return true;
}

IndexBufferSlice OpenGLBackend::createIndexBuffer(std::uint64_t size, IndexType type, BufferUpdateFrequency flag, const void* data) {
    return IndexBufferSlice(makeBuffer(BufferType::Index, size, flag, data), 0, size, flag, type);
}

bool OpenGLBackend::setIndexBufferData(const IndexBufferSlice& slice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return updateBuffer(BufferType::Index, slice.handle(), slice.offset(), slice.size(), data);
}

bool OpenGLBackend::updateIndexBufferData(const IndexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
    if (slice.usageHint() == BufferUpdateFrequency::Never) {
        throw std::runtime_error(LOC("updating_buffer_frequency_none"));
    }
    
    return partialUpdateBuffer(BufferType::Index, slice.handle(), slice.offset(), subSlice.offset(), subSlice.size(), data);
}

bool OpenGLBackend::destroyIndexBuffer(const IndexBufferSlice& slice) {
    GLuint name = static_cast<GLuint>(slice.handle());
    glDeleteBuffers(1, &name);
    
    return true;
}

void OpenGLBackend::setClear(glm::vec4 color, double depth, int stencil) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClearDepth(depth);
    glClearStencil(stencil); 
}

//void OpenGLBackend::setRenderArea(glm::uvec4 rectangle) {
    //glViewport(rectangle.x, rectangle.y, rectangle.z, rectangle.w);
//}

void OpenGLBackend::setViewport(std::uint32_t first, std::uint32_t count, const std::vector<Viewport>& viewports) {
    if (viewports.size() == 0) {
        //TODO atstatom į visa ekrana.
        return;
    }
    
    viewportTemp.clear();
    depthTemp.clear();
    
    viewportTemp.resize(viewports.size() * 4);
    depthTemp.resize(viewports.size() * 2);
    
    for (size_t i = 0; i < viewports.size(); ++i) {
        auto &v = viewports[i];
        
        viewportTemp[0 + 4 * i] = static_cast<GLfloat>(v.x);
        viewportTemp[1 + 4 * i] = static_cast<GLfloat>(v.y);
        viewportTemp[2 + 4 * i] = static_cast<GLfloat>(v.width);
        viewportTemp[3 + 4 * i] = static_cast<GLfloat>(v.height);
        
        depthTemp[0 + 2 * i] = static_cast<GLdouble>(v.minDepth);
        depthTemp[1 + 2 * i] = static_cast<GLdouble>(v.maxDepth);
    }

    glViewportArrayv(first, count, viewportTemp.data());
    glDepthRangeArrayv(first, count, depthTemp.data());
}

void OpenGLBackend::setScissor(std::uint32_t first, std::uint32_t count, const std::vector<Rect2D>& rectangles) {
    if (rectangles.size() != 0) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
        return;
    }
    
    scissorTemp.clear();
    scissorTemp.resize(rectangles.size() * 4);
    
    for (size_t i = 0; i < rectangles.size(); ++i) {
        auto &r = rectangles[i];
        scissorTemp[0 + 4 * i] = static_cast<GLint>(r.offset.x);
        scissorTemp[1 + 4 * i] = static_cast<GLint>(r.offset.y);
        scissorTemp[2 + 4 * i] = static_cast<GLint>(r.extent.x);
        scissorTemp[3 + 4 * i] = static_cast<GLint>(r.extent.y);
    }

    glScissorArrayv(first, count, scissorTemp.data());
}

void OpenGLBackend::beginPass(ClearFlag clearFlags) {
    GLbitfield mask = 0;
    if (static_cast<bool>(clearFlags & ClearFlag::Color)) {
        mask |= GL_COLOR_BUFFER_BIT;
    }
    
    if (static_cast<bool>(clearFlags & ClearFlag::Depth)) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    
    if (static_cast<bool>(clearFlags & ClearFlag::Stencil)) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    glClear(mask);
}

void OpenGLBackend::draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance) {
    glDrawArraysInstancedBaseInstance(pipelines[currentlyBoundPipeline].assembly.topology, firstVertex, vertexCount, instanceCount, firstInstance);
}

void OpenGLBackend::drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance) {
    //std::cout << currentIndexSize << std::endl;
    GLenum currentIndexSize = getIndexType(currentIndexBuffer.indexType());
    std::uint64_t offsetSizeMultiplier = (currentIndexSize == GL_UNSIGNED_INT) ? 4 : 2;
    glDrawElementsInstancedBaseVertexBaseInstance(pipelines[currentlyBoundPipeline].assembly.topology, indexCount, currentIndexSize, (void*)(offsetSizeMultiplier * firstIndex), instanceCount, vertexOffset, firstInstance);
}

void OpenGLBackend::bindVertexBuffers(std::uint32_t firstBinding, std::uint32_t bindingCount, const std::vector<VertexBufferSlice>& buffers) {
    //currentVertexBuffers = buffers; //TODO bind'ai, kurių stride nėra tarp pipeline kūrimo duomenų
    tempBuffers.clear();
    tempOffsets.clear();
    tempStrides.clear();
    
    if (tempBuffers.capacity() < bindingCount) { // TODO sutvarkius tuos be stride, padaryti dydį fiksuotą ir lygų maksimaliam bind'ų skaičiui
        tempBuffers.resize(bindingCount);
        tempOffsets.resize(bindingCount);
        tempStrides.resize(bindingCount);
    }
    
    std::uint32_t fb = static_cast<std::uint32_t>(firstBinding);
    for (size_t i = 0; i < bindingCount; ++i) { //TODO kol kas įdomūs tik tie, kuriuos rišam. Sutvarkius praeitus 2 TODO, bus įdomūs visi
        tempBuffers[i] = buffers[i].handle();
        tempOffsets[i] = buffers[i].offset();
        
        bool strideFound = false;
        for (auto& p : pipelines[currentlyBoundPipeline].inputState.vertexBindingDescriptions) {
            if (static_cast<std::uint32_t>(p.binding) == fb) {
                tempStrides[i] = p.stride;
                strideFound = true;
                break;
            }
        }
        
        if (!strideFound) {
            std::cerr << fb;
            throw std::runtime_error("Stride not found");
        }
        
        fb++;
    }
    
    glBindVertexBuffers(static_cast<GLuint>(firstBinding), bindingCount, tempBuffers.data(), tempOffsets.data(), tempStrides.data());
}

void OpenGLBackend::bindIndexBuffer(const IndexBufferSlice& slice) {
    currentIndexBuffer = slice;
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currentIndexBuffer.handle()); // TODO offset?
}
    
void OpenGLBackend::endPass() {
    //printWMInfo();
}

bool OpenGLBackend::updateBuffer(BufferType type, std::uint64_t handle, std::uint64_t offset, std::uint64_t size, const void* data) {
    return partialUpdateBuffer(type, handle, offset, 0, size, data);
    
    //GLenum target;
    //switch (type) {
    //    case BufferType::Vertex :
    //        target = GL_ARRAY_BUFFER;
    //        break;
    //    case BufferType::Index :
    //        target = GL_ELEMENT_ARRAY_BUFFER;
    //        break;
    //    case BufferType::Uniform :
    //        target = GL_UNIFORM_BUFFER;
    //        break;
    //}

    //glBindBuffer(target, handle);
    
    //// TODO perdaryti, kad galima butu naudoti GL_MAP_UNSYNCHRONIZED_BIT
    //// https://www.opengl.org/wiki/Buffer_Object_Streaming#Buffer_update
    //// Pagrinde reikia, kad arba visi MVP ir M butu dideliam buferyje, arba sukurti konstantas.
    ////void* p = glMapBufferRange(target, offset, size, GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_WRITE_BIT);
    //void* p = glMapBufferRange(target, offset, size, GL_MAP_WRITE_BIT);
    //std::memcpy(p, data, size);
    //glUnmapBuffer(target);
    //glBindBuffer(target, 0);
    
    //return true;
}

bool OpenGLBackend::partialUpdateBuffer(BufferType type, std::uint64_t handle, std::uint64_t offset, std::uint64_t subOffset, std::uint64_t subSize, const void* data) {
    GLenum target;
    switch (type) {
        case BufferType::Vertex :
            target = GL_ARRAY_BUFFER;
            break;
        case BufferType::Index :
            target = GL_ELEMENT_ARRAY_BUFFER;
            break;
        case BufferType::Uniform :
            target = GL_UNIFORM_BUFFER;
            break;
        default:
            throw("no support for this type");
    }

    glBindBuffer(target, handle);
    
#if defined(PERSISTENT_COHERENT_BUFFER_WRITES)
    char* p = static_cast<char*>(persistentHandles[handle]);
    std::memcpy(p + offset + subOffset, data, subSize);
    //std::cout << (void*)(p + offset + subOffset) << " " << (void*)(p) << " " << offset << " " << subOffset << std::endl;
    
    //GLsync gs = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    //while (true) {
    //    GLenum waitReturn = glClientWaitSync(gs, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
    //    if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED) {
    //        glDeleteSync(gs);
    //        break;
    //    }
    //}
    
#elif defined (PERSISTENT_EXPLICITLY_FLUSHED_BUFFER_WRITES)
    char* p = static_cast<char*>(persistentHandles[handle]);
    std::memcpy(p + offset + subOffset, data, subSize);
    
    glFlushMappedBufferRange(target, static_cast<GLintptr>(offset + subOffset), static_cast<GLsizeiptr>(subSize));
    //glFlushMappedBufferRange(target, 0, size);
    
    //GLsync gs = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    //while (true) {
    //    GLenum waitReturn = glClientWaitSync(gs, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
    //    if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED) {
    //        glDeleteSync(gs);
    //        break;
    //    }
    //}
    
    //glGetBufferParameter()
    GLsizeiptr ginp = subSize;
    std::cout << ginp << " " << handle << " " << (void*)(p + offset + subOffset) << " " << (void*)(p) << " " << offset << " " << subOffset << " " << subSize << " " << size << std::endl;
#else
    // Naudoti šitą? Ar bandyti su map ir sinchronizacija?
    //glBufferData(target, subSize, data, //NeedsFlag);
    
    char* p = static_cast<char*>(glMapBufferRange(target, offset + subOffset, subSize, GL_MAP_WRITE_BIT));
    std::memcpy(p, data, subSize);
    glUnmapBuffer(target);
#endif
    
    //std::cout << "Errr." << glGetError() << std::endl;
    // Atrišimas sugagina VAO būseną. Kol kas nepasitaikė situacijos, kurioje atrišimo reikėtų, BET reikėtų planuoti ką daryti tokiu atveju
    // TODO išsaugoti pradžioje ir atkurti?
    //glBindBuffer(target, 0);
    
    return true;
}

GLuint OpenGLBackend::makeBuffer(BufferType type, std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
    GLenum target;
    switch (type) {
        case BufferType::Vertex :
            target = GL_ARRAY_BUFFER;
            break;
        case BufferType::Index :
            target = GL_ELEMENT_ARRAY_BUFFER;
            break;
        case BufferType::Uniform :
            target = GL_UNIFORM_BUFFER;
            break;
        default:
            throw("no support for this type");
    }
    
    GLuint name = 0;
    glGenBuffers(1, &name);
    glBindBuffer(target, name);
    
    GLenum usage;
    switch (flag) {
    case BufferUpdateFrequency::Never:
        usage = GL_STATIC_DRAW; // TODO padaryti immutable?
        break;
    case BufferUpdateFrequency::OncePerFrame:
        usage = GL_DYNAMIC_DRAW;
        break;
    case BufferUpdateFrequency::ManyTimesPerFrame:
        usage = GL_STREAM_DRAW;
        break;
    default:
        throw std::runtime_error("Invalid buffer usage flag value");
    }
    
    if (flag == BufferUpdateFrequency::Never && data == nullptr) {
        throw std::runtime_error(LOC("device_visible_without_data"));
    }
    // Minimum size? Cache of existing buffers to append to, if small enough? Check Unreal's code, maybe?
    
#if defined(PERSISTENT_COHERENT_BUFFER_WRITES)
    // Pažiūrėti GDC14 Bringing Unreal Engine 4 to OpenGL
    // 1. glMapBuffer pakeisti į malloc + glBufferSubData
    // 2. glGen??? - sukurti daug vardų ir imti iš cache.
    // https://www.gamedev.net/topic/668918-teaching-advice-glbufferdata-or-glmapbuffer/#entry5232850
    // glBufferStorage with these flags is a LOT more efficient than glBufferData, but you need to sync yourself
    // this is more fit for geometry streaming and not uniforms. Check slides ???WHICH AZDO???
    // TODO SYNC https://ferransole.wordpress.com/2014/06/08/persistent-mapped-buffers/
    GLbitfield flags = GL_MAP_WRITE_BIT
                   | GL_MAP_PERSISTENT_BIT
                   | GL_MAP_COHERENT_BIT;
    glBufferStorage(target, size, data, flags);
    
    if (flag != BufferUpdateFrequency::Never) {
        void* p = glMapBufferRange(target, 0, size, flags);
        persistentHandles[name] = p;
    }
#elif defined (PERSISTENT_EXPLICITLY_FLUSHED_BUFFER_WRITES)
    glBufferStorage(target, static_cast<GLsizeiptr>(size), data, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
    
    if (flag != BufferUpdateFrequency::Never) {
        //void* p = glMapBufferRange(target, static_cast<GLintptr>(0), static_cast<GLsizeiptr>(size), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
        void* p = glMapBufferRange(target, static_cast<GLintptr>(0), static_cast<GLsizeiptr>(size), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
        persistentHandles[name] = p;
    }
    
    //GLsizeiptr si = size;
    std::cout << name << " " << size << " " << static_cast<int>(flag) << std::endl;
#else
    glBufferData(target, size, data, usage);
    glBindBuffer(target, 0);
#endif
    
    return name;
}

Image OpenGLBackend::loadDDSImage(const std::string& path) {
    // TODO anisotropy čia ir loadTexture
    gli::texture texture = gli::load(path.c_str());
    if (texture.empty()) {
        throw std::runtime_error(LOC("empty_texture"));
    }
    
    gli::gl GL(gli::gl::PROFILE_GL33);
    gli::gl::format const format = GL.translate(texture.format(), texture.swizzles());
    GLenum target = GL.translate(texture.target());

    GLuint name = 0;
    glGenTextures(1, &name);
    glBindTexture(target, name);
    glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texture.levels() - 1));
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, format.Swizzles[0]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, format.Swizzles[1]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, format.Swizzles[2]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, format.Swizzles[3]);

    glm::tvec3<GLsizei> const extent(texture.extent());
    GLsizei const faceTotal = static_cast<GLsizei>(texture.layers() * texture.faces());

    switch (texture.target()) {
        case gli::TARGET_1D:
            glTexStorage1D(target, static_cast<GLint>(texture.levels()), format.Internal, extent.x);
            break;
        case gli::TARGET_1D_ARRAY:
        case gli::TARGET_2D:
        case gli::TARGET_CUBE:
            glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal,
                           extent.x, texture.target() == gli::TARGET_2D ? extent.y : faceTotal);
            break;
        case gli::TARGET_2D_ARRAY:
        case gli::TARGET_3D:
        case gli::TARGET_CUBE_ARRAY:
            glTexStorage3D(target, static_cast<GLint>(texture.levels()), format.Internal, extent.x, extent.y,
                           texture.target() == gli::TARGET_3D ? extent.z : faceTotal);
            break;
        default:
            assert(0);
            break;
    }

    for (std::size_t layer = 0; layer < texture.layers(); ++layer) {
        for (std::size_t face = 0; face < texture.faces(); ++face) {
            for (std::size_t level = 0; level < texture.levels(); ++level) {
                GLsizei const layerGL = static_cast<GLsizei>(layer);
                glm::tvec3<GLsizei> extent(texture.extent(level));
                target = gli::is_target_cube(texture.target()) 
                        ? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face)
                        : target;

                switch (texture.target()) {
                    case gli::TARGET_1D:
                        if (gli::is_compressed(texture.format())) {
                            glCompressedTexSubImage1D(target, static_cast<GLint>(level), 0, extent.x,
                                                      format.Internal, static_cast<GLsizei>(texture.size(level)),
                                                      texture.data(layer, face, level));
                        } else {
                            glTexSubImage1D(target, static_cast<GLint>(level), 0, extent.x,
                                            format.External, format.Type,
                                            texture.data(layer, face, level));
                        }
                        break;
                    case gli::TARGET_1D_ARRAY:
                    case gli::TARGET_2D:
                    case gli::TARGET_CUBE:
                        if (gli::is_compressed(texture.format())) {
                            glCompressedTexSubImage2D(target, static_cast<GLint>(level), 0, 0, extent.x,
                                                      texture.target() == gli::TARGET_1D_ARRAY ? layerGL : extent.y,
                                                      format.Internal, static_cast<GLsizei>(texture.size(level)),
                                                      texture.data(layer, face, level));
                        } else {
                            glTexSubImage2D(target, static_cast<GLint>(level), 0, 0, extent.x,
                                            texture.target() == gli::TARGET_1D_ARRAY ? layerGL : extent.y,
                                            format.External, format.Type,
                                            texture.data(layer, face, level));
                        }
                        break;
                    case gli::TARGET_2D_ARRAY:
                    case gli::TARGET_3D:
                    case gli::TARGET_CUBE_ARRAY:
                        if (gli::is_compressed(texture.format())) {
                            glCompressedTexSubImage3D(target, static_cast<GLint>(level), 0, 0, 0, extent.x, extent.y, 
                                                      texture.target() == gli::TARGET_3D ? extent.z : layerGL,
                                                      format.Internal, static_cast<GLsizei>(texture.size(level)),
                                                      texture.data(layer, face, level));
                        } else {
                            glTexSubImage3D(target, static_cast<GLint>(level), 0, 0, 0,
                                            extent.x, extent.y,
                                            texture.target() == gli::TARGET_3D ? extent.z : layerGL,
                                            format.External, format.Type, texture.data(layer, face, level));
                        }
                        break;
                    default: 
                        assert(0);
                        break;
                }
            }
        }
    }
    return {name, static_cast<std::uint64_t>(extent.x), static_cast<std::uint64_t>(extent.y), static_cast<std::uint64_t>(texture.levels()), static_cast<std::uint64_t>(texture.layers())};
}

Image OpenGLBackend::loadImage(const std::string& path) {
    int x, y, n;
    unsigned char *data = stbi_load(path.c_str(), &x, &y, &n, STBI_rgb_alpha);
    
    if (data == nullptr) {
        throw std::runtime_error("Nepavyko pakrauti: " + path);
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (n == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else if (n == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    stbi_image_free(data);

    return {texture, static_cast<std::uint64_t>(x), static_cast<std::uint64_t>(y), 1, 1};
}

}

#endif /* BUILD_WITH_OPENGL */
