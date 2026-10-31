#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static const int GRID_COLUMNS=140,GRID_ROWS=70,GRID_VERSION=9800;
static const double CELL=180.0/GRID_ROWS;

struct TiffField{uint16_t tag=0,type=0;uint64_t count=0,offset=0;unsigned char inlineValue[8]{};};

class GeoTiff{
 public:
  explicit GeoTiff(const std::filesystem::path& path):file(path,std::ios::binary),little(false),bigTiff(false),width(0),height(0),tileWidth(0),tileHeight(0),compression(0),bitsPerSample(0),samplesPerPixel(0),sampleFormat(0),planarConfiguration(0),predictor(1),scaleX(0),scaleY(0),originX(0),originY(0),pixelIsPoint(false),nodata(-1e30),hasNodata(false){}

  bool open(){
    if(!file)return fail(L"Cannot open GeoTIFF.");file.seekg(0,std::ios::end);std::streamoff length=file.tellg();if(length<16)return fail(L"GeoTIFF is too short.");fileSize=(uint64_t)length;
    unsigned char head[16]{};if(!readAt(0,head,sizeof(head)))return fail(L"GeoTIFF header is truncated.");
    little=head[0]=='I'&&head[1]=='I';if(!little&&!(head[0]=='M'&&head[1]=='M'))return fail(L"Unknown TIFF byte order.");uint16_t version=u16(head+2);uint64_t ifd=0,entryCount=0,ifdStart=0;
    if(version==42){ifd=u32(head+4);unsigned char nbuf[2]{};if(!readAt(ifd,nbuf,2))return fail(L"Missing TIFF directory.");entryCount=u16(nbuf);ifdStart=ifd+2;}
    else if(version==43){if(u16(head+4)!=8||u16(head+6)!=0)return fail(L"Unsupported BigTIFF offset size.");bigTiff=true;ifd=u64(head+8);unsigned char nbuf[8]{};if(!readAt(ifd,nbuf,8))return fail(L"Missing BigTIFF directory.");entryCount=u64(nbuf);ifdStart=ifd+8;}
    else return fail(L"Unsupported TIFF header.");
    if(entryCount>4096)return fail(L"Too many TIFF directory entries.");const size_t entryBytes=bigTiff?20:12;
    for(uint64_t i=0;i<entryCount;i++){
      unsigned char e[20]{};if(!readAt(ifdStart+i*entryBytes,e,entryBytes))return fail(L"Truncated TIFF directory.");TiffField f;f.tag=u16(e);f.type=u16(e+2);
      if(bigTiff){f.count=u64(e+4);std::memcpy(f.inlineValue,e+12,8);f.offset=f.count*(uint64_t)typeSize(f.type)<=8?ifdStart+i*20+12:u64(e+12);}
      else{f.count=u32(e+4);std::memcpy(f.inlineValue,e+8,4);f.offset=f.count*(uint64_t)typeSize(f.type)<=4?ifdStart+i*12+8:u32(e+8);}
      fields.push_back(f);
    }
    if(!getUnsigned(256,width)||!getUnsigned(257,height)||width<1000||height<1000)return fail(L"Unexpected GeoTIFF dimensions.");
    if(!getUnsigned(258,bitsPerSample)||!getUnsigned(259,compression)||!getUnsigned(277,samplesPerPixel)||!getUnsigned(284,planarConfiguration)||!getUnsigned(339,sampleFormat))return fail(L"Missing sample format tags.");
    getUnsigned(317,predictor);
    if(bitsPerSample!=32||compression!=5||samplesPerPixel!=1||planarConfiguration!=1||sampleFormat!=3||predictor!=1)return fail(L"Only tiled, single-band, 32-bit float TIFF LZW with no predictor is supported.");
    if(!getUnsigned(322,tileWidth)||!getUnsigned(323,tileHeight)||tileWidth==0||tileHeight==0)return fail(L"Missing TIFF tile dimensions.");
    std::vector<double> scale,tie;if(!getDoubles(33550,scale)||scale.size()<2||!getDoubles(33922,tie)||tie.size()<6)return fail(L"Missing GeoTIFF pixel scale/tiepoint.");
    scaleX=scale[0];scaleY=scale[1];originX=tie[3]-tie[0]*scaleX;originY=tie[4]+tie[1]*scaleY;if(!(scaleX>0&&scaleY>0))return fail(L"Invalid GeoTIFF pixel scale.");
    std::vector<uint16_t> keys;if(getShorts(34735,keys)&&keys.size()>=4){uint16_t keyCount=keys[3];for(size_t i=0;i<keyCount&&4+i*4+3<keys.size();i++)if(keys[4+i*4]==1025&&keys[4+i*4+1]==0&&keys[4+i*4+2]==1)pixelIsPoint=keys[4+i*4+3]==2;}
    std::string nodataText;if(getAscii(42113,nodataText)){char* end=nullptr;double v=std::strtod(nodataText.c_str(),&end);if(end!=nodataText.c_str()&&std::isfinite(v)){nodata=v;hasNodata=true;}}
    if(!readUnsignedArray(324,tileOffsets)||!readUnsignedArray(325,tileByteCounts)||tileOffsets.size()!=tileByteCounts.size())return fail(L"Missing TIFF tile offset/byte-count arrays.");
    uint64_t across=(width+tileWidth-1)/tileWidth,down=(height+tileHeight-1)/tileHeight;if(across*down!=tileOffsets.size())return fail(L"TIFF tile index count does not match image dimensions.");
    for(size_t i=0;i<tileOffsets.size();i++)if(tileByteCounts[i]==0||tileByteCounts[i]>tileWidth*(uint64_t)tileHeight*8||tileOffsets[i]>fileSize||tileByteCounts[i]>fileSize-tileOffsets[i])return fail(L"Invalid TIFF tile byte range.");
    double east=originX+width*scaleX,north=originY,south=originY-height*scaleY;
    std::wcout<<L"GeoTIFF: "<<width<<L" × "<<height<<L", extent "<<std::fixed<<std::setprecision(4)<<originX<<L".."<<east<<L" lon, "<<south<<L".."<<north<<L" lat, pixel "<<scaleX<<L"° × "<<scaleY<<L"°.\n";
    std::wcout<<L"Tiled LZW BigTIFF: "<<tileWidth<<L" × "<<tileHeight<<L"; nodata="<<(hasNodata?nodata:0)<<L".\n";
    if(std::fabs(originX+180)>0.02||std::fabs(east-180)>0.02||north>90||south< -90||north-south<100)return fail(L"GeoTIFF extent is not a global WGS84 grid.");return true;
  }

  bool readTile(size_t index,std::vector<unsigned char>& bytes)const{
    if(index>=tileOffsets.size()||tileByteCounts[index]>SIZE_MAX)return false;bytes.resize((size_t)tileByteCounts[index]);return readAt(tileOffsets[index],bytes.data(),bytes.size());
  }
  std::wstring error;mutable std::ifstream file;bool little,bigTiff;uint32_t width,height,tileWidth,tileHeight,compression,bitsPerSample,samplesPerPixel,sampleFormat,planarConfiguration,predictor;double scaleX,scaleY,originX,originY;bool pixelIsPoint;double nodata;bool hasNodata;uint64_t fileSize=0;std::vector<uint64_t> tileOffsets,tileByteCounts;

 private:
  std::vector<TiffField> fields;
  bool fail(const wchar_t* message){error=message;return false;}
  uint16_t u16(const unsigned char* p)const{return little?(uint16_t)(p[0]|(p[1]<<8)):(uint16_t)((p[0]<<8)|p[1]);}
  uint32_t u32(const unsigned char* p)const{return little?(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24):((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3];}
  uint64_t u64(const unsigned char* p)const{uint64_t v=0;if(little)for(int i=7;i>=0;i--)v=(v<<8)|p[i];else for(int i=0;i<8;i++)v=(v<<8)|p[i];return v;}
  int typeSize(uint16_t t)const{switch(t){case 1:case 2:case 6:case 7:return 1;case 3:case 8:return 2;case 4:case 9:case 11:case 5:case 10:return 4;case 12:case 16:case 17:case 18:return 8;default:return 0;}}
  const TiffField* find(uint16_t tag)const{for(const auto& f:fields)if(f.tag==tag)return &f;return nullptr;}
  bool readAt(uint64_t offset,void* out,size_t bytes)const{if(offset>fileSize||bytes>fileSize-offset)return false;file.clear();file.seekg((std::streamoff)offset);file.read((char*)out,(std::streamsize)bytes);return (size_t)file.gcount()==bytes;}
  bool readValues(const TiffField& f,unsigned char* out,size_t bytes)const{size_t inlined=bigTiff?8:4;if(bytes>inlined)return readAt(f.offset,out,bytes);std::memcpy(out,f.inlineValue,bytes);return true;}
  bool getUnsigned(uint16_t tag,uint32_t& value)const{const TiffField* f=find(tag);if(!f||f->count<1)return false;int sz=typeSize(f->type);if(sz!=2&&sz!=4&&sz!=8)return false;unsigned char b[8]{};if(!readValues(*f,b,(size_t)sz))return false;uint64_t n=sz==2?u16(b):(sz==4?u32(b):u64(b));if(n>UINT32_MAX)return false;value=(uint32_t)n;return true;}
  bool getDoubles(uint16_t tag,std::vector<double>& values)const{const TiffField* f=find(tag);if(!f||f->type!=12||f->count>32)return false;size_t n=(size_t)f->count*8;std::vector<unsigned char>b(n);if(!readValues(*f,b.data(),n))return false;for(size_t i=0;i<f->count;i++){uint64_t bits=u64(&b[i*8]);double v;std::memcpy(&v,&bits,8);values.push_back(v);}return true;}
  bool getShorts(uint16_t tag,std::vector<uint16_t>& values)const{const TiffField* f=find(tag);if(!f||f->type!=3||f->count>4096)return false;size_t n=(size_t)f->count*2;std::vector<unsigned char>b(n);if(!readValues(*f,b.data(),n))return false;for(size_t i=0;i<f->count;i++)values.push_back(u16(&b[i*2]));return true;}
  bool getAscii(uint16_t tag,std::string& value)const{const TiffField* f=find(tag);if(!f||f->type!=2||!f->count||f->count>256)return false;std::vector<unsigned char>b((size_t)f->count);if(!readValues(*f,b.data(),b.size()))return false;value.assign((char*)b.data(),b.size());return true;}
  bool readUnsignedArray(uint16_t tag,std::vector<uint64_t>& values)const{const TiffField* f=find(tag);if(!f||f->count==0||f->count>10000000)return false;int sz=typeSize(f->type);if(sz!=2&&sz!=4&&sz!=8)return false;size_t n=(size_t)f->count*(size_t)sz;std::vector<unsigned char>b(n);if(!readValues(*f,b.data(),n))return false;values.reserve((size_t)f->count);for(size_t i=0;i<f->count;i++){const unsigned char* p=&b[i*(size_t)sz];values.push_back(sz==2?u16(p):(sz==4?u32(p):u64(p)));}return true;}
};

class BitReader{
 public:
  explicit BitReader(const std::vector<unsigned char>& bytes):data(bytes),position(0),buffer(0),available(0){}
  bool read(int width,uint16_t& code){while(available<width){if(position>=data.size())return false;buffer=(buffer<<8)|data[position++];available+=8;}available-=width;code=(uint16_t)((buffer>>available)&((1u<<width)-1));return true;}
 private:
  const std::vector<unsigned char>& data;size_t position;uint64_t buffer;int available;
};

static bool decodeTiffLzw(const std::vector<unsigned char>& input,size_t expected,std::vector<unsigned char>& output){
  std::array<uint16_t,4096> prefix{};std::array<unsigned char,4096> suffix{};std::array<unsigned char,4097> stack{};BitReader bits(input);output.clear();output.reserve(expected);int codeWidth=9,nextCode=258,previous=-1;unsigned char previousFirst=0;bool sawEnd=false;uint16_t code=0;
  while(bits.read(codeWidth,code)){
    if(code==256){codeWidth=9;nextCode=258;previous=-1;continue;}if(code==257){sawEnd=true;break;}if(code>nextCode||code>=4096)return false;
    uint16_t inputCode=code;int count=0;if(code==nextCode){if(previous<0)return false;stack[count++]=previousFirst;code=(uint16_t)previous;}
    while(code>=256){if(code>=nextCode||count>=4096)return false;stack[count++]=suffix[code];code=prefix[code];}
    if(count>=4096)return false;stack[count++]=(unsigned char)code;unsigned char first=stack[count-1];
    if(output.size()+(size_t)count>expected)return false;while(count>0)output.push_back(stack[--count]);
    if(previous>=0&&nextCode<4096){prefix[nextCode]=(uint16_t)previous;suffix[nextCode]=first;nextCode++;if(codeWidth<12&&nextCode==((1<<codeWidth)-1))codeWidth++;}
    previous=inputCode;previousFirst=first;
  }
  return sawEnd&&output.size()==expected;
}

static bool parseDoubleField(const std::string& s,size_t begin,size_t end,const char* key,double& value){
  std::string needle=std::string("\"")+key+"\"";size_t p=s.find(needle,begin);if(p==std::string::npos||p>=end)return false;p=s.find(':',p+needle.size());if(p==std::string::npos||p>=end)return false;p++;while(p<end&&isspace((unsigned char)s[p]))p++;char* parsedEnd=nullptr;double v=std::strtod(s.c_str()+p,&parsedEnd);if(parsedEnd==s.c_str()+p||(size_t)(parsedEnd-s.c_str())>end||!std::isfinite(v))return false;value=v;return true;
}
static bool parseStringField(const std::string& s,size_t begin,size_t end,const char* key,std::string& value){
  std::string needle=std::string("\"")+key+"\"";size_t p=s.find(needle,begin);if(p==std::string::npos||p>=end)return false;p=s.find(':',p+needle.size());if(p==std::string::npos||p>=end)return false;p++;while(p<end&&isspace((unsigned char)s[p]))p++;if(p>=end||s[p++]!='\"')return false;size_t q=s.find('\"',p);if(q==std::string::npos||q>=end)return false;value=s.substr(p,q-p);return true;
}
static bool loadLandAreas(const std::filesystem::path& path,std::vector<double>& areas){
  std::ifstream in(path,std::ios::binary);if(!in)return false;std::string data((std::istreambuf_iterator<char>(in)),{});if(data.empty()||data.size()>64*1024*1024)return false;
  areas.assign((size_t)GRID_ROWS*GRID_COLUMNS,0);std::vector<unsigned char> seen(areas.size(),0);size_t zones=data.find("\"zones\"");if(zones==std::string::npos)return false;size_t array=data.find('[',zones);if(array==std::string::npos)return false;size_t p=data.find("\"zoneId\"",array);int indexed=0;
  while(p!=std::string::npos){size_t next=data.find("\"zoneId\"",p+8);size_t end=next==std::string::npos?data.size():next;std::string id;if(!parseStringField(data,p,end,"zoneId",id))return false;int version=-1,row=-1,col=-1;if(std::sscanf(id.c_str(),"Z%d-%d-%d",&version,&row,&col)!=3||version!=GRID_VERSION||row<0||row>=GRID_ROWS||col<0||col>=GRID_COLUMNS)return false;size_t i=(size_t)row*GRID_COLUMNS+col;double area=0;if(!parseDoubleField(data,p,end,"landAreaKm2",area)||area<0)return false;if(seen[i])return false;areas[i]=area;seen[i]=1;indexed++;p=next;}
  return indexed==GRID_ROWS*GRID_COLUMNS;
}
static void jsonNumber(std::ostream& out,double v){if(!std::isfinite(v))v=0;out<<std::fixed<<std::setprecision(6)<<v;}

int wmain(int argc,wchar_t** argv){
  if(argc!=4){std::wcerr<<L"Usage: aggregate-worldpop.exe <WorldPop TIFF> <zones.json> <output.json>\n";return 2;}
  GeoTiff geo{std::filesystem::path(argv[1])};if(!geo.open()){std::wcerr<<geo.error<<L"\n";return 3;}
  std::vector<double> landArea;if(!loadLandAreas(std::filesystem::path(argv[2]),landArea)){std::wcerr<<L"Could not read land areas from the zones array in zones.json.\n";return 4;}
  const size_t tilePixels=(size_t)geo.tileWidth*geo.tileHeight,expectedBytes=tilePixels*4;std::vector<double> sums((size_t)GRID_ROWS*GRID_COLUMNS,0);std::vector<unsigned char> compressed,pixels;uint64_t validPixels=0,positivePixels=0;double total=0;uint32_t tilesAcross=(geo.width+geo.tileWidth-1)/geo.tileWidth;
  for(size_t tile=0;tile<geo.tileOffsets.size();tile++){
    if(!geo.readTile(tile,compressed)||!decodeTiffLzw(compressed,expectedBytes,pixels)){std::wcerr<<L"LZW decode failed at tile "<<tile<<L" ("<<compressed.size()<<L" compressed bytes).\n";return 5;}
    uint32_t tileRow=(uint32_t)(tile/tilesAcross),tileCol=(uint32_t)(tile%tilesAcross);for(uint32_t y=0;y<geo.tileHeight;y++){uint32_t row=tileRow*geo.tileHeight+y;if(row>=geo.height)break;double lat=geo.originY-(row+(geo.pixelIsPoint?0.0:0.5))*geo.scaleY;if(lat< -90||lat>90)continue;int zr=(int)std::floor((90.0-lat)/CELL);if(zr<0||zr>=GRID_ROWS)continue;const unsigned char* line=pixels.data()+(size_t)y*geo.tileWidth*4;
      for(uint32_t x=0;x<geo.tileWidth;x++){uint32_t col=tileCol*geo.tileWidth+x;if(col>=geo.width)break;double lon=geo.originX+(col+(geo.pixelIsPoint?0.0:0.5))*geo.scaleX;if(lon< -180||lon>=180)continue;int zc=(int)std::floor((lon+180.0)/(360.0/GRID_COLUMNS));if(zc<0||zc>=GRID_COLUMNS)continue;float value;std::memcpy(&value,line+(size_t)x*4,4);double pop=value;if(!std::isfinite(pop)||pop<0||(geo.hasNodata&&std::fabs(pop-geo.nodata)<std::max(1e-6,std::fabs(geo.nodata)*1e-6)))continue;validPixels++;if(pop<=0)continue;size_t index=(size_t)zr*GRID_COLUMNS+zc;sums[index]+=pop;total+=pop;positivePixels++;}
    }
    if((tile+1)%100==0||tile+1==geo.tileOffsets.size())std::wcerr<<L"Decoded "<<tile+1<<L" / "<<geo.tileOffsets.size()<<L" TIFF tiles.\r";
  }
  std::wcerr<<L"\n";if(total<5000000000.0||total>12000000000.0){std::wcerr<<L"Population total outside expected global range: "<<std::fixed<<total<<L"\n";return 6;}
  double north=geo.originY+(geo.pixelIsPoint?geo.scaleY*.5:0),south=geo.originY-geo.height*geo.scaleY+(geo.pixelIsPoint?geo.scaleY*.5:0);size_t zoneCount=(size_t)GRID_ROWS*GRID_COLUMNS;std::vector<double> zoneCounts(zoneCount,0),zoneDensity(zoneCount,0),zoneCoverage(zoneCount,0);std::vector<unsigned char> available(zoneCount,0);
  for(int row=0;row<GRID_ROWS;row++){double zNorth=90-row*CELL,zSouth=zNorth-CELL;double overlap=std::max(0.0,std::min(zNorth,north)-std::max(zSouth,south));double coverage=std::max(0.0,std::min(100.0,overlap/CELL*100.0));for(int col=0;col<GRID_COLUMNS;col++){size_t i=(size_t)row*GRID_COLUMNS+col;zoneCounts[i]=std::round(sums[i]);zoneCoverage[i]=coverage;available[i]=coverage>0?1:0;if(landArea[i]>0)zoneDensity[i]=zoneCounts[i]/landArea[i];}}
  std::ofstream out(std::filesystem::path(argv[3]),std::ios::binary|std::ios::trunc);if(!out){std::wcerr<<L"Cannot write output JSON.\n";return 7;}
  out<<"{\n  \"schemaVersion\": 1,\n  \"gridVersion\": \"Z9800\",\n  \"zoneCount\": 9800,\n  \"year\": 2023,\n  \"resolutionArcSeconds\": 30,\n  \"dataset\": \"Global2 R2025A v1\",\n  \"license\": \"CC BY 4.0\",\n  \"attribution\": \"WorldPop, University of Southampton\",\n  \"sourceUrl\": \"https://data.worldpop.org/GIS/Population/Global_2015_2030/R2025A/2023/0_Mosaicked/v1/1km_ua/constrained/global_pop_2023_CN_1km_R2025A_UA_v1.tif\",\n  \"sourceLatitudeNorth\": ";jsonNumber(out,north);out<<",\n  \"sourceLatitudeSouth\": ";jsonNumber(out,south);out<<",\n  \"rasterPopulationSum\": ";jsonNumber(out,total);out<<",\n  \"positivePixelCount\": "<<positivePixels<<",\n  \"validPixelCount\": "<<validPixels<<",\n  \"aggregation\": \"30 arc-second cell values assigned by pixel center to one Z9800 zone; density divides the zone estimate by NOAA ETOPO 2022 land area from the matching zones.json archive.\",\n  \"limitations\": \"Modelled 2023 estimates, not census counts. The R2025A v1 release is alpha; national and local accuracy varies. The source grid covers the extent stated in sourceLatitudeNorth/sourceLatitudeSouth; partial-latitude edge cells include only source-covered rows.\",\n  \"zones\": [\n";
  for(int row=0;row<GRID_ROWS;row++)for(int col=0;col<GRID_COLUMNS;col++){size_t i=(size_t)row*GRID_COLUMNS+col;out<<"    {\"zoneId\":\"Z9800-"<<std::setw(3)<<std::setfill('0')<<row<<"-"<<std::setw(3)<<col<<std::setfill(' ')<<"\",\"available\":"<<(int)available[i]<<",\"populationCount\":"<<std::fixed<<std::setprecision(0)<<zoneCounts[i]<<",\"populationDensityPerLandKm2\":";jsonNumber(out,zoneDensity[i]);out<<",\"latitudeCoveragePercent\":";jsonNumber(out,zoneCoverage[i]);out<<"}"<<(i+1==zoneCount?"\n":",\n");}
  out<<"  ]\n}\n";out.close();if(!out){std::wcerr<<L"Failed while writing population summary.\n";return 8;}
  std::wcout<<L"Created 9,800-zone summary. Raster population sum="<<std::fixed<<std::setprecision(0)<<total<<L", positive pixels="<<positivePixels<<L", valid pixels="<<validPixels<<L".\n";return 0;
}
