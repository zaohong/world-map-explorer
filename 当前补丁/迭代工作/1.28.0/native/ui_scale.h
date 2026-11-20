#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
static int gUiScaleMode=0,gUiDpi=96;
static const UINT UI_SCALE_CHANGED=WM_APP+0x41;
static UINT systemUiDpi(){using Fn=UINT(WINAPI*)();static auto fn=(Fn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForSystem");return fn?fn():96;}
static UINT windowUiDpi(HWND window){using Fn=UINT(WINAPI*)(HWND);static auto fn=(Fn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow");UINT dpi=window&&fn?fn(window):0;return dpi?dpi:systemUiDpi();}
static int uiDpiForMode(int mode,HWND window,UINT known=0){static const int choices[]={0,96,120,144,192};int dpi=mode>=1&&mode<=4?choices[mode]:(int)(known?known:windowUiDpi(window));return std::max(72,std::min(480,dpi));}
static int effectiveUiDpi(HWND window,UINT known=0){return uiDpiForMode(gUiScaleMode,window,known);}
struct UiDpiScope {int previous;explicit UiDpiScope(HWND window,UINT known=0):previous(gUiDpi){gUiDpi=effectiveUiDpi(window,known);}~UiDpiScope(){gUiDpi=previous;}};
static int uiPx(int value){return MulDiv(value,gUiDpi,96);}
static int uiDip(int value){return MulDiv(value,96,gUiDpi);}
static float uiPxF(float value){return value*gUiDpi/96.f;}
static float uiScaleFactor(){return gUiDpi/96.f;}
static RECT uiRect(int left,int top,int right,int bottom){return RECT{uiPx(left),uiPx(top),uiPx(right),uiPx(bottom)};}
static RECT uiDipRect(const RECT& r){return RECT{uiDip(r.left),uiDip(r.top),uiDip(r.right),uiDip(r.bottom)};}
static void placeUiControl(HWND control,int x,int y,int width,int height){SetWindowPos(control,nullptr,uiPx(x),uiPx(y),std::max(1,uiPx(width)),std::max(1,uiPx(height)),SWP_NOZORDER|SWP_NOACTIVATE);}
static RECT uiWindowBounds(HWND window,int width,int height){MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);const RECT& work=monitor.rcWork;RECT current{};if(window)GetWindowRect(window,&current);else current=work;int w=std::max(1,std::min(width,(int)(work.right-work.left)-uiPx(12))),h=std::max(1,std::min(height,(int)(work.bottom-work.top)-uiPx(12)));int x=std::max((int)work.left,std::min((int)work.right-w,(int)((current.left+current.right-w)/2))),y=std::max((int)work.top,std::min((int)work.bottom-h,(int)((current.top+current.bottom-h)/2)));return RECT{x,y,x+w,y+h};}
static void applyUiWindowBounds(HWND window,UINT message,WPARAM oldDpi,LPARAM suggested){if(message==WM_DPICHANGED&&suggested){RECT r=*(RECT*)suggested;SetWindowPos(window,nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);}else if(oldDpi&&!IsZoomed(window)&&!IsIconic(window)){RECT r{};GetWindowRect(window,&r);RECT fit=uiWindowBounds(window,MulDiv(r.right-r.left,gUiDpi,(int)oldDpi),MulDiv(r.bottom-r.top,gUiDpi,(int)oldDpi));SetWindowPos(window,nullptr,fit.left,fit.top,fit.right-fit.left,fit.bottom-fit.top,SWP_NOZORDER|SWP_NOACTIVATE);}}
static void uiMinimumWindow(HWND window,MINMAXINFO* limits,int width,int height){MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);limits->ptMinTrackSize=POINT{std::min(uiPx(width),(int)(monitor.rcWork.right-monitor.rcWork.left)),std::min(uiPx(height),(int)(monitor.rcWork.bottom-monitor.rcWork.top))};}
// Non-client chrome always follows the real monitor, even with a custom client scale.
static SIZE uiOuterSize(HWND owner,int width,int height,DWORD style,DWORD extended){RECT r=uiRect(0,0,width,height);using Fn=BOOL(WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);static auto fn=(Fn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi");if(fn)fn(&r,style,FALSE,extended,windowUiDpi(owner));else AdjustWindowRectEx(&r,style,FALSE,extended);return SIZE{r.right-r.left,r.bottom-r.top};}
