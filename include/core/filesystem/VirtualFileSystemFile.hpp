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

#ifndef IYF_VIRTUAL_FILE_SYSTEM_FILE_HPP
#define IYF_VIRTUAL_FILE_SYSTEM_FILE_HPP

#include <cstdint>
#include <fstream>

#include <physfs.h>

#include "utilities/NonCopyable.hpp"
#include "io/DefaultFileSystem.hpp"
#include "io/File.hpp"

namespace iyf {

/// Encapsulates PhysFS file access that is limited to the virtual file system. If you wish to access files
/// from anywhere, you'll need to rely on C++'s file system functions, however, that is strongly discouraged.
///
/// \todo Start using size_t here and in serializers (for sizes)
class VirtualFileSystemFile : public File {
public:
    NON_COPYABLE(VirtualFileSystemFile)

    virtual ~VirtualFileSystemFile();
    virtual bool close() final override;    
    virtual bool flush() final override;

    virtual std::int64_t seek(std::int64_t offset, SeekFrom whence) final override;
    virtual std::int64_t tell() final override;
    virtual std::int64_t readBytes(void* bytes, std::uint64_t count) final override;

    virtual bool isEOF() final override;
    virtual std::int64_t writeBytes(const void* bytes, std::uint64_t count) final override;
protected:
    friend class VirtualFileSystem;
    VirtualFileSystemFile(const Path& path, FileOpenMode openMode);

    PHYSFS_File* file;
    bool isOpen;
//     /// Opens a file that resides in the virtual file system. The path must be provided in platform independent format
//     ///
//     /// \warning reading from a file that was opened for writing and vice versa will invoke undefined behaviour.
//     File(const Path& path, OpenMode openMode);
    
//     ~File();
    
//     File(File&& other) : file(other.file), path(std::move(other.path)), openMode(other.openMode), isOpen(other.isOpen) {
//         other.file = nullptr;
//         other.isOpen = false;
//     }
    
//     File& operator=(File&& other) {
//         if (this != &other) {
//             close();
            
//             file = other.file;
//             path = std::move(other.path);
//             isOpen = other.isOpen;
//             openMode = other.openMode;
            
//             other.file = nullptr;
//             other.isOpen = false;
//         }
        
//         return *this;
//     }
    
//     /// Closes the file. Normallly, the destructor takes care of that. Closing the file and performing any other operations on it
//     /// invokes undefined behaviour
//     bool close();
    
//     /// \todo Quite a few engine functions read or write many small values and may benefit from buffered I/O.
//     /// Asses the performance impact and start using this function where appropriate.
//     inline bool setBuffer(std::uint64_t bufferSize) {
//         return PHYSFS_setBuffer(file, bufferSize);
//     }
    
//     inline bool flush() {
//         return PHYSFS_flush(file);
//     }

//     /// Seeks to a specified position in file.
//     /// \return new position if successful, -1 if an error has occured. You can retrieve the error from the file system object
//     std::int64_t seek(std::int64_t offset, SeekFrom whence);
    
//     /// \return the current position in the file or -1 if an error occured
//     std::int64_t tell() const {
//         return PHYSFS_tell(file);
//     }

//     inline const Path& getPath() const {
//         return path;
//     }

//     inline OpenMode getOpenMode() const {
//         return openMode;
//     }
    
//     // File read methods --------------------------------------------------------------------------------------
//     /// Reads a string from a file and into the provided std::string.
//     ///
//     /// \param[out] string The string that data will be appended to.
//     /// \param[in] indicator The length indicator that the string uses.
//     /// \param[in] count Number of bytes to read. Ignored if length is not StringLengthIndicator::None
//     std::int64_t readString(std::string& string, StringLengthIndicator indicator = StringLengthIndicator::None, std::uint64_t count = 0);
    
//     inline std::int64_t readBytes(void* bytes, std::uint64_t count) {
//         return PHYSFS_readBytes(file, bytes, count);
//     }

//     inline bool isEOF() {
//         return PHYSFS_eof(file);
//     }

//     inline bool readInt8(std::int8_t* val) {
//         return readBytes(val, sizeof(std::int8_t)) == 1;
//     }

//     inline std::int8_t readInt8() {
//         std::int8_t temp;
//         if (readInt8(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readUInt8(std::uint8_t* val) {
//         return readBytes(val, sizeof(std::uint8_t)) == 1;
//     }

//     inline std::uint8_t readUInt8() {
//         std::uint8_t temp;
//         if (readUInt8(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readInt16(std::int16_t* val) {
//         return PHYSFS_readSLE16(file, val);
//     }

//     inline std::int16_t readInt16() {
//         std::int16_t temp;
//         if (readInt16(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readUInt16(std::uint16_t* val) {
//         return PHYSFS_readULE16(file, val);
//     }

//     inline std::uint16_t readUInt16() {
//         std::uint16_t temp;
//         if (readUInt16(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readInt32(std::int32_t* val) {
//         return PHYSFS_readSLE32(file, val);
//     }

//     inline std::int32_t readInt32() {
//         std::int32_t temp;
//         if (readInt32(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readUInt32(std::uint32_t* val) {
//         return PHYSFS_readULE32(file, val);
//     }

//     inline std::uint32_t readUInt32() {
//         std::uint32_t temp;
//         if (readUInt32(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readInt64(std::int64_t* val) {
//         // Reinterpret cast on the pointer because PHYSFS_sint64 is
//         // a long long int and std::int64_t is long int on 64-bit 
//         // CLANG and GCC
//         return PHYSFS_readSLE64(file, reinterpret_cast<PHYSFS_sint64*>(val));
//     }

//     inline std::int64_t readInt64() {
//         std::int64_t temp;
//         if (readInt64(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readUInt64(std::uint64_t* val) {
//         // Reinterpret cast on the pointer because PHYSFS_uint64 is
//         // an unsigned long long int and std::uint64_t is an unsigned long 
//         // int on 64-bit CLANG and GCC
//         return PHYSFS_readULE64(file, reinterpret_cast<PHYSFS_uint64*>(val));
//     }

//     inline std::uint64_t readUInt64() {
//         std::uint64_t temp;
//         if (readUInt64(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }

//     inline bool readFloat(float* val) {
//         if (readBytes(val, sizeof(float)) != sizeof(float)) {
//             return false;
//         }

//         *val = util::SwapLE(*val);
//         return true;
//     }

//     inline float readFloat() {
//         float temp;
//         if (readFloat(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }
//     inline bool readDouble(double* val) {
//         if (readBytes(val, sizeof(double)) != sizeof(double)) {
//             return false;
//         }

//         *val = util::SwapLE(*val);
//         return true;
//     }

//     inline double readDouble() {
//         double temp;
//         if (readDouble(&temp)) {
//             return temp;
//         } else {
//             throw FileException("Failed to read from file ", path.getGenericString());
//         }
//     }
    
//     /// Reads the entire file into a memory buffer and returns a buffer and size pair. The actual size of the buffer is 
//     /// size + 1 and it is null terminated. However, do not rely on that when reading binary files because they may contain
//     /// nulls earlier.
//     std::pair<std::unique_ptr<char[]>, std::int64_t> readWholeFile();
    
//     // File write methods -------------------------------------------------------------------------
    
//     /// Writes a string with an optional length indicator prefix.
//     ///
//     /// \remark This method never writes the null terminators to the file.
//     ///
//     /// \return Number of bytes that were written. Should be equal to StringLengthIndicator length + the length of the string.
//     std::int64_t writeString(const std::string& string, StringLengthIndicator indicator = StringLengthIndicator::None);
    
//     /// Writes a string_view with an optional length indicator prefix.
//     ///
//     /// \remark This method never writes the null terminators to the file.
//     ///
//     /// \return Number of bytes that were written. Should be equal to StringLengthIndicator length + the length of the string.
//     std::int64_t writeString(const std::string_view& stringView, StringLengthIndicator indicator = StringLengthIndicator::None);
    
//     /// Writes a char array with an optional length indicator prefix.
//     ///
//     /// \remark This method never writes the null terminators to the file and they should not be included in length
//     ///
//     /// \return Number of bytes that were written. Should be equal to StringLengthIndicator length + the length of the string.
//     std::int64_t writeString(const char* string, std::size_t length, StringLengthIndicator indicator = StringLengthIndicator::None);
    
//     inline std::int64_t writeBytes(const void* bytes, std::uint64_t count) {
//         return PHYSFS_writeBytes(file, bytes, count);
//     }

//     inline bool writeInt8(std::int8_t val) {
//         return writeBytes(&val, sizeof(std::int8_t)) == 1;
//     }

//     inline bool writeUInt8(std::uint8_t val) {
//         return writeBytes(&val, sizeof(std::uint8_t)) == 1;
//     }

//     inline bool writeInt16(std::int16_t val) {
//         return PHYSFS_writeSLE16(file, val);
//     }

//     inline bool writeUInt16(std::uint16_t val) {
//         return PHYSFS_writeULE16(file, val);
//     }

//     inline bool writeInt32(std::int32_t val) {
//         return PHYSFS_writeSLE32(file, val);
//     }

//     inline bool writeUInt32(std::uint32_t val) {
//         return PHYSFS_writeULE32(file, val);
//     }

//     inline bool writeInt64(std::int64_t val) {
//         return PHYSFS_writeSLE64(file, val);
//     }

//     inline bool writeUInt64(std::uint64_t val) {
//         return PHYSFS_writeULE64(file, val);
//     }

//     inline bool writeFloat(float val) {
//         // Files store floats in little endian. Swapping using SDL since PhysFS lacks float write method
//         val = util::SwapLE(val);
//         if (writeBytes(&val, sizeof(float)) != sizeof(float)) {
//             return false;
//         }

//         return true;
//     }

//     inline bool writeDouble(double val) {
//         // Files store doubles in little endian. Swapping using our own thing since SDL and PhysFS lack a double write method
//         val = util::SwapLE(val);
//         if (writeBytes(&val, sizeof(double)) != sizeof(double)) {
//             return false;
//         }

//         return true;
//     }
// protected:
//     PHYSFS_File* file;
//     Path path;
//     OpenMode openMode;
//     bool isOpen;
};

}

#endif // IYF_VIRTUAL_FILE_SYSTEM_FILE_HPP
