#pragma once
#include "saved_places.h"
namespace savedPlaces {
enum class Operation { Add,Remove,Rename };
enum class Status { Changed,Unchanged,Busy,Unreadable,Full,Missing,Conflict,Invalid,WriteFailed };
struct Request {Operation operation;Place place;std::wstring expectedName;};
struct Result {Status status=Status::Invalid;std::vector<Place> places;bool snapshot=false,hadPrevious=false;Place previous;};
inline bool sameCollection(const std::vector<Place>& a,const std::vector<Place>& b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].name!=b[i].name||a[i].latitude!=b[i].latitude||a[i].longitude!=b[i].longitude||a[i].zoom!=b[i].zoom)return false;return true;}
inline bool load(const std::wstring& path,std::vector<Place>& places){DWORD attributes=GetFileAttributesW(path.c_str());if(attributes==INVALID_FILE_ATTRIBUTES){DWORD error=GetLastError();if(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND){places.clear();return true;}return false;}return read(path,places);}
class FileLock {
    HANDLE file_=INVALID_HANDLE_VALUE;DWORD error_=ERROR_SUCCESS;
public:
    explicit FileLock(const std::wstring& path,DWORD wait=250){ULONGLONG start=GetTickCount64();do{file_=CreateFileW((path+L".lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN,nullptr);if(file_!=INVALID_HANDLE_VALUE)break;error_=GetLastError();if(error_!=ERROR_SHARING_VIOLATION&&error_!=ERROR_LOCK_VIOLATION)break;if(GetTickCount64()-start>=wait)break;Sleep(10);}while(true);}
    explicit operator bool()const{return file_!=INVALID_HANDLE_VALUE;}
    DWORD error()const{return error_;}
    FileLock(const FileLock&)=delete;FileLock& operator=(const FileLock&)=delete;
    ~FileLock(){if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);}
};
inline Result update(const std::wstring& path,const Request& request){
    Result result;if(!valid(request.place))return result;FileLock lock(path);if(!lock){result.status=lock.error()==ERROR_SHARING_VIOLATION||lock.error()==ERROR_LOCK_VIOLATION?Status::Busy:Status::WriteFailed;return result;}
    if(!load(path,result.places)){result.status=Status::Unreadable;return result;}result.snapshot=true;int index=find(result.places,request.place.latitude,request.place.longitude);
    if(index>=0){result.previous=result.places[index];result.hadPrevious=true;}
    if(request.operation==Operation::Add){if(index>=0){result.status=Status::Unchanged;return result;}if(result.places.size()>=64){result.status=Status::Full;return result;}result.places.push_back(request.place);}
    else if(request.operation==Operation::Remove){if(index<0){result.status=Status::Unchanged;return result;}result.places.erase(result.places.begin()+index);}
    else {if(index<0){result.status=Status::Missing;return result;}if(!request.expectedName.empty()&&result.places[index].name!=request.expectedName){result.status=Status::Conflict;return result;}if(result.places[index].name==request.place.name){result.status=Status::Unchanged;return result;}result.places[index].name=request.place.name;}
    if(!write(path,result.places)){result.status=Status::WriteFailed;result.snapshot=load(path,result.places);return result;}result.status=Status::Changed;return result;
}
struct Undo {Operation operation;Place place;std::wstring expectedName;};
}
