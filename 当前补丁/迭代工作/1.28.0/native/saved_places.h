#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace savedPlaces {
struct Place {std::wstring name;double latitude=0,longitude=0,zoom=1;};
inline bool valid(const Place& p){return !p.name.empty()&&p.name.size()<=120&&p.name.find(L'\0')==std::wstring::npos&&p.name.find_first_of(L"\r\n")==std::wstring::npos&&std::isfinite(p.latitude)&&std::abs(p.latitude)<=90&&std::isfinite(p.longitude)&&std::abs(p.longitude)<=180&&std::isfinite(p.zoom)&&p.zoom>=1&&p.zoom<=128;}
inline int find(const std::vector<Place>& places,double latitude,double longitude){for(size_t i=0;i<places.size();++i)if(std::abs(places[i].latitude-latitude)<1e-5&&std::abs(places[i].longitude-longitude)<1e-5)return (int)i;return -1;}
inline bool write(const std::wstring& path,const std::vector<Place>& places){
    if(places.size()>64)return false;std::vector<unsigned char> bytes{'W','M','P','L','A','C','E','1'};
    auto append=[&](const void* p,size_t n){auto first=(const unsigned char*)p;bytes.insert(bytes.end(),first,first+n);};uint32_t count=(uint32_t)places.size();append(&count,4);
    for(size_t i=0;i<places.size();++i){const auto& p=places[i];if(!valid(p))return false;for(size_t j=0;j<i;++j)if(std::abs(p.latitude-places[j].latitude)<1e-5&&std::abs(p.longitude-places[j].longitude)<1e-5)return false;uint32_t size=(uint32_t)p.name.size();append(&size,4);append(p.name.data(),size*2);append(&p.latitude,8);append(&p.longitude,8);append(&p.zoom,8);}
    std::wstring temporary=path+L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L".tmp";HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;DWORD written=0;bool ok=WriteFile(file,bytes.data(),(DWORD)bytes.size(),&written,nullptr)&&written==bytes.size()&&FlushFileBuffers(file);CloseHandle(file);if(ok)ok=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;if(!ok)DeleteFileW(temporary.c_str());return ok;
}
inline bool read(const std::wstring& path,std::vector<Place>& places){
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;DWORD size=GetFileSize(file,nullptr),got=0;if(size<12||size>65536){CloseHandle(file);return false;}std::vector<unsigned char> bytes(size);bool ok=ReadFile(file,bytes.data(),size,&got,nullptr)&&got==size;CloseHandle(file);if(!ok||std::memcmp(bytes.data(),"WMPLACE1",8))return false;
    size_t at=8;auto take=[&](void* p,size_t n){if(n>bytes.size()-at)return false;std::memcpy(p,bytes.data()+at,n);at+=n;return true;};uint32_t count;if(!take(&count,4)||count>64)return false;std::vector<Place> candidate;
    for(uint32_t i=0;i<count;++i){uint32_t length;if(!take(&length,4)||!length||length>120)return false;Place p;p.name.resize(length);if(!take(&p.name[0],length*2)||!take(&p.latitude,8)||!take(&p.longitude,8)||!take(&p.zoom,8)||!valid(p)||find(candidate,p.latitude,p.longitude)>=0)return false;candidate.push_back(std::move(p));}
    if(at!=bytes.size())return false;places.swap(candidate);return true;
}
}
