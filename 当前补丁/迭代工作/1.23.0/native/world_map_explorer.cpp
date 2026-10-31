#define NOMINMAX
#include <algorithm>
using std::max;
using std::min;
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cctype>
#include <cwctype>
#include <cmath>
#include <vector>
#include "history_years.generated.h"
using namespace Gdiplus;

static const int GRID_COLUMNS=140,GRID_ROWS=70,GRID_VERSION=9800,LEGACY_COLUMNS=100,LEGACY_ROWS=50,FINE_GRID_COLUMNS=1120,FINE_GRID_ROWS=560;
static const wchar_t* APP_VERSION=L"1.23.0";
static const int POPULATION_YEAR=2023;
static const double GRID_CELL_DEGREES=180.0/GRID_ROWS;
static ULONG_PTR gToken; static Image* gMap=nullptr; static Image* gMapOriginal=nullptr; static Image* gMapHighRes=nullptr; static Image* gMapNasa=nullptr; static Bitmap* gLandCoverRaster=nullptr; static Bitmap* gPopulationRaster=nullptr; static Bitmap* gWorld2026Raster=nullptr; static Bitmap* gHistoryRaster=nullptr; static RECT gMapRect{},gInfoRect{}; static int gCol=-1,gRow=-1,gGridLevel=0; static double gSelectedLatitude=0,gSelectedLongitude=0;static bool gSelectedCoordinateExact=false,gSelectedCoordinateFromInput=false,gSelectedCoordinateIsFineGridCenter=false;static std::string gArchive,gLandCoverArchive,gHeritageArchive,gPopulationArchive,gWorld2026Archive,gHistoryArchive; static bool gArchiveLoaded=false,gLandCoverLoaded=false,gHeritageLoaded=false,gPopulationLoaded=false,gWorld2026Loaded=false,gHistoryLoaded=false,gShowPopulation=false,gShowWorld2026=false,gShowHistory=false,gNasaMapAvailable=false,gNasaDetailAvailable=false,gNasa1kmAvailable=false,gNasa500mAvailable=false; static DWORD gArchiveError=0,gLandCoverError=0,gHeritageError=0,gPopulationError=0,gWorld2026Error=0,gHistoryError=0; static int gScrollOffset=0,gContentHeight=0,gViewportHeight=0,gFillOpacity=72,gHistoryYear=2024;
static int gHistoryRasterYear=INT_MIN;
static Bitmap* gHistoryTransitionPreviousRaster=nullptr;static bool gHistoryAnimationEnabled=false,gHistoryTransitionActive=false;static DWORD gHistoryTransitionStart=0;static int gHistoryTransitionDurationMs=700;
static int gHistoryEventCycleHeight=0;static bool gHistoryEventFractionPending=false;static double gHistoryEventScrollFraction=0;static DWORD gHistoryScrollPauseUntil=0;static bool gHistoryEventRelayMode=false;static std::vector<size_t> gHistoryRelayEvents;static std::vector<size_t> gHistoryRangeEventCache;static int gHistoryCacheRangeStart=INT_MIN,gHistoryCacheRangeEnd=INT_MIN;static std::vector<int> gHistoryEventDescriptionHeights;static int gHistoryEventDescriptionWidth=0;
static HBRUSH gUiControlBrush=nullptr,gUiPanelBrush=nullptr;
static const UINT MAP_VIEW_SETTLE_TIMER=1024;
struct ArchiveRange{size_t begin,end;ArchiveRange():begin(std::string::npos),end(std::string::npos){}};static std::vector<ArchiveRange> gZoneRanges,gLandCoverRanges,gHeritageRanges,gPopulationRanges;
struct UiFontEntry{int size;bool bold;HFONT handle;};static std::vector<UiFontEntry> gUiFonts;
static HDC gBackBufferDC=nullptr;static HBITMAP gBackBufferBitmap=nullptr,gBackBufferOriginalBitmap=nullptr;static int gBackBufferWidth=0,gBackBufferHeight=0;
struct MapSceneCacheKey{int width,height;RECT mapRect;Image* map;Bitmap* landCover;Bitmap* population;Bitmap* world2026;Bitmap* history;double zoom,centerX,centerY,selectedLatitude,selectedLongitude;int row,col,gridLevel,fillOpacity,historyYear;bool selectedExact,showControlPoints,showLandCover,showPopulation,showWorld2026,showHistory,showGridLines,nasaDetail,nasa1km,nasa500m;};
static Bitmap* gMapSceneCache=nullptr;static MapSceneCacheKey gMapSceneCacheKey{};static bool gMapSceneCacheValid=false,gMapViewPreviewActive=false;
struct MapBaseCacheKey{int width,height;RECT mapRect;Image* map;double zoom,centerX,centerY;bool nasaDetail,nasa1km,nasa500m;};static Bitmap* gMapBaseCache=nullptr;static MapBaseCacheKey gMapBaseCacheKey{};static bool gMapBaseCacheValid=false;
struct SearchMatch{int row,col,gridLevel;double latitude,longitude;bool hasExactCoordinate;SearchMatch(int r,int c):row(r),col(c),gridLevel(-1),latitude(0),longitude(0),hasExactCoordinate(false){}SearchMatch(int r,int c,double lat,double lon):row(r),col(c),gridLevel(-1),latitude(lat),longitude(lon),hasExactCoordinate(true){}SearchMatch(int r,int c,double lat,double lon,int level):row(r),col(c),gridLevel(level),latitude(lat),longitude(lon),hasExactCoordinate(true){}};static std::vector<SearchMatch> gSearchMatches;static std::wstring gLastSearchQuery;static size_t gSearchMatchIndex=0;static bool gSearchWasLegacy=false;
struct NasaDetailTile{int row,col;Image* image;DWORD lastUsed;NasaDetailTile(int r,int c,Image* i,DWORD t):row(r),col(c),image(i),lastUsed(t){}};static std::vector<NasaDetailTile> gNasaDetailTiles;static DWORD gNasaTileUseCounter=0;static int gNasaTileLoadFailures=0;static std::wstring gNasaDetailFolder;static int gNasaDetailPixels=1200,gNasaDetailCacheLimit=24,gNasaDetailResolutionMeters=1000;
struct LandCoverClass{int code;std::wstring name;unsigned int rgb;};struct World2026Country{std::string code;std::wstring name,nameZh;int mapColor;unsigned int rgb;};struct HistoryPolity{int id;std::wstring name;std::string color;unsigned int rgb;};static std::vector<LandCoverClass> gLandCoverLegend;static std::vector<int> gLandCoverZoneCodes;static std::vector<unsigned int> gLandCoverZoneRgb;static std::vector<World2026Country> gWorld2026Countries;static std::vector<unsigned short> gWorld2026CellCountries;static std::vector<HistoryPolity> gHistoryPolities;static std::vector<unsigned short> gHistoryCellPolities;
struct HistoryEvent{int year;std::wstring category,place,title,description,source;};static std::vector<HistoryEvent> gHistoryEvents;static std::string gHistoryEventsText;static bool gHistoryEventsLoaded=false;static DWORD gHistoryEventsError=0;static int gInfoTab=0,gHistoryEventRangeStart=-3401,gHistoryEventRangeEnd=-3400,gHoveredButtonId=0;
static HWND gControlPointsButton=nullptr,gSearchEdit=nullptr,gSearchButton=nullptr,gHighResMapButton=nullptr,gGridLevelCombo=nullptr,gNasaMapButton=nullptr,gLandCoverToggleButton=nullptr,gPopulationToggleButton=nullptr,gWorld2026ToggleButton=nullptr,gHistoryToggleButton=nullptr,gHistoryYearCombo=nullptr,gOpacityTrackbar=nullptr,gOpacityLabel=nullptr,gGridLinesToggleButton=nullptr,gHistoryPlaybackButton=nullptr,gHistorySpeedButton=nullptr,gHistoryAnimationButton=nullptr,gGeoTabButton=nullptr,gEventsTabButton=nullptr,gFullscreenButton=nullptr; static bool gShowControlPoints=true,gShowLandCover=false,gPointerStatusActive=false,gTrackingMouseLeave=false,gShowGridLines=true,gHistoryPlaying=false,gHistoryPaused=false,gMapFullscreen=false;static LONG_PTR gWindowedStyle=0;static WINDOWPLACEMENT gWindowedPlacement{sizeof(WINDOWPLACEMENT)};static int gHistorySpeedIndex=0;static const UINT HISTORY_PLAYBACK_TIMER=1017,HISTORY_TRANSITION_TIMER=1021,HISTORY_EVENT_SCROLL_TIMER=1022;static const int HISTORY_PLAYBACK_INTERVALS[]={500,250,125,1000};static const int HISTORY_ANIMATED_PLAYBACK_INTERVALS[]={1100,550,275,2200};static const wchar_t* HISTORY_PLAYBACK_SPEEDS[]={L"速度 1×",L"速度 2×",L"速度 4×",L"速度 0.5×"};static RECT gLandCoverLegendRect{},gPopulationLegendRect{},gWorld2026LegendRect{},gHistoryLegendRect{};static std::vector<double> gPopulationCounts,gPopulationDensity,gPopulationCoverage;static std::vector<unsigned char> gPopulationAvailable;static double gPopulationMaxDensity=0; static std::wstring gNotice=L"地图操作：滚轮缩放 · 拖动平移 · 双击复位";
static double gMapZoom=1.0,gMapCenterX=0.5,gMapCenterY=0.5; static bool gMapDragging=false,gMapDragged=false; static POINT gDragStart{},gLastMouse{};
static int activeGridScale(){return 1<<std::max(0,std::min(3,gGridLevel));}
static int activeGridRows(){return GRID_ROWS*activeGridScale();}
static int activeGridColumns(){return GRID_COLUMNS*activeGridScale();}
static int activeGridCount(){return GRID_VERSION<<(2*std::max(0,std::min(3,gGridLevel)));}
static int gridLevelForCount(int count){for(int level=0;level<=3;level++)if((GRID_VERSION<<(2*level))==count)return level;return -1;}
static std::wstring gridIdText(int count,int row,int col){wchar_t id[48];if(count==GRID_VERSION)swprintf(id,48,L"Z%d-%03d-%03d",count,row,col);else swprintf(id,48,L"Z%d-%04d-%04d",count,row,col);return id;}
static void fineGridCellForWorld(double worldX,double worldY,int& row,int& col){int rows=activeGridRows(),columns=activeGridColumns();row=std::min(rows-1,std::max(0,(int)std::floor(worldY*rows)));col=std::min(columns-1,std::max(0,(int)std::floor(worldX*columns)));}
static void fineGridCellForCoordinate(double latitude,double longitude,int& row,int& col){fineGridCellForWorld((longitude+180.0)/360.0,(90.0-latitude)/180.0,row,col);}
static void world2026CellForCoordinate(double latitude,double longitude,int& row,int& col){double worldX=(longitude+180.0)/360.0,worldY=(90.0-latitude)/180.0;row=std::min(FINE_GRID_ROWS-1,std::max(0,(int)std::floor(worldY*FINE_GRID_ROWS)));col=std::min(FINE_GRID_COLUMNS-1,std::max(0,(int)std::floor(worldX*FINE_GRID_COLUMNS)));}
static bool selectedFineGridCell(int& row,int& col){if(gCol<0)return false;fineGridCellForCoordinate(gSelectedLatitude,gSelectedLongitude,row,col);return true;}
static std::wstring appDir(){ wchar_t p[MAX_PATH]; GetModuleFileNameW(nullptr,p,MAX_PATH); std::wstring s=p; return s.substr(0,s.find_last_of(L"\\/")); }
static std::wstring utf8(const std::string& s){if(s.empty())return L"";int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0);std::wstring w(n,L' ');MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),&w[0],n);return w;}
static std::string toUtf8(const std::wstring& s){if(s.empty())return "";int n=WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);std::string v(n,' ');WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),&v[0],n,nullptr,nullptr);return v;}
static bool containsRange(const std::string& s,const std::string& q,size_t begin,size_t end){if(q.empty()||begin>=end||q.size()>end-begin)return false;size_t last=end-q.size();for(size_t i=begin;i<=last;i++)if(s[i]==q[0]&&s.compare(i,q.size(),q)==0)return true;return false;}
static unsigned utf8PointAt(const std::string& s,size_t p){unsigned char c=(unsigned char)s[p];if(c<0x80)return c;if((c&0xe0)==0xc0&&p+1<s.size())return ((c&0x1f)<<6)|((unsigned char)s[p+1]&0x3f);if((c&0xf0)==0xe0&&p+2<s.size())return ((c&0x0f)<<12)|(((unsigned char)s[p+1]&0x3f)<<6)|((unsigned char)s[p+2]&0x3f);if((c&0xf8)==0xf0&&p+3<s.size())return ((c&7)<<18)|(((unsigned char)s[p+1]&0x3f)<<12)|(((unsigned char)s[p+2]&0x3f)<<6)|((unsigned char)s[p+3]&0x3f);return c;}
static unsigned utf8PointBefore(const std::string& s,size_t p){if(p==0)return 0;size_t i=p-1;while(i>0&&(((unsigned char)s[i]&0xc0)==0x80))i--;return utf8PointAt(s,i);}
static bool wordPoint(unsigned cp){if(cp<128)return (cp>='a'&&cp<='z')||(cp>='A'&&cp<='Z')||(cp>='0'&&cp<='9')||cp=='_';return (cp>=0x2e80&&cp<=0x9fff)||(cp>=0xf900&&cp<=0xfaff)||(cp>=0x3040&&cp<=0x30ff)||(cp>=0xac00&&cp<=0xd7af)||(cp>=0xff10&&cp<=0xff19)||(cp>=0xff21&&cp<=0xff3a)||(cp>=0xff41&&cp<=0xff5a);}
static bool exactNameRange(const std::string& s,const std::string& q,size_t begin,size_t end){if(q.empty()||begin>=end||q.size()>end-begin)return false;size_t last=end-q.size();for(size_t i=begin;i<=last;i++){if(s[i]!=q[0]||s.compare(i,q.size(),q)!=0)continue;size_t after=i+q.size();unsigned left=utf8PointBefore(s,i),right=after<s.size()?utf8PointAt(s,after):0;if(!wordPoint(left)&&!wordPoint(right))return true;}return false;}
static size_t jsonStringEnd(const std::string& s,size_t begin,size_t limit){for(size_t i=begin;i<limit;i++){if(s[i]=='\\'){i++;continue;}if(s[i]=='\"')return i;}return limit;}
static bool loadTextFile(const std::wstring& path,std::string& output,DWORD& error){HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE){error=GetLastError();return false;}LARGE_INTEGER size;if(!GetFileSizeEx(f,&size)||size.QuadPart<=0||size.QuadPart>64*1024*1024){error=ERROR_FILE_TOO_LARGE;CloseHandle(f);return false;}output.resize((size_t)size.QuadPart);DWORD got=0;BOOL ok=ReadFile(f,&output[0],(DWORD)output.size(),&got,nullptr);if(!ok||got!=(DWORD)output.size()){error=ok?ERROR_HANDLE_EOF:GetLastError();output.clear();CloseHandle(f);return false;}CloseHandle(f);error=ERROR_SUCCESS;return true;}
static bool loadArchiveFile(const std::wstring& path){return loadTextFile(path,gArchive,gArchiveError);}
static bool parseWorldFileBounds(const std::string& content,int width,int height,double& west,double& east,double& north,double& south){
  if(width<=0||height<=0)return false;
  std::stringstream values(content);double a=0,d=0,b=0,e=0,c=0,f=0;
  if(!(values>>a>>d>>b>>e>>c>>f)||!std::isfinite(a)||!std::isfinite(d)||!std::isfinite(b)||!std::isfinite(e)||!std::isfinite(c)||!std::isfinite(f))return false;
  if(a<=0||e>=0||std::fabs(d)>1e-12||std::fabs(b)>1e-12)return false;
  west=c-a/2;east=c+a*(width-.5);north=f-e/2;south=f+e*(height-.5);
  return std::isfinite(west)&&std::isfinite(east)&&std::isfinite(north)&&std::isfinite(south)&&east>west&&north>south;
}
static bool parseGlobalWorldFile(const std::string& content,int width,int height){double west=0,east=0,north=0,south=0,tol=1e-6;return parseWorldFileBounds(content,width,height,west,east,north,south)&&std::fabs(west+180)<tol&&std::fabs(east-180)<tol&&std::fabs(north-90)<tol&&std::fabs(south+90)<tol;}
static bool readSmallTextFile(const std::wstring& path,std::string& content){HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER size;if(!GetFileSizeEx(f,&size)||size.QuadPart<=0||size.QuadPart>4096){CloseHandle(f);return false;}content.resize((size_t)size.QuadPart);DWORD got=0;BOOL ok=ReadFile(f,&content[0],(DWORD)content.size(),&got,nullptr);CloseHandle(f);if(!ok||got!=(DWORD)content.size()){content.clear();return false;}return true;}
static bool validateWorldMapMetadata(const std::wstring& base,int width,int height,double expectedWest,double expectedEast,double expectedNorth,double expectedSouth){std::string world,projection;double west=0,east=0,north=0,south=0,tol=1e-6;return readSmallTextFile(base+L".jgw",world)&&parseWorldFileBounds(world,width,height,west,east,north,south)&&std::fabs(west-expectedWest)<tol&&std::fabs(east-expectedEast)<tol&&std::fabs(north-expectedNorth)<tol&&std::fabs(south-expectedSouth)<tol&&readSmallTextFile(base+L".prj",projection)&&projection.find("GCS_WGS_1984")!=std::string::npos;}
static bool validateGlobalMapMetadata(const std::wstring& base,int width,int height){return validateWorldMapMetadata(base,width,height,-180,180,90,-90);}
static void clearNasaDetailTileCache(){for(auto& tile:gNasaDetailTiles)delete tile.image;gNasaDetailTiles.clear();gNasaTileUseCounter=0;}
static size_t buildZoneIndex(const std::string& archive,std::vector<ArchiveRange>& ranges){
  ranges.assign(GRID_ROWS*GRID_COLUMNS,ArchiveRange());
  size_t zones=archive.find("\"zones\"");if(zones==std::string::npos)return 0;
  size_t array=archive.find('[',zones);if(array==std::string::npos)return 0;
  const std::string key="\"zoneId\"";size_t pos=archive.find(key,array),indexed=0;
  while(pos!=std::string::npos){
    size_t value=pos+key.size();while(value<archive.size()&&isspace((unsigned char)archive[value]))value++;
    if(value>=archive.size()||archive[value++]!=':')return 0;
    while(value<archive.size()&&isspace((unsigned char)archive[value]))value++;
    if(value>=archive.size()||archive[value++]!='\"')return 0;
    size_t idEnd=archive.find('\"',value);if(idEnd==std::string::npos)return 0;
    size_t next=archive.find(key,idEnd+1),end=next==std::string::npos?archive.size():next;
    int version=0,row=-1,col=-1;std::string id=archive.substr(value,idEnd-value);
    if(sscanf(id.c_str(),"Z%d-%d-%d",&version,&row,&col)==3&&version==GRID_VERSION&&row>=0&&row<GRID_ROWS&&col>=0&&col<GRID_COLUMNS){
      ArchiveRange&range=ranges[(size_t)row*GRID_COLUMNS+col];if(range.begin!=std::string::npos)return 0;range.begin=pos;range.end=end;indexed++;
    }
    if(next==std::string::npos)break;pos=next;
  }
  return indexed;
}
static size_t buildArchiveIndex(){return buildZoneIndex(gArchive,gZoneRanges);}
static void appendUtf8(std::string& out,unsigned cp){if(cp<=0x7f)out.push_back((char)cp);else if(cp<=0x7ff){out.push_back((char)(0xc0|(cp>>6)));out.push_back((char)(0x80|(cp&0x3f)));}else if(cp<=0xffff){out.push_back((char)(0xe0|(cp>>12)));out.push_back((char)(0x80|((cp>>6)&0x3f)));out.push_back((char)(0x80|(cp&0x3f)));}else{out.push_back((char)(0xf0|(cp>>18)));out.push_back((char)(0x80|((cp>>12)&0x3f)));out.push_back((char)(0x80|((cp>>6)&0x3f)));out.push_back((char)(0x80|(cp&0x3f)));}}
static unsigned readHex4(const std::string& s,size_t p){unsigned value=0;for(int i=0;i<4;i++){char c=s[p+i];value<<=4;if(c>='0'&&c<='9')value|=c-'0';else if(c>='a'&&c<='f')value|=c-'a'+10;else if(c>='A'&&c<='F')value|=c-'A'+10;else return 0xfffd;}return value;}
static bool jsonStringField(const std::string& archive,size_t begin,size_t limit,const char* key,std::string& value){
  std::string needle=std::string("\"")+key+"\"";size_t p=archive.find(needle,begin);if(p==std::string::npos||p>=limit)return false;p+=needle.size();
  while(p<limit&&isspace((unsigned char)archive[p]))p++;if(p>=limit||archive[p++]!=':')return false;while(p<limit&&isspace((unsigned char)archive[p]))p++;if(p>=limit||archive[p++]!='\"')return false;
  value.clear();for(;p<limit;p++){unsigned char c=(unsigned char)archive[p];if(c=='\"')return true;if(c!='\\'){value.push_back((char)c);continue;}if(++p>=limit)return false;char e=archive[p];
    if(e=='\"'||e=='\\'||e=='/')value.push_back(e);else if(e=='n')value.push_back('\n');else if(e=='r')value.push_back('\r');else if(e=='t')value.push_back('\t');else if(e=='b')value.push_back('\b');else if(e=='f')value.push_back('\f');
    else if(e=='u'&&p+4<limit){unsigned cp=readHex4(archive,p+1);p+=4;if(cp>=0xd800&&cp<=0xdbff&&p+6<limit&&archive[p+1]=='\\'&&archive[p+2]=='u'){unsigned low=readHex4(archive,p+3);if(low>=0xdc00&&low<=0xdfff){cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);p+=6;}}appendUtf8(value,cp);}else value.push_back(e);
  }return false;
}
static bool jsonIntegerField(const std::string& archive,size_t begin,size_t limit,const char* key,int& value){
  std::string needle=std::string("\"")+key+"\"";size_t p=archive.find(needle,begin);if(p==std::string::npos||p>=limit)return false;p+=needle.size();
  while(p<limit&&isspace((unsigned char)archive[p]))p++;if(p>=limit||archive[p++]!=':')return false;while(p<limit&&isspace((unsigned char)archive[p]))p++;if(p>=limit)return false;
  const char* first=archive.c_str()+p;char* end=nullptr;long parsed=strtol(first,&end,10);if(end==first||parsed<INT_MIN||parsed>INT_MAX||(size_t)(end-archive.c_str())>limit)return false;value=(int)parsed;return true;
}
static bool jsonDoubleField(const std::string& archive,size_t begin,size_t limit,const char* key,double& value){
  std::string needle=std::string("\"")+key+"\"";size_t p=archive.find(needle,begin);if(p==std::string::npos||p>=limit)return false;p+=needle.size();
  while(p<limit&&isspace((unsigned char)archive[p]))p++;if(p>=limit||archive[p++]!=':')return false;while(p<limit&&isspace((unsigned char)archive[p]))p++;if(p>=limit)return false;
  const char* first=archive.c_str()+p;char* end=nullptr;double parsed=strtod(first,&end);if(end==first||!std::isfinite(parsed)||(size_t)(end-archive.c_str())>limit)return false;value=parsed;return true;
}
static size_t jsonArrayEnd(const std::string& archive,size_t begin){if(begin>=archive.size()||archive[begin]!='[')return std::string::npos;int depth=0;bool quoted=false,escaped=false;for(size_t p=begin;p<archive.size();p++){char c=archive[p];if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;continue;}if(c=='"'){quoted=true;continue;}if(c=='[')depth++;else if(c==']'&&--depth==0)return p;}return std::string::npos;}
static bool validateNasaDetailManifest(const std::wstring& folder,int expectedPixels,int expectedArcSeconds){
  std::string manifest;if(!readSmallTextFile(appDir()+L"\\assets\\"+folder+L"\\manifest.json",manifest))return false;
  int schema=0,degrees=0,width=0,height=0,count=0,resolution=0;
  return jsonIntegerField(manifest,0,manifest.size(),"schemaVersion",schema)&&schema==1
    &&jsonIntegerField(manifest,0,manifest.size(),"tileDegrees",degrees)&&degrees==10
    &&jsonIntegerField(manifest,0,manifest.size(),"tileWidthPixels",width)&&width==expectedPixels
    &&jsonIntegerField(manifest,0,manifest.size(),"tileHeightPixels",height)&&height==expectedPixels
    &&jsonIntegerField(manifest,0,manifest.size(),"tileCount",count)&&count==648
    &&jsonIntegerField(manifest,0,manifest.size(),"outputResolutionArcSeconds",resolution)&&resolution==expectedArcSeconds;
}
static std::wstring archiveField(int row,int col,const char* key,const std::wstring& fallback){if(!gArchiveLoaded)return L"档案未加载";char id[16];sprintf(id,"Z%02d-%03d",row,col);size_t p=gArchive.find(id);if(p==std::string::npos)return fallback;std::string needle=std::string("\"")+key+"\": \"";p=gArchive.find(needle,p);if(p==std::string::npos)return fallback;p+=needle.size();std::string value;for(;p<gArchive.size();p++){unsigned char c=(unsigned char)gArchive[p];if(c=='"')return utf8(value);if(c!='\\'){value.push_back((char)c);continue;}if(++p>=gArchive.size())break;char e=gArchive[p];if(e=='"'||e=='\\'||e=='/')value.push_back(e);else if(e=='n')value.push_back('\n');else if(e=='r')value.push_back('\r');else if(e=='t')value.push_back('\t');else if(e=='b')value.push_back('\b');else if(e=='f')value.push_back('\f');else if(e=='u'&&p+4<gArchive.size()){unsigned cp=readHex4(gArchive,p+1);p+=4;if(cp>=0xd800&&cp<=0xdbff&&p+6<gArchive.size()&&gArchive[p+1]=='\\'&&gArchive[p+2]=='u'){unsigned low=readHex4(gArchive,p+3);if(low>=0xdc00&&low<=0xdfff){cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);p+=6;}}appendUtf8(value,cp);}else value.push_back(e);}return fallback;}
static std::wstring zoneField(const std::string& archive,const std::vector<ArchiveRange>& ranges,int row,int col,const char* key,const std::wstring& fallback){
  if(row<0||row>=GRID_ROWS||col<0||col>=GRID_COLUMNS)return fallback;
  size_t index=(size_t)row*GRID_COLUMNS+col;
  if(index>=ranges.size()||ranges[index].begin==std::string::npos)return fallback;
  std::string value;size_t record=ranges[index].begin,recordEnd=ranges[index].end;
  return jsonStringField(archive,record,recordEnd,key,value)?utf8(value):fallback;
}
static bool buildLandCoverLegend(){
  gLandCoverLegend.clear();size_t key=gLandCoverArchive.find("\"classes\"");if(key==std::string::npos)return false;size_t array=gLandCoverArchive.find('[',key),arrayEnd=array==std::string::npos?std::string::npos:gLandCoverArchive.find(']',array);if(array==std::string::npos||arrayEnd==std::string::npos)return false;
  size_t object=gLandCoverArchive.find('{',array);while(object!=std::string::npos&&object<arrayEnd){size_t objectEnd=gLandCoverArchive.find('}',object);if(objectEnd==std::string::npos||objectEnd>arrayEnd)return false;int code=0,rgb=0;std::string name;
    if(!jsonIntegerField(gLandCoverArchive,object,objectEnd,"code",code)||!jsonIntegerField(gLandCoverArchive,object,objectEnd,"rgb",rgb)||!jsonStringField(gLandCoverArchive,object,objectEnd,"name",name)||code<=0||rgb<0||rgb>0xffffff||name.empty())return false;
    for(const auto& item:gLandCoverLegend)if(item.code==code)return false;gLandCoverLegend.push_back(LandCoverClass{code,utf8(name),(unsigned int)rgb});object=gLandCoverArchive.find('{',objectEnd+1);
  }
  return gLandCoverLegend.size()==22;
}
static size_t buildLandCoverIndex(){
  size_t indexed=buildZoneIndex(gLandCoverArchive,gLandCoverRanges);if(indexed!=(size_t)(GRID_ROWS*GRID_COLUMNS))return indexed;
  int schema=0;if(!jsonIntegerField(gLandCoverArchive,0,gLandCoverArchive.size(),"schemaVersion",schema)||schema!=2||!buildLandCoverLegend())return 0;
  gLandCoverZoneCodes.assign(indexed,0);gLandCoverZoneRgb.assign(indexed,0);
  for(size_t i=0;i<indexed;i++){const ArchiveRange&range=gLandCoverRanges[i];int code=0,rgb=0;if(range.begin==std::string::npos||!jsonIntegerField(gLandCoverArchive,range.begin,range.end,"dominantCode",code)||!jsonIntegerField(gLandCoverArchive,range.begin,range.end,"dominantRgb",rgb)||code<=0||rgb<0||rgb>0xffffff)return 0;
    bool found=false;for(const auto& category:gLandCoverLegend)if(category.code==code){if(category.rgb!=(unsigned int)rgb)return 0;found=true;break;}if(!found)return 0;gLandCoverZoneCodes[i]=code;gLandCoverZoneRgb[i]=(unsigned int)rgb;
  }
  return indexed;
}
static size_t buildHeritageIndex(){
  size_t indexed=buildZoneIndex(gHeritageArchive,gHeritageRanges);if(indexed!=(size_t)(GRID_ROWS*GRID_COLUMNS))return 0;
  int schema=0,zoneCount=0,recordCount=0;std::string gridVersion,datasetId,license,attribution;
  if(!jsonIntegerField(gHeritageArchive,0,gHeritageArchive.size(),"schemaVersion",schema)||schema!=1
    ||!jsonIntegerField(gHeritageArchive,0,gHeritageArchive.size(),"zoneCount",zoneCount)||zoneCount!=GRID_ROWS*GRID_COLUMNS
    ||!jsonIntegerField(gHeritageArchive,0,gHeritageArchive.size(),"totalRecordCount",recordCount)||recordCount<1
    ||!jsonStringField(gHeritageArchive,0,gHeritageArchive.size(),"gridVersion",gridVersion)||gridVersion!="Z9800"
    ||!jsonStringField(gHeritageArchive,0,gHeritageArchive.size(),"id",datasetId)||datasetId!="whc001"
    ||!jsonStringField(gHeritageArchive,0,gHeritageArchive.size(),"license",license)||license!="CC BY-SA 4.0"
    ||!jsonStringField(gHeritageArchive,0,gHeritageArchive.size(),"attribution",attribution)||attribution!="UNESCO")return 0;
  for(size_t i=0;i<indexed;i++){
    const ArchiveRange& range=gHeritageRanges[i];int siteCount=-1;std::string value;
    if(range.begin==std::string::npos||!jsonIntegerField(gHeritageArchive,range.begin,range.end,"siteCount",siteCount)||siteCount<0
      ||!jsonStringField(gHeritageArchive,range.begin,range.end,"heritage",value)||value.empty())return 0;
  }
  return indexed;
}
static size_t buildPopulationIndex(){
  size_t indexed=buildZoneIndex(gPopulationArchive,gPopulationRanges);if(indexed!=(size_t)(GRID_ROWS*GRID_COLUMNS))return indexed;
  int schema=0,zoneCount=0,year=0,resolution=0;std::string gridVersion,license,attribution,dataset;
  if(!jsonIntegerField(gPopulationArchive,0,gPopulationArchive.size(),"schemaVersion",schema)||schema!=1
    ||!jsonIntegerField(gPopulationArchive,0,gPopulationArchive.size(),"zoneCount",zoneCount)||zoneCount!=GRID_ROWS*GRID_COLUMNS
    ||!jsonIntegerField(gPopulationArchive,0,gPopulationArchive.size(),"year",year)||year!=POPULATION_YEAR
    ||!jsonIntegerField(gPopulationArchive,0,gPopulationArchive.size(),"resolutionArcSeconds",resolution)||resolution!=30
    ||!jsonStringField(gPopulationArchive,0,gPopulationArchive.size(),"gridVersion",gridVersion)||gridVersion!="Z9800"
    ||!jsonStringField(gPopulationArchive,0,gPopulationArchive.size(),"license",license)||license!="CC BY 4.0"
    ||!jsonStringField(gPopulationArchive,0,gPopulationArchive.size(),"attribution",attribution)||attribution!="WorldPop, University of Southampton"
    ||!jsonStringField(gPopulationArchive,0,gPopulationArchive.size(),"dataset",dataset)||dataset!="Global2 R2025A v1")return 0;
  gPopulationCounts.assign(indexed,0);gPopulationDensity.assign(indexed,0);gPopulationCoverage.assign(indexed,0);gPopulationAvailable.assign(indexed,0);gPopulationMaxDensity=0;
  for(size_t i=0;i<indexed;i++){
    const ArchiveRange& range=gPopulationRanges[i];int available=-1;double count=0,density=0,coverage=0;
    if(range.begin==std::string::npos||!jsonIntegerField(gPopulationArchive,range.begin,range.end,"available",available)||(available!=0&&available!=1)
      ||!jsonDoubleField(gPopulationArchive,range.begin,range.end,"populationCount",count)||count<0
      ||!jsonDoubleField(gPopulationArchive,range.begin,range.end,"populationDensityPerLandKm2",density)||density<0
      ||!jsonDoubleField(gPopulationArchive,range.begin,range.end,"latitudeCoveragePercent",coverage)||coverage<0||coverage>100)return 0;
    gPopulationCounts[i]=count;gPopulationDensity[i]=density;gPopulationCoverage[i]=coverage;gPopulationAvailable[i]=(unsigned char)available;
    if(available&&density>gPopulationMaxDensity)gPopulationMaxDensity=density;
  }
  return indexed;
}
static bool buildLandCoverRaster(){
  delete gLandCoverRaster;gLandCoverRaster=nullptr;
  if(gLandCoverZoneRgb.size()!=(size_t)(GRID_ROWS*GRID_COLUMNS))return false;
  Bitmap* raster=new Bitmap(GRID_COLUMNS,GRID_ROWS,PixelFormat32bppARGB);if(!raster||raster->GetLastStatus()!=Ok){delete raster;return false;}
  BitmapData pixels{};Rect bounds(0,0,GRID_COLUMNS,GRID_ROWS);if(raster->LockBits(&bounds,ImageLockModeWrite,PixelFormat32bppARGB,&pixels)!=Ok){delete raster;return false;}
  bool valid=pixels.Scan0!=nullptr&&std::abs(pixels.Stride)>=GRID_COLUMNS*4;
  if(valid){for(int row=0;row<GRID_ROWS;row++){BYTE* line=(BYTE*)pixels.Scan0+(ptrdiff_t)row*pixels.Stride;for(int col=0;col<GRID_COLUMNS;col++){unsigned int rgb=gLandCoverZoneRgb[(size_t)row*GRID_COLUMNS+col];BYTE* pixel=line+col*4;pixel[0]=(BYTE)(rgb&255);pixel[1]=(BYTE)((rgb>>8)&255);pixel[2]=(BYTE)((rgb>>16)&255);pixel[3]=82;}}}
  Status unlocked=raster->UnlockBits(&pixels);if(!valid||unlocked!=Ok){delete raster;return false;}gLandCoverRaster=raster;return true;
}
static unsigned int populationColor(double t){
  t=std::max(0.0,std::min(1.0,t));double r=0,g=0,b=0;
  if(t<0.5){double u=t*2.0;r=37+(250-37)*u;g=99+(204-99)*u;b=235+(21-235)*u;}
  else{double u=(t-.5)*2.0;r=250+(220-250)*u;g=204+(38-204)*u;b=21+(38-21)*u;}
  return ((unsigned int)std::lround(r)<<16)|((unsigned int)std::lround(g)<<8)|(unsigned int)std::lround(b);
}
static bool buildPopulationRaster(){
  delete gPopulationRaster;gPopulationRaster=nullptr;if(gPopulationDensity.size()!=(size_t)(GRID_ROWS*GRID_COLUMNS)||gPopulationAvailable.size()!=gPopulationDensity.size())return false;
  Bitmap* raster=new Bitmap(GRID_COLUMNS,GRID_ROWS,PixelFormat32bppARGB);if(!raster||raster->GetLastStatus()!=Ok){delete raster;return false;}
  BitmapData pixels{};Rect bounds(0,0,GRID_COLUMNS,GRID_ROWS);if(raster->LockBits(&bounds,ImageLockModeWrite,PixelFormat32bppARGB,&pixels)!=Ok){delete raster;return false;}
  bool valid=pixels.Scan0!=nullptr&&std::abs(pixels.Stride)>=GRID_COLUMNS*4;double denom=std::log1p(std::max(1.0,gPopulationMaxDensity));
  if(valid){for(int row=0;row<GRID_ROWS;row++){BYTE* line=(BYTE*)pixels.Scan0+(ptrdiff_t)row*pixels.Stride;for(int col=0;col<GRID_COLUMNS;col++){size_t index=(size_t)row*GRID_COLUMNS+col;BYTE* pixel=line+col*4;pixel[0]=pixel[1]=pixel[2]=pixel[3]=0;if(!gPopulationAvailable[index]||gPopulationDensity[index]<=0)continue;double t=denom>0?std::log1p(gPopulationDensity[index])/denom:0;unsigned int rgb=populationColor(t);pixel[0]=(BYTE)(rgb&255);pixel[1]=(BYTE)((rgb>>8)&255);pixel[2]=(BYTE)((rgb>>16)&255);pixel[3]=160;}}}
  Status unlocked=raster->UnlockBits(&pixels);if(!valid||unlocked!=Ok){delete raster;return false;}gPopulationRaster=raster;return true;
}
static unsigned int world2026RgbFromHex(const std::string& value){if(value.size()!=7||value[0]!='#')return 0;char* end=nullptr;unsigned long rgb=strtoul(value.c_str()+1,&end,16);return end==value.c_str()+7&&rgb<=0xffffff?(unsigned int)rgb:0;}
static bool buildWorld2026Index(){
  gWorld2026Countries.clear();gWorld2026CellCountries.clear();int schema=0,rows=0,columns=0;
  if(!jsonIntegerField(gWorld2026Archive,0,gWorld2026Archive.size(),"schemaVersion",schema)||schema!=1
    ||!jsonIntegerField(gWorld2026Archive,0,gWorld2026Archive.size(),"gridRows",rows)||rows!=FINE_GRID_ROWS
    ||!jsonIntegerField(gWorld2026Archive,0,gWorld2026Archive.size(),"gridColumns",columns)||columns!=FINE_GRID_COLUMNS)return false;
  size_t countryKey=gWorld2026Archive.find("\"countries\"");if(countryKey==std::string::npos)return false;size_t countryArray=gWorld2026Archive.find('[',countryKey);if(countryArray==std::string::npos)return false;
  size_t countryArrayEnd=gWorld2026Archive.find(']',countryArray);if(countryArrayEnd==std::string::npos)return false;size_t object=gWorld2026Archive.find('{',countryArray);if(object==std::string::npos||object>=countryArrayEnd)return false;
  while(object<countryArrayEnd){size_t objectEnd=gWorld2026Archive.find('}',object);if(objectEnd==std::string::npos||objectEnd>countryArrayEnd)return false;int id=0,mapColor=0;std::string code,name,nameZh,color;
    if(!jsonIntegerField(gWorld2026Archive,object,objectEnd,"id",id)||id!=(int)gWorld2026Countries.size()+1
      ||!jsonIntegerField(gWorld2026Archive,object,objectEnd,"mapColor",mapColor)||mapColor<1||mapColor>7
      ||!jsonStringField(gWorld2026Archive,object,objectEnd,"code",code)||code.empty()
      ||!jsonStringField(gWorld2026Archive,object,objectEnd,"name",name)||name.empty()
      ||!jsonStringField(gWorld2026Archive,object,objectEnd,"nameZh",nameZh)
      ||!jsonStringField(gWorld2026Archive,object,objectEnd,"color",color))return false;
    unsigned int rgb=world2026RgbFromHex(color);if(rgb==0)return false;gWorld2026Countries.push_back(World2026Country{code,utf8(name),utf8(nameZh),mapColor,rgb});
    object=gWorld2026Archive.find('{',objectEnd+1);if(object==std::string::npos||object>=countryArrayEnd)break;
  }
  if(gWorld2026Countries.size()<100)return false;
  size_t cellsKey=gWorld2026Archive.find("\"cells\"",countryArrayEnd);if(cellsKey==std::string::npos)return false;size_t cellsArray=gWorld2026Archive.find('[',cellsKey);if(cellsArray==std::string::npos)return false;
  size_t p=cellsArray+1;gWorld2026CellCountries.reserve((size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS);
  while(p<gWorld2026Archive.size()){
    unsigned char c=(unsigned char)gWorld2026Archive[p];if(c==']')break;if(isspace(c)||c==','){p++;continue;}if(c<'0'||c>'9')return false;
    const char* first=gWorld2026Archive.c_str()+p;char* end=nullptr;long value=strtol(first,&end,10);if(end==first||value<0||value>(long)gWorld2026Countries.size())return false;
    gWorld2026CellCountries.push_back((unsigned short)value);p=(size_t)(end-gWorld2026Archive.c_str());
    if(gWorld2026CellCountries.size()>(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return false;
  }
  return gWorld2026CellCountries.size()==(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS;
}
static bool buildWorld2026Raster(){
  delete gWorld2026Raster;gWorld2026Raster=nullptr;if(gWorld2026CellCountries.size()!=(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return false;
  Bitmap* raster=new Bitmap(FINE_GRID_COLUMNS,FINE_GRID_ROWS,PixelFormat32bppARGB);if(!raster||raster->GetLastStatus()!=Ok){delete raster;return false;}
  BitmapData pixels{};Rect bounds(0,0,FINE_GRID_COLUMNS,FINE_GRID_ROWS);if(raster->LockBits(&bounds,ImageLockModeWrite,PixelFormat32bppARGB,&pixels)!=Ok){delete raster;return false;}
  bool valid=pixels.Scan0!=nullptr&&std::abs(pixels.Stride)>=FINE_GRID_COLUMNS*4;
  if(valid){for(int row=0;row<FINE_GRID_ROWS;row++){BYTE* line=(BYTE*)pixels.Scan0+(ptrdiff_t)row*pixels.Stride;for(int col=0;col<FINE_GRID_COLUMNS;col++){size_t index=(size_t)row*FINE_GRID_COLUMNS+col;BYTE* pixel=line+col*4;unsigned short id=gWorld2026CellCountries[index];pixel[0]=pixel[1]=pixel[2]=pixel[3]=0;if(id==0)continue;if(id>gWorld2026Countries.size()){valid=false;break;}unsigned int rgb=gWorld2026Countries[id-1].rgb;pixel[0]=(BYTE)(rgb&255);pixel[1]=(BYTE)((rgb>>8)&255);pixel[2]=(BYTE)((rgb>>16)&255);pixel[3]=255;}if(!valid)break;}}
  Status unlocked=raster->UnlockBits(&pixels);if(!valid||unlocked!=Ok){delete raster;return false;}gWorld2026Raster=raster;return true;
}
static bool buildHistoryIndex(){
  gHistoryPolities.clear();gHistoryCellPolities.clear();int schema=0,rows=0,columns=0,year=0;
  if(!jsonIntegerField(gHistoryArchive,0,gHistoryArchive.size(),"schemaVersion",schema)||schema!=1
    ||!jsonIntegerField(gHistoryArchive,0,gHistoryArchive.size(),"gridRows",rows)||rows!=FINE_GRID_ROWS
    ||!jsonIntegerField(gHistoryArchive,0,gHistoryArchive.size(),"gridColumns",columns)||columns!=FINE_GRID_COLUMNS
    ||!jsonIntegerField(gHistoryArchive,0,gHistoryArchive.size(),"year",year)||year!=gHistoryYear)return false;
  size_t entitiesKey=gHistoryArchive.find("\"entities\"");if(entitiesKey==std::string::npos)return false;size_t entitiesArray=gHistoryArchive.find('[',entitiesKey);size_t entitiesEnd=jsonArrayEnd(gHistoryArchive,entitiesArray);if(entitiesEnd==std::string::npos)return false;
  size_t object=gHistoryArchive.find('{',entitiesArray);if(object==std::string::npos||object>=entitiesEnd)return false;
  while(object<entitiesEnd){size_t objectEnd=gHistoryArchive.find('}',object);if(objectEnd==std::string::npos||objectEnd>entitiesEnd)return false;int id=0;std::string name,color;
    if(!jsonIntegerField(gHistoryArchive,object,objectEnd,"id",id)||id!=(int)gHistoryPolities.size()+1
      ||!jsonStringField(gHistoryArchive,object,objectEnd,"name",name)||name.empty()
      ||!jsonStringField(gHistoryArchive,object,objectEnd,"color",color))return false;
    unsigned int rgb=world2026RgbFromHex(color);if(rgb==0)return false;gHistoryPolities.push_back(HistoryPolity{id,utf8(name),color,rgb});
    object=gHistoryArchive.find('{',objectEnd+1);if(object==std::string::npos||object>=entitiesEnd)break;
  }
  if(gHistoryPolities.empty())return false;
  size_t cellsKey=gHistoryArchive.find("\"cells\"",entitiesEnd);if(cellsKey==std::string::npos)return false;size_t cellsArray=gHistoryArchive.find('[',cellsKey);if(cellsArray==std::string::npos)return false;
  size_t cellsEnd=jsonArrayEnd(gHistoryArchive,cellsArray);if(cellsEnd==std::string::npos)return false;size_t p=cellsArray+1;gHistoryCellPolities.reserve((size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS);
  while(p<cellsEnd){unsigned char c=(unsigned char)gHistoryArchive[p];if(isspace(c)||c==','){p++;continue;}if(c<'0'||c>'9')return false;const char* first=gHistoryArchive.c_str()+p;char* end=nullptr;long value=strtol(first,&end,10);if(end==first||value<0||value>(long)gHistoryPolities.size())return false;gHistoryCellPolities.push_back((unsigned short)value);p=(size_t)(end-gHistoryArchive.c_str());if(gHistoryCellPolities.size()>(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return false;}
  return gHistoryCellPolities.size()==(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS;
}
static bool buildHistoryRaster(){
  delete gHistoryRaster;gHistoryRaster=nullptr;if(gHistoryCellPolities.size()!=(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return false;Bitmap* raster=new Bitmap(FINE_GRID_COLUMNS,FINE_GRID_ROWS,PixelFormat32bppARGB);if(!raster||raster->GetLastStatus()!=Ok){delete raster;return false;}
  BitmapData pixels{};Rect bounds(0,0,FINE_GRID_COLUMNS,FINE_GRID_ROWS);if(raster->LockBits(&bounds,ImageLockModeWrite,PixelFormat32bppARGB,&pixels)!=Ok){delete raster;return false;}bool valid=pixels.Scan0!=nullptr&&std::abs(pixels.Stride)>=FINE_GRID_COLUMNS*4;
  if(valid){for(int row=0;row<FINE_GRID_ROWS;row++){BYTE* line=(BYTE*)pixels.Scan0+(ptrdiff_t)row*pixels.Stride;for(int col=0;col<FINE_GRID_COLUMNS;col++){unsigned short id=gHistoryCellPolities[(size_t)row*FINE_GRID_COLUMNS+col];BYTE* pixel=line+col*4;pixel[0]=pixel[1]=pixel[2]=pixel[3]=0;if(!id)continue;if(id>gHistoryPolities.size()){valid=false;break;}unsigned int rgb=gHistoryPolities[id-1].rgb;pixel[0]=(BYTE)(rgb&255);pixel[1]=(BYTE)((rgb>>8)&255);pixel[2]=(BYTE)((rgb>>16)&255);pixel[3]=255;}if(!valid)break;}}
  Status unlocked=raster->UnlockBits(&pixels);if(!valid||unlocked!=Ok){delete raster;return false;}gHistoryRaster=raster;return true;
}
static bool loadHistoryModule(int year){
  if(gHistoryLoaded&&gHistoryRasterYear==year&&gHistoryRaster){gHistoryYear=year;return true;}delete gHistoryRaster;gHistoryRaster=nullptr;gHistoryArchive.clear();gHistoryPolities.clear();gHistoryCellPolities.clear();gHistoryLoaded=false;gHistoryRasterYear=INT_MIN;gHistoryYear=year;
  std::wstring file=appDir()+L"\\data\\world-"+std::to_wstring(year)+L".json";if(!loadTextFile(file,gHistoryArchive,gHistoryError))return false;
  if(!buildHistoryIndex()){gHistoryError=ERROR_REVISION_MISMATCH;gHistoryArchive.clear();gHistoryPolities.clear();gHistoryCellPolities.clear();return false;}
  if(!buildHistoryRaster()){gHistoryError=ERROR_NOT_ENOUGH_MEMORY;gHistoryArchive.clear();gHistoryPolities.clear();gHistoryCellPolities.clear();return false;}gHistoryLoaded=true;gHistoryRasterYear=year;return true;
}
static std::wstring historyYearText(int year){return year<0?L"公元前"+std::to_wstring(-year)+L"年":L"公元"+std::to_wstring(year)+L"年";}
static bool loadHistoryEvents(const std::wstring& path){
  gHistoryEvents.clear();gHistoryEventsText.clear();gHistoryEventsLoaded=false;gHistoryRangeEventCache.clear();gHistoryCacheRangeStart=gHistoryCacheRangeEnd=INT_MIN;gHistoryEventDescriptionHeights.clear();gHistoryEventDescriptionWidth=0;if(!loadTextFile(path,gHistoryEventsText,gHistoryEventsError))return false;
  std::istringstream input(gHistoryEventsText);std::string line;int previousYear=INT_MIN;std::vector<HistoryEvent> candidate;
  while(std::getline(input,line)){
    if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;
    std::vector<std::string> columns;size_t begin=0;for(size_t i=0;i<=line.size();i++)if(i==line.size()||line[i]=='\t'){columns.push_back(line.substr(begin,i-begin));begin=i+1;}
    if(columns.size()!=6||candidate.size()>=10000){gHistoryEventsError=columns.size()!=6?ERROR_INVALID_DATA:ERROR_FILE_TOO_LARGE;return false;}
    char* end=nullptr;long parsed=std::strtol(columns[0].c_str(),&end,10);if(!end||*end||parsed<-100000||parsed>9999||parsed==0||parsed<previousYear||columns[1].empty()||columns[2].empty()||columns[3].empty()||columns[4].empty()||(columns[5].find("https://")!=0&&columns[5].find("http://")!=0)){gHistoryEventsError=ERROR_INVALID_DATA;return false;}
    previousYear=(int)parsed;candidate.push_back(HistoryEvent{(int)parsed,utf8(columns[1]),utf8(columns[2]),utf8(columns[3]),utf8(columns[4]),utf8(columns[5])});
  }
  if(candidate.empty()){gHistoryEventsError=ERROR_INVALID_DATA;return false;}gHistoryEvents.swap(candidate);gHistoryEventsLoaded=true;gHistoryEventsError=ERROR_SUCCESS;return true;
}
static void setHistoryEventRange(int selected,bool playback){
  if(selected<0||selected>=HISTORY_YEAR_COUNT){gHistoryEventRangeStart=gHistoryEventRangeEnd=0;return;}int year=HISTORY_YEARS[selected];
  if(playback&&selected+1<HISTORY_YEAR_COUNT){int older=HISTORY_YEARS[selected+1];gHistoryEventRangeStart=(selected+1==HISTORY_YEAR_COUNT-1)?older:older+1;gHistoryEventRangeEnd=year;}
  else gHistoryEventRangeStart=gHistoryEventRangeEnd=year;
  if(playback)return;
  gHistoryEventRelayMode=false;gHistoryRelayEvents.clear();gHistoryEventCycleHeight=0;gScrollOffset=0;gHistoryEventFractionPending=false;
}
static void appendHistoryEventsInRange(int startYear,int endYear){if(!gHistoryEventsLoaded||startYear>endYear)return;for(size_t i=0;i<gHistoryEvents.size();i++)if(gHistoryEvents[i].year>=startYear&&gHistoryEvents[i].year<=endYear)gHistoryRelayEvents.push_back(i);}
static const std::vector<size_t>& historyEventsForCurrentRange(){
  if(gHistoryCacheRangeStart==gHistoryEventRangeStart&&gHistoryCacheRangeEnd==gHistoryEventRangeEnd)return gHistoryRangeEventCache;
  gHistoryCacheRangeStart=gHistoryEventRangeStart;gHistoryCacheRangeEnd=gHistoryEventRangeEnd;gHistoryRangeEventCache.clear();if(!gHistoryEventsLoaded)return gHistoryRangeEventCache;
  auto first=std::lower_bound(gHistoryEvents.begin(),gHistoryEvents.end(),gHistoryEventRangeStart,[](const HistoryEvent& event,int year){return event.year<year;});
  for(auto it=first;it!=gHistoryEvents.end()&&it->year<=gHistoryEventRangeEnd;++it)gHistoryRangeEventCache.push_back((size_t)(it-gHistoryEvents.begin()));
  if(!gHistoryRangeEventCache.empty())return gHistoryRangeEventCache;
  const int year=gHistoryEventRangeEnd;auto nearest=std::lower_bound(gHistoryEvents.begin(),gHistoryEvents.end(),year,[](const HistoryEvent& event,int value){return event.year<value;});size_t right=(size_t)(nearest-gHistoryEvents.begin()),left=right;
  while(gHistoryRangeEventCache.size()<6&&(left>0||right<gHistoryEvents.size())){
    if(left==0)gHistoryRangeEventCache.push_back(right++);
    else if(right==gHistoryEvents.size())gHistoryRangeEventCache.push_back(--left);
    else{long long before=std::llabs((long long)gHistoryEvents[left-1].year-year),after=std::llabs((long long)gHistoryEvents[right].year-year);if(before<=after)gHistoryRangeEventCache.push_back(--left);else gHistoryRangeEventCache.push_back(right++);}
  }
  std::sort(gHistoryRangeEventCache.begin(),gHistoryRangeEventCache.end());return gHistoryRangeEventCache;
}
static std::wstring historyEventSourceLabel(const std::wstring& source){const std::wstring prefix=L"https://en.wikipedia.org/wiki/";if(source.find(prefix)==0){std::wstring name=source.substr(prefix.size());for(auto& c:name)if(c==L'_')c=L' ';size_t query=name.find_first_of(L"?#");if(query!=std::wstring::npos)name.resize(query);return L"Wikipedia · "+name;}return source;}
static void updateHistoryPlaybackButton(){if(gHistoryPlaybackButton)SetWindowTextW(gHistoryPlaybackButton,gHistoryPlaying?L"暂停播放":(gHistoryPaused?L"继续播放":L"从古到今播放"));}
static UINT historyPlaybackInterval(){return gHistoryAnimationEnabled?HISTORY_ANIMATED_PLAYBACK_INTERVALS[gHistorySpeedIndex]:HISTORY_PLAYBACK_INTERVALS[gHistorySpeedIndex];}
static void finishHistoryTransition(HWND w){if(w)KillTimer(w,HISTORY_TRANSITION_TIMER);delete gHistoryTransitionPreviousRaster;gHistoryTransitionPreviousRaster=nullptr;gHistoryTransitionActive=false;gMapSceneCacheValid=false;if(w)InvalidateRect(w,&gMapRect,FALSE);}
static float historyTransitionProgress(){if(!gHistoryTransitionActive||gHistoryTransitionDurationMs<=0)return 1.0f;DWORD elapsed=GetTickCount()-gHistoryTransitionStart;float linear=std::min(1.0f,(float)elapsed/gHistoryTransitionDurationMs);return linear*linear*(3.0f-2.0f*linear);}
static Bitmap* cloneHistoryRaster(){if(!gHistoryRaster)return nullptr;Bitmap* copy=gHistoryRaster->Clone(0,0,(INT)gHistoryRaster->GetWidth(),(INT)gHistoryRaster->GetHeight(),PixelFormat32bppARGB);if(!copy||copy->GetLastStatus()!=Ok){delete copy;return nullptr;}return copy;}
static void startHistoryTransition(HWND w,Bitmap* previous){
  finishHistoryTransition(w);if(!gHistoryAnimationEnabled||!gHistoryPlaying||!previous)return;
  gHistoryTransitionPreviousRaster=previous;gHistoryTransitionActive=true;gHistoryTransitionStart=GetTickCount();gHistoryTransitionDurationMs=std::max(180,(int)historyPlaybackInterval()-120);SetTimer(w,HISTORY_TRANSITION_TIMER,33,nullptr);gMapSceneCacheValid=false;InvalidateRect(w,&gMapRect,FALSE);
}
static bool selectHistoryYear(HWND w,int selected){
  if(selected<0||selected>=HISTORY_YEAR_COUNT)return false;int year=HISTORY_YEARS[selected];gHistoryYear=year;
  if(loadHistoryModule(year)){if(gHistoryToggleButton)EnableWindow(gHistoryToggleButton,TRUE);gNotice=L"已载入"+historyYearText(year)+L"历史填色模块；空白区域表示来源未收录";}
  else{gShowHistory=false;if(gHistoryToggleButton){SendMessageW(gHistoryToggleButton,BM_SETCHECK,BST_UNCHECKED,0);EnableWindow(gHistoryToggleButton,FALSE);}gNotice=historyYearText(year)+L"历史模块读取失败；请检查随附 data 文件";}
  if(gOpacityLabel){wchar_t opacity[48];swprintf(opacity,48,L"填色透明度 %d%%",gFillOpacity);SetWindowTextW(gOpacityLabel,opacity);}InvalidateRect(w,nullptr,FALSE);return gHistoryLoaded;
}
static std::wstring historyZoneField(){
  if(!gHistoryLoaded||gHistoryCellPolities.size()!=(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return L"历史世界图层未加载";int row=0,col=0;world2026CellForCoordinate(gSelectedLatitude,gSelectedLongitude,row,col);unsigned short id=gHistoryCellPolities[(size_t)row*FINE_GRID_COLUMNS+col];
  if(id==0)return L"该细格中心没有对应的已收录政权边界，保持透明；不代表无人居住或现代国界。";if(id>gHistoryPolities.size())return L"历史数据编号无效";return gHistoryPolities[id-1].name+L"（"+historyYearText(gHistoryYear)+L"；按细格中心匹配）";
}
static std::wstring world2026ZoneField(){
  if(!gWorld2026Loaded||gWorld2026CellCountries.size()!=(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return L"世界2026 模块未加载";
  int row=0,col=0;world2026CellForCoordinate(gSelectedLatitude,gSelectedLongitude,row,col);unsigned short id=gWorld2026CellCountries[(size_t)row*FINE_GRID_COLUMNS+col];
  if(id==0)return L"本子格中心未落入 Natural Earth 国界面（海洋或未收录的微小岛屿）；不据此判断主权归属。";
  if(id>gWorld2026Countries.size())return L"世界2026 数据编号无效";const auto& country=gWorld2026Countries[id-1];
  std::wstring label=country.nameZh.empty()?country.name:country.nameZh;if(label!=country.name)label+=L"（"+country.name+L"）";
  return label+L"（"+utf8(country.code)+L"）；按 627,200 细格中心归属，边界格只归入一个国别。";
}
static std::wstring archiveZoneField(int row,int col,const char* key,const std::wstring& fallback){return gArchiveLoaded?zoneField(gArchive,gZoneRanges,row,col,key,fallback):L"档案未加载";}
static std::wstring landCoverZoneField(int row,int col,const char* key,const std::wstring& fallback){return gLandCoverLoaded?zoneField(gLandCoverArchive,gLandCoverRanges,row,col,key,fallback):fallback;}
static std::wstring heritageZoneField(int row,int col,const std::wstring& fallback){return gHeritageLoaded?zoneField(gHeritageArchive,gHeritageRanges,row,col,"heritage",fallback):fallback;}
static std::wstring formatPopulationCount(double value){long long n=(long long)std::llround(std::max(0.0,std::min(value,9.0e15)));std::wstring digits=std::to_wstring(n);for(int i=(int)digits.size()-3;i>0;i-=3)digits.insert((size_t)i,L",");return digits;}
static std::wstring populationZoneField(int row,int col,const std::wstring& fallback){
  if(!gPopulationLoaded||row<0||row>=GRID_ROWS||col<0||col>=GRID_COLUMNS)return fallback;size_t index=(size_t)row*GRID_COLUMNS+col;if(index>=gPopulationAvailable.size())return fallback;
  if(!gPopulationAvailable[index])return L"本格超出 WorldPop "+std::to_wstring(POPULATION_YEAR)+L" 1 km 栅格的纬度覆盖范围（约 60°S 至 84°N）；不提供估值，也不表示人口为零。";
  std::wostringstream density,coverage;density<<std::fixed<<std::setprecision(gPopulationDensity[index]>=100?0:(gPopulationDensity[index]>=10?1:2))<<gPopulationDensity[index];coverage<<std::fixed<<std::setprecision(1)<<gPopulationCoverage[index];
  std::wstring text=L"WorldPop "+std::to_wstring(POPULATION_YEAR)+L" 模型估算约 "+formatPopulationCount(gPopulationCounts[index])+L" 人；按 ETOPO 陆地面积计算，密度约 "+density.str()+L" 人/km²。本格栅格纬度覆盖约 "+coverage.str()+L"%。这是基于约 1 km 人口栅格汇总的估算，不是当地普查；边界像元按中心位置归入单格，国家间和年份间精度存在差异。";
  if(gPopulationDensity[index]>=0&&gPopulationCounts[index]==0)text+=L"本格估算为零不代表已证明无人居住。";return text;
}
static bool locateQuery(const std::wstring& raw,int& row,int& col,int& matches,bool* legacyConverted=nullptr,std::vector<SearchMatch>* allMatches=nullptr){
  std::vector<SearchMatch> emptyMatches;std::vector<SearchMatch>& found=allMatches?*allMatches:emptyMatches;found.clear();
  if(legacyConverted)*legacyConverted=false;
  std::wstring q=raw;while(!q.empty()&&iswspace(q.front()))q.erase(q.begin());while(!q.empty()&&iswspace(q.back()))q.pop_back();if(q.empty())return false;
  if(q[0]==L'Z'||q[0]==L'z'){
    int version=-1,r=-1,c=-1;
    if(swscanf(q.c_str()+1,L"%d-%d-%d",&version,&r,&c)==3){
      int level=gridLevelForCount(version);
      if(level>=0){int scale=1<<level,rows=GRID_ROWS*scale,columns=GRID_COLUMNS*scale;if(r>=0&&r<rows&&c>=0&&c<columns){row=r/scale;col=c/scale;double lat=90.0-(r+.5)*(180.0/rows),longitude=-180.0+(c+.5)*(360.0/columns);matches=1;found.push_back(SearchMatch(row,col,lat,longitude,level));return true;}}
    }
    int legacyRow=-1,legacyCol=-1;
    if(swscanf(q.c_str()+1,L"%d-%d",&legacyRow,&legacyCol)==2&&legacyRow>=0&&legacyRow<LEGACY_ROWS&&legacyCol>=0&&legacyCol<LEGACY_COLUMNS){double legacyLat=90.0-(legacyRow+.5)*(180.0/LEGACY_ROWS),legacyLon=-180.0+(legacyCol+.5)*(360.0/LEGACY_COLUMNS);row=std::min(GRID_ROWS-1,std::max(0,(int)floor((90.0-legacyLat)/GRID_CELL_DEGREES)));col=std::min(GRID_COLUMNS-1,std::max(0,(int)floor((legacyLon+180.0)/(360.0/GRID_COLUMNS))));matches=1;found.push_back(SearchMatch(row,col));if(legacyConverted)*legacyConverted=true;return true;}
  }
  double lat=0,lo=0;if(swscanf(q.c_str(),L"%lf , %lf",&lat,&lo)==2&&lat>=-90&&lat<=90&&lo>=-180&&lo<=180){row=std::min(GRID_ROWS-1,std::max(0,(int)floor((90.0-lat)/GRID_CELL_DEGREES)));col=std::min(GRID_COLUMNS-1,std::max(0,(int)floor((lo+180.0)/(360.0/GRID_COLUMNS))));matches=1;found.push_back(SearchMatch(row,col,lat,lo));return true;}
  std::string query=toUtf8(q);if(query.empty()||!gArchiveLoaded)return false;
  const char* fields[]={"cities","countries","rivers","lakes","mountains","landforms","history","formerNames","biomes","ecoregions","climate"};
  const std::string prefix="\"zoneId\": \"";size_t pos=gArchive.find("\"zones\": [");if(pos==std::string::npos)return false;int bestScore=0;std::vector<unsigned char> seen((size_t)(GRID_ROWS*GRID_COLUMNS),0);
  auto addMatch=[&](int candidateRow,int candidateCol,int score){if(score>bestScore){bestScore=score;found.clear();std::fill(seen.begin(),seen.end(),0);}if(score!=bestScore)return;size_t index=(size_t)candidateRow*GRID_COLUMNS+candidateCol;if(!seen[index]){seen[index]=1;found.push_back(SearchMatch(candidateRow,candidateCol));}};
  while((pos=gArchive.find(prefix,pos))!=std::string::npos){size_t value=pos+prefix.size(),idEnd=gArchive.find('\"',value);if(idEnd==std::string::npos)break;size_t next=gArchive.find(prefix,idEnd+1);size_t end=next==std::string::npos?gArchive.size():next;bool hit=false;
    int score=0;for(const char* field:fields){std::string key=std::string("\"")+field+"\": \"";size_t f=gArchive.find(key,idEnd);if(f==std::string::npos||f>=end)continue;size_t start=f+key.size(),valueEnd=jsonStringEnd(gArchive,start,end);if(!containsRange(gArchive,query,start,valueEnd))continue;hit=true;int candidate=std::string(field)=="cities"?(exactNameRange(gArchive,query,start,valueEnd)?3:2):1;score=std::max(score,candidate);}
    if(hit){int version=-1,r=-1,c=-1;std::string id=gArchive.substr(value,idEnd-value);if(sscanf(id.c_str(),"Z%d-%d-%d",&version,&r,&c)==3&&version==GRID_VERSION&&r>=0&&r<GRID_ROWS&&c>=0&&c<GRID_COLUMNS)addMatch(r,c,score);}
    if(next==std::string::npos)break;pos=idEnd+1;
  }
  if(gLandCoverLoaded){
    for(size_t index=0;index<gLandCoverRanges.size();index++){
      if(gLandCoverRanges[index].begin==std::string::npos)continue;
      int candidateRow=(int)(index/GRID_COLUMNS),candidateCol=(int)(index%GRID_COLUMNS);
      std::string value=toUtf8(landCoverZoneField(candidateRow,candidateCol,"landCover",L""));
      if(value.find(query)==std::string::npos)continue;
      addMatch(candidateRow,candidateCol,2);
    }
  }
  if(gHeritageLoaded){
    for(size_t index=0;index<gHeritageRanges.size();index++){
      if(gHeritageRanges[index].begin==std::string::npos)continue;
      int siteCount=0;if(!jsonIntegerField(gHeritageArchive,gHeritageRanges[index].begin,gHeritageRanges[index].end,"siteCount",siteCount)||siteCount<=0)continue;
      int candidateRow=(int)(index/GRID_COLUMNS),candidateCol=(int)(index%GRID_COLUMNS);
      std::string value=toUtf8(heritageZoneField(candidateRow,candidateCol,L""));
      if(value.find(query)==std::string::npos)continue;
      addMatch(candidateRow,candidateCol,4);
    }
  }
  if(bestScore==0||found.empty())return false;
  row=found.front().row;col=found.front().col;matches=(int)found.size();
  return true;
}
static std::wstring trimmedSearchQuery(const std::wstring& raw){std::wstring q=raw;while(!q.empty()&&iswspace(q.front()))q.erase(q.begin());while(!q.empty()&&iswspace(q.back()))q.pop_back();return q;}
static void clampMapView();
static bool locateOrCycleQuery(const std::wstring& raw,int& row,int& col,int& matches,bool* legacyConverted=nullptr){
  std::wstring query=trimmedSearchQuery(raw);if(query.empty()){gLastSearchQuery.clear();gSearchMatches.clear();gSearchMatchIndex=0;gSearchWasLegacy=false;return false;}
  if(query==gLastSearchQuery&&!gSearchMatches.empty()){gSearchMatchIndex=(gSearchMatchIndex+1)%gSearchMatches.size();row=gSearchMatches[gSearchMatchIndex].row;col=gSearchMatches[gSearchMatchIndex].col;matches=(int)gSearchMatches.size();if(legacyConverted)*legacyConverted=gSearchWasLegacy;return true;}
  std::vector<SearchMatch> results;bool legacy=false;if(!locateQuery(query,row,col,matches,&legacy,&results)){gLastSearchQuery=query;gSearchMatches.clear();gSearchMatchIndex=0;gSearchWasLegacy=false;if(legacyConverted)*legacyConverted=false;return false;}
  gLastSearchQuery=query;gSearchMatches.swap(results);gSearchMatchIndex=0;gSearchWasLegacy=legacy;if(legacyConverted)*legacyConverted=legacy;return true;
}
static void selectSearchResultOnMap(int row,int col){gRow=row;gCol=col;gScrollOffset=0;gMapZoom=std::max(gMapZoom,4.0);gMapCenterX=(col+.5)/GRID_COLUMNS;gMapCenterY=(row+.5)/GRID_ROWS;clampMapView();gSelectedLatitude=90.0-(row+.5)*GRID_CELL_DEGREES;gSelectedLongitude=-180.0+(col+.5)*(360.0/GRID_COLUMNS);gSelectedCoordinateExact=false;gSelectedCoordinateFromInput=false;gSelectedCoordinateIsFineGridCenter=false;}
static void selectSearchResultOnMap(const SearchMatch& match){selectSearchResultOnMap(match.row,match.col);if(match.gridLevel>=0){gGridLevel=match.gridLevel;if(gGridLevelCombo)SendMessageW(gGridLevelCombo,CB_SETCURSEL,gGridLevel,0);}if(match.hasExactCoordinate){gSelectedLatitude=match.latitude;gSelectedLongitude=match.longitude;gSelectedCoordinateExact=true;gSelectedCoordinateFromInput=match.gridLevel<0;gSelectedCoordinateIsFineGridCenter=match.gridLevel>=0;}}
static double bounded(double value,double low,double high){return std::max(low,std::min(high,value));}
static void clampMapView(){double halfX=0.5/gMapZoom,halfY=0.5/gMapZoom;gMapCenterX=bounded(gMapCenterX,halfX,1.0-halfX);gMapCenterY=bounded(gMapCenterY,halfY,1.0-halfY);}
static void resetMapView(){gMapZoom=1.0;gMapCenterX=0.5;gMapCenterY=0.5;}
static double mapWorldX(int x){double width=std::max(1L,gMapRect.right-gMapRect.left);return gMapCenterX+(x-(gMapRect.left+gMapRect.right)/2.0)/(width*gMapZoom);}
static double mapWorldY(int y){double height=std::max(1L,gMapRect.bottom-gMapRect.top);return gMapCenterY+(y-(gMapRect.top+gMapRect.bottom)/2.0)/(height*gMapZoom);}
static int mapScreenX(double fraction){return (int)std::lround((gMapRect.left+gMapRect.right)/2.0+(fraction-gMapCenterX)*(gMapRect.right-gMapRect.left)*gMapZoom);}
static int mapScreenY(double fraction){return (int)std::lround((gMapRect.top+gMapRect.bottom)/2.0+(fraction-gMapCenterY)*(gMapRect.bottom-gMapRect.top)*gMapZoom);}
static bool selectedPointScreenPosition(int& x,int& y){if(gCol<0||!gSelectedCoordinateExact||gMapRect.right<=gMapRect.left||gMapRect.bottom<=gMapRect.top||gSelectedLatitude<-90||gSelectedLatitude>90||gSelectedLongitude<-180||gSelectedLongitude>180)return false;int px=mapScreenX((gSelectedLongitude+180.0)/360.0),py=mapScreenY((90.0-gSelectedLatitude)/180.0);if(px<gMapRect.left||px>gMapRect.right||py<gMapRect.top||py>gMapRect.bottom)return false;x=std::max((int)gMapRect.left,std::min((int)gMapRect.right-1,px));y=std::max((int)gMapRect.top,std::min((int)gMapRect.bottom-1,py));return true;}
static bool selectNasaDetailLayerForZoom(){
  std::wstring folder;int pixels=0,resolution=0,cacheLimit=0;
  if(gMap==gMapNasa&&gMapZoom>=12.0&&gNasa500mAvailable){folder=L"nasa-500m";pixels=2400;resolution=500;cacheLimit=12;}
  else if(gMap==gMapNasa&&gMapZoom>=8.0&&gNasa1kmAvailable){folder=L"nasa-1km";pixels=1200;resolution=1000;cacheLimit=24;}
  else if(gMap==gMapNasa&&gMapZoom>=8.0&&gNasa500mAvailable){folder=L"nasa-500m";pixels=2400;resolution=500;cacheLimit=12;}
  if(folder.empty()){
    if(!gNasaDetailFolder.empty())clearNasaDetailTileCache();
    gNasaDetailFolder.clear();return false;
  }
  if(folder!=gNasaDetailFolder)clearNasaDetailTileCache();
  gNasaDetailFolder=folder;gNasaDetailPixels=pixels;gNasaDetailResolutionMeters=resolution;gNasaDetailCacheLimit=cacheLimit;return true;
}
static Image* getNasaDetailTile(int row,int col){
  for(auto& tile:gNasaDetailTiles)if(tile.row==row&&tile.col==col){tile.lastUsed=++gNasaTileUseCounter;return tile.image;}
  wchar_t name[48];swprintf(name,48,L"r%02d-c%02d",row,col);std::wstring base=appDir()+L"\\assets\\"+gNasaDetailFolder+L"\\tiles\\"+name;Image* candidate=Image::FromFile((base+L".jpg").c_str());
  double west=-180.0+col*10.0,north=90.0-row*10.0;
  if(!candidate||candidate->GetLastStatus()!=Ok||candidate->GetWidth()!=gNasaDetailPixels||candidate->GetHeight()!=gNasaDetailPixels||!validateWorldMapMetadata(base,gNasaDetailPixels,gNasaDetailPixels,west,west+10.0,north,north-10.0)){delete candidate;gNasaTileLoadFailures++;return nullptr;}
  if(gNasaDetailTiles.size()>=(size_t)gNasaDetailCacheLimit){size_t oldest=0;for(size_t i=1;i<gNasaDetailTiles.size();i++)if(gNasaDetailTiles[i].lastUsed<gNasaDetailTiles[oldest].lastUsed)oldest=i;delete gNasaDetailTiles[oldest].image;gNasaDetailTiles.erase(gNasaDetailTiles.begin()+oldest);}
  gNasaDetailTiles.push_back(NasaDetailTile(row,col,candidate,++gNasaTileUseCounter));return candidate;
}
static void drawNasaDetailTiles(Graphics& gr){
  if(!gNasaDetailAvailable||!selectNasaDetailLayerForZoom())return;
  double left=std::max(0.0,gMapCenterX-0.5/gMapZoom),right=std::min(1.0,gMapCenterX+0.5/gMapZoom),top=std::max(0.0,gMapCenterY-0.5/gMapZoom),bottom=std::min(1.0,gMapCenterY+0.5/gMapZoom);
  int firstCol=std::max(0,std::min(35,(int)std::floor(left*36.0))),lastCol=std::max(0,std::min(35,(int)std::ceil(right*36.0)-1));
  int firstRow=std::max(0,std::min(17,(int)std::floor(top*18.0))),lastRow=std::max(0,std::min(17,(int)std::ceil(bottom*18.0)-1));
  InterpolationMode previous=gr.GetInterpolationMode();gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
  for(int row=firstRow;row<=lastRow;row++)for(int col=firstCol;col<=lastCol;col++){
    Image* tile=getNasaDetailTile(row,col);if(!tile)continue;
    int x1=mapScreenX(col/36.0),x2=mapScreenX((col+1)/36.0),y1=mapScreenY(row/18.0),y2=mapScreenY((row+1)/18.0);
    if(x2>x1&&y2>y1)gr.DrawImage(tile,x1,y1,x2-x1,y2-y1);
  }
  gr.SetInterpolationMode(previous);
}
static bool mapContainsPoint(int x,int y){return x>=gMapRect.left&&x<gMapRect.right&&y>=gMapRect.top&&y<gMapRect.bottom;}
static bool selectMapPoint(int x,int y){if(!mapContainsPoint(x,y))return false;double wx=mapWorldX(x),wy=mapWorldY(y);if(wx<0||wx>=1||wy<0||wy>=1)return false;gCol=std::min(GRID_COLUMNS-1,std::max(0,(int)floor(wx*GRID_COLUMNS+1e-10)));gRow=std::min(GRID_ROWS-1,std::max(0,(int)floor(wy*GRID_ROWS+1e-10)));gScrollOffset=0;gSelectedLatitude=90.0-wy*180.0;gSelectedLongitude=-180.0+wx*360.0;gSelectedCoordinateExact=true;gSelectedCoordinateFromInput=false;gSelectedCoordinateIsFineGridCenter=false;return true;}
static double mapZoomLimit(){
  if(gMap&&gMap==gMapNasa&&gNasa500mAvailable)return 64.0;
  if(gMap&&gMap==gMapNasa&&gNasa1kmAvailable)return 32.0;
  if(gMap&&gMap==gMapHighRes&&gMapHighRes->GetWidth()>=16000)return 32.0;
  return 16.0;
}
static void zoomMapAt(int x,int y,double factor){double next=bounded(gMapZoom*factor,1.0,mapZoomLimit());if(std::fabs(next-gMapZoom)<1e-9)return;double wx=mapWorldX(x),wy=mapWorldY(y);gMapZoom=next;gMapCenterX=wx-(x-(gMapRect.left+gMapRect.right)/2.0)/((gMapRect.right-gMapRect.left)*gMapZoom);gMapCenterY=wy-(y-(gMapRect.top+gMapRect.bottom)/2.0)/((gMapRect.bottom-gMapRect.top)*gMapZoom);clampMapView();}
static void panMapBy(int dx,int dy){double width=std::max(1L,gMapRect.right-gMapRect.left),height=std::max(1L,gMapRect.bottom-gMapRect.top);gMapCenterX-=dx/(width*gMapZoom);gMapCenterY-=dy/(height*gMapZoom);clampMapView();}
static std::wstring num(double x){ std::wostringstream o;o<<std::fixed<<std::setprecision(2)<<std::fabs(x)<<L"°"<<(x>=0?L"N":L"S");return o.str(); }
static std::wstring lon(double x){ std::wostringstream o;o<<std::fixed<<std::setprecision(2)<<std::fabs(x)<<L"°"<<(x>=0?L"E":L"W");return o.str(); }
static std::wstring preciseLatitude(double x){std::wostringstream o;o<<std::fixed<<std::setprecision(4)<<std::fabs(x)<<L"°"<<(x>=0?L"N":L"S");return o.str();}
static std::wstring preciseLongitude(double x){std::wostringstream o;o<<std::fixed<<std::setprecision(4)<<std::fabs(x)<<L"°"<<(x>=0?L"E":L"W");return o.str();}
static std::wstring selectedCoordinateText(){if(!gSelectedCoordinateExact)return L"定位参考：分区中心";const wchar_t* source=gSelectedCoordinateFromInput?L"输入坐标 ":(gSelectedCoordinateIsFineGridCenter?L"细格中心 ":L"点击换算 ");return std::wstring(source)+preciseLatitude(gSelectedLatitude)+L"，"+preciseLongitude(gSelectedLongitude);}
static std::wstring gridCellText(){std::wostringstream o;o<<std::fixed<<std::setprecision(2)<<GRID_CELL_DEGREES;return o.str();}
static std::wstring fineGridCellText(){std::wostringstream o;o<<std::fixed<<std::setprecision(4)<<180.0/activeGridRows();return o.str();}
static std::wstring fineGridBoundsText(int row,int col){int rows=activeGridRows(),columns=activeGridColumns();double north=90.0-row*(180.0/rows),south=90.0-(row+1)*(180.0/rows),west=-180.0+col*(360.0/columns),east=-180.0+(col+1)*(360.0/columns);return L"北 "+preciseLatitude(north)+L" · 南 "+preciseLatitude(south)+L"\r\n西 "+preciseLongitude(west)+L" · 东 "+preciseLongitude(east);}
static double wgs84ZoneAreaKm2(int row){
  if(row<0||row>=GRID_ROWS)return 0.0;
  const double a=6378137.0,e2=0.0066943799901413165,e=std::sqrt(e2);
  const double north=(90.0-row*GRID_CELL_DEGREES)*3.14159265358979323846/180.0;
  const double south=(90.0-(row+1)*GRID_CELL_DEGREES)*3.14159265358979323846/180.0;
  auto primitive=[&](double latitude){double s=std::sin(latitude),es=e*s;return a*a*(1.0-e2)*(s/(2.0*(1.0-e2*s*s))+(std::log((1.0+es)/(1.0-es))/(4.0*e)));};
  const double longitudeWidth=(360.0/GRID_COLUMNS)*3.14159265358979323846/180.0;
  return longitudeWidth*(primitive(north)-primitive(south))/1000000.0;
}
static std::wstring wgs84ZoneAreaText(int row){
  long long rounded=(long long)std::llround(wgs84ZoneAreaKm2(row));std::wstring digits=std::to_wstring(rounded);
  for(int i=(int)digits.size()-3;i>0;i-=3)digits.insert((size_t)i,L",");
  return L"约 "+digits+L" km²（WGS 84 椭球面，含海洋）";
}
static std::wstring pointerStatus(int x,int y){
  if(!mapContainsPoint(x,y))return L"";
  if(PtInRect(&gLandCoverLegendRect,POINT{x,y})||PtInRect(&gPopulationLegendRect,POINT{x,y})||PtInRect(&gWorld2026LegendRect,POINT{x,y})||PtInRect(&gHistoryLegendRect,POINT{x,y}))return L"";
  double wx=mapWorldX(x),wy=mapWorldY(y);if(wx<0||wx>=1||wy<0||wy>=1)return L"";
  int fineRow=0,fineCol=0;fineGridCellForWorld(wx,wy,fineRow,fineCol);int scale=activeGridScale(),parentRow=fineRow/scale,parentCol=fineCol/scale;
  std::wstring status=L"指针位置 "+num(90.0-wy*180.0)+L" "+lon(-180.0+wx*360.0)+L" · "+gridIdText(activeGridCount(),fineRow,fineCol);
  if(gGridLevel>0)status+=L" · 父格 "+gridIdText(GRID_VERSION,parentRow,parentCol);
  return status;
}
static void invalidateNotice(HWND w){RECT client;GetClientRect(w,&client);RECT status{700,40,client.right-220,68};if(status.right>status.left)InvalidateRect(w,&status,FALSE);}
static std::wstring terrain(double lat,double lo){if(std::fabs(lat)>66)return L"极地冰盖、冻原或寒冷海域";if(std::fabs(lat)<15)return L"热带雨林、热带草原、岛屿或赤道海域";if(std::fabs(lat)<32&&lo>-20&&lo<65)return L"干旱—半干旱带、荒漠与高原交错";return L"山地、平原、丘陵或温带海域";}
static std::wstring nearCity(double lat,double lo){struct C{const wchar_t*n;double a,b;};static C c[]={ {L"北京",39.9,116.4},{L"东京",35.7,139.7},{L"新加坡",1.35,103.8},{L"悉尼",-33.9,151.2},{L"巴黎",48.9,2.35},{L"伦敦",51.5,-.13},{L"纽约",40.7,-74},{L"洛杉矶",34.1,-118.2},{L"开罗",30,31.2},{L"圣保罗",-23.6,-46.6} };double best=1e9;const wchar_t*name=nullptr;for(auto &v:c){double d=(v.a-lat)*(v.a-lat)+(v.b-lo)*(v.b-lo);if(d<best){best=d;name=v.n;}}return best<324?std::wstring(name)+L"（附近已编入城市）":L"本区暂无已编入的主要城市";}
static HFONT uiFontFor(int size,bool bold){
  for(const auto& entry:gUiFonts)if(entry.size==size&&entry.bold==bold)return entry.handle;
  LOGFONTW spec{};spec.lfHeight=-size;spec.lfWeight=bold?FW_BOLD:FW_NORMAL;spec.lfCharSet=DEFAULT_CHARSET;spec.lfOutPrecision=OUT_DEFAULT_PRECIS;spec.lfClipPrecision=CLIP_DEFAULT_PRECIS;spec.lfQuality=CLEARTYPE_QUALITY;spec.lfPitchAndFamily=DEFAULT_PITCH;lstrcpynW(spec.lfFaceName,L"Microsoft YaHei UI",LF_FACESIZE);
  HFONT handle=CreateFontIndirectW(&spec);if(handle)gUiFonts.push_back(UiFontEntry{size,bold,handle});return handle?handle:(HFONT)GetStockObject(DEFAULT_GUI_FONT);
}
static void clearUiFonts(){for(const auto& entry:gUiFonts)DeleteObject(entry.handle);gUiFonts.clear();}
static void text(HDC hdc,const std::wstring&s,RECT r,int size,COLORREF color,bool bold=false){HFONT f=uiFontFor(size,bold);auto old=(HFONT)SelectObject(hdc,f);SetTextColor(hdc,color);SetBkMode(hdc,TRANSPARENT);DrawTextW(hdc,s.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(hdc,old);}
static int measureText(HDC hdc,const std::wstring& s,int width,int size){HFONT f=uiFontFor(size,false);auto old=(HFONT)SelectObject(hdc,f);RECT r{0,0,width,0};DrawTextW(hdc,s.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX|DT_CALCRECT);SelectObject(hdc,old);return std::max(size+2,(int)(r.bottom-r.top));}
static bool isToggleButtonId(int id){return id==1001||id==1004||id==1005||id==1006||id==1007||id==1009||id==1011||id==1014||id==1020;}
static LRESULT CALLBACK styledButtonSubclass(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam,UINT_PTR id,DWORD_PTR){
  if(isToggleButtonId((int)id)&&message==BM_GETCHECK)return GetWindowLongPtrW(hwnd,GWLP_USERDATA)?BST_CHECKED:BST_UNCHECKED;
  if(isToggleButtonId((int)id)&&message==BM_SETCHECK){SetWindowLongPtrW(hwnd,GWLP_USERDATA,wParam==BST_CHECKED?1:0);InvalidateRect(hwnd,nullptr,TRUE);return 0;}
  if(message==WM_MOUSEMOVE){if(gHoveredButtonId!=(int)id){gHoveredButtonId=(int)id;InvalidateRect(hwnd,nullptr,FALSE);}TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);}
  if(message==WM_MOUSELEAVE){if(gHoveredButtonId==(int)id)gHoveredButtonId=0;InvalidateRect(hwnd,nullptr,FALSE);}
  if(message==WM_NCDESTROY)RemoveWindowSubclass(hwnd,styledButtonSubclass,id);
  return DefSubclassProc(hwnd,message,wParam,lParam);
}
static bool styledButtonActive(int id,HWND hwnd){if(isToggleButtonId(id))return SendMessageW(hwnd,BM_GETCHECK,0,0)==BST_CHECKED;if(id==1015)return gHistoryPlaying||gHistoryPaused;if(id==1016)return gHistorySpeedIndex!=0;if(id==1018)return gInfoTab==0;if(id==1019)return gInfoTab==1;if(id==1023)return gMapFullscreen;return false;}
static void drawStyledButton(const DRAWITEMSTRUCT* item){
  int id=(int)item->CtlID;RECT r=item->rcItem;bool active=styledButtonActive(id,item->hwndItem),hover=gHoveredButtonId==id,pressed=(item->itemState&ODS_SELECTED)!=0,disabled=(item->itemState&ODS_DISABLED)!=0;
  COLORREF fill=active?RGB(13,91,111):RGB(18,43,57),edge=active?RGB(61,190,194):RGB(53,91,108),foreground=RGB(222,238,244);
  if(hover&&!active){fill=RGB(27,63,79);edge=RGB(90,151,170);}if(pressed){fill=RGB(8,121,143);edge=RGB(100,220,220);}if(disabled){fill=RGB(25,37,45);edge=RGB(55,68,74);foreground=RGB(128,143,150);}
  int saved=SaveDC(item->hDC);SetBkMode(item->hDC,TRANSPARENT);HBRUSH brush=CreateSolidBrush(fill);HPEN pen=CreatePen(PS_SOLID,1,edge);HGDIOBJ oldBrush=SelectObject(item->hDC,brush),oldPen=SelectObject(item->hDC,pen);RoundRect(item->hDC,r.left,r.top,r.right,r.bottom,8,8);SelectObject(item->hDC,oldBrush);SelectObject(item->hDC,oldPen);DeleteObject(brush);DeleteObject(pen);
  if((id==1018||id==1019)&&active){HBRUSH accent=CreateSolidBrush(RGB(99,227,208));RECT line{r.left+8,r.bottom-3,r.right-8,r.bottom-2};FillRect(item->hDC,&line,accent);DeleteObject(accent);}
  if(id==1018||id==1019){HPEN glyphPen=CreatePen(PS_SOLID,1,foreground);HGDIOBJ oldGlyph=SelectObject(item->hDC,glyphPen);int gx=r.left+12,gy=(r.top+r.bottom)/2;if(id==1018){Ellipse(item->hDC,gx-6,gy-6,gx+6,gy+6);MoveToEx(item->hDC,gx-6,gy,nullptr);LineTo(item->hDC,gx+6,gy);MoveToEx(item->hDC,gx,gy-6,nullptr);LineTo(item->hDC,gx,gy+6);}else{for(int row=0;row<3;row++){int yy=gy-5+row*5;Ellipse(item->hDC,gx-6,yy-1,gx-4,yy+1);MoveToEx(item->hDC,gx-1,yy,nullptr);LineTo(item->hDC,gx+7,yy);}}SelectObject(item->hDC,oldGlyph);DeleteObject(glyphPen);}
  wchar_t label[256]{};GetWindowTextW(item->hwndItem,label,256);HFONT font=uiFontFor(9,true),oldFont=(HFONT)SelectObject(item->hDC,font);SetTextColor(item->hDC,foreground);RECT textRect=r;InflateRect(&textRect,-5,-1);if(id==1018||id==1019)textRect.left+=10;DrawTextW(item->hDC,label,-1,&textRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(item->hDC,oldFont);
  if(item->itemState&ODS_FOCUS){RECT focus=r;InflateRect(&focus,-4,-4);DrawFocusRect(item->hDC,&focus);}RestoreDC(item->hDC,saved);
}
static void drawCalibrationPoints(Graphics& gr){
  struct Point{const wchar_t* label;double lat,lon;};
  static const Point points[]={{L"0°,0°",0,0},{L"纽约",40.7128,-74.0060},{L"基多",-0.1807,-78.4678},{L"北京",39.9042,116.4074},{L"新加坡",1.3521,103.8198},{L"开普敦",-33.9249,18.4241},{L"悉尼",-33.8688,151.2093}};
  Pen marker(Color(255,255,86,112),2);SolidBrush dot(Color(235,255,225,90)),labelBrush(Color(255,255,245,220));Font labelFont(L"Microsoft YaHei UI",10,FontStyleBold,UnitPixel);
  for(const auto& p:points){
    int x=mapScreenX((p.lon+180.0)/360.0);
    int y=mapScreenY((90.0-p.lat)/180.0);
    gr.FillEllipse(&dot,x-4,y-4,8,8);gr.DrawEllipse(&marker,x-5,y-5,10,10);
    gr.DrawString(p.label,-1,&labelFont,PointF((REAL)x+6,(REAL)y-11),&labelBrush);
  }
}
static void drawSelectedPoint(Graphics& gr){
  int x=0,y=0;if(!selectedPointScreenPosition(x,y))return;
  int radius=std::max(5,std::min(9,(int)std::lround(5.0+1.25*std::log(gMapZoom)/std::log(2.0))));
  Pen halo(Color(245,5,18,30),5),cross(Color(255,55,224,244),2),ring(Color(255,246,252,255),2);SolidBrush center(Color(255,55,224,244));
  gr.DrawLine(&halo,x-12,y,x+12,y);gr.DrawLine(&halo,x,y-12,x,y+12);gr.DrawEllipse(&halo,x-radius-1,y-radius-1,2*radius+2,2*radius+2);
  gr.DrawLine(&cross,x-12,y,x+12,y);gr.DrawLine(&cross,x,y-12,x,y+12);gr.DrawEllipse(&ring,x-radius,y-radius,2*radius,2*radius);gr.FillEllipse(&center,x-2,y-2,4,4);
}
static void drawLandCoverOverlay(Graphics& gr){
  if(!gShowLandCover||!gLandCoverLoaded||!gLandCoverRaster)return;
  int x1=mapScreenX(0),x2=mapScreenX(1),y1=mapScreenY(0),y2=mapScreenY(1);InterpolationMode previous=gr.GetInterpolationMode();gr.SetInterpolationMode(InterpolationModeNearestNeighbor);gr.DrawImage(gLandCoverRaster,x1,y1,x2-x1,y2-y1);gr.SetInterpolationMode(previous);
}
static void drawLandCoverLegend(Graphics& gr,int mapWidth,int mapHeight){
  gLandCoverLegendRect=RECT{0,0,0,0};if(!gShowLandCover||!gLandCoverLoaded||gLandCoverLegend.size()!=22)return;
  if(mapWidth<350||mapHeight<250){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));gr.DrawString(L"放大窗口可查看完整图例",-1,&hintFont,PointF((REAL)gMapRect.left+10,(REAL)gMapRect.top+76),&hintBrush);return;}
  int left=gMapRect.left+9,top=gMapRect.bottom-207,width=std::max(330,std::min(430,mapWidth-18)),height=198;gLandCoverLegendRect=RECT{left,top,left+width,top+height};SolidBrush panel(Color(255,6,19,31));Pen border(Color(240,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);
  Font titleFont(L"Microsoft YaHei UI",10,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,240,246,249));gr.DrawString(L"2020 土地覆盖主类 · 分区主类着色",-1,&titleFont,PointF((REAL)left+8,(REAL)top+4),&titleBrush);
  int columnWidth=(width-24)/2;StringFormat labelFormat;labelFormat.SetTrimming(StringTrimmingEllipsisCharacter);labelFormat.SetFormatFlags(StringFormatFlagsNoWrap);
  for(size_t i=0;i<gLandCoverLegend.size();i++){int column=(int)(i/11),row=(int)(i%11),x=left+8+column*columnWidth,y=top+24+row*15;const auto& category=gLandCoverLegend[i];BYTE red=(BYTE)((category.rgb>>16)&255),green=(BYTE)((category.rgb>>8)&255),blue=(BYTE)(category.rgb&255);SolidBrush swatch(Color(255,red,green,blue));Pen outline(Color(220,245,250,250),1);gr.FillRectangle(&swatch,x,y+2,10,10);gr.DrawRectangle(&outline,x,y+2,10,10);std::wstring label=std::to_wstring(category.code)+L" "+category.name;RectF labelRect((REAL)x+14,(REAL)y,(REAL)std::max(1,columnWidth-26),15);gr.DrawString(label.c_str(),-1,&labelFont,labelRect,&labelFormat,&labelBrush);}
}
static void drawPopulationOverlay(Graphics& gr){
  if(!gShowPopulation||!gPopulationLoaded||!gPopulationRaster)return;
  int x1=mapScreenX(0),x2=mapScreenX(1),y1=mapScreenY(0),y2=mapScreenY(1);InterpolationMode previous=gr.GetInterpolationMode();gr.SetInterpolationMode(InterpolationModeNearestNeighbor);gr.DrawImage(gPopulationRaster,x1,y1,x2-x1,y2-y1);gr.SetInterpolationMode(previous);
}
static ColorMatrix fillOpacityMatrix(float layerAlpha=1.0f){ColorMatrix matrix{};matrix.m[0][0]=1;matrix.m[1][1]=1;matrix.m[2][2]=1;matrix.m[3][3]=(gFillOpacity/100.0f)*std::max(0.0f,std::min(1.0f,layerAlpha));matrix.m[4][4]=1;return matrix;}
static void drawWorld2026Overlay(Graphics& gr){
  if(!gShowWorld2026||!gWorld2026Loaded||!gWorld2026Raster)return;
  if(gFillOpacity<=0)return;int x1=mapScreenX(0),x2=mapScreenX(1),y1=mapScreenY(0),y2=mapScreenY(1);InterpolationMode previous=gr.GetInterpolationMode();gr.SetInterpolationMode(InterpolationModeNearestNeighbor);ColorMatrix matrix=fillOpacityMatrix();ImageAttributes attributes;attributes.SetColorMatrix(&matrix,ColorMatrixFlagsDefault,ColorAdjustTypeBitmap);gr.DrawImage(gWorld2026Raster,Rect(x1,y1,x2-x1,y2-y1),0,0,FINE_GRID_COLUMNS,FINE_GRID_ROWS,UnitPixel,&attributes);gr.SetInterpolationMode(previous);
}
static void drawHistoryOverlay(Graphics& gr){
  if(!gShowHistory||!gHistoryLoaded||!gHistoryRaster||gFillOpacity<=0)return;int x1=mapScreenX(0),x2=mapScreenX(1),y1=mapScreenY(0),y2=mapScreenY(1);InterpolationMode previous=gr.GetInterpolationMode();gr.SetInterpolationMode(InterpolationModeNearestNeighbor);Rect target(x1,y1,x2-x1,y2-y1);
  auto drawRaster=[&](Bitmap* raster,float alpha){if(!raster||alpha<=0)return;ColorMatrix matrix=fillOpacityMatrix(alpha);ImageAttributes attributes;attributes.SetColorMatrix(&matrix,ColorMatrixFlagsDefault,ColorAdjustTypeBitmap);gr.DrawImage(raster,target,0,0,FINE_GRID_COLUMNS,FINE_GRID_ROWS,UnitPixel,&attributes);};
  if(gHistoryTransitionActive&&gHistoryTransitionPreviousRaster){float progress=historyTransitionProgress();drawRaster(gHistoryTransitionPreviousRaster,1.0f-progress);drawRaster(gHistoryRaster,progress);}else drawRaster(gHistoryRaster,1.0f);gr.SetInterpolationMode(previous);
}
static void drawWorld2026Legend(Graphics& gr,int mapWidth,int mapHeight){
  gWorld2026LegendRect=RECT{0,0,0,0};if(!gShowWorld2026||!gWorld2026Loaded||!gWorld2026Raster)return;
  if(mapWidth<300||mapHeight<125){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));gr.DrawString(L"世界2026：国别填色",-1,&hintFont,PointF((REAL)gMapRect.left+10,(REAL)gMapRect.bottom-40),&hintBrush);return;}
  int width=254,height=49,left=gMapRect.right-width-9,top=gMapRect.bottom-height-9;if(gShowHistory&&gHistoryLoaded&&gHistoryRaster&&mapWidth<width+274+27)top-=54;gWorld2026LegendRect=RECT{left,top,left+width,top+height};SolidBrush panel(Color(238,6,19,31));Pen border(Color(220,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);
  Font titleFont(L"Microsoft YaHei UI",9,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,240,246,249));gr.DrawString(L"世界 2026 · 国家／地区分色（无数值含义）",-1,&titleFont,PointF((REAL)left+7,(REAL)top+3),&titleBrush);
  static const unsigned int palette[7]={0x55a8b5,0xf1c75b,0x927bb8,0xe97870,0x70a7ce,0xe5a261,0x76b573};
  for(int i=0;i<7;i++){BYTE r=(BYTE)((palette[i]>>16)&255),g=(BYTE)((palette[i]>>8)&255),b=(BYTE)(palette[i]&255);SolidBrush swatch(Color(245,r,g,b));gr.FillRectangle(&swatch,left+8+i*33,top+25,25,13);}
}
static void drawHistoryLegend(Graphics& gr,int mapWidth,int mapHeight){
  gHistoryLegendRect=RECT{0,0,0,0};if(!gShowHistory||!gHistoryLoaded||!gHistoryRaster)return;if(mapWidth<300||mapHeight<125){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));std::wstring hint=historyYearText(gHistoryYear)+L"：历史政权填色";gr.DrawString(hint.c_str(),-1,&hintFont,PointF((REAL)gMapRect.left+10,(REAL)gMapRect.bottom-22),&hintBrush);return;}
  int width=274,height=45,left=gMapRect.left+9,top=gMapRect.bottom-height-9;if(gShowWorld2026&&gWorld2026Loaded&&gWorld2026Raster&&mapWidth<width+254+27)top-=54;gHistoryLegendRect=RECT{left,top,left+width,top+height};SolidBrush panel(Color(238,6,19,31));Pen border(Color(220,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);Font titleFont(L"Microsoft YaHei UI",9,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,224,237,241));std::wstring title=L"世界 "+historyYearText(gHistoryYear)+L" · 历史政权边界（示意）";gr.DrawString(title.c_str(),-1,&titleFont,PointF((REAL)left+7,(REAL)top+4),&titleBrush);std::wstring note=gHistoryAnimationEnabled?L"试行模式开启；中间帧不代表真实疆界":L"仅填有来源记录的区域；空白表示未收录";gr.DrawString(note.c_str(),-1,&labelFont,PointF((REAL)left+7,(REAL)top+24),&labelBrush);
}
static std::wstring formatPopulationDensity(double value){std::wostringstream out;if(value>=10000)out<<std::fixed<<std::setprecision(0)<<value/1000.0<<L"k";else if(value>=100)out<<std::fixed<<std::setprecision(0)<<value;else if(value>=10)out<<std::fixed<<std::setprecision(1)<<value;else out<<std::fixed<<std::setprecision(2)<<value;return out.str();}
static void drawPopulationLegend(Graphics& gr,int mapWidth,int mapHeight){
  gPopulationLegendRect=RECT{0,0,0,0};if(!gShowPopulation||!gPopulationLoaded||!gPopulationRaster)return;
  if(mapWidth<310||mapHeight<160){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));gr.DrawString(L"人口密度：蓝→黄→红（对数色阶）",-1,&hintFont,PointF((REAL)gMapRect.left+10,(REAL)gMapRect.top+76),&hintBrush);return;}
  int left=gMapRect.left+9,top=gMapRect.bottom-78,width=295,height=68;gPopulationLegendRect=RECT{left,top,left+width,top+height};SolidBrush panel(Color(238,6,19,31));Pen border(Color(220,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);
  Font titleFont(L"Microsoft YaHei UI",9,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,240,246,249));std::wstring title=L"WorldPop "+std::to_wstring(POPULATION_YEAR)+L" 人口密度估算 · 人/km²陆地";gr.DrawString(title.c_str(),-1,&titleFont,PointF((REAL)left+8,(REAL)top+4),&titleBrush);
  double logMax=std::log1p(std::max(1.0,gPopulationMaxDensity));for(int i=0;i<5;i++){double t=i/4.0,value=std::expm1(logMax*t);unsigned int rgb=populationColor(t);BYTE red=(BYTE)((rgb>>16)&255),green=(BYTE)((rgb>>8)&255),blue=(BYTE)(rgb&255);SolidBrush swatch(Color(220,red,green,blue));int x=left+9+i*55;gr.FillRectangle(&swatch,x,top+25,46,13);std::wstring label=i==0?L"0":formatPopulationDensity(value);gr.DrawString(label.c_str(),-1,&labelFont,PointF((REAL)x,(REAL)top+42),&labelBrush);}
}
static void drawGridOverlay(Graphics& gr){
  const int scale=activeGridScale(),columns=activeGridColumns(),rows=activeGridRows();
  const double spacingX=(gMapRect.right-gMapRect.left)*gMapZoom/columns,spacingY=(gMapRect.bottom-gMapRect.top)*gMapZoom/rows;
  const int stepX=std::max(1,(int)std::ceil(1.6/std::max(0.01,spacingX))),stepY=std::max(1,(int)std::ceil(1.6/std::max(0.01,spacingY)));
  Pen fine(Color(26,190,225,235),1),parent(Color(58,190,225,235),1),major(Color(104,190,225,235),1);
  for(int c=0;c<=columns;c++){bool isParent=scale>1&&c%scale==0,isMajor=c%(scale*10)==0;if(!isParent&&!isMajor&&c%stepX!=0)continue;int x=mapScreenX(c/(double)columns);gr.DrawLine(isMajor?&major:(isParent?&parent:&fine),x,gMapRect.top,x,gMapRect.bottom);}
  for(int r=0;r<=rows;r++){bool isParent=scale>1&&r%scale==0,isMajor=r%(scale*10)==0;if(!isParent&&!isMajor&&r%stepY!=0)continue;int y=mapScreenY(r/(double)rows);gr.DrawLine(isMajor?&major:(isParent?&parent:&fine),gMapRect.left,y,gMapRect.right,y);}
}
static void drawSelectedGridCell(Graphics& gr){
  int row=0,col=0;if(!selectedFineGridCell(row,col))return;int rows=activeGridRows(),columns=activeGridColumns();
  int x1=mapScreenX(col/(double)columns),x2=mapScreenX((col+1)/(double)columns),y1=mapScreenY(row/(double)rows),y2=mapScreenY((row+1)/(double)rows);
  SolidBrush b(Color(72,255,204,65));gr.FillRectangle(&b,x1,y1,std::max(1,x2-x1),std::max(1,y2-y1));Pen sel(Color(255,255,209,74),2);gr.DrawRectangle(&sel,x1,y1,std::max(1,x2-x1),std::max(1,y2-y1));
}
static MapSceneCacheKey currentMapSceneCacheKey(int width,int height){return MapSceneCacheKey{width,height,gMapRect,gMap,gLandCoverRaster,gPopulationRaster,gWorld2026Raster,gHistoryRaster,gMapZoom,gMapCenterX,gMapCenterY,gSelectedLatitude,gSelectedLongitude,gRow,gCol,gGridLevel,gFillOpacity,gHistoryYear,gSelectedCoordinateExact,gShowControlPoints,gShowLandCover,gShowPopulation,gShowWorld2026,gShowHistory,gShowGridLines,gNasaDetailAvailable,gNasa1kmAvailable,gNasa500mAvailable};}
static bool sameMapSceneCacheKey(const MapSceneCacheKey& a,const MapSceneCacheKey& b){return a.width==b.width&&a.height==b.height&&a.mapRect.left==b.mapRect.left&&a.mapRect.top==b.mapRect.top&&a.mapRect.right==b.mapRect.right&&a.mapRect.bottom==b.mapRect.bottom&&a.map==b.map&&a.landCover==b.landCover&&a.population==b.population&&a.world2026==b.world2026&&a.history==b.history&&a.zoom==b.zoom&&a.centerX==b.centerX&&a.centerY==b.centerY&&a.selectedLatitude==b.selectedLatitude&&a.selectedLongitude==b.selectedLongitude&&a.row==b.row&&a.col==b.col&&a.gridLevel==b.gridLevel&&a.fillOpacity==b.fillOpacity&&a.historyYear==b.historyYear&&a.selectedExact==b.selectedExact&&a.showControlPoints==b.showControlPoints&&a.showLandCover==b.showLandCover&&a.showPopulation==b.showPopulation&&a.showWorld2026==b.showWorld2026&&a.showHistory==b.showHistory&&a.showGridLines==b.showGridLines&&a.nasaDetail==b.nasaDetail&&a.nasa1km==b.nasa1km&&a.nasa500m==b.nasa500m;}
static bool sameMapSceneCacheKeyForViewPreview(MapSceneCacheKey current,const MapSceneCacheKey& cached){current.zoom=cached.zoom;current.centerX=cached.centerX;current.centerY=cached.centerY;return sameMapSceneCacheKey(current,cached);}
static bool mapViewPreviewCanUseCache(const MapSceneCacheKey& current){return gMapViewPreviewActive&&gMapSceneCache&&gMapSceneCacheValid&&sameMapSceneCacheKeyForViewPreview(current,gMapSceneCacheKey);}
static void drawMapSceneCache(Graphics& gr,int clientWidth,int clientHeight){
  if(!gMapSceneCache)return;MapSceneCacheKey current=currentMapSceneCacheKey(clientWidth,clientHeight);
  if(!mapViewPreviewCanUseCache(current)){gr.DrawImage(gMapSceneCache,0,0);return;}
  double scale=gMapZoom/std::max(0.0001,gMapSceneCacheKey.zoom),centerX=(gMapRect.left+gMapRect.right)/2.0,centerY=(gMapRect.top+gMapRect.bottom)/2.0;
  double dx=centerX+(gMapSceneCacheKey.centerX-gMapCenterX)*(gMapRect.right-gMapRect.left)*gMapZoom-centerX*scale;
  double dy=centerY+(gMapSceneCacheKey.centerY-gMapCenterY)*(gMapRect.bottom-gMapRect.top)*gMapZoom-centerY*scale;
  gr.SetInterpolationMode(InterpolationModeBilinear);gr.DrawImage(gMapSceneCache,RectF((REAL)dx,(REAL)dy,(REAL)(clientWidth*scale),(REAL)(clientHeight*scale)));
}
static bool sameMapBaseCacheKey(const MapBaseCacheKey& a,const MapBaseCacheKey& b){return a.width==b.width&&a.height==b.height&&a.mapRect.left==b.mapRect.left&&a.mapRect.top==b.mapRect.top&&a.mapRect.right==b.mapRect.right&&a.mapRect.bottom==b.mapRect.bottom&&a.map==b.map&&a.zoom==b.zoom&&a.centerX==b.centerX&&a.centerY==b.centerY&&a.nasaDetail==b.nasaDetail&&a.nasa1km==b.nasa1km&&a.nasa500m==b.nasa500m;}
static bool renderMapSceneCache(int clientWidth,int clientHeight){
  if(clientWidth<=0||clientHeight<=0||gMapRect.right<=gMapRect.left||gMapRect.bottom<=gMapRect.top)return false;
  MapSceneCacheKey key=currentMapSceneCacheKey(clientWidth,clientHeight);
  if(gMapSceneCache&&gMapSceneCacheValid&&sameMapSceneCacheKey(key,gMapSceneCacheKey))return true;
  if(mapViewPreviewCanUseCache(key))return true;
  delete gMapSceneCache;gMapSceneCache=nullptr;gMapSceneCacheValid=false;
  int availW=gMapFullscreen?clientWidth:clientWidth-360-36-20,availH=gMapFullscreen?clientHeight:std::max(1,clientHeight-108-116);double ar=gMap?double(gMap->GetWidth())/gMap->GetHeight():2.0;int mw=availW,mh=(int)(mw/ar);if(mh>availH){mh=availH;mw=(int)(mh*ar);}
  MapBaseCacheKey baseKey{clientWidth,clientHeight,gMapRect,gMap,gMapZoom,gMapCenterX,gMapCenterY,gNasaDetailAvailable,gNasa1kmAvailable,gNasa500mAvailable};
  if(!gMapBaseCache||!gMapBaseCacheValid||!sameMapBaseCacheKey(baseKey,gMapBaseCacheKey)){
    delete gMapBaseCache;gMapBaseCache=nullptr;gMapBaseCacheValid=false;Bitmap* base=new Bitmap(clientWidth,clientHeight,PixelFormat32bppARGB);if(!base||base->GetLastStatus()!=Ok){delete base;return false;}Graphics baseGraphics(base);if(baseGraphics.GetLastStatus()!=Ok){delete base;return false;}baseGraphics.Clear(Color(255,2,8,12));baseGraphics.SetClip(Rect(gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top));int imageLeft=mapScreenX(0),imageTop=mapScreenY(0),imageRight=mapScreenX(1),imageBottom=mapScreenY(1);if(gMap)baseGraphics.DrawImage(gMap,imageLeft,imageTop,imageRight-imageLeft,imageBottom-imageTop);drawNasaDetailTiles(baseGraphics);gMapBaseCache=base;gMapBaseCacheKey=baseKey;gMapBaseCacheValid=true;
  }
  Bitmap* candidate=new Bitmap(clientWidth,clientHeight,PixelFormat32bppARGB);if(!candidate||candidate->GetLastStatus()!=Ok){delete candidate;return false;}
  Graphics gr(candidate);if(gr.GetLastStatus()!=Ok){delete candidate;return false;}
  gr.Clear(Color(255,2,8,12));gr.DrawImage(gMapBaseCache,0,0);gr.SetClip(Rect(gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top));
  drawLandCoverOverlay(gr);drawPopulationOverlay(gr);drawWorld2026Overlay(gr);drawHistoryOverlay(gr);
  if(gShowGridLines)drawGridOverlay(gr);drawSelectedGridCell(gr);
  if(gShowControlPoints)drawCalibrationPoints(gr);drawSelectedPoint(gr);drawLandCoverLegend(gr,mw,mh);drawPopulationLegend(gr,mw,mh);drawWorld2026Legend(gr,mw,mh);drawHistoryLegend(gr,mw,mh);
  gMapSceneCache=candidate;gMapSceneCacheKey=key;gMapSceneCacheValid=true;return true;
}
static void paintHistoryEvents(HDC h){
  int left=gInfoRect.left+16,right=gInfoRect.right-18,width=std::max(120,right-left),headerY=gInfoRect.top+54,bodyTop=gInfoRect.top+104,trackBottom=gInfoRect.bottom-12;
  text(h,L"播放时间："+historyYearText(gHistoryYear),RECT{left,headerY,right,headerY+22},14,RGB(241,249,251),true);
  text(h,L"离线 "+std::to_wstring(gHistoryEvents.size())+L" 条 · 自动滚动 · 滚轮暂停",RECT{left,headerY+24,right,headerY+43},10,RGB(129,194,207));
  if(!gHistoryEventsLoaded){std::wstring error=L"历史事件模块无法读取。\n请检查 data\\world-events.tsv";text(h,error,RECT{left,bodyTop,right,bodyTop+72},12,RGB(235,173,151));gContentHeight=0;gViewportHeight=trackBottom-bodyTop;return;}
  const std::vector<size_t>& events=gHistoryEventRelayMode?gHistoryRelayEvents:historyEventsForCurrentRange();bool nearby=!gHistoryEventRelayMode&&events.size()>0&&(gHistoryEvents[events.front()].year<gHistoryEventRangeStart||gHistoryEvents[events.front()].year>gHistoryEventRangeEnd);
  std::wstring rangeLabel=gHistoryEventRelayMode?L"时间线接力 · 事件按年代连续进入":(nearby?L"附近的已收录事件（本段没有事件条目）":L"本段播放事件 · "+std::to_wstring(events.size())+L" 条");
  if(gHistoryEventRelayMode&&events.empty()){text(h,rangeLabel,RECT{left,bodyTop,right,bodyTop+20},11,RGB(112,222,225),true);text(h,L"当前年代暂无已收录事件；后续事件会接续进入滚动队列。",RECT{left,bodyTop+34,right,bodyTop+78},11,RGB(180,210,223));gContentHeight=0;gViewportHeight=std::max(1,trackBottom-bodyTop);gHistoryEventCycleHeight=0;return;}
  text(h,rangeLabel,RECT{left,bodyTop,right,bodyTop+20},11,RGB(112,222,225),true);bodyTop+=26;gViewportHeight=std::max(1,trackBottom-bodyTop);gContentHeight=0;
  if(gHistoryEventDescriptionWidth!=width||gHistoryEventDescriptionHeights.size()!=gHistoryEvents.size()){gHistoryEventDescriptionWidth=width;gHistoryEventDescriptionHeights.assign(gHistoryEvents.size(),0);}
  std::vector<int> heights;heights.reserve(events.size());int baseContentHeight=0;for(size_t index:events){int& desc=gHistoryEventDescriptionHeights[index];if(desc<=0)desc=measureText(h,gHistoryEvents[index].description,width-18,11);int height=14+18+22+17+desc+16+9;heights.push_back(height);baseContentHeight+=height+7;}
  gHistoryEventCycleHeight=baseContentHeight;if(gHistoryEventFractionPending){gScrollOffset=baseContentHeight>0?(int)(gHistoryEventScrollFraction*baseContentHeight):0;gHistoryEventFractionPending=false;}
  int copies=baseContentHeight>gViewportHeight&&baseContentHeight>0?2:1;if(gHistoryEventRelayMode&&gHistoryPlaying)copies=1;gContentHeight=baseContentHeight*copies;
  gScrollOffset=std::max(0,std::min(gScrollOffset,std::max(0,gContentHeight-gViewportHeight)));int saved=SaveDC(h);IntersectClipRect(h,left,bodyTop,right+1,trackBottom);
  for(int copy=0;copy<copies;copy++){int y=bodyTop-gScrollOffset+copy*baseContentHeight;
    for(size_t i=0;i<events.size();i++){
      const HistoryEvent& event=gHistoryEvents[events[i]];int height=heights[i];if(y+height<=bodyTop||y>=trackBottom){y+=height+7;continue;}RECT card{left,y,right,y+height};HBRUSH fill=CreateSolidBrush(RGB(16,43,57));HPEN edge=CreatePen(PS_SOLID,1,RGB(36,83,101));HGDIOBJ oldBrush=SelectObject(h,fill),oldPen=SelectObject(h,edge);RoundRect(h,card.left,card.top,card.right,card.bottom,8,8);SelectObject(h,oldBrush);SelectObject(h,oldPen);DeleteObject(fill);DeleteObject(edge);
      int tx=left+9,ty=y+6,textRight=right-9,desc=gHistoryEventDescriptionHeights[events[i]];text(h,historyYearText(event.year)+L"  ·  "+event.category,RECT{tx,ty,textRight,ty+16},10,RGB(112,222,225),true);ty+=19;text(h,event.title,RECT{tx,ty,textRight,ty+21},12,RGB(241,249,251),true);ty+=22;text(h,L"地点："+event.place,RECT{tx,ty,textRight,ty+16},10,RGB(169,202,213));ty+=17;text(h,event.description,RECT{tx,ty,textRight,ty+desc},11,RGB(222,235,241));ty+=desc+3;text(h,L"资料：Wikipedia 条目（详见随附事件数据）",RECT{tx,ty,textRight,ty+14},9,RGB(128,175,190));y+=height+7;
    }
  }
  RestoreDC(h,saved);if(gContentHeight>gViewportHeight&&gViewportHeight>0){HBRUSH track=CreateSolidBrush(RGB(24,49,62));RECT tr{gInfoRect.right-11,bodyTop,gInfoRect.right-7,trackBottom};FillRect(h,&tr,track);DeleteObject(track);int trackH=trackBottom-bodyTop,thumbH=std::max(22,trackH*gViewportHeight/gContentHeight);int thumbTop=bodyTop+(trackH-thumbH)*gScrollOffset/(gContentHeight-gViewportHeight);HBRUSH thumb=CreateSolidBrush(RGB(74,157,177));RECT th{gInfoRect.right-12,thumbTop,gInfoRect.right-6,thumbTop+thumbH};FillRect(h,&th,thumb);DeleteObject(thumb);}
}
static void drawGlobeEmblem(Graphics& gr,int x,int y,int size){
  GraphicsState saved=gr.Save();gr.SetSmoothingMode(SmoothingModeAntiAlias);RectF bounds((REAL)x,(REAL)y,(REAL)size,(REAL)size);LinearGradientBrush sphere(bounds,Color(255,18,127,151),Color(255,8,48,83),LinearGradientModeForwardDiagonal);gr.FillEllipse(&sphere,bounds);Pen rim(Color(235,132,231,223),1.4f);gr.DrawEllipse(&rim,bounds);
  GraphicsPath clip;clip.AddEllipse(bounds);gr.SetClip(&clip,CombineModeIntersect);Pen grid(Color(130,166,235,230),1.0f);gr.DrawArc(&grid,x+size*.30f,y+size*.06f,size*.40f,size*.88f,90,180);gr.DrawArc(&grid,x+size*.30f,y+size*.06f,size*.40f,size*.88f,270,180);gr.DrawArc(&grid,x+size*.08f,y+size*.32f,size*.84f,size*.36f,0,360);gr.DrawLine(&grid,x+size*.08f,y+size*.50f,x+size*.92f,y+size*.50f);
  SolidBrush land(Color(220,109,198,151));PointF continentA[]={{(REAL)(x+size*.24f),(REAL)(y+size*.31f)},{(REAL)(x+size*.38f),(REAL)(y+size*.22f)},{(REAL)(x+size*.46f),(REAL)(y+size*.30f)},{(REAL)(x+size*.42f),(REAL)(y+size*.39f)},{(REAL)(x+size*.31f),(REAL)(y+size*.43f)}};gr.FillPolygon(&land,continentA,5);PointF continentB[]={{(REAL)(x+size*.55f),(REAL)(y+size*.47f)},{(REAL)(x+size*.73f),(REAL)(y+size*.40f)},{(REAL)(x+size*.81f),(REAL)(y+size*.50f)},{(REAL)(x+size*.71f),(REAL)(y+size*.59f)},{(REAL)(x+size*.60f),(REAL)(y+size*.57f)}};gr.FillPolygon(&land,continentB,5);gr.Restore(saved);
  SolidBrush marker(Color(255,255,205,112));gr.FillEllipse(&marker,(REAL)(x+size*.68f),(REAL)(y+size*.16f),(REAL)(size*.11f),(REAL)(size*.11f));
}
static void paint(HWND w,HDC h){
  RECT client;GetClientRect(w,&client);HBRUSH bg=CreateSolidBrush(RGB(7,20,33));FillRect(h,&client,bg);DeleteObject(bg);
  if(gMapFullscreen){double ar=gMap?double(gMap->GetWidth())/gMap->GetHeight():2.0;int mw=client.right,mh=std::max(1,(int)(mw/ar));if(mh>client.bottom){mh=client.bottom;mw=std::max(1,(int)(mh*ar));}int mx=(client.right-mw)/2,my=(client.bottom-mh)/2;gInfoRect=RECT{0,0,0,0};gMapRect=RECT{mx,my,mx+mw,my+mh};if(renderMapSceneCache(client.right,client.bottom)){Graphics scene(h);scene.SetClip(Rect(gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top),CombineModeIntersect);drawMapSceneCache(scene,client.right,client.bottom);}else{Graphics scene(h);scene.SetClip(Rect(gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top),CombineModeIntersect);int left=mapScreenX(0),top=mapScreenY(0),right=mapScreenX(1),bottom=mapScreenY(1);if(gMap)scene.DrawImage(gMap,left,top,right-left,bottom-top);drawNasaDetailTiles(scene);drawLandCoverOverlay(scene);drawPopulationOverlay(scene);drawWorld2026Overlay(scene);drawHistoryOverlay(scene);if(gShowGridLines)drawGridOverlay(scene);drawSelectedGridCell(scene);drawSelectedPoint(scene);}Graphics overlay(h);overlay.SetSmoothingMode(SmoothingModeAntiAlias);SolidBrush hintPanel(Color(205,7,25,38));Pen hintBorder(Color(180,83,150,170),1);overlay.FillRectangle(&hintPanel,client.right-222,18,204,36);overlay.DrawRectangle(&hintBorder,client.right-222,18,204,36);Font hintFont(L"Microsoft YaHei UI",10,FontStyleBold,UnitPixel);SolidBrush hintText(Color(245,225,242,246));overlay.DrawString(L"F11 / Esc  退出地图全屏",-1,&hintFont,PointF((REAL)client.right-210,29),&hintText);return;}
  RECT header{0,0,client.right,72};HBRUSH head=CreateSolidBrush(RGB(8,64,91));FillRect(h,&header,head);DeleteObject(head);HPEN headerLine=CreatePen(PS_SOLID,1,RGB(43,128,148));HGDIOBJ oldHeaderPen=SelectObject(h,headerLine);MoveToEx(h,0,71,nullptr);LineTo(h,client.right,71);SelectObject(h,oldHeaderPen);DeleteObject(headerLine);
  Graphics emblem(h);drawGlobeEmblem(emblem,23,13,44);text(h,L"世界地图分区档案",RECT{76,12,500,43},22,RGB(238,250,255),true);text(h,L"历史疆域 · 627,200 细格 · 可离线运行 · 版本 "+std::wstring(APP_VERSION),RECT{77,42,690,66},11,RGB(180,220,235));text(h,gNotice,RECT{700,40,client.right-116,66},11,RGB(214,235,241));
  int right=360,margin=18;gInfoRect={client.right-right+8,90,client.right-18,client.bottom-18};HBRUSH card=CreateSolidBrush(RGB(11,32,47));HPEN cardBorder=CreatePen(PS_SOLID,1,RGB(28,70,88));HGDIOBJ oldCardBrush=SelectObject(h,card),oldCardPen=SelectObject(h,cardBorder);RoundRect(h,gInfoRect.left,gInfoRect.top,gInfoRect.right,gInfoRect.bottom,12,12);SelectObject(h,oldCardBrush);SelectObject(h,oldCardPen);DeleteObject(card);DeleteObject(cardBorder);
  int toolbarRight=(int)client.right-right-13;HBRUSH toolbarBrush=CreateSolidBrush(RGB(10,29,43));HPEN toolbarPen=CreatePen(PS_SOLID,1,RGB(24,64,81));HGDIOBJ oldToolbarBrush=SelectObject(h,toolbarBrush),oldToolbarPen=SelectObject(h,toolbarPen);RoundRect(h,16,74,toolbarRight,151,12,12);SelectObject(h,oldToolbarBrush);SelectObject(h,oldToolbarPen);DeleteObject(toolbarBrush);DeleteObject(toolbarPen);
  int availW=std::max(1,(int)client.right-right-margin*2-20),availH=std::max(1,(int)client.bottom-108-116);double ar=gMap?double(gMap->GetWidth())/gMap->GetHeight():2.0;int mw=availW,mh=std::max(1,(int)(mw/ar));if(mh>availH){mh=availH;mw=std::max(1,(int)(mh*ar));}int mx=24,my=90+116+(availH-mh)/2;gMapRect={mx,my,mx+mw,my+mh};
  if(gInfoTab==1)paintHistoryEvents(h);
  else if(gCol<0){text(h,L"格网定位",RECT{gInfoRect.left+20,gInfoRect.top+64,gInfoRect.right-18,gInfoRect.top+90},16,RGB(238,250,255),true);text(h,L"选择格网层级，再点击地图查看细格坐标与父格档案。细格仅用于定位，地形、历史、人口等数据仍按原 9,800 格读取。",RECT{gInfoRect.left+20,gInfoRect.top+101,gInfoRect.right-20,gInfoRect.bottom-16},13,RGB(180,210,223));gContentHeight=0;gViewportHeight=std::max(1,(int)(gInfoRect.bottom-gInfoRect.top-120));}
  else{
  double lat=90-(gRow+.5)*GRID_CELL_DEGREES,lo=-180+(gCol+.5)*GRID_CELL_DEGREES;int x=gInfoRect.left+22,y=gInfoRect.top+61;
  if(gGridLevel==0){text(h,L"分区 "+gridIdText(GRID_VERSION,gRow,gCol),RECT{x,y,gInfoRect.right-18,y+30},20,RGB(238,250,255),true);y+=32;text(h,L"格中心 "+preciseLatitude(lat)+L"，"+preciseLongitude(lo),RECT{x,y,gInfoRect.right-18,y+18},12,RGB(128,203,211));y+=19;}
  else{int fineRow=0,fineCol=0;selectedFineGridCell(fineRow,fineCol);double fineLat=90.0-(fineRow+.5)*(180.0/activeGridRows()),fineLon=-180.0+(fineCol+.5)*(360.0/activeGridColumns());text(h,L"细格 "+gridIdText(activeGridCount(),fineRow,fineCol),RECT{x,y,gInfoRect.right-18,y+30},20,RGB(238,250,255),true);y+=32;text(h,L"父级档案 "+gridIdText(GRID_VERSION,gRow,gCol),RECT{x,y,gInfoRect.right-18,y+18},12,RGB(128,203,211));y+=19;text(h,L"细格中心 "+preciseLatitude(fineLat)+L"，"+preciseLongitude(fineLon),RECT{x,y,gInfoRect.right-18,y+18},12,RGB(128,203,211));y+=19;}
  text(h,selectedCoordinateText(),RECT{x,y,gInfoRect.right-18,y+18},12,RGB(128,203,211));y+=22;
  const int textRight=gInfoRect.right-29, textWidth=textRight-x;
  struct Field{const wchar_t* title;std::wstring body;};
  std::vector<Field> fields={
    {gGridLevel>0?L"父格经纬网范围":L"经纬网范围",archiveZoneField(gRow,gCol,"latitudeBand",L"")+L"；"+archiveZoneField(gRow,gCol,"longitudeBand",L"")+L"；网格 "+gridCellText()+L"° × "+gridCellText()+L"°"},
    {gGridLevel>0?L"父格单元面积":L"网格单元面积",wgs84ZoneAreaText(gRow)},
    {L"所在国家／地区",archiveZoneField(gRow,gCol,"countries",L"暂无行政区匹配")},
    {L"地形地貌",archiveZoneField(gRow,gCol,"terrain",L"尚无地貌记录")},
    {L"地表概况",archiveZoneField(gRow,gCol,"surface",L"尚无地表记录")},
    {L"地表覆盖（2020）",landCoverZoneField(gRow,gCol,"landCover",L"地表覆盖数据层未安装；可单独补丁安装")},
    {L"覆盖统计口径",landCoverZoneField(gRow,gCol,"landCoverCoverage",L"此独立数据层未安装，不影响地图与其他档案")},
    {L"生物群系",archiveZoneField(gRow,gCol,"biomes",L"尚无生物群系记录")},
    {L"生态区",archiveZoneField(gRow,gCol,"ecoregions",L"尚无生态区记录")},
    {L"生态区样点覆盖",archiveZoneField(gRow,gCol,"ecoregionCoverage",L"")},
    {L"气候带（柯本—盖格）",archiveZoneField(gRow,gCol,"climate",L"尚无气候分类记录")},
    {L"气候样点覆盖",archiveZoneField(gRow,gCol,"climateCoverage",L"")},
    {L"城市／聚落",archiveZoneField(gRow,gCol,"cities",L"尚无地点记录")},
    {L"附近主要城市",archiveZoneField(gRow,gCol,"nearby",L"尚无城市记录")},
    {L"河流／水道",archiveZoneField(gRow,gCol,"rivers",L"尚无河流记录")},
    {L"湖泊与海湾／其他水体",archiveZoneField(gRow,gCol,"lakes",L"尚无水域记录")},
    {L"山地地名／山脉与高峰",archiveZoneField(gRow,gCol,"mountains",L"尚无山地记录")},
    {L"命名地貌",archiveZoneField(gRow,gCol,"landforms",L"尚无地貌名称")},
    {L"历史地点",archiveZoneField(gRow,gCol,"history",L"尚无历史地点记录")},
    {L"世界遗产（UNESCO）",heritageZoneField(gRow,gCol,L"UNESCO 世界遗产离线数据模块未安装；可通过独立数据补丁添加。")},
    {L"曾用地名／年代别名",archiveZoneField(gRow,gCol,"formerNames",L"尚无曾用名记录")},
    {L"数据覆盖说明",archiveZoneField(gRow,gCol,"sources",L"详见随附数据说明")},
    {L"条目数量",archiveZoneField(gRow,gCol,"featureCounts",L"")},
  };
  if(gShowWorld2026)fields.insert(fields.begin()+3,{L"当前 627,200 细格国别（世界2026）",world2026ZoneField()});
  if(gShowHistory)fields.insert(fields.begin()+4,{L"历史时期政权",historyZoneField()});
  if(gGridLevel>0){int fineRow=0,fineCol=0;selectedFineGridCell(fineRow,fineCol);std::vector<Field> fineFields={{L"细格经纬范围",fineGridBoundsText(fineRow,fineCol)},{L"细格跨度",fineGridCellText()+L"° 纬 × "+fineGridCellText()+L"° 经"},{L"档案关联",gridIdText(GRID_VERSION,gRow,gCol)+L"；下方地形、历史、人口等数据均按此父格读取"}};fields.insert(fields.begin(),fineFields.begin(),fineFields.end());}
  if(gPopulationLoaded&&gPopulationRaster)fields.push_back({L"人口估算（2023）",populationZoneField(gRow,gCol,L"")});
  int bodyTop=y+4;gViewportHeight=gInfoRect.bottom-14-bodyTop;gContentHeight=0;
  for(const auto& f:fields)gContentHeight+=22+measureText(h,f.body,textWidth,12)+9;
  gScrollOffset=std::max(0,std::min(gScrollOffset,std::max(0,gContentHeight-gViewportHeight)));
  int saved=SaveDC(h);IntersectClipRect(h,gInfoRect.left+12,bodyTop,gInfoRect.right-12,gInfoRect.bottom-12);y=bodyTop-gScrollOffset;
  for(const auto& f:fields){text(h,f.title,RECT{x,y,textRight,y+20},12,RGB(112,222,225),true);y+=21;int height=measureText(h,f.body,textWidth,12);text(h,f.body,RECT{x,y,textRight,y+height},12,RGB(230,242,247));y+=height+9;}
  RestoreDC(h,saved);
  if(gContentHeight>gViewportHeight&&gViewportHeight>0){int trackTop=bodyTop,trackBottom=gInfoRect.bottom-12,trackH=trackBottom-trackTop;HBRUSH track=CreateSolidBrush(RGB(24,49,62));RECT tr{gInfoRect.right-11,trackTop,gInfoRect.right-7,trackBottom};FillRect(h,&tr,track);DeleteObject(track);int thumbH=std::max(22,trackH*gViewportHeight/gContentHeight);int thumbTop=trackTop+(trackH-thumbH)*gScrollOffset/(gContentHeight-gViewportHeight);HBRUSH thumb=CreateSolidBrush(RGB(74,157,177));RECT th{gInfoRect.right-12,thumbTop,gInfoRect.right-6,thumbTop+thumbH};FillRect(h,&th,thumb);DeleteObject(thumb);}
  }
  if(renderMapSceneCache(client.right,client.bottom)){
    Graphics gr(h);gr.SetClip(Rect(gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top),CombineModeIntersect);drawMapSceneCache(gr,client.right,client.bottom);
  }else{
    HBRUSH black=CreateSolidBrush(RGB(2,8,12));FillRect(h,&gMapRect,black);DeleteObject(black);
    Graphics gr(h);gr.SetClip(Rect(gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top),CombineModeIntersect);
    int imageLeft=mapScreenX(0),imageTop=mapScreenY(0),imageRight=mapScreenX(1),imageBottom=mapScreenY(1);
    if(gMap)gr.DrawImage(gMap,imageLeft,imageTop,imageRight-imageLeft,imageBottom-imageTop);
    drawNasaDetailTiles(gr);drawLandCoverOverlay(gr);drawPopulationOverlay(gr);drawWorld2026Overlay(gr);drawHistoryOverlay(gr);
    if(gShowGridLines)drawGridOverlay(gr);drawSelectedGridCell(gr);
    if(gShowControlPoints)drawCalibrationPoints(gr);drawSelectedPoint(gr);drawLandCoverLegend(gr,mw,mh);drawPopulationLegend(gr,mw,mh);drawWorld2026Legend(gr,mw,mh);drawHistoryLegend(gr,mw,mh);
  }
}
static void releaseBackBuffer(){
  if(gBackBufferDC&&gBackBufferOriginalBitmap)SelectObject(gBackBufferDC,gBackBufferOriginalBitmap);
  if(gBackBufferBitmap)DeleteObject(gBackBufferBitmap);
  if(gBackBufferDC)DeleteDC(gBackBufferDC);
  gBackBufferDC=nullptr;gBackBufferBitmap=nullptr;gBackBufferOriginalBitmap=nullptr;gBackBufferWidth=0;gBackBufferHeight=0;
}
static bool ensureBackBuffer(HDC reference,int width,int height){
  if(width<=0||height<=0)return false;
  if(gBackBufferDC&&gBackBufferBitmap&&gBackBufferWidth==width&&gBackBufferHeight==height)return true;
  releaseBackBuffer();gBackBufferDC=CreateCompatibleDC(reference);if(!gBackBufferDC)return false;
  gBackBufferBitmap=CreateCompatibleBitmap(reference,width,height);if(!gBackBufferBitmap){releaseBackBuffer();return false;}
  HGDIOBJ previous=SelectObject(gBackBufferDC,gBackBufferBitmap);if(!previous||previous==HGDI_ERROR){releaseBackBuffer();return false;}
  gBackBufferOriginalBitmap=(HBITMAP)previous;gBackBufferWidth=width;gBackBufferHeight=height;return true;
}
static void layoutControls(HWND w){
  if(!gControlPointsButton||!gGridLevelCombo)return;
  RECT r;GetClientRect(w,&r);
  if(gMapFullscreen){gInfoRect=RECT{0,0,0,0};double ar=gMap?double(gMap->GetWidth())/gMap->GetHeight():2.0;int mw=r.right,mh=std::max(1,(int)(mw/ar));if(mh>r.bottom){mh=r.bottom;mw=std::max(1,(int)(mh*ar));}int mx=(r.right-mw)/2,my=(r.bottom-mh)/2;gMapRect=RECT{mx,my,mx+mw,my+mh};return;}
  int right=r.right-116,gap=8,gridW=185,editW=230,searchW=60,hdW=92,pointsW=134;
  int available=std::max(0,right-520),shortage=gridW+editW+searchW+hdW+pointsW+5*gap-available;
  auto shrink=[&shortage](int&width,int minimum){int amount=std::min(shortage,std::max(0,width-minimum));width-=amount;shortage-=amount;};
  shrink(editW,120);shrink(pointsW,115);shrink(hdW,86);shrink(searchW,55);shrink(gridW,125);
  int x=right-(gridW+editW+searchW+hdW+pointsW+5*gap);
  SetWindowPos(gGridLevelCombo,nullptr,x,14,gridW,150,SWP_NOZORDER);x+=gridW+gap;
  SetWindowPos(gSearchEdit,nullptr,x,14,editW,25,SWP_NOZORDER);x+=editW+gap;
  SetWindowPos(gSearchButton,nullptr,x,14,searchW,25,SWP_NOZORDER);x+=searchW+gap;
  SetWindowPos(gHighResMapButton,nullptr,x,14,hdW,25,SWP_NOZORDER);x+=hdW+gap;
  SetWindowPos(gControlPointsButton,nullptr,x,14,pointsW,25,SWP_NOZORDER);
  const int toolbarHeight=116;
  int availW=std::max(1,(int)r.right-360-36-20),availH=std::max(1,(int)r.bottom-108-toolbarHeight);
  double ar=gMap?double(gMap->GetWidth())/gMap->GetHeight():2.0;
  int mw=availW,mh=std::max(1,(int)(mw/ar));
  if(mh>availH){mh=availH;mw=std::max(1,(int)(mh*ar));}
  int mx=24,my=90+toolbarHeight+(availH-mh)/2;
  gMapRect=RECT{mx,my,mx+mw,my+mh};
  int toolbarX=mx+9,toolbarWidth=std::max(1,mw-18),layerGap=6;
  int layerWidths[4]={167,150,145,160},layerMinimums[4]={120,120,120,130};
  int shortageLayers=layerWidths[0]+layerWidths[1]+layerWidths[2]+layerWidths[3]+3*layerGap-toolbarWidth;
  for(int i=0;i<4&&shortageLayers>0;i++){int amount=std::min(shortageLayers,layerWidths[i]-layerMinimums[i]);layerWidths[i]-=amount;shortageLayers-=amount;}
  int row1Y=80;x=toolbarX;
  if(gLandCoverToggleButton){SetWindowPos(gLandCoverToggleButton,nullptr,x,row1Y,layerWidths[0],27,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gLandCoverToggleButton,BM_SETCHECK,gShowLandCover?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(gLandCoverToggleButton,gLandCoverLoaded&&gLandCoverLegend.size()==22);}x+=layerWidths[0]+layerGap;
  if(gNasaMapButton){SetWindowPos(gNasaMapButton,nullptr,x,row1Y,layerWidths[1],27,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gNasaMapButton,BM_SETCHECK,gMap==gMapNasa?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(gNasaMapButton,gNasaMapAvailable);}x+=layerWidths[1]+layerGap;
  if(gPopulationToggleButton){SetWindowPos(gPopulationToggleButton,nullptr,x,row1Y,layerWidths[2],27,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gPopulationToggleButton,BM_SETCHECK,gShowPopulation?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(gPopulationToggleButton,gPopulationLoaded&&gPopulationRaster);}x+=layerWidths[2]+layerGap;
  if(gWorld2026ToggleButton){SetWindowPos(gWorld2026ToggleButton,nullptr,x,row1Y,layerWidths[3],27,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gWorld2026ToggleButton,BM_SETCHECK,gShowWorld2026?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(gWorld2026ToggleButton,gWorld2026Loaded&&gWorld2026Raster);}
  int yearW=132,playW=112,speedW=82,animationW=112,historyW=142,labelW=118,trackW=166,gridLinesW=132,row2Gap=7;
  int shortageRow2=yearW+playW+speedW+animationW+historyW+labelW+trackW+gridLinesW+7*row2Gap-toolbarWidth;
  int minimums[8]={90,86,62,90,120,90,100,112};int* widths[8]={&yearW,&playW,&speedW,&animationW,&historyW,&labelW,&trackW,&gridLinesW};
  for(int i=0;i<8&&shortageRow2>0;i++){int amount=std::min(shortageRow2,*widths[i]-minimums[i]);*widths[i]-=amount;shortageRow2-=amount;}
  const int row2Y=113;x=toolbarX;
  if(gHistoryYearCombo){SetWindowPos(gHistoryYearCombo,nullptr,x,row2Y,yearW,140,SWP_NOZORDER|SWP_NOACTIVATE);EnableWindow(gHistoryYearCombo,gHistoryLoaded||gHistoryError!=ERROR_SUCCESS);}x+=yearW+row2Gap;
  if(gHistoryPlaybackButton){SetWindowPos(gHistoryPlaybackButton,nullptr,x,row2Y,playW,27,SWP_NOZORDER|SWP_NOACTIVATE);updateHistoryPlaybackButton();EnableWindow(gHistoryPlaybackButton,gHistoryLoaded&&gHistoryRaster);}x+=playW+row2Gap;
  if(gHistorySpeedButton){SetWindowPos(gHistorySpeedButton,nullptr,x,row2Y,speedW,27,SWP_NOZORDER|SWP_NOACTIVATE);SetWindowTextW(gHistorySpeedButton,HISTORY_PLAYBACK_SPEEDS[gHistorySpeedIndex]);EnableWindow(gHistorySpeedButton,gHistoryLoaded&&gHistoryRaster);}x+=speedW+row2Gap;
  if(gHistoryAnimationButton){SetWindowPos(gHistoryAnimationButton,nullptr,x,row2Y,animationW,27,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gHistoryAnimationButton,BM_SETCHECK,gHistoryAnimationEnabled?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(gHistoryAnimationButton,gHistoryLoaded&&gHistoryRaster);}x+=animationW+row2Gap;
  if(gHistoryToggleButton){SetWindowPos(gHistoryToggleButton,nullptr,x,row2Y,historyW,27,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gHistoryToggleButton,BM_SETCHECK,gShowHistory?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(gHistoryToggleButton,gHistoryLoaded&&gHistoryRaster);}x+=historyW+row2Gap;
  if(gOpacityLabel){SetWindowPos(gOpacityLabel,nullptr,x,row2Y+3,labelW,22,SWP_NOZORDER|SWP_NOACTIVATE);wchar_t opacity[48];swprintf(opacity,48,L"填色透明度 %d%%",gFillOpacity);SetWindowTextW(gOpacityLabel,opacity);}x+=labelW+row2Gap;
  if(gOpacityTrackbar){SetWindowPos(gOpacityTrackbar,nullptr,x,row2Y,trackW,30,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gOpacityTrackbar,TBM_SETPOS,TRUE,gFillOpacity);EnableWindow(gOpacityTrackbar,gWorld2026Loaded||gHistoryLoaded);}x+=trackW+row2Gap;
  if(gGridLinesToggleButton){SetWindowPos(gGridLinesToggleButton,nullptr,x,row2Y,gridLinesW,27,SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(gGridLinesToggleButton,BM_SETCHECK,gShowGridLines?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(gGridLinesToggleButton,TRUE);}
  if(gFullscreenButton)SetWindowPos(gFullscreenButton,nullptr,std::max(0,(int)r.right-108),14,94,29,SWP_NOZORDER|SWP_NOACTIVATE);
  int sideLeft=r.right-352,tabTop=101,tabGap=8,tabWidth=159;if(gGeoTabButton)SetWindowPos(gGeoTabButton,nullptr,sideLeft+12,tabTop,tabWidth,32,SWP_NOZORDER|SWP_NOACTIVATE);if(gEventsTabButton)SetWindowPos(gEventsTabButton,nullptr,sideLeft+12+tabWidth+tabGap,tabTop,tabWidth,32,SWP_NOZORDER|SWP_NOACTIVATE);
}
static void setMapFullscreen(HWND w,bool enabled){
  if(enabled==gMapFullscreen)return;
  HWND controls[]={gControlPointsButton,gSearchEdit,gSearchButton,gHighResMapButton,gGridLevelCombo,gNasaMapButton,gLandCoverToggleButton,gPopulationToggleButton,gWorld2026ToggleButton,gHistoryToggleButton,gHistoryYearCombo,gOpacityTrackbar,gOpacityLabel,gGridLinesToggleButton,gHistoryPlaybackButton,gHistorySpeedButton,gHistoryAnimationButton,gGeoTabButton,gEventsTabButton,gFullscreenButton};
  if(enabled){gWindowedStyle=GetWindowLongPtrW(w,GWL_STYLE);gWindowedPlacement.length=sizeof(WINDOWPLACEMENT);GetWindowPlacement(w,&gWindowedPlacement);gMapFullscreen=true;for(HWND control:controls)if(control)ShowWindow(control,SW_HIDE);SetFocus(w);LONG_PTR style=(gWindowedStyle&~WS_OVERLAPPEDWINDOW)|WS_POPUP|WS_CLIPCHILDREN|WS_VISIBLE;SetWindowLongPtrW(w,GWL_STYLE,style);MONITORINFO monitor{sizeof(MONITORINFO)};HMONITOR target=MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST);if(GetMonitorInfoW(target,&monitor)){RECT bounds=monitor.rcMonitor;SetWindowPos(w,HWND_TOP,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_FRAMECHANGED|SWP_NOOWNERZORDER|SWP_SHOWWINDOW);}if(gFullscreenButton)SetWindowTextW(gFullscreenButton,L"退出全屏");}
  else{gMapFullscreen=false;SetWindowLongPtrW(w,GWL_STYLE,gWindowedStyle);SetWindowPlacement(w,&gWindowedPlacement);SetWindowPos(w,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED|SWP_SHOWWINDOW);for(HWND control:controls)if(control)ShowWindow(control,SW_SHOW);if(gFullscreenButton)SetWindowTextW(gFullscreenButton,L"地图全屏");SetFocus(w);}
  KillTimer(w,MAP_VIEW_SETTLE_TIMER);gMapViewPreviewActive=false;layoutControls(w);gMapSceneCacheValid=false;InvalidateRect(w,nullptr,TRUE);
}
static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM a,LPARAM b){
  if((m==WM_CTLCOLOREDIT||m==WM_CTLCOLORLISTBOX)&&gUiControlBrush){HDC dc=(HDC)a;SetTextColor(dc,RGB(230,242,247));SetBkColor(dc,RGB(18,43,57));return (LRESULT)gUiControlBrush;}
  if(m==WM_CTLCOLORSTATIC&&(HWND)b==gOpacityLabel&&gUiPanelBrush){HDC dc=(HDC)a;SetTextColor(dc,RGB(166,215,224));SetBkColor(dc,RGB(10,29,43));return (LRESULT)gUiPanelBrush;}
  if(m==WM_DRAWITEM&&((const DRAWITEMSTRUCT*)b)->CtlType==ODT_BUTTON){drawStyledButton((const DRAWITEMSTRUCT*)b);return TRUE;}
  if(m==WM_COMMAND&&LOWORD(a)==1023&&HIWORD(a)==BN_CLICKED){setMapFullscreen(w,!gMapFullscreen);return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&isToggleButtonId(LOWORD(a))){HWND button=(HWND)b;LRESULT checked=SendMessageW(button,BM_GETCHECK,0,0);SendMessageW(button,BM_SETCHECK,checked==BST_CHECKED?BST_UNCHECKED:BST_CHECKED,0);}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&(LOWORD(a)==1018||LOWORD(a)==1019)){gInfoTab=LOWORD(a)==1018?0:1;if(!gHistoryEventRelayMode)gScrollOffset=0;gHistoryEventFractionPending=false;if(gInfoTab==1)SetTimer(w,HISTORY_EVENT_SCROLL_TIMER,33,nullptr);else KillTimer(w,HISTORY_EVENT_SCROLL_TIMER);InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_ERASEBKGND)return 1;
  if(m==WM_PAINT){PAINTSTRUCT ps;HDC h=BeginPaint(w,&ps);RECT client;GetClientRect(w,&client);if(ensureBackBuffer(h,client.right,client.bottom)){int saved=SaveDC(gBackBufferDC);if(saved){IntersectClipRect(gBackBufferDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);paint(w,gBackBufferDC);RestoreDC(gBackBufferDC,saved);BitBlt(h,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,gBackBufferDC,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);}else paint(w,h);}else paint(w,h);EndPaint(w,&ps);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1008&&HIWORD(a)==CBN_SELCHANGE){int level=(int)SendMessageW(gGridLevelCombo,CB_GETCURSEL,0,0);if(level>=0&&level<=3&&level!=gGridLevel){gGridLevel=level;gScrollOffset=0;gNotice=L"格网已切换为 "+std::to_wstring(activeGridCount())+L" 格；地形、历史、人口等仍按 Z9800 父格读取";InvalidateRect(w,nullptr,TRUE);}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1010&&HIWORD(a)==CBN_SELCHANGE){if(gHistoryPlaying)KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;finishHistoryTransition(w);updateHistoryPlaybackButton();int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);setHistoryEventRange(selected,false);selectHistoryYear(w,selected);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1015&&HIWORD(a)==BN_CLICKED){
    if(gHistoryPlaying){KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=true;finishHistoryTransition(w);gNotice=L"历史播放已暂停；可继续播放或手动选择年份";updateHistoryPlaybackButton();InvalidateRect(w,nullptr,FALSE);return 0;}
    bool resume=gHistoryPaused&&gHistoryEventRelayMode;int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);if(!resume)selected=HISTORY_YEAR_COUNT-1;
    finishHistoryTransition(w);
    gShowHistory=true;if(gHistoryToggleButton){SendMessageW(gHistoryToggleButton,BM_SETCHECK,BST_CHECKED,0);EnableWindow(gHistoryToggleButton,TRUE);}if(!resume){setHistoryEventRange(selected,false);gHistoryEventRelayMode=true;appendHistoryEventsInRange(HISTORY_YEARS[selected],HISTORY_YEARS[selected]);gScrollOffset=0;gHistoryScrollPauseUntil=0;}else if(gHistoryEventCycleHeight>0)gScrollOffset%=gHistoryEventCycleHeight;
    if(!selectHistoryYear(w,selected)){gShowHistory=false;if(gHistoryToggleButton)SendMessageW(gHistoryToggleButton,BM_SETCHECK,BST_UNCHECKED,0);gHistoryPaused=false;updateHistoryPlaybackButton();return 0;}
    gHistoryPlaying=true;gHistoryPaused=false;updateHistoryPlaybackButton();gNotice=gHistoryAnimationEnabled?L"试行渐变播放：中间帧仅为视觉过渡，不代表真实疆界":L"正在从公元前3400年向公元2024年播放历史政权变化";SetTimer(w,HISTORY_PLAYBACK_TIMER,historyPlaybackInterval(),nullptr);InvalidateRect(w,nullptr,FALSE);return 0;
  }
  if(m==WM_COMMAND&&LOWORD(a)==1016&&HIWORD(a)==BN_CLICKED){gHistorySpeedIndex=(gHistorySpeedIndex+1)%4;if(gHistorySpeedButton)SetWindowTextW(gHistorySpeedButton,HISTORY_PLAYBACK_SPEEDS[gHistorySpeedIndex]);if(gHistoryPlaying)SetTimer(w,HISTORY_PLAYBACK_TIMER,historyPlaybackInterval(),nullptr);gNotice=L"历史播放速度："+std::wstring(HISTORY_PLAYBACK_SPEEDS[gHistorySpeedIndex]).substr(3);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1020&&HIWORD(a)==BN_CLICKED){gHistoryAnimationEnabled=SendMessageW(gHistoryAnimationButton,BM_GETCHECK,0,0)==BST_CHECKED;if(!gHistoryAnimationEnabled)finishHistoryTransition(w);if(gHistoryPlaying)SetTimer(w,HISTORY_PLAYBACK_TIMER,historyPlaybackInterval(),nullptr);gNotice=gHistoryAnimationEnabled?L"试行渐变已开启：中间帧仅为视觉过渡，不代表真实疆界":L"试行渐变已关闭；播放按原年份节点切换";InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_TIMER&&a==MAP_VIEW_SETTLE_TIMER){KillTimer(w,MAP_VIEW_SETTLE_TIMER);if(gMapViewPreviewActive){gMapViewPreviewActive=false;gMapSceneCacheValid=false;InvalidateRect(w,&gMapRect,FALSE);}return 0;}
  if(m==WM_TIMER&&a==HISTORY_PLAYBACK_TIMER){int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);if(selected<=0){KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;updateHistoryPlaybackButton();gNotice=L"历史播放已到最新年份公元2024年";InvalidateRect(w,nullptr,FALSE);return 0;}int next=selected-1;Bitmap* previous=gHistoryAnimationEnabled?cloneHistoryRaster():nullptr;SendMessageW(gHistoryYearCombo,CB_SETCURSEL,next,0);setHistoryEventRange(next,true);appendHistoryEventsInRange(gHistoryEventRangeStart,gHistoryEventRangeEnd);if(!selectHistoryYear(w,next)){delete previous;finishHistoryTransition(w);KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;updateHistoryPlaybackButton();return 0;}if(gHistoryAnimationEnabled){startHistoryTransition(w,previous);gNotice=L"试行渐变至"+historyYearText(HISTORY_YEARS[next])+L"；中间帧不代表真实疆界";}else delete previous;if(next==0){KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;updateHistoryPlaybackButton();gNotice=L"历史播放已到最新年份公元2024年";InvalidateRect(w,nullptr,FALSE);}return 0;}
  if(m==WM_TIMER&&a==HISTORY_TRANSITION_TIMER){if(!gHistoryTransitionActive){KillTimer(w,HISTORY_TRANSITION_TIMER);return 0;}if(historyTransitionProgress()>=1.0f)finishHistoryTransition(w);else{gMapSceneCacheValid=false;InvalidateRect(w,&gMapRect,FALSE);}return 0;}
  if(m==WM_TIMER&&a==HISTORY_EVENT_SCROLL_TIMER){if(gInfoTab!=1){KillTimer(w,HISTORY_EVENT_SCROLL_TIMER);return 0;}DWORD now=GetTickCount();if((LONG)(gHistoryScrollPauseUntil-now)>0)return 0;if(gHistoryEventCycleHeight>gViewportHeight&&gHistoryEventCycleHeight>0){if(gHistoryEventRelayMode&&gHistoryPlaying){int end=std::max(0,gHistoryEventCycleHeight-gViewportHeight);if(gScrollOffset<end){gScrollOffset=std::min(end,gScrollOffset+1);InvalidateRect(w,&gInfoRect,FALSE);}}else{gScrollOffset++;if(gScrollOffset>=gHistoryEventCycleHeight)gScrollOffset=0;InvalidateRect(w,&gInfoRect,FALSE);}}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1014&&HIWORD(a)==BN_CLICKED){gShowGridLines=SendMessageW(gGridLinesToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;gNotice=gShowGridLines?L"已显示网格线":L"已隐藏网格线；格子定位仍有效";InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1011&&HIWORD(a)==BN_CLICKED){gShowHistory=SendMessageW(gHistoryToggleButton,BM_GETCHECK,0,0)==BST_CHECKED&&gHistoryLoaded;gNotice=gShowHistory?L"已开启"+historyYearText(gHistoryYear)+L"历史政权填色；该模块独立开关":L"已关闭历史填色；其他数据图层保持原状";InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_HSCROLL&&(HWND)b==gOpacityTrackbar){int opacity=(int)SendMessageW(gOpacityTrackbar,TBM_GETPOS,0,0);gFillOpacity=std::max(0,std::min(100,opacity));if(gOpacityLabel){wchar_t value[48];swprintf(value,48,L"填色透明度 %d%%",gFillOpacity);SetWindowTextW(gOpacityLabel,value);}InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1003&&HIWORD(a)==BN_CLICKED){
    wchar_t q[256]{};GetWindowTextW(gSearchEdit,q,256);int row=-1,col=-1,matches=0;
    bool legacy=false;if(locateOrCycleQuery(q,row,col,matches,&legacy)){if(gSearchMatchIndex<gSearchMatches.size())selectSearchResultOnMap(gSearchMatches[gSearchMatchIndex]);else selectSearchResultOnMap(row,col);int fineRow=0,fineCol=0;fineGridCellForCoordinate(gSelectedLatitude,gSelectedLongitude,fineRow,fineCol);gNotice=legacy?L"旧编号换算至 ":L"已定位 ";gNotice+=gridIdText(activeGridCount(),fineRow,fineCol);if(gGridLevel>0)gNotice+=L" · 父格 "+gridIdText(GRID_VERSION,fineRow/activeGridScale(),fineCol/activeGridScale());if(matches>1)gNotice+=L" · "+std::to_wstring(gSearchMatchIndex+1)+L"/"+std::to_wstring(matches)+L"（再点查找切换）";else gNotice+=L" · 已放大并高亮";}
    else{std::wstring missed=trimmedSearchQuery(q);if(missed.size()>24)missed=missed.substr(0,24)+L"…";gNotice=L"未找到“"+missed+L"”；试试 Z9800-019-115、39.9,116.4、城市名或土地覆盖类别";}
    InvalidateRect(w,nullptr,TRUE);return 0;
  }
  if(m==WM_COMMAND&&LOWORD(a)==1005&&HIWORD(a)==BN_CLICKED){gShowLandCover=SendMessageW(gLandCoverToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;if(gShowLandCover){gShowPopulation=false;if(gPopulationToggleButton)SendMessageW(gPopulationToggleButton,BM_SETCHECK,BST_UNCHECKED,0);}gNotice=gShowLandCover?L"已开启土地覆盖分区着色；完整类别图例显示在地图左下角":L"已关闭土地覆盖着色";InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1007&&HIWORD(a)==BN_CLICKED){gShowPopulation=SendMessageW(gPopulationToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;if(gShowPopulation){gShowLandCover=false;SendMessageW(gLandCoverToggleButton,BM_SETCHECK,BST_UNCHECKED,0);}gNotice=gShowPopulation?L"已开启 WorldPop 2023 人口密度估算着色；蓝→黄→红为低→高密度":L"已关闭人口密度着色";InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1009&&HIWORD(a)==BN_CLICKED){gShowWorld2026=SendMessageW(gWorld2026ToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;gNotice=gShowWorld2026?L"已开启世界2026国别填色；图层独立开关，按 627,200 细格中心定位":L"已关闭世界2026国别填色；其他图层保持原状态";InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1001&&HIWORD(a)==BN_CLICKED){gShowControlPoints=SendMessageW(gControlPointsButton,BM_GETCHECK,0,0)==BST_CHECKED;InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1004&&HIWORD(a)==BN_CLICKED){
    bool requested=SendMessageW(gHighResMapButton,BM_GETCHECK,0,0)==BST_CHECKED;
    if(requested&&!gMapHighRes){std::wstring base=appDir()+L"\\assets\\world-map-hd",path=base+L".jpg";Image* candidate=Image::FromFile(path.c_str());if(!candidate||candidate->GetLastStatus()!=Ok||candidate->GetWidth()<4000||candidate->GetHeight()<2000||!validateGlobalMapMetadata(base,candidate->GetWidth(),candidate->GetHeight())){delete candidate;SendMessageW(gHighResMapButton,BM_SETCHECK,BST_UNCHECKED,0);gNotice=L"高清底图或其 WGS84 世界文件校验失败";InvalidateRect(w,nullptr,TRUE);return 0;}gMapHighRes=candidate;}
    if(requested){SendMessageW(gNasaMapButton,BM_SETCHECK,BST_UNCHECKED,0);delete gMapNasa;gMapNasa=nullptr;clearNasaDetailTileCache();gMap=gMapHighRes;resetMapView();gNotice=L"已启用 Natural Earth 10m 高清底图；全球范围与 WGS84 配准校验通过";}
    else{gMap=gMapOriginal;delete gMapHighRes;gMapHighRes=nullptr;resetMapView();gNotice=L"已切回 4,050 × 2,025 WGS84 标准底图";}
    InvalidateRect(w,nullptr,TRUE);return 0;
  }
  if(m==WM_COMMAND&&LOWORD(a)==1006&&HIWORD(a)==BN_CLICKED){
    bool requested=SendMessageW(gNasaMapButton,BM_GETCHECK,0,0)==BST_CHECKED;
    if(requested&&!gMapNasa){std::wstring base=appDir()+L"\\assets\\world-map-nasa",path=base+L".jpg";Image* candidate=Image::FromFile(path.c_str());if(!candidate||candidate->GetLastStatus()!=Ok||candidate->GetWidth()!=21600||candidate->GetHeight()!=10800||!validateGlobalMapMetadata(base,21600,10800)){delete candidate;SendMessageW(gNasaMapButton,BM_SETCHECK,BST_UNCHECKED,0);gNotice=L"NASA 底图或 WGS84 配准文件校验失败；当前底图未更改";InvalidateRect(w,nullptr,TRUE);return 0;}gMapNasa=candidate;}
    if(requested){SendMessageW(gHighResMapButton,BM_SETCHECK,BST_UNCHECKED,0);delete gMapHighRes;gMapHighRes=nullptr;gMap=gMapNasa;resetMapView();gNotice=gNasa500mAvailable?L"NASA 底图已启用：800% 显示 1 km 细节，1200% 起可用 500 m 原始分辨率瓦片":(gNasa1kmAvailable?L"NASA 底图已启用：放大至 800% 自动显示 1 km 细节瓦片":L"已启用 NASA Blue Marble 2004-01 高清底图（21,600 × 10,800）");}
    else{gMap=gMapOriginal;delete gMapNasa;gMapNasa=nullptr;clearNasaDetailTileCache();resetMapView();gNotice=L"已切回 Natural Earth WGS84 标准底图";}
    InvalidateRect(w,nullptr,TRUE);return 0;
  }
  if(m==WM_LBUTTONDOWN){int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);if(mapContainsPoint(x,y)){gMapDragging=true;gMapDragged=false;gDragStart=gLastMouse={x,y};SetCapture(w);}return 0;}
  if(m==WM_MOUSEMOVE){if(!gTrackingMouseLeave){TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT),TME_LEAVE,w,0};gTrackingMouseLeave=TrackMouseEvent(&track)!=FALSE;}
    int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);
    if(gMapDragging&&(a&MK_LBUTTON)){int dx=x-gLastMouse.x,dy=y-gLastMouse.y;if(!gMapDragged&&std::abs(x-gDragStart.x)+std::abs(y-gDragStart.y)>4){RECT client{};GetClientRect(w,&client);MapSceneCacheKey current=currentMapSceneCacheKey(client.right,client.bottom);gMapDragged=true;gMapViewPreviewActive=gMapSceneCacheValid&&sameMapSceneCacheKey(current,gMapSceneCacheKey);}if(gMapDragged&&(dx||dy)){panMapBy(dx,dy);InvalidateRect(w,&gMapRect,FALSE);}gLastMouse={x,y};return 0;}
    if(!gMapDragging){std::wstring status=pointerStatus(x,y);if(!status.empty()){if(status!=gNotice){gNotice=status;gPointerStatusActive=true;invalidateNotice(w);}}else if(gPointerStatusActive){gPointerStatusActive=false;gNotice=L"地图操作：滚轮缩放 · 拖动平移 · 双击复位";invalidateNotice(w);}}
    return 0;
  }
  if(m==WM_MOUSELEAVE){gTrackingMouseLeave=false;if(gPointerStatusActive){gPointerStatusActive=false;gNotice=L"地图操作：滚轮缩放 · 拖动平移 · 双击复位";invalidateNotice(w);}return 0;}
  if(m==WM_LBUTTONUP&&gMapDragging){int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);gMapDragging=false;if(GetCapture()==w)ReleaseCapture();if(gMapDragged){KillTimer(w,MAP_VIEW_SETTLE_TIMER);gMapViewPreviewActive=false;gMapSceneCacheValid=false;InvalidateRect(w,&gMapRect,FALSE);}else{if(PtInRect(&gLandCoverLegendRect,POINT{x,y})||PtInRect(&gPopulationLegendRect,POINT{x,y})||PtInRect(&gWorld2026LegendRect,POINT{x,y})||PtInRect(&gHistoryLegendRect,POINT{x,y}))return 0;if(selectMapPoint(x,y)){gNotice=L"已选中分区；滚轮缩放、拖动平移";InvalidateRect(w,nullptr,TRUE);}}return 0;}
  if(m==WM_LBUTTONDBLCLK){int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);if(mapContainsPoint(x,y)){gMapDragging=false;if(GetCapture()==w)ReleaseCapture();if(PtInRect(&gLandCoverLegendRect,POINT{x,y})||PtInRect(&gPopulationLegendRect,POINT{x,y})||PtInRect(&gWorld2026LegendRect,POINT{x,y})||PtInRect(&gHistoryLegendRect,POINT{x,y}))return 0;resetMapView();selectMapPoint(x,y);gNotice=L"地图已复位至全局视图";InvalidateRect(w,nullptr,TRUE);}return 0;}
  if(m==WM_CAPTURECHANGED){gMapDragging=false;return 0;}
  if(m==WM_MOUSEWHEEL){POINT point{GET_X_LPARAM(b),GET_Y_LPARAM(b)};ScreenToClient(w,&point);if(mapContainsPoint(point.x,point.y)){RECT client{};GetClientRect(w,&client);MapSceneCacheKey current=currentMapSceneCacheKey(client.right,client.bottom);bool preview=gMapSceneCacheValid&&(sameMapSceneCacheKey(current,gMapSceneCacheKey)||(gMapViewPreviewActive&&sameMapSceneCacheKeyForViewPreview(current,gMapSceneCacheKey)));if(!preview)gMapViewPreviewActive=false;double factor=std::pow(1.2,GET_WHEEL_DELTA_WPARAM(a)/(double)WHEEL_DELTA);zoomMapAt(point.x,point.y,factor);if(preview){gMapViewPreviewActive=true;SetTimer(w,MAP_VIEW_SETTLE_TIMER,110,nullptr);}else gMapSceneCacheValid=false;if(gMap==gMapNasa&&gNasaDetailAvailable&&gMapZoom>=8.0){if(gMapZoom>=12.0&&gNasa500mAvailable)gNotice=L"NASA 500 m 原始分辨率瓦片已启用 · 拖动地图浏览相邻区域";else if(gNasa1kmAvailable)gNotice=L"NASA 1 km 细节瓦片已启用 · 继续放大可查看 500 m 瓦片";else gNotice=L"NASA 500 m 原始分辨率瓦片已启用 · 拖动地图浏览相邻区域";}else{wchar_t status[48];swprintf(status,48,L"地图缩放 %.0f%%",gMapZoom*100);gNotice=status;}InvalidateRect(w,&gMapRect,FALSE);return 0;}if(PtInRect(&gInfoRect,point)&&(gCol>=0||gInfoTab==1)){int step=(GET_WHEEL_DELTA_WPARAM(a)/WHEEL_DELTA)*72;gScrollOffset=std::max(0,std::min(gScrollOffset-step,std::max(0,gContentHeight-gViewportHeight)));if(gInfoTab==1){gHistoryScrollPauseUntil=GetTickCount()+5000;if(gHistoryEventCycleHeight>gViewportHeight&&gScrollOffset>=gHistoryEventCycleHeight)gScrollOffset%=gHistoryEventCycleHeight;}InvalidateRect(w,nullptr,FALSE);return 0;}}
  if(m==WM_SIZE){KillTimer(w,MAP_VIEW_SETTLE_TIMER);gMapViewPreviewActive=false;gMapSceneCacheValid=false;layoutControls(w);InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_DESTROY){KillTimer(w,HISTORY_PLAYBACK_TIMER);KillTimer(w,HISTORY_TRANSITION_TIMER);KillTimer(w,HISTORY_EVENT_SCROLL_TIMER);KillTimer(w,MAP_VIEW_SETTLE_TIMER);gMapViewPreviewActive=false;gHistoryPlaying=false;delete gHistoryTransitionPreviousRaster;gHistoryTransitionPreviousRaster=nullptr;gHistoryTransitionActive=false;delete gMapSceneCache;gMapSceneCache=nullptr;gMapSceneCacheValid=false;delete gMapBaseCache;gMapBaseCache=nullptr;gMapBaseCacheValid=false;releaseBackBuffer();clearUiFonts();if(gUiControlBrush)DeleteObject(gUiControlBrush);if(gUiPanelBrush)DeleteObject(gUiPanelBrush);gUiControlBrush=gUiPanelBrush=nullptr;PostQuitMessage(0);return 0;}
  return DefWindowProcW(w,m,a,b);
}
#ifndef WORLD_MAP_GRID_LOGIC_TEST
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR,int n){
  GdiplusStartupInput in;GdiplusStartup(&gToken,&in,nullptr);
  gUiControlBrush=CreateSolidBrush(RGB(18,43,57));gUiPanelBrush=CreateSolidBrush(RGB(10,29,43));
  INITCOMMONCONTROLSEX commonControls{sizeof(commonControls),ICC_BAR_CLASSES};InitCommonControlsEx(&commonControls);
  std::wstring base=appDir()+L"\\assets\\world-map",image=base+L".jpg";gMapOriginal=Image::FromFile(image.c_str());if(!gMapOriginal||gMapOriginal->GetLastStatus()!=Ok||!validateGlobalMapMetadata(base,gMapOriginal?gMapOriginal->GetWidth():0,gMapOriginal?gMapOriginal->GetHeight():0)){MessageBoxW(nullptr,L"无法读取或验证 WGS84 全球底图。请确认 assets\\world-map.jpg、world-map.jgw 与 world-map.prj 来自同一版本。",L"世界地图分区档案",MB_OK|MB_ICONERROR);delete gMapOriginal;gMapOriginal=nullptr;GdiplusShutdown(gToken);return 2;}gMap=gMapOriginal;
  std::wstring nasaBase=appDir()+L"\\assets\\world-map-nasa";gNasaMapAvailable=GetFileAttributesW((nasaBase+L".jpg").c_str())!=INVALID_FILE_ATTRIBUTES&&GetFileAttributesW((nasaBase+L".jgw").c_str())!=INVALID_FILE_ATTRIBUTES&&GetFileAttributesW((nasaBase+L".prj").c_str())!=INVALID_FILE_ATTRIBUTES;gNasa1kmAvailable=validateNasaDetailManifest(L"nasa-1km",1200,30);gNasa500mAvailable=validateNasaDetailManifest(L"nasa-500m",2400,15);gNasaDetailAvailable=gNasa1kmAvailable||gNasa500mAvailable;
  std::wstring data=appDir()+L"\\data\\zones.json";gArchiveLoaded=loadArchiveFile(data);if(gArchiveLoaded&&buildArchiveIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gArchiveLoaded=false;gArchiveError=ERROR_REVISION_MISMATCH;}
  if(!gArchiveLoaded){std::wstring msg=gArchiveError==ERROR_REVISION_MISMATCH?L"分区档案与此程序的 9,800 格网版本不匹配。\r\n请从同一版本完整包安装，或同步更新 WorldMapExplorer.exe 与 data\\zones.json。\r\n\r\n档案路径：\r\n"+data:L"无法读取分区档案：\r\n"+data+L"\r\nWindows 错误码："+std::to_wstring(gArchiveError);MessageBoxW(nullptr,msg.c_str(),L"世界地图分区档案",MB_OK|MB_ICONWARNING);}
  std::wstring landCoverData=appDir()+L"\\data\\land-cover-2020.json";gLandCoverLoaded=loadTextFile(landCoverData,gLandCoverArchive,gLandCoverError);if(gLandCoverLoaded){if(buildLandCoverIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gLandCoverLoaded=false;gLandCoverError=ERROR_REVISION_MISMATCH;gLandCoverArchive.clear();gLandCoverRanges.clear();}else if(!buildLandCoverRaster()){gLandCoverLoaded=false;gLandCoverError=ERROR_NOT_ENOUGH_MEMORY;gLandCoverArchive.clear();gLandCoverRanges.clear();gLandCoverZoneCodes.clear();gLandCoverZoneRgb.clear();gLandCoverLegend.clear();}}
  std::wstring heritageData=appDir()+L"\\data\\world-heritage-2026.json";gHeritageLoaded=loadTextFile(heritageData,gHeritageArchive,gHeritageError);if(gHeritageLoaded&&buildHeritageIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gHeritageLoaded=false;gHeritageError=ERROR_REVISION_MISMATCH;gHeritageArchive.clear();gHeritageRanges.clear();}
  std::wstring populationData=appDir()+L"\\data\\population-2023.json";gPopulationLoaded=loadTextFile(populationData,gPopulationArchive,gPopulationError);if(gPopulationLoaded){if(buildPopulationIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gPopulationLoaded=false;gPopulationError=ERROR_REVISION_MISMATCH;gPopulationArchive.clear();gPopulationRanges.clear();gPopulationCounts.clear();gPopulationDensity.clear();gPopulationCoverage.clear();gPopulationAvailable.clear();}else if(!buildPopulationRaster()){gPopulationLoaded=false;gPopulationError=ERROR_NOT_ENOUGH_MEMORY;gPopulationArchive.clear();gPopulationRanges.clear();gPopulationCounts.clear();gPopulationDensity.clear();gPopulationCoverage.clear();gPopulationAvailable.clear();}}
  std::wstring world2026Data=appDir()+L"\\data\\world-2026.json";gWorld2026Loaded=loadTextFile(world2026Data,gWorld2026Archive,gWorld2026Error);if(gWorld2026Loaded){if(!buildWorld2026Index()){gWorld2026Loaded=false;gWorld2026Error=ERROR_REVISION_MISMATCH;gWorld2026Archive.clear();gWorld2026Countries.clear();gWorld2026CellCountries.clear();}else if(!buildWorld2026Raster()){gWorld2026Loaded=false;gWorld2026Error=ERROR_NOT_ENOUGH_MEMORY;gWorld2026Archive.clear();gWorld2026Countries.clear();gWorld2026CellCountries.clear();}}
  gHistoryEventsLoaded=loadHistoryEvents(appDir()+L"\\data\\world-events.tsv");setHistoryEventRange(0,false);
  gHistoryLoaded=loadHistoryModule(HISTORY_YEARS[0]);
  WNDCLASSW c{};c.style=CS_DBLCLKS;c.hInstance=h;c.lpszClassName=L"WorldMapExplorer";c.lpfnWndProc=proc;c.hIcon=(HICON)LoadImageW(h,MAKEINTRESOURCEW(101),IMAGE_ICON,0,0,LR_DEFAULTSIZE);c.hCursor=LoadCursor(nullptr,IDC_CROSS);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);
  RECT workArea{};SystemParametersInfoW(SPI_GETWORKAREA,0,&workArea,0);int workW=std::max(640,(int)(workArea.right-workArea.left)),workH=std::max(480,(int)(workArea.bottom-workArea.top));int windowW=std::min(1360,std::max(640,workW-24)),windowH=std::min(820,std::max(480,workH-32));int windowX=workArea.left+(workW-windowW)/2,windowY=workArea.top+(workH-windowH)/2;
  HWND w=CreateWindowExW(0,c.lpszClassName,L"世界地图分区档案",WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPCHILDREN,windowX,windowY,windowW,windowH,nullptr,nullptr,h,nullptr);
   gSearchEdit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,0,0,230,25,w,(HMENU)1002,h,nullptr);gSearchButton=CreateWindowExW(0,L"BUTTON",L"查找",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,60,25,w,(HMENU)1003,h,nullptr);gHighResMapButton=CreateWindowExW(0,L"BUTTON",L"高清底图",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,0,0,92,25,w,(HMENU)1004,h,nullptr);gControlPointsButton=CreateWindowExW(0,L"BUTTON",L"配准校验点",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,0,0,134,25,w,(HMENU)1001,h,nullptr);gGridLevelCombo=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,0,0,185,150,w,(HMENU)1008,h,nullptr);SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 9,800 格（原格）");SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 39,200 格");SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 156,800 格");SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 627,200 格");SendMessageW(gGridLevelCombo,CB_SETCURSEL,0,0);gNasaMapButton=CreateWindowExW(0,L"BUTTON",L"NASA 2 km 底图",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,150,27,w,(HMENU)1006,h,nullptr);gLandCoverToggleButton=CreateWindowExW(0,L"BUTTON",L"土地覆盖着色",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,167,27,w,(HMENU)1005,h,nullptr);if(gPopulationLoaded&&gPopulationRaster)gPopulationToggleButton=CreateWindowExW(0,L"BUTTON",L"人口密度（2023）",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,145,27,w,(HMENU)1007,h,nullptr);if(gWorld2026Loaded&&gWorld2026Raster)gWorld2026ToggleButton=CreateWindowExW(0,L"BUTTON",L"世界2026 国别填色",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,160,27,w,(HMENU)1009,h,nullptr);
   gHistoryYearCombo=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,0,0,132,140,w,(HMENU)1010,h,nullptr);for(int i=0;i<HISTORY_YEAR_COUNT;i++){std::wstring label=historyYearText(HISTORY_YEARS[i]);SendMessageW(gHistoryYearCombo,CB_ADDSTRING,0,(LPARAM)label.c_str());}SendMessageW(gHistoryYearCombo,CB_SETCURSEL,0,0);gHistoryPlaybackButton=CreateWindowExW(0,L"BUTTON",L"从古到今播放",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,112,27,w,(HMENU)1015,h,nullptr);gHistorySpeedButton=CreateWindowExW(0,L"BUTTON",L"速度 1×",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,82,27,w,(HMENU)1016,h,nullptr);gHistoryAnimationButton=CreateWindowExW(0,L"BUTTON",L"渐变动画试行",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,112,27,w,(HMENU)1020,h,nullptr);gHistoryToggleButton=CreateWindowExW(0,L"BUTTON",L"历史政权填色",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,142,27,w,(HMENU)1011,h,nullptr);gOpacityLabel=CreateWindowExW(0,L"STATIC",L"填色透明度 72%",WS_CHILD|WS_VISIBLE|SS_LEFT,0,0,118,22,w,(HMENU)1013,h,nullptr);gOpacityTrackbar=CreateWindowExW(0,TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|TBS_HORZ|TBS_NOTICKS,0,0,184,30,w,(HMENU)1012,h,nullptr);SendMessageW(gOpacityTrackbar,TBM_SETRANGE,TRUE,MAKELONG(0,100));SendMessageW(gOpacityTrackbar,TBM_SETPOS,TRUE,gFillOpacity);gGridLinesToggleButton=CreateWindowExW(0,L"BUTTON",L"显示网格线",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,132,27,w,(HMENU)1014,h,nullptr);SendMessageW(gGridLinesToggleButton,BM_SETCHECK,BST_CHECKED,0);gGeoTabButton=CreateWindowExW(0,L"BUTTON",L"地理资料",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,159,32,w,(HMENU)1018,h,nullptr);gEventsTabButton=CreateWindowExW(0,L"BUTTON",L"历史事件",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,159,32,w,(HMENU)1019,h,nullptr);
   gFullscreenButton=CreateWindowExW(0,L"BUTTON",L"地图全屏",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,94,29,w,(HMENU)1023,h,nullptr);const HWND styledButtons[]={gControlPointsButton,gSearchButton,gHighResMapButton,gNasaMapButton,gLandCoverToggleButton,gPopulationToggleButton,gWorld2026ToggleButton,gHistoryToggleButton,gGridLinesToggleButton,gHistoryPlaybackButton,gHistorySpeedButton,gHistoryAnimationButton,gGeoTabButton,gEventsTabButton,gFullscreenButton};for(HWND button:styledButtons)if(button){LONG_PTR style=GetWindowLongPtrW(button,GWL_STYLE);SetWindowLongPtrW(button,GWL_STYLE,(style&~BS_TYPEMASK)|BS_OWNERDRAW|BS_NOTIFY);SetWindowSubclass(button,styledButtonSubclass,(UINT_PTR)GetDlgCtrlID(button),0);}
   SendMessageW(gControlPointsButton,BM_SETCHECK,BST_CHECKED,0);layoutControls(w);ShowWindow(w,n);
  MSG msg;while(GetMessageW(&msg,nullptr,0,0)){if((msg.hwnd==w||IsChild(w,msg.hwnd))&&(msg.message==WM_KEYDOWN||msg.message==WM_SYSKEYDOWN)){if(msg.wParam==VK_F11){setMapFullscreen(w,!gMapFullscreen);continue;}if(msg.wParam==VK_ESCAPE&&gMapFullscreen){setMapFullscreen(w,false);continue;}}TranslateMessage(&msg);DispatchMessageW(&msg);}
    delete gLandCoverRaster;delete gPopulationRaster;delete gWorld2026Raster;delete gHistoryRaster;clearNasaDetailTileCache();delete gMapNasa;delete gMapHighRes;delete gMapOriginal;GdiplusShutdown(gToken);return 0;
}
#endif
