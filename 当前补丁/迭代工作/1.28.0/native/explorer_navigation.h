#pragma once
#include <cmath>
#include <string>
#include <vector>
namespace explorerNavigation {
struct View {
    double zoom=1,cx=.5,cy=.5,latitude=0,longitude=0;
    int row=-1,column=-1,grid=0;
    bool exact=false,input=false,fine=false;
    std::wstring name,origin;
};
inline bool same(const View& a,const View& b){return std::abs(a.zoom-b.zoom)<1e-9&&std::abs(a.cx-b.cx)<1e-10&&std::abs(a.cy-b.cy)<1e-10&&std::abs(a.latitude-b.latitude)<1e-9&&std::abs(a.longitude-b.longitude)<1e-9&&a.row==b.row&&a.column==b.column&&a.grid==b.grid&&a.exact==b.exact&&a.input==b.input&&a.fine==b.fine;}
class History {
    std::vector<View> back_,forward_;View current_;bool initialized_=false;
    static void push(std::vector<View>& list,const View& value){if(list.size()==64)list.erase(list.begin());list.push_back(value);}
public:
    void reset(const View& value){back_.clear();forward_.clear();current_=value;initialized_=true;}
    bool record(const View& value){if(!initialized_){reset(value);return false;}if(same(current_,value)){current_=value;return false;}push(back_,current_);current_=value;forward_.clear();return true;}
    bool canBack()const{return !back_.empty();}bool canForward()const{return !forward_.empty();}
    bool back(View& result){if(!canBack())return false;push(forward_,current_);current_=back_.back();back_.pop_back();result=current_;return true;}
    bool forward(View& result){if(!canForward())return false;push(back_,current_);current_=forward_.back();forward_.pop_back();result=current_;return true;}
    size_t backCount()const{return back_.size();}size_t forwardCount()const{return forward_.size();}
};
}
