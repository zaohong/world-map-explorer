#pragma once
#include "local_state_store.h"
#include "explorer_navigation.h"
#include <cstring>
namespace sessionState {
struct State {
    explorerNavigation::View view;
    bool selected=false;
    int basemap=0,year=2024,workspace=0,infoTab=0,regionPage=0;
    uint32_t layers=0;
    std::wstring routeId;
    double routeFraction=0;
};
inline bool label(const std::wstring& value){return value.size()<=512&&value.find(L'\0')==std::wstring::npos&&value.find_first_of(L"\r\n")==std::wstring::npos;}
inline bool valid(const State& s){auto& v=s.view;return std::isfinite(v.zoom)&&v.zoom>=1&&v.zoom<=128&&std::isfinite(v.cx)&&v.cx>=0&&v.cx<=1&&std::isfinite(v.cy)&&v.cy>=0&&v.cy<=1&&std::isfinite(v.latitude)&&std::abs(v.latitude)<=90&&std::isfinite(v.longitude)&&std::abs(v.longitude)<=180&&v.grid>=0&&v.grid<=3&&(!v.input||v.exact)&&(!v.fine||v.exact)&&!(v.input&&v.fine)&&label(v.name)&&label(v.origin)&&s.basemap>=0&&s.basemap<=2&&s.year>=-3400&&s.year<=2024&&s.year!=0&&s.workspace>=0&&s.workspace<=2&&s.infoTab>=0&&s.infoTab<=1&&s.regionPage>=0&&s.regionPage<=3&&s.layers<=15&&(s.layers&12)!=12&&s.routeId.size()<=80&&label(s.routeId)&&std::isfinite(s.routeFraction)&&s.routeFraction>=0&&s.routeFraction<=1;}
inline std::vector<unsigned char> encode(const State& s){
    if(!valid(s))return {};std::vector<unsigned char> out{'W','M','S','V','I','E','W','1'};
    auto u32=[&](uint32_t x){for(int i=0;i<4;++i)out.push_back((unsigned char)(x>>(i*8)));};
    auto number=[&](double x){uint64_t bits;std::memcpy(&bits,&x,8);for(int i=0;i<8;++i)out.push_back((unsigned char)(bits>>(i*8)));};
    auto text=[&](const std::wstring& value){u32((uint32_t)value.size());for(wchar_t c:value){out.push_back((unsigned char)c);out.push_back((unsigned char)(c>>8));}};
    u32(s.selected?1:0);u32(s.basemap);u32((uint32_t)s.year);u32(s.workspace);u32(s.infoTab);u32(s.regionPage);u32(s.layers);u32(s.view.grid);u32((s.view.exact?1:0)|(s.view.input?2:0)|(s.view.fine?4:0));
    number(s.view.zoom);number(s.view.cx);number(s.view.cy);number(s.view.latitude);number(s.view.longitude);number(s.routeFraction);text(s.view.name);text(s.view.origin);text(s.routeId);u32(localState::crc32(out.data(),out.size()));return out;
}
inline bool decode(const std::vector<unsigned char>& bytes,State& state){
    if(bytes.size()<108||bytes.size()>8192||std::memcmp(bytes.data(),"WMSVIEW1",8))return false;size_t at=8,end=bytes.size()-4;
    auto u32=[&](uint32_t& x){if(at+4>end)return false;x=0;for(int i=0;i<4;++i)x|=(uint32_t)bytes[at++]<<(i*8);return true;};
    uint32_t checksum=0;for(int i=0;i<4;++i)checksum|=(uint32_t)bytes[end+i]<<(i*8);if(checksum!=localState::crc32(bytes.data(),end))return false;
    auto number=[&](double& x){if(at+8>end)return false;uint64_t bits=0;for(int i=0;i<8;++i)bits|=(uint64_t)bytes[at++]<<(i*8);std::memcpy(&x,&bits,8);return true;};
    auto text=[&](std::wstring& value){uint32_t n;if(!u32(n)||n>512||at+(size_t)n*2>end)return false;value.clear();for(uint32_t i=0;i<n;++i){value+=(wchar_t)(bytes[at]|((uint32_t)bytes[at+1]<<8));at+=2;}return true;};
    State s;uint32_t selected,basemap,year,workspace,tab,page,layers,grid,flags;
    if(!u32(selected)||selected>1||!u32(basemap)||!u32(year)||!u32(workspace)||!u32(tab)||!u32(page)||!u32(layers)||!u32(grid)||!u32(flags)||flags>7)return false;
    s.selected=selected!=0;s.basemap=(int32_t)basemap;s.year=(int32_t)year;s.workspace=(int32_t)workspace;s.infoTab=(int32_t)tab;s.regionPage=(int32_t)page;s.layers=layers;s.view.grid=(int32_t)grid;s.view.exact=(flags&1)!=0;s.view.input=(flags&2)!=0;s.view.fine=(flags&4)!=0;
    if(!number(s.view.zoom)||!number(s.view.cx)||!number(s.view.cy)||!number(s.view.latitude)||!number(s.view.longitude)||!number(s.routeFraction)||!text(s.view.name)||!text(s.view.origin)||!text(s.routeId)||at!=end||!valid(s))return false;state=std::move(s);return true;
}
inline bool read(const std::wstring& path,State& state){std::vector<unsigned char> bytes;return localState::read(path,bytes,8192)&&decode(bytes,state);}
inline bool write(const std::wstring& path,const State& state){auto bytes=encode(state);return !bytes.empty()&&localState::write(path,bytes);}
}
