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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <cctype>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, std::string> nameMap = {
    {"UNDEFINED", "Undefined"},
    {"UNORM", "uNorm"},
    {"SNORM", "sNorm"},
    {"USCALED", "uScaled"},
    {"SSCALED", "sScaled"},
    {"UINT", "uInt"},
    {"SINT", "sInt"},
    {"SRGB", "sRGB"},
    {"RGB", "RGB"},
    {"RGBA", "RGBA"},
    {"SFLOAT", "sFloat"},
    {"UFLOAT", "uFloat"},
    {"EAC", "EAC"},
    {"ETC2", "ETC2"},
    {"2BPP", "2bpp"},
    {"4BPP", "4bpp"},
    {"BLOCK", "block"},
    {"ASTC", "ASTC"},
    {"PVRTC1", "PVRTC1"},
    {"PVRTC2", "PVRTC2"},
    {"IMG", ""},
    {"BC1", "BC1"},
    {"BC2", "BC2"},
    {"BC3", "BC3"},
    {"BC4", "BC4"},
    {"BC5", "BC5"},
    {"BC6H", "BC6H"},
    {"BC7", "BC7"},
    {"PACK8", "pack8"},
    {"PACK16", "pack16"},
    {"PACK32", "pack32"},
};

std::string handleSegment(const std::string& segment);

int main() {
    std::vector<std::string> inEngineFormatNames;
    inEngineFormatNames.reserve(300);
    
    std::vector<std::string> vulkanFormatNamesWithCommas;
    vulkanFormatNamesWithCommas.reserve(300);
    std::size_t longestVulkanFormatLength = 0;
    
    std::string line;
    std::ifstream formatList("formats");
    
    if (!formatList.is_open()) {
        throw std::runtime_error("No format list");
    }
    
    while (std::getline(formatList, line)) {
        std::stringstream ss;
        
        std::string formatOnly = line.substr(0, 10);
        
        if (formatOnly != "VK_FORMAT_") {
            throw std::runtime_error("Unexpected start of line. Needs to be VK_FORMAT_, found: " + formatOnly);
        }
        
        // Remove the unnecessary end of the line
        std::size_t spacePos = line.find_first_of(" ");
        
        if (spacePos == std::string::npos) {
            throw std::runtime_error("Unexpected end of the line. Should end with \" = SomeInt\", actually is: " + line);
        }
        
        formatOnly = line.substr(0, spacePos);
        
        vulkanFormatNamesWithCommas.push_back(formatOnly + ",");
        longestVulkanFormatLength = std::max(formatOnly.length(), longestVulkanFormatLength);
        
        // Remove the repeating unnecessary start of the line
        formatOnly = formatOnly.substr(10);
        
        std::size_t segmentStart = 0;
        std::size_t segmentEnd = formatOnly.find_first_of("_");
        std::string segment = formatOnly.substr(segmentStart, segmentEnd);
        
        // Parse each segment
        size_t segmentCount = 0;
        while (true) {
            std::string result = handleSegment(segment);
            
            if (!result.empty()) {
                if (segmentCount > 0) {
                    ss << "_";
                }
                ss << result;
            }
            
            segmentCount++;
                        
            if (segmentEnd == std::string::npos) {
                break;
            }
            
            segmentStart = segmentEnd + 1;
            segmentEnd = formatOnly.find_first_of("_", segmentStart);
            
            segment = formatOnly.substr(segmentStart, segmentEnd - segmentStart);
        }
        
        inEngineFormatNames.push_back(ss.str());
//        std::cout << ss.str() << "; " << formatOnly << "\n";
    }
    
    // Output what we need:
    // 1. The engine formats themselves
    for (const auto& f : inEngineFormatNames) {
        std::cout << "    " << f << ",\n";
    }
    std::cout << "    COUNT,\n";
    
    std::cout << "----------------------\n";
    
    // 2. Engine format to Vulkan format
    std::cout << std::left << std::setfill(' ');
    std::cout << "ConstantMapper<Format, VkFormat, static_cast<size_t>(Format::COUNT)> format = {\n";
    for (std::size_t i = 0; i < vulkanFormatNamesWithCommas.size(); ++i) {
        std::cout << "    " << std::setw(longestVulkanFormatLength + 1) << vulkanFormatNamesWithCommas[i] << " //Format::" << inEngineFormatNames[i] << "\n";
    }
    std::cout << "};\n";
    
    std::cout << "----------------------\n";
    
    // 3. Engine format to human readable string
    std::cout << "ConstantMapper<Format, std::string, static_cast<size_t>(Format::COUNT)> formatName = {\n";
    for (const auto& f : inEngineFormatNames) {
        std::cout << "    \"" << f << "\",\n";
    }
    std::cout << "};\n";
    
    return 0;
}

std::string handleSegment(const std::string& segment) {
    // Component type, followed by one or more digits
    if ((segment[0] == 'R' || segment[0] == 'G' || segment[0] == 'B' || segment[0] == 'A' || 
         segment[0] == 'D' || segment[0] == 'S' ||
         segment[0] == 'X' ||
         segment[0] == 'E') 
        &&
         (std::isdigit(segment[1]))) {   
        std::stringstream ss;
        
        for (size_t i = 0; i < segment.length(); ++i) {
            if (!std::isdigit(segment[i]) && i > 0) {
                ss << "_";
            }

            ss << segment[i];
        }
        
        return ss.str();
    } else if (std::isdigit(segment[0])) {
        return segment;
    } else if (nameMap.find(segment) != nameMap.end()) {
        return nameMap[segment];
    } else {
        throw std::runtime_error("unhandled segment: " + segment);
    }
}
