#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace mapViewport {
// A viewport can have any shape. The world remains a 2:1 equirectangular image.
struct Projection {
    RECT viewport;double zoom,centerX,centerY;
    double width()const{return std::min(std::max(1.0,double(viewport.right-viewport.left)),std::max(1.0,double(viewport.bottom-viewport.top))*2.0);}
    double height()const{return width()*.5;}
    double screenX(double x)const{return (viewport.left+viewport.right)*.5+(x-centerX)*width()*zoom;}
    double screenY(double y)const{return (viewport.top+viewport.bottom)*.5+(y-centerY)*height()*zoom;}
    double worldX(double x)const{return centerX+(x-(viewport.left+viewport.right)*.5)/(width()*zoom);}
    double worldY(double y)const{return centerY+(y-(viewport.top+viewport.bottom)*.5)/(height()*zoom);}
    double halfX()const{return std::min(.5,std::max(1.0,double(viewport.right-viewport.left))/(width()*zoom)*.5);}
    double halfY()const{return std::min(.5,std::max(1.0,double(viewport.bottom-viewport.top))/(height()*zoom)*.5);}
    void clamp(){centerX=std::max(halfX(),std::min(1-halfX(),centerX));centerY=std::max(halfY(),std::min(1-halfY(),centerY));}
};
inline size_t visibleTiles(const Projection& view,int columns=36,int rows=18){double left=std::max(0.,view.worldX(view.viewport.left)),right=std::min(1.,view.worldX(view.viewport.right)),top=std::max(0.,view.worldY(view.viewport.top)),bottom=std::min(1.,view.worldY(view.viewport.bottom));if(left>=right||top>=bottom)return 0;return (size_t)std::max(1,(int)std::ceil(right*columns)-(int)std::floor(left*columns))*(size_t)std::max(1,(int)std::ceil(bottom*rows)-(int)std::floor(top*rows));}
inline bool detailFits(const Projection& view,int pixels,size_t budget=192*1024*1024){return pixels>0&&visibleTiles(view)*(uint64_t)pixels*pixels*4<=budget;}
}
