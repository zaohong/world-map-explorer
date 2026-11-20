#define NOMINMAX
#include "../native/place_catalog.h"
#include "../native/saved_places.h"
#include "../native/explorer_navigation.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <set>
#include <chrono>
#include <limits>

static int checks=0,failures=0;
static void check(bool ok,const char* message){++checks;if(!ok){++failures;std::cout<<"FAIL: "<<message<<"\n";}}
static std::wstring decode(const std::string& text){if(text.empty())return {};int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),(int)text.size(),nullptr,0);if(!n)return {};std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),(int)text.size(),&out[0],n);return out;}
static std::string encode(const std::wstring& text){int n=WideCharToMultiByte(CP_UTF8,0,text.data(),(int)text.size(),nullptr,0,nullptr,nullptr);std::string out(n,0);if(n)WideCharToMultiByte(CP_UTF8,0,text.data(),(int)text.size(),&out[0],n,nullptr,nullptr);return out;}
static std::string readBytes(const std::wstring& file){std::ifstream in(std::filesystem::path(file),std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
static void rawWrite(const std::wstring& file,const std::string& bytes){std::ofstream out(std::filesystem::path(file),std::ios::binary);out.write(bytes.data(),bytes.size());}
int wmain(int argc,wchar_t** argv){
    if(argc!=3)return 2;placeCatalog::Catalog catalog;
    check(catalog.parse(readBytes(argv[1]),decode),"full offline place catalog loads");
    check(catalog.entries().size()==34506,"all 34248 cities and 258 country references are present");
    std::set<std::wstring> ids;int countries=0;for(const auto& p:catalog.entries()){check(ids.insert(p.id).second,"unique source identity");countries+=p.country;check(std::isfinite(p.latitude)&&std::abs(p.latitude)<=90&&std::abs(p.longitude)<=180,"bounded source coordinates");}check(countries==258,"country reference count");
    auto first=[&](const wchar_t* q,const wchar_t* id){auto m=catalog.search(q,1);bool ok=!m.empty()&&catalog.entries()[m[0].index].id==id;if(!ok)std::cout<<"Query "<<encode(q)<<" expected "<<encode(id)<<" actual "<<(m.empty()?"(none)":encode(catalog.entries()[m[0].index].id))<<"\n";check(ok,"expected first match for multilingual/disambiguated query");};
    first(L"London",L"g:2643743");first(L"LONDON Ontario",L"g:6058560");first(L"中国 北京",L"g:1816670");first(L"beijing CHN",L"g:1816670");first(L"紐約",L"g:5128581");first(L"成都",L"g:1815286");first(L"São Paulo",L"g:3448439");first(L"中国",L"c:CHN");first(L"Georgia",L"c:GEO");first(L"Mexico",L"c:MEX");
    auto london=catalog.search(L"London",60);bool uk=false,ca=false;for(const auto& m:london){const auto& p=catalog.entries()[m.index];uk|=p.id==L"g:2643743";ca|=p.id==L"g:6058560";}check(uk&&ca,"homonymous cities remain separate selectable results");
    auto beijing=catalog.search(L"北京",1);check(!beijing.empty()&&std::abs(catalog.entries()[beijing[0].index].latitude-39.9075)<1e-8&&std::abs(catalog.entries()[beijing[0].index].longitude-116.39723)<1e-8,"Beijing keeps source reference coordinates");
    check(placeCatalog::fold(L"  SÃO-Paulo / 紐約 ")==L"sao paulo 纽约","normalization handles accents, separators and traditional Chinese");
    size_t total=0;check(catalog.search(L"London",0,&total).empty()&&total>=2,"zero result limit still returns total count");check(catalog.search(L"zzzznonexistentplace983451",60).empty(),"unknown search produces no false city");
    auto count=catalog.entries().size();for(auto raw:{std::string("city\tx\tx"),std::string("city\tx\tx\t\t\t\t\tNaN\t0\t0\tPPL\t\t12\n"),std::string("city\tx\tx\t\t\t\t\t91\t0\t0\tPPL\t\t12\n")}){check(!catalog.parse(raw,decode),"malformed catalog rejected");check(catalog.entries().size()==count,"failed reload preserves previous catalog");}
    auto start=std::chrono::steady_clock::now();for(int i=0;i<100;++i)catalog.search(i%2?L"London Ontario":L"北京",30);auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();std::cout<<"100 catalog searches: "<<elapsed<<" ms\n";

    explorerNavigation::History history;explorerNavigation::View v,out;history.reset(v);check(!history.canBack()&&!history.canForward(),"initial navigation has no phantom entries");for(int i=1;i<=100;++i){v.cx=i/200.;v.name=L"Point "+std::to_wstring(i);v.origin=L"Source "+std::to_wstring(i);history.record(v);}check(history.backCount()==64,"navigation memory is bounded to 64 views");for(int i=99;i>=36;--i){check(history.back(out)&&out.name==L"Point "+std::to_wstring(i)&&out.origin==L"Source "+std::to_wstring(i),"back preserves coordinates, title and source");}check(!history.back(out),"oldest retained view is a true boundary");check(!history.record(out)&&history.canForward(),"duplicate capture does not destroy forward history");check(history.forward(out)&&out.name==L"Point 37","forward restores the next view");v=out;v.latitude=24;v.exact=true;v.origin=L"New branch";check(history.record(v)&&!history.canForward(),"new navigation clears stale forward branch");history.reset(v);check(!history.canBack()&&!history.canForward(),"reset clears both directions");

    v.name=L"Translated name";v.origin=L"Translated origin";check(!history.record(v)&&!history.canBack(),"translation updates metadata without creating navigation");v.cx+=.01;history.record(v);check(history.back(out)&&out.name==L"Translated name"&&out.origin==L"Translated origin","metadata refresh is retained by later navigation");
    std::filesystem::create_directories(argv[2]);std::wstring file=std::wstring(argv[2])+L"\\saved-roundtrip.bin";std::vector<savedPlaces::Place> places{{L"北京 · \"城市\"",39.9075,116.39723,12},{L"São Paulo / 紐約",-23.5475,-46.63611,64}},loaded;
    check(savedPlaces::write(file,places)&&savedPlaces::read(file,loaded),"Unicode saved places round trip");check(loaded.size()==2&&loaded[0].name==places[0].name&&loaded[1].zoom==64&&loaded[1].latitude==places[1].latitude,"saved names, coordinates and zoom are lossless");auto original=readBytes(file);auto duplicate=places;duplicate.push_back(places[0]);check(!savedPlaces::write(file,duplicate)&&readBytes(file)==original,"duplicate rejection preserves last good file");
    auto invalid=places;invalid[0].name=L"bad\nname";check(!savedPlaces::write(file,invalid)&&readBytes(file)==original,"invalid name cannot overwrite saved places");invalid=places;invalid[0].latitude=std::numeric_limits<double>::infinity();check(!savedPlaces::write(file,invalid),"nonfinite saved coordinate rejected");invalid.assign(65,{L"x",0,0,1});check(!savedPlaces::write(file,invalid),"saved place count is bounded");
    for(auto corrupt:{original.substr(0,original.size()-1),original+"x",std::string("BADMAGICxxxx")}){auto bad=std::wstring(argv[2])+L"\\corrupt.bin";rawWrite(bad,corrupt);auto previous=loaded;check(!savedPlaces::read(bad,loaded)&&loaded.size()==previous.size()&&loaded[0].name==previous[0].name,"corrupt file rejected without erasing loaded places");}
    auto badPath=std::wstring(argv[2])+L"\\missing\\saved.bin";check(!savedPlaces::write(badPath,places)&&readBytes(file)==original,"write failure leaves existing collection intact");check(savedPlaces::write(file,{})&&savedPlaces::read(file,loaded)&&loaded.empty(),"empty saved collection can be persisted");
    std::cout<<"RESULT checks="<<checks<<" failures="<<failures<<"\n";return failures?1:0;
}
