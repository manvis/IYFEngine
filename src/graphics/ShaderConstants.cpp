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

#include "graphics/ShaderConstants.hpp"

namespace iyf {
namespace con {

const std::array<SpecializationConstant, 5> DefaultSpecializationConstants = {
    SpecializationConstant(MaxDirectionalLightsConstName, Format::R32_uInt, MaxDirectionalLights),
    SpecializationConstant(MaxPointLightsConstName,       Format::R32_uInt, MaxPointLights),
    SpecializationConstant(MaxSpotLightsConstName,        Format::R32_uInt, MaxSpotLights),
    SpecializationConstant(MaxMaterialsConstName,         Format::R32_uInt, MaxMaterials),
    SpecializationConstant(MaxTransformationsConstName,   Format::R32_uInt, MaxTransformations),
};

const std::unordered_map<Format, std::pair<ShaderDataFormat, ShaderDataType>> FormatToShaderDataType = {
    {Format::R8_uNorm,                      {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R8_sNorm,                      {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R8_uScaled,                    {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R8_sScaled,                    {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R8_uInt,                       {ShaderDataFormat::UnsignedInteger, ShaderDataType::Scalar}},
    {Format::R8_sInt,                       {ShaderDataFormat::Integer,         ShaderDataType::Scalar}},
    {Format::R8_G8_uNorm,                   {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R8_G8_sNorm,                   {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R8_G8_uScaled,                 {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R8_G8_sScaled,                 {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R8_G8_uInt,                    {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector2D}},
    {Format::R8_G8_sInt,                    {ShaderDataFormat::Integer,         ShaderDataType::Vector2D}},
    {Format::R8_G8_B8_uNorm,                {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R8_G8_B8_sNorm,                {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R8_G8_B8_uScaled,              {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R8_G8_B8_sScaled,              {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R8_G8_B8_uInt,                 {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector3D}},
    {Format::R8_G8_B8_sInt,                 {ShaderDataFormat::Integer,         ShaderDataType::Vector3D}},
    {Format::B8_G8_R8_uNorm,                {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::B8_G8_R8_sNorm,                {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::B8_G8_R8_uScaled,              {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::B8_G8_R8_sScaled,              {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::B8_G8_R8_uInt,                 {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector3D}},
    {Format::B8_G8_R8_sInt,                 {ShaderDataFormat::Integer,         ShaderDataType::Vector3D}},
    {Format::R8_G8_B8_A8_uNorm,             {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R8_G8_B8_A8_sNorm,             {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R8_G8_B8_A8_uScaled,           {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R8_G8_B8_A8_sScaled,           {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R8_G8_B8_A8_uInt,              {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector4D}},
    {Format::R8_G8_B8_A8_sInt,              {ShaderDataFormat::Integer,         ShaderDataType::Vector4D}},
    {Format::B8_G8_R8_A8_uNorm,             {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::B8_G8_R8_A8_sNorm,             {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::B8_G8_R8_A8_uScaled,           {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::B8_G8_R8_A8_sScaled,           {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::B8_G8_R8_A8_uInt,              {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector4D}},
    {Format::B8_G8_R8_A8_sInt,              {ShaderDataFormat::Integer,         ShaderDataType::Vector4D}},
    {Format::A8_B8_G8_R8_uNorm_pack32,      {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A8_B8_G8_R8_sNorm_pack32,      {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A8_B8_G8_R8_uScaled_pack32,    {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A8_B8_G8_R8_sScaled_pack32,    {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A8_B8_G8_R8_uInt_pack32,       {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector4D}},
    {Format::A8_B8_G8_R8_sInt_pack32,       {ShaderDataFormat::Integer,         ShaderDataType::Vector4D}},
    {Format::A2_R10_G10_B10_uNorm_pack32,   {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_R10_G10_B10_sNorm_pack32,   {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_R10_G10_B10_uScaled_pack32, {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_R10_G10_B10_sScaled_pack32, {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_R10_G10_B10_uInt_pack32,    {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector4D}},
    {Format::A2_R10_G10_B10_sInt_pack32,    {ShaderDataFormat::Integer,         ShaderDataType::Vector4D}},
    {Format::A2_B10_G10_R10_uNorm_pack32,   {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_B10_G10_R10_sNorm_pack32,   {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_B10_G10_R10_uScaled_pack32, {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_B10_G10_R10_sScaled_pack32, {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::A2_B10_G10_R10_uInt_pack32,    {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector4D}},
    {Format::A2_B10_G10_R10_sInt_pack32,    {ShaderDataFormat::Integer,         ShaderDataType::Vector4D}},
    {Format::R16_uNorm,                     {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R16_sNorm,                     {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R16_uScaled,                   {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R16_sScaled,                   {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R16_uInt,                      {ShaderDataFormat::UnsignedInteger, ShaderDataType::Scalar}},
    {Format::R16_sInt,                      {ShaderDataFormat::Integer,         ShaderDataType::Scalar}},
    {Format::R16_sFloat,                    {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R16_G16_uNorm,                 {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R16_G16_sNorm,                 {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R16_G16_uScaled,               {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R16_G16_sScaled,               {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R16_G16_uInt,                  {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector2D}},
    {Format::R16_G16_sInt,                  {ShaderDataFormat::Integer,         ShaderDataType::Vector2D}},
    {Format::R16_G16_sFloat,                {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R16_G16_B16_uNorm,             {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R16_G16_B16_sNorm,             {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R16_G16_B16_uScaled,           {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R16_G16_B16_sScaled,           {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R16_G16_B16_uInt,              {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector3D}},
    {Format::R16_G16_B16_sInt,              {ShaderDataFormat::Integer,         ShaderDataType::Vector3D}},
    {Format::R16_G16_B16_sFloat,            {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R16_G16_B16_A16_uNorm,         {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R16_G16_B16_A16_sNorm,         {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R16_G16_B16_A16_uScaled,       {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R16_G16_B16_A16_sScaled,       {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R16_G16_B16_A16_uInt,          {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector4D}},
    {Format::R16_G16_B16_A16_sInt,          {ShaderDataFormat::Integer,         ShaderDataType::Vector4D}},
    {Format::R16_G16_B16_A16_sFloat,        {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
    {Format::R32_uInt,                      {ShaderDataFormat::UnsignedInteger, ShaderDataType::Scalar}},
    {Format::R32_sInt,                      {ShaderDataFormat::Integer,         ShaderDataType::Scalar}},
    {Format::R32_sFloat,                    {ShaderDataFormat::Float,           ShaderDataType::Scalar}},
    {Format::R32_G32_uInt,                  {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector2D}},
    {Format::R32_G32_sInt,                  {ShaderDataFormat::Integer,         ShaderDataType::Vector2D}},
    {Format::R32_G32_sFloat,                {ShaderDataFormat::Float,           ShaderDataType::Vector2D}},
    {Format::R32_G32_B32_uInt,              {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector3D}},
    {Format::R32_G32_B32_sInt,              {ShaderDataFormat::Integer,         ShaderDataType::Vector3D}},
    {Format::R32_G32_B32_sFloat,            {ShaderDataFormat::Float,           ShaderDataType::Vector3D}},
    {Format::R32_G32_B32_A32_uInt,          {ShaderDataFormat::UnsignedInteger, ShaderDataType::Vector4D}},
    {Format::R32_G32_B32_A32_sInt,          {ShaderDataFormat::Integer,         ShaderDataType::Vector4D}},
    {Format::R32_G32_B32_A32_sFloat,        {ShaderDataFormat::Float,           ShaderDataType::Vector4D}},
//     {Format::R64_uInt, // What to use here ???
//     {Format::R64_sInt, // What to use here ???
    {Format::R64_sFloat,                    {ShaderDataFormat::Double,          ShaderDataType::Scalar}},
//     {Format::R64_G64_uInt, // What to use here ???
//     {Format::R64_G64_sInt, // What to use here ???
    {Format::R64_G64_sFloat,                {ShaderDataFormat::Double,          ShaderDataType::Vector2D}},
//     {Format::R64_G64_B64_uInt, // What to use here ???
//     {Format::R64_G64_B64_sInt, // What to use here ???
    {Format::R64_G64_B64_sFloat,            {ShaderDataFormat::Double,          ShaderDataType::Vector3D}},
//     {Format::R64_G64_B64_A64_uInt, // What to use here ???
//     {Format::R64_G64_B64_A64_sInt, // What to use here ???
    {Format::R64_G64_B64_A64_sFloat,        {ShaderDataFormat::Double,          ShaderDataType::Vector4D}},
};

}
}
