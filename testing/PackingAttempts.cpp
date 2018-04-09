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


//------------------------WRITE HALF START
            fw.writeUInt16(glm::packHalf1x16(t[j].x));
            fw.writeUInt16(glm::packHalf1x16(t[j].y));
            fw.writeUInt16(glm::packHalf1x16(t[j].z));
            
            fw.writeUInt16(glm::packHalf1x16(b[j].x));
            fw.writeUInt16(glm::packHalf1x16(b[j].y));
            fw.writeUInt16(glm::packHalf1x16(b[j].z));
            
            fw.writeUInt16(glm::packHalf1x16(n[j].x));
            fw.writeUInt16(glm::packHalf1x16(n[j].y));
            fw.writeUInt16(glm::packHalf1x16(n[j].z));
//------------------------WRITE HALF END

//------------------------HALF TEST START
            float nx = glm::unpackHalf1x16(glm::packHalf1x16(n[j].x));
            float ny = glm::unpackHalf1x16(glm::packHalf1x16(n[j].y));
            float nz = glm::unpackHalf1x16(glm::packHalf1x16(n[j].z));
            
            float tx = glm::unpackHalf1x16(glm::packHalf1x16(t[j].x));
            float ty = glm::unpackHalf1x16(glm::packHalf1x16(t[j].y));
            float tz = glm::unpackHalf1x16(glm::packHalf1x16(t[j].z));
            
            float bx = glm::unpackHalf1x16(glm::packHalf1x16(b[j].x));
            float by = glm::unpackHalf1x16(glm::packHalf1x16(b[j].y));
            float bz = glm::unpackHalf1x16(glm::packHalf1x16(b[j].z));
            
            glm::mat3 tbn(glm::normalize(glm::vec3(tx, ty, tz)),
                          glm::normalize(glm::vec3(bx, by, bz)),
                          glm::normalize(glm::vec3(nx, ny, nz)));

            float determinant = std::round(glm::determinant(tbn));
            
            glm::vec3 no(nx, ny, nz);
            no = glm::normalize(no);
            glm::vec3 ta(tx, ty, tz);
            ta = glm::normalize(ta);
            
            glm::vec3 bi = glm::cross(no, ta);
            bi *= determinant;
            bi = glm::normalize(bi);
            
            LOG_D("NP " << nx << ";" << ny << ";" << nz << "\n\t" <<
                  "NR " << n[j].x << ";" << n[j].y << ";" << n[j].z << "\n\t" <<
                  "TP " << tx << ";" << ty << ";" << tz << "\n\t" <<
                  "TR " << t[j].x << ";" << t[j].y << ";" << t[j].z << "\n\t" <<
                  "BP " << bx << ";" << by << ";" << bz << "\n\t" <<
                  "BR " << b[j].x << ";" << b[j].y << ";" << b[j].z << "\n\n\t" <<
                  "DT " << determinant << ";" << glm::determinant(tbn) << "\n\t" <<
                  "BU " << bi.x << ";" << bi.y << ";" << bi.z)
            
            glm::vec4 tBias(t[j].x, t[j].y, t[j].z, determinant);
            fw.writeInt64(glm::packHalf4x16(tBias));
//------------------------HALF TEST END

//------------------------PACKED TEST START
            std::uint32_t tp = glm::packSnorm3x10_1x2(glm::vec4(t[j].x, t[j].y, t[j].z, 0.0f));
            std::uint32_t bp = glm::packSnorm3x10_1x2(glm::vec4(b[j].x, b[j].y, b[j].z, 0.0f));
            std::uint32_t np = glm::packSnorm3x10_1x2(glm::vec4(n[j].x, n[j].y, n[j].z, 0.0f));
            
            glm::vec4 tu = glm::unpackSnorm3x10_1x2(tp);
            glm::vec4 bu = glm::unpackSnorm3x10_1x2(bp);
            glm::vec4 nu = glm::unpackSnorm3x10_1x2(np);
            LOG_D("NP " << nu.x << ";" << nu.y << ";" << nu.z << ";" << nu.w << "\n\t" <<
                  "NR " << n[j].x << ";" << n[j].y << ";" << n[j].z << "\n\t" <<
                  "TP " << tu.x << ";" << tu.y << ";" << tu.z << ";" << tu.w <<  "\n\t" <<
                  "TR " << t[j].x << ";" << t[j].y << ";" << t[j].z << "\n\t" <<
                  "BP " << bu.x << ";" << bu.y << ";" << bu.z << ";" << bu.w <<  "\n\t" <<
                  "BR " << b[j].x << ";" << b[j].y << ";" << b[j].z)
//------------------------PACKED TEST END
