#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <cwctype>
#pragma comment(lib,"normaliz.lib")
namespace placeCatalog {
inline std::wstring simplified(const std::wstring& text){if(text.empty())return text;int size=LCMapStringW(0x0804,LCMAP_SIMPLIFIED_CHINESE,text.data(),(int)text.size(),nullptr,0);if(size<=0)return text;std::wstring out(size,L'\0');LCMapStringW(0x0804,LCMAP_SIMPLIFIED_CHINESE,text.data(),(int)text.size(),&out[0],size);return out;}
inline std::wstring fold(const std::wstring& text){
    auto out=simplified(text);if(out.empty())return out;CharLowerBuffW(&out[0],(DWORD)out.size());
    int size=NormalizeString(NormalizationD,out.data(),(int)out.size(),nullptr,0);if(size>0){std::wstring normal(size,L'\0');int written=NormalizeString(NormalizationD,out.data(),(int)out.size(),&normal[0],size);if(written>0){normal.resize(written);out.swap(normal);}}
    std::vector<WORD> types(out.size());GetStringTypeW(CT_CTYPE3,out.data(),(int)out.size(),types.data());std::wstring result;bool space=true;
    for(size_t i=0;i<out.size();++i){if(types[i]&C3_NONSPACING)continue;wchar_t c=out[i];if(c==L','||c==L';'||c==L'/'||c==L'|'||c==L'('||c==L')'||c==L'（'||c==L'）'||c==L'-'||c==L'_'||iswspace(c)){if(!space){result+=L' ';space=true;}}else{result+=c;space=false;}}
    if(!result.empty()&&result.back()==L' ')result.pop_back();return result;
}
inline bool number(const std::string& text,double& value){if(text.empty()||text.size()>40)return false;char* end=nullptr;value=std::strtod(text.c_str(),&end);return end!=text.c_str()&&!*end&&std::isfinite(value);}
struct Place {
    bool country=false;std::wstring id,en,zh,countryEn,countryZh,admin,feature,aliases,folded,titleFolded,zhFolded,countryAliasFolded;
    double latitude=0,longitude=0,population=0,zoom=12;
    const std::wstring& name(bool english)const{return english||zh.empty()?en:zh;}
    std::wstring context(bool english)const{auto value=english||countryZh.empty()?countryEn:countryZh;return admin.empty()?value:value+L" · "+admin;}
};
struct Match {size_t index;double score;};
class Catalog {
    std::vector<Place> places_;
public:
    const std::vector<Place>& entries()const{return places_;}
    bool parse(const std::string& raw,std::wstring(*decode)(const std::string&)){
        std::vector<Place> candidate;std::istringstream stream(raw);std::string line;
        while(std::getline(stream,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;std::vector<std::string> cols;size_t begin=0;for(size_t i=0;i<=line.size();++i)if(i==line.size()||line[i]=='\t'){cols.push_back(line.substr(begin,i-begin));begin=i+1;}
            if(cols.size()!=13||candidate.size()>=100000||(cols[0]!="city"&&cols[0]!="country"))return false;Place p;p.country=cols[0]=="country";p.id=decode(cols[1]);p.en=decode(cols[2]);p.zh=simplified(decode(cols[3]));p.countryEn=decode(cols[4]);p.countryZh=simplified(decode(cols[5]));p.admin=decode(cols[6]);p.feature=decode(cols[10]);p.aliases=decode(cols[11]);
            if(p.id.empty()||p.en.empty()||!number(cols[7],p.latitude)||!number(cols[8],p.longitude)||std::abs(p.latitude)>90||std::abs(p.longitude)>180||!number(cols[9],p.population)||p.population<0||!number(cols[12],p.zoom)||p.zoom<1||p.zoom>128)return false;
            p.titleFolded=fold(p.en);p.zhFolded=fold(p.zh);if(p.country)p.countryAliasFolded=L" "+fold(p.aliases)+L" ";p.folded=fold(p.en+L" "+p.zh+L" "+p.countryEn+L" "+p.countryZh+L" "+p.admin+L" "+p.aliases);candidate.push_back(std::move(p));
        }
        if(candidate.empty())return false;places_.swap(candidate);return true;
    }
    std::vector<Match> search(const std::wstring& query,size_t limit=40,size_t* total=nullptr)const{
        auto needle=fold(query);std::wistringstream words(needle);std::vector<std::wstring> terms;std::wstring word;while(words>>word)terms.push_back(word);std::vector<Match> matches;
        for(size_t i=0;i<places_.size();++i){const auto& p=places_[i];bool hit=true;for(const auto& term:terms)if(p.folded.find(term)==std::wstring::npos){hit=false;break;}if(!hit)continue;
            double score=p.country?15:std::min(80.0,std::log10(std::max(1.0,p.population))*10);if(p.feature==L"PPLC")score+=24;
            if(!needle.empty()){if(p.titleFolded==needle||p.zhFolded==needle)score+=p.country?1200:1000;else if(p.country&&p.countryAliasFolded.find(L" "+needle+L" ")!=std::wstring::npos)score+=1100;else if(p.titleFolded.find(needle)==0||p.zhFolded.find(needle)==0)score+=600;else score+=400;}
            matches.push_back({i,score});
        }
        if(total)*total=matches.size();limit=std::min(limit,matches.size());std::partial_sort(matches.begin(),matches.begin()+limit,matches.end(),[&](const Match& a,const Match& b){return a.score!=b.score?a.score>b.score:places_[a.index].id<places_[b.index].id;});matches.resize(limit);return matches;
    }
};
}
