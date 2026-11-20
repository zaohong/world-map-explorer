#pragma once
// Application view module; included after the shared text, scrolling and data helpers.
static bool regionMetric(const char* key,double& value){
  if(!gArchiveLoaded||gRow<0||gCol<0)return false;size_t i=(size_t)gRow*GRID_COLUMNS+gCol;if(i>=gZoneRanges.size())return false;const auto& range=gZoneRanges[i];return jsonDoubleField(gArchive,range.begin,range.end,key,value)&&std::isfinite(value);
}
static void layoutRegionPageControls(HWND window){
  RECT client{};GetClientRect(window,&client);int side=uiDip(sidePanelWidth(client.right)),left=uiDip(client.right)-side+20,top=uiDip(infoTopFor(client.right))+51,width=side-50,gap=4,buttonWidth=(width-3*gap)/4;
  const wchar_t* zh[]={L"概况",L"自然",L"人文",L"资料"};const wchar_t* en[]={L"Summary",L"Nature",L"Places",L"Sources"};
  for(int i=0;i<4;++i)if(gRegionPageButtons[i]){ShowWindow(gRegionPageButtons[i],gInfoTab==0&&!gMapFullscreen?SW_SHOW:SW_HIDE);SetWindowTextW(gRegionPageButtons[i],gEnglish?en[i]:zh[i]);placeUiControl(gRegionPageButtons[i],left+i*(buttonWidth+gap),top,buttonWidth,25);InvalidateRect(gRegionPageButtons[i],nullptr,FALSE);}
  if(gRegionRawButton){ShowWindow(gRegionRawButton,gInfoTab==0&&gRegionPage==3&&!gMapFullscreen?SW_SHOW:SW_HIDE);SetWindowTextW(gRegionRawButton,gEnglish?L"Open original record...":L"查看完整原始记录…");placeUiControl(gRegionRawButton,left,top+31,width,26);}
}
static std::wstring regionCountryName(){
  if(gHistoryVectors.valid()&&gHistoryVectors.year()==2024){auto hits=gHistoryVectors.hits(gSelectedLatitude,gSelectedLongitude);for(size_t i:hits){const auto& e=gHistoryVectors.entity(gHistoryVectors.record(i).entity);if(e.sourceName.find(L"Natural Earth reference: ")==0)return gEnglish||e.zh.empty()?e.en:e.zh;}}
  int r=0,c=0;world2026CellForCoordinate(gSelectedLatitude,gSelectedLongitude,r,c);size_t index=(size_t)r*FINE_GRID_COLUMNS+c;
  if(index<gWorld2026CellCountries.size()){unsigned id=gWorld2026CellCountries[index];if(id&&id<=gWorld2026Countries.size()){const auto& country=gWorld2026Countries[id-1];return gEnglish?country.name:(country.nameZh.empty()?country.name:country.nameZh);}}
  auto country=archiveZoneField(gRow,gCol,"countries",L"");return regionReading::known(country)?country:L"";
}
static std::wstring climateReading(const std::wstring& source){
  if(!regionReading::known(source))return L"";std::wstring out;int count=0;
  for(auto item:regionReading::split(source)){
    if(gEnglish){size_t length=0;while(length<item.size()&&((item[length]>=L'A'&&item[length]<=L'Z')||(item[length]>=L'a'&&item[length]<=L'z')))++length;item.resize(length);}
    else{size_t p=item.find(L"样点");if(p!=std::wstring::npos){size_t left=item.rfind(L'（',p),right=item.find(L'）',p);if(left!=std::wstring::npos&&right!=std::wstring::npos)item.erase(left,right-left+1);}}
    if(!item.empty()){if(!out.empty())out+=gEnglish?L" · ":L"；";out+=regionReading::trim(item);if(++count==2)break;}
  }return out;
}
static std::wstring regionTitle(){
  if(!gSelectedPlaceTitle.empty())return gSelectedPlaceTitle;
  if(gSelectedCoordinateExact)return gEnglish?L"Selected location":L"地图位置";
  auto country=regionCountryName(),city=regionReading::names(archiveZoneField(gRow,gCol,"cities",L""),1,gEnglish);
  if(gHistoryVectors.valid()&&gHistoryVectors.year()==2024){auto hits=gHistoryVectors.hits(gSelectedLatitude,gSelectedLongitude);for(size_t i:hits)if(gHistoryVectors.entity(gHistoryVectors.record(i).entity).sourceName.find(L"Natural Earth reference: ")==0)return country;}
  if(!city.empty())return city+(gEnglish?L" area":L"一带");
  if(!country.empty())return country;
  double land=0;if(regionMetric("landAreaPercent",land)&&land<1)return gEnglish?L"Ocean region":L"海洋分区";
  return gEnglish?L"Location overview":L"位置概况";
}
static void showRegionOriginal(HWND owner){
  if(gCol<0)return;
  std::wstring body=(gEnglish?L"Original source record for ":L"基础分区原始资料：")+gridIdText(GRID_VERSION,gRow,gCol)+L"\r\n\r\n";
  const std::pair<const wchar_t*,const char*> fields[]={{L"国家／地区","countries"},{L"地形地貌","terrain"},{L"地表面积","surface"},{L"气候分类","climate"},{L"生物群系","biomes"},{L"生态区","ecoregions"},{L"城市／聚落","cities"},{L"附近城市","nearby"},{L"河流","rivers"},{L"水域","lakes"},{L"山地","mountains"},{L"地貌","landforms"},{L"历史地点","history"},{L"曾用名","formerNames"},{L"来源与方法","sources"}};
  for(const auto& field:fields)body+=localizeUiText(field.first)+L"\r\n"+archiveZoneField(gRow,gCol,field.second,L"")+L"\r\n\r\n";
  body+=L"UNESCO\r\n"+heritageZoneField(gRow,gCol,L"");
  body+=L"\r\n\r\n"+localizeUiText(L"土地覆盖")+L"\r\n"+landCoverZoneField(gRow,gCol,"landCover",L"")+L"\r\n"+landCoverZoneField(gRow,gCol,"landCoverCoverage",L"");
  body+=L"\r\n\r\n"+localizeUiText(L"人口估算（2023）")+L"\r\n"+populationZoneField(gRow,gCol,L"");
  for(size_t p=0;(p=body.find(L"\\n",p))!=std::wstring::npos;p+=2)body.replace(p,2,L"\r\n");
  showRouteNotes(owner,body,gEnglish?L"Original region record":L"分区原始资料");
}
static void paintHumanRegion(HDC dc){
  using namespace regionReading;
  const int left=gInfoRect.left+uiPx(15),right=gInfoRect.right-uiPx(15),width=std::max(uiPx(80),right-left),bodyTop=gInfoRect.top+uiPx(gRegionPage==3?116:84),bottom=gInfoRect.bottom-uiPx(13);
  if(gCol<0){paintStartExperience(dc,left,right,bodyTop,bottom);gContentHeight=0;gViewportHeight=std::max(1,bottom-bodyTop);return;}
  struct Card{std::wstring title,body;};struct Metric{std::wstring title,value;};
  auto key=std::make_tuple(gRow,gCol,gRegionPage,gEnglish,gHistoryYear,gHistoryDataRevision,gSelectedLatitude,gSelectedLongitude,width,gSelectedPlaceTitle,gSelectedPlaceOrigin,gUiDpi);
  static auto previousKey=key;static bool ready=false;static std::vector<Card> cards;static std::vector<Metric> metrics;static std::wstring title,subtitle,scopeText;static std::vector<int> heights,titleHeights;
  if(!ready||key!=previousKey){
    cards.clear();metrics.clear();heights.clear();titleHeights.clear();title=regionTitle();subtitle=preciseLatitude(gSelectedLatitude)+L"  "+preciseLongitude(gSelectedLongitude);
    auto add=[&](const wchar_t* zh,const wchar_t* en,const std::wstring& value){if(!value.empty())cards.push_back({gEnglish?en:zh,value});};
    double land=0,mean=0,min=0,max=0,sea=0,total=0;bool hasLand=regionMetric("landAreaPercent",land),hasMean=regionMetric("meanLandElevationM",mean),hasMin=regionMetric("minLandElevationM",min),hasMax=regionMetric("maxLandElevationM",max),hasSea=regionMetric("meanSeaDepthM",sea);regionMetric("totalAreaKm2",total);
    scopeText=gEnglish?L"Surrounding base-region statistics · "+number(total)+L" km²":L"周边分区统计 · 覆盖约 "+(total>=10000?number(total/10000,1)+L"万":number(total))+L" km²";
    size_t index=(size_t)gRow*GRID_COLUMNS+gCol;bool population=index<gPopulationAvailable.size()&&gPopulationAvailable[index];
    const auto cities=names(archiveZoneField(gRow,gCol,"cities",L""),6,gEnglish),rivers=names(archiveZoneField(gRow,gCol,"rivers",L""),5,gEnglish),lakes=names(archiveZoneField(gRow,gCol,"lakes",L""),5,gEnglish),mountains=names(archiveZoneField(gRow,gCol,"mountains",L""),5,gEnglish),landforms=names(archiveZoneField(gRow,gCol,"landforms",L""),4,gEnglish),historic=names(archiveZoneField(gRow,gCol,"history",L""),5,gEnglish),heritage=heritageNames(heritageZoneField(gRow,gCol,L""),5,gEnglish);
    std::wstring terrain;if(hasLand)terrain=(gEnglish?L"Land covers ":L"陆地占 ")+number(land,1)+L"%";
    if(hasMean){if(!terrain.empty())terrain+=gEnglish?L"; ":L"；";terrain+=(gEnglish?L"mean land elevation ":L"陆地平均海拔 ")+number(mean)+L" m";}
    if(hasSea&&(!hasLand||land<99)){if(!terrain.empty())terrain+=gEnglish?L"; ":L"；";terrain+=(gEnglish?L"mean water depth ":L"海水平均深度 ")+number(sea)+L" m";}
    std::wstring cover=landCoverZoneField(gRow,gCol,"landCover",L"");if(!known(cover))cover.clear();if(cover.find(L"主要类别（前五）：")==0)cover.erase(0,std::wcslen(L"主要类别（前五）："));
    if(gEnglish&&!cover.empty())cover=localizeUiText(cover);
    if(gRegionPage==0){
      if(population)metrics.push_back({gEnglish?L"Region pop. '23":L"分区人口",compactPopulation(gPopulationCounts[index],gEnglish)});
      if(hasLand)metrics.push_back({gEnglish?L"Land share":L"陆地占比",number(land)+L"%"});
      if(hasMean)metrics.push_back({gEnglish?L"Elevation":L"平均海拔",number(mean)+L" m"});else if(hasSea)metrics.push_back({gEnglish?L"Sea depth":L"平均水深",number(sea)+L" m"});
      std::wstring brief;
      if(hasMin&&hasMax)brief=(gEnglish?L"Land elevation within this region: ":L"区内陆地高程约 ")+number(min)+L" – "+number(max)+L" m";
      if(!landforms.empty()){if(!brief.empty())brief+=L"\n";brief+=names(archiveZoneField(gRow,gCol,"landforms",L""),2,gEnglish);}
      if(brief.empty())brief=terrain;
      add(L"地形线索",L"Landscape",brief);
      add(L"在所选年份 · 来源边界",L"In the selected year · mapped source",historyYearText(gHistoryYear)+L"\n"+historyZoneField());
      add(L"已收录的主要城市",L"Mapped major cities",names(archiveZoneField(gRow,gCol,"cities",L""),3,gEnglish));
      add(L"土地覆盖 · 2020 预览",L"Land cover · 2020 preview",cover);
      if(!heritage.empty())add(L"分区内的世界遗产",L"World Heritage in this region",heritage);
    }else if(gRegionPage==1){
      add(L"地形与陆海",L"Terrain and land / sea",terrain);
      if(hasMin&&hasMax)add(L"陆地高程范围",L"Land elevation range",number(min)+L" – "+number(max)+L" m");
      add(L"土地覆盖 · 2020 预览",L"Land cover · 2020 preview",cover);
      add(L"气候分类 · 1991–2020",L"Climate classes · 1991–2020",climateReading(archiveZoneField(gRow,gCol,"climate",L"")));
      add(L"生态区样点",L"Sampled ecological regions",names(archiveZoneField(gRow,gCol,"biomes",L""),2,gEnglish));
      add(L"河流",L"Rivers",rivers);add(L"湖泊与其他水域",L"Lakes and other waters",lakes);add(L"山地与地貌",L"Mountains and landforms",mountains+(mountains.empty()||landforms.empty()?L"":L"\n")+landforms);
    }else if(gRegionPage==2){
      add(L"城市与聚落",L"Cities and settlements",cities);
      if(population)add(L"人口概况 · 2023 模型估算",L"Population · 2023 model estimate",compactPopulation(gPopulationCounts[index],gEnglish)+(gEnglish?L" people in the base region; density about ":L"人；按分区陆地面积计算，密度约 ")+number(gPopulationDensity[index],1)+(gEnglish?L" people/km².":L" 人/km²。"));
      add(L"世界遗产 · 分区内已收录",L"World Heritage · mapped in the base region",heritage);
      add(L"历史地点",L"Historical places",historic);
      if(!known(cities)&&heritage.empty()&&historic.empty())add(L"资料覆盖",L"Source coverage",gEnglish?L"No named places are recorded for this base region yet. This does not mean the area has no people or heritage.":L"此分区暂缺已收录的地名与遗产资料，不表示当地没有人类活动或文化遗产。");
    }else{
      if(!gSelectedPlaceOrigin.empty())add(L"当前位置的来源",L"Location source",gSelectedPlaceOrigin);
      add(L"这份概况覆盖多大范围",L"Extent of this overview",gridIdText(GRID_VERSION,gRow,gCol)+L"\n"+baseRegionBoundsText(gRow,gCol)+L"\n"+(gEnglish?L"About ":L"约 ")+number(total)+L" km²"+(gEnglish?L" including ocean. Statistics are not point measurements.":L"（含海洋）。统计不是点位测量。"));
      int fineRow=0,fineCol=0;selectedFineGridCell(fineRow,fineCol);
      add(L"当前网格仅用于定位",L"Location grid",gridIdText(activeGridCount(),fineRow,fineCol)+L"\n"+fineGridBoundsText(fineRow,fineCol)+(gEnglish?L"\nChoosing a finer grid refines location; summaries still use the base region.":L"\n更细网格提高定位精度；概况仍按基础分区汇总。"));
      add(L"历史边界与年份",L"Historical boundaries and years",gEnglish?L"Cliopatria / Seshat source polygons, 3400 BCE–2024 CE. A source validity interval is not necessarily a polity's founding and end date. Alliance / allegiance records are kept separate from territory.":L"使用 Cliopatria / Seshat 来源矢量，范围为前3400年至2024年。资料的有效年段不一定是政权建立、灭亡日期。联盟和附属关系不当作政权领土填色。");
      if(gHistoryVectors.valid()){auto hits=gHistoryVectors.hits(gSelectedLatitude,gSelectedLongitude);for(size_t i:hits){const auto& e=gHistoryVectors.entity(gHistoryVectors.record(i).entity);auto note=gEnglish?e.noteEn:e.noteZh;if(!note.empty())add(L"这条边界的补充说明",L"Note for this boundary",note);}}
      if(gHistoryVectors.valid()){auto relations=gHistoryVectors.hits(gSelectedLatitude,gSelectedLongitude,true);std::wstring names;for(size_t n=0;n<std::min((size_t)4,relations.size());++n){const auto& record=gHistoryVectors.record(relations[n]);const auto& entity=gHistoryVectors.entity(record.entity);if(!names.empty())names+=L"\n";names+=(gEnglish||entity.zh.empty()?entity.en:entity.zh);names+=L" · "+historyYearText(record.from)+L" – "+historyYearText(record.to);}if(!names.empty())add(L"联盟与附属关系 · 来源记录",L"Alliances / allegiance · source records",names+(gEnglish?L"\nThese records do not establish sovereign territory.":L"\n关系记录不直接作为主权领土填色。"));}
      add(L"地理与人口资料",L"Geography and population sources",gEnglish?L"Elevation / depth: NOAA ETOPO 2022. Population: WorldPop 2023 model estimates. Land cover: 2020 statistical preview. Climate: 1991–2020 classification; not current weather.":L"高程与海深：NOAA ETOPO 2022。人口：WorldPop 2023 模型估算。土地覆盖：2020 年统计预览。气候：1991–2020 年分类，不是实时天气。");
      add(L"地名与文化资料",L"Place and heritage sources",gEnglish?L"Place names: GeoNames and Natural Earth. Heritage: UNESCO snapshot. Lists are limited by source coverage; omitted entries do not establish absence.":L"地名来自 GeoNames、Natural Earth；遗产来自 UNESCO 快照。清单受来源覆盖与条目上限影响，未列出不表示不存在。");
    }
    if(population&&gPopulationCoverage[index]<99.9)add(L"人口估值覆盖不完整",L"Partial population coverage",(gEnglish?L"Latitude coverage in this base region: ":L"此分区纬度范围的估值覆盖约 ")+number(gPopulationCoverage[index],1)+L"%"+(gEnglish?L". Uncovered areas are not counted as zero.":L"；未覆盖地区不按零人口理解。"));
    for(const auto& card:cards){int titleHeight=measureText(dc,card.title,width-uiPx(24),13,true,true);titleHeights.push_back(titleHeight);heights.push_back(titleHeight+measureText(dc,card.body,width-uiPx(24),14,false,true)+uiPx(28));}
    previousKey=key;ready=true;
  }
  int titleHeight=measureText(dc,title,width-uiPx(8),19,true,true),scopeHeight=measureText(dc,scopeText,width-uiPx(8),12,false,true);
  bool stackedMetrics=width<uiPx(260);int metricsHeight=metrics.empty()?0:(stackedMetrics?(int)metrics.size()*uiPx(46)+uiPx(4):uiPx(74));
  gViewportHeight=std::max(1,bottom-bodyTop);gContentHeight=titleHeight+uiPx(24)+scopeHeight+uiPx(15)+metricsHeight;for(int height:heights)gContentHeight+=height+uiPx(10);
  gScrollOffset=std::max(0,std::min(gScrollOffset,std::max(0,gContentHeight-gViewportHeight)));
  int saved=SaveDC(dc);IntersectClipRect(dc,left,bodyTop,right+uiPx(1),bottom);int y=bodyTop+uiPx(4)-gScrollOffset;
  text(dc,title,RECT{left+uiPx(2),y,right-uiPx(4),y+titleHeight},19,uiTheme::text,true,true);y+=titleHeight+uiPx(4);text(dc,subtitle,RECT{left+uiPx(2),y,right-uiPx(4),y+uiPx(18)},12,RGB(154,208,220),false,true);y+=uiPx(23);
  text(dc,scopeText,RECT{left+uiPx(2),y,right-uiPx(4),y+scopeHeight},12,uiTheme::muted,false,true);y+=scopeHeight+uiPx(13);
  if(!metrics.empty()){int gap=uiPx(6),mw=stackedMetrics?width:(width-(int)(metrics.size()-1)*gap)/(int)metrics.size();for(size_t i=0;i<metrics.size();++i){int ty=y+(stackedMetrics?(int)i*uiPx(46):0);RECT tile{left+(stackedMetrics?0:(int)i*(mw+gap)),ty,left+(stackedMetrics?width:(int)i*(mw+gap)+mw),ty+(uiPx(stackedMetrics?40:63))};HBRUSH brush=CreateSolidBrush(RGB(18,53,68));FillRect(dc,&tile,brush);DeleteObject(brush);text(dc,metrics[i].title,RECT{tile.left+uiPx(7),ty+uiPx(7),stackedMetrics?tile.left+uiPx(99):tile.right-uiPx(5),ty+(uiPx(stackedMetrics?34:22))},11,uiTheme::muted,false,true);int size=18,valueWidth=stackedMetrics?width-uiPx(110):mw-uiPx(12);for(;size>11;--size){HFONT old=(HFONT)SelectObject(dc,uiFontFor(size,true));SIZE extent{};GetTextExtentPoint32W(dc,metrics[i].value.c_str(),(int)metrics[i].value.size(),&extent);SelectObject(dc,old);if(extent.cx<valueWidth)break;}text(dc,metrics[i].value,RECT{stackedMetrics?tile.left+uiPx(104):tile.left+uiPx(7),ty+(uiPx(stackedMetrics?9:29)),tile.right-uiPx(5),ty+(uiPx(stackedMetrics?35:57))},size,RGB(236,249,250),true,true);}y+=metricsHeight;}
  for(size_t i=0;i<cards.size();++i){int height=heights[i];if(y+height>bodyTop&&y<bottom){RECT tile{left,y,right,y+height};HBRUSH brush=CreateSolidBrush(RGB(15,42,58));HPEN pen=CreatePen(PS_SOLID,std::max(1,uiPx(1)),RGB(35,73,91));auto oldBrush=SelectObject(dc,brush),oldPen=SelectObject(dc,pen);RoundRect(dc,tile.left,tile.top,tile.right,tile.bottom,uiPx(10),uiPx(10));SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(brush);DeleteObject(pen);text(dc,cards[i].title,RECT{left+uiPx(11),y+uiPx(10),right-uiPx(10),y+uiPx(10)+titleHeights[i]},13,uiTheme::accent,true,true);text(dc,cards[i].body,RECT{left+uiPx(11),y+uiPx(17)+titleHeights[i],right-uiPx(10),y+height-uiPx(8)},14,uiTheme::text,false,true);}y+=height+uiPx(10);}
  RestoreDC(dc,saved);paintInfoScrollbar(dc,bodyTop,bottom);
}
