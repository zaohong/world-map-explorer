#define NOMINMAX
#include <algorithm>
using std::max;
using std::min;
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <commdlg.h>
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
#include <string>
#include "ui_localization.h"
#include "ui_scale.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cctype>
#include <cwctype>
#include <cmath>
#include <vector>
#include <memory>
#include <tuple>
#include "display_preferences.h"
#include "map_metrics.h"
#include "map_viewport.h"
#include "event_catalog.h"
#include "history_vectors.h"
#include "region_reading.h"
#include "ui_theme.h"
#include "history_years.generated.h"
#include "route_catalog.h"
using namespace Gdiplus;

static const int GRID_COLUMNS=140,GRID_ROWS=70,GRID_VERSION=9800,LEGACY_COLUMNS=100,LEGACY_ROWS=50,FINE_GRID_COLUMNS=1120,FINE_GRID_ROWS=560;
static const int STACKED_TOOLBAR_WIDTH=980;
static const wchar_t* APP_VERSION=L"1.28.0";
static const int POPULATION_YEAR=2023;
static const double GRID_CELL_DEGREES=180.0/GRID_ROWS;
static ULONG_PTR gToken; static Image* gMap=nullptr; static Image* gMapOriginal=nullptr; static Image* gMapHighRes=nullptr; static Image* gMapNasa=nullptr; static Bitmap* gLandCoverRaster=nullptr; static Bitmap* gPopulationRaster=nullptr; static Bitmap* gWorld2026Raster=nullptr; static Bitmap* gHistoryRaster=nullptr; static RECT gMapRect{},gInfoRect{}; static int gCol=-1,gRow=-1,gGridLevel=0; static double gSelectedLatitude=0,gSelectedLongitude=0;static bool gSelectedCoordinateExact=false,gSelectedCoordinateFromInput=false,gSelectedCoordinateIsFineGridCenter=false;static std::string gArchive,gLandCoverArchive,gHeritageArchive,gPopulationArchive,gWorld2026Archive,gHistoryArchive; static bool gArchiveLoaded=false,gLandCoverLoaded=false,gHeritageLoaded=false,gPopulationLoaded=false,gWorld2026Loaded=false,gHistoryLoaded=false,gShowPopulation=false,gShowWorld2026=false,gShowHistory=false,gNasaMapAvailable=false,gNasaDetailAvailable=false,gNasa1kmAvailable=false,gNasa500mAvailable=false; static DWORD gArchiveError=0,gLandCoverError=0,gHeritageError=0,gPopulationError=0,gWorld2026Error=0,gHistoryError=0; static int gScrollOffset=0,gContentHeight=0,gViewportHeight=0,gInfoWheelRemainder=0,gFillOpacity=72,gHistoryYear=2024;
static int gHistoryRasterYear=INT_MIN;
static Bitmap* gHistoryTransitionPreviousRaster=nullptr;static bool gHistoryAnimationEnabled=false,gHistoryTransitionActive=false;static DWORD gHistoryTransitionStart=0;static int gHistoryTransitionDurationMs=700;
static int gHistoryEventCycleHeight=0;static bool gHistoryEventFractionPending=false;static double gHistoryEventScrollFraction=0;static DWORD gHistoryScrollPauseUntil=0;static bool gHistoryEventRelayMode=false;static std::vector<size_t> gHistoryRelayEvents;static std::vector<size_t> gHistoryRangeEventCache;static int gHistoryCacheRangeStart=INT_MIN,gHistoryCacheRangeEnd=INT_MIN;static std::vector<int> gHistoryEventDescriptionHeights,gHistoryEventTitleHeights,gHistoryEventPlaceHeights;static int gHistoryEventDescriptionWidth=0;
static HBRUSH gUiControlBrush=nullptr,gUiPanelBrush=nullptr;
static const UINT MAP_VIEW_SETTLE_TIMER=1024,MAP_VIEW_FRAME_TIMER=1026,MAP_VIEW_FRAME_READY_MESSAGE=WM_APP+0x35;
static const UINT MAP_VIEW_SETTLE_MIN_DELAY_MS=70,MAP_VIEW_SETTLE_MAX_DELAY_MS=180;
static const double MAP_VIEW_PREVIEW_MIN_FRAME_MS=1000.0/60.0,MAP_VIEW_PREVIEW_MAX_FRAME_MS=50.0;
static const UINT MAP_VIEW_PAN_SETTLE_DELAY_MS=90;
static const DWORD MAP_CREATE_WAITABLE_TIMER_HIGH_RESOLUTION=0x00000002;
static LARGE_INTEGER gMapViewPreviewFrameFrequency{};static LONGLONG gMapViewPreviewLastFrameCounter=0,gMapViewLastWheelCounter=0;static double gMapViewPreviewFrameIntervalMs=MAP_VIEW_PREVIEW_MIN_FRAME_MS,gMapViewWheelIntervalMs=72.0;static bool gMapViewPreviewTimerScheduled=false,gMapViewPreviewTimerWaitable=false;static HANDLE gMapViewPreviewWaitableTimer=nullptr;
struct ArchiveRange{size_t begin,end;ArchiveRange():begin(std::string::npos),end(std::string::npos){}};static std::vector<ArchiveRange> gZoneRanges,gLandCoverRanges,gHeritageRanges,gPopulationRanges;
struct UiFontEntry{int size;bool bold,english;int dpi;HFONT handle;};static std::vector<UiFontEntry> gUiFonts;
static HDC gBackBufferDC=nullptr;static HBITMAP gBackBufferBitmap=nullptr,gBackBufferOriginalBitmap=nullptr;static int gBackBufferWidth=0,gBackBufferHeight=0;
struct MapSceneCacheKey{int width,height;RECT mapRect;Image* map;Bitmap* landCover;Bitmap* population;Bitmap* world2026;Bitmap* history;double zoom,centerX,centerY,selectedLatitude,selectedLongitude;int row,col,gridLevel,fillOpacity,historyYear;bool selectedExact,showControlPoints,showLandCover,showPopulation,showWorld2026,showHistory,showGridLines,nasaDetail,nasa1km,nasa500m;};
static Bitmap* gMapSceneCache=nullptr;static MapSceneCacheKey gMapSceneCacheKey{};static bool gMapSceneCacheValid=false,gMapViewPreviewActive=false;
static HWND gRouteButton=nullptr,gRouteCombo=nullptr,gRouteHideButton=nullptr,gRouteInfoButton=nullptr,gRouteFitButton=nullptr;
static HWND gRoutePanelButton=nullptr;
static HWND gRoutePickerButton=nullptr,gYearPickerButton=nullptr;
static HWND gExportButton=nullptr;
static bool gRoutePanelOpen=false;
static Bitmap* gRouteRaster=nullptr;
static Image* gMapHighResOverview=nullptr;static Image* gMapNasaOverview=nullptr;
static int gRouteIndex=-1;
static double gRouteProgress=0;
static routes::DistanceProfile gRouteDistance;static int gRouteDistanceIndex=-1;
static bool gRoutePlaying=false;
static const UINT ROUTE_PLAYBACK_TIMER=1042;
static ULONGLONG gRouteLastPlaybackTime=0;
static ULONGLONG routePlaybackTime(){ULONGLONG ticks=0;QueryUnbiasedInterruptTime(&ticks);return ticks;}
static double routePlaybackAdvance(double progress,double end,ULONGLONG elapsed){double duration=std::max(8.0,std::min(30.0,end));return std::min(end,progress+end*((double)elapsed/10000000.0)/duration);}
struct MapBaseCacheKey{int width,height;RECT mapRect;Image* map;double zoom,centerX,centerY;bool nasaDetail,nasa1km,nasa500m;};static Bitmap* gMapBaseCache=nullptr;static MapBaseCacheKey gMapBaseCacheKey{};static bool gMapBaseCacheValid=false;static Bitmap* gMapBaseOverviewCache=nullptr;static MapBaseCacheKey gMapBaseOverviewCacheKey{};static bool gMapBaseOverviewCacheValid=false;
struct SearchMatch{int row,col,gridLevel;double latitude,longitude;bool hasExactCoordinate;SearchMatch(int r,int c):row(r),col(c),gridLevel(-1),latitude(0),longitude(0),hasExactCoordinate(false){}SearchMatch(int r,int c,double lat,double lon):row(r),col(c),gridLevel(-1),latitude(lat),longitude(lon),hasExactCoordinate(true){}SearchMatch(int r,int c,double lat,double lon,int level):row(r),col(c),gridLevel(level),latitude(lat),longitude(lon),hasExactCoordinate(true){}};static std::vector<SearchMatch> gSearchMatches;static std::wstring gLastSearchQuery;static size_t gSearchMatchIndex=0;static bool gSearchWasLegacy=false;
struct NasaDetailTile{int row,col;Image* image;DWORD lastUsed;NasaDetailTile(int r,int c,Image* i,DWORD t):row(r),col(c),image(i),lastUsed(t){}};static std::vector<NasaDetailTile> gNasaDetailTiles;static DWORD gNasaTileUseCounter=0;static int gNasaTileLoadFailures=0;static std::wstring gNasaDetailFolder;static int gNasaDetailPixels=1200,gNasaDetailCacheLimit=24,gNasaDetailResolutionMeters=1000;
struct LandCoverClass{int code;std::wstring name;unsigned int rgb;};struct World2026Country{std::string code;std::wstring name,nameZh;int mapColor;unsigned int rgb;};struct HistoryPolity{int id;std::wstring name;std::string color;unsigned int rgb;};static std::vector<LandCoverClass> gLandCoverLegend;static std::vector<int> gLandCoverZoneCodes;static std::vector<unsigned int> gLandCoverZoneRgb;static std::vector<World2026Country> gWorld2026Countries;static std::vector<unsigned short> gWorld2026CellCountries;static std::vector<HistoryPolity> gHistoryPolities;static std::vector<unsigned short> gHistoryCellPolities;
using HistoryEvent=events::Event;static std::vector<HistoryEvent> gHistoryEvents;static std::string gHistoryEventsText;static bool gHistoryEventsLoaded=false;static DWORD gHistoryEventsError=0;static int gInfoTab=0,gHistoryEventRangeStart=-3401,gHistoryEventRangeEnd=-3400,gHoveredButtonId=0;
static HWND gControlPointsButton=nullptr,gSearchEdit=nullptr,gSearchButton=nullptr,gHighResMapButton=nullptr,gGridLevelCombo=nullptr,gNasaMapButton=nullptr,gLandCoverToggleButton=nullptr,gPopulationToggleButton=nullptr,gWorld2026ToggleButton=nullptr,gHistoryToggleButton=nullptr,gHistoryYearCombo=nullptr,gOpacityTrackbar=nullptr,gOpacityLabel=nullptr,gGridLinesToggleButton=nullptr,gHistoryPlaybackButton=nullptr,gHistorySpeedButton=nullptr,gHistoryAnimationButton=nullptr,gGeoTabButton=nullptr,gEventsTabButton=nullptr,gFullscreenButton=nullptr,gLanguageButton=nullptr,gMapPreviewRateCombo=nullptr; static bool gShowControlPoints=true,gShowLandCover=false,gPointerStatusActive=false,gTrackingMouseLeave=false,gShowGridLines=true,gHistoryPlaying=false,gHistoryPaused=false,gMapFullscreen=false;static LONG_PTR gWindowedStyle=0;static WINDOWPLACEMENT gWindowedPlacement{sizeof(WINDOWPLACEMENT)};static int gHistorySpeedIndex=0;static const UINT HISTORY_PLAYBACK_TIMER=1017,HISTORY_TRANSITION_TIMER=1021,HISTORY_EVENT_SCROLL_TIMER=1022;static const int HISTORY_PLAYBACK_INTERVALS[]={500,250,125,1000};static const int HISTORY_ANIMATED_PLAYBACK_INTERVALS[]={1100,550,275,2200};static const wchar_t* HISTORY_PLAYBACK_SPEEDS[]={L"速度 1×",L"速度 2×",L"速度 4×",L"速度 0.5×"};static RECT gLandCoverLegendRect{},gPopulationLegendRect{},gWorld2026LegendRect{},gHistoryLegendRect{};static std::vector<double> gPopulationCounts,gPopulationDensity,gPopulationCoverage;static std::vector<unsigned char> gPopulationAvailable;static double gPopulationMaxDensity=0; static std::wstring gNotice=L"地图操作：滚轮缩放 · 拖动平移 · 双击复位";
enum MapPreviewRateMode{MAP_PREVIEW_RATE_30,MAP_PREVIEW_RATE_60,MAP_PREVIEW_RATE_ADAPTIVE};static MapPreviewRateMode gMapViewPreviewRateMode=MAP_PREVIEW_RATE_ADAPTIVE;
static double gMapZoom=1.0,gMapCenterX=0.5,gMapCenterY=0.5; static bool gMapDragging=false,gMapDragged=false; static POINT gDragStart{},gLastMouse{};
static int gPreferredPanelWidth=0;static HWND gUiScaleCombo=nullptr;
static bool gPanelResizing=false,gShowScaleBar=true;
static RECT gPanelDividerRect{},gMapNavigationRect{},gMapScaleRect{};
static HWND gZoomInButton=nullptr,gZoomOutButton=nullptr,gResetMapButton=nullptr,gHelpButton=nullptr,gUiTooltip=nullptr;
static const UINT PREFERENCES_SAVE_TIMER=1058;
static bool gPreferencesReady=false,gPreferencesDirty=false;
static int gRegionPage=0;
static HWND gRegionPageButtons[4]{},gRegionRawButton=nullptr;
static HWND gEventSearchWindow=nullptr,gEventSearchButton=nullptr;
#include "experience_state.h"
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
static bool fileSha256Matches(const std::wstring& path,const char* expected){
  HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
  BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;DWORD objectSize=0,received=0;bool ok=false;std::vector<BYTE> object;
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objectSize,sizeof(objectSize),&received,0)>=0){
    object.resize(objectSize);std::vector<BYTE> buffer(65536);BYTE digest[32]{};
    if(BCryptCreateHash(algorithm,&hash,object.data(),objectSize,nullptr,0,0)>=0){
      bool valid=true;DWORD count=0;
      while(true){if(!ReadFile(file,buffer.data(),(DWORD)buffer.size(),&count,nullptr)){valid=false;break;}if(!count)break;if(BCryptHashData(hash,buffer.data(),count,0)<0){valid=false;break;}}
      if(valid&&BCryptFinishHash(hash,digest,sizeof(digest),0)>=0){static const char hex[]="0123456789ABCDEF";char actual[65];for(size_t i=0;i<sizeof(digest);i++){actual[2*i]=hex[digest[i]>>4];actual[2*i+1]=hex[digest[i]&15];}actual[64]=0;ok=std::strcmp(actual,expected)==0;}
    }
  }
  if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);return ok;
}
static Image* loadVerifiedMapOverview(const std::wstring& source,const std::wstring& overview,const char* sourceHash,const char* overviewHash){
  if(!fileSha256Matches(source,sourceHash)||!fileSha256Matches(overview,overviewHash))return nullptr;
  Image* image=Image::FromFile(overview.c_str());if(!image||image->GetLastStatus()!=Ok||image->GetWidth()!=4050||image->GetHeight()!=2025){delete image;return nullptr;}return image;
}
static bool selectNasaDetailLayerForZoom();
static Image* mapImageForSize(int width,int height){
  Image* overview=gMap==gMapHighRes?gMapHighResOverview:(gMap==gMapNasa?gMapNasaOverview:nullptr);
  // Verified detail tiles supply the visible pixels; the overview supplies the underlay.
  // This avoids a second full-resolution base draw underneath those tiles.
  if(overview&&gMap==gMapNasa&&gNasaDetailAvailable&&selectNasaDetailLayerForZoom())return overview;
  return overview&&width>0&&height>0&&width<=(int)overview->GetWidth()&&height<=(int)overview->GetHeight()?overview:gMap;
}
static std::wstring displayPreferencesPath(){wchar_t local[MAX_PATH]{};DWORD size=GetEnvironmentVariableW(L"LOCALAPPDATA",local,MAX_PATH);std::wstring base=(size>0&&size<MAX_PATH)?std::wstring(local):appDir();std::wstring folder=base+L"\\WorldMapExplorer";CreateDirectoryW(folder.c_str(),nullptr);return folder+L"\\settings.ini";}
static void loadDisplayPreferences(){
  DisplayPreferences p=readDisplayPreferences(displayPreferencesPath());
  gUiScaleMode=p.uiScale;gEnglish=p.english;gGridLevel=p.gridLevel;gFillOpacity=p.opacity;gMapViewPreviewRateMode=(MapPreviewRateMode)p.previewRate;
  gRestoreLastView=p.restoreView;gShowGridLines=p.gridLines;gShowControlPoints=p.controlPoints;gRoutePanelOpen=p.routePanel;gShowScaleBar=p.scaleBar;
  gHistorySpeedIndex=p.historySpeed;gHistoryAnimationEnabled=p.historyAnimation;gPreferredPanelWidth=p.panelWidth;
}
static bool saveDisplayPreferences(){
  DisplayPreferences p;
  p.uiScale=gUiScaleMode;p.english=gEnglish;p.gridLevel=gGridLevel;p.opacity=gFillOpacity;p.previewRate=(int)gMapViewPreviewRateMode;
  p.restoreView=gRestoreLastView;p.gridLines=gShowGridLines;p.controlPoints=gShowControlPoints;p.routePanel=gRoutePanelOpen;p.scaleBar=gShowScaleBar;
  p.historySpeed=gHistorySpeedIndex;p.historyAnimation=gHistoryAnimationEnabled;p.panelWidth=gPreferredPanelWidth;
  return writeDisplayPreferences(displayPreferencesPath(),p);
}
static void schedulePreferencesSave(HWND w){if(gPreferencesReady){gPreferencesDirty=true;SetTimer(w,PREFERENCES_SAVE_TIMER,650,nullptr);}}
static void setUiText(HWND control,const std::wstring& value){if(control){std::wstring display=localizeUiText(value);SetWindowTextW(control,display.c_str());}}
static const wchar_t* languageButtonCaption(){return gEnglish?L"中文":L"English";}
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
struct NasaTileFailure{int row,col;ULONGLONG retryAfter;};
static std::vector<NasaTileFailure> gNasaTileFailures;
static void clearNasaDetailTileCache(){for(auto& tile:gNasaDetailTiles)delete tile.image;gNasaDetailTiles.clear();gNasaTileFailures.clear();gNasaTileUseCounter=0;}
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
struct HistorySnapshot{
  std::string archive;
  std::vector<HistoryPolity> polities;
  std::vector<unsigned short> cells;
  std::unique_ptr<Bitmap> raster;
  WIN32_FILE_ATTRIBUTE_DATA stamp{};
  size_t bytes() const {size_t result=archive.capacity()+cells.capacity()*sizeof(unsigned short)+polities.capacity()*sizeof(HistoryPolity)+FINE_GRID_ROWS*FINE_GRID_COLUMNS*4;for(const auto& p:polities)result+=p.name.capacity()*sizeof(wchar_t)+p.color.capacity();return result;}
};
static BoundedLruCache<int,HistorySnapshot> gHistorySnapshotCache(8,32*1024*1024);
static WIN32_FILE_ATTRIBUTE_DATA gHistoryFileStamp{};
static ULONGLONG gHistoryDataRevision=0;
static historyVectors::Archive gHistoryVectors;
static bool sameHistoryFileStamp(const WIN32_FILE_ATTRIBUTE_DATA& a,const WIN32_FILE_ATTRIBUTE_DATA& b){return a.nFileSizeHigh==b.nFileSizeHigh&&a.nFileSizeLow==b.nFileSizeLow&&CompareFileTime(&a.ftLastWriteTime,&b.ftLastWriteTime)==0;}
static bool loadHistoryModule(int year){
  if(gHistoryVectors.valid()){
    if(gHistoryLoaded&&gHistoryVectors.year()==year&&gHistoryRaster){gHistoryYear=year;return true;}
    ++gHistoryDataRevision;gMapSceneCacheValid=false;gHistoryYear=year;gHistoryLoaded=false;
    delete gHistoryRaster;gHistoryRaster=nullptr;gHistoryRasterYear=INT_MIN;
    gHistoryArchive.clear();gHistoryPolities.clear();gHistoryCellPolities.clear();
    if(!gHistoryVectors.selectYear(year)){gHistoryError=ERROR_INVALID_DATA;return false;}
    gHistoryRaster=gHistoryVectors.raster(FINE_GRID_COLUMNS,FINE_GRID_ROWS);
    if(!gHistoryRaster){gHistoryError=ERROR_NOT_ENOUGH_MEMORY;return false;}
    gHistoryLoaded=true;gHistoryRasterYear=year;gHistoryError=ERROR_SUCCESS;return true;
  }
  std::wstring file=appDir()+L"\\data\\world-"+std::to_wstring(year)+L".json";
  WIN32_FILE_ATTRIBUTE_DATA stamp{};bool hasStamp=GetFileAttributesExW(file.c_str(),GetFileExInfoStandard,&stamp)!=FALSE;
  if(gHistoryLoaded&&gHistoryRasterYear==year&&gHistoryRaster&&hasStamp&&sameHistoryFileStamp(stamp,gHistoryFileStamp)){gHistoryYear=year;return true;}
  ++gHistoryDataRevision;gMapSceneCacheValid=false;
  if(gHistoryLoaded&&gHistoryRaster){
    HistorySnapshot snapshot;snapshot.archive=std::move(gHistoryArchive);snapshot.polities=std::move(gHistoryPolities);snapshot.cells=std::move(gHistoryCellPolities);snapshot.raster.reset(gHistoryRaster);snapshot.stamp=gHistoryFileStamp;
    size_t bytes=snapshot.bytes();gHistorySnapshotCache.put(gHistoryRasterYear,std::move(snapshot),bytes);gHistoryRaster=nullptr;
  }
  delete gHistoryRaster;gHistoryRaster=nullptr;gHistoryArchive.clear();gHistoryPolities.clear();gHistoryCellPolities.clear();gHistoryLoaded=false;gHistoryRasterYear=INT_MIN;gHistoryYear=year;
  HistorySnapshot cached;
  if(gHistorySnapshotCache.take(year,cached)&&cached.raster&&hasStamp&&sameHistoryFileStamp(stamp,cached.stamp)){
    gHistoryArchive=std::move(cached.archive);gHistoryPolities=std::move(cached.polities);gHistoryCellPolities=std::move(cached.cells);gHistoryRaster=cached.raster.release();gHistoryFileStamp=stamp;gHistoryLoaded=true;gHistoryRasterYear=year;gHistoryError=ERROR_SUCCESS;return true;
  }
  if(!loadTextFile(file,gHistoryArchive,gHistoryError))return false;
  if(!buildHistoryIndex()){gHistoryError=ERROR_REVISION_MISMATCH;gHistoryArchive.clear();gHistoryPolities.clear();gHistoryCellPolities.clear();return false;}
  if(!buildHistoryRaster()){gHistoryError=ERROR_NOT_ENOUGH_MEMORY;gHistoryArchive.clear();gHistoryPolities.clear();gHistoryCellPolities.clear();return false;}gHistoryLoaded=true;gHistoryRasterYear=year;gHistoryFileStamp=stamp;return true;
}
static std::wstring historyYearText(int year){if(gEnglish)return year<0?std::to_wstring(-year)+L" BCE":std::to_wstring(year)+L" CE";return year<0?L"公元前"+std::to_wstring(-year)+L"年":L"公元"+std::to_wstring(year)+L"年";}
static bool loadHistoryEvents(const std::wstring& path){
  gHistoryEvents.clear();gHistoryEventsText.clear();gHistoryEventsLoaded=false;gHistoryRangeEventCache.clear();gHistoryCacheRangeStart=gHistoryCacheRangeEnd=INT_MIN;gHistoryEventDescriptionHeights.clear();gHistoryEventDescriptionWidth=0;if(!loadTextFile(path,gHistoryEventsText,gHistoryEventsError))return false;
  std::vector<HistoryEvent> candidate;if(!events::parse(gHistoryEventsText,candidate,utf8)){gHistoryEventsError=ERROR_INVALID_DATA;return false;}gHistoryEvents.swap(candidate);gHistoryEventsLoaded=true;gHistoryEventsError=ERROR_SUCCESS;return true;
}
static void setHistoryEventRange(int selected,bool playback){
  if(selected<0||selected>=HISTORY_YEAR_COUNT){gHistoryEventRangeStart=gHistoryEventRangeEnd=0;return;}int year=HISTORY_YEARS[selected];
  if(playback&&selected+1<HISTORY_YEAR_COUNT){int older=HISTORY_YEARS[selected+1];gHistoryEventRangeStart=(selected+1==HISTORY_YEAR_COUNT-1)?older:older+1;gHistoryEventRangeEnd=year;}
  else gHistoryEventRangeStart=gHistoryEventRangeEnd=year;
  if(playback)return;
  gHistoryEventRelayMode=false;gHistoryRelayEvents.clear();gHistoryEventCycleHeight=0;gScrollOffset=0;gInfoWheelRemainder=0;gHistoryEventFractionPending=false;
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
static int nextHistoryPlaybackIndex(int selected){
  if(!gHistoryVectors.valid())return std::max(0,selected-1);
  static std::vector<int> nodes;if(nodes.empty()){nodes=gHistoryVectors.changeYears();for(const auto& event:gHistoryEvents){nodes.push_back(event.year);nodes.push_back(event.endYear);}for(int year:{-3400,-3000,-2500,-2000,-1500,-1000,-500,-1,1,500,1000,1500,1800,1900,1950,2000,2024})nodes.push_back(year);std::sort(nodes.begin(),nodes.end());nodes.erase(std::unique(nodes.begin(),nodes.end()),nodes.end());}
  for(int i=selected-1;i>=0;--i)if(std::binary_search(nodes.begin(),nodes.end(),HISTORY_YEARS[i]))return i;return 0;
}
static void updateHistoryPlaybackButton(){if(gHistoryPlaybackButton)setUiText(gHistoryPlaybackButton,gHistoryPlaying?L"暂停播放":(gHistoryPaused?L"继续播放":(gHistoryYear==2024?L"播放历史":L"从此播放")));}
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
  if(gYearPickerButton)SetWindowTextW(gYearPickerButton,(historyYearText(year)+L"  \u25be").c_str());
  if(loadHistoryModule(year)){if(gHistoryToggleButton)EnableWindow(gHistoryToggleButton,TRUE);gNotice=L"已载入"+historyYearText(year)+L"历史填色模块；空白区域表示来源未收录";}
  else{gShowHistory=false;if(gHistoryToggleButton){SendMessageW(gHistoryToggleButton,BM_SETCHECK,BST_UNCHECKED,0);EnableWindow(gHistoryToggleButton,FALSE);}gNotice=historyYearText(year)+L"历史模块读取失败；请检查随附 data 文件";}
  if(gOpacityLabel){wchar_t opacity[48];swprintf(opacity,48,L"不透明度 %d%%",gFillOpacity);setUiText(gOpacityLabel,opacity);}syncExperienceControls();InvalidateRect(w,nullptr,FALSE);return gHistoryLoaded;
}
static std::wstring historyZoneField(){
  if(gHistoryLoaded&&gHistoryVectors.valid()){
    auto hits=gHistoryVectors.hits(gSelectedLatitude,gSelectedLongitude);
    if(hits.empty())return gEnglish?L"No source polity polygon covers this position in the selected year. This is a gap in the source, not evidence of an uninhabited area.":L"来源资料在所选年份未覆盖此位置；资料空白不表示无人居住。";
    std::wstring result;
    for(size_t i=0;i<std::min((size_t)3,hits.size());++i){const auto& record=gHistoryVectors.record(hits[i]);const auto& entity=gHistoryVectors.entity(record.entity);if(i)result+=L"\n";result+=(gEnglish||entity.zh.empty()?entity.en:entity.zh);if(entity.sourceName.find(L"Natural Earth reference: ")==0)result+=gEnglish?L" (modern reference)":L"（现代参考补充）";if(record.from!=record.to)result+=std::wstring(L"\n")+(gEnglish?L"Source interval: ":L"资料有效段：")+historyYearText(record.from)+L" – "+historyYearText(record.to);}
    if(hits.size()>1)result+=gEnglish?L"\nOverlapping source polities; the smallest mapped area is listed first.":L"\n来源存在重叠记录，先列面积较小者。";
    return result;
  }
  if(!gHistoryLoaded||gHistoryCellPolities.size()!=(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return L"历史世界图层未加载";int row=0,col=0;world2026CellForCoordinate(gSelectedLatitude,gSelectedLongitude,row,col);unsigned short id=gHistoryCellPolities[(size_t)row*FINE_GRID_COLUMNS+col];
  if(id==0)return L"该网格单元中心没有对应的已收录政权边界，保持透明；不代表无人居住或现代国界。";if(id>gHistoryPolities.size())return L"历史数据编号无效";return gHistoryPolities[id-1].name+L"（"+historyYearText(gHistoryYear)+L"；按网格单元中心匹配）";
}
static std::wstring world2026ZoneField(){
  if(!gWorld2026Loaded||gWorld2026CellCountries.size()!=(size_t)FINE_GRID_ROWS*FINE_GRID_COLUMNS)return L"世界2026 模块未加载";
  int row=0,col=0;world2026CellForCoordinate(gSelectedLatitude,gSelectedLongitude,row,col);unsigned short id=gWorld2026CellCountries[(size_t)row*FINE_GRID_COLUMNS+col];
  if(id==0)return L"本子格中心未落入 Natural Earth 国界面（海洋或未收录的微小岛屿）；不据此判断主权归属。";
  if(id>gWorld2026Countries.size())return L"世界2026 数据编号无效";const auto& country=gWorld2026Countries[id-1];
  std::wstring label=gEnglish?country.name:(country.nameZh.empty()?country.name:country.nameZh);if(!gEnglish&&label!=country.name)label+=L"（"+country.name+L"）";
  return label+L"（"+utf8(country.code)+L"）；按 627,200 网格单元中心归属，边界格只归入一个国别。";
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
static void skipSearchSpace(const wchar_t*& cursor){while(*cursor&&iswspace(*cursor))cursor++;}
static bool parseSearchInteger(const wchar_t*& cursor,long& value){skipSearchSpace(cursor);errno=0;wchar_t* end=nullptr;long parsed=wcstol(cursor,&end,10);if(end==cursor||errno==ERANGE)return false;cursor=end;value=parsed;return true;}
static int parseSearchGridParts(const std::wstring& text,long (&parts)[3]){
  if(text.empty()||(text[0]!=L'Z'&&text[0]!=L'z'))return 0;const wchar_t* cursor=text.c_str()+1;
  if(!parseSearchInteger(cursor,parts[0]))return 0;skipSearchSpace(cursor);if(*cursor!=L'-')return 0;cursor++;
  if(!parseSearchInteger(cursor,parts[1]))return 0;skipSearchSpace(cursor);if(!*cursor)return 2;if(*cursor!=L'-')return 0;cursor++;
  if(!parseSearchInteger(cursor,parts[2]))return 0;skipSearchSpace(cursor);return *cursor?0:3;
}
static bool parseSearchCoordinates(const std::wstring& text,double& latitude,double& longitude){
  const wchar_t* cursor=text.c_str();skipSearchSpace(cursor);errno=0;wchar_t* end=nullptr;latitude=wcstod(cursor,&end);
  if(end==cursor||errno==ERANGE||!std::isfinite(latitude))return false;cursor=end;skipSearchSpace(cursor);if(*cursor!=L','&&*cursor!=L'，')return false;cursor++;
  skipSearchSpace(cursor);errno=0;longitude=wcstod(cursor,&end);if(end==cursor||errno==ERANGE||!std::isfinite(longitude))return false;cursor=end;skipSearchSpace(cursor);
  return !*cursor&&latitude>=-90&&latitude<=90&&longitude>=-180&&longitude<=180;
}
static bool locateQuery(const std::wstring& raw,int& row,int& col,int& matches,bool* legacyConverted=nullptr,std::vector<SearchMatch>* allMatches=nullptr){
  std::vector<SearchMatch> emptyMatches;std::vector<SearchMatch>& found=allMatches?*allMatches:emptyMatches;found.clear();
  if(legacyConverted)*legacyConverted=false;
  std::wstring q=raw;while(!q.empty()&&iswspace(q.front()))q.erase(q.begin());while(!q.empty()&&iswspace(q.back()))q.pop_back();if(q.empty())return false;
  if(q[0]==L'Z'||q[0]==L'z'){
    long parts[3]{};int count=parseSearchGridParts(q,parts);
    if(count==3){int level=gridLevelForCount((int)parts[0]);
      if(level>=0){int scale=1<<level,rows=GRID_ROWS*scale,columns=GRID_COLUMNS*scale;long r=parts[1],c=parts[2];if(r>=0&&r<rows&&c>=0&&c<columns){row=(int)r/scale;col=(int)c/scale;double lat=90.0-(r+.5)*(180.0/rows),longitude=-180.0+(c+.5)*(360.0/columns);matches=1;found.push_back(SearchMatch(row,col,lat,longitude,level));return true;}}
    }
    if(count==2){long legacyRow=parts[0],legacyCol=parts[1];if(legacyRow>=0&&legacyRow<LEGACY_ROWS&&legacyCol>=0&&legacyCol<LEGACY_COLUMNS){double legacyLat=90.0-(legacyRow+.5)*(180.0/LEGACY_ROWS),legacyLon=-180.0+(legacyCol+.5)*(360.0/LEGACY_COLUMNS);row=std::min(GRID_ROWS-1,std::max(0,(int)floor((90.0-legacyLat)/GRID_CELL_DEGREES)));col=std::min(GRID_COLUMNS-1,std::max(0,(int)floor((legacyLon+180.0)/(360.0/GRID_COLUMNS))));matches=1;found.push_back(SearchMatch(row,col));if(legacyConverted)*legacyConverted=true;return true;}}
  }
  double lat=0,lo=0;if(parseSearchCoordinates(q,lat,lo)){row=std::min(GRID_ROWS-1,std::max(0,(int)floor((90.0-lat)/GRID_CELL_DEGREES)));col=std::min(GRID_COLUMNS-1,std::max(0,(int)floor((lo+180.0)/(360.0/GRID_COLUMNS))));matches=1;found.push_back(SearchMatch(row,col,lat,lo));return true;}
  std::string query=toUtf8(q);if(query.empty()||!gArchiveLoaded)return false;
  const char* fields[]={"cities","countries","rivers","lakes","mountains","landforms","history","formerNames","biomes","ecoregions","climate"};
  const std::string prefix="\"zoneId\": \"";size_t pos=gArchive.find("\"zones\": [");if(pos==std::string::npos)return false;size_t queryPos=gArchive.find(query,pos);int bestScore=0;std::vector<unsigned char> seen((size_t)(GRID_ROWS*GRID_COLUMNS),0);
  auto addMatch=[&](int candidateRow,int candidateCol,int score){if(score>bestScore){bestScore=score;found.clear();std::fill(seen.begin(),seen.end(),0);}if(score!=bestScore)return;size_t index=(size_t)candidateRow*GRID_COLUMNS+candidateCol;if(!seen[index]){seen[index]=1;found.push_back(SearchMatch(candidateRow,candidateCol));}};
  while((pos=gArchive.find(prefix,pos))!=std::string::npos){size_t value=pos+prefix.size(),idEnd=gArchive.find('\"',value);if(idEnd==std::string::npos)break;size_t next=gArchive.find(prefix,idEnd+1);size_t end=next==std::string::npos?gArchive.size():next;bool hit=false;
    if(queryPos<idEnd)queryPos=gArchive.find(query,idEnd);
    if(queryPos>=end){if(next==std::string::npos)break;pos=idEnd+1;continue;}
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
static void selectSearchResultOnMap(int row,int col){gSelectedPlaceTitle.clear();gSelectedPlaceOrigin.clear();gRow=row;gCol=col;gScrollOffset=0;gInfoWheelRemainder=0;gMapZoom=std::max(gMapZoom,4.0);gMapCenterX=(col+.5)/GRID_COLUMNS;gMapCenterY=(row+.5)/GRID_ROWS;clampMapView();gSelectedLatitude=90.0-(row+.5)*GRID_CELL_DEGREES;gSelectedLongitude=-180.0+(col+.5)*(360.0/GRID_COLUMNS);gSelectedCoordinateExact=false;gSelectedCoordinateFromInput=false;gSelectedCoordinateIsFineGridCenter=false;}
static void selectSearchResultOnMap(const SearchMatch& match){selectSearchResultOnMap(match.row,match.col);if(match.gridLevel>=0){gGridLevel=match.gridLevel;if(gGridLevelCombo)SendMessageW(gGridLevelCombo,CB_SETCURSEL,gGridLevel,0);}if(match.hasExactCoordinate){gSelectedLatitude=match.latitude;gSelectedLongitude=match.longitude;gSelectedCoordinateExact=true;gSelectedCoordinateFromInput=match.gridLevel<0;gSelectedCoordinateIsFineGridCenter=match.gridLevel>=0;gMapCenterX=(match.longitude+180)/360;gMapCenterY=(90-match.latitude)/180;clampMapView();}}
static double bounded(double value,double low,double high){return std::max(low,std::min(high,value));}
static mapViewport::Projection mapProjection(){return {gMapRect,gMapZoom,gMapCenterX,gMapCenterY};}
static double mapProjectionWidth(){return mapProjection().width();}
static double mapProjectionHeight(){return mapProjection().height();}
static void clampMapView(){auto view=mapProjection();view.clamp();gMapCenterX=view.centerX;gMapCenterY=view.centerY;}
static void resetMapView(){gMapZoom=1.0;gMapCenterX=0.5;gMapCenterY=0.5;}
static double mapWorldX(int x){return mapProjection().worldX(x);}
static double mapWorldY(int y){return mapProjection().worldY(y);}
static int mapScreenX(double fraction){return (int)std::lround(mapProjection().screenX(fraction));}
static int mapScreenY(double fraction){return (int)std::lround(mapProjection().screenY(fraction));}
static RECT mapContentRect(){return RECT{std::max(gMapRect.left,(LONG)mapScreenX(0)),std::max(gMapRect.top,(LONG)mapScreenY(0)),std::min(gMapRect.right,(LONG)mapScreenX(1)),std::min(gMapRect.bottom,(LONG)mapScreenY(1))};}
static bool selectedPointScreenPosition(int& x,int& y){if(gCol<0||!gSelectedCoordinateExact||gMapRect.right<=gMapRect.left||gMapRect.bottom<=gMapRect.top||gSelectedLatitude<-90||gSelectedLatitude>90||gSelectedLongitude<-180||gSelectedLongitude>180)return false;int px=mapScreenX((gSelectedLongitude+180.0)/360.0),py=mapScreenY((90.0-gSelectedLatitude)/180.0);if(px<gMapRect.left||px>gMapRect.right||py<gMapRect.top||py>gMapRect.bottom)return false;x=std::max((int)gMapRect.left,std::min((int)gMapRect.right-1,px));y=std::max((int)gMapRect.top,std::min((int)gMapRect.bottom-1,py));return true;}
static bool selectNasaDetailLayerForZoom(){
  std::wstring folder;int pixels=0,resolution=0,cacheLimit=0;
  if(gMap==gMapNasa&&gMapZoom>=12.0&&gNasa500mAvailable&&(!gNasa1kmAvailable||mapViewport::detailFits(mapProjection(),2400))){folder=L"nasa-500m";pixels=2400;resolution=500;cacheLimit=12;}
  else if(gMap==gMapNasa&&gMapZoom>=8.0&&gNasa1kmAvailable){folder=L"nasa-1km";pixels=1200;resolution=1000;cacheLimit=24;}
  else if(gMap==gMapNasa&&gMapZoom>=8.0&&gNasa500mAvailable){folder=L"nasa-500m";pixels=2400;resolution=500;cacheLimit=12;}
  if(!folder.empty()&&(!mapViewport::detailFits(mapProjection(),pixels)||mapViewport::visibleTiles(mapProjection())>(size_t)cacheLimit))folder.clear();
  if(folder.empty()){
    if(!gNasaDetailFolder.empty())clearNasaDetailTileCache();
    gNasaDetailFolder.clear();return false;
  }
  if(folder!=gNasaDetailFolder)clearNasaDetailTileCache();
  gNasaDetailFolder=folder;gNasaDetailPixels=pixels;gNasaDetailResolutionMeters=resolution;gNasaDetailCacheLimit=cacheLimit;return true;
}
static size_t nasaTileCacheLimitForView(){
  const size_t tileBytes=(size_t)gNasaDetailPixels*gNasaDetailPixels*4;
  const size_t budget=std::max((size_t)1,(192*1024*1024)/tileBytes);
  double left=std::max(0.0,mapWorldX(gMapRect.left)),right=std::min(1.0,mapWorldX(gMapRect.right));
  double top=std::max(0.0,mapWorldY(gMapRect.top)),bottom=std::min(1.0,mapWorldY(gMapRect.bottom));
  size_t visible=(size_t)std::max(1,(int)std::ceil(right*36)-(int)std::floor(left*36))*(size_t)std::max(1,(int)std::ceil(bottom*18)-(int)std::floor(top*18));
  // Keep a complete visible set to avoid decode/evict cycles near tile boundaries.
  return std::max((size_t)1,std::min((size_t)gNasaDetailCacheLimit,std::max(budget,visible)));
}
static Image* getNasaDetailTile(int row,int col){
  if(row<0||row>=18||col<0||col>=36)return nullptr;
  for(auto& tile:gNasaDetailTiles)if(tile.row==row&&tile.col==col){tile.lastUsed=++gNasaTileUseCounter;return tile.image;}
  ULONGLONG now=GetTickCount64();
  for(auto it=gNasaTileFailures.begin();it!=gNasaTileFailures.end();){if(it->retryAfter<=now)it=gNasaTileFailures.erase(it);else{if(it->row==row&&it->col==col)return nullptr;++it;}}
  wchar_t name[48];swprintf(name,48,L"r%02d-c%02d",row,col);std::wstring base=appDir()+L"\\assets\\"+gNasaDetailFolder+L"\\tiles\\"+name;Image* candidate=Image::FromFile((base+L".jpg").c_str());
  double west=-180.0+col*10.0,north=90.0-row*10.0;
  if(!candidate||candidate->GetLastStatus()!=Ok||candidate->GetWidth()!=gNasaDetailPixels||candidate->GetHeight()!=gNasaDetailPixels||!validateWorldMapMetadata(base,gNasaDetailPixels,gNasaDetailPixels,west,west+10.0,north,north-10.0)){delete candidate;gNasaTileLoadFailures++;gNasaTileFailures.push_back({row,col,now+5000});return nullptr;}
  const size_t limit=nasaTileCacheLimitForView();
  if(gNasaDetailTiles.size()>=limit){size_t oldest=0;for(size_t i=1;i<gNasaDetailTiles.size();i++)if(gNasaDetailTiles[i].lastUsed<gNasaDetailTiles[oldest].lastUsed)oldest=i;delete gNasaDetailTiles[oldest].image;gNasaDetailTiles.erase(gNasaDetailTiles.begin()+oldest);}
  gNasaDetailTiles.push_back(NasaDetailTile(row,col,candidate,++gNasaTileUseCounter));return candidate;
}
static void drawNasaDetailTiles(Graphics& gr){
  if(!gNasaDetailAvailable||!selectNasaDetailLayerForZoom())return;
  double left=std::max(0.0,mapWorldX(gMapRect.left)),right=std::min(1.0,mapWorldX(gMapRect.right)),top=std::max(0.0,mapWorldY(gMapRect.top)),bottom=std::min(1.0,mapWorldY(gMapRect.bottom));
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
static bool selectMapPoint(int x,int y){if(!mapContainsPoint(x,y))return false;double wx=mapWorldX(x),wy=mapWorldY(y);if(wx<0||wx>=1||wy<0||wy>=1)return false;gSelectedPlaceTitle.clear();gSelectedPlaceOrigin.clear();gCol=std::min(GRID_COLUMNS-1,std::max(0,(int)floor(wx*GRID_COLUMNS+1e-10)));gRow=std::min(GRID_ROWS-1,std::max(0,(int)floor(wy*GRID_ROWS+1e-10)));gScrollOffset=0;gInfoWheelRemainder=0;gSelectedLatitude=90.0-wy*180.0;gSelectedLongitude=-180.0+wx*360.0;gSelectedCoordinateExact=true;gSelectedCoordinateFromInput=false;gSelectedCoordinateIsFineGridCenter=false;return true;}
static double mapZoomLimit(){
  if(gMap&&gMap==gMapNasa&&gNasa500mAvailable)return 64.0;
  if(gMap&&gMap==gMapNasa&&gNasa1kmAvailable)return 32.0;
  if(gMap&&gMap==gMapHighRes&&gMapHighRes->GetWidth()>=16000)return 32.0;
  return 16.0;
}
static void zoomMapAt(int x,int y,double factor){double next=bounded(gMapZoom*factor,1.0,mapZoomLimit());if(std::fabs(next-gMapZoom)<1e-9)return;double wx=mapWorldX(x),wy=mapWorldY(y);gMapZoom=next;gMapCenterX=wx-(x-(gMapRect.left+gMapRect.right)/2.0)/(mapProjectionWidth()*gMapZoom);gMapCenterY=wy-(y-(gMapRect.top+gMapRect.bottom)/2.0)/(mapProjectionHeight()*gMapZoom);clampMapView();}
static void panMapBy(int dx,int dy){double width=mapProjectionWidth(),height=mapProjectionHeight();gMapCenterX-=dx/(width*gMapZoom);gMapCenterY-=dy/(height*gMapZoom);clampMapView();}
static std::wstring num(double x){ std::wostringstream o;o<<std::fixed<<std::setprecision(2)<<std::fabs(x)<<L"°"<<(x>=0?L"N":L"S");return o.str(); }
static std::wstring lon(double x){ std::wostringstream o;o<<std::fixed<<std::setprecision(2)<<std::fabs(x)<<L"°"<<(x>=0?L"E":L"W");return o.str(); }
static std::wstring preciseLatitude(double x){std::wostringstream o;o<<std::fixed<<std::setprecision(4)<<std::fabs(x)<<L"°"<<(x>=0?L"N":L"S");return o.str();}
static std::wstring preciseLongitude(double x){std::wostringstream o;o<<std::fixed<<std::setprecision(4)<<std::fabs(x)<<L"°"<<(x>=0?L"E":L"W");return o.str();}
static std::wstring selectedCoordinateText(){if(!gSelectedCoordinateExact)return L"定位参考：分区中心";const wchar_t* source=gSelectedCoordinateFromInput?L"输入坐标 ":(gSelectedCoordinateIsFineGridCenter?L"网格单元中心 ":L"点击换算 ");return std::wstring(source)+preciseLatitude(gSelectedLatitude)+L"，"+preciseLongitude(gSelectedLongitude);}
static std::wstring gridCellText(){std::wostringstream o;o<<std::fixed<<std::setprecision(2)<<GRID_CELL_DEGREES;return o.str();}
static std::wstring fineGridCellText(){std::wostringstream o;o<<std::fixed<<std::setprecision(4)<<180.0/activeGridRows();return o.str();}
static std::wstring fineGridBoundsText(int row,int col){int rows=activeGridRows(),columns=activeGridColumns();double north=90.0-row*(180.0/rows),south=90.0-(row+1)*(180.0/rows),west=-180.0+col*(360.0/columns),east=-180.0+(col+1)*(360.0/columns);return std::wstring(gEnglish?L"North: ":L"北 ")+preciseLatitude(north)+(gEnglish?L" · South: ":L" · 南 ")+preciseLatitude(south)+(gEnglish?L"\r\nWest: ":L"\r\n西 ")+preciseLongitude(west)+(gEnglish?L" · East: ":L" · 东 ")+preciseLongitude(east);}
static std::wstring baseRegionBoundsText(int row,int col){
  if(gEnglish)return num(90.0-row*GRID_CELL_DEGREES)+L" to "+num(90.0-(row+1)*GRID_CELL_DEGREES)+L"; "+lon(-180.0+col*GRID_CELL_DEGREES)+L" to "+lon(-180.0+(col+1)*GRID_CELL_DEGREES)+L"; Grid "+gridCellText()+L"° × "+gridCellText()+L"°";
  return archiveZoneField(row,col,"latitudeBand",L"")+L"；"+archiveZoneField(row,col,"longitudeBand",L"")+L"；网格 "+gridCellText()+L"° × "+gridCellText()+L"°";
}
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
  if(gGridLevel>0)status+=L" · 基础网格 "+gridIdText(GRID_VERSION,parentRow,parentCol);
  return status;
}
static void invalidateNotice(HWND w){if(!gMapFullscreen&&gExperienceStatusRect.right>gExperienceStatusRect.left)InvalidateRect(w,&gExperienceStatusRect,FALSE);}
static std::wstring terrain(double lat,double lo){if(std::fabs(lat)>66)return L"极地冰盖、冻原或寒冷海域";if(std::fabs(lat)<15)return L"热带雨林、热带草原、岛屿或赤道海域";if(std::fabs(lat)<32&&lo>-20&&lo<65)return L"干旱—半干旱带、荒漠与高原交错";return L"山地、平原、丘陵或温带海域";}
static std::wstring nearCity(double lat,double lo){struct C{const wchar_t*n;double a,b;};static C c[]={ {L"北京",39.9,116.4},{L"东京",35.7,139.7},{L"新加坡",1.35,103.8},{L"悉尼",-33.9,151.2},{L"巴黎",48.9,2.35},{L"伦敦",51.5,-.13},{L"纽约",40.7,-74},{L"洛杉矶",34.1,-118.2},{L"开罗",30,31.2},{L"圣保罗",-23.6,-46.6} };double best=1e9;const wchar_t*name=nullptr;for(auto &v:c){double d=(v.a-lat)*(v.a-lat)+(v.b-lo)*(v.b-lo);if(d<best){best=d;name=v.n;}}return best<324?std::wstring(name)+L"（附近已编入城市）":L"本区暂无已编入的主要城市";}
static HFONT uiFontFor(int size,bool bold){
  for(const auto& entry:gUiFonts)if(entry.size==size&&entry.bold==bold&&entry.english==gEnglish&&entry.dpi==gUiDpi)return entry.handle;
  LOGFONTW spec{};spec.lfHeight=-uiPx(size);spec.lfWeight=bold?FW_BOLD:FW_NORMAL;spec.lfCharSet=DEFAULT_CHARSET;spec.lfOutPrecision=OUT_DEFAULT_PRECIS;spec.lfClipPrecision=CLIP_DEFAULT_PRECIS;spec.lfQuality=CLEARTYPE_QUALITY;spec.lfPitchAndFamily=DEFAULT_PITCH;lstrcpynW(spec.lfFaceName,gEnglish?L"Segoe UI":L"Microsoft YaHei UI",LF_FACESIZE);
  HFONT handle=CreateFontIndirectW(&spec);if(handle)gUiFonts.push_back(UiFontEntry{size,bold,gEnglish,gUiDpi,handle});return handle?handle:(HFONT)GetStockObject(DEFAULT_GUI_FONT);
}
static BOOL CALLBACK refreshUiChildFont(HWND child,LPARAM){HFONT current=(HFONT)SendMessageW(child,WM_GETFONT,0,0);int size=13;bool bold=false;for(const auto& entry:gUiFonts)if(entry.handle==current){size=entry.size;bold=entry.bold;break;}SendMessageW(child,WM_SETFONT,(WPARAM)uiFontFor(size,bold),TRUE);return TRUE;}
static void refreshUiChildFonts(HWND window){EnumChildWindows(window,refreshUiChildFont,0);InvalidateRect(window,nullptr,TRUE);}
static BOOL CALLBACK broadcastUiScale(HWND window,LPARAM oldMode){SendMessageW(window,UI_SCALE_CHANGED,(WPARAM)uiDpiForMode((int)oldMode,window),0);return TRUE;}
static void populateUiScaleChoices(HWND window){if(!gUiScaleCombo)return;SendMessageW(gUiScaleCombo,CB_RESETCONTENT,0,0);for(const wchar_t* value:std::initializer_list<const wchar_t*>{gEnglish?L"Follow Windows (recommended)":L"跟随 Windows（推荐）",L"100%",L"125%",L"150%",L"200%"})SendMessageW(gUiScaleCombo,CB_ADDSTRING,0,(LPARAM)value);SendMessageW(gUiScaleCombo,CB_SETCURSEL,gUiScaleMode,0);}
static void clearUiFonts(){for(const auto& entry:gUiFonts)DeleteObject(entry.handle);gUiFonts.clear();}
static std::wstring infoDisplayText(const std::wstring& source,bool sourceNarrative){
  if(!sourceNarrative)return localizeUiText(source);
  static const wchar_t* fallbacks[]={L"档案未加载",L"暂无行政区匹配",L"尚无地貌记录",L"尚无地表记录",L"尚无生物群系记录",L"尚无生态区记录",L"尚无气候分类记录",L"尚无地点记录",L"尚无城市记录",L"尚无河流记录",L"尚无水域记录",L"尚无山地记录",L"尚无地貌名称",L"尚无历史地点记录",L"尚无曾用名记录",L"详见随附数据说明",L"地表覆盖数据层未安装；可单独补丁安装",L"此独立数据层未安装，不影响地图与其他档案",L"UNESCO 世界遗产离线数据模块未安装；可通过独立数据补丁添加。"};
  for(const auto* fallback:fallbacks)if(source==fallback)return localizeUiText(source);
  return source;
}
static void text(HDC hdc,const std::wstring&s,RECT r,int size,COLORREF color,bool bold=false,bool sourceNarrative=false){std::wstring display=infoDisplayText(s,sourceNarrative);HFONT f=uiFontFor(size,bold);auto old=(HFONT)SelectObject(hdc,f);SetTextColor(hdc,color);SetBkMode(hdc,TRANSPARENT);DrawTextW(hdc,display.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(hdc,old);}
static int measureText(HDC hdc,const std::wstring& s,int width,int size,bool bold=false,bool sourceNarrative=false){std::wstring display=infoDisplayText(s,sourceNarrative);HFONT f=uiFontFor(size,bold);auto old=(HFONT)SelectObject(hdc,f);RECT r{0,0,width,0};DrawTextW(hdc,display.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX|DT_CALCRECT);SelectObject(hdc,old);return std::max(uiPx(size+2),(int)(r.bottom-r.top));}
static void drawSearchPlaceholder(HWND edit,HDC dc){
  if(GetWindowTextLengthW(edit)||GetFocus()==edit)return;
  RECT bounds{};GetClientRect(edit,&bounds);InflateRect(&bounds,-uiPx(5),0);int saved=SaveDC(dc);
  SelectObject(dc,uiFontFor(13,false));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(166,194,208));
  DrawTextW(dc,gEnglish?L"Places, events, journeys… (Ctrl+F)":L"搜索地点、事件、路线…（Ctrl+F）",-1,&bounds,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);RestoreDC(dc,saved);
}
static LRESULT CALLBACK searchEditSubclass(HWND edit,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR){
  UiDpiScope dpi(edit);LRESULT result=DefSubclassProc(edit,message,wp,lp);
  if(message==WM_PAINT){HDC dc=GetDC(edit);if(dc){drawSearchPlaceholder(edit,dc);ReleaseDC(edit,dc);}}
  if(message==WM_PRINTCLIENT||(message==WM_PRINT&&(lp&PRF_CLIENT)))drawSearchPlaceholder(edit,(HDC)wp);
  if(message==WM_SETFOCUS||message==WM_KILLFOCUS||message==WM_SETTEXT)InvalidateRect(edit,nullptr,FALSE);
  if(message==WM_NCDESTROY)RemoveWindowSubclass(edit,searchEditSubclass,id);
  return result;
}
static bool isToggleButtonId(int id){return id==1085||id==1001||id==1004||id==1005||id==1006||id==1007||id==1009||id==1011||id==1014||id==1020;}
static LRESULT CALLBACK styledButtonSubclass(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam,UINT_PTR id,DWORD_PTR){
  if(isToggleButtonId((int)id)&&message==BM_GETCHECK)return GetWindowLongPtrW(hwnd,GWLP_USERDATA)?BST_CHECKED:BST_UNCHECKED;
  if(isToggleButtonId((int)id)&&message==BM_SETCHECK){SetWindowLongPtrW(hwnd,GWLP_USERDATA,wParam==BST_CHECKED?1:0);InvalidateRect(hwnd,nullptr,TRUE);return 0;}
  if(message==WM_MOUSEMOVE){if(gHoveredButtonId!=(int)id){gHoveredButtonId=(int)id;InvalidateRect(hwnd,nullptr,FALSE);}TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);}
  if(message==WM_MOUSELEAVE){if(gHoveredButtonId==(int)id)gHoveredButtonId=0;InvalidateRect(hwnd,nullptr,FALSE);}
  if(message==WM_NCDESTROY)RemoveWindowSubclass(hwnd,styledButtonSubclass,id);
  return DefSubclassProc(hwnd,message,wParam,lParam);
}
static bool styledButtonActive(int id,HWND hwnd){if(id>=1070&&id<=1072)return (int)gWorkspace==id-1070;if(id==1074||id==1075)return gExperiencePanel==(id==1074?1:2);if(id==1083)return gCol>=0&&savedPlaces::find(gSavedPlaces,gSelectedLatitude,gSelectedLongitude)>=0;if(isToggleButtonId(id))return SendMessageW(hwnd,BM_GETCHECK,0,0)==BST_CHECKED;if(id==1015)return gHistoryPlaying||gHistoryPaused;if(id==1016)return gHistorySpeedIndex!=0;if(id==1018)return gInfoTab==0;if(id==1019)return gInfoTab==1;if(id==1023)return gMapFullscreen;if(id>=1061&&id<=1064)return gInfoTab==0&&gRegionPage==id-1061;return false;}
static void drawStyledButton(const DRAWITEMSTRUCT* item){
  UiDpiScope dpi(item->hwndItem);int id=(int)item->CtlID;RECT r=item->rcItem;bool active=styledButtonActive(id,item->hwndItem),hover=gHoveredButtonId==id,pressed=(item->itemState&ODS_SELECTED)!=0,disabled=(item->itemState&ODS_DISABLED)!=0;
  COLORREF fill=active?uiTheme::selected:uiTheme::button,edge=active?RGB(97,204,191):RGB(58,91,105),foreground=uiTheme::text;
  if(hover&&!active){fill=RGB(32,67,80);edge=RGB(108,168,176);}if(pressed){fill=RGB(17,126,132);edge=RGB(143,225,209);}if(disabled){fill=RGB(25,39,47);edge=RGB(57,71,78);foreground=RGB(137,151,156);}
  int saved=SaveDC(item->hDC);SetBkMode(item->hDC,TRANSPARENT);COLORREF backdrop=uiTheme::toolbar;if(id==1003||id==1023||id==1024||id==1082||id==1083)backdrop=uiTheme::header;else if(id==1018||id==1019||id==1065||id==1066||id==1049||(id>=1061&&id<=1064)||(id>=1140&&id<=1145))backdrop=uiTheme::panel;else if(id==1047||id==1048||(id>=1040&&id<=1045))backdrop=RGB(12,30,43);else if(id==1050||id==1051||id==1052||id==1076||id==1077)backdrop=uiTheme::canvas;if(gExperiencePanel&&(id==1084||id==1085||id==1053||isToggleButtonId(id)))backdrop=RGB(15,39,54);HBRUSH background=CreateSolidBrush(backdrop);FillRect(item->hDC,&r,background);DeleteObject(background);HBRUSH brush=CreateSolidBrush(fill);HPEN pen=CreatePen(PS_SOLID,std::max(1,uiPx(1)),edge);HGDIOBJ oldBrush=SelectObject(item->hDC,brush),oldPen=SelectObject(item->hDC,pen);RoundRect(item->hDC,r.left,r.top,r.right,r.bottom,uiPx(8),uiPx(8));SelectObject(item->hDC,oldBrush);SelectObject(item->hDC,oldPen);DeleteObject(brush);DeleteObject(pen);
  if((id==1018||id==1019)&&active){HBRUSH accent=CreateSolidBrush(RGB(99,227,208));RECT line{r.left+uiPx(8),r.bottom-uiPx(3),r.right-uiPx(8),r.bottom-uiPx(2)};FillRect(item->hDC,&line,accent);DeleteObject(accent);}
  if(id==1018||id==1019){HPEN glyphPen=CreatePen(PS_SOLID,std::max(1,uiPx(1)),foreground);HGDIOBJ oldGlyph=SelectObject(item->hDC,glyphPen),oldGlyphBrush=SelectObject(item->hDC,GetStockObject(HOLLOW_BRUSH));int gx=r.left+uiPx(12),gy=(r.top+r.bottom)/2;if(id==1018){Ellipse(item->hDC,gx-uiPx(6),gy-uiPx(6),gx+uiPx(6),gy+uiPx(6));MoveToEx(item->hDC,gx-uiPx(6),gy,nullptr);LineTo(item->hDC,gx+uiPx(6),gy);MoveToEx(item->hDC,gx,gy-uiPx(6),nullptr);LineTo(item->hDC,gx,gy+uiPx(6));}else{for(int row=0;row<3;row++){int yy=gy-uiPx(5)+row*uiPx(5);Ellipse(item->hDC,gx-uiPx(6),yy-1,gx-uiPx(4),yy+1);MoveToEx(item->hDC,gx-uiPx(1),yy,nullptr);LineTo(item->hDC,gx+uiPx(7),yy);}}SelectObject(item->hDC,oldGlyphBrush);SelectObject(item->hDC,oldGlyph);DeleteObject(glyphPen);}
  wchar_t label[256]{};GetWindowTextW(item->hwndItem,label,256);bool smallLabel=((id==1018||id==1019)&&r.right-r.left<uiPx(120))||(id==1048&&r.right-r.left<uiPx(115))||(id>=1061&&id<=1064&&r.right-r.left<uiPx(75));HFONT font=uiFontFor(smallLabel?11:13,false),oldFont=(HFONT)SelectObject(item->hDC,font);SetTextColor(item->hDC,foreground);RECT textRect=r;InflateRect(&textRect,-uiPx(5),-uiPx(1));if(id==1018||id==1019)textRect.left+=uiPx(10);DrawTextW(item->hDC,label,-1,&textRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(item->hDC,oldFont);
  if(item->itemState&ODS_FOCUS){RECT focus=r;InflateRect(&focus,-uiPx(4),-uiPx(4));DrawFocusRect(item->hDC,&focus);}RestoreDC(item->hDC,saved);
}
struct PickerState{HWND hint=nullptr,edit=nullptr,list=nullptr,status=nullptr,ok=nullptr,cancel=nullptr;std::vector<std::wstring> labels;int initial=-1,result=-1;};
static RECT pickerWindowBounds(const RECT& owner,const RECT& work){
  int width=std::max(1,std::min(uiPx(536),std::min((int)(work.right-work.left)-uiPx(20),(int)(owner.right-owner.left)-uiPx(20))));
  int height=std::max(1,std::min(uiPx(482),std::min((int)(work.bottom-work.top)-uiPx(20),(int)(owner.bottom-owner.top)-uiPx(20))));
  int x=std::max((int)work.left,std::min((int)work.right-width,((int)owner.left+(int)owner.right-width)/2));
  int y=std::max((int)work.top,std::min((int)work.bottom-height,((int)owner.top+(int)owner.bottom-height)/2));return RECT{x,y,x+width,y+height};
}
static void layoutPicker(HWND window,PickerState* state){
  if(!state)return;RECT actual{};GetClientRect(window,&actual);RECT client=uiDipRect(actual);int contentWidth=std::max(1,(int)client.right-40),buttonY=std::max(87,(int)client.bottom-42),listHeight=std::max(1,buttonY-101);
  if(state->hint)placeUiControl(state->hint,20,17,contentWidth,22);
  if(state->edit)placeUiControl(state->edit,20,44,contentWidth,31);
  if(state->list)placeUiControl(state->list,20,86,contentWidth,listHeight);
  if(state->status)placeUiControl(state->status,20,buttonY+5,std::max(1,(int)client.right-240),22);
  if(state->ok)placeUiControl(state->ok,std::max(20,(int)client.right-202),buttonY,86,32);
  if(state->cancel)placeUiControl(state->cancel,std::max(20,(int)client.right-106),buttonY,86,32);
}
static std::wstring pickerFold(std::wstring s){for(auto& c:s)c=(wchar_t)towlower(c);return s;}
static void filterPicker(PickerState* state){
  wchar_t query[256]{};GetWindowTextW(state->edit,query,256);std::wstring needle=pickerFold(query);
  SendMessageW(state->list,LB_RESETCONTENT,0,0);int selected=-1;
  for(size_t i=0;i<state->labels.size();++i){if(!needle.empty()&&pickerFold(state->labels[i]).find(needle)==std::wstring::npos)continue;int row=(int)SendMessageW(state->list,LB_ADDSTRING,0,(LPARAM)state->labels[i].c_str());SendMessageW(state->list,LB_SETITEMDATA,row,(LPARAM)i);if((int)i==state->initial)selected=row;}
  int count=(int)SendMessageW(state->list,LB_GETCOUNT,0,0);if(selected<0&&count>0)selected=0;if(selected>=0)SendMessageW(state->list,LB_SETCURSEL,selected,0);
  if(state->status){std::wstring status=count?(gEnglish?std::to_wstring(count)+L" matches":L"匹配 "+std::to_wstring(count)+L" 项"):(gEnglish?L"No matches":L"没有匹配结果");SetWindowTextW(state->status,status.c_str());}
}
static LRESULT CALLBACK pickerProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
  UiDpiScope dpi(w,msg==WM_DPICHANGED?LOWORD(wp):0);
  PickerState* state=(PickerState*)GetWindowLongPtrW(w,GWLP_USERDATA);
  if(msg==WM_NCCREATE){state=(PickerState*)((CREATESTRUCTW*)lp)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)state);}
  if(msg==WM_CREATE){HINSTANCE instance=GetModuleHandleW(nullptr);
    state->hint=CreateWindowExW(0,L"STATIC",gEnglish?L"Type to filter, then choose an item":L"输入关键词筛选，然后选择一项",WS_CHILD|WS_VISIBLE,20,17,480,22,w,nullptr,instance,nullptr);
    state->edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,20,44,480,31,w,(HMENU)10,instance,nullptr);
    state->list=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,20,86,480,300,w,(HMENU)11,instance,nullptr);
    state->status=CreateWindowExW(0,L"STATIC",L"",WS_CHILD|WS_VISIBLE,20,406,270,22,w,nullptr,instance,nullptr);
    state->ok=CreateWindowExW(0,L"BUTTON",gEnglish?L"Select":L"选择",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,318,401,86,32,w,(HMENU)IDOK,instance,nullptr);
    state->cancel=CreateWindowExW(0,L"BUTTON",gEnglish?L"Cancel":L"取消",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,414,401,86,32,w,(HMENU)IDCANCEL,instance,nullptr);
    SetWindowSubclass(state->ok,styledButtonSubclass,IDOK,0);SetWindowSubclass(state->cancel,styledButtonSubclass,IDCANCEL,0);
    for(HWND control:{state->hint,state->edit,state->list,state->status,state->ok,state->cancel})SendMessageW(control,WM_SETFONT,(WPARAM)uiFontFor(15,false),TRUE);
    layoutPicker(w,state);filterPicker(state);SetFocus(state->edit);return 0;
  }
  if((msg==WM_DPICHANGED||msg==UI_SCALE_CHANGED)&&state){refreshUiChildFonts(w);applyUiWindowBounds(w,msg,wp,lp);layoutPicker(w,state);return 0;}
  if(msg==WM_SIZE&&state){layoutPicker(w,state);return 0;}
  if(msg==WM_DRAWITEM&&((const DRAWITEMSTRUCT*)lp)->CtlType==ODT_BUTTON){drawStyledButton((const DRAWITEMSTRUCT*)lp);return TRUE;}
  if(msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX){HDC dc=(HDC)wp;SetTextColor(dc,uiTheme::text);SetBkColor(dc,RGB(18,43,57));return (LRESULT)gUiControlBrush;}
  if(msg==WM_CTLCOLORSTATIC){HDC dc=(HDC)wp;SetTextColor(dc,RGB(187,220,228));SetBkColor(dc,uiTheme::toolbar);return (LRESULT)gUiPanelBrush;}
  if(msg==WM_COMMAND&&state){if(LOWORD(wp)==10&&HIWORD(wp)==EN_CHANGE){filterPicker(state);return 0;}if(LOWORD(wp)==IDOK||(LOWORD(wp)==11&&HIWORD(wp)==LBN_DBLCLK)){int row=(int)SendMessageW(state->list,LB_GETCURSEL,0,0);if(row!=LB_ERR){state->result=(int)SendMessageW(state->list,LB_GETITEMDATA,row,0);DestroyWindow(w);}return 0;}if(LOWORD(wp)==IDCANCEL){DestroyWindow(w);return 0;}}
  if(msg==WM_CLOSE){DestroyWindow(w);return 0;}if(msg==WM_DESTROY){PostMessageW(GetWindow(w,GW_OWNER),WM_NULL,0,0);return 0;}return DefWindowProcW(w,msg,wp,lp);
}
static int showPicker(HWND owner,const wchar_t* title,const std::vector<std::wstring>& labels,int initial){
  UiDpiScope dpi(owner);HWND previousFocus=GetFocus();
  WNDCLASSW cls{};cls.hIcon=(HICON)LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(101),IMAGE_ICON,0,0,LR_SHARED|LR_DEFAULTSIZE);cls.lpfnWndProc=pickerProc;cls.hInstance=GetModuleHandleW(nullptr);cls.hCursor=LoadCursor(nullptr,IDC_ARROW);cls.hbrBackground=gUiPanelBrush;cls.lpszClassName=L"WorldMapExplorerPicker";RegisterClassW(&cls);
  PickerState state;state.labels=labels;state.initial=initial;RECT parent{};GetWindowRect(owner,&parent);MONITORINFO monitor{sizeof(MONITORINFO)};RECT work=parent;if(GetMonitorInfoW(MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST),&monitor))work=monitor.rcWork;RECT bounds=pickerWindowBounds(parent,work);
  HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,title,WS_CAPTION|WS_SYSMENU|WS_POPUP|WS_VISIBLE,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,owner,nullptr,cls.hInstance,&state);if(!dialog)return -1;
  EnableWindow(owner,FALSE);MSG message{};int received=1;while(IsWindow(dialog)&&(received=GetMessageW(&message,nullptr,0,0))>0){if(message.message==WM_KEYDOWN&&message.wParam==VK_RETURN){SendMessageW(dialog,WM_COMMAND,GetDlgCtrlID(GetFocus())==IDCANCEL?IDCANCEL:IDOK,0);continue;}if(message.message==WM_KEYDOWN&&message.wParam==VK_ESCAPE){SendMessageW(dialog,WM_COMMAND,IDCANCEL,0);continue;}if(!IsDialogMessageW(dialog,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}if(received==0)PostQuitMessage((int)message.wParam);EnableWindow(owner,TRUE);SetForegroundWindow(owner);if(previousFocus&&IsWindow(previousFocus)&&IsChild(owner,previousFocus)&&IsWindowVisible(previousFocus)&&IsWindowEnabled(previousFocus))SetFocus(previousFocus);return state.result;
}
static void updatePickerCaptions(){
  if(gYearPickerButton)SetWindowTextW(gYearPickerButton,(historyYearText(gHistoryYear)+L"  \u25be").c_str());
  if(gRoutePickerButton){const auto& all=routes::catalog();const wchar_t* caption=gRouteIndex>=0&&(size_t)gRouteIndex<all.size()?(gEnglish?all[gRouteIndex].en:all[gRouteIndex].zh):(gEnglish?L"Choose a route":L"选择路线");SetWindowTextW(gRoutePickerButton,(std::wstring(caption)+L"  \u25be").c_str());}
}
static void drawCalibrationPoints(Graphics& gr){
  struct Point{const wchar_t* label;double lat,lon;};
  static const Point points[]={{L"0°,0°",0,0},{L"纽约",40.7128,-74.0060},{L"基多",-0.1807,-78.4678},{L"北京",39.9042,116.4074},{L"新加坡",1.3521,103.8198},{L"开普敦",-33.9249,18.4241},{L"悉尼",-33.8688,151.2093}};
  Pen marker(Color(255,255,86,112),2);SolidBrush dot(Color(235,255,225,90)),labelBrush(Color(255,255,245,220));Font labelFont(L"Microsoft YaHei UI",uiPxF(10),FontStyleBold,UnitPixel);
  for(const auto& p:points){
    int x=mapScreenX((p.lon+180.0)/360.0);
    int y=mapScreenY((90.0-p.lat)/180.0);
    gr.FillEllipse(&dot,x-4,y-4,8,8);gr.DrawEllipse(&marker,x-5,y-5,10,10);
    std::wstring label=localizeUiText(p.label);gr.DrawString(label.c_str(),-1,&labelFont,PointF((REAL)x+6,(REAL)y-11),&labelBrush);
  }
}
static void drawSelectedPoint(Graphics& gr){
  int x=0,y=0;if(!selectedPointScreenPosition(x,y))return;
  int radius=uiPx(std::max(5,std::min(9,(int)std::lround(5.0+1.25*std::log(gMapZoom)/std::log(2.0)))));
  Pen halo(Color(245,5,18,30),uiPxF(5)),cross(Color(255,55,224,244),uiPxF(2)),ring(Color(255,246,252,255),uiPxF(2));SolidBrush center(Color(255,55,224,244));
  gr.DrawLine(&halo,x-uiPx(12),y,x+uiPx(12),y);gr.DrawLine(&halo,x,y-uiPx(12),x,y+uiPx(12));gr.DrawEllipse(&halo,x-radius-1,y-radius-1,2*radius+2,2*radius+2);
  gr.DrawLine(&cross,x-uiPx(12),y,x+uiPx(12),y);gr.DrawLine(&cross,x,y-uiPx(12),x,y+uiPx(12));gr.DrawEllipse(&ring,x-radius,y-radius,2*radius,2*radius);gr.FillEllipse(&center,x-uiPx(2),y-uiPx(2),uiPx(4),uiPx(4));
}
static void drawLandCoverOverlay(Graphics& gr){
  if(!gShowLandCover||!gLandCoverLoaded||!gLandCoverRaster)return;
  int x1=mapScreenX(0),x2=mapScreenX(1),y1=mapScreenY(0),y2=mapScreenY(1);InterpolationMode previous=gr.GetInterpolationMode();gr.SetInterpolationMode(InterpolationModeNearestNeighbor);gr.DrawImage(gLandCoverRaster,x1,y1,x2-x1,y2-y1);gr.SetInterpolationMode(previous);
}
struct UiHudTransform{Graphics& graphics;GraphicsState state;explicit UiHudTransform(Graphics& g):graphics(g),state(g.Save()){g.ScaleTransform(uiScaleFactor(),uiScaleFactor());}~UiHudTransform(){graphics.Restore(state);}};
static void drawLandCoverLegend(Graphics& gr,int mapWidth,int mapHeight){
  UiHudTransform transform(gr);const RECT content=uiDipRect(mapContentRect());mapWidth=content.right-content.left;mapHeight=content.bottom-content.top;
  gLandCoverLegendRect=RECT{0,0,0,0};if(!gShowLandCover||!gLandCoverLoaded||gLandCoverLegend.size()!=22)return;
  if(mapWidth<350||mapHeight<250){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));gr.DrawString(localizeUiText(L"放大窗口可查看完整图例").c_str(),-1,&hintFont,PointF((REAL)content.left+10,(REAL)content.top+76),&hintBrush);return;}
  int left=content.left+9,top=content.bottom-207,width=std::max(330,std::min(430,mapWidth-18)),height=198;if(gShowHistory&&gHistoryLoaded&&gHistoryRaster&&mapHeight>=310)top-=54;gLandCoverLegendRect=uiRect(left,top,left+width,top+height);SolidBrush panel(Color(255,6,19,31));Pen border(Color(240,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);
  Font titleFont(L"Microsoft YaHei UI",10,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,240,246,249));gr.DrawString(localizeUiText(L"2020 土地覆盖主类 · 分区主类着色").c_str(),-1,&titleFont,PointF((REAL)left+8,(REAL)top+4),&titleBrush);
  int columnWidth=(width-24)/2;StringFormat labelFormat;labelFormat.SetTrimming(StringTrimmingEllipsisCharacter);labelFormat.SetFormatFlags(StringFormatFlagsNoWrap);
  for(size_t i=0;i<gLandCoverLegend.size();i++){int column=(int)(i/11),row=(int)(i%11),x=left+8+column*columnWidth,y=top+24+row*15;const auto& category=gLandCoverLegend[i];BYTE red=(BYTE)((category.rgb>>16)&255),green=(BYTE)((category.rgb>>8)&255),blue=(BYTE)(category.rgb&255);SolidBrush swatch(Color(255,red,green,blue));Pen outline(Color(220,245,250,250),1);gr.FillRectangle(&swatch,x,y+2,10,10);gr.DrawRectangle(&outline,x,y+2,10,10);std::wstring label=localizeUiText(std::to_wstring(category.code)+L" "+category.name);RectF labelRect((REAL)x+14,(REAL)y,(REAL)std::max(1,columnWidth-26),15);gr.DrawString(label.c_str(),-1,&labelFont,labelRect,&labelFormat,&labelBrush);}
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
  if(gHistoryVectors.valid()){
    float alpha=1.0f;if(gHistoryTransitionActive&&gHistoryTransitionPreviousRaster){alpha=historyTransitionProgress();drawRaster(gHistoryTransitionPreviousRaster,1.0f-alpha);}
    gHistoryVectors.draw(gr,gMapRect,gMapZoom,gMapCenterX,gMapCenterY,(BYTE)std::lround(255*gFillOpacity/100.0*alpha));
  }else if(gHistoryTransitionActive&&gHistoryTransitionPreviousRaster){float progress=historyTransitionProgress();drawRaster(gHistoryTransitionPreviousRaster,1.0f-progress);drawRaster(gHistoryRaster,progress);}else drawRaster(gHistoryRaster,1.0f);gr.SetInterpolationMode(previous);
}
static bool updateRouteRaster(){
  const auto& all=routes::catalog();
  if(gRouteIndex<0 || (size_t)gRouteIndex>=all.size())return false;
  auto points=routes::pointsFor(all[gRouteIndex]);if(gRouteDistanceIndex!=gRouteIndex){gRouteDistance=routes::DistanceProfile(points);gRouteDistanceIndex=gRouteIndex;}
  auto cells=routes::rasterize(points,FINE_GRID_COLUMNS,FINE_GRID_ROWS,gRouteProgress,0);
  if(cells.size()!=(size_t)FINE_GRID_COLUMNS*FINE_GRID_ROWS)return false;
  Bitmap* bitmap=new Bitmap(FINE_GRID_COLUMNS,FINE_GRID_ROWS,PixelFormat32bppARGB);
  if(!bitmap||bitmap->GetLastStatus()!=Ok){delete bitmap;return false;}
  Rect bounds(0,0,FINE_GRID_COLUMNS,FINE_GRID_ROWS);BitmapData data{};
  if(bitmap->LockBits(&bounds,ImageLockModeWrite,PixelFormat32bppARGB,&data)!=Ok){delete bitmap;return false;}
  for(int y=0;y<FINE_GRID_ROWS;y++){
    auto* row=reinterpret_cast<unsigned int*>(static_cast<unsigned char*>(data.Scan0)+(ptrdiff_t)y*data.Stride);
    for(int x=0;x<FINE_GRID_COLUMNS;x++)row[x]=cells[(size_t)y*FINE_GRID_COLUMNS+x]?0xeaff971b:0;
  }
  bitmap->UnlockBits(&data);delete gRouteRaster;gRouteRaster=bitmap;return true;
}
static bool routeSegmentTouchesRect(double x0,double y0,double x1,double y1,const RECT& rect){
  double t0=0,t1=1,dx=x1-x0,dy=y1-y0;
  auto clip=[&](double p,double q){if(std::fabs(p)<1e-12)return q>=0;double t=q/p;if(p<0){if(t>t1)return false;t0=std::max(t0,t);}else{if(t<t0)return false;t1=std::min(t1,t);}return true;};
  return clip(-dx,x0-rect.left)&&clip(dx,rect.right-x0)&&clip(-dy,y0-rect.top)&&clip(dy,rect.bottom-y0);
}
static RECT routeLegendPlacement(const routes::Campaign& route,int width,int height){
  const RECT content=mapContentRect();
  int left=content.left+uiPx(9),right=content.right-width-9,top=content.top+(uiPx(gMapFullscreen?9:49)),bottom=content.bottom-height-9;
  RECT options[]={{left,top,left+width,top+height},{right,top,right+width,top+height},{left,bottom,left+width,bottom+height},{right,bottom,right+width,bottom+height}};
  int best=-1,bestScore=INT_MAX;
  for(int candidate=0;candidate<4;candidate++){
    RECT area=options[candidate];InflateRect(&area,uiPx(5),uiPx(5));int score=candidate>=2?1:0;
    RECT hudOverlap{};if(IntersectRect(&hudOverlap,&area,&gMapScaleRect))continue;
    if(!gMapFullscreen&&IntersectRect(&hudOverlap,&area,&gMapNavigationRect))continue;
    if(!gMapFullscreen){
      RECT overlap{};
      if(gShowLandCover&&IntersectRect(&overlap,&area,&gLandCoverLegendRect))continue;
      if(gShowPopulation&&IntersectRect(&overlap,&area,&gPopulationLegendRect))continue;
      if(gShowWorld2026&&IntersectRect(&overlap,&area,&gWorld2026LegendRect))continue;
      if(gShowHistory&&IntersectRect(&overlap,&area,&gHistoryLegendRect))continue;
    }
    for(size_t i=0;i<route.stops.size();i++){
      const auto& p=route.stops[i].position;int px=mapScreenX((p.longitude+180.0)/360.0),py=mapScreenY((90.0-p.latitude)/180.0);
      if(px>=area.left&&px<=area.right&&py>=area.top&&py<=area.bottom)score+=10;
      if(i==0||p.breakBefore)continue;
      const auto& a=route.stops[i-1].position;double ax=(a.longitude+180.0)/360.0,bx=(p.longitude+180.0)/360.0,dx=bx-ax;if(dx>0.5)dx-=1;if(dx<-.5)dx+=1;
      double ay=(90.0-a.latitude)/180.0,by=(90.0-p.latitude)/180.0;
      for(int wrap=-1;wrap<=1;wrap++)if(routeSegmentTouchesRect(mapScreenX(ax+wrap),mapScreenY(ay),mapScreenX(ax+dx+wrap),mapScreenY(by),area))score+=2;
    }
    if(score<bestScore){bestScore=score;best=candidate;}
  }
  return best>=0?options[best]:RECT{};
}
static void drawRouteOverlay(Graphics& gr){
  const auto& all=routes::catalog();if(!gRouteRaster||gRouteIndex<0||(size_t)gRouteIndex>=all.size())return;
  const auto& route=all[gRouteIndex];int x1=mapScreenX(0),x2=mapScreenX(1),y1=mapScreenY(0),y2=mapScreenY(1);
  InterpolationMode previous=gr.GetInterpolationMode();gr.SetInterpolationMode(InterpolationModeNearestNeighbor);
  ColorMatrix matrix=fillOpacityMatrix();ImageAttributes attributes;attributes.SetColorMatrix(&matrix,ColorMatrixFlagsDefault,ColorAdjustTypeBitmap);
  gr.DrawImage(gRouteRaster,Rect(x1,y1,x2-x1,y2-y1),0,0,FINE_GRID_COLUMNS,FINE_GRID_ROWS,UnitPixel,&attributes);gr.SetInterpolationMode(previous);
  if(gFillOpacity>0){
    GraphicsState saved=gr.Save();gr.SetSmoothingMode(SmoothingModeAntiAlias);BYTE alpha=(BYTE)std::lround(255.0*gFillOpacity/100.0);
    Pen halo(Color(alpha,65,31,3),uiPxF(3.0f)),line(Color(alpha,255,224,110),uiPxF(1.5f)),markerBorder(Color(alpha,65,31,3),uiPxF(1.0f));SolidBrush marker(Color(alpha,255,242,185));
    for(size_t i=1;i<route.stops.size();i++){
      double t=std::max(0.0,std::min(1.0,gRouteProgress-(double)(i-1)));if(t<=0)break;
      const auto& a=route.stops[i-1].position;const auto& b=route.stops[i].position;if(b.breakBefore)continue;
      double ax=(a.longitude+180.0)/360.0,bx=(b.longitude+180.0)/360.0,dx=bx-ax;if(dx>0.5)dx-=1;if(dx<-.5)dx+=1;
      double ay=(90.0-a.latitude)/180.0,by=(90.0-b.latitude)/180.0;
      for(int wrap=-1;wrap<=1;wrap++){int sx=mapScreenX(ax+wrap),sy=mapScreenY(ay),ex=mapScreenX(ax+dx*t+wrap),ey=mapScreenY(ay+(by-ay)*t);gr.DrawLine(&halo,sx,sy,ex,ey);gr.DrawLine(&line,sx,sy,ex,ey);}
    }
    int markerRadius=uiPx(gMapZoom>=4?3:2);
    for(size_t i=0;i<route.stops.size()&&(double)i<=gRouteProgress;i++){bool last=i==(size_t)std::floor(gRouteProgress);if(!last&&route.stops[i].kind!=routes::WaypointKind::Milestone&&route.stops[i].kind!=routes::WaypointKind::Uncertain)continue;const auto& p=route.stops[i].position;int px=mapScreenX((p.longitude+180.0)/360.0),py=mapScreenY((90.0-p.latitude)/180.0);gr.FillEllipse(&marker,px-markerRadius,py-markerRadius,markerRadius*2,markerRadius*2);gr.DrawEllipse(&markerBorder,px-markerRadius,py-markerRadius,markerRadius*2,markerRadius*2);}
    gr.Restore(saved);
  }
  const RECT content=mapContentRect();int width=std::min(uiPx(390),(int)(content.right-content.left)-uiPx(18));if(width<uiPx(160))return;
  RECT legend=routeLegendPlacement(route,width,uiPx(48));if(legend.right<=legend.left)return;int x=legend.left,y=legend.top;SolidBrush background(Color(225,6,19,31));Pen border(Color(230,255,172,65),1);
  gr.FillRectangle(&background,x,y,width,uiPx(48));gr.DrawRectangle(&border,x,y,width,uiPx(48));
  Font font(L"Microsoft YaHei UI",uiPxF(10),FontStyleBold,UnitPixel),smallFont(L"Microsoft YaHei UI",uiPxF(9),FontStyleRegular,UnitPixel);
  SolidBrush ink(Color(255,255,239,211));std::wstring title=gEnglish?route.en:route.zh;
  size_t stop=std::min((size_t)std::floor(gRouteProgress),route.stops.size()-1);
  int percent=gRouteDistance.length()>0?(int)std::lround(100*gRouteDistance.at(gRouteProgress)/gRouteDistance.length()):100;
  size_t labelStop=stop;while(labelStop>0&&route.stops[labelStop].kind==routes::WaypointKind::Auxiliary)--labelStop;
  std::wstring detail=std::wstring(routes::travelModeLabel(route.mode,gEnglish))+L" · "+std::to_wstring(percent)+L"% · "+(gEnglish?route.stops[labelStop].en:route.stops[labelStop].zh);
  gr.DrawString(title.c_str(),-1,&font,PointF((REAL)x+uiPx(7),(REAL)y+uiPx(5)),&ink);
  gr.DrawString(detail.c_str(),-1,&smallFont,PointF((REAL)x+uiPx(7),(REAL)y+uiPx(25)),&ink);
}
static void updateRouteControls(){syncExperienceControls();
  bool selected=gRouteIndex>=0;
  if(gRouteCombo)SendMessageW(gRouteCombo,CB_SETCURSEL,selected?gRouteIndex+1:0,0);
  if(gRouteButton){EnableWindow(gRouteButton,selected);SetWindowTextW(gRouteButton,gRoutePlaying?(gEnglish?L"Pause":L"暂停路线"):(gRouteProgress>0&&gRouteIndex>=0&&gRouteProgress+1e-6<routes::catalog()[gRouteIndex].stops.size()-1?(gEnglish?L"Continue":L"继续路线"):(gEnglish?L"Play / replay":L"播放 / 重播")));}
  if(gRouteHideButton){EnableWindow(gRouteHideButton,selected);SetWindowTextW(gRouteHideButton,gEnglish?L"Hide":L"关闭路线");}
  if(gRouteInfoButton){EnableWindow(gRouteInfoButton,selected);SetWindowTextW(gRouteInfoButton,gEnglish?L"Notes":L"路线说明");}
  if(gRouteFitButton){EnableWindow(gRouteFitButton,selected);SetWindowTextW(gRouteFitButton,gEnglish?L"Fit route":L"定位路线");}
  updatePickerCaptions();
  // Owner-drawn controls need a fresh paint after both their enabled state and
  // caption change, including when a route is hidden during playback.
  for(HWND button:{gRouteButton,gRouteHideButton,gRouteInfoButton,gRouteFitButton})
    if(button)RedrawWindow(button,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
}
static void populateRouteCombo(){
  if(!gRouteCombo)return;SendMessageW(gRouteCombo,CB_RESETCONTENT,0,0);
  SendMessageW(gRouteCombo,CB_ADDSTRING,0,(LPARAM)(gEnglish?L"Select a campaign route":L"选择行军或远征路线"));
  for(const auto& route:routes::catalog())SendMessageW(gRouteCombo,CB_ADDSTRING,0,(LPARAM)(gEnglish?route.en:route.zh));
  updateRouteControls();
}
static void closeRouteNotesWindow(HWND window){HWND owner=GetWindow(window,GW_OWNER),previous=(HWND)RemovePropW(window,L"WorldMapPreviousFocus");DestroyWindow(window);if(owner&&previous&&IsWindow(previous)&&IsChild(owner,previous)&&IsWindowVisible(previous)&&IsWindowEnabled(previous))SetFocus(previous);}
static LRESULT CALLBACK routeNotesWindowProc(HWND window,UINT message,WPARAM wp,LPARAM lp){
  UiDpiScope dpi(window,message==WM_DPICHANGED?LOWORD(wp):0);if(message==WM_DPICHANGED||message==UI_SCALE_CHANGED){refreshUiChildFonts(window);applyUiWindowBounds(window,message,wp,lp);SendMessageW(window,WM_SIZE,0,0);return 0;}
  if(message==WM_CREATE){
    auto* creation=reinterpret_cast<CREATESTRUCTW*>(lp);
    auto* content=reinterpret_cast<const std::wstring*>(creation->lpCreateParams);
    HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",content?content->c_str():L"",
      WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
      0,0,0,0,window,(HMENU)1,creation->hInstance,nullptr);
    HWND close=CreateWindowExW(0,L"BUTTON",gEnglish?L"Close":L"关闭",
      WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,0,0,0,0,window,(HMENU)2,creation->hInstance,nullptr);
    if(!edit||!close)return -1;
    SendMessageW(edit,WM_SETFONT,(WPARAM)uiFontFor(14,false),TRUE);
    SendMessageW(close,WM_SETFONT,(WPARAM)uiFontFor(14,false),TRUE);
    return 0;
  }
  if(message==WM_GETMINMAXINFO){uiMinimumWindow(window,(MINMAXINFO*)lp,420,300);return 0;}
  if(message==WM_SIZE){RECT r{};GetClientRect(window,&r);int width=uiDip(r.right),height=uiDip(r.bottom);placeUiControl(GetDlgItem(window,1),10,10,std::max(1,width-20),std::max(1,height-60));placeUiControl(GetDlgItem(window,2),std::max(10,width-100),std::max(10,height-42),90,30);return 0;}
  if(message==WM_COMMAND&&LOWORD(wp)==2){closeRouteNotesWindow(window);return 0;}
  if(message==WM_CLOSE){closeRouteNotesWindow(window);return 0;}
  if(message==WM_NCDESTROY)RemovePropW(window,L"WorldMapPreviousFocus");
  return DefWindowProcW(window,message,wp,lp);
}
static void showRouteNotes(HWND owner,const std::wstring& text,const wchar_t* title=nullptr){
  UiDpiScope dpi(owner);HWND previousFocus=GetFocus();
  HINSTANCE instance=GetModuleHandleW(nullptr);const wchar_t* className=L"WorldMapExplorerRouteNotes";
  WNDCLASSW cls{};cls.hIcon=(HICON)LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(101),IMAGE_ICON,0,0,LR_SHARED|LR_DEFAULTSIZE);cls.lpfnWndProc=routeNotesWindowProc;cls.hInstance=instance;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);cls.lpszClassName=className;
  if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){MessageBoxW(owner,gEnglish?L"Unable to open route notes.":L"无法打开路线说明。",L"World Map Explorer",MB_OK|MB_ICONWARNING);return;}
  MONITORINFO monitor{sizeof(MONITORINFO)};GetMonitorInfoW(MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST),&monitor);
  int availableWidth=monitor.rcWork.right-monitor.rcWork.left,availableHeight=monitor.rcWork.bottom-monitor.rcWork.top;
  int width=std::max(1,std::min(uiPx(780),availableWidth-uiPx(40))),height=std::max(1,std::min(uiPx(650),availableHeight-uiPx(40)));
  int x=monitor.rcWork.left+(availableWidth-width)/2,y=monitor.rcWork.top+(availableHeight-height)/2;
  std::wstring content;content.reserve(text.size()+64);wchar_t previous=0;for(wchar_t c:text){if(c==L'\n'&&previous!=L'\r')content+=L'\r';content+=c;previous=c;}
  HWND notes=CreateWindowExW(WS_EX_DLGMODALFRAME,className,title?title:(gEnglish?L"Route notes":L"路线说明"),WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPCHILDREN,
      x,y,width,height,owner,nullptr,instance,&content);if(!notes)MessageBoxW(owner,gEnglish?L"Unable to open route notes.":L"无法打开路线说明。",L"World Map Explorer",MB_OK|MB_ICONWARNING);else{if(previousFocus&&IsChild(owner,previousFocus))SetPropW(notes,L"WorldMapPreviousFocus",(HANDLE)previousFocus);SetForegroundWindow(notes);HWND body=GetDlgItem(notes,1);if(body)SetFocus(body);}
}
static void handleRouteCommand(HWND w,int cmd){
  const auto& all=routes::catalog();
  if(cmd>=3000&&cmd<3000+(int)all.size()){
    KillTimer(w,ROUTE_PLAYBACK_TIMER);gRoutePlaying=false;gRouteIndex=cmd-3000;gRouteProgress=(double)all[gRouteIndex].stops.size()-1;
    if(!updateRouteRaster()){gRouteIndex=-1;delete gRouteRaster;gRouteRaster=nullptr;MessageBoxW(w,gEnglish?L"Unable to create the route layer.":L"无法创建路线图层。",L"World Map Explorer",MB_OK|MB_ICONWARNING);}
    updateRouteControls();InvalidateRect(w,nullptr,FALSE);return;
  }
  if(gRouteIndex<0)return;
  if(cmd==3100){
    if(gRoutePlaying){KillTimer(w,ROUTE_PLAYBACK_TIMER);gRoutePlaying=false;}
    else{if(gRouteProgress>=all[gRouteIndex].stops.size()-1)gRouteProgress=0;if(!updateRouteRaster())return;gRoutePlaying=SetTimer(w,ROUTE_PLAYBACK_TIMER,100,nullptr)!=0;if(gRoutePlaying)gRouteLastPlaybackTime=routePlaybackTime();}
    updateRouteControls();InvalidateRect(w,&gMapRect,FALSE);
  }else if(cmd==3101){KillTimer(w,ROUTE_PLAYBACK_TIMER);gRoutePlaying=false;gRouteIndex=-1;delete gRouteRaster;gRouteRaster=nullptr;updateRouteControls();InvalidateRect(w,&gMapRect,FALSE);}
  else if(cmd==3102){const auto& route=all[gRouteIndex];std::wstring info=(gEnglish?route.en:route.zh)+std::wstring(L"\n")+route.period+L"\n\n"+(gEnglish?route.noteEn:route.noteZh)+L"\n\n"+(gEnglish?L"Colored cells use the existing 627,200-cell grid and connect selected milestones schematically; they are not a precisely reconstructed path.":L"路线使用原有 627,200 格网格填色，示意连接关键地点，并非逐公里考证的实际路线。")+L"\n\n";
    routes::DistanceProfile profile(routes::pointsFor(route));size_t guides=0,logged=0,gaps=0;for(const auto& stop:route.stops){guides+=stop.kind==routes::WaypointKind::Auxiliary;logged+=stop.kind==routes::WaypointKind::LoggedPosition;gaps+=stop.position.breakBefore;}
    info+=std::wstring(routes::travelModeLabel(route.mode,gEnglish))+L" · "+std::to_wstring(route.stops.size())+(gEnglish?L" reference points / ":L" 个参考点 / ")+std::to_wstring(guides)+(gEnglish?L" guides / ":L" 个辅助点 / ")+std::to_wstring(gaps)+(gEnglish?L" gaps\n":L" 处断开\n");
    info+=(gEnglish?L"Connection length ≈ ":L"参考点连线长度约 ")+std::to_wstring((int)std::lround(profile.length()))+L" km"+(gEnglish?L"; not actual travel mileage. Animation advances by connection distance, not historical duration.\n\n":L"，不等于真实旅程里程。播放按连线距离推进，不表示历史行进速度。\n\n");
    for(size_t i=0;i<route.stops.size();i++){const auto& stop=route.stops[i];if(stop.kind==routes::WaypointKind::Auxiliary)continue;info+=std::to_wstring(i+1)+L". "+(gEnglish?stop.en:stop.zh)+L" · "+routes::waypointKindLabel(stop.kind,gEnglish)+(stop.position.breakBefore?(gEnglish?L" [gap before this point]":L"【此前断开】"):L"")+L"\n";}
    info+=gEnglish?L"\nSources:\n":L"\n资料来源：\n";for(const char* source:route.sources)info+=utf8(source)+L"\n";showRouteNotes(w,info);
  }
}
static void drawWorld2026Legend(Graphics& gr,int mapWidth,int mapHeight){
  UiHudTransform transform(gr);const RECT content=uiDipRect(mapContentRect());mapWidth=content.right-content.left;mapHeight=content.bottom-content.top;
  gWorld2026LegendRect=RECT{0,0,0,0};if(!gShowWorld2026||!gWorld2026Loaded||!gWorld2026Raster)return;
  if(mapWidth<300||mapHeight<125){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));gr.DrawString(localizeUiText(L"世界2026：国家与地区").c_str(),-1,&hintFont,PointF((REAL)content.left+10,(REAL)content.bottom-40),&hintBrush);return;}
  int width=254,height=49,left=content.right-width-9,top=content.bottom-height-9;if(gShowHistory&&gHistoryLoaded&&gHistoryRaster&&mapWidth<width+274+27)top-=54;gWorld2026LegendRect=uiRect(left,top,left+width,top+height);SolidBrush panel(Color(238,6,19,31));Pen border(Color(220,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);
  Font titleFont(L"Microsoft YaHei UI",9,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,240,246,249));gr.DrawString(localizeUiText(L"世界 2026 · 国家／地区分色（无数值含义）").c_str(),-1,&titleFont,PointF((REAL)left+7,(REAL)top+3),&titleBrush);
  static const unsigned int palette[7]={0x55a8b5,0xf1c75b,0x927bb8,0xe97870,0x70a7ce,0xe5a261,0x76b573};
  for(int i=0;i<7;i++){BYTE r=(BYTE)((palette[i]>>16)&255),g=(BYTE)((palette[i]>>8)&255),b=(BYTE)(palette[i]&255);SolidBrush swatch(Color(245,r,g,b));gr.FillRectangle(&swatch,left+8+i*33,top+25,25,13);}
}
static void drawHistoryLegend(Graphics& gr,int mapWidth,int mapHeight){
  UiHudTransform transform(gr);const RECT content=uiDipRect(mapContentRect());mapWidth=content.right-content.left;mapHeight=content.bottom-content.top;
  gHistoryLegendRect=RECT{0,0,0,0};if(!gShowHistory||!gHistoryLoaded||!gHistoryRaster)return;if(mapWidth<300||mapHeight<125){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));std::wstring hint=localizeUiText(historyYearText(gHistoryYear)+L"：历史疆域");gr.DrawString(hint.c_str(),-1,&hintFont,PointF((REAL)content.left+10,(REAL)content.bottom-22),&hintBrush);return;}
  int width=310,height=45,left=content.left+9,top=content.bottom-height-9;if(gShowWorld2026&&gWorld2026Loaded&&gWorld2026Raster&&mapWidth<width+254+27)top-=54;gHistoryLegendRect=uiRect(left,top,left+width,top+height);SolidBrush panel(Color(238,6,19,31));Pen border(Color(220,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);Font titleFont(L"Microsoft YaHei UI",9,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,224,237,241));std::wstring title=localizeUiText(L"世界 "+historyYearText(gHistoryYear)+L" · 历史疆域 · 来源矢量");gr.DrawString(title.c_str(),-1,&titleFont,PointF((REAL)left+7,(REAL)top+4),&titleBrush);std::wstring note=localizeUiText(gHistoryAnimationEnabled?L"试行模式开启；中间帧不代表真实疆界":L"仅填有来源记录的区域；空白表示未收录");gr.DrawString(note.c_str(),-1,&labelFont,PointF((REAL)left+7,(REAL)top+24),&labelBrush);
}
static std::wstring formatPopulationDensity(double value){std::wostringstream out;if(value>=10000)out<<std::fixed<<std::setprecision(0)<<value/1000.0<<L"k";else if(value>=100)out<<std::fixed<<std::setprecision(0)<<value;else if(value>=10)out<<std::fixed<<std::setprecision(1)<<value;else out<<std::fixed<<std::setprecision(2)<<value;return out.str();}
static void drawPopulationLegend(Graphics& gr,int mapWidth,int mapHeight){
  UiHudTransform transform(gr);const RECT content=uiDipRect(mapContentRect());mapWidth=content.right-content.left;mapHeight=content.bottom-content.top;
  gPopulationLegendRect=RECT{0,0,0,0};if(!gShowPopulation||!gPopulationLoaded||!gPopulationRaster)return;
  if(mapWidth<310||mapHeight<160){Font hintFont(L"Microsoft YaHei UI",9,FontStyleRegular,UnitPixel);SolidBrush hintBrush(Color(255,238,246,249));gr.DrawString(localizeUiText(L"人口密度：蓝→黄→红（对数色阶）").c_str(),-1,&hintFont,PointF((REAL)content.left+10,(REAL)content.top+76),&hintBrush);return;}
  int left=content.left+9,top=content.bottom-78,width=295,height=68;if(gShowHistory&&gHistoryLoaded&&gHistoryRaster&&mapHeight>=200)top-=54;gPopulationLegendRect=uiRect(left,top,left+width,top+height);SolidBrush panel(Color(238,6,19,31));Pen border(Color(220,105,156,174),1);gr.FillRectangle(&panel,left,top,width,height);gr.DrawRectangle(&border,left,top,width,height);
  Font titleFont(L"Microsoft YaHei UI",9,FontStyleBold,UnitPixel),labelFont(L"Microsoft YaHei UI",8,FontStyleRegular,UnitPixel);SolidBrush titleBrush(Color(255,240,249,251)),labelBrush(Color(255,240,246,249));std::wstring title=localizeUiText(L"WorldPop "+std::to_wstring(POPULATION_YEAR)+L" 人口密度估算 · 人/km²陆地");gr.DrawString(title.c_str(),-1,&titleFont,PointF((REAL)left+8,(REAL)top+4),&titleBrush);
  double logMax=std::log1p(std::max(1.0,gPopulationMaxDensity));for(int i=0;i<5;i++){double t=i/4.0,value=std::expm1(logMax*t);unsigned int rgb=populationColor(t);BYTE red=(BYTE)((rgb>>16)&255),green=(BYTE)((rgb>>8)&255),blue=(BYTE)(rgb&255);SolidBrush swatch(Color(220,red,green,blue));int x=left+9+i*55;gr.FillRectangle(&swatch,x,top+25,46,13);std::wstring label=i==0?L"0":formatPopulationDensity(value);gr.DrawString(label.c_str(),-1,&labelFont,PointF((REAL)x,(REAL)top+42),&labelBrush);}
}
static void drawGridOverlay(Graphics& gr){
  const int scale=activeGridScale(),columns=activeGridColumns(),rows=activeGridRows();
  const double spacingX=mapProjectionWidth()*gMapZoom/columns,spacingY=mapProjectionHeight()*gMapZoom/rows;
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
static bool mapBaseOverviewCanUseCache(const MapSceneCacheKey& current){const MapBaseCacheKey& base=gMapBaseOverviewCacheKey;return gMapBaseOverviewCache&&gMapBaseOverviewCacheValid&&current.width==base.width&&current.height==base.height&&current.mapRect.left==base.mapRect.left&&current.mapRect.top==base.mapRect.top&&current.mapRect.right==base.mapRect.right&&current.mapRect.bottom==base.mapRect.bottom&&current.map==base.map&&current.nasaDetail==base.nasaDetail&&current.nasa1km==base.nasa1km&&current.nasa500m==base.nasa500m;}
static void drawMapSceneCache(Graphics& gr,int clientWidth,int clientHeight){
  if(!gMapSceneCache)return;MapSceneCacheKey current=currentMapSceneCacheKey(clientWidth,clientHeight);
  if(!mapViewPreviewCanUseCache(current)){gr.DrawImage(gMapSceneCache,0,0);return;}
  double scale=gMapZoom/std::max(0.0001,gMapSceneCacheKey.zoom),centerX=(gMapRect.left+gMapRect.right)/2.0,centerY=(gMapRect.top+gMapRect.bottom)/2.0;
  double dx=centerX+(gMapSceneCacheKey.centerX-gMapCenterX)*mapProjectionWidth()*gMapZoom-centerX*scale;
  double dy=centerY+(gMapSceneCacheKey.centerY-gMapCenterY)*mapProjectionHeight()*gMapZoom-centerY*scale;
  const RECT& source=gMapSceneCacheKey.mapRect;int sourceWidth=source.right-source.left,sourceHeight=source.bottom-source.top;
  if(sourceWidth<=0||sourceHeight<=0){gr.DrawImage(gMapSceneCache,RectF((REAL)dx,(REAL)dy,(REAL)(clientWidth*scale),(REAL)(clientHeight*scale)));return;}
  const bool integerPan=std::fabs(scale-1.0)<1e-9&&std::fabs(dx-std::round(dx))<1e-6&&std::fabs(dy-std::round(dy))<1e-6;const int sourceInset=!integerPan&&sourceWidth>2&&sourceHeight>2?1:0;double mapLeft=dx+(source.left+sourceInset)*scale,mapTop=dy+(source.top+sourceInset)*scale,mapWidth=(sourceWidth-2*sourceInset)*scale,mapHeight=(sourceHeight-2*sourceInset)*scale,mapRight=mapLeft+mapWidth,mapBottom=mapTop+mapHeight;
  gr.SetInterpolationMode(std::fabs(scale-1.0)<1e-9?InterpolationModeNearestNeighbor:InterpolationModeBilinear);SolidBrush ocean(Color(255,7,20,33));gr.FillRectangle(&ocean,gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top);
  if((mapLeft>gMapRect.left||mapTop>gMapRect.top||mapRight<gMapRect.right||mapBottom<gMapRect.bottom)&&mapBaseOverviewCanUseCache(current)){
    const MapBaseCacheKey& overview=gMapBaseOverviewCacheKey;double overviewScale=gMapZoom/std::max(0.0001,overview.zoom),overviewDx=centerX+(overview.centerX-gMapCenterX)*mapProjectionWidth()*gMapZoom-centerX*overviewScale,overviewDy=centerY+(overview.centerY-gMapCenterY)*mapProjectionHeight()*gMapZoom-centerY*overviewScale;
    double overviewLeft=overviewDx+overview.mapRect.left*overviewScale,overviewTop=overviewDy+overview.mapRect.top*overviewScale,overviewWidth=(overview.mapRect.right-overview.mapRect.left)*overviewScale,overviewHeight=(overview.mapRect.bottom-overview.mapRect.top)*overviewScale;
    auto drawOverviewGap=[&](int left,int top,int right,int bottom){if(right<=left||bottom<=top)return;double drawLeft=std::max((double)left,overviewLeft),drawTop=std::max((double)top,overviewTop),drawRight=std::min((double)right,overviewLeft+overviewWidth),drawBottom=std::min((double)bottom,overviewTop+overviewHeight);if(drawRight<=drawLeft||drawBottom<=drawTop)return;double srcLeft=overview.mapRect.left+(drawLeft-overviewLeft)/overviewScale,srcTop=overview.mapRect.top+(drawTop-overviewTop)/overviewScale,srcWidth=(drawRight-drawLeft)/overviewScale,srcHeight=(drawBottom-drawTop)/overviewScale;gr.DrawImage(gMapBaseOverviewCache,RectF((REAL)drawLeft,(REAL)drawTop,(REAL)(drawRight-drawLeft),(REAL)(drawBottom-drawTop)),(REAL)srcLeft,(REAL)srcTop,(REAL)srcWidth,(REAL)srcHeight,UnitPixel);};
    int mapClipLeft=std::max((int)gMapRect.left,(int)std::floor(mapLeft)),mapClipRight=std::min((int)gMapRect.right,(int)std::ceil(mapRight));
    if(mapLeft>gMapRect.left)drawOverviewGap(gMapRect.left,gMapRect.top,std::min((int)gMapRect.right,(int)std::ceil(mapLeft)),gMapRect.bottom);
    if(mapRight<gMapRect.right)drawOverviewGap(std::max((int)gMapRect.left,(int)std::floor(mapRight)),gMapRect.top,gMapRect.right,gMapRect.bottom);
    if(mapTop>gMapRect.top)drawOverviewGap(mapClipLeft,gMapRect.top,mapClipRight,std::min((int)gMapRect.bottom,(int)std::ceil(mapTop)));
    if(mapBottom<gMapRect.bottom)drawOverviewGap(mapClipLeft,std::max((int)gMapRect.top,(int)std::floor(mapBottom)),mapClipRight,gMapRect.bottom);
  }
  if(integerPan){GraphicsState saved=gr.Save();gr.SetClip(Rect((int)mapLeft,(int)mapTop,(int)mapWidth,(int)mapHeight),CombineModeIntersect);gr.DrawImage(gMapSceneCache,(REAL)std::round(dx),(REAL)std::round(dy));gr.Restore(saved);}else gr.DrawImage(gMapSceneCache,RectF((REAL)mapLeft,(REAL)mapTop,(REAL)mapWidth,(REAL)mapHeight),(REAL)(source.left+sourceInset),(REAL)(source.top+sourceInset),(REAL)(sourceWidth-2*sourceInset),(REAL)(sourceHeight-2*sourceInset),UnitPixel);
}
static bool sameMapBaseCacheKey(const MapBaseCacheKey& a,const MapBaseCacheKey& b){return a.width==b.width&&a.height==b.height&&a.mapRect.left==b.mapRect.left&&a.mapRect.top==b.mapRect.top&&a.mapRect.right==b.mapRect.right&&a.mapRect.bottom==b.mapRect.bottom&&a.map==b.map&&a.zoom==b.zoom&&a.centerX==b.centerX&&a.centerY==b.centerY&&a.nasaDetail==b.nasaDetail&&a.nasa1km==b.nasa1km&&a.nasa500m==b.nasa500m;}
static bool renderMapSceneCache(int clientWidth,int clientHeight){
  if(clientWidth<=0||clientHeight<=0||gMapRect.right<=gMapRect.left||gMapRect.bottom<=gMapRect.top)return false;
  MapSceneCacheKey key=currentMapSceneCacheKey(clientWidth,clientHeight);
  if(gMapSceneCache&&gMapSceneCacheValid&&sameMapSceneCacheKey(key,gMapSceneCacheKey))return true;
  if(mapViewPreviewCanUseCache(key))return true;
  delete gMapSceneCache;gMapSceneCache=nullptr;gMapSceneCacheValid=false;
  int mw=gMapRect.right-gMapRect.left,mh=gMapRect.bottom-gMapRect.top;
  MapBaseCacheKey baseKey{clientWidth,clientHeight,gMapRect,gMap,gMapZoom,gMapCenterX,gMapCenterY,gNasaDetailAvailable,gNasa1kmAvailable,gNasa500mAvailable};
  if(!gMapBaseCache||!gMapBaseCacheValid||!sameMapBaseCacheKey(baseKey,gMapBaseCacheKey)){
    delete gMapBaseCache;gMapBaseCache=nullptr;gMapBaseCacheValid=false;Bitmap* base=new Bitmap(clientWidth,clientHeight,PixelFormat32bppARGB);if(!base||base->GetLastStatus()!=Ok){delete base;return false;}Graphics baseGraphics(base);if(baseGraphics.GetLastStatus()!=Ok){delete base;return false;}baseGraphics.Clear(Color(255,2,8,12));baseGraphics.SetClip(Rect(gMapRect.left,gMapRect.top,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top));int imageLeft=mapScreenX(0),imageTop=mapScreenY(0),imageRight=mapScreenX(1),imageBottom=mapScreenY(1);if(gMap)baseGraphics.DrawImage(mapImageForSize(imageRight-imageLeft,imageBottom-imageTop),imageLeft,imageTop,imageRight-imageLeft,imageBottom-imageTop);drawNasaDetailTiles(baseGraphics);gMapBaseCache=base;gMapBaseCacheKey=baseKey;gMapBaseCacheValid=true;
  }
  if(std::fabs(baseKey.zoom-1.0)<1e-9&&std::fabs(baseKey.centerX-.5)<1e-9&&std::fabs(baseKey.centerY-.5)<1e-9&&(!gMapBaseOverviewCacheValid||!sameMapBaseCacheKey(baseKey,gMapBaseOverviewCacheKey))){Bitmap* overview=gMapBaseCache->Clone(0,0,(INT)gMapBaseCache->GetWidth(),(INT)gMapBaseCache->GetHeight(),PixelFormat32bppARGB);if(overview&&overview->GetLastStatus()==Ok){delete gMapBaseOverviewCache;gMapBaseOverviewCache=overview;gMapBaseOverviewCacheKey=baseKey;gMapBaseOverviewCacheValid=true;}else delete overview;}
  Bitmap* candidate=new Bitmap(clientWidth,clientHeight,PixelFormat32bppARGB);if(!candidate||candidate->GetLastStatus()!=Ok){delete candidate;return false;}
  Graphics gr(candidate);if(gr.GetLastStatus()!=Ok){delete candidate;return false;}
  gr.Clear(Color(255,2,8,12));gr.DrawImage(gMapBaseCache,0,0);RECT content=mapContentRect();gr.SetClip(Rect(content.left,content.top,content.right-content.left,content.bottom-content.top));
  drawLandCoverOverlay(gr);drawPopulationOverlay(gr);drawWorld2026Overlay(gr);drawHistoryOverlay(gr);
  if(gShowGridLines)drawGridOverlay(gr);drawSelectedGridCell(gr);
  if(gShowControlPoints)drawCalibrationPoints(gr);drawSelectedPoint(gr);
  gMapSceneCache=candidate;gMapSceneCacheKey=key;gMapSceneCacheValid=true;return true;
}
static RECT mapScalePlacement(){
  const RECT content=mapContentRect();
  int right=content.right-uiPx(222),left=content.left+uiPx(9),top=content.top+uiPx(9),bottom=content.bottom-uiPx(56);
  RECT options[]={{right,bottom,right+uiPx(212),bottom+uiPx(46)},{right,top+uiPx(40),right+uiPx(212),top+uiPx(86)},{left,top+uiPx(40),left+uiPx(212),top+uiPx(86)},{left,bottom,left+uiPx(212),bottom+uiPx(46)}};
  for(const RECT& area:options){
    if(area.left<content.left||area.right>content.right||area.top<content.top||area.bottom>content.bottom)continue;
    RECT overlap{};bool blocked=(!gMapFullscreen&&IntersectRect(&overlap,&area,&gMapNavigationRect))
      ||(gShowLandCover&&IntersectRect(&overlap,&area,&gLandCoverLegendRect))
      ||(gShowPopulation&&IntersectRect(&overlap,&area,&gPopulationLegendRect))
      ||(gShowWorld2026&&IntersectRect(&overlap,&area,&gWorld2026LegendRect))
      ||(gShowHistory&&IntersectRect(&overlap,&area,&gHistoryLegendRect));
    if(!blocked)return area;
  }
  return RECT{};
}
static void drawMapScale(Graphics& gr){
  gMapScaleRect=RECT{};
  int width=gMapRect.right-gMapRect.left,height=gMapRect.bottom-gMapRect.top;
  if(!gShowScaleBar||width<uiPx(280)||height<uiPx(140))return;
  RECT area=mapScalePlacement();if(area.right<=area.left)return;
  int x=area.left,y=area.top;
  double latitude=90.0-180.0*mapWorldY(y+uiPx(34));
  MapScale scale=mapScaleForView((int)std::lround(mapProjectionWidth()),gMapZoom,latitude,uiPx(150));
  if(!scale.pixels)return;
  gMapScaleRect=area;
  SolidBrush panel(Color(222,8,27,40)),ink(Color(255,231,246,249));Pen border(Color(180,77,140,157),uiPxF(1)),bar(Color(255,217,242,245),uiPxF(2));
  gr.FillRectangle(&panel,x,y,uiPx(212),uiPx(46));gr.DrawRectangle(&border,x,y,uiPx(212),uiPx(46));
  std::wostringstream label;
  if(scale.kilometers<1)label<<std::fixed<<std::setprecision(0)<<scale.kilometers*1000<<L" m";
  else label<<std::fixed<<std::setprecision(scale.kilometers<10?1:0)<<scale.kilometers<<L" km";
  label<<(gEnglish?L" · near ":L" · 约 ")<<std::fixed<<std::setprecision(1)<<std::fabs(latitude)<<L"°"<<(latitude<0?L"S":L"N");
  Font font(L"Microsoft YaHei UI",uiPxF(11),FontStyleRegular,UnitPixel);gr.DrawString(label.str().c_str(),-1,&font,PointF((REAL)x+uiPx(9),(REAL)y+uiPx(6)),&ink);
  int lineX=x+uiPx(10),lineY=y+uiPx(34);gr.DrawLine(&bar,lineX,lineY,lineX+scale.pixels,lineY);gr.DrawLine(&bar,lineX,lineY-uiPx(4),lineX,lineY+uiPx(4));gr.DrawLine(&bar,lineX+scale.pixels,lineY-uiPx(4),lineX+scale.pixels,lineY+uiPx(4));
}
static void drawMapSurface(HDC h,int clientWidth,int clientHeight,int mapWidth,int mapHeight,bool fullscreen){
  const RECT content=mapContentRect();HBRUSH backdrop=CreateSolidBrush(RGB(2,8,12));FillRect(h,&gMapRect,backdrop);DeleteObject(backdrop);
  if(renderMapSceneCache(clientWidth,clientHeight)){
    Graphics gr(h);gr.SetClip(Rect(content.left,content.top,content.right-content.left,content.bottom-content.top),CombineModeIntersect);drawMapSceneCache(gr,clientWidth,clientHeight);drawLandCoverLegend(gr,mapWidth,mapHeight);drawPopulationLegend(gr,mapWidth,mapHeight);drawWorld2026Legend(gr,mapWidth,mapHeight);drawHistoryLegend(gr,mapWidth,mapHeight);drawMapScale(gr);drawRouteOverlay(gr);return;
  }
  HBRUSH black=CreateSolidBrush(RGB(2,8,12));FillRect(h,&gMapRect,black);DeleteObject(black);
  Graphics gr(h);gr.SetClip(Rect(content.left,content.top,content.right-content.left,content.bottom-content.top),CombineModeIntersect);
  int left=mapScreenX(0),top=mapScreenY(0),right=mapScreenX(1),bottom=mapScreenY(1);if(gMap)gr.DrawImage(mapImageForSize(right-left,bottom-top),left,top,right-left,bottom-top);
  drawNasaDetailTiles(gr);drawLandCoverOverlay(gr);drawPopulationOverlay(gr);drawWorld2026Overlay(gr);drawHistoryOverlay(gr);
  if(gShowGridLines)drawGridOverlay(gr);drawSelectedGridCell(gr);
  if(!fullscreen&&gShowControlPoints)drawCalibrationPoints(gr);drawSelectedPoint(gr);
  if(!fullscreen){drawLandCoverLegend(gr,mapWidth,mapHeight);drawPopulationLegend(gr,mapWidth,mapHeight);drawWorld2026Legend(gr,mapWidth,mapHeight);drawHistoryLegend(gr,mapWidth,mapHeight);}drawMapScale(gr);drawRouteOverlay(gr);
}
static std::wstring exportMapNotes(){
  std::wstring notes=gEnglish?L"Layers: ":L"显示：";bool hasLayer=false;
  auto add=[&](const std::wstring& value){if(hasLayer)notes+=L" · ";notes+=value;hasLayer=true;};
  if(gShowHistory&&gHistoryLoaded&&gHistoryRaster)add((gEnglish?L"Historical boundaries ":L"历史疆域 ")+historyYearText(gHistoryYear)+(gHistoryTransitionActive?(gEnglish?L" (transition preview)":L"（过渡示意）"):L""));
  if(gShowWorld2026&&gWorld2026Loaded)add(gEnglish?L"Countries / regions (2026 snapshot)":L"国家与地区（2026 快照）");
  if(gShowPopulation&&gPopulationLoaded)add(gEnglish?L"Population estimate (2023)":L"人口估算（2023）");
  if(gShowLandCover&&gLandCoverLoaded)add(gEnglish?L"Land cover (2020)":L"土地覆盖（2020）");
  if(gRouteIndex>=0&&(size_t)gRouteIndex<routes::catalog().size()){const auto& route=routes::catalog()[gRouteIndex];add(std::wstring(gEnglish?L"Route: ":L"路线：")+(gEnglish?route.en:route.zh)+(gEnglish?L" (schematic)":L"（示意）"));}
  if(!hasLayer)notes+=gEnglish?L"Basemap only":L"底图";
  if(gShowHistory||gShowWorld2026||gRouteIndex>=0)notes+=(gEnglish?L" · Fill opacity ":L" · 填色不透明度 ")+std::to_wstring(gFillOpacity)+L"%";
  notes+=gEnglish?(gShowGridLines?L" · Grid lines on":L" · Grid lines off"):(gShowGridLines?L" · 显示网格线":L" · 隐藏网格线");
  if(gShowLandCover&&gLandCoverLoaded&&gLandCoverLegend.size()==22){notes+=gEnglish?L"\nLand-cover legend: ":L"\n土地覆盖图例：";bool first=true;for(const auto& category:gLandCoverLegend){if(!first)notes+=L"; ";notes+=localizeUiText(std::to_wstring(category.code)+L" "+category.name);first=false;}}
  notes+=gEnglish?L"\nSources: ":L"\n数据来源：";
  notes+=(gMap==gMapNasa)?L"NASA Blue Marble (2004-01)":L"Natural Earth";
  if(gShowWorld2026&&gWorld2026Loaded)notes+=L"; Natural Earth Admin 0 (2026 snapshot, grid-derived)";
  if(gShowHistory&&gHistoryLoaded&&gHistoryRaster)notes+=L"; Seshat / Cliopatria, Bennett et al. (2025), source vectors, CC BY 4.0, doi:10.1038/s41597-025-04516-9";
  if(gShowPopulation&&gPopulationLoaded)notes+=L"; WorldPop 2023 Global2 R2025A (grid-aggregated), CC BY 4.0, doi:10.5258/SOTON/WP00845; NOAA ETOPO 2022 land area";
  if(gShowLandCover&&gLandCoverLoaded)notes+=L"; ESA CCI / Copernicus C3S 2020 (grid-aggregated preview), doi:10.24381/cds.006f2c9a";
  if((gShowHistory&&gHistoryLoaded&&gHistoryRaster)||(gShowPopulation&&gPopulationLoaded))notes+=L"; https://creativecommons.org/licenses/by/4.0/";
  if(gRouteIndex>=0&&(size_t)gRouteIndex<routes::catalog().size()){notes+=gEnglish?L"\nRoute references: ":L"\n路线参考资料：";bool first=true;std::vector<std::wstring> hosts;for(const char* source:routes::catalog()[gRouteIndex].sources){std::wstring host=utf8(source);size_t scheme=host.find(L"://");if(scheme!=std::wstring::npos)host.erase(0,scheme+3);host=host.substr(0,host.find(L'/'));if(std::find(hosts.begin(),hosts.end(),host)==hosts.end())hosts.push_back(host);}for(const auto& host:hosts){if(!first)notes+=L"; ";notes+=host;first=false;}notes+=gEnglish?L" (full links in route notes / route reading list)":L"（完整链接见路线说明及路线资料清单）";}
  notes+=gEnglish?L"\nGrid is not equal-area. Historical coverage may be incomplete; routes are schematic. See DATA_SOURCES.md for details.":L"\n经纬网格并非等面积；历史资料可能不完整，路线为示意。详见 DATA_SOURCES.md。";
  if(gShowScaleBar)notes+=gEnglish?L"\nScale bar: approximate WGS 84 distance at the labeled latitude.":L"\n比例尺：按标注纬度附近的 WGS 84 椭球面距离估算。";
  return notes;
}
static bool exportMapPng(HWND window,const std::wstring& path){
  RECT client{};GetClientRect(window,&client);const RECT crop=mapContentRect();const int mapWidth=crop.right-crop.left,mapHeight=crop.bottom-crop.top;
  if(client.right<=0||client.bottom<=0||mapWidth<=0||mapHeight<=0)return false;
  std::wstring rawNotes=exportMapNotes();std::vector<std::wstring> paragraphs;size_t begin=0;for(size_t i=0;i<=rawNotes.size();++i)if(i==rawNotes.size()||rawNotes[i]==L'\n'){paragraphs.push_back(rawNotes.substr(begin,i-begin));begin=i+1;}HDC screen=GetDC(window);if(!screen)return false;
  HFONT font=uiFontFor(13,false);HGDIOBJ old=SelectObject(screen,font);std::vector<int> heights;int textHeight=0;for(const auto& paragraph:paragraphs){RECT measure{0,0,std::max(1,mapWidth-uiPx(32)),0};DrawTextW(screen,paragraph.c_str(),-1,&measure,DT_WORDBREAK|DT_NOPREFIX|DT_CALCRECT|DT_EDITCONTROL);int height=std::max(uiPx(16),(int)(measure.bottom-measure.top));heights.push_back(height);textHeight+=height+uiPx(5);}SelectObject(screen,old);ReleaseDC(window,screen);
  int footerHeight=std::max(uiPx(48),textHeight+uiPx(24)),headerHeight=uiPx(50);
  Bitmap scene(client.right,client.bottom,PixelFormat32bppARGB);if(scene.GetLastStatus()!=Ok)return false;
  gMapViewPreviewActive=false;gMapSceneCacheValid=false;
  {Graphics sceneGraphics(&scene);if(sceneGraphics.GetLastStatus()!=Ok)return false;HDC dc=sceneGraphics.GetHDC();if(!dc)return false;drawMapSurface(dc,client.right,client.bottom,mapWidth,mapHeight,false);sceneGraphics.ReleaseHDC(dc);}
  Bitmap output(mapWidth,headerHeight+mapHeight+footerHeight,PixelFormat32bppARGB);if(output.GetLastStatus()!=Ok)return false;
  {Graphics graphics(&output);if(graphics.GetLastStatus()!=Ok)return false;graphics.Clear(Color(255,10,29,43));graphics.DrawImage(&scene,Rect(0,headerHeight,mapWidth,mapHeight),crop.left,crop.top,mapWidth,mapHeight,UnitPixel);
   HDC dc=graphics.GetHDC();if(!dc)return false;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(237,247,248));HFONT title=uiFontFor(18,true);HGDIOBJ previous=SelectObject(dc,title);RECT titleRect{uiPx(16),uiPx(12),mapWidth-uiPx(16),uiPx(40)};const wchar_t* heading=gEnglish?L"World Map Explorer · Map snapshot":L"世界地图探索器 · 地图快照";DrawTextW(dc,heading,-1,&titleRect,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,font);SetTextColor(dc,RGB(202,224,228));int y=headerHeight+mapHeight+uiPx(12);for(size_t i=0;i<paragraphs.size();++i){RECT noteRect{uiPx(16),y,mapWidth-uiPx(16),y+heights[i]};DrawTextW(dc,paragraphs[i].c_str(),-1,&noteRect,DT_WORDBREAK|DT_NOPREFIX|DT_EDITCONTROL);y+=heights[i]+uiPx(5);}SelectObject(dc,previous);graphics.ReleaseHDC(dc);}
  const CLSID pngEncoder={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
  return output.Save(path.c_str(),&pngEncoder,nullptr)==Ok;
}
static void chooseMapExport(HWND window){
  wchar_t path[32768]=L"WorldMapExplorer.png";OPENFILENAMEW save{};save.lStructSize=sizeof(save);save.hwndOwner=window;save.lpstrFilter=gEnglish?L"PNG image (*.png)\0*.png\0\0":L"PNG 图片 (*.png)\0*.png\0\0";save.lpstrFile=path;save.nMaxFile=(DWORD)(sizeof(path)/sizeof(path[0]));save.lpstrDefExt=L"png";save.lpstrTitle=gEnglish?L"Export map image":L"导出地图图片";save.Flags=OFN_EXPLORER|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  if(!GetSaveFileNameW(&save)){if(CommDlgExtendedError())MessageBoxW(window,gEnglish?L"Could not open the save dialog.":L"无法打开保存窗口。",gEnglish?L"Export failed":L"导出失败",MB_OK|MB_ICONERROR);return;}
  const wchar_t* name=wcsrchr(path,L'\\');name=name?name+1:path;const wchar_t* extension=wcsrchr(name,L'.');if(!extension||_wcsicmp(extension,L".png")!=0){MessageBoxW(window,gEnglish?L"Please choose a filename ending in .png.":L"请使用以 .png 结尾的文件名。",gEnglish?L"Export failed":L"导出失败",MB_OK|MB_ICONERROR);return;}
  if(exportMapPng(window,path)){gNotice=gEnglish?L"Map image exported.":L"地图图片已导出。";InvalidateRect(window,nullptr,FALSE);}
  else MessageBoxW(window,gEnglish?L"Unable to save the PNG image. Check the destination and available space.":L"无法保存 PNG 图片，请检查保存位置及可用空间。",gEnglish?L"Export failed":L"导出失败",MB_OK|MB_ICONERROR);
}
static void ensureMapViewPreviewClock(){if(gMapViewPreviewFrameFrequency.QuadPart<=0)QueryPerformanceFrequency(&gMapViewPreviewFrameFrequency);}
static void resetMapViewPreviewCadence(){gMapViewPreviewLastFrameCounter=0;gMapViewPreviewFrameIntervalMs=MAP_VIEW_PREVIEW_MIN_FRAME_MS;}
static void cancelMapViewPreviewTimer(HWND hwnd){if(gMapViewPreviewTimerScheduled&&gMapViewPreviewTimerWaitable&&gMapViewPreviewWaitableTimer)CancelWaitableTimer(gMapViewPreviewWaitableTimer);KillTimer(hwnd,MAP_VIEW_FRAME_TIMER);gMapViewPreviewTimerScheduled=false;gMapViewPreviewTimerWaitable=false;}
static void scheduleMapViewPreviewTimer(HWND hwnd,double delayMs){if(gMapViewPreviewTimerScheduled)return;delayMs=std::max(.1,delayMs);if(gMapViewPreviewWaitableTimer){LARGE_INTEGER due{};due.QuadPart=-(LONGLONG)std::ceil(delayMs*10000.0);if(SetWaitableTimer(gMapViewPreviewWaitableTimer,&due,0,nullptr,nullptr,FALSE)){gMapViewPreviewTimerScheduled=true;gMapViewPreviewTimerWaitable=true;return;}}UINT fallbackDelay=(UINT)std::max(1.0,std::ceil(delayMs));gMapViewPreviewTimerScheduled=SetTimer(hwnd,MAP_VIEW_FRAME_TIMER,fallbackDelay,nullptr)!=0;gMapViewPreviewTimerWaitable=false;}
// Prefer a precise process-local timer; older systems fall back without changing global timer resolution.
static HANDLE createMapViewPreviewWaitableTimer(){typedef HANDLE(WINAPI* CreateWaitableTimerExProc)(LPSECURITY_ATTRIBUTES,LPCWSTR,DWORD,DWORD);HMODULE kernel=GetModuleHandleW(L"kernel32.dll");auto create=(CreateWaitableTimerExProc)(kernel?GetProcAddress(kernel,"CreateWaitableTimerExW"):nullptr);if(!create)return nullptr;HANDLE timer=create(nullptr,nullptr,MAP_CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_MODIFY_STATE|SYNCHRONIZE);if(!timer)timer=create(nullptr,nullptr,0,TIMER_MODIFY_STATE|SYNCHRONIZE);return timer;}
static std::wstring mapViewPreviewRateText(MapPreviewRateMode mode){switch(mode){case MAP_PREVIEW_RATE_30:return localizeUiText(L"预览帧率 30 帧");case MAP_PREVIEW_RATE_60:return localizeUiText(L"预览帧率 60 帧");default:return localizeUiText(L"预览帧率 自适应");}}
static std::wstring mapViewPreviewRateText(){return mapViewPreviewRateText(gMapViewPreviewRateMode);}
static double mapViewPreviewFrameInterval(){if(gMapViewPreviewRateMode==MAP_PREVIEW_RATE_30)return 1000.0/30.0;if(gMapViewPreviewRateMode==MAP_PREVIEW_RATE_60)return 1000.0/60.0;return gMapViewPreviewFrameIntervalMs;}
static bool mapViewInputPending(HWND hwnd){MSG pending{};return PeekMessageW(&pending,hwnd,WM_MOUSEFIRST,WM_MOUSELAST,PM_NOREMOVE)!=FALSE;}
static void paintMapViewPreviewFrame(HWND hwnd){
  if(!gMapViewPreviewActive)return;ensureMapViewPreviewClock();if(gMapViewPreviewFrameFrequency.QuadPart<=0)return;cancelMapViewPreviewTimer(hwnd);if(mapViewInputPending(hwnd)){scheduleMapViewPreviewTimer(hwnd,1);return;}
  LARGE_INTEGER start{},finish{};QueryPerformanceCounter(&start);gMapViewPreviewLastFrameCounter=start.QuadPart;InvalidateRect(hwnd,&gMapRect,FALSE);UpdateWindow(hwnd);QueryPerformanceCounter(&finish);
  if(gMapViewPreviewRateMode==MAP_PREVIEW_RATE_ADAPTIVE){double elapsed=(finish.QuadPart-start.QuadPart)*1000.0/gMapViewPreviewFrameFrequency.QuadPart;double cost=std::max(MAP_VIEW_PREVIEW_MIN_FRAME_MS,std::min(MAP_VIEW_PREVIEW_MAX_FRAME_MS,elapsed));gMapViewPreviewFrameIntervalMs=std::max(MAP_VIEW_PREVIEW_MIN_FRAME_MS,std::min(MAP_VIEW_PREVIEW_MAX_FRAME_MS,(gMapViewPreviewFrameIntervalMs*3+cost)/4.0));}
}
static void requestMapViewPreviewFrame(HWND hwnd){
  if(!gMapViewPreviewActive)return;ensureMapViewPreviewClock();if(gMapViewPreviewFrameFrequency.QuadPart<=0)return;LARGE_INTEGER now{};QueryPerformanceCounter(&now);double elapsed=gMapViewPreviewLastFrameCounter>0?(now.QuadPart-gMapViewPreviewLastFrameCounter)*1000.0/gMapViewPreviewFrameFrequency.QuadPart:mapViewPreviewFrameInterval(),interval=mapViewPreviewFrameInterval();
  if(elapsed<interval){scheduleMapViewPreviewTimer(hwnd,std::max(.1,interval-elapsed));return;}if(mapViewInputPending(hwnd)){scheduleMapViewPreviewTimer(hwnd,1.0);return;}paintMapViewPreviewFrame(hwnd);
}
static UINT scheduleMapViewSettle(HWND hwnd,bool wheelInput){
  UINT delay=MAP_VIEW_PAN_SETTLE_DELAY_MS;
  if(wheelInput){ensureMapViewPreviewClock();LARGE_INTEGER now{};QueryPerformanceCounter(&now);if(gMapViewLastWheelCounter>0&&gMapViewPreviewFrameFrequency.QuadPart>0){double observed=(now.QuadPart-gMapViewLastWheelCounter)*1000.0/gMapViewPreviewFrameFrequency.QuadPart;if(observed>=20.0&&observed<=500.0)gMapViewWheelIntervalMs=gMapViewWheelIntervalMs*.7+observed*.3;}gMapViewLastWheelCounter=now.QuadPart;delay=(UINT)std::lround(gMapViewWheelIntervalMs+18.0);delay=std::max(MAP_VIEW_SETTLE_MIN_DELAY_MS,std::min(MAP_VIEW_SETTLE_MAX_DELAY_MS,delay));}
  SetTimer(hwnd,MAP_VIEW_SETTLE_TIMER,delay,nullptr);return delay;
}
static RECT gInfoScrollTrack{},gInfoScrollThumb{};static bool gInfoScrollDragging=false;static int gInfoScrollGrabY=0;
static void paintInfoScrollbar(HDC h,int top,int bottom){
  if(gContentHeight<=gViewportHeight||gViewportHeight<=0||bottom<=top)return;
  int trackH=bottom-top,thumbH=std::min(trackH,std::max(uiPx(22),(int)((long long)trackH*gViewportHeight/gContentHeight)));
  int thumbTop=top+(int)((long long)(trackH-thumbH)*gScrollOffset/(gContentHeight-gViewportHeight));
  gInfoScrollTrack=RECT{gInfoRect.right-uiPx(21),top,gInfoRect.right-uiPx(2),bottom};gInfoScrollThumb=RECT{gInfoRect.right-uiPx(21),thumbTop,gInfoRect.right-uiPx(2),thumbTop+thumbH};
  HBRUSH track=CreateSolidBrush(RGB(24,49,62));RECT tr{gInfoRect.right-uiPx(11),top,gInfoRect.right-uiPx(7),bottom};FillRect(h,&tr,track);DeleteObject(track);
  HBRUSH thumb=CreateSolidBrush(RGB(74,157,177));RECT th{gInfoRect.right-uiPx(12),thumbTop,gInfoRect.right-uiPx(6),thumbTop+thumbH};FillRect(h,&th,thumb);DeleteObject(thumb);
}
static void pauseInfoAutoScroll(){if(gInfoTab==1)gHistoryScrollPauseUntil=GetTickCount()+5000;gInfoWheelRemainder=0;}
static void dragInfoScrollbar(HWND w,int y){
  int travel=(gInfoScrollTrack.bottom-gInfoScrollTrack.top)-(gInfoScrollThumb.bottom-gInfoScrollThumb.top),maximum=std::max(0,gContentHeight-gViewportHeight);
  if(travel>0&&maximum>0){int relative=std::max(0,std::min(travel,y-gInfoScrollGrabY-(int)gInfoScrollTrack.top));gScrollOffset=(int)(((long long)relative*maximum+travel/2)/travel);pauseInfoAutoScroll();InvalidateRect(w,&gInfoRect,FALSE);}
}
static void paintHistoryEvents(HDC h){
  bool shortPanel=gInfoRect.bottom-gInfoRect.top<uiPx(180);int left=gInfoRect.left+uiPx(16),right=gInfoRect.right-uiPx(18),width=std::max(uiPx(120),right-left),headerY=gInfoRect.top+uiPx(shortPanel?48:54),bodyTop=gInfoRect.top+uiPx(shortPanel?72:104),trackBottom=gInfoRect.bottom-uiPx(12);
  text(h,L"播放时间："+historyYearText(gHistoryYear),RECT{left,headerY,right,headerY+uiPx(22)},14,RGB(241,249,251),true);
  if(!shortPanel)text(h,L"离线 "+std::to_wstring(gHistoryEvents.size())+L" 条 · 自动滚动 · 滚轮暂停",RECT{left,headerY+uiPx(24),right,headerY+uiPx(43)},12,RGB(160,201,210));
  if(trackBottom<=bodyTop){gContentHeight=0;gViewportHeight=0;return;}
  if(!gHistoryEventsLoaded){std::wstring error=L"历史事件模块无法读取。\n请检查 data\\world-events.tsv";text(h,error,RECT{left,bodyTop,right,std::min(trackBottom,bodyTop+uiPx(72))},12,RGB(235,173,151));gContentHeight=0;gViewportHeight=trackBottom-bodyTop;return;}
  const std::vector<size_t>& events=gHistoryEventRelayMode?gHistoryRelayEvents:historyEventsForCurrentRange();bool nearby=!gHistoryEventRelayMode&&events.size()>0&&(gHistoryEvents[events.front()].year<gHistoryEventRangeStart||gHistoryEvents[events.front()].year>gHistoryEventRangeEnd);
  std::wstring rangeLabel=gHistoryEventRelayMode?L"时间线接力 · 事件按年代连续进入":(nearby?L"附近的已收录事件（本段没有事件条目）":L"本段播放事件 · "+std::to_wstring(events.size())+L" 条");
  if(gHistoryEventRelayMode&&events.empty()){text(h,rangeLabel,RECT{left,bodyTop,right,std::min(trackBottom,bodyTop+uiPx(20))},11,RGB(112,222,225),true);if(!shortPanel)text(h,L"当前年代暂无已收录事件；后续事件会接续进入滚动队列。",RECT{left,bodyTop+uiPx(34),right,std::min(trackBottom,bodyTop+uiPx(78))},11,RGB(180,210,223));gContentHeight=0;gViewportHeight=std::max(1,trackBottom-bodyTop);gHistoryEventCycleHeight=0;return;}
  if(!shortPanel){text(h,rangeLabel,RECT{left,bodyTop,right,bodyTop+uiPx(20)},12,uiTheme::accent,true);bodyTop+=uiPx(26);}gViewportHeight=std::max(1,trackBottom-bodyTop);gContentHeight=0;
  if(gHistoryEventDescriptionWidth!=width||gHistoryEventDescriptionHeights.size()!=gHistoryEvents.size()){gHistoryEventDescriptionWidth=width;gHistoryEventDescriptionHeights.assign(gHistoryEvents.size(),0);gHistoryEventTitleHeights.assign(gHistoryEvents.size(),0);gHistoryEventPlaceHeights.assign(gHistoryEvents.size(),0);}
  std::vector<int> heights,titleHeights,placeHeights;heights.reserve(events.size());titleHeights.reserve(events.size());placeHeights.reserve(events.size());int baseContentHeight=0;for(size_t index:events){int& desc=gHistoryEventDescriptionHeights[index],&titleHeight=gHistoryEventTitleHeights[index],&placeHeight=gHistoryEventPlaceHeights[index];if(desc<=0){desc=measureText(h,gHistoryEvents[index].summary(gEnglish),width-uiPx(18),13,false,true);titleHeight=measureText(h,gHistoryEvents[index].heading(gEnglish),width-uiPx(18),13,true,true);placeHeight=measureText(h,std::wstring(gEnglish?L"Place: ":L"地点：")+gHistoryEvents[index].location(gEnglish),width-uiPx(18),12,false,true);}int height=uiPx(6+20+4+4+4+17+6)+titleHeight+placeHeight+desc;heights.push_back(height);titleHeights.push_back(titleHeight);placeHeights.push_back(placeHeight);baseContentHeight+=height+uiPx(7);}
  gHistoryEventCycleHeight=baseContentHeight;if(gHistoryEventFractionPending){gScrollOffset=baseContentHeight>0?(int)(gHistoryEventScrollFraction*baseContentHeight):0;gHistoryEventFractionPending=false;}
  int copies=baseContentHeight>gViewportHeight&&baseContentHeight>0?2:1;if(gHistoryEventRelayMode&&gHistoryPlaying)copies=1;gContentHeight=baseContentHeight*copies;
  gScrollOffset=std::max(0,std::min(gScrollOffset,std::max(0,gContentHeight-gViewportHeight)));int saved=SaveDC(h);IntersectClipRect(h,left,bodyTop,right+uiPx(1),trackBottom);
  for(int copy=0;copy<copies;copy++){int y=bodyTop-gScrollOffset+copy*baseContentHeight;
    for(size_t i=0;i<events.size();i++){
      const HistoryEvent& event=gHistoryEvents[events[i]];int height=heights[i];if(y+height<=bodyTop||y>=trackBottom){y+=height+uiPx(7);continue;}RECT card{left,y,right,y+height};HBRUSH fill=CreateSolidBrush(RGB(16,43,57));HPEN edge=CreatePen(PS_SOLID,1,RGB(36,83,101));HGDIOBJ oldBrush=SelectObject(h,fill),oldPen=SelectObject(h,edge);RoundRect(h,card.left,card.top,card.right,card.bottom,uiPx(8),uiPx(8));SelectObject(h,oldBrush);SelectObject(h,oldPen);DeleteObject(fill);DeleteObject(edge);
      int tx=left+uiPx(9),ty=y+uiPx(6),textRight=right-uiPx(9),desc=gHistoryEventDescriptionHeights[events[i]];text(h,historyYearText(event.year)+L"  ·  "+event.kind(gEnglish),RECT{tx,ty,textRight,ty+uiPx(18)},12,uiTheme::accent,true);ty+=uiPx(20);text(h,event.heading(gEnglish),RECT{tx,ty,textRight,ty+titleHeights[i]},13,RGB(241,249,251),true,true);ty+=titleHeights[i]+uiPx(4);text(h,std::wstring(gEnglish?L"Place: ":L"地点：")+event.location(gEnglish),RECT{tx,ty,textRight,ty+placeHeights[i]},12,RGB(184,211,218),false,true);ty+=placeHeights[i]+uiPx(4);text(h,event.summary(gEnglish),RECT{tx,ty,textRight,ty+desc},13,RGB(222,235,241),false,true);ty+=desc+uiPx(4);text(h,gEnglish?L"Sources and location → Find…":L"来源与地图定位 → 检索…",RECT{tx,ty,textRight,ty+uiPx(17)},11,RGB(152,190,198));y+=height+uiPx(7);
    }
  }
  RestoreDC(h,saved);paintInfoScrollbar(h,bodyTop,trackBottom);
}
static void drawGlobeEmblem(Graphics& gr,int x,int y,int size){
  GraphicsState saved=gr.Save();gr.SetSmoothingMode(SmoothingModeAntiAlias);RectF bounds((REAL)x,(REAL)y,(REAL)size,(REAL)size);LinearGradientBrush sphere(bounds,Color(255,18,127,151),Color(255,8,48,83),LinearGradientModeForwardDiagonal);gr.FillEllipse(&sphere,bounds);Pen rim(Color(235,132,231,223),1.4f);gr.DrawEllipse(&rim,bounds);
  GraphicsPath clip;clip.AddEllipse(bounds);gr.SetClip(&clip,CombineModeIntersect);Pen grid(Color(130,166,235,230),1.0f);gr.DrawArc(&grid,x+size*.30f,y+size*.06f,size*.40f,size*.88f,90,180);gr.DrawArc(&grid,x+size*.30f,y+size*.06f,size*.40f,size*.88f,270,180);gr.DrawArc(&grid,x+size*.08f,y+size*.32f,size*.84f,size*.36f,0,360);gr.DrawLine(&grid,x+size*.08f,y+size*.50f,x+size*.92f,y+size*.50f);
  SolidBrush land(Color(220,109,198,151));PointF continentA[]={{(REAL)(x+size*.24f),(REAL)(y+size*.31f)},{(REAL)(x+size*.38f),(REAL)(y+size*.22f)},{(REAL)(x+size*.46f),(REAL)(y+size*.30f)},{(REAL)(x+size*.42f),(REAL)(y+size*.39f)},{(REAL)(x+size*.31f),(REAL)(y+size*.43f)}};gr.FillPolygon(&land,continentA,5);PointF continentB[]={{(REAL)(x+size*.55f),(REAL)(y+size*.47f)},{(REAL)(x+size*.73f),(REAL)(y+size*.40f)},{(REAL)(x+size*.81f),(REAL)(y+size*.50f)},{(REAL)(x+size*.71f),(REAL)(y+size*.59f)},{(REAL)(x+size*.60f),(REAL)(y+size*.57f)}};gr.FillPolygon(&land,continentB,5);gr.Restore(saved);
  SolidBrush marker(Color(255,255,205,112));gr.FillEllipse(&marker,(REAL)(x+size*.68f),(REAL)(y+size*.16f),(REAL)(size*.11f),(REAL)(size*.11f));
}
static int sidePanelWidthDip(int clientWidth){if(!gPreferredPanelWidth)return clientWidth<820?240+std::max(0,clientWidth-760):(clientWidth<1200?300:360);return std::max(240,std::min(gPreferredPanelWidth,std::min(520,clientWidth-560)));}
static int sidePanelWidth(int clientWidth){return uiPx(sidePanelWidthDip(uiDip(clientWidth)));}
static bool compactLayoutFor(int clientWidth){return clientWidth<1200||clientWidth-sidePanelWidth(clientWidth)-56<850;}
static bool stackedLayoutFor(int clientWidth){return clientWidth<STACKED_TOOLBAR_WIDTH||clientWidth-sidePanelWidth(clientWidth)-56<620;}
static int toolbarHeightFor(int clientWidth){return compactLayoutFor(clientWidth)?150:116;}
static int headerExtraFor(int clientWidth){return clientWidth<STACKED_TOOLBAR_WIDTH?32:0;}
static int infoTopFor(int clientWidth){return uiPx((uiDip(clientWidth)<1000?110:72)+8);}
static int infoPanelScrollStep(int viewportHeight){int height=std::max(1,viewportHeight);return std::max(1,std::min(uiPx(72),height-height/4));}
// Preserve sub-notch wheel movement as fractional pixels, and discard pressure at the ends.
static int scrollInfoPanel(int offset,int maximum,int delta,int step,int& remainder){
  maximum=std::max(0,maximum);offset=std::max(0,std::min(offset,maximum));
  if(!maximum){remainder=0;return 0;}
  int amount=remainder+delta*step;int movement=amount/WHEEL_DELTA;remainder=amount%WHEEL_DELTA;
  int next=std::max(0,std::min(maximum,offset-movement));
  if((next==0&&amount>0)||(next==maximum&&amount<0))remainder=0;
  return next;
}
#include "event_search_impl.h"
#include "region_panel_impl.h"
#include "experience_shell.h"
#include "unified_search_impl.h"
#include "saved_places_ui.h"
#include "session_impl.h"
#include "startup_feedback.h"
static void paint(HWND w,HDC h){paintExperienceShell(w,h);}
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
static void layoutControls(HWND w){UiDpiScope dpi(w);layoutExperienceControls(w);}
static void refreshUiLanguage(HWND w){
  refreshSelectedReferenceLanguage();populateUiScaleChoices(w);refreshUiChildFonts(w);
  gHistoryEventDescriptionWidth=0;
  setUiText(w,L"世界地图探索器");
  if(gSearchEdit)InvalidateRect(gSearchEdit,nullptr,TRUE);
  if(gUiTooltip)SendMessageW(gUiTooltip,TTM_POP,0,0);
  for(HWND control:{gSearchEdit,gGridLevelCombo,gMapPreviewRateCombo,gOpacityLabel})if(control)SendMessageW(control,WM_SETFONT,(WPARAM)uiFontFor(13,false),TRUE);
  setUiText(gSearchButton,L"查找");if(gHighResMapButton)SetWindowTextW(gHighResMapButton,gEnglish?L"HD map":L"高清底图");setUiText(gControlPointsButton,L"配准校验点");
  setUiText(gNasaMapButton,L"NASA 2 km 底图");setUiText(gLandCoverToggleButton,L"土地覆盖");setUiText(gPopulationToggleButton,L"人口估算（2023）");
  setUiText(gWorld2026ToggleButton,L"国家与地区（2026）");if(gHistoryToggleButton)SetWindowTextW(gHistoryToggleButton,gEnglish?L"History map":L"历史疆域");setUiText(gHistoryAnimationButton,L"渐变动画试行");
  setUiText(gGridLinesToggleButton,L"显示网格线");SetWindowTextW(gGeoTabButton,gEnglish?L"Region":L"区域");SetWindowTextW(gEventsTabButton,gEnglish?L"Events":L"事件");if(gEventSearchButton)SetWindowTextW(gEventSearchButton,gEnglish?L"Find…":L"检索…");refreshEventSearchLanguage();setUiText(gFullscreenButton,gMapFullscreen?L"退出全屏":L"地图全屏");if(gExportButton)SetWindowTextW(gExportButton,gEnglish?L"Export map image...":L"导出地图图片…");
  if(gMapPreviewRateCombo){SendMessageW(gMapPreviewRateCombo,CB_RESETCONTENT,0,0);for(int mode=MAP_PREVIEW_RATE_30;mode<=MAP_PREVIEW_RATE_ADAPTIVE;mode++){std::wstring label=mapViewPreviewRateText((MapPreviewRateMode)mode);SendMessageW(gMapPreviewRateCombo,CB_ADDSTRING,0,(LPARAM)label.c_str());}SendMessageW(gMapPreviewRateCombo,CB_SETCURSEL,(WPARAM)gMapViewPreviewRateMode,0);}
  if(gLanguageButton)SetWindowTextW(gLanguageButton,languageButtonCaption());
  populateRouteCombo();
  if(gGridLevelCombo){int selected=(int)SendMessageW(gGridLevelCombo,CB_GETCURSEL,0,0);SendMessageW(gGridLevelCombo,CB_RESETCONTENT,0,0);const wchar_t* levels[]={L"格网 9,800 格（原格）",L"格网 39,200 格",L"格网 156,800 格",L"格网 627,200 格"};for(const wchar_t* level:levels){std::wstring label=localizeUiText(level);SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)label.c_str());}SendMessageW(gGridLevelCombo,CB_SETCURSEL,selected>=0?selected:gGridLevel,0);}
  if(gHistoryYearCombo){int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);SendMessageW(gHistoryYearCombo,CB_RESETCONTENT,0,0);for(int i=0;i<HISTORY_YEAR_COUNT;i++){std::wstring label=historyYearText(HISTORY_YEARS[i]);SendMessageW(gHistoryYearCombo,CB_ADDSTRING,0,(LPARAM)label.c_str());}SendMessageW(gHistoryYearCombo,CB_SETCURSEL,selected>=0?selected:0,0);}
  updateHistoryPlaybackButton();updatePickerCaptions();if(gHistorySpeedButton)setUiText(gHistorySpeedButton,HISTORY_PLAYBACK_SPEEDS[gHistorySpeedIndex]);
  if(gOpacityLabel){wchar_t opacity[48];swprintf(opacity,48,L"不透明度 %d%%",gFillOpacity);setUiText(gOpacityLabel,opacity);}
  layoutControls(w);gMapSceneCacheValid=false;InvalidateRect(w,nullptr,TRUE);
}
static std::vector<HWND> mainKeyboardControls(HWND w){return experienceKeyboardControls(w);}
static const wchar_t* controlHint(int id){
  switch(id){
    case 1070:return gEnglish?L"Explore places and surrounding geography.":L"查找地点，阅读周边地理与人文概况。";
    case 1071:return gEnglish?L"Choose a year, seek the timeline or play historical changes.":L"选择年份、拖动时间轴或播放历史变化。";
    case 1072:return gEnglish?L"Choose, inspect and play a historical journey.":L"选择、阅读与播放历史旅行路线。";
    case 1073:return gEnglish?L"Change terrain, detailed terrain or NASA imagery while keeping your current view.":L"切换地形、高清地形或NASA底图，保留当前位置与缩放。";
    case 1074:return gEnglish?L"Show historical, country, population or land-cover layers.":L"按需显示历史、国家/地区、人口或土地覆盖图层。";
    case 1075:return gEnglish?L"Location grids, grid lines, animation, interaction smoothness and help.":L"定位网格、格线、动画、交互预览与操作帮助。";
    case 1076:return gEnglish?L"Previous map view (Alt+Left).":L"返回上一个地图位置（Alt+左）。";
    case 1077:return gEnglish?L"Next map view (Alt+Right).":L"前进到下一个地图位置（Alt+右）。";
    case 1078:return gEnglish?L"Previous year; BCE 1 is immediately before CE 1.":L"前一年；公元1年的前一年是公元前1年。";
    case 1079:return gEnglish?L"Next year; no year zero is inserted.":L"后一年；不插入不存在的公元0年。";
    case 1080:return gEnglish?L"Seek a year and pause historical playback.":L"拖动前往所选年份，并暂停历史播放。";
    case 1081:return gEnglish?L"Seek by reference connection distance and pause the journey.":L"按参考连线距离选择进度，并暂停路线。";
    case 1082:return gEnglish?L"Open places saved on this computer.":L"查看保存在本机的收藏位置。";
    case 1084:return gEnglish?L"Begin again from the global map (Alt+Home).":L"从全球地图开始新的探索（Alt+Home）。";
    case 1085:return gEnglish?L"Restore your view and paused journeys when this app starts.":L"启动时恢复地图位置、图层与暂停的路线。";
    case 1083:return gEnglish?L"Add or remove the selected location from saved places.":L"收藏当前位置，或取消这个位置的收藏。";
    case 1001:return gEnglish?L"Reference control points used to check map alignment.":L"显示用于核对底图配准的位置参考点。";
    case 1086:return gEnglish?L"Follow Windows, or choose a comfortable interface size. Changes are saved.":L"跟随系统缩放，或选择舒适的界面大小；设置会自动保存。";
    case 1002:case 1003:return gEnglish?L"Search places, events, journeys, years, grid IDs or latitude,longitude. Ctrl+F opens selectable results.":L"可查地点、事件、路线、年份、网格编号或 纬度,经度。Ctrl+F 打开可选择的搜索结果。";
    case 1008:return gEnglish?L"Four location grids. Detailed records still refer to the original 9,800 base regions.":L"四档网格用于定位；详细资料仍关联原有 9,800 个基础分区。";
    case 1010:case 1047:return gEnglish?L"Choose any of 5,424 years. Borders follow source validity intervals; blank means no matching record.":L"可选择5424个年份。疆界按来源有效年段显示，空白表示缺少对应记录。";
    case 1012:return gEnglish?L"Adjust opacity for historical, country and route overlays.":L"调整历史疆域、国别与路线填色的不透明度。";
    case 1024:return gEnglish?L"Switch between English and Simplified Chinese. Source narratives remain in their original language.":L"切换简体中文与英文界面；资料叙述保留原始语言。";
    case 1025:return gEnglish?L"Frame rate for temporary pan/zoom previews. The settled map redraws from original imagery.":L"控制平移与缩放预览的帧率；操作结束后仍从原图绘制清晰画面。";
    case 1066:return gEnglish?L"Search all historical events independently (Ctrl+E). Filter by keywords, category and years; navigate to a place or year.":L"独立检索全部历史事件（Ctrl+E），按关键词、类别和年份筛选，并定位地图或查看年份。";
    case 1048:return gEnglish?L"Search 42 historical journeys, voyages and campaigns. Choose a route to play, inspect or fit it.":L"搜索42条历史旅行、航海与战役路线，可播放、查看说明或定位。";
    case 1049:return gEnglish?L"Save the current map view, legends and data notes as PNG. Ctrl+S.":L"将当前地图、图例与资料说明保存为 PNG 图片。Ctrl+S。";
    case 1050:return gEnglish?L"Zoom in around the map center. Ctrl++.":L"以地图中心放大。Ctrl++。";
    case 1051:return gEnglish?L"Zoom out around the map center. Ctrl+-.":L"以地图中心缩小。Ctrl+-。";
    case 1052:return gEnglish?L"Reset to the global view. Ctrl+0.":L"恢复全球视图。Ctrl+0。";
    case 1053:return gEnglish?L"Help and shortcuts (F1). Drag the sidebar divider to resize; double-click it to restore automatic width.":L"使用提示与快捷键（F1）。拖动信息栏左边缘调整宽度，双击恢复自动宽度。";
    default:return nullptr;
  }
}
static void createControlHints(HWND w){
  gUiTooltip=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,w,nullptr,GetModuleHandleW(nullptr),nullptr);
  if(!gUiTooltip)return;
  SendMessageW(gUiTooltip,TTM_SETMAXTIPWIDTH,0,360);SendMessageW(gUiTooltip,TTM_SETDELAYTIME,TTDT_AUTOPOP,9000);
  for(HWND control:mainKeyboardControls(w)){if(!control||!controlHint(GetDlgCtrlID(control)))continue;TOOLINFOW info{sizeof(info)};info.uFlags=TTF_IDISHWND|TTF_SUBCLASS;info.hwnd=w;info.uId=(UINT_PTR)control;info.lpszText=LPSTR_TEXTCALLBACKW;SendMessageW(gUiTooltip,TTM_ADDTOOLW,0,(LPARAM)&info);}
}
static void showAppHelp(HWND w){
  const wchar_t* help=gEnglish?
    L"Browse: scroll to zoom, drag to pan, double-click to reset.\nSearch: a place, a grid ID, or latitude,longitude. Choose a result to open it. Add a country to distinguish cities with the same name.\n\nCtrl+F  Search places, events, journeys or years\nAlt+Left / Alt+Right  Previous / next view\nCtrl+E  Explore all historical events\nCtrl+S  Export map PNG\nCtrl++ / Ctrl+-  Zoom\nCtrl+0  Global view\nAlt+Home  Start a new exploration\nCtrl+Shift+C  Copy selected coordinates\nCtrl+Shift+S  Show / hide scale bar\nF11 / Esc  Enter / leave full screen\nTab / Shift+Tab  Move keyboard focus\n\nDrag the divider beside the information panel to resize it; double-click to restore automatic width. Display choices are remembered.\n\nThe scale bar is approximate at its labeled latitude on the WGS 84 ellipsoid. Grid cells are not equal in area. Source narratives may remain in Chinese.":
    L"地图：滚轮缩放、拖动平移、双击复位。\n搜索：地点、网格编号或 纬度,经度。选择结果即可前往；同名城市可添加国家或地区。\n\nCtrl+F  搜索地点、事件、路线或年份\nAlt+左 / Alt+右  返回 / 前进\nCtrl+E  独立检索全部历史事件\nCtrl+S  导出地图 PNG 图片\nCtrl++ / Ctrl+-  放大 / 缩小\nCtrl+0  恢复全球视图\nAlt+Home  开始新的探索\nCtrl+Shift+C  复制选中位置的坐标\nCtrl+Shift+S  显示 / 隐藏比例尺\nF11 / Esc  进入 / 退出全屏\nTab / Shift+Tab  移动键盘焦点\n\n拖动信息栏左边缘可调整宽度，双击恢复自动宽度。显示偏好会自动记忆。\n\n比例尺按 WGS 84 椭球在标注纬度附近估算；经纬网格并非等面积。界面支持中英文，资料叙述保留原始语言。";
  MessageBoxW(w,help,gEnglish?L"Map help":L"地图使用提示",MB_OK|MB_ICONINFORMATION);
}
static bool copySelectedCoordinates(HWND w){
  if(gCol<0)return false;
  std::wostringstream out;out<<std::fixed<<std::setprecision(6)<<gSelectedLatitude<<L","<<gSelectedLongitude;std::wstring value=out.str();
  HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));if(!memory)return false;
  void* data=GlobalLock(memory);if(!data){GlobalFree(memory);return false;}std::memcpy(data,value.c_str(),(value.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);
  if(!OpenClipboard(w)){GlobalFree(memory);return false;}
  bool ok=EmptyClipboard()&&SetClipboardData(CF_UNICODETEXT,memory)!=nullptr;CloseClipboard();if(!ok)GlobalFree(memory);return ok;
}
static void enableMainKeyboardStops(HWND w){for(HWND control:mainKeyboardControls(w))if(control){LONG_PTR style=GetWindowLongPtrW(control,GWL_STYLE);SetWindowLongPtrW(control,GWL_STYLE,style|WS_TABSTOP);}}
static bool focusNextMainControl(HWND w,bool backwards){
  std::vector<HWND> controls=mainKeyboardControls(w);if(controls.empty())return false;HWND focus=GetFocus();int current=-1;
  for(size_t i=0;i<controls.size();i++)if(controls[i]&&(controls[i]==focus||IsChild(controls[i],focus))){current=(int)i;break;}
  int size=(int)controls.size();for(int step=1;step<=size;step++){
    int index=current<0?(backwards?size-step:step-1):(current+(backwards?-step:step)+size*2)%size;
    HWND target=controls[index];if(!target||!IsWindowVisible(target)||!IsWindowEnabled(target))continue;
    SendMessageW(w,WM_CHANGEUISTATE,MAKEWPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);SetFocus(target);return true;
  }return false;
}
static bool handleMainKeyboard(HWND w,const MSG& message){
  if(message.message!=WM_KEYDOWN&&message.message!=WM_SYSKEYDOWN)return false;bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,alt=(GetKeyState(VK_MENU)&0x8000)!=0;
  if(message.wParam==VK_ESCAPE&&gExperiencePanel){if((GetFocus()==gGridLevelCombo||GetFocus()==gMapPreviewRateCombo)&&SendMessageW(GetFocus(),CB_GETDROPPEDSTATE,0,0)){SendMessageW(GetFocus(),CB_SHOWDROPDOWN,FALSE,0);return true;}closeExperiencePanel(w);return true;}
  if(alt&&!ctrl&&message.wParam==VK_HOME){startFreshExploration(w);return true;}
  if(alt&&!ctrl&&(message.wParam==VK_LEFT||message.wParam==VK_RIGHT)){restoreExperienceView(w,message.wParam==VK_RIGHT);return true;}
  if(message.wParam==VK_TAB&&!ctrl&&!alt)return focusNextMainControl(w,(GetKeyState(VK_SHIFT)&0x8000)!=0);
  if(message.wParam=='F'&&ctrl&&!alt){showUnifiedSearch(w);return true;}
  bool shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
  if(message.wParam==VK_F1&&!ctrl&&!alt){showAppHelp(w);return true;}
  if(ctrl&&!alt){
    if(message.wParam=='E'){showEventSearch(w);return true;}
    if(message.wParam=='S'&&shift){gShowScaleBar=!gShowScaleBar;schedulePreferencesSave(w);InvalidateRect(w,&gMapRect,FALSE);return true;}
    if(message.wParam=='S'){chooseMapExport(w);return true;}
    if(message.wParam=='C'&&shift){bool copied=copySelectedCoordinates(w);gNotice=copied?(gEnglish?L"Coordinates copied":L"已复制坐标"):(gEnglish?L"Select a map location before copying coordinates":L"请先在地图中选择位置再复制坐标");invalidateNotice(w);return true;}
    int command=message.wParam=='0'||message.wParam==VK_NUMPAD0?1052:message.wParam==VK_ADD||message.wParam==VK_OEM_PLUS?1050:message.wParam==VK_SUBTRACT||message.wParam==VK_OEM_MINUS?1051:0;
    if(command){SendMessageW(w,WM_COMMAND,MAKEWPARAM(command,BN_CLICKED),0);return true;}
  }
  if(message.wParam==VK_RETURN&&!alt){HWND focus=GetFocus();if(focus==gSearchEdit&&IsWindowEnabled(gSearchButton)){SendMessageW(w,WM_COMMAND,MAKEWPARAM(1003,BN_CLICKED),(LPARAM)gSearchButton);return true;}
    wchar_t name[32]{};if(focus&&IsWindowVisible(focus)&&IsWindowEnabled(focus)&&GetClassNameW(focus,name,32)&&_wcsicmp(name,L"BUTTON")==0){SendMessageW(focus,BM_CLICK,0,0);return true;}}
  return false;
}
static void setMapFullscreen(HWND w,bool enabled){
  if(enabled){closeExperiencePanel(w);commitHistorySeek(w);}
  if(enabled==gMapFullscreen)return;
  HWND controls[]={gControlPointsButton,gSearchEdit,gSearchButton,gHighResMapButton,gGridLevelCombo,gNasaMapButton,gLandCoverToggleButton,gPopulationToggleButton,gWorld2026ToggleButton,gHistoryToggleButton,gYearPickerButton,gOpacityTrackbar,gOpacityLabel,gGridLinesToggleButton,gHistoryPlaybackButton,gHistorySpeedButton,gHistoryAnimationButton,gGeoTabButton,gEventsTabButton,gEventSearchButton,gFullscreenButton,gExportButton,gLanguageButton,gMapPreviewRateCombo,gRoutePanelButton,gRouteButton,gRoutePickerButton,gRouteHideButton,gRouteInfoButton,gRouteFitButton,gZoomInButton,gZoomOutButton,gResetMapButton,gHelpButton};
  if(enabled){gWindowedStyle=GetWindowLongPtrW(w,GWL_STYLE);gWindowedPlacement.length=sizeof(WINDOWPLACEMENT);GetWindowPlacement(w,&gWindowedPlacement);gMapFullscreen=true;for(HWND control:controls)if(control)ShowWindow(control,SW_HIDE);SetFocus(w);LONG_PTR style=(gWindowedStyle&~WS_OVERLAPPEDWINDOW)|WS_POPUP|WS_CLIPCHILDREN|WS_VISIBLE;SetWindowLongPtrW(w,GWL_STYLE,style);MONITORINFO monitor{sizeof(MONITORINFO)};HMONITOR target=MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST);if(GetMonitorInfoW(target,&monitor)){RECT bounds=monitor.rcMonitor;SetWindowPos(w,HWND_TOP,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_FRAMECHANGED|SWP_NOOWNERZORDER|SWP_SHOWWINDOW);}setUiText(gFullscreenButton,L"退出全屏");}
  else{gMapFullscreen=false;SetWindowLongPtrW(w,GWL_STYLE,gWindowedStyle);SetWindowPlacement(w,&gWindowedPlacement);SetWindowPos(w,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED|SWP_SHOWWINDOW);for(HWND control:controls)if(control)ShowWindow(control,SW_SHOW);setUiText(gFullscreenButton,L"地图全屏");SetFocus(w);}
  KillTimer(w,MAP_VIEW_SETTLE_TIMER);cancelMapViewPreviewTimer(w);gMapViewPreviewActive=false;resetMapViewPreviewCadence();layoutControls(w);gMapSceneCacheValid=false;InvalidateRect(w,nullptr,TRUE);
}
static bool pointInsideRect(const RECT& rect,int x,int y){return x>=rect.left&&x<rect.right&&y>=rect.top&&y<rect.bottom;}
static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM a,LPARAM b){
  UiDpiScope dpi(w,m==WM_DPICHANGED?LOWORD(a):0);
  if(m==WM_DPICHANGED||m==UI_SCALE_CHANGED){gHistoryEventDescriptionWidth=0;gInfoScrollTrack=RECT{};gInfoScrollThumb=RECT{};gMapSceneCacheValid=false;gMapBaseCacheValid=false;gMapBaseOverviewCacheValid=false;refreshUiChildFonts(w);if(!gMapFullscreen)applyUiWindowBounds(w,m,a,b);layoutControls(w);InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1086&&HIWORD(a)==CBN_SELCHANGE){int choice=(int)SendMessageW(gUiScaleCombo,CB_GETCURSEL,0,0);if(choice>=0&&choice<=4&&choice!=gUiScaleMode){int previous=gUiScaleMode;SendMessageW(gUiScaleCombo,CB_SHOWDROPDOWN,FALSE,0);gUiScaleMode=choice;gUiDpi=effectiveUiDpi(w);schedulePreferencesSave(w);EnumThreadWindows(GetCurrentThreadId(),broadcastUiScale,previous);}return 0;}
  if(handleExperienceMessage(w,m,a,b))return 0;
  if(m==WM_ACTIVATE&&LOWORD(a)!=WA_INACTIVE&&gPreferencesReady){syncSavedPlacesFromDisk(w);}
  if(m==WM_TIMER&&a==SAVED_PLACES_WATCH_TIMER){DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);if(gPreferencesReady&&process==GetCurrentProcessId()&&!(GetAsyncKeyState(VK_LBUTTON)&0x8000))syncSavedPlacesFromDisk(w);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1066&&HIWORD(a)==BN_CLICKED){showEventSearch(w);return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&LOWORD(a)>=1061&&LOWORD(a)<=1064){gRegionPage=LOWORD(a)-1061;gScrollOffset=0;gInfoWheelRemainder=0;layoutControls(w);InvalidateRect(w,&gInfoRect,FALSE);return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&LOWORD(a)==1065){showRegionOriginal(w);return 0;}
  if(m==WM_NOTIFY&&gUiTooltip&&((NMHDR*)b)->hwndFrom==gUiTooltip&&((NMHDR*)b)->code==TTN_GETDISPINFOW){auto* hint=(NMTTDISPINFOW*)b;hint->lpszText=const_cast<LPWSTR>(controlHint(GetDlgCtrlID((HWND)hint->hdr.idFrom)));return 0;}
  if(m==WM_COMMAND){int id=LOWORD(a);if(id==1001||id==1008||id==1014||id==1016||id==1020||id==1024||id==1025||id==1046)schedulePreferencesSave(w);}
  if(m==WM_HSCROLL&&(HWND)b==gOpacityTrackbar)schedulePreferencesSave(w);
  if(m==WM_TIMER&&a==PREFERENCES_SAVE_TIMER){KillTimer(w,PREFERENCES_SAVE_TIMER);if(gPreferencesDirty&&saveDisplayPreferences())gPreferencesDirty=false;return 0;}
  if(m==WM_QUERYENDSESSION){commitHistorySeek(w);saveLastView();if(gPreferencesReady&&gPreferencesDirty)saveDisplayPreferences();return TRUE;}
  if(m==WM_CLOSE){commitHistorySeek(w);saveLastView();KillTimer(w,NAVIGATION_CAPTURE_TIMER);if(gPreferencesReady&&gPreferencesDirty)saveDisplayPreferences();DestroyWindow(w);return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&LOWORD(a)>=1050&&LOWORD(a)<=1053){
    int id=LOWORD(a);if(id==1053){showAppHelp(w);return 0;}
    KillTimer(w,MAP_VIEW_SETTLE_TIMER);cancelMapViewPreviewTimer(w);gMapViewPreviewActive=false;
    if(id==1052)resetMapView();else zoomMapAt((gMapRect.left+gMapRect.right)/2,(gMapRect.top+gMapRect.bottom)/2,id==1050?1.4:1.0/1.4);
    gMapSceneCacheValid=false;rememberView();InvalidateRect(w,&gMapRect,FALSE);return 0;
  }
  if(m==WM_SETCURSOR&&(HWND)a==w&&LOWORD(b)==HTCLIENT){POINT point{};GetCursorPos(&point);ScreenToClient(w,&point);if(!gMapFullscreen&&(gPanelResizing||PtInRect(&gPanelDividerRect,point))){SetCursor(LoadCursorW(nullptr,IDC_SIZEWE));return TRUE;}if(mapContainsPoint(point.x,point.y)){SetCursor(LoadCursorW(nullptr,gMapDragging?IDC_SIZEALL:IDC_CROSS));return TRUE;}SetCursor(LoadCursorW(nullptr,IDC_ARROW));return TRUE;}
  if(m==WM_LBUTTONDOWN&&!gMapFullscreen){POINT point{GET_X_LPARAM(b),GET_Y_LPARAM(b)};if(PtInRect(&gPanelDividerRect,point)){gPanelResizing=true;SetCapture(w);return 0;}}
  if(m==WM_MOUSEMOVE&&gPanelResizing&&(a&MK_LBUTTON)){RECT client{};GetClientRect(w,&client);int width=std::max(260,std::min(520,uiDip((int)client.right-GET_X_LPARAM(b))+2));if(width!=gPreferredPanelWidth){gPreferredPanelWidth=width;gHistoryEventDescriptionWidth=0;gMapSceneCacheValid=false;layoutControls(w);InvalidateRect(w,nullptr,FALSE);}return 0;}
  if(m==WM_LBUTTONUP&&gPanelResizing){gPanelResizing=false;if(GetCapture()==w)ReleaseCapture();schedulePreferencesSave(w);InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_LBUTTONDBLCLK&&!gMapFullscreen){POINT point{GET_X_LPARAM(b),GET_Y_LPARAM(b)};if(PtInRect(&gPanelDividerRect,point)){gPanelResizing=false;if(GetCapture()==w)ReleaseCapture();gPreferredPanelWidth=0;layoutControls(w);gMapSceneCacheValid=false;schedulePreferencesSave(w);InvalidateRect(w,nullptr,FALSE);return 0;}}
  if((m==WM_CAPTURECHANGED||m==WM_CANCELMODE)&&gPanelResizing){gPanelResizing=false;schedulePreferencesSave(w);if(m==WM_CANCELMODE&&GetCapture()==w)ReleaseCapture();}
  if(m==WM_GETMINMAXINFO){MONITORINFO monitor{sizeof(monitor)};if(GetMonitorInfoW(MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST),&monitor)){MINMAXINFO* limits=(MINMAXINFO*)b;limits->ptMinTrackSize.x=std::min((LONG)uiPx(800),monitor.rcWork.right-monitor.rcWork.left);limits->ptMinTrackSize.y=std::min((LONG)uiPx(600),monitor.rcWork.bottom-monitor.rcWork.top);}return 0;}
  if((m==WM_CTLCOLOREDIT||m==WM_CTLCOLORLISTBOX)&&gUiControlBrush){HDC dc=(HDC)a;SetTextColor(dc,uiTheme::text);SetBkColor(dc,RGB(18,43,57));return (LRESULT)gUiControlBrush;}
  if(m==WM_CTLCOLORSTATIC&&(HWND)b==gOpacityLabel&&gUiPanelBrush){HDC dc=(HDC)a;SetTextColor(dc,RGB(166,215,224));SetBkColor(dc,uiTheme::toolbar);return (LRESULT)gUiPanelBrush;}
  if(m==WM_DRAWITEM&&((const DRAWITEMSTRUCT*)b)->CtlType==ODT_BUTTON){drawStyledButton((const DRAWITEMSTRUCT*)b);return TRUE;}
  if(m==WM_COMMAND&&LOWORD(a)==1025&&HIWORD(a)==CBN_SELCHANGE){int mode=(int)SendMessageW(gMapPreviewRateCombo,CB_GETCURSEL,0,0);if(mode>=MAP_PREVIEW_RATE_30&&mode<=MAP_PREVIEW_RATE_ADAPTIVE){gMapViewPreviewRateMode=(MapPreviewRateMode)mode;cancelMapViewPreviewTimer(w);resetMapViewPreviewCadence();gNotice=mapViewPreviewRateText();invalidateNotice(w);if(gMapViewPreviewActive)requestMapViewPreviewFrame(w);}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1024&&HIWORD(a)==BN_CLICKED){gEnglish=!gEnglish;refreshUiLanguage(w);refreshUnifiedSearchLanguage();return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1023&&HIWORD(a)==BN_CLICKED){setMapFullscreen(w,!gMapFullscreen);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1049&&HIWORD(a)==BN_CLICKED){chooseMapExport(w);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1046&&HIWORD(a)==BN_CLICKED){gRoutePanelOpen=!gRoutePanelOpen;layoutControls(w);InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1047&&HIWORD(a)==BN_CLICKED){std::vector<std::wstring> labels;for(int year:HISTORY_YEARS)labels.push_back(historyYearText(year));int selected=showPicker(w,gEnglish?L"Choose a historical year":L"选择历史年份",labels,(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0));if(selected>=0){SendMessageW(gHistoryYearCombo,CB_SETCURSEL,selected,0);SendMessageW(w,WM_COMMAND,MAKEWPARAM(1010,CBN_SELCHANGE),(LPARAM)gHistoryYearCombo);}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1048&&HIWORD(a)==BN_CLICKED){HWND previousFocus=GetFocus();std::vector<std::wstring> labels;for(const auto& route:routes::catalog())labels.push_back(std::wstring(gEnglish?route.en:route.zh)+L"  ·  "+route.period+L"  ·  "+routes::travelModeLabel(route.mode,gEnglish));int previous=gRouteIndex;int selected=showPicker(w,gEnglish?L"Choose a route":L"选择路线",labels,previous);if(selected>=0&&selected!=previous){SendMessageW(gRouteCombo,CB_SETCURSEL,selected+1,0);SendMessageW(w,WM_COMMAND,MAKEWPARAM(1041,CBN_SELCHANGE),(LPARAM)gRouteCombo);if(gRouteIndex==selected){SendMessageW(w,WM_COMMAND,MAKEWPARAM(1045,BN_CLICKED),(LPARAM)gRouteFitButton);if(previousFocus&&IsWindow(previousFocus)&&IsChild(w,previousFocus)&&IsWindowVisible(previousFocus)&&IsWindowEnabled(previousFocus))SetFocus(previousFocus);}}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1041&&HIWORD(a)==CBN_SELCHANGE){int index=(int)SendMessageW(gRouteCombo,CB_GETCURSEL,0,0)-1;if(index>=0&&index<(int)routes::catalog().size())handleRouteCommand(w,3000+index);else if(gRouteIndex>=0)handleRouteCommand(w,3101);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1040&&HIWORD(a)==BN_CLICKED){handleRouteCommand(w,3100);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1043&&HIWORD(a)==BN_CLICKED){handleRouteCommand(w,3101);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1044&&HIWORD(a)==BN_CLICKED){handleRouteCommand(w,3102);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1045&&HIWORD(a)==BN_CLICKED){
    const auto& all=routes::catalog();if(gRouteIndex<0||(size_t)gRouteIndex>=all.size())return 0;
    const auto& stops=all[gRouteIndex].stops;double left=1,right=0,top=1,bottom=0;
    for(const auto& stop:stops){double x=(stop.position.longitude+180.0)/360.0,y=(90.0-stop.position.latitude)/180.0;left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);}
    if(right-left>0.5){resetMapView();}else{double span=std::max(right-left,bottom-top);gMapZoom=bounded(0.78/std::max(span,0.02),1.0,mapZoomLimit());gMapCenterX=(left+right)/2;gMapCenterY=(top+bottom)/2;clampMapView();}
    KillTimer(w,MAP_VIEW_SETTLE_TIMER);cancelMapViewPreviewTimer(w);gMapViewPreviewActive=false;gMapSceneCacheValid=false;InvalidateRect(w,&gMapRect,FALSE);return 0;
  }
  if(m==WM_COMMAND&&HIWORD(a)==0&&LOWORD(a)>=3000&&LOWORD(a)<=3102){handleRouteCommand(w,LOWORD(a));return 0;}
  if(m==WM_TIMER&&a==ROUTE_PLAYBACK_TIMER){const auto& all=routes::catalog();if(!gRoutePlaying||gRouteIndex<0||(size_t)gRouteIndex>=all.size()){KillTimer(w,ROUTE_PLAYBACK_TIMER);gRoutePlaying=false;updateRouteControls();return 0;}double end=(double)all[gRouteIndex].stops.size()-1;ULONGLONG now=routePlaybackTime(),elapsed=now>=gRouteLastPlaybackTime?now-gRouteLastPlaybackTime:0;gRouteLastPlaybackTime=now;double duration=std::max(12.0,std::min(45.0,gRouteDistance.length()/1500.0));gRouteProgress=gRouteDistance.advance(gRouteProgress,(double)elapsed/10000000.0,duration);if(!updateRouteRaster()||gRouteProgress>=end){KillTimer(w,ROUTE_PLAYBACK_TIMER);gRoutePlaying=false;updateRouteControls();}syncExperienceControls();InvalidateRect(w,&gMapRect,FALSE);return 0;}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&isToggleButtonId(LOWORD(a))){HWND button=(HWND)b;LRESULT checked=SendMessageW(button,BM_GETCHECK,0,0);SendMessageW(button,BM_SETCHECK,checked==BST_CHECKED?BST_UNCHECKED:BST_CHECKED,0);}
  if(m==WM_COMMAND&&HIWORD(a)==BN_CLICKED&&(LOWORD(a)==1018||LOWORD(a)==1019)){gInfoTab=LOWORD(a)==1018?0:1;gInfoWheelRemainder=0;if(!gHistoryEventRelayMode)gScrollOffset=0;gHistoryEventFractionPending=false;layoutRegionPageControls(w);if(gInfoTab==1)SetTimer(w,HISTORY_EVENT_SCROLL_TIMER,33,nullptr);else KillTimer(w,HISTORY_EVENT_SCROLL_TIMER);InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_ERASEBKGND)return 1;
  if(m==WM_PAINT){PAINTSTRUCT ps;HDC h=BeginPaint(w,&ps);RECT client;GetClientRect(w,&client);if(ensureBackBuffer(h,client.right,client.bottom)){int saved=SaveDC(gBackBufferDC);if(saved){IntersectClipRect(gBackBufferDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);paint(w,gBackBufferDC);RestoreDC(gBackBufferDC,saved);BitBlt(h,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,gBackBufferDC,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);}else paint(w,h);}else paint(w,h);EndPaint(w,&ps);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1008&&HIWORD(a)==CBN_SELCHANGE){int level=(int)SendMessageW(gGridLevelCombo,CB_GETCURSEL,0,0);if(level>=0&&level<=3&&level!=gGridLevel){gGridLevel=level;gScrollOffset=0;gInfoWheelRemainder=0;gNotice=L"格网已切换为 "+std::to_wstring(activeGridCount())+L" 格；地形、历史、人口等仍按 Z9800 基础网格读取";InvalidateRect(w,nullptr,TRUE);}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1010&&HIWORD(a)==CBN_SELCHANGE){if(gHistoryPlaying)KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;finishHistoryTransition(w);updateHistoryPlaybackButton();int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);setHistoryEventRange(selected,false);selectHistoryYear(w,selected);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1015&&HIWORD(a)==BN_CLICKED){
    if(gHistoryPlaying){KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=true;finishHistoryTransition(w);gNotice=L"历史播放已暂停；可继续播放或手动选择年份";updateHistoryPlaybackButton();InvalidateRect(w,nullptr,FALSE);return 0;}
    bool resume=gHistoryPaused&&gHistoryEventRelayMode;int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);if(!resume&&selected==0)selected=HISTORY_YEAR_COUNT-1;
    finishHistoryTransition(w);
    gShowHistory=true;if(gHistoryToggleButton){SendMessageW(gHistoryToggleButton,BM_SETCHECK,BST_CHECKED,0);EnableWindow(gHistoryToggleButton,TRUE);}if(!resume){setHistoryEventRange(selected,false);gHistoryEventRelayMode=true;appendHistoryEventsInRange(HISTORY_YEARS[selected],HISTORY_YEARS[selected]);gScrollOffset=0;gHistoryScrollPauseUntil=0;}else if(gHistoryEventCycleHeight>0)gScrollOffset%=gHistoryEventCycleHeight;
    if(!selectHistoryYear(w,selected)){gShowHistory=false;if(gHistoryToggleButton)SendMessageW(gHistoryToggleButton,BM_SETCHECK,BST_UNCHECKED,0);gHistoryPaused=false;updateHistoryPlaybackButton();return 0;}
    SendMessageW(gHistoryYearCombo,CB_SETCURSEL,selected,0);
    gHistoryPlaying=true;gHistoryPaused=false;updateHistoryPlaybackButton();gNotice=gHistoryAnimationEnabled?L"试行渐变播放：中间帧仅为视觉过渡，不代表真实疆界":L"按边界变更与事件节点播放；任意年份仍可单独选择";SetTimer(w,HISTORY_PLAYBACK_TIMER,historyPlaybackInterval(),nullptr);InvalidateRect(w,nullptr,FALSE);return 0;
  }
  if(m==WM_COMMAND&&LOWORD(a)==1016&&HIWORD(a)==BN_CLICKED){gHistorySpeedIndex=(gHistorySpeedIndex+1)%4;if(gHistorySpeedButton)setUiText(gHistorySpeedButton,HISTORY_PLAYBACK_SPEEDS[gHistorySpeedIndex]);if(gHistoryPlaying)SetTimer(w,HISTORY_PLAYBACK_TIMER,historyPlaybackInterval(),nullptr);gNotice=L"历史播放速度："+std::wstring(HISTORY_PLAYBACK_SPEEDS[gHistorySpeedIndex]).substr(3);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1020&&HIWORD(a)==BN_CLICKED){gHistoryAnimationEnabled=SendMessageW(gHistoryAnimationButton,BM_GETCHECK,0,0)==BST_CHECKED;if(!gHistoryAnimationEnabled)finishHistoryTransition(w);if(gHistoryPlaying)SetTimer(w,HISTORY_PLAYBACK_TIMER,historyPlaybackInterval(),nullptr);gNotice=gHistoryAnimationEnabled?L"试行渐变已开启：中间帧仅为视觉过渡，不代表真实疆界":L"试行渐变已关闭；播放按原年份节点切换";InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_TIMER&&a==MAP_VIEW_FRAME_TIMER){cancelMapViewPreviewTimer(w);if(gMapViewPreviewActive)requestMapViewPreviewFrame(w);return 0;}
  if(m==MAP_VIEW_FRAME_READY_MESSAGE){if(gMapViewPreviewActive)requestMapViewPreviewFrame(w);return 0;}
  if(m==WM_TIMER&&a==MAP_VIEW_SETTLE_TIMER){KillTimer(w,MAP_VIEW_SETTLE_TIMER);cancelMapViewPreviewTimer(w);gMapViewLastWheelCounter=0;if(gMapViewPreviewActive){gMapViewPreviewActive=false;gMapSceneCacheValid=false;resetMapViewPreviewCadence();InvalidateRect(w,&gMapRect,FALSE);UpdateWindow(w);}return 0;}
  if(m==WM_TIMER&&a==HISTORY_PLAYBACK_TIMER){if(!gHistoryPlaying)return 0;int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);if(selected<=0){KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;updateHistoryPlaybackButton();gNotice=L"历史播放已到最新年份公元2024年";InvalidateRect(w,nullptr,FALSE);return 0;}int next=nextHistoryPlaybackIndex(selected);int olderYear=HISTORY_YEARS[selected];Bitmap* previous=gHistoryAnimationEnabled?cloneHistoryRaster():nullptr;SendMessageW(gHistoryYearCombo,CB_SETCURSEL,next,0);setHistoryEventRange(next,true);gHistoryEventRangeStart=olderYear==-1?1:olderYear+1;gHistoryEventRangeEnd=HISTORY_YEARS[next];appendHistoryEventsInRange(gHistoryEventRangeStart,gHistoryEventRangeEnd);if(!selectHistoryYear(w,next)){delete previous;finishHistoryTransition(w);KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;updateHistoryPlaybackButton();return 0;}if(gHistoryAnimationEnabled){startHistoryTransition(w,previous);gNotice=L"试行渐变至"+historyYearText(HISTORY_YEARS[next])+L"；中间帧不代表真实疆界";}else delete previous;if(next==0){KillTimer(w,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;updateHistoryPlaybackButton();gNotice=L"历史播放已到最新年份公元2024年";InvalidateRect(w,nullptr,FALSE);}return 0;}
  if(m==WM_TIMER&&a==HISTORY_TRANSITION_TIMER){if(!gHistoryTransitionActive){KillTimer(w,HISTORY_TRANSITION_TIMER);return 0;}if(historyTransitionProgress()>=1.0f)finishHistoryTransition(w);else{gMapSceneCacheValid=false;InvalidateRect(w,&gMapRect,FALSE);}return 0;}
  if(m==WM_TIMER&&a==HISTORY_EVENT_SCROLL_TIMER){if(gInfoScrollDragging)return 0;if(gInfoTab!=1){KillTimer(w,HISTORY_EVENT_SCROLL_TIMER);return 0;}DWORD now=GetTickCount();if((LONG)(gHistoryScrollPauseUntil-now)>0)return 0;if(gHistoryEventCycleHeight>gViewportHeight&&gHistoryEventCycleHeight>0){if(gHistoryEventRelayMode&&gHistoryPlaying){int end=std::max(0,gHistoryEventCycleHeight-gViewportHeight);if(gScrollOffset<end){gScrollOffset=std::min(end,gScrollOffset+std::max(1,uiPx(1)));InvalidateRect(w,&gInfoRect,FALSE);}}else{gScrollOffset+=std::max(1,uiPx(1));if(gScrollOffset>=gHistoryEventCycleHeight)gScrollOffset=0;InvalidateRect(w,&gInfoRect,FALSE);}}return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1014&&HIWORD(a)==BN_CLICKED){gShowGridLines=SendMessageW(gGridLinesToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;gNotice=gShowGridLines?L"已显示网格线":L"已隐藏网格线；格子定位仍有效";InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1011&&HIWORD(a)==BN_CLICKED){gShowHistory=SendMessageW(gHistoryToggleButton,BM_GETCHECK,0,0)==BST_CHECKED&&gHistoryLoaded;gNotice=gShowHistory?L"已开启"+historyYearText(gHistoryYear)+L"历史疆域；该模块独立开关":L"已关闭历史填色；其他数据图层保持原状";InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_HSCROLL&&(HWND)b==gOpacityTrackbar){int opacity=(int)SendMessageW(gOpacityTrackbar,TBM_GETPOS,0,0);gFillOpacity=std::max(0,std::min(100,opacity));if(gOpacityLabel){wchar_t value[48];swprintf(value,48,L"不透明度 %d%%",gFillOpacity);setUiText(gOpacityLabel,value);}InvalidateRect(w,nullptr,FALSE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1003&&HIWORD(a)==BN_CLICKED){
    wchar_t q[256]{};GetWindowTextW(gSearchEdit,q,256);auto query=trimmedSearchQuery(q);bool grid=query.size()>1&&(query[0]==L'Z'||query[0]==L'z')&&iswdigit(query[1]);bool coordinates=!query.empty()&&(iswdigit(query[0])||query[0]==L'-'||query[0]==L'+')&&(query.find(L',')!=std::wstring::npos||query.find(L'，')!=std::wstring::npos);if(!grid&&!coordinates){showUnifiedSearch(w,query);return 0;}int row=-1,col=-1,matches=0;
    bool legacy=false;if(locateOrCycleQuery(q,row,col,matches,&legacy)){if(gSearchMatchIndex<gSearchMatches.size())selectSearchResultOnMap(gSearchMatches[gSearchMatchIndex]);else selectSearchResultOnMap(row,col);int fineRow=0,fineCol=0;fineGridCellForCoordinate(gSelectedLatitude,gSelectedLongitude,fineRow,fineCol);gNotice=legacy?L"旧编号换算至 ":L"已定位 ";gNotice+=gridIdText(activeGridCount(),fineRow,fineCol);if(gGridLevel>0)gNotice+=L" · 基础网格 "+gridIdText(GRID_VERSION,fineRow/activeGridScale(),fineCol/activeGridScale());if(matches>1)gNotice+=L" · "+std::to_wstring(gSearchMatchIndex+1)+L"/"+std::to_wstring(matches)+L"（再点查找切换）";else gNotice+=L" · 已放大并高亮";rememberView();layoutControls(w);}
    else{std::wstring missed=trimmedSearchQuery(q);if(missed.size()>24)missed=missed.substr(0,24)+L"…";gNotice=L"未找到“"+missed+L"”；试试 Z9800-019-115、39.9,116.4、城市名或土地覆盖类别";}
    InvalidateRect(w,nullptr,TRUE);return 0;
  }
  if(m==WM_COMMAND&&LOWORD(a)==1005&&HIWORD(a)==BN_CLICKED){gShowLandCover=SendMessageW(gLandCoverToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;if(gShowLandCover){gShowPopulation=false;if(gPopulationToggleButton)SendMessageW(gPopulationToggleButton,BM_SETCHECK,BST_UNCHECKED,0);}gNotice=gShowLandCover?L"已开启土地覆盖分区着色；完整类别图例显示在地图左下角":L"已关闭土地覆盖";InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1007&&HIWORD(a)==BN_CLICKED){gShowPopulation=SendMessageW(gPopulationToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;if(gShowPopulation){gShowLandCover=false;SendMessageW(gLandCoverToggleButton,BM_SETCHECK,BST_UNCHECKED,0);}gNotice=gShowPopulation?L"已开启 WorldPop 2023 人口密度估算着色；蓝→黄→红为低→高密度":L"已关闭人口密度着色";InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1009&&HIWORD(a)==BN_CLICKED){gShowWorld2026=SendMessageW(gWorld2026ToggleButton,BM_GETCHECK,0,0)==BST_CHECKED;gNotice=gShowWorld2026?L"已开启世界2026国别填色；图层独立开关，按 627,200 网格单元中心定位":L"已关闭世界2026国别填色；其他图层保持原状态";InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1001&&HIWORD(a)==BN_CLICKED){gShowControlPoints=SendMessageW(gControlPointsButton,BM_GETCHECK,0,0)==BST_CHECKED;InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_COMMAND&&LOWORD(a)==1004&&HIWORD(a)==BN_CLICKED){
    bool requested=SendMessageW(gHighResMapButton,BM_GETCHECK,0,0)==BST_CHECKED;
    if(requested&&!gMapHighRes){std::wstring base=appDir()+L"\\assets\\world-map-hd",path=base+L".jpg";Image* candidate=Image::FromFile(path.c_str());if(!candidate||candidate->GetLastStatus()!=Ok||candidate->GetWidth()<4000||candidate->GetHeight()<2000||!validateGlobalMapMetadata(base,candidate->GetWidth(),candidate->GetHeight())){delete candidate;SendMessageW(gHighResMapButton,BM_SETCHECK,BST_UNCHECKED,0);gNotice=L"高清底图或其 WGS84 世界文件校验失败";InvalidateRect(w,nullptr,TRUE);return 0;}gMapHighRes=candidate;gMapHighResOverview=loadVerifiedMapOverview(path,base+L"-overview.png","027791B611CC29EF98F3495A996995B348688C224FE062598C42C533AFDA6E24","83E1120827D20E8F3C8997626506603DD6B15A8178D90198441ED39D31B11655");}
    if(requested){SendMessageW(gNasaMapButton,BM_SETCHECK,BST_UNCHECKED,0);delete gMapNasaOverview;gMapNasaOverview=nullptr;delete gMapNasa;gMapNasa=nullptr;clearNasaDetailTileCache();gMap=gMapHighRes;gMapZoom=std::min(gMapZoom,mapZoomLimit());clampMapView();gNotice=L"已启用 Natural Earth 10m 高清底图；全球范围与 WGS84 配准校验通过";}
    else{gMap=gMapOriginal;delete gMapHighResOverview;gMapHighResOverview=nullptr;delete gMapHighRes;gMapHighRes=nullptr;gMapZoom=std::min(gMapZoom,mapZoomLimit());clampMapView();gNotice=L"已切回 4,050 × 2,025 WGS84 标准底图";}
    InvalidateRect(w,nullptr,TRUE);return 0;
  }
  if(m==WM_COMMAND&&LOWORD(a)==1006&&HIWORD(a)==BN_CLICKED){
    bool requested=SendMessageW(gNasaMapButton,BM_GETCHECK,0,0)==BST_CHECKED;
    if(requested&&!gMapNasa){std::wstring base=appDir()+L"\\assets\\world-map-nasa",path=base+L".jpg";Image* candidate=Image::FromFile(path.c_str());if(!candidate||candidate->GetLastStatus()!=Ok||candidate->GetWidth()!=21600||candidate->GetHeight()!=10800||!validateGlobalMapMetadata(base,21600,10800)){delete candidate;SendMessageW(gNasaMapButton,BM_SETCHECK,BST_UNCHECKED,0);gNotice=L"NASA 底图或 WGS84 配准文件校验失败；当前底图未更改";InvalidateRect(w,nullptr,TRUE);return 0;}gMapNasa=candidate;gMapNasaOverview=loadVerifiedMapOverview(path,base+L"-overview.png","5DBB5F101C3336F260506DCD574AABA4D5E9AF2FF8533161367098AFFEA86408","D674255082033E0178A0C6036E29C923C3AA60E38EBD5AB35465BAE87B264082");}
    if(requested){SendMessageW(gHighResMapButton,BM_SETCHECK,BST_UNCHECKED,0);delete gMapHighResOverview;gMapHighResOverview=nullptr;delete gMapHighRes;gMapHighRes=nullptr;gMap=gMapNasa;gMapZoom=std::min(gMapZoom,mapZoomLimit());clampMapView();gNotice=gNasa500mAvailable?L"NASA 底图已启用：800% 显示 1 km 细节，1200% 起可用 500 m 原始分辨率瓦片":(gNasa1kmAvailable?L"NASA 底图已启用：放大至 800% 自动显示 1 km 细节瓦片":L"已启用 NASA Blue Marble 2004-01 高清底图（21,600 × 10,800）");}
    else{gMap=gMapOriginal;delete gMapNasaOverview;gMapNasaOverview=nullptr;delete gMapNasa;gMapNasa=nullptr;clearNasaDetailTileCache();gMapZoom=std::min(gMapZoom,mapZoomLimit());clampMapView();gNotice=L"已切回 Natural Earth WGS84 标准底图";}
    InvalidateRect(w,nullptr,TRUE);return 0;
  }
  if(m==WM_LBUTTONDOWN){int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);POINT pt{x,y};if(PtInRect(&gInfoScrollTrack,pt)){
    pauseInfoAutoScroll();if(PtInRect(&gInfoScrollThumb,pt)){gInfoScrollDragging=true;gInfoScrollGrabY=y-gInfoScrollThumb.top;SetCapture(w);}else{int page=std::max(1,gViewportHeight-gViewportHeight/10);gScrollOffset=std::max(0,std::min(std::max(0,gContentHeight-gViewportHeight),gScrollOffset+(y<gInfoScrollThumb.top?-page:page)));InvalidateRect(w,&gInfoRect,FALSE);}return 0;}
    if(mapContainsPoint(x,y)){gMapDragging=true;gMapDragged=false;gDragStart=gLastMouse={x,y};SetCapture(w);}return 0;}
  if(m==WM_MOUSEMOVE){if(!gTrackingMouseLeave){TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT),TME_LEAVE,w,0};gTrackingMouseLeave=TrackMouseEvent(&track)!=FALSE;}
    int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);
    if(gInfoScrollDragging){if(a&MK_LBUTTON)dragInfoScrollbar(w,y);return 0;}
    if(gMapDragging&&(a&MK_LBUTTON)){int dx=x-gLastMouse.x,dy=y-gLastMouse.y;if(!gMapDragged&&std::abs(x-gDragStart.x)+std::abs(y-gDragStart.y)>4){RECT client{};GetClientRect(w,&client);MapSceneCacheKey current=currentMapSceneCacheKey(client.right,client.bottom);gMapDragged=true;gMapViewPreviewActive=gMapSceneCacheValid&&sameMapSceneCacheKey(current,gMapSceneCacheKey);}if(gMapDragged&&(dx||dy)){if(!gMapViewPreviewActive){RECT client{};GetClientRect(w,&client);MapSceneCacheKey current=currentMapSceneCacheKey(client.right,client.bottom);gMapViewPreviewActive=gMapSceneCacheValid&&sameMapSceneCacheKeyForViewPreview(current,gMapSceneCacheKey);}panMapBy(dx,dy);if(gMapViewPreviewActive){scheduleMapViewSettle(w,false);requestMapViewPreviewFrame(w);}else InvalidateRect(w,&gMapRect,FALSE);}gLastMouse={x,y};return 0;}
    if(!gMapDragging){std::wstring status=pointerStatus(x,y);if(!status.empty()){if(status!=gNotice){gNotice=status;gPointerStatusActive=true;invalidateNotice(w);}}else if(gPointerStatusActive){gPointerStatusActive=false;gNotice=L"地图操作：滚轮缩放 · 拖动平移 · 双击复位";invalidateNotice(w);}}
    return 0;
  }
  if(m==WM_MOUSELEAVE){gTrackingMouseLeave=false;if(gPointerStatusActive){gPointerStatusActive=false;gNotice=L"地图操作：滚轮缩放 · 拖动平移 · 双击复位";invalidateNotice(w);}return 0;}
  if(m==WM_LBUTTONUP&&gInfoScrollDragging){dragInfoScrollbar(w,GET_Y_LPARAM(b));gInfoScrollDragging=false;pauseInfoAutoScroll();if(GetCapture()==w)ReleaseCapture();return 0;}
  if(m==WM_LBUTTONUP&&gMapDragging){int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);gMapDragging=false;if(GetCapture()==w)ReleaseCapture();if(gMapDragged){RECT client{};GetClientRect(w,&client);MapSceneCacheKey current=currentMapSceneCacheKey(client.right,client.bottom);bool compatible=gMapSceneCacheValid&&sameMapSceneCacheKeyForViewPreview(current,gMapSceneCacheKey),alreadySettled=compatible&&sameMapSceneCacheKey(current,gMapSceneCacheKey);if(compatible&&!alreadySettled){gMapViewPreviewActive=true;scheduleMapViewSettle(w,false);requestMapViewPreviewFrame(w);}else{KillTimer(w,MAP_VIEW_SETTLE_TIMER);cancelMapViewPreviewTimer(w);gMapViewPreviewActive=false;resetMapViewPreviewCadence();if(!alreadySettled){gMapSceneCacheValid=false;InvalidateRect(w,&gMapRect,FALSE);}}}else{if(pointInsideRect(gLandCoverLegendRect,x,y)||pointInsideRect(gPopulationLegendRect,x,y)||pointInsideRect(gWorld2026LegendRect,x,y)||pointInsideRect(gHistoryLegendRect,x,y))return 0;if(selectMapPoint(x,y)){layoutControls(w);gNotice=L"已选中分区；滚轮缩放、拖动平移";InvalidateRect(w,nullptr,TRUE);}}return 0;}
  if(m==WM_LBUTTONDBLCLK){int x=GET_X_LPARAM(b),y=GET_Y_LPARAM(b);if(mapContainsPoint(x,y)){gMapDragging=false;if(GetCapture()==w)ReleaseCapture();if(pointInsideRect(gLandCoverLegendRect,x,y)||pointInsideRect(gPopulationLegendRect,x,y)||pointInsideRect(gWorld2026LegendRect,x,y)||pointInsideRect(gHistoryLegendRect,x,y))return 0;resetMapView();selectMapPoint(x,y);layoutControls(w);gNotice=L"地图已复位至全局视图";InvalidateRect(w,nullptr,TRUE);}return 0;}
  if(m==WM_CAPTURECHANGED){gMapDragging=false;gInfoScrollDragging=false;return 0;}
  if(m==WM_CANCELMODE&&gInfoScrollDragging){gInfoScrollDragging=false;if(GetCapture()==w)ReleaseCapture();return 0;}
  if(m==WM_MOUSEWHEEL){SetTimer(w,NAVIGATION_CAPTURE_TIMER,240,nullptr);POINT point{GET_X_LPARAM(b),GET_Y_LPARAM(b)};ScreenToClient(w,&point);if(mapContainsPoint(point.x,point.y)){RECT client{};GetClientRect(w,&client);MapSceneCacheKey current=currentMapSceneCacheKey(client.right,client.bottom);bool preview=gMapSceneCacheValid&&(sameMapSceneCacheKey(current,gMapSceneCacheKey)||(gMapViewPreviewActive&&sameMapSceneCacheKeyForViewPreview(current,gMapSceneCacheKey)));if(!preview)gMapViewPreviewActive=false;double factor=std::pow(1.2,GET_WHEEL_DELTA_WPARAM(a)/(double)WHEEL_DELTA);zoomMapAt(point.x,point.y,factor);if(preview){gMapViewPreviewActive=true;scheduleMapViewSettle(w,true);}else{gMapSceneCacheValid=false;cancelMapViewPreviewTimer(w);}if(gMap==gMapNasa&&gNasaDetailAvailable&&gMapZoom>=8.0&&selectNasaDetailLayerForZoom()){if(gNasaDetailResolutionMeters==500)gNotice=L"NASA 500 m 原始分辨率瓦片已启用 · 拖动地图浏览相邻区域";else if(gNasa1kmAvailable)gNotice=L"NASA 1 km 细节瓦片已启用 · 继续放大可查看 500 m 瓦片";else gNotice=L"NASA 500 m 原始分辨率瓦片已启用 · 拖动地图浏览相邻区域";}else{wchar_t status[48];swprintf(status,48,L"地图缩放 %.0f%%",gMapZoom*100);gNotice=status;}if(preview)requestMapViewPreviewFrame(w);else InvalidateRect(w,&gMapRect,FALSE);return 0;}if(PtInRect(&gInfoRect,point)&&(gCol>=0||gInfoTab==1)){static int lastTab=-1,lastRow=-2,lastCol=-2,lastViewport=-1;if(lastTab!=gInfoTab||lastRow!=gRow||lastCol!=gCol||lastViewport!=gViewportHeight){gInfoWheelRemainder=0;lastTab=gInfoTab;lastRow=gRow;lastCol=gCol;lastViewport=gViewportHeight;}gScrollOffset=scrollInfoPanel(gScrollOffset,gContentHeight-gViewportHeight,GET_WHEEL_DELTA_WPARAM(a),infoPanelScrollStep(gViewportHeight),gInfoWheelRemainder);if(gInfoTab==1){gHistoryScrollPauseUntil=GetTickCount()+5000;if(gHistoryEventCycleHeight>gViewportHeight&&gScrollOffset>=gHistoryEventCycleHeight)gScrollOffset%=gHistoryEventCycleHeight;}InvalidateRect(w,&gInfoRect,FALSE);return 0;}}
  if(m==WM_SIZE){if(gInfoScrollDragging){gInfoScrollDragging=false;if(GetCapture()==w)ReleaseCapture();}gInfoScrollTrack=RECT{};gInfoScrollThumb=RECT{};KillTimer(w,MAP_VIEW_SETTLE_TIMER);cancelMapViewPreviewTimer(w);gMapViewPreviewActive=false;gMapViewLastWheelCounter=0;resetMapViewPreviewCadence();gMapSceneCacheValid=false;layoutControls(w);InvalidateRect(w,nullptr,TRUE);return 0;}
  if(m==WM_DESTROY){KillTimer(w,SAVED_PLACES_WATCH_TIMER);KillTimer(w,ROUTE_PLAYBACK_TIMER);delete gRouteRaster;gRouteRaster=nullptr;KillTimer(w,HISTORY_PLAYBACK_TIMER);KillTimer(w,HISTORY_TRANSITION_TIMER);KillTimer(w,HISTORY_EVENT_SCROLL_TIMER);KillTimer(w,MAP_VIEW_SETTLE_TIMER);cancelMapViewPreviewTimer(w);gMapViewPreviewActive=false;gMapViewLastWheelCounter=0;resetMapViewPreviewCadence();gHistoryPlaying=false;delete gHistoryTransitionPreviousRaster;gHistoryTransitionPreviousRaster=nullptr;gHistoryTransitionActive=false;delete gMapSceneCache;gMapSceneCache=nullptr;gMapSceneCacheValid=false;delete gMapBaseCache;gMapBaseCache=nullptr;gMapBaseCacheValid=false;delete gMapBaseOverviewCache;gMapBaseOverviewCache=nullptr;gMapBaseOverviewCacheValid=false;releaseBackBuffer();clearUiFonts();if(gUiControlBrush)DeleteObject(gUiControlBrush);if(gUiPanelBrush)DeleteObject(gUiPanelBrush);gUiControlBrush=gUiPanelBrush=nullptr;PostQuitMessage(0);return 0;}
  return DefWindowProcW(w,m,a,b);
}
#ifndef WORLD_MAP_GRID_LOGIC_TEST
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR,int n){
  loadDisplayPreferences();gUiDpi=effectiveUiDpi(nullptr);
  StartupResources startupResources;StartupFeedback startup(h);if(!startup.step(1,gEnglish?L"Loading offline place search":L"读取离线地点搜索"))return 0;
  {DWORD error=0;std::string places;if(loadTextFile(appDir()+L"\\data\\place-search.tsv",places,error))gPlaceCatalog.parse(places,utf8);auto saved=savedPlacesPath();if(!savedPlaces::read(saved,gSavedPlaces)&&GetFileAttributesW(saved.c_str())!=INVALID_FILE_ATTRIBUTES)gSavedPlacesUnreadable=true;}
  if(!startup.step(2,gEnglish?L"Opening the map":L"准备地图"))return 0;
  GdiplusStartupInput in;GdiplusStartup(&gToken,&in,nullptr);
  gUiControlBrush=CreateSolidBrush(RGB(18,43,57));gUiPanelBrush=CreateSolidBrush(uiTheme::toolbar);
  INITCOMMONCONTROLSEX commonControls{sizeof(commonControls),ICC_BAR_CLASSES|ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&commonControls);
  std::wstring base=appDir()+L"\\assets\\world-map",image=base+L".jpg";gMapOriginal=Image::FromFile(image.c_str());if(!gMapOriginal||gMapOriginal->GetLastStatus()!=Ok||!validateGlobalMapMetadata(base,gMapOriginal?gMapOriginal->GetWidth():0,gMapOriginal?gMapOriginal->GetHeight():0)){MessageBoxW(nullptr,localizeUiText(L"无法读取或验证 WGS84 全球底图。请确认 assets\\world-map.jpg、world-map.jgw 与 world-map.prj 来自同一版本。").c_str(),localizeUiText(L"世界地图探索器").c_str(),MB_OK|MB_ICONERROR);delete gMapOriginal;gMapOriginal=nullptr;return 2;}gMap=gMapOriginal;
  std::wstring nasaBase=appDir()+L"\\assets\\world-map-nasa";gNasaMapAvailable=GetFileAttributesW((nasaBase+L".jpg").c_str())!=INVALID_FILE_ATTRIBUTES&&GetFileAttributesW((nasaBase+L".jgw").c_str())!=INVALID_FILE_ATTRIBUTES&&GetFileAttributesW((nasaBase+L".prj").c_str())!=INVALID_FILE_ATTRIBUTES;gNasa1kmAvailable=validateNasaDetailManifest(L"nasa-1km",1200,30);gNasa500mAvailable=validateNasaDetailManifest(L"nasa-500m",2400,15);gNasaDetailAvailable=gNasa1kmAvailable||gNasa500mAvailable;
  if(!startup.step(3,gEnglish?L"Reading geographic records":L"读取地理档案"))return 0;
  std::wstring data=appDir()+L"\\data\\zones.json";gArchiveLoaded=loadArchiveFile(data);if(gArchiveLoaded&&buildArchiveIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gArchiveLoaded=false;gArchiveError=ERROR_REVISION_MISMATCH;}
  if(!gArchiveLoaded){std::wstring msg=gArchiveError==ERROR_REVISION_MISMATCH?L"分区档案与此程序的 9,800 格网版本不匹配。\r\n请从同一版本完整包安装，或同步更新 WorldMapExplorer.exe 与 data\\zones.json。\r\n\r\n档案路径：\r\n"+data:L"无法读取分区档案：\r\n"+data+L"\r\nWindows 错误码："+std::to_wstring(gArchiveError);msg=localizeUiText(msg);MessageBoxW(nullptr,msg.c_str(),localizeUiText(L"世界地图探索器").c_str(),MB_OK|MB_ICONWARNING);}
  if(!startup.step(4,gEnglish?L"Reading natural and cultural records":L"读取自然与文化资料"))return 0;
  std::wstring landCoverData=appDir()+L"\\data\\land-cover-2020.json";gLandCoverLoaded=loadTextFile(landCoverData,gLandCoverArchive,gLandCoverError);if(gLandCoverLoaded){if(buildLandCoverIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gLandCoverLoaded=false;gLandCoverError=ERROR_REVISION_MISMATCH;gLandCoverArchive.clear();gLandCoverRanges.clear();}else if(!buildLandCoverRaster()){gLandCoverLoaded=false;gLandCoverError=ERROR_NOT_ENOUGH_MEMORY;gLandCoverArchive.clear();gLandCoverRanges.clear();gLandCoverZoneCodes.clear();gLandCoverZoneRgb.clear();gLandCoverLegend.clear();}}
  std::wstring heritageData=appDir()+L"\\data\\world-heritage-2026.json";gHeritageLoaded=loadTextFile(heritageData,gHeritageArchive,gHeritageError);if(gHeritageLoaded&&buildHeritageIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gHeritageLoaded=false;gHeritageError=ERROR_REVISION_MISMATCH;gHeritageArchive.clear();gHeritageRanges.clear();}
  if(!startup.step(5,gEnglish?L"Reading population records":L"读取人口资料"))return 0;
  std::wstring populationData=appDir()+L"\\data\\population-2023.json";gPopulationLoaded=loadTextFile(populationData,gPopulationArchive,gPopulationError);if(gPopulationLoaded){if(buildPopulationIndex()!=(size_t)(GRID_ROWS*GRID_COLUMNS)){gPopulationLoaded=false;gPopulationError=ERROR_REVISION_MISMATCH;gPopulationArchive.clear();gPopulationRanges.clear();gPopulationCounts.clear();gPopulationDensity.clear();gPopulationCoverage.clear();gPopulationAvailable.clear();}else if(!buildPopulationRaster()){gPopulationLoaded=false;gPopulationError=ERROR_NOT_ENOUGH_MEMORY;gPopulationArchive.clear();gPopulationRanges.clear();gPopulationCounts.clear();gPopulationDensity.clear();gPopulationCoverage.clear();gPopulationAvailable.clear();}}
  if(!startup.step(6,gEnglish?L"Preparing map layers":L"准备地图图层"))return 0;
  std::wstring world2026Data=appDir()+L"\\data\\world-2026.json";gWorld2026Loaded=loadTextFile(world2026Data,gWorld2026Archive,gWorld2026Error);if(gWorld2026Loaded){if(!buildWorld2026Index()){gWorld2026Loaded=false;gWorld2026Error=ERROR_REVISION_MISMATCH;gWorld2026Archive.clear();gWorld2026Countries.clear();gWorld2026CellCountries.clear();}else if(!buildWorld2026Raster()){gWorld2026Loaded=false;gWorld2026Error=ERROR_NOT_ENOUGH_MEMORY;gWorld2026Archive.clear();gWorld2026Countries.clear();gWorld2026CellCountries.clear();}}
  if(!startup.step(7,gEnglish?L"Reading history and journeys":L"读取历史与路线"))return 0;
  gHistoryEventsLoaded=loadHistoryEvents(appDir()+L"\\data\\world-events-v2.tsv");if(!gHistoryEventsLoaded&&gHistoryEventsError==ERROR_FILE_NOT_FOUND)gHistoryEventsLoaded=loadHistoryEvents(appDir()+L"\\data\\world-events.tsv");setHistoryEventRange(0,false);
  gHistoryVectors.open(appDir()+L"\\data\\historical-boundaries.wmh2");
  gHistoryLoaded=loadHistoryModule(HISTORY_YEARS[0]);
  if(!startup.step(8,gEnglish?L"Preparing your workspace":L"准备浏览界面"))return 0;
  WNDCLASSW c{};c.style=CS_DBLCLKS;c.hInstance=h;c.lpszClassName=L"WorldMapExplorer";c.lpfnWndProc=proc;c.hIcon=(HICON)LoadImageW(h,MAKEINTRESOURCEW(101),IMAGE_ICON,0,0,LR_DEFAULTSIZE);c.hCursor=LoadCursor(nullptr,IDC_CROSS);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);
  RECT workArea{};SystemParametersInfoW(SPI_GETWORKAREA,0,&workArea,0);int workW=std::max(640,(int)(workArea.right-workArea.left)),workH=std::max(480,(int)(workArea.bottom-workArea.top));int windowW=std::min(uiPx(1360),std::max(1,workW-uiPx(24))),windowH=std::min(uiPx(820),std::max(1,workH-uiPx(32)));int windowX=workArea.left+(workW-windowW)/2,windowY=workArea.top+(workH-windowH)/2;
  std::wstring windowTitle=localizeUiText(L"世界地图探索器");HWND w=CreateWindowExW(0,c.lpszClassName,windowTitle.c_str(),WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,windowX,windowY,windowW,windowH,nullptr,nullptr,h,nullptr);
   startupResources.window=w;
   gSearchEdit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,0,0,230,25,w,(HMENU)1002,h,nullptr);gSearchButton=CreateWindowExW(0,L"BUTTON",L"查找",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,60,25,w,(HMENU)1003,h,nullptr);gHighResMapButton=CreateWindowExW(0,L"BUTTON",L"高清底图",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,0,0,92,25,w,(HMENU)1004,h,nullptr);gControlPointsButton=CreateWindowExW(0,L"BUTTON",L"配准校验点",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,0,0,134,25,w,(HMENU)1001,h,nullptr);gGridLevelCombo=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,0,0,185,150,w,(HMENU)1008,h,nullptr);SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 9,800 格（原格）");SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 39,200 格");SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 156,800 格");SendMessageW(gGridLevelCombo,CB_ADDSTRING,0,(LPARAM)L"格网 627,200 格");SendMessageW(gGridLevelCombo,CB_SETCURSEL,0,0);gNasaMapButton=CreateWindowExW(0,L"BUTTON",L"NASA 2 km 底图",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,150,27,w,(HMENU)1006,h,nullptr);gLandCoverToggleButton=CreateWindowExW(0,L"BUTTON",L"土地覆盖",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,167,27,w,(HMENU)1005,h,nullptr);if(gPopulationLoaded&&gPopulationRaster)gPopulationToggleButton=CreateWindowExW(0,L"BUTTON",L"人口估算（2023）",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,145,27,w,(HMENU)1007,h,nullptr);if(gWorld2026Loaded&&gWorld2026Raster)gWorld2026ToggleButton=CreateWindowExW(0,L"BUTTON",L"国家与地区（2026）",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,160,27,w,(HMENU)1009,h,nullptr);
   gHistoryYearCombo=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,0,0,132,140,w,(HMENU)1010,h,nullptr);for(int i=0;i<HISTORY_YEAR_COUNT;i++){std::wstring label=historyYearText(HISTORY_YEARS[i]);SendMessageW(gHistoryYearCombo,CB_ADDSTRING,0,(LPARAM)label.c_str());}SendMessageW(gHistoryYearCombo,CB_SETCURSEL,0,0);gHistoryPlaybackButton=CreateWindowExW(0,L"BUTTON",L"从古到今播放",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,112,27,w,(HMENU)1015,h,nullptr);gHistorySpeedButton=CreateWindowExW(0,L"BUTTON",L"速度 1×",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,82,27,w,(HMENU)1016,h,nullptr);gHistoryAnimationButton=CreateWindowExW(0,L"BUTTON",L"渐变动画试行",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,112,27,w,(HMENU)1020,h,nullptr);gHistoryToggleButton=CreateWindowExW(0,L"BUTTON",L"历史疆域",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,142,27,w,(HMENU)1011,h,nullptr);gOpacityLabel=CreateWindowExW(0,L"STATIC",L"不透明度 72%",WS_CHILD|WS_VISIBLE|SS_LEFT,0,0,118,22,w,(HMENU)1013,h,nullptr);gOpacityTrackbar=CreateWindowExW(0,TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|TBS_HORZ|TBS_NOTICKS,0,0,184,30,w,(HMENU)1012,h,nullptr);SendMessageW(gOpacityTrackbar,TBM_SETRANGE,TRUE,MAKELONG(0,100));SendMessageW(gOpacityTrackbar,TBM_SETPOS,TRUE,gFillOpacity);gGridLinesToggleButton=CreateWindowExW(0,L"BUTTON",L"显示网格线",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE,0,0,132,27,w,(HMENU)1014,h,nullptr);SendMessageW(gGridLinesToggleButton,BM_SETCHECK,BST_CHECKED,0);gGeoTabButton=CreateWindowExW(0,L"BUTTON",L"区域信息",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,159,32,w,(HMENU)1018,h,nullptr);gEventsTabButton=CreateWindowExW(0,L"BUTTON",L"历史事件",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,159,32,w,(HMENU)1019,h,nullptr);
   gFullscreenButton=CreateWindowExW(0,L"BUTTON",L"地图全屏",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,94,29,w,(HMENU)1023,h,nullptr);gLanguageButton=CreateWindowExW(0,L"BUTTON",languageButtonCaption(),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,0,76,27,w,(HMENU)1024,h,nullptr);gMapPreviewRateCombo=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,0,0,136,145,w,(HMENU)1025,h,nullptr);for(int mode=MAP_PREVIEW_RATE_30;mode<=MAP_PREVIEW_RATE_ADAPTIVE;mode++){std::wstring label=mapViewPreviewRateText((MapPreviewRateMode)mode);SendMessageW(gMapPreviewRateCombo,CB_ADDSTRING,0,(LPARAM)label.c_str());}SendMessageW(gMapPreviewRateCombo,CB_SETCURSEL,(WPARAM)gMapViewPreviewRateMode,0);const HWND styledButtons[]={gControlPointsButton,gSearchButton,gHighResMapButton,gNasaMapButton,gLandCoverToggleButton,gPopulationToggleButton,gWorld2026ToggleButton,gHistoryToggleButton,gGridLinesToggleButton,gHistoryPlaybackButton,gHistorySpeedButton,gHistoryAnimationButton,gGeoTabButton,gEventsTabButton,gFullscreenButton,gLanguageButton};for(HWND button:styledButtons)if(button){LONG_PTR style=GetWindowLongPtrW(button,GWL_STYLE);SetWindowLongPtrW(button,GWL_STYLE,(style&~BS_TYPEMASK)|BS_OWNERDRAW|BS_NOTIFY);SetWindowSubclass(button,styledButtonSubclass,(UINT_PTR)GetDlgCtrlID(button),0);}
   gRoutePanelButton=CreateWindowExW(0,L"BUTTON",L"路线工具",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,108,27,w,(HMENU)1046,h,nullptr);
   SetWindowSubclass(gRoutePanelButton,styledButtonSubclass,1046,0);
   gYearPickerButton=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,132,27,w,(HMENU)1047,h,nullptr);
   gRoutePickerButton=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,260,27,w,(HMENU)1048,h,nullptr);
   gExportButton=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,240,32,w,(HMENU)1049,h,nullptr);
   gZoomInButton=CreateWindowExW(0,L"BUTTON",L"+",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,33,30,w,(HMENU)1050,h,nullptr);
   gZoomOutButton=CreateWindowExW(0,L"BUTTON",L"−",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,33,30,w,(HMENU)1051,h,nullptr);
   gResetMapButton=CreateWindowExW(0,L"BUTTON",L"↺",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,33,30,w,(HMENU)1052,h,nullptr);
   gHelpButton=CreateWindowExW(0,L"BUTTON",L"?",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,32,28,w,(HMENU)1053,h,nullptr);
   for(HWND button:{gZoomInButton,gZoomOutButton,gResetMapButton,gHelpButton})SetWindowSubclass(button,styledButtonSubclass,(UINT_PTR)GetDlgCtrlID(button),0);
   SetWindowSubclass(gYearPickerButton,styledButtonSubclass,1047,0);SetWindowSubclass(gRoutePickerButton,styledButtonSubclass,1048,0);
   SetWindowSubclass(gExportButton,styledButtonSubclass,1049,0);
   gRouteCombo=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,0,0,300,360,w,(HMENU)1041,h,nullptr);
   gRouteButton=CreateWindowExW(0,L"BUTTON",L"播放 / 重播",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,102,27,w,(HMENU)1040,h,nullptr);
   gRouteHideButton=CreateWindowExW(0,L"BUTTON",L"关闭路线",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,82,27,w,(HMENU)1043,h,nullptr);
   gRouteInfoButton=CreateWindowExW(0,L"BUTTON",L"路线说明",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,90,27,w,(HMENU)1044,h,nullptr);
   gRouteFitButton=CreateWindowExW(0,L"BUTTON",L"定位路线",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY,0,0,90,27,w,(HMENU)1045,h,nullptr);
   const HWND routeButtons[]={gRouteButton,gRouteHideButton,gRouteInfoButton,gRouteFitButton};for(HWND button:routeButtons)if(button)SetWindowSubclass(button,styledButtonSubclass,(UINT_PTR)GetDlgCtrlID(button),0);
   for(HWND control:{gSearchEdit,gGridLevelCombo,gMapPreviewRateCombo,gOpacityLabel})if(control)SendMessageW(control,WM_SETFONT,(WPARAM)uiFontFor(13,false),TRUE);enableMainKeyboardStops(w);
   for(int i=0;i<4;++i){gRegionPageButtons[i]=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,70,25,w,(HMENU)(INT_PTR)(1061+i),h,nullptr);SetWindowSubclass(gRegionPageButtons[i],styledButtonSubclass,1061+i,0);}
   gEventSearchButton=CreateWindowExW(0,L"BUTTON",L"检索…",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,64,32,w,(HMENU)1066,h,nullptr);SetWindowSubclass(gEventSearchButton,styledButtonSubclass,1066,0);
    for(int i=0;i<3;++i){gWorkspaceButtons[i]=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,70,32,w,(HMENU)(INT_PTR)(1070+i),h,nullptr);SetWindowSubclass(gWorkspaceButtons[i],styledButtonSubclass,1070+i,0);}
    struct NewButton{HWND* handle;int id;};for(auto item:std::initializer_list<NewButton>{{&gBasemapMenuButton,1073},{&gLayersMenuButton,1074},{&gDisplayMenuButton,1075},{&gBackButton,1076},{&gForwardButton,1077},{&gPreviousYearButton,1078},{&gNextYearButton,1079},{&gSavedPlacesButton,1082},{&gBookmarkButton,1083},{&gStartFreshButton,1084},{&gRestoreSessionButton,1085}}){*item.handle=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,70,30,w,(HMENU)(INT_PTR)item.id,h,nullptr);SetWindowSubclass(*item.handle,styledButtonSubclass,item.id,0);}
    for(int i=0;i<6;++i){gRecommendedButtons[i]=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,80,30,w,(HMENU)(INT_PTR)(1140+i),h,nullptr);SetWindowSubclass(gRecommendedButtons[i],styledButtonSubclass,1140+i,0);}
    gHistoryScrub=CreateWindowExW(0,TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,0,0,200,30,w,(HMENU)1080,h,nullptr);SendMessageW(gHistoryScrub,TBM_SETRANGEMIN,FALSE,0);SendMessageW(gHistoryScrub,TBM_SETRANGEMAX,FALSE,HISTORY_YEAR_COUNT-1);
    gRouteScrub=CreateWindowExW(0,TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,0,0,200,30,w,(HMENU)1081,h,nullptr);SendMessageW(gRouteScrub,TBM_SETRANGEMIN,FALSE,0);SendMessageW(gRouteScrub,TBM_SETRANGEMAX,FALSE,10000);
    gRegionRawButton=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|BS_OWNERDRAW|BS_NOTIFY|WS_TABSTOP,0,0,200,26,w,(HMENU)1065,h,nullptr);SetWindowSubclass(gRegionRawButton,styledButtonSubclass,1065,0);
   gUiScaleCombo=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST,0,0,1,1,w,(HMENU)1086,h,nullptr);SendMessageW(gUiScaleCombo,WM_SETFONT,(WPARAM)uiFontFor(13,false),FALSE);populateUiScaleChoices(w);
   SetWindowSubclass(gSearchEdit,searchEditSubclass,1002,0);
   SendMessageW(gControlPointsButton,BM_SETCHECK,gShowControlPoints?BST_CHECKED:BST_UNCHECKED,0);SendMessageW(gGridLevelCombo,CB_SETCURSEL,gGridLevel,0);refreshUiLanguage(w);layoutControls(w);createControlHints(w);gPreferencesReady=true;if(!startup.step(9,gEnglish?L"Restoring your view":L"恢复浏览位置"))return 0;restoreLastView(w);gSessionReady=true;if(!startup.step(9,gEnglish?L"Ready":L"已准备就绪"))return 0;gViewHistory.reset(captureExperienceView());syncExperienceControls();startup.close();startupResources.completed=true;SetTimer(w,SAVED_PLACES_WATCH_TIMER,1000,nullptr);ShowWindow(w,n);
  gMapViewPreviewWaitableTimer=createMapViewPreviewWaitableTimer();MSG msg{};bool quit=false;auto dispatchQueuedMessage=[&](MSG& queued){UiDpiScope dpi(queued.hwnd);if(gUnifiedSearchWindow&&(queued.hwnd==gUnifiedSearchWindow||IsChild(gUnifiedSearchWindow,queued.hwnd))){if(queued.message==WM_KEYDOWN){if(queued.wParam==VK_ESCAPE){SendMessageW(gUnifiedSearchWindow,WM_CLOSE,0,0);return;}if(queued.wParam==VK_RETURN&&(GetFocus()==GetDlgItem(gUnifiedSearchWindow,2301)||GetFocus()==GetDlgItem(gUnifiedSearchWindow,2303))){if(GetFocus()==GetDlgItem(gUnifiedSearchWindow,2301))refreshExploreResults(gUnifiedSearchWindow);openExploreResult(gUnifiedSearchWindow);return;}if((queued.wParam==VK_DOWN||queued.wParam==VK_UP)&&GetFocus()==GetDlgItem(gUnifiedSearchWindow,2301)){SetFocus(GetDlgItem(gUnifiedSearchWindow,2303));return;}}if(IsDialogMessageW(gUnifiedSearchWindow,&queued))return;}if(gEventSearchWindow&&(queued.hwnd==gEventSearchWindow||IsChild(gEventSearchWindow,queued.hwnd))){if(queued.message==WM_KEYDOWN&&queued.wParam=='F'&&(GetKeyState(VK_CONTROL)&0x8000)){SetFocus(GetDlgItem(gEventSearchWindow,2101));return;}if(IsDialogMessageW(gEventSearchWindow,&queued))return;}if((queued.hwnd==w||IsChild(w,queued.hwnd))&&(queued.message==WM_KEYDOWN||queued.message==WM_SYSKEYDOWN)){if(queued.wParam==VK_F11){setMapFullscreen(w,!gMapFullscreen);return;}if(queued.wParam==VK_ESCAPE&&gMapFullscreen){setMapFullscreen(w,false);return;}if(handleMainKeyboard(w,queued))return;}
    if((queued.message==WM_KEYDOWN||queued.message==WM_SYSKEYDOWN)&&queued.hwnd){HWND root=GetAncestor(queued.hwnd,GA_ROOT);wchar_t name[64]{};if(root&&root!=w&&GetWindow(root,GW_OWNER)==w&&GetClassNameW(root,name,64)&&wcscmp(name,L"WorldMapExplorerRouteNotes")==0&&queued.message==WM_KEYDOWN){
      if(queued.wParam==VK_ESCAPE){SendMessageW(root,WM_CLOSE,0,0);return;}
      HWND body=GetDlgItem(root,1),close=GetDlgItem(root,2),focus=GetFocus();bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,alt=(GetKeyState(VK_MENU)&0x8000)!=0;
      if(queued.wParam==VK_TAB&&!ctrl&&!alt){HWND target=focus==body?close:body;if(target&&IsWindowEnabled(target)){SendMessageW(root,WM_CHANGEUISTATE,MAKEWPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);SetFocus(target);return;}}
      if(queued.wParam==VK_RETURN&&focus==close&&IsWindowEnabled(close)){SendMessageW(close,BM_CLICK,0,0);return;}
    }}
    TranslateMessage(&queued);DispatchMessageW(&queued);};
  while(!quit){DWORD wait=MsgWaitForMultipleObjectsEx(gMapViewPreviewWaitableTimer?1:0,gMapViewPreviewWaitableTimer?&gMapViewPreviewWaitableTimer:nullptr,INFINITE,QS_ALLINPUT,MWMO_INPUTAVAILABLE);if(wait==WAIT_OBJECT_0&&gMapViewPreviewWaitableTimer){if(gMapViewPreviewTimerScheduled&&gMapViewPreviewTimerWaitable){gMapViewPreviewTimerScheduled=false;gMapViewPreviewTimerWaitable=false;PostMessageW(w,MAP_VIEW_FRAME_READY_MESSAGE,0,0);}continue;}if(wait==WAIT_FAILED){BOOL received=GetMessageW(&msg,nullptr,0,0);if(received<=0)break;dispatchQueuedMessage(msg);continue;}DWORD messageQueueIndex=WAIT_OBJECT_0+(gMapViewPreviewWaitableTimer?1:0);if(wait!=messageQueueIndex)continue;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){quit=true;break;}dispatchQueuedMessage(msg);}}
  if(gMapViewPreviewWaitableTimer){CloseHandle(gMapViewPreviewWaitableTimer);gMapViewPreviewWaitableTimer=nullptr;}
    gHistoryVectors.close();gHistorySnapshotCache.clear();delete gLandCoverRaster;delete gPopulationRaster;delete gWorld2026Raster;delete gHistoryRaster;clearNasaDetailTileCache();delete gMapNasaOverview;delete gMapHighResOverview;delete gMapNasa;delete gMapHighRes;delete gMapOriginal;return 0;
}
#endif

