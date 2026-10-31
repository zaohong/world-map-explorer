#define WORLD_MAP_GRID_LOGIC_TEST
#include "../native/world_map_explorer.cpp"
#include <iostream>

static int checks=0,failures=0;
static void check(bool condition,const char* name){checks++;if(!condition){failures++;std::cout<<"FAIL: "<<name<<"\n";}}

struct Sample{double lat,lon;const char* name;};
static int expectedLongitudeIndex(double value,int divisions){
  int i=(int)std::floor((value+180.0)/360.0*divisions);
  return std::max(0,std::min(divisions-1,i));
}
static int expectedLatitudeIndex(double value,int divisions){
  int i=(int)std::floor((90.0-value)/180.0*divisions);
  return std::max(0,std::min(divisions-1,i));
}
static std::wstring expectedId(int count,int row,int col){
  wchar_t id[48];
  if(count==9800)swprintf(id,48,L"Z9800-%03d-%03d",row,col);
  else swprintf(id,48,L"Z%d-%04d-%04d",count,row,col);
  return id;
}

int main(){
  const int counts[4]={9800,39200,156800,627200};
  const Sample samples[]={
    {10.1205,10.1205,"China-region positive lat/lon"},
    {39.9042,116.4074,"Beijing"},
    {-33.8688,151.2093,"Sydney"},
    {-23.5505,-46.6333,"Sao Paulo"},
    {64.1466,-21.9426,"Reykjavik"},
    {-45.0,-120.0,"southern Pacific"},
    {80.0,179.9,"high latitude near date line"},
    {-1.0,179.999,"south of equator near date line"},
    {0.0,0.0,"equator and prime meridian"},
    {90.0,-180.0,"north-west global edge"},
    {-90.0,180.0,"south-east global edge"}
  };
  for(int level=0;level<4;level++){
    gGridLevel=level;
    const int scale=1<<level,rows=70*scale,columns=140*scale;
    check(activeGridCount()==counts[level],"grid level total");
    check(activeGridRows()==rows&&activeGridColumns()==columns,"grid dimensions");
    check(gridLevelForCount(counts[level])==level,"grid level parser");
    for(const Sample& s:samples){
      int row=-1,col=-1;
      fineGridCellForCoordinate(s.lat,s.lon,row,col);
      const int wantRow=expectedLatitudeIndex(s.lat,rows);
      const int wantCol=expectedLongitudeIndex(s.lon,columns);
      std::string label=std::string("sample mapping: ")+s.name;
      check(row==wantRow&&col==wantCol,label.c_str());
      check(gridIdText(counts[level],row,col)==expectedId(counts[level],wantRow,wantCol),"sample fine-cell ID");
      check(row/scale==expectedLatitudeIndex(s.lat,70)&&col/scale==expectedLongitudeIndex(s.lon,140),"sample Z9800 parent ownership");
      const double north=90.0-(double)row*180.0/rows;
      const double south=90.0-(double)(row+1)*180.0/rows;
      const double west=-180.0+(double)col*360.0/columns;
      const double east=-180.0+(double)(col+1)*360.0/columns;
      check(s.lat<=north+1e-10&&s.lat>=south-1e-10&&s.lon>=west-1e-10&&s.lon<=east+1e-10,"sample lies inside computed cell bounds");
      check(std::fabs((north-south)-180.0/rows)<1e-12&&std::fabs((east-west)-360.0/columns)<1e-12,"sample cell bounds have expected angular span");
    }
    int seamRow=-1,seamCol=-1;
    fineGridCellForCoordinate(45.0,-90.0,seamRow,seamCol);
    check(seamRow==rows/4&&seamCol==columns/4,"exact internal seam belongs to south-east cell");
    int topRow=-1,topCol=-1,bottomRow=-1,bottomCol=-1;
    fineGridCellForCoordinate(90.0,-180.0,topRow,topCol);
    fineGridCellForCoordinate(-90.0,180.0,bottomRow,bottomCol);
    check(topRow==0&&topCol==0&&bottomRow==rows-1&&bottomCol==columns-1,"all world edges clamp to valid cells");
  }

  gGridLevel=0;
  int row=-1,col=-1,matches=0;std::vector<SearchMatch> found;
  check(locateQuery(L"Z627200-0248-0591",row,col,matches,nullptr,&found),"fine cell ID can be searched");
  check(matches==1&&row==31&&col==73&&found.size()==1&&found[0].gridLevel==3,"fine search resolves its Z9800 archive parent");
  selectSearchResultOnMap(found[0]);
  check(gGridLevel==3&&gRow==31&&gCol==73,"fine search selects child grid and retains parent data cell");
  check(gSelectedCoordinateIsFineGridCenter&&!gSelectedCoordinateFromInput,"fine-cell ID is identified as a child-cell-center location");
  check(std::fabs(gSelectedLatitude-(90.0-(248.5*180.0/560.0)))<1e-12&&std::fabs(gSelectedLongitude-(-180.0+(591.5*360.0/1120.0)))<1e-12,"fine search centers on the requested child cell");
  gGridLevel=0;MapSceneCacheKey oldKey=currentMapSceneCacheKey(1360,820);gGridLevel=3;MapSceneCacheKey newKey=currentMapSceneCacheKey(1360,820);
  check(!sameMapSceneCacheKey(oldKey,newKey),"grid change invalidates rendered map cache");
  check(gridLevelForCount(9800)==0&&gridLevelForCount(39200)==1&&gridLevelForCount(156800)==2&&gridLevelForCount(627200)==3,"all four menu choices decode consistently");
  std::wstring modulePath=appDir()+L"\\..\\..\\..\\WorldMapExplorer-1.22.0\\data\\world-2026.json";DWORD moduleError=0;
  check(loadTextFile(modulePath,gWorld2026Archive,moduleError),"World 2026 standalone data module loads from the release folder");
  if(!gWorld2026Archive.empty()){
    check(buildWorld2026Index(),"World 2026 module validates its 560 x 1,120 cell grid and country table");
    check(gWorld2026Countries.size()==258&&gWorld2026CellCountries.size()==627200,"World 2026 contains 258 country or area records and all 627,200 child cells");
    int recordCount=0,fallbackCount=0,occupiedCount=0;
    check(jsonIntegerField(gWorld2026Archive,0,gWorld2026Archive.size(),"countryRecords",recordCount)&&recordCount==258,"World 2026 record count matches module metadata");
    check(jsonIntegerField(gWorld2026Archive,0,gWorld2026Archive.size(),"representativePointFallbacks",fallbackCount)&&fallbackCount==57,"sub-cell country fallback count is documented in module metadata");
    check(jsonIntegerField(gWorld2026Archive,0,gWorld2026Archive.size(),"countriesWithCells",occupiedCount)&&occupiedCount==258,"country-cell coverage statistics include representative cells for sub-cell areas");
    struct CountrySample{double lat,lon;const char* code;};
    const CountrySample countrySamples[]={{39.9042,116.4074,"CHN"},{48.8566,2.3522,"FRA"},{51.5072,-0.1276,"GBR"},{35.6762,139.6503,"JPN"},{38.9072,-77.0369,"USA"},{28.6139,77.2090,"IND"},{-15.7939,-47.8828,"BRA"},{30.0444,31.2357,"EGY"},{55.7558,37.6173,"RUS"},{-35.2809,149.1300,"AUS"},{-33.9249,18.4241,"ZAF"},{19.4326,-99.1332,"MEX"}};
    for(const auto& sample:countrySamples){int fineRow=0,fineCol=0;world2026CellForCoordinate(sample.lat,sample.lon,fineRow,fineCol);unsigned short id=gWorld2026CellCountries[(size_t)fineRow*FINE_GRID_COLUMNS+fineCol];std::string actual=id>0&&id<=gWorld2026Countries.size()?gWorld2026Countries[id-1].code:"OCEAN";check(actual==sample.code,"World 2026 sample country assignment");}
    auto china=std::find_if(gWorld2026Countries.begin(),gWorld2026Countries.end(),[](const World2026Country& country){return country.code=="CHN";});
    auto france=std::find_if(gWorld2026Countries.begin(),gWorld2026Countries.end(),[](const World2026Country& country){return country.code=="FRA";});
    check(china!=gWorld2026Countries.end()&&china->nameZh==L"中华人民共和国","World 2026 preserves the Chinese name for China");
    check(france!=gWorld2026Countries.end()&&france->nameZh==L"法国","World 2026 preserves the Chinese name for France");
    int oceanRow=0,oceanCol=0;world2026CellForCoordinate(-45.0,-120.0,oceanRow,oceanCol);check(gWorld2026CellCountries[(size_t)oceanRow*FINE_GRID_COLUMNS+oceanCol]==0,"World 2026 keeps a South Pacific ocean cell transparent");
    GdiplusStartupInput gdiplusInput{};ULONG_PTR gdiplusToken=0;Status gdiplusStatus=GdiplusStartup(&gdiplusToken,&gdiplusInput,nullptr);check(gdiplusStatus==Ok,"GDI+ starts for the World 2026 raster check");
    if(gdiplusStatus==Ok){check(buildWorld2026Raster()&&gWorld2026Raster!=nullptr,"World 2026 cell assignments build a 1,120 x 560 display raster");delete gWorld2026Raster;gWorld2026Raster=nullptr;GdiplusShutdown(gdiplusToken);}
  }
  std::cout<<"SAMPLES: "<<(sizeof(samples)/sizeof(samples[0]))<<" geographic points x 4 grid levels, plus seam and edge cases\n";
  std::cout<<"TOTAL: "<<(checks-failures)<<" passed, "<<failures<<" failed ("<<checks<<" checks)\n";
  return failures?1:0;
}
