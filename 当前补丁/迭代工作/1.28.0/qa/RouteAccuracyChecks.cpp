#include "../native/route_catalog.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <cstring>
#include <codecvt>
#include <locale>
static std::string json(const std::string& s){std::string out="\"";for(unsigned char c:s){if(c=='"'||c=='\\')out+='\\';if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else if(c=='\t')out+="\\t";else out+=(char)c;}return out+'"';}
static std::string utf8(const wchar_t* s){return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(s);}
int main(int argc,char** argv){
    int checks=0;auto check=[&](bool value,const char* message){if(!value){std::cerr<<"FAIL "<<message<<"\n";std::exit(1);}++checks;};
    const auto& all=routes::catalog();check(all.size()==42,"42 routes");std::set<std::string> ids;int longRoutes=0;
    for(const auto& route:all){
        check(ids.insert(route.id).second,"unique route ID");check(route.zh&&*route.zh&&route.en&&*route.en,"bilingual name");check(route.noteZh&&*route.noteZh&&route.noteEn&&*route.noteEn,"bilingual method");check(!route.sources.empty(),"source links");
        for(const auto* source:route.sources)check(std::string(source).find("https://")==0,"HTTPS source");
        auto points=routes::pointsFor(route);check(routes::validRoute(points),"valid coordinates and breaks");routes::DistanceProfile profile(points);check(profile.length()>0,"positive connection length");if(profile.length()>10000)++longRoutes;
        check(profile.progress(profile.length())==points.size()-1,"distance reaches final point");
        auto previous=routes::rasterize(points,1120,560,0);const int frames=80;
        double old=0;for(int frame=1;frame<=frames;++frame){double progress=profile.progress(profile.length()*frame/frames);check(progress>=old,"distance progress is monotonic");old=progress;
            auto current=routes::rasterize(points,1120,560,progress);check(current.size()==627200,"grid dimensions");bool subset=true;for(size_t i=0;i<current.size();++i)if(previous[i]&&!current[i]){subset=false;break;}check(subset,"route does not erase previous cells");previous.swap(current);
        }
        double chunked=0;for(int tick=0;tick<100;++tick)chunked=profile.advance(chunked,.2,30);double single=profile.advance(0,20,30);check(std::abs(profile.at(chunked)-profile.at(single))<1e-6,"playback independent of timer chunk size");
        if(std::strcmp(route.id,"beagle_darwin")==0){check(points.size()==596,"596 source-derived Beagle positions");size_t logged=0,gaps=0;for(const auto& p:route.stops){logged+=p.kind==routes::WaypointKind::LoggedPosition;gaps+=p.position.breakBefore;}check(logged==407,"407 FitzRoy-attributed positions");check(gaps==13,"13 explicit source gaps");}
        std::cout<<"PASS "<<route.id<<" points="<<points.size()<<" length-km="<<(int)profile.length()<<"\n";
    }
    check(longRoutes>=8,"at least eight routes exceed 10000 km between reference points");
    std::vector<routes::Point> split{{0,0},{0,1},{40,80,true},{40,81}};routes::DistanceProfile broken(split);check(std::abs(broken.length()-(routes::distanceKm(split[0],split[1])+routes::distanceKm(split[2],split[3])))<1e-7,"gap contributes no invented distance");
    check(broken.progress(broken.cumulative[1])>=2,"playback jumps the unconnected gap");
    std::vector<routes::Point> dateLine{{10,179},{10,-179}};check(routes::DistanceProfile(dateLine).length()<230,"date-line distance uses short crossing");
    check(routes::distanceKm({-90,1},{-90,179})<1e-7,"pole has no unique longitude");
    if(argc>1){std::ofstream output(argv[1],std::ios::binary);output<<"[";for(size_t i=0;i<all.size();++i){if(i)output<<",";const auto& r=all[i];output<<"{\"id\":"<<json(r.id)<<",\"zh\":"<<json(utf8(r.zh))<<",\"en\":"<<json(utf8(r.en))<<",\"period\":"<<json(utf8(r.period))<<",\"mode\":"<<json(utf8(routes::travelModeLabel(r.mode,false)))<<",\"noteZh\":"<<json(utf8(r.noteZh))<<",\"noteEn\":"<<json(utf8(r.noteEn))<<",\"sources\":[";for(size_t u=0;u<r.sources.size();++u){if(u)output<<",";output<<json(r.sources[u]);}output<<"],\"sea\":"<<(r.mode==routes::TravelMode::Sea?"true":"false")<<",\"points\":[";for(size_t j=0;j<r.stops.size();++j){if(j)output<<",";auto p=r.stops[j].position;output<<"["<<p.latitude<<","<<p.longitude<<","<<(p.breakBefore?"true":"false")<<"]";}output<<"],\"places\":[";for(size_t j=0;j<r.stops.size();++j){if(j)output<<",";const auto& p=r.stops[j];output<<"{\"zh\":"<<json(utf8(p.zh))<<",\"en\":"<<json(utf8(p.en))<<",\"kind\":"<<json(utf8(routes::waypointKindLabel(p.kind,false)))<<",\"date\":"<<json(utf8(p.date))<<"}";}output<<"]}";}output<<"]";}
    std::cout<<checks<<" meaningful route checks passed\n";return 0;
}
