#define NOMINMAX
#include "../native/history_vectors.h"
#include "../native/history_years.generated.h"
#include <iostream>
#include <fstream>
#include <chrono>
using namespace Gdiplus;
static int checks=0,failures=0;
static void check(bool ok,const char* name){++checks;std::cout<<(ok?"PASS: ":"FAIL: ")<<name<<"\n";if(!ok)++failures;}
static std::string narrow(const std::wstring& s){int n=WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);std::string r(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),r.data(),n,nullptr,nullptr);return r;}
static bool named(historyVectors::Archive& archive,double lat,double lon,const wchar_t* expected){auto hits=archive.hits(lat,lon);for(size_t i:hits)if(archive.entity(archive.record(i).entity).en.find(expected)!=std::wstring::npos)return true;std::cout<<"OBSERVED at "<<lat<<","<<lon<<" in "<<archive.year()<<":";for(size_t i:hits)std::cout<<" "<<narrow(archive.entity(archive.record(i).entity).en);std::cout<<"\n";return false;}
int wmain(int argc,wchar_t** argv){
 if(argc!=3)return 2;GdiplusStartupInput startup;ULONG_PTR token=0;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return 2;
 {
  historyVectors::Archive archive;
  auto start=std::chrono::steady_clock::now();check(archive.open(argv[1]),"original-coordinate history archive loads");
  std::cout<<"Archive load ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<"\n";
  check(archive.recordCount()==13800&&archive.geometryCount()==11317&&archive.pointCount()==3422262,"source records and six documented modern-reference supplements are present");
  check(HISTORY_YEAR_COUNT==5424&&HISTORY_YEARS[0]==2024&&HISTORY_YEARS[HISTORY_YEAR_COUNT-1]==-3400,"annual selection covers all nonzero years from 3400 BCE through 2024 CE");
  for(int i=0;i<HISTORY_YEAR_COUNT;i++)if(HISTORY_YEARS[i]==0||(i&&HISTORY_YEARS[i]>=HISTORY_YEARS[i-1])){check(false,"annual timeline is unique and descending");break;}
  check(!archive.selectYear(0)&&!archive.selectYear(-3401)&&!archive.selectYear(2025),"unsupported history years are rejected");
  bool annual=true;size_t annualSelections=0;auto annualStart=std::chrono::steady_clock::now();
  for(int year:HISTORY_YEARS){if(!archive.selectYear(year)){annual=false;break;}std::vector<size_t> expected;for(size_t i=0;i<archive.recordCount();++i){const auto& record=archive.record(i);if(!record.relation&&record.from<=year&&record.to>=year)expected.push_back(i);}auto actual=archive.active();std::sort(actual.begin(),actual.end());if(actual!=expected){annual=false;break;}++annualSelections;}
  check(annual&&annualSelections==5424,"all 5424 year selections match an independent inclusive-interval scan");
  std::cout<<"Annual scan ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-annualStart).count()<<"\n";
  for(int year:{-3400,-1000,1,476,1000,1200,1600,1800,1914,2024}){
   check(archive.selectYear(year),"requested source year selects successfully");bool polities=true;for(size_t i:archive.active())if(archive.record(i).relation)polities=false;check(polities,"all displayed records are polities rather than relationships");
  }
  archive.selectYear(1600);
  check(named(archive,39.9,116.4,L"Ming"),"Beijing has the source Ming polity in 1600");
  check(named(archive,48.85,2.35,L"France"),"Paris has the source French polity in 1600");
  check(named(archive,30.04,31.24,L"Ottoman"),"Cairo has the source Ottoman polity in 1600");
  check(named(archive,51.5,-.12,L"England"),"London has the source English polity in 1600");
  archive.selectYear(2024);
  check(named(archive,43.7384,7.4246,L"Monaco"),"Monaco is selectable using its polygon despite the coarse original grid");
  check(named(archive,47.14,9.52,L"Liechtenstein"),"Liechtenstein remains selectable as a small polity");
  auto micro=archive.hits(47.14,9.52);check(micro.size()==1&&archive.entity(archive.record(micro[0]).entity).sourceName.find(L"Natural Earth reference: ")==0,"modern-reference correction is identified and takes priority over the coarse source polygon");
  archive.selectYear(1600);auto oldHits=archive.hits(47.14,9.52);bool noProjection=true;for(size_t i:oldHits)if(archive.entity(archive.record(i).entity).sourceName.find(L"Natural Earth reference: ")==0)noProjection=false;check(noProjection,"modern microstate references are never back-projected to earlier years");archive.selectYear(2024);
  check(named(archive,64.7333,177.5,L"Russia"),"Russian geometry is preserved near the date line");
  check(archive.hits(0,-140).empty(),"uncovered open ocean remains unassigned");
  for(const auto& place:std::vector<historyVectors::XY>{{116.4,39.9},{2.35,48.85},{-.12,51.5},{177.5,64.7333}}){
   RECT map{0,0,1200,600};double cx=(place.x+180)/360,cy=(90-place.y)/180;Bitmap image(1200,600,PixelFormat32bppARGB);
   {Graphics gr(&image);gr.Clear(Color(0,0,0,0));archive.draw(gr,map,8,cx,cy,255,false);}Color pixel;image.GetPixel(600,300,&pixel);
   auto hits=archive.hits(place.y,place.x);bool same=false;for(size_t i:hits){unsigned rgb=archive.entity(archive.record(i).entity).rgb;if((pixel.GetValue()&0xffffff)==rgb&&pixel.GetA()==255)same=true;}
   check(!hits.empty()&&same,"zoomed rendering places the selected source polygon at the viewport center");
  }
  Bitmap overview(1200,600,PixelFormat32bppARGB);{Graphics gr(&overview);gr.Clear(Color(255,18,36,53));RECT map{0,0,1200,600};archive.draw(gr,map,1,.5,.5,205);}
  CLSID png{};UINT count=0,bytes=0;GetImageEncodersSize(&count,&bytes);std::vector<BYTE> storage(bytes);auto codecs=(ImageCodecInfo*)storage.data();GetImageEncoders(count,bytes,codecs);for(UINT i=0;i<count;i++)if(std::wcscmp(codecs[i].MimeType,L"image/png")==0)png=codecs[i].Clsid;
  check(overview.Save((std::wstring(argv[2])+L"\\vector-world-2024.png").c_str(),&png,nullptr)==Ok,"original-coordinate world overview is captured");
  const std::wstring invalid=std::wstring(argv[2])+L"\\invalid-history-header.wmh2";{std::ofstream out(invalid,std::ios::binary);std::string data(52,0);out.write(data.data(),data.size());}
  historyVectors::Archive invalidArchive;check(!invalidArchive.open(invalid),"invalid history pack is rejected safely");DeleteFileW(invalid.c_str());
  historyVectors::Archive fixture;const std::wstring fixturePath=std::wstring(argv[2])+L"\\vector-fixture.wmh2";
  check(fixture.open(fixturePath)&&fixture.selectYear(100),"independently assembled synthetic geometry fixture loads");
  check(fixture.hits(0,0).empty(),"polygon hole remains unassigned");
  check(fixture.hits(0,0,true).size()==1,"alliance polygon is available separately from territory");
  check(fixture.hits(0,2.1).size()==1,"outer polygon covers a point immediately outside the hole");
  auto overlap=fixture.hits(6,6);check(overlap.size()==2&&fixture.record(overlap.front()).entity==1,"overlapping point reports smaller polity first");
  check(fixture.hits(0,175).size()==1&&fixture.hits(0,-175).size()==1,"unwrapped date-line polygon hits on both sides of the world");
  check(fixture.hits(0,160).empty()&&fixture.hits(0,-160).empty(),"date-line polygon does not cover the opposite longitude arc");
  check(fixture.hits(0,181).empty()&&fixture.hits(91,0).empty(),"invalid query coordinates are rejected");
  for(int alpha:{255,128}){Bitmap image(300,900,PixelFormat32bppARGB);{Graphics g(&image);g.Clear(Color(0,0,0,0));RECT view{0,0,300,900};fixture.draw(g,view,4,.5,2./3.,(BYTE)alpha,false);}Color pixel;image.GetPixel(170,330,&pixel);check(std::abs((int)pixel.GetR()-(int)((fixture.entity(1).rgb>>16)&255))<=2&&std::abs((int)pixel.GetG()-(int)((fixture.entity(1).rgb>>8)&255))<=2&&std::abs((int)pixel.GetB()-(int)(fixture.entity(1).rgb&255))<=2&&std::abs((int)pixel.GetA()-alpha)<=1,"tall viewport preserves polygon geometry and includes added latitude extent");}
  {Bitmap image(1000,300,PixelFormat32bppARGB);{Graphics g(&image);g.Clear(Color(0,0,0,0));RECT view{0,0,1000,300};fixture.draw(g,view,4,150./360.,.5,255,false);}Color pixel;image.GetPixel(740,110,&pixel);check((pixel.GetValue()&0xffffff)==fixture.entity(1).rgb,"wide viewport uses the same angular scale for both axes");}
  {Bitmap image(1000,300,PixelFormat32bppARGB);{Graphics g(&image);g.Clear(Color(0,0,0,0));RECT view{0,0,1000,300};fixture.draw(g,view,1,.5,.5,255,false);}Color outside,inside;image.GetPixel(190,150,&outside);image.GetPixel(210,150,&inside);check(outside.GetA()==0&&inside.GetA()==255,"global wide view keeps date-line copies within the single Earth image");}
  {
   Bitmap synthetic(720,360,PixelFormat32bppARGB);RECT map{0,0,720,360};{Graphics graphics(&synthetic);graphics.Clear(Color(0,0,0,0));fixture.draw(graphics,map,1,.5,.5,128,false);}
   Color hole,edge,east,west,over;synthetic.GetPixel(360,180,&hole);synthetic.GetPixel(366,180,&edge);synthetic.GetPixel(710,180,&east);synthetic.GetPixel(10,180,&west);synthetic.GetPixel(372,168,&over);
   check(hole.GetA()==0&&edge.GetA()>=127&&edge.GetA()<=129,"rendered polygon hole stays transparent");
   check(over.GetA()>=127&&over.GetA()<=129,"layer opacity is applied once across overlapping polity polygons");
   check(east.GetA()>=127&&west.GetA()>=127,"date-line geometry is painted on both visible edges");
  }
  check(fixture.selectYear(150)&&!fixture.active().empty(),"source end year is inclusive");
  check(fixture.selectYear(151)&&fixture.active().empty()&&fixture.hits(6,6).empty(),"geometry disappears after its source interval");
  fixture.close();
  std::ifstream fixtureIn(fixturePath,std::ios::binary);std::string fixtureBytes((std::istreambuf_iterator<char>(fixtureIn)),{});fixtureIn.close();
  const std::wstring corrupt=std::wstring(argv[2])+L"\\corrupt-vector-fixture.wmh2";
  for(size_t length:{(size_t)51,fixtureBytes.size()-1}){{std::ofstream out(corrupt,std::ios::binary);out.write(fixtureBytes.data(),length);}historyVectors::Archive test;check(!test.open(corrupt),"truncated geometry archive is rejected");}
  DeleteFileW(corrupt.c_str());
 }
 GdiplusShutdown(token);std::cout<<"TOTAL: "<<checks-failures<<" passed, "<<failures<<" failed ("<<checks<<" checks)\n";return failures?1:0;
}
