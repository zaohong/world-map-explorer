#pragma once
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace events {
struct Event {
    int year=1;
    std::wstring category,place,title,description,source;
    int endYear=1;
    double latitude=0,longitude=0;
    bool hasCoordinate=false;
    std::wstring titleEn,descriptionEn,placeEn,categoryEn,dateNoteZh,dateNoteEn,id;
    const std::wstring& heading(bool en)const{return en&&!titleEn.empty()?titleEn:title;}
    const std::wstring& summary(bool en)const{return en&&!descriptionEn.empty()?descriptionEn:description;}
    const std::wstring& location(bool en)const{return en&&!placeEn.empty()?placeEn:place;}
    const std::wstring& kind(bool en)const{return en&&!categoryEn.empty()?categoryEn:category;}
};
inline bool integer(const std::string& text,int& value){
    if(text.empty()||text.size()>7)return false;char* end=nullptr;long n=std::strtol(text.c_str(),&end,10);
    if(end==text.c_str()||*end||n<-100000||n>9999||n==0)return false;value=(int)n;return true;
}
inline bool coordinate(const std::string& text,double low,double high,double& value){
    if(text.empty()||text.size()>40)return false;char* end=nullptr;double n=std::strtod(text.c_str(),&end);
    if(end==text.c_str()||*end||!std::isfinite(n)||n<low||n>high)return false;value=n;return true;
}
inline bool parse(const std::string& raw,std::vector<Event>& output,std::wstring(*decode)(const std::string&)){
    std::vector<Event> candidate;std::istringstream input(raw);std::string line;int previous=INT_MIN;
    while(std::getline(input,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;
        std::vector<std::string> col;size_t begin=0;for(size_t i=0;i<=line.size();++i)if(i==line.size()||line[i]=='\t'){col.push_back(line.substr(begin,i-begin));begin=i+1;}
        if((col.size()!=6&&col.size()!=16)||candidate.size()>=10000)return false;
        Event e;if(!integer(col[0],e.year)||e.year<previous)return false;e.endYear=e.year;
        if(col[1].empty()||col[2].empty()||col[3].empty()||col[4].empty()||(col[5].find("https://")!=0&&col[5].find("http://")!=0))return false;
        e.category=decode(col[1]);e.place=decode(col[2]);e.title=decode(col[3]);e.description=decode(col[4]);e.source=decode(col[5]);
        if(col.size()==16){
            if(!integer(col[6],e.endYear)||e.endYear<e.year)return false;
            if(!col[7].empty()||!col[8].empty()){if(!coordinate(col[7],-90,90,e.latitude)||!coordinate(col[8],-180,180,e.longitude))return false;e.hasCoordinate=true;}
            e.titleEn=decode(col[9]);e.descriptionEn=decode(col[10]);e.placeEn=decode(col[11]);e.categoryEn=decode(col[12]);e.dateNoteZh=decode(col[13]);e.dateNoteEn=decode(col[14]);e.id=decode(col[15]);
        }
        previous=e.year;candidate.push_back(std::move(e));
    }
    if(candidate.empty())return false;output.swap(candidate);return true;
}
inline std::wstring fold(std::wstring value){for(auto& c:value)c=(wchar_t)std::towlower(c);return value;}
inline bool matches(const Event& e,const std::wstring& query,const std::wstring& category,int from=INT_MIN,int to=INT_MAX){
    if(e.year>to||e.endYear<from||(!category.empty()&&category!=e.category))return false;
    if(query.empty())return true;
    std::wstring hay=fold(e.title+L" "+e.description+L" "+e.place+L" "+e.category+L" "+e.titleEn+L" "+e.descriptionEn+L" "+e.placeEn+L" "+e.categoryEn+L" "+e.id);
    std::wistringstream terms(fold(query));std::wstring term;while(terms>>term)if(hay.find(term)==std::wstring::npos)return false;return true;
}
inline std::vector<size_t> filter(const std::vector<Event>& all,const std::wstring& query,const std::wstring& category,int from=INT_MIN,int to=INT_MAX){
    std::vector<size_t> result;if(from>to)return result;for(size_t i=0;i<all.size();++i)if(matches(all[i],query,category,from,to))result.push_back(i);return result;
}
}
