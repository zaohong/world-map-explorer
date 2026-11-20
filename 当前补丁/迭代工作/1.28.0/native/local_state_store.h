#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
namespace localState {
inline bool read(const std::wstring& path,std::vector<unsigned char>& bytes,size_t maximum=65536){HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER size{};bool ok=GetFileSizeEx(file,&size)&&size.QuadPart>=0&&(uint64_t)size.QuadPart<=maximum;std::vector<unsigned char> candidate;if(ok){candidate.resize((size_t)size.QuadPart);DWORD got=0;ok=ReadFile(file,candidate.data(),(DWORD)candidate.size(),&got,nullptr)&&got==candidate.size();}CloseHandle(file);if(ok)bytes.swap(candidate);return ok;}
inline bool write(const std::wstring& path,const std::vector<unsigned char>& bytes){if(bytes.size()>65536)return false;HANDLE file=INVALID_HANDLE_VALUE;std::wstring temporary;for(int attempt=0;attempt<16;++attempt){temporary=path+L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L"."+std::to_wstring(attempt)+L".tmp";file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file!=INVALID_HANDLE_VALUE)break;if(GetLastError()!=ERROR_FILE_EXISTS&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;}if(file==INVALID_HANDLE_VALUE)return false;DWORD got=0;bool ok=WriteFile(file,bytes.data(),(DWORD)bytes.size(),&got,nullptr)&&got==bytes.size()&&FlushFileBuffers(file);CloseHandle(file);if(ok)ok=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;if(!ok)DeleteFileW(temporary.c_str());return ok;}
inline uint32_t crc32(const unsigned char* bytes,size_t size){uint32_t crc=0xffffffff;for(size_t i=0;i<size;++i){crc^=bytes[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&-(int32_t)(crc&1));}return ~crc;}
}
