#include "../native/event_catalog.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <set>
static std::wstring decode(const std::string& s){int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0);std::wstring out(size,L'\0');if(size)MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),&out[0],size);return out;}
int main(int argc,char** argv){int checks=0;auto check=[&](bool value,const char* label){if(!value){std::cerr<<"FAIL "<<label<<"\n";std::exit(1);}++checks;};if(argc<2)return 2;std::ifstream in(argv[1],std::ios::binary);std::string raw((std::istreambuf_iterator<char>(in)),{});std::vector<events::Event> all;check(events::parse(raw,all,decode),"parse real V2 catalog");check(all.size()==346,"346 events");std::set<std::wstring> ids,categories;int located=0;
for(const auto& e:all){check(e.year!=0&&e.endYear>=e.year,"valid dating");check(!e.titleEn.empty()&&!e.descriptionEn.empty(),"English reading fields");check(ids.insert(e.id).second,"unique event ID");categories.insert(e.category);located+=e.hasCoordinate;}
check(categories.size()==19,"19 normalized categories");check(located==120,"120 source Earth reference points");
auto baghdad=events::filter(all,L"蒙古 巴格达",L"");check(baghdad.size()==1&&all[baghdad[0]].year==1258,"Chinese AND search");check(events::filter(all,L"BEAGLE voyage",L"").size()==1,"case-insensitive English AND search");check(events::filter(all,L"zzz-not-an-event",L"").empty(),"empty result");check(events::filter(all,L"",L"",1000,900).empty(),"reversed interval");
bool eastHan=false;for(const auto& e:all)if(e.title.find(L"东汉建立")!=std::wstring::npos){eastHan=e.year==25;}check(eastHan,"East Han year corrected to 25 CE");
events::Event period;period.year=1914;period.endYear=1918;check(events::matches(period,L"",L"",1916,1916),"period intersects middle year");check(!events::matches(period,L"",L"",1919,1920),"period outside range");
auto saved=all;for(const char* bad:{"0\tx\tx\tx\tx\thttps://example.com\n","25\tx\tx\tx\tx\tfile:///secret\n","25\tx\tx\tx\tx\thttps://example.com\n24\tx\tx\tx\tx\thttps://example.com\n","25\tx\tx\tx\tx\thttps://example.com\t25\tnan\t0\tt\td\tp\tc\tn\tn\tid\n"}){check(!events::parse(bad,all,decode),"reject malformed catalog");check(all.size()==saved.size(),"rejection preserves previous catalog");}
check(events::parse("25\tx\tx\tx\tx\thttps://example.com\n",all,decode)&&all.size()==1&&all[0].endYear==25,"legacy six-column compatibility");std::cout<<checks<<" event catalog checks passed\n";return 0;}
