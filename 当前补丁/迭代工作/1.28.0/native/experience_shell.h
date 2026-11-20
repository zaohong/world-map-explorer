#pragma once
// The application shell owns placement, hierarchy and navigation; map/data engines stay independent.
static int experienceHeaderHeightDip(int width){return width<1000?110:72;}
static int experienceCommandHeightDip(int width){return width-sidePanelWidthDip(width)-36<560?90:52;}
static int experienceContextHeightDip(int width){if(gWorkspace==ExplorerWorkspace::Browse)return 0;return width-sidePanelWidthDip(width)-48<660?90:58;}
static int experienceHeaderHeight(int width){return uiPx(experienceHeaderHeightDip(uiDip(width)));}
static int experienceCommandHeight(int width){return uiPx(experienceCommandHeightDip(uiDip(width)));}
static int experienceContextHeight(int width){return uiPx(experienceContextHeightDip(uiDip(width)));}
static RECT experienceMapBounds(int width,int height){int w=uiDip(width);return uiRect(18,experienceHeaderHeightDip(w)+experienceCommandHeightDip(w)+experienceContextHeightDip(w),w-sidePanelWidthDip(w)-18,uiDip(height)-34);}
static explorerNavigation::View captureExperienceView(){explorerNavigation::View v;v.zoom=gMapZoom;v.cx=gMapCenterX;v.cy=gMapCenterY;v.latitude=gSelectedLatitude;v.longitude=gSelectedLongitude;v.row=gRow;v.column=gCol;v.grid=gGridLevel;v.exact=gSelectedCoordinateExact;v.input=gSelectedCoordinateFromInput;v.fine=gSelectedCoordinateIsFineGridCenter;v.name=gSelectedPlaceTitle;v.origin=gSelectedPlaceOrigin;return v;}
static void rememberView(){gViewHistory.record(captureExperienceView());if(gBackButton)EnableWindow(gBackButton,gViewHistory.canBack());if(gForwardButton)EnableWindow(gForwardButton,gViewHistory.canForward());}
static void restoreExperienceView(HWND window,bool forward){
    KillTimer(window,NAVIGATION_CAPTURE_TIMER);rememberView();explorerNavigation::View v;if(!(forward?gViewHistory.forward(v):gViewHistory.back(v)))return;
    gMapZoom=v.zoom;gMapCenterX=v.cx;gMapCenterY=v.cy;gSelectedLatitude=v.latitude;gSelectedLongitude=v.longitude;gRow=v.row;gCol=v.column;gGridLevel=v.grid;gSelectedCoordinateExact=v.exact;gSelectedCoordinateFromInput=v.input;gSelectedCoordinateIsFineGridCenter=v.fine;gSelectedPlaceTitle=v.name;gSelectedPlaceOrigin=v.origin;refreshSelectedReferenceLanguage();clampMapView();if(gGridLevelCombo)SendMessageW(gGridLevelCombo,CB_SETCURSEL,gGridLevel,0);gScrollOffset=0;gInfoTab=0;gMapSceneCacheValid=false;syncExperienceControls();layoutControls(window);InvalidateRect(window,nullptr,FALSE);
}
static void syncExperienceControls(){
    if(gBackButton)EnableWindow(gBackButton,gViewHistory.canBack());if(gForwardButton)EnableWindow(gForwardButton,gViewHistory.canForward());
    int selected=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0);if(selected<0)selected=0;
    if(gHistoryScrub&&gPendingHistorySelection<0)SendMessageW(gHistoryScrub,TBM_SETPOS,TRUE,HISTORY_YEAR_COUNT-1-selected);
    if(gPreviousYearButton)EnableWindow(gPreviousYearButton,selected+1<HISTORY_YEAR_COUNT);if(gNextYearButton)EnableWindow(gNextYearButton,selected>0);
    if(gRouteScrub){bool valid=gRouteIndex>=0;EnableWindow(gRouteScrub,valid);int progress=valid&&gRouteDistance.length()>0?(int)std::lround(gRouteDistance.at(gRouteProgress)/gRouteDistance.length()*10000):0;SendMessageW(gRouteScrub,TBM_SETPOS,TRUE,progress);}
    for(int i=0;i<3;++i)if(gWorkspaceButtons[i]){const wchar_t* zh[]={L"浏览",L"历史",L"路线"};const wchar_t* en[]={L"Explore",L"History",L"Routes"};SetWindowTextW(gWorkspaceButtons[i],gEnglish?en[i]:zh[i]);InvalidateRect(gWorkspaceButtons[i],nullptr,FALSE);}
    struct Label{HWND handle;const wchar_t* zh;const wchar_t* en;};for(const auto& label:std::initializer_list<Label>{{gBasemapMenuButton,L"底图 ▾",L"Basemap ▾"},{gLayersMenuButton,L"图层 ▾",L"Layers ▾"},{gDisplayMenuButton,L"显示 ▾",L"Display ▾"},{gSavedPlacesButton,L"收藏",L"Saved"},{gBackButton,L"←",L"←"},{gForwardButton,L"→",L"→"},{gPreviousYearButton,L"‹",L"‹"},{gNextYearButton,L"›",L"›"}})if(label.handle)SetWindowTextW(label.handle,gEnglish?label.en:label.zh);
    if(gRestoreSessionButton){SendMessageW(gRestoreSessionButton,BM_SETCHECK,gRestoreLastView?BST_CHECKED:BST_UNCHECKED,0);SetWindowTextW(gRestoreSessionButton,gEnglish?L"Restore last view on startup":L"启动时恢复上次视图");}if(gStartFreshButton)SetWindowTextW(gStartFreshButton,gEnglish?L"Start a new exploration (Alt+Home)":L"开始新的探索（Alt+Home）");
    if(gBookmarkButton){int saved=gCol>=0?savedPlaces::find(gSavedPlaces,gSelectedLatitude,gSelectedLongitude):-1;SetWindowTextW(gBookmarkButton,saved>=0?L"★":L"☆");InvalidateRect(gBookmarkButton,nullptr,FALSE);}
    if(gBasemapMenuButton)SetWindowTextW(gBasemapMenuButton,gMap==gMapHighRes&&gMapHighRes?(gEnglish?L"HD ▾":L"高清地形 ▾"):(gMap==gMapNasa&&gMapNasa?L"NASA ▾":(gEnglish?L"Terrain ▾":L"地形底图 ▾")));
    const wchar_t* recommendedZh[]={L"北京",L"罗马",L"开罗",L"京都",L"纽约",L"悉尼"};const wchar_t* recommendedEn[]={L"Beijing",L"Rome",L"Cairo",L"Kyoto",L"New York",L"Sydney"};for(int i=0;i<6;++i)if(gRecommendedButtons[i])SetWindowTextW(gRecommendedButtons[i],gEnglish?recommendedEn[i]:recommendedZh[i]);
}
static void closeExperiencePanel(HWND window){if(!gExperiencePanel)return;int previous=gExperiencePanel;gExperiencePanel=0;layoutControls(window);SetFocus(previous==1?gLayersMenuButton:gDisplayMenuButton);InvalidateRect(window,nullptr,FALSE);}
static void commitHistorySeek(HWND window){
    KillTimer(window,HISTORY_SEEK_TIMER);if(gPendingHistorySelection<0)return;int selected=gPendingHistorySelection;gPendingHistorySelection=-1;
    if(selected!=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0)){SendMessageW(gHistoryYearCombo,CB_SETCURSEL,selected,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1010,CBN_SELCHANGE),(LPARAM)gHistoryYearCombo);}syncExperienceControls();
}
static void activateWorkspace(HWND window,ExplorerWorkspace mode){
    UiDpiScope dpi(window);commitHistorySeek(window);gWorkspace=mode;gRoutePanelOpen=mode==ExplorerWorkspace::Routes;gExperiencePanel=0;
    if(mode==ExplorerWorkspace::History){gShowHistory=gHistoryLoaded;SendMessageW(gHistoryToggleButton,BM_SETCHECK,gShowHistory?BST_CHECKED:BST_UNCHECKED,0);}
    if(mode!=ExplorerWorkspace::History&&gHistoryPlaying){SendMessageW(window,WM_COMMAND,MAKEWPARAM(1015,BN_CLICKED),(LPARAM)gHistoryPlaybackButton);}
    if(mode!=ExplorerWorkspace::Routes&&gRoutePlaying)handleRouteCommand(window,3100);
    syncExperienceControls();layoutControls(window);gMapSceneCacheValid=false;InvalidateRect(window,nullptr,FALSE);
}
static void layoutExperienceControls(HWND window){
    if(!gControlPointsButton||!gGridLevelCombo||IsIconic(window))return;RECT r{};GetClientRect(window,&r);int width=uiDip(r.right),height=uiDip(r.bottom),header=experienceHeaderHeightDip(width),side=sidePanelWidthDip(width),leftWidth=width-side-36;
    auto place=[&](HWND control,int x,int y,int w,int h,bool visible=true){if(!control)return;ShowWindow(control,visible&&!gMapFullscreen?SW_SHOW:SW_HIDE);if(visible)placeUiControl(control,x,y,w,h);};
    if(gMapFullscreen){for(HWND control:experienceKeyboardControls(window))if(control)ShowWindow(control,SW_HIDE);gInfoRect=RECT{};gMapRect=r;clampMapView();layoutRegionPageControls(window);return;}
    gInfoRect=uiRect(width-side+6,header+8,width-16,height-54);gMapRect=experienceMapBounds(r.right,r.bottom);clampMapView();gExperienceStatusRect=uiRect(18,height-29,width-side-10,height-4);
    int searchX=width<1000?18:(width<1150?248:296),searchY=width<1000?64:19,searchRight=width<1000?width-18:width-332,searchWidth=std::min(600,searchRight-searchX-72);
    place(gSearchEdit,searchX,searchY,searchWidth,31);place(gSearchButton,searchX+searchWidth+8,searchY,64,31);place(gLanguageButton,width-184,20,78,29);place(gFullscreenButton,width-98,20,80,29);
    int x=18,y=header+8;for(int i=0;i<3;++i){int w=gEnglish?80:66;place(gWorkspaceButtons[i],x,y,w,32);x+=w+5;}x+=12;
    if(experienceCommandHeightDip(width)>52){x=18;y+=38;}
    place(gBasemapMenuButton,x,y,88,32);x+=94;place(gLayersMenuButton,x,y,78,32);x+=84;place(gDisplayMenuButton,x,y,82,32);place(gSavedPlacesButton,width-274,20,78,29);
    // Existing controls are retained, but appear only in the workspace or drawer where they are useful.
    for(HWND control:{gUiScaleCombo,gRestoreSessionButton,gStartFreshButton,gControlPointsButton,gHighResMapButton,gGridLevelCombo,gNasaMapButton,gLandCoverToggleButton,gPopulationToggleButton,gWorld2026ToggleButton,gHistoryToggleButton,gOpacityTrackbar,gOpacityLabel,gGridLinesToggleButton,gHistoryPlaybackButton,gHistorySpeedButton,gHistoryAnimationButton,gMapPreviewRateCombo,gRoutePanelButton,gRouteButton,gRoutePickerButton,gRouteHideButton,gRouteInfoButton,gRouteFitButton,gYearPickerButton,gPreviousYearButton,gNextYearButton,gHistoryScrub,gRouteScrub})if(control)ShowWindow(control,SW_HIDE);
    if(gHistoryYearCombo)ShowWindow(gHistoryYearCombo,SW_HIDE);if(gRouteCombo)ShowWindow(gRouteCombo,SW_HIDE);
    gExperienceContextRect=uiRect(18,header+experienceCommandHeightDip(width),width-side-18,header+experienceCommandHeightDip(width)+experienceContextHeightDip(width));
    bool narrow=leftWidth<660;int cy=uiDip(gExperienceContextRect.top)+7;
    if(gWorkspace==ExplorerWorkspace::History){
        place(gPreviousYearButton,18,cy,30,30);place(gYearPickerButton,54,cy,130,30);place(gNextYearButton,190,cy,30,30);place(gHistoryPlaybackButton,228,cy,std::max(80,std::min(116,leftWidth-228)),30);
        if(narrow){place(gHistorySpeedButton,18,cy+36,80,27);place(gHistoryScrub,106,cy+35,std::max(100,leftWidth-106),30);}else{place(gHistorySpeedButton,354,cy,82,30);place(gHistoryScrub,444,cy,std::max(100,leftWidth-444),30);}
    }
    if(gWorkspace==ExplorerWorkspace::Routes){
        int chooseWidth=narrow?std::max(130,leftWidth-112):std::min(300,std::max(160,leftWidth-428));place(gRoutePickerButton,18,cy,chooseWidth,30);place(gRouteButton,26+chooseWidth,cy,96,30);
        if(narrow){place(gRouteHideButton,18,cy+36,64,27);place(gRouteInfoButton,88,cy+36,76,27);place(gRouteFitButton,170,cy+36,64,27);place(gRouteScrub,240,cy+35,std::max(70,leftWidth-240),30);}
        else{int rx=130+chooseWidth;place(gRouteHideButton,rx,cy,64,30);place(gRouteInfoButton,rx+70,cy,80,30);place(gRouteFitButton,rx+156,cy,72,30);place(gRouteScrub,rx+236,cy,std::max(80,leftWidth+18-rx-236),30);}
    }
    const RECT info=uiDipRect(gInfoRect);int tabLeft=info.left+12,tabWidth=(info.right-info.left-94)/2;place(gGeoTabButton,tabLeft,info.top+10,tabWidth,31);place(gEventsTabButton,tabLeft+tabWidth+4,info.top+10,tabWidth,31);place(gEventSearchButton,info.right-68,info.top+10,56,31);
    place(gExportButton,info.left,height-44,info.right-info.left,30);
    const RECT content=uiDipRect(mapContentRect());gMapNavigationRect=uiRect(content.right-118,content.top+10,content.right-10,content.top+40);int nx=uiDip(gMapNavigationRect.left);for(HWND c:{gZoomOutButton,gResetMapButton,gZoomInButton}){place(c,nx,uiDip(gMapNavigationRect.top),32,30);nx+=38;}
    place(gBackButton,content.left+10,content.top+10,32,30);place(gForwardButton,content.left+48,content.top+10,32,30);place(gHelpButton,18,height-28,28,24,false);
    place(gBookmarkButton,width-310,20,30,29,gCol>=0);
    bool recommend=gCol<0&&gInfoTab==0;int buttonWidth=(info.right-info.left-42)/2,startY=info.top+200;for(int i=0;i<6;++i)place(gRecommendedButtons[i],info.left+14+(i%2)*(buttonWidth+8),startY+(i/2)*39,buttonWidth,31,recommend&&startY+(i/2)*39+31<info.bottom-10);
    layoutRegionPageControls(window);gPanelDividerRect=uiRect(info.left-10,info.top,info.left-2,info.bottom);
    int panelWidth=std::min(340,leftWidth),panelLeft=leftWidth<600?18:std::min(width-side-panelWidth-18,252),panelHeight=gExperiencePanel==2?466:260,panelTop=std::max(8,std::min(header+experienceCommandHeightDip(width)-5,height-panelHeight-8));gExperiencePanelRect=uiRect(panelLeft,panelTop,panelLeft+panelWidth,panelTop+panelHeight);
    if(gExperiencePanel){int px=panelLeft+14,py=panelTop+46,pw=panelWidth-28;if(gExperiencePanel==1){int row=0;for(HWND c:{gHistoryToggleButton,gWorld2026ToggleButton,gPopulationToggleButton,gLandCoverToggleButton}){place(c,px,py+row*36,pw,30);row++;}place(gOpacityLabel,px,py+150,116,22);place(gOpacityTrackbar,px+120,py+146,pw-120,30);}else{place(gUiScaleCombo,px,panelTop+68,pw,180);place(gGridLevelCombo,px,panelTop+132,pw,160);place(gGridLinesToggleButton,px,panelTop+172,pw,28);place(gControlPointsButton,px,panelTop+207,pw,28);place(gHistoryAnimationButton,px,panelTop+242,pw,28);place(gMapPreviewRateCombo,px,panelTop+300,pw,140);place(gRestoreSessionButton,px,panelTop+360,pw,28);place(gStartFreshButton,px,panelTop+396,pw,27);place(gHelpButton,px,panelTop+430,pw,24);SetWindowTextW(gHelpButton,gEnglish?L"Help and shortcuts (F1)":L"操作帮助与快捷键（F1）");}}
    if(gExperiencePanel){std::vector<HWND> drawerControls=gExperiencePanel==1?std::vector<HWND>{gHistoryToggleButton,gWorld2026ToggleButton,gPopulationToggleButton,gLandCoverToggleButton,gOpacityLabel,gOpacityTrackbar}:std::vector<HWND>{gUiScaleCombo,gGridLevelCombo,gGridLinesToggleButton,gControlPointsButton,gHistoryAnimationButton,gMapPreviewRateCombo,gRestoreSessionButton,gStartFreshButton,gHelpButton};for(HWND control:experienceKeyboardControls(window)){if(!control||!IsWindowVisible(control)||std::find(drawerControls.begin(),drawerControls.end(),control)!=drawerControls.end())continue;RECT bounds{};GetWindowRect(control,&bounds);MapWindowPoints(nullptr,window,(POINT*)&bounds,2);RECT intersection{};if(IntersectRect(&intersection,&bounds,&gExperiencePanelRect))ShowWindow(control,SW_HIDE);}}
    syncExperienceControls();
}
static void paintStartExperience(HDC dc,int left,int right,int top,int bottom){
    text(dc,gEnglish?(right-left<uiPx(250)?L"Start exploring":L"Where will you explore?"):L"从哪里开始探索",RECT{left+uiPx(2),top+uiPx(14),right,top+uiPx(45)},20,uiTheme::text,true);
    text(dc,gEnglish?L"Search a place, event or journey, or choose a city below.":L"搜索地点、事件或旅行路线，\n也可以先选一个城市。",RECT{left+uiPx(2),top+uiPx(60),right-uiPx(4),std::min(bottom,top+uiPx(110))},14,uiTheme::muted);
    int after=gInfoRect.top+uiPx(324);if(after+uiPx(70)<bottom)text(dc,gEnglish?L"Click anywhere on the map to read the surrounding area. The map and search work offline.":L"点击地图任何位置，了解周边地理与人文。地图与搜索均可离线使用。",RECT{left+uiPx(2),after,right-uiPx(4),bottom},13,uiTheme::muted);
}
static void paintExperienceShell(HWND window,HDC dc){
    RECT client{};GetClientRect(window,&client);int width=client.right,height=client.bottom;HBRUSH canvas=CreateSolidBrush(uiTheme::canvas);FillRect(dc,&client,canvas);DeleteObject(canvas);
    if(gMapFullscreen){drawMapSurface(dc,width,height,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top,true);return;}
    int header=experienceHeaderHeight(width);HBRUSH headerBrush=CreateSolidBrush(uiTheme::header);RECT head{0,0,width,header};FillRect(dc,&head,headerBrush);DeleteObject(headerBrush);
    Graphics graphics(dc);drawGlobeEmblem(graphics,uiPx(20),uiPx(15),uiPx(34));text(dc,L"世界地图探索器",uiRect(64,14,246,40),18,uiTheme::text,true);text(dc,gEnglish?L"Explore the world offline":L"离线探索世界",uiRect(65,41,246,62),11,uiTheme::muted);
    HBRUSH tool=CreateSolidBrush(uiTheme::toolbar);RECT commands{0,header,width-sidePanelWidth(width),header+experienceCommandHeight(width)-uiPx(4)};FillRect(dc,&commands,tool);DeleteObject(tool);
    if(gWorkspace!=ExplorerWorkspace::Browse){HBRUSH context=CreateSolidBrush(RGB(12,30,43));FillRect(dc,&gExperienceContextRect,context);DeleteObject(context);}
    HBRUSH panel=CreateSolidBrush(uiTheme::panel);FillRect(dc,&gInfoRect,panel);DeleteObject(panel);HPEN line=CreatePen(PS_SOLID,1,uiTheme::border);auto old=(HPEN)SelectObject(dc,line);MoveToEx(dc,gInfoRect.left-uiPx(5),gInfoRect.top,nullptr);LineTo(dc,gInfoRect.left-uiPx(5),gInfoRect.bottom);SelectObject(dc,old);DeleteObject(line);
    if(gInfoTab==1)paintHistoryEvents(dc);else paintHumanRegion(dc);
    drawMapSurface(dc,width,height,gMapRect.right-gMapRect.left,gMapRect.bottom-gMapRect.top,false);
    text(dc,gNotice,gExperienceStatusRect,11,uiTheme::muted);
    if(gExperiencePanel){Graphics g(dc);SolidBrush shadow(Color(130,0,0,0)),surface(Color(255,15,39,54));Pen edge(Color(255,62,109,124));auto r=gExperiencePanelRect;g.FillRectangle(&shadow,r.left+uiPx(4),r.top+uiPx(5),r.right-r.left,r.bottom-r.top);g.FillRectangle(&surface,r.left,r.top,r.right-r.left,r.bottom-r.top);g.DrawRectangle(&edge,r.left,r.top,r.right-r.left,r.bottom-r.top);text(dc,gExperiencePanel==1?(gEnglish?L"Map layers":L"地图图层"):(gEnglish?L"Display settings":L"显示设置"),RECT{r.left+uiPx(14),r.top+uiPx(13),r.right-uiPx(12),r.top+uiPx(36)},16,uiTheme::text,true);if(gExperiencePanel==2){text(dc,gEnglish?L"Interface size":L"界面大小",RECT{r.left+uiPx(14),r.top+uiPx(47),r.right-uiPx(12),r.top+uiPx(68)},12,uiTheme::muted);text(dc,gEnglish?L"Continue exploring":L"继续探索",RECT{r.left+uiPx(14),r.top+uiPx(337),r.right-uiPx(12),r.top+uiPx(357)},12,uiTheme::muted);text(dc,gEnglish?L"Location grid":L"定位格网",RECT{r.left+uiPx(14),r.top+uiPx(110),r.right-uiPx(12),r.top+uiPx(131)},12,uiTheme::muted);text(dc,gEnglish?L"Interaction smoothness":L"交互流畅度",RECT{r.left+uiPx(14),r.top+uiPx(278),r.right-uiPx(12),r.top+uiPx(299)},12,uiTheme::muted);}}
}
static std::vector<HWND> experienceKeyboardControls(HWND){
    std::vector<HWND> result{gSearchEdit,gSearchButton,gBookmarkButton,gSavedPlacesButton,gLanguageButton,gFullscreenButton,gWorkspaceButtons[0],gWorkspaceButtons[1],gWorkspaceButtons[2],gBasemapMenuButton,gLayersMenuButton,gDisplayMenuButton,gPreviousYearButton,gYearPickerButton,gNextYearButton,gHistoryPlaybackButton,gHistorySpeedButton,gHistoryScrub,gRoutePickerButton,gRouteButton,gRouteHideButton,gRouteInfoButton,gRouteFitButton,gRouteScrub,gControlPointsButton,gGridLevelCombo,gMapPreviewRateCombo,gHistoryAnimationButton,gLandCoverToggleButton,gPopulationToggleButton,gWorld2026ToggleButton,gHistoryToggleButton,gOpacityTrackbar,gGridLinesToggleButton,gGeoTabButton,gEventsTabButton,gEventSearchButton};
    result.push_back(gUiScaleCombo);result.push_back(gRestoreSessionButton);result.push_back(gStartFreshButton);for(HWND c:gRegionPageButtons)result.push_back(c);for(HWND c:gRecommendedButtons)result.push_back(c);result.insert(result.end(),{gRegionRawButton,gExportButton,gBackButton,gForwardButton,gZoomOutButton,gResetMapButton,gZoomInButton,gHelpButton});return result;
}
static bool handleExperienceMessage(HWND window,UINT message,WPARAM a,LPARAM b){
    if(message==WM_LBUTTONUP)SetTimer(window,NAVIGATION_CAPTURE_TIMER,220,nullptr);
    if(message==WM_TIMER&&a==HISTORY_SEEK_TIMER){commitHistorySeek(window);return true;}
    if(message==WM_TIMER&&a==NAVIGATION_CAPTURE_TIMER){KillTimer(window,NAVIGATION_CAPTURE_TIMER);rememberView();return true;}
    if(message==WM_KEYDOWN&&a==VK_ESCAPE&&gExperiencePanel){closeExperiencePanel(window);return true;}
    if((message==WM_LBUTTONDOWN||message==WM_LBUTTONDBLCLK)&&gExperiencePanel){POINT p{GET_X_LPARAM(b),GET_Y_LPARAM(b)};if(PtInRect(&gExperiencePanelRect,p))return true;closeExperiencePanel(window);}
    if(message==WM_MOUSEWHEEL&&gExperiencePanel){POINT p{GET_X_LPARAM(b),GET_Y_LPARAM(b)};ScreenToClient(window,&p);if(PtInRect(&gExperiencePanelRect,p))return true;}
    if(message==WM_HSCROLL&&(HWND)b==gHistoryScrub){if(gHistoryPlaying){KillTimer(window,HISTORY_PLAYBACK_TIMER);gHistoryPlaying=false;gHistoryPaused=false;finishHistoryTransition(window);updateHistoryPlaybackButton();}int position=(int)SendMessageW(gHistoryScrub,TBM_GETPOS,0,0);gPendingHistorySelection=HISTORY_YEAR_COUNT-1-position;SetWindowTextW(gYearPickerButton,historyYearText(HISTORY_YEARS[gPendingHistorySelection]).c_str());if(LOWORD(a)==TB_THUMBTRACK)SetTimer(window,HISTORY_SEEK_TIMER,85,nullptr);else commitHistorySeek(window);return true;}
    if(message==WM_HSCROLL&&(HWND)b==gRouteScrub){if(gRouteIndex<0)return true;KillTimer(window,ROUTE_PLAYBACK_TIMER);gRoutePlaying=false;int pos=(int)SendMessageW(gRouteScrub,TBM_GETPOS,0,0);gRouteProgress=gRouteDistance.progress(gRouteDistance.length()*pos/10000.0);updateRouteRaster();updateRouteControls();gNotice=gEnglish?L"Route paused at the selected position":L"路线已暂停在所选进度，可继续播放";InvalidateRect(window,nullptr,FALSE);return true;}
    if(message!=WM_COMMAND||HIWORD(a)!=BN_CLICKED)return false;int id=LOWORD(a);
    if(id>=1070&&id<=1072){activateWorkspace(window,(ExplorerWorkspace)(id-1070));return true;}
    if(id==1074||id==1075){int panel=id==1074?1:2;gExperiencePanel=gExperiencePanel==panel?0:panel;layoutControls(window);InvalidateRect(window,nullptr,FALSE);return true;}
    if(id==1076||id==1077){restoreExperienceView(window,id==1077);return true;}
    if(id==1078||id==1079){commitHistorySeek(window);int current=(int)SendMessageW(gHistoryYearCombo,CB_GETCURSEL,0,0),next=current+(id==1078?1:-1);if(next>=0&&next<HISTORY_YEAR_COUNT){SendMessageW(gHistoryYearCombo,CB_SETCURSEL,next,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1010,CBN_SELCHANGE),(LPARAM)gHistoryYearCombo);}return true;}
    if(id==1084){startFreshExploration(window);return true;}if(id==1085){gRestoreLastView=!gRestoreLastView;syncExperienceControls();schedulePreferencesSave(window);if(gRestoreLastView)saveLastView();return true;}
    if(id==1082){showSavedPlaces(window);return true;}if(id==1083){togglePlaceBookmark(window);return true;}if(id>=1140&&id<1146){activateRecommendedPlace(window,id-1140);return true;}
    if(id>=1120&&id<=1122){
        // The shared command handler flips owner-drawn toggle state once.
        if(id==1120){if(gMap==gMapNasa){SendMessageW(gNasaMapButton,BM_SETCHECK,BST_CHECKED,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1006,BN_CLICKED),(LPARAM)gNasaMapButton);}else if(gMap==gMapHighRes){SendMessageW(gHighResMapButton,BM_SETCHECK,BST_CHECKED,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1004,BN_CLICKED),(LPARAM)gHighResMapButton);}}
        else if(id==1121&&gMap!=gMapHighRes){SendMessageW(gHighResMapButton,BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1004,BN_CLICKED),(LPARAM)gHighResMapButton);}
        else if(id==1122&&gMap!=gMapNasa){SendMessageW(gNasaMapButton,BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1006,BN_CLICKED),(LPARAM)gNasaMapButton);}syncExperienceControls();return true;
    }
    if(id==1073){HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING|(gMap==gMapOriginal?MF_CHECKED:0),1120,gEnglish?L"Terrain overview":L"地形概览");AppendMenuW(menu,MF_STRING|(gMap==gMapHighRes&&gMapHighRes?MF_CHECKED:0),1121,gEnglish?L"Detailed terrain":L"高清地形");AppendMenuW(menu,MF_STRING|(gMap==gMapNasa&&gMapNasa?MF_CHECKED:0),1122,gEnglish?L"NASA satellite map":L"NASA 卫星底图");RECT button{};GetWindowRect(gBasemapMenuButton,&button);int command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_LEFTALIGN,button.left,button.bottom+4,0,window,nullptr);DestroyMenu(menu);
        if(command)SendMessageW(window,WM_COMMAND,MAKEWPARAM(command,BN_CLICKED),0);return true;}
    return false;
}

