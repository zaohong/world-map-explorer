#pragma once
#include "map_viewport.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
using std::min;
using std::max;
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <cstdint>
#include <climits>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "bounded_cache.h"

namespace historyVectors {
struct Entity {
    std::wstring en,zh,wikipedia,wikidata,type,sourceName,noteZh,noteEn;
    unsigned rgb=0;
};
struct Record { uint32_t entity,geometry; int from,to; double area; bool relation; };
struct Geometry { double minX,minY,maxX,maxY; uint32_t firstPolygon,polygonCount; };
struct Polygon { uint32_t firstRing,ringCount; double minX,minY,maxX,maxY; };
struct Ring { uint32_t firstPoint,pointCount; };
struct XY { double x,y; };

class Archive {
    HANDLE file_=INVALID_HANDLE_VALUE,mapping_=nullptr;
    const BYTE* bytes_=nullptr; size_t size_=0,pointOffset_=0;
    uint32_t pointCount_=0;
    int earliest_=0,latest_=0,year_=INT_MIN;
    std::vector<Entity> entities_;
    std::vector<Record> records_;
    std::vector<Geometry> geometries_;
    std::vector<Polygon> polygons_;
    std::vector<Ring> rings_;
    std::vector<size_t> active_,relations_;
    BoundedLruCache<uint32_t,std::unique_ptr<Gdiplus::GraphicsPath>> paths_{2048,32*1024*1024};
    uint32_t u32(size_t at)const{uint32_t value;std::memcpy(&value,bytes_+at,4);return value;}
    int32_t i32(size_t at)const{int32_t value;std::memcpy(&value,bytes_+at,4);return value;}
    double f64(size_t at)const{double value;std::memcpy(&value,bytes_+at,8);return value;}
    bool range(size_t at,uint64_t count,uint64_t stride)const{return at<=size_&&count<=(size_-at)/stride;}
    bool stringAt(size_t& at,size_t limit,std::wstring& value){
        if(at+4>limit)return false;uint32_t length=u32(at);at+=4;
        if(length>16384||length>limit-at)return false;
        if(!length){value.clear();return true;}
        int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)bytes_+at,length,nullptr,0);
        if(count<=0)return false;value.resize(count);
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)bytes_+at,length,&value[0],count);at+=length;
        return value.find(L'\0')==std::wstring::npos;
    }
    static unsigned color(const std::wstring& key){
        uint32_t hash=2166136261u;for(wchar_t c:key){hash^=(uint32_t)c;hash*=16777619u;}
        double h=(hash%360)/60.0,s=.63,l=.57,chroma=(1-std::fabs(2*l-1))*s,x=chroma*(1-std::fabs(std::fmod(h,2.0)-1)),m=l-chroma/2;
        double r=0,g=0,b=0;if(h<1){r=chroma;g=x;}else if(h<2){r=x;g=chroma;}else if(h<3){g=chroma;b=x;}else if(h<4){g=x;b=chroma;}else if(h<5){r=x;b=chroma;}else{r=chroma;b=x;}
        return ((unsigned)std::lround((r+m)*255)<<16)|((unsigned)std::lround((g+m)*255)<<8)|(unsigned)std::lround((b+m)*255);
    }
    bool insideRing(const Ring& ring,double x,double y)const{
        bool inside=false;XY last=point(ring.firstPoint+ring.pointCount-1);
        for(uint32_t i=0;i<ring.pointCount;++i){XY p=point(ring.firstPoint+i);
            double dx=p.x-last.x,dy=p.y-last.y,cross=(x-last.x)*dy-(y-last.y)*dx;
            if(std::fabs(cross)<=1e-10*std::max(1.0,std::hypot(dx,dy))&&x>=std::min(last.x,p.x)-1e-10&&x<=std::max(last.x,p.x)+1e-10&&y>=std::min(last.y,p.y)-1e-10&&y<=std::max(last.y,p.y)+1e-10)return true;
            if((p.y>y)!=(last.y>y)&&x<(last.x-p.x)*(y-p.y)/(last.y-p.y)+p.x)inside=!inside;
            last=p;
        }
        return inside;
    }
    bool contains(const Geometry& geometry,double longitude,double latitude)const{
        if(latitude<geometry.minY||latitude>geometry.maxY)return false;
        for(uint32_t p=0;p<geometry.polygonCount;++p){const auto& polygon=polygons_[geometry.firstPolygon+p];
            if(latitude<polygon.minY||latitude>polygon.maxY)continue;
            double x=longitude+std::round(((polygon.minX+polygon.maxX)*.5-longitude)/360.0)*360;
            if(x<polygon.minX||x>polygon.maxX||!insideRing(rings_[polygon.firstRing],x,latitude))continue;
            bool hole=false;for(uint32_t r=1;r<polygon.ringCount&&!hole;++r)hole=insideRing(rings_[polygon.firstRing+r],x,latitude);
            if(!hole)return true;
        }
        return false;
    }
    Gdiplus::GraphicsPath* pathFor(uint32_t id){
        if(auto* cached=paths_.find(id))return cached->get();
        std::unique_ptr<Gdiplus::GraphicsPath> result(new Gdiplus::GraphicsPath(Gdiplus::FillModeAlternate));size_t points=0;
        const auto& g=geometries_[id];
        for(uint32_t p=0;p<g.polygonCount;++p){const auto& polygon=polygons_[g.firstPolygon+p];for(uint32_t r=0;r<polygon.ringCount;++r){const auto& ring=rings_[polygon.firstRing+r];std::vector<Gdiplus::PointF> vertices;vertices.reserve(ring.pointCount);
            for(uint32_t v=0;v<ring.pointCount;++v){XY xy=point(ring.firstPoint+v);vertices.emplace_back((Gdiplus::REAL)xy.x,(Gdiplus::REAL)xy.y);}
            if(result->AddPolygon(vertices.data(),(INT)vertices.size())!=Gdiplus::Ok)return nullptr;points+=ring.pointCount;
        }}
        // Oversized single paths can be rendered once via transientPath_ without evicting the entire cache.
        if(points*20>32*1024*1024){transientPath_=std::move(result);return transientPath_.get();}
        paths_.put(id,std::move(result),points*20);auto* cached=paths_.find(id);return cached?cached->get():nullptr;
    }
    std::unique_ptr<Gdiplus::GraphicsPath> transientPath_;
public:
    Archive()=default;Archive(const Archive&)=delete;Archive& operator=(const Archive&)=delete;
    ~Archive(){close();}
    void close(){paths_.clear();transientPath_.reset();entities_.clear();records_.clear();geometries_.clear();polygons_.clear();rings_.clear();active_.clear();relations_.clear();year_=INT_MIN;pointCount_=0;pointOffset_=0;
        if(bytes_)UnmapViewOfFile(bytes_);bytes_=nullptr;if(mapping_)CloseHandle(mapping_);mapping_=nullptr;if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);file_=INVALID_HANDLE_VALUE;size_=0;
    }
    bool open(const std::wstring& file){
        close();file_=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_RANDOM_ACCESS,nullptr);if(file_==INVALID_HANDLE_VALUE)return false;
        LARGE_INTEGER size{};if(!GetFileSizeEx(file_,&size)||size.QuadPart<52||size.QuadPart>1024LL*1024*1024){close();return false;}size_=(size_t)size.QuadPart;
        mapping_=CreateFileMappingW(file_,nullptr,PAGE_READONLY,0,0,nullptr);if(!mapping_){close();return false;}
        bytes_=(const BYTE*)MapViewOfFile(mapping_,FILE_MAP_READ,0,0,0);if(!bytes_){close();return false;}
        auto reject=[&](){close();return false;};
        if(std::memcmp(bytes_,"WMHIST2\0",8)||u32(8)!=2)return reject();
        uint32_t ne=u32(12),nr=u32(16),ng=u32(20),np=u32(24),nh=u32(28),nv=u32(32),entityBytes=u32(44);
        earliest_=i32(36);latest_=i32(40);
        if(!ne||ne>20000||!nr||nr>200000||!ng||ng>200000||!np||np>2000000||!nh||nh>4000000||!nv||nv>20000000||earliest_<-100000||latest_>9999||earliest_>=latest_||entityBytes>size_-52||u32(48)!=0)return reject();
        size_t at=52,limit=52+(size_t)entityBytes;entities_.reserve(ne);
        for(uint32_t i=0;i<ne;++i){Entity e;if(!stringAt(at,limit,e.en)||!stringAt(at,limit,e.zh)||!stringAt(at,limit,e.wikipedia)||!stringAt(at,limit,e.wikidata)||!stringAt(at,limit,e.type)||!stringAt(at,limit,e.sourceName)||!stringAt(at,limit,e.noteZh)||!stringAt(at,limit,e.noteEn)||e.en.empty()||(e.type!=L"POLITY"&&e.type!=L"RELATION"))return reject();e.rgb=color(e.wikidata.empty()?e.en:e.wikidata);entities_.push_back(std::move(e));}
        if(at!=limit||!range(at,nr,32))return reject();records_.reserve(nr);
        for(uint32_t i=0;i<nr;++i,at+=32){Record r{u32(at),u32(at+12),i32(at+4),i32(at+8),f64(at+16),u32(at+24)!=0};if(r.entity>=ne||r.geometry>=ng||r.from>r.to||r.from<earliest_||r.to>latest_||!std::isfinite(r.area)||r.area<0||u32(at+24)>1||u32(at+28)!=0||(r.relation!=(entities_[r.entity].type==L"RELATION")))return reject();records_.push_back(r);}
        if(!range(at,ng,40))return reject();geometries_.reserve(ng);
        for(uint32_t i=0;i<ng;++i,at+=40){Geometry g{f64(at),f64(at+8),f64(at+16),f64(at+24),u32(at+32),u32(at+36)};if(!std::isfinite(g.minX)||!std::isfinite(g.maxX)||!std::isfinite(g.minY)||!std::isfinite(g.maxY)||g.minX>g.maxX||g.minY>g.maxY||g.minY<-90.000001||g.maxY>90.000001||g.minX<-1080||g.maxX>1080||!g.polygonCount||(uint64_t)g.firstPolygon+g.polygonCount>np)return reject();geometries_.push_back(g);}
        if(!range(at,np,8))return reject();polygons_.reserve(np);
        for(uint32_t i=0;i<np;++i,at+=8){Polygon p{u32(at),u32(at+4),0,0,0,0};if(!p.ringCount||(uint64_t)p.firstRing+p.ringCount>nh)return reject();polygons_.push_back(p);}
        if(!range(at,nh,8))return reject();rings_.reserve(nh);
        for(uint32_t i=0;i<nh;++i,at+=8){Ring r{u32(at),u32(at+4)};if(r.pointCount<4||(uint64_t)r.firstPoint+r.pointCount>nv)return reject();rings_.push_back(r);}
        if(!range(at,nv,16)||at+(size_t)nv*16!=size_)return reject();pointOffset_=at;pointCount_=nv;
        for(uint32_t i=0;i<nv;++i){XY p=point(i);if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::fabs(p.x)>1080||std::fabs(p.y)>90.000001)return reject();}
        for(auto& polygon:polygons_){const auto& ring=rings_[polygon.firstRing];polygon.minX=polygon.minY=1e100;polygon.maxX=polygon.maxY=-1e100;for(uint32_t i=0;i<ring.pointCount;++i){XY p=point(ring.firstPoint+i);polygon.minX=std::min(polygon.minX,p.x);polygon.maxX=std::max(polygon.maxX,p.x);polygon.minY=std::min(polygon.minY,p.y);polygon.maxY=std::max(polygon.maxY,p.y);}}
        return true;
    }
    bool valid()const{return bytes_&&!records_.empty();}
    int earliest()const{return earliest_;}int latest()const{return latest_;}int year()const{return year_;}
    size_t recordCount()const{return records_.size();}size_t geometryCount()const{return geometries_.size();}size_t pointCount()const{return pointCount_;}
    XY point(uint32_t i)const{return XY{f64(pointOffset_+(size_t)i*16),f64(pointOffset_+(size_t)i*16+8)};}
    const Entity& entity(size_t i)const{return entities_[i];}const Record& record(size_t i)const{return records_[i];}
    const std::vector<size_t>& active()const{return active_;}
    bool selectYear(int year){
        if(!valid()||year==0||year<earliest_||year>latest_)return false;if(year==year_)return true;
        year_=year;active_.clear();relations_.clear();
        for(size_t i=0;i<records_.size();++i){const auto& r=records_[i];if(r.from<=year&&year<=r.to)(r.relation?relations_:active_).push_back(i);}
        std::stable_sort(active_.begin(),active_.end(),[&](size_t a,size_t b){return records_[a].area>records_[b].area;});return true;
    }
    std::vector<size_t> hits(double latitude,double longitude,bool relationships=false)const{
        std::vector<size_t> hits;if(!valid()||!std::isfinite(latitude)||!std::isfinite(longitude)||std::fabs(latitude)>90||std::fabs(longitude)>180)return hits;
        for(size_t index:relationships?relations_:active_){const auto& r=records_[index];if(contains(geometries_[r.geometry],longitude,latitude))hits.push_back(index);}
        if(!relationships&&std::any_of(hits.begin(),hits.end(),[&](size_t i){return entities_[records_[i].entity].sourceName.find(L"Natural Earth reference: ")==0;}))hits.erase(std::remove_if(hits.begin(),hits.end(),[&](size_t i){return entities_[records_[i].entity].sourceName.find(L"Natural Earth reference: ")!=0;}),hits.end());
        std::stable_sort(hits.begin(),hits.end(),[&](size_t a,size_t b){return records_[a].area<records_[b].area;});return hits;
    }
    std::vector<int> changeYears()const{
        std::vector<int> years;for(const auto& r:records_)if(!r.relation){if(r.from&&r.from>=earliest_&&r.from<=latest_)years.push_back(r.from);int after=r.to+1;if(after&&after>=earliest_&&after<=latest_)years.push_back(after);}std::sort(years.begin(),years.end());years.erase(std::unique(years.begin(),years.end()),years.end());return years;
    }
    void draw(Gdiplus::Graphics& graphics,const RECT& map,double zoom,double centerX,double centerY,BYTE alpha,bool boundaries=true){
        if(!valid()||year_==INT_MIN||!alpha||zoom<=0||map.right<=map.left||map.bottom<=map.top)return;
        mapViewport::Projection projection{map,zoom,centerX,centerY};auto clip=graphics.Save();
        double left=std::max((double)map.left,projection.screenX(0)),right=std::min((double)map.right,projection.screenX(1)),top=std::max((double)map.top,projection.screenY(0)),bottom=std::min((double)map.bottom,projection.screenY(1));
        graphics.SetClip(Gdiplus::RectF((Gdiplus::REAL)left,(Gdiplus::REAL)top,(Gdiplus::REAL)std::max(0.,right-left),(Gdiplus::REAL)std::max(0.,bottom-top)),Gdiplus::CombineModeIntersect);
        if(alpha<255){
            int width=map.right-map.left,height=map.bottom-map.top;Gdiplus::Bitmap layer(width,height,PixelFormat32bppARGB);if(layer.GetLastStatus()!=Gdiplus::Ok){graphics.Restore(clip);return;}
            {Gdiplus::Graphics offscreen(&layer);offscreen.Clear(Gdiplus::Color(0,0,0,0));RECT local{0,0,width,height};draw(offscreen,local,zoom,centerX,centerY,255,boundaries);}
            Gdiplus::ColorMatrix matrix{};matrix.m[0][0]=matrix.m[1][1]=matrix.m[2][2]=matrix.m[4][4]=1;matrix.m[3][3]=alpha/255.0f;
            Gdiplus::ImageAttributes attributes;attributes.SetColorMatrix(&matrix);graphics.DrawImage(&layer,Gdiplus::Rect(map.left,map.top,width,height),0,0,width,height,Gdiplus::UnitPixel,&attributes);graphics.Restore(clip);return;
        }
        const double sx=projection.width()*zoom/360.0,sy=projection.height()*zoom/180.0;
        const double tx=(map.left+map.right)/2.0+(.5-centerX)*projection.width()*zoom,ty=(map.top+map.bottom)/2.0+(.5-centerY)*projection.height()*zoom;
        double north=90-projection.worldY(map.top)*180,south=90-projection.worldY(map.bottom)*180,west=projection.worldX(map.left)*360-180,east=projection.worldX(map.right)*360-180;
        auto saved=graphics.Save();graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        for(size_t index:active_){const auto& r=records_[index];const auto& g=geometries_[r.geometry];if(g.maxY<south||g.minY>north)continue;
            Gdiplus::GraphicsPath* path=nullptr;const auto& e=entities_[r.entity];Gdiplus::SolidBrush fill(Gdiplus::Color(alpha,(BYTE)(e.rgb>>16),(BYTE)(e.rgb>>8),(BYTE)e.rgb));Gdiplus::Pen border(Gdiplus::Color((BYTE)std::min(205,(int)alpha),25,47,60),(Gdiplus::REAL)(.65/std::max(.0001,sx)));
            for(int wrap=-1;wrap<=1;++wrap){double shift=wrap*360.0;if(g.maxX+shift<west||g.minX+shift>east)continue;if(!path)path=pathFor(r.geometry);if(!path)continue;
                Gdiplus::Matrix transform((Gdiplus::REAL)sx,0,0,(Gdiplus::REAL)-sy,(Gdiplus::REAL)(tx+shift*sx),(Gdiplus::REAL)ty);graphics.SetTransform(&transform);graphics.FillPath(&fill,path);if(boundaries)graphics.DrawPath(&border,path);
            }
        }
        graphics.Restore(saved);graphics.Restore(clip);
    }
    Gdiplus::Bitmap* raster(int width=1120,int height=560){
        std::unique_ptr<Gdiplus::Bitmap> bitmap(new Gdiplus::Bitmap(width,height,PixelFormat32bppARGB));if(bitmap->GetLastStatus()!=Gdiplus::Ok)return nullptr;
        {Gdiplus::Graphics gr(bitmap.get());gr.Clear(Gdiplus::Color(0,0,0,0));RECT map{0,0,width,height};draw(gr,map,1,.5,.5,255,false);}return bitmap.release();
    }
};
}
