#pragma once
#include <shellapi.h>
#pragma comment(lib,"shell32.lib")
// Modeless, independent search. It does not alter the map until a navigation button is used.
static void layoutControls(HWND);
struct EventSearchState{std::vector<size_t> results;std::vector<std::wstring> categories;bool refreshing=false;};
static std::wstring eventControlText(HWND window,int id){HWND child=GetDlgItem(window,id);int length=GetWindowTextLengthW(child);std::wstring value((size_t)length+1,L'\0');GetWindowTextW(child,&value[0],length+1);value.resize(length);return value;}
static const HistoryEvent* selectedSearchEvent(HWND window){auto* state=(EventSearchState*)GetWindowLongPtrW(window,GWLP_USERDATA);int row=ListView_GetNextItem(GetDlgItem(window,2105),-1,LVNI_SELECTED);return state&&row>=0&&(size_t)row<state->results.size()?&gHistoryEvents[state->results[row]]:nullptr;}
static std::wstring eventYearLabel(const HistoryEvent& event){return historyYearText(event.year)+(event.endYear!=event.year?L" – "+historyYearText(event.endYear):L"");}
static void updateEventSearchDetail(HWND window){
    auto* e=selectedSearchEvent(window);for(int id:{2107,2108,2109})EnableWindow(GetDlgItem(window,id),e&&(id!=2108||e->hasCoordinate));
    if(!e){SetWindowTextW(GetDlgItem(window,2106),gEnglish?L"Select an event to read its summary and sources.":L"选择一条事件，阅读摘要与资料来源。");return;}
    std::wstring detail=e->heading(gEnglish)+L"\r\n\r\n"+eventYearLabel(*e)+L"  ·  "+e->kind(gEnglish)+L"\r\n"+e->location(gEnglish)+L"\r\n\r\n"+e->summary(gEnglish);
    auto note=gEnglish?e->dateNoteEn:e->dateNoteZh;if(!note.empty())detail+=L"\r\n\r\n"+note;
    if(e->hasCoordinate){wchar_t point[100]{};swprintf(point,100,L"%.4f°, %.4f°",e->latitude,e->longitude);detail+=gEnglish?L"\r\n\r\nMap reference point: ":L"\r\n\r\n地图参考点：";detail+=point;detail+=gEnglish?L"\r\nA source location for orientation; not a surveyed event boundary.":L"\r\n用于辨识事件所在地区，不是测量所得的事件范围。";}
    else detail+=gEnglish?L"\r\n\r\nNo reliable Earth location is recorded for this entry.":L"\r\n\r\n此条没有可用的地球坐标。";
    detail+=gEnglish?L"\r\n\r\nSource\r\n":L"\r\n\r\n资料来源\r\n";detail+=e->source;
    SetWindowTextW(GetDlgItem(window,2106),detail.c_str());SendMessageW(GetDlgItem(window,2106),EM_SETSEL,0,0);SendMessageW(GetDlgItem(window,2106),EM_SCROLLCARET,0,0);
}
static void filterEventSearch(HWND window){
    auto* state=(EventSearchState*)GetWindowLongPtrW(window,GWLP_USERDATA);if(!state||state->refreshing)return;
    int from=INT_MIN,to=INT_MAX;bool valid=true;
    for(auto pair:{std::pair<int,int*>(2103,&from),std::pair<int,int*>(2104,&to)}){
        auto value=trimmedSearchQuery(eventControlText(window,pair.first));if(value.empty())continue;
        std::string ascii;for(wchar_t c:value){if((c>=L'0'&&c<=L'9')||c==L'-'||c==L'+')ascii+=(char)c;else valid=false;}
        if(!events::integer(ascii,*pair.second))valid=false;
    }
    if(from>to)valid=false;int category=(int)SendDlgItemMessageW(window,2102,CB_GETCURSEL,0,0);
    std::wstring selected=category>0&&(size_t)category<=state->categories.size()?state->categories[category-1]:L"";
    state->results=valid?events::filter(gHistoryEvents,eventControlText(window,2101),selected,from,to):std::vector<size_t>{};
    HWND list=GetDlgItem(window,2105);SendMessageW(list,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(list);
    for(size_t row=0;row<state->results.size();++row){const auto& event=gHistoryEvents[state->results[row]];auto year=gEnglish?(std::to_wstring(std::abs(event.year))+(event.year<0?L" BCE":L" CE")):(std::wstring(event.year<0?L"前":L"")+std::to_wstring(std::abs(event.year))+L"年");LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=(int)row;item.pszText=&year[0];ListView_InsertItem(list,&item);ListView_SetItemText(list,(int)row,1,const_cast<LPWSTR>(event.heading(gEnglish).c_str()));ListView_SetItemText(list,(int)row,2,const_cast<LPWSTR>(event.location(gEnglish).c_str()));}
    if(!state->results.empty())ListView_SetItemState(list,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    SendMessageW(list,WM_SETREDRAW,TRUE,0);InvalidateRect(list,nullptr,TRUE);
    std::wstring count=valid?std::to_wstring(state->results.size())+(gEnglish?L" results / ":L" 条结果 / ")+std::to_wstring(gHistoryEvents.size())+(gEnglish?L" events. Search titles, places and summaries.":L" 条事件。可搜索标题、地点与摘要。"):(gEnglish?L"Use whole years; negative = BCE, no year 0. Start must precede end.":L"年份请填整数；负数表示公元前，没有公元0年。起年不能晚于止年。");
    SetWindowTextW(GetDlgItem(window,2112),count.c_str());updateEventSearchDetail(window);
}
static void layoutEventSearch(HWND window){
    RECT r{};GetClientRect(window,&r);int width=uiDip(r.right),height=uiDip(r.bottom),pad=20,inner=width-pad*2,listWidth=std::max(260,(inner-18)*48/100),detailX=pad+listWidth+18,detailWidth=width-detailX-pad;
    auto move=[&](int id,int x,int y,int w,int h){placeUiControl(GetDlgItem(window,id),x,y,w,h);};
    move(2120,pad,18,inner,29);move(2121,pad,54,inner,18);move(2101,pad,76,inner-92,29);move(2110,width-pad-82,76,82,29);
    int categoryWidth=std::max(150,inner-370);move(2122,pad,116,categoryWidth,18);move(2102,pad,137,categoryWidth,260);
    int x=pad+categoryWidth+12;move(2123,x,116,140,18);move(2103,x,137,130,27);x+=142;move(2124,x,116,140,18);move(2104,x,137,130,27);
    move(2112,pad,176,inner,35);int contentTop=218,contentHeight=height-contentTop-68;
    move(2105,pad,contentTop,listWidth,contentHeight);move(2106,detailX,contentTop,detailWidth,contentHeight);
    int buttonY=height-48,buttonGap=7,buttonWidth=std::min(132,(inner-3*buttonGap-78)/3);
    move(2107,pad,buttonY,buttonWidth,29);move(2108,pad+buttonWidth+buttonGap,buttonY,buttonWidth,29);move(2109,pad+2*(buttonWidth+buttonGap),buttonY,buttonWidth,29);move(2111,width-pad-78,buttonY,78,29);
    ListView_SetColumnWidth(GetDlgItem(window,2105),0,uiPx(gEnglish?100:97));ListView_SetColumnWidth(GetDlgItem(window,2105),1,uiPx(std::max(140,listWidth-103)));ListView_SetColumnWidth(GetDlgItem(window,2105),2,uiPx(220));
}
static void refreshEventSearchLanguage(){
    HWND window=gEventSearchWindow;UiDpiScope dpi(window);if(!window||!IsWindow(window))return;auto* state=(EventSearchState*)GetWindowLongPtrW(window,GWLP_USERDATA);if(!state)return;
    state->refreshing=true;int selected=(int)SendDlgItemMessageW(window,2102,CB_GETCURSEL,0,0);std::wstring previous=selected>0&&(size_t)selected<=state->categories.size()?state->categories[selected-1]:L"";
    SetWindowTextW(window,gEnglish?L"Explore historical events":L"历史事件检索");
    struct Label{int id;const wchar_t* zh;const wchar_t* en;};for(const auto& label:std::initializer_list<Label>{{2120,L"历史事件检索",L"Explore historical events"},{2121,L"关键词 · 中英文地名、事件、人物或主题",L"Keywords · event, place, person or theme in either language"},{2122,L"类别",L"Category"},{2123,L"起年（负数为前）",L"From year (− = BCE)"},{2124,L"止年（可留空）",L"To year (optional)"},{2110,L"重置",L"Reset"},{2107,L"查看这一年",L"View year"},{2108,L"定位地图",L"Locate on map"},{2109,L"打开资料来源",L"Open source"},{2111,L"关闭",L"Close"}})SetWindowTextW(GetDlgItem(window,label.id),gEnglish?label.en:label.zh);
    SendDlgItemMessageW(window,2101,EM_SETCUEBANNER,TRUE,(LPARAM)(gEnglish?L"e.g. Baghdad, 航海, Darwin":L"例如：巴格达、航海、Darwin"));
    SendDlgItemMessageW(window,2102,CB_RESETCONTENT,0,0);SendDlgItemMessageW(window,2102,CB_ADDSTRING,0,(LPARAM)(gEnglish?L"All categories":L"全部类别"));state->categories.clear();
    for(const auto& event:gHistoryEvents)if(std::find(state->categories.begin(),state->categories.end(),event.category)==state->categories.end())state->categories.push_back(event.category);
    std::sort(state->categories.begin(),state->categories.end());int newSelection=0;
    for(size_t i=0;i<state->categories.size();++i){auto event=std::find_if(gHistoryEvents.begin(),gHistoryEvents.end(),[&](const HistoryEvent& e){return e.category==state->categories[i];});SendDlgItemMessageW(window,2102,CB_ADDSTRING,0,(LPARAM)event->kind(gEnglish).c_str());if(previous==state->categories[i])newSelection=(int)i+1;}
    SendDlgItemMessageW(window,2102,CB_SETCURSEL,newSelection,0);
    for(int column=0;column<3;++column){LVCOLUMNW value{};value.mask=LVCF_TEXT;value.pszText=const_cast<LPWSTR>(column==0?(gEnglish?L"Year":L"年份"):column==1?(gEnglish?L"Event":L"事件"):(gEnglish?L"Place":L"地点"));ListView_SetColumn(GetDlgItem(window,2105),column,&value);}
    state->refreshing=false;refreshUiChildFonts(window);layoutEventSearch(window);filterEventSearch(window);
}
static LRESULT CALLBACK eventSearchProc(HWND window,UINT message,WPARAM a,LPARAM b){
    UiDpiScope dpi(window,message==WM_DPICHANGED?LOWORD(a):0);
    if(message==WM_DPICHANGED||message==UI_SCALE_CHANGED){refreshUiChildFonts(window);SendDlgItemMessageW(window,2106,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(uiPx(12),uiPx(12)));applyUiWindowBounds(window,message,a,b);layoutEventSearch(window);return 0;}
    if(message==WM_NCCREATE){SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)new EventSearchState);return TRUE;}
    if(message==WM_CREATE){
        HINSTANCE instance=GetModuleHandleW(nullptr);
        auto make=[&](int id,const wchar_t* cls,DWORD style){HWND control=CreateWindowExW(wcscmp(cls,L"EDIT")==0?WS_EX_CLIENTEDGE:0,cls,L"",WS_CHILD|WS_VISIBLE|style,0,0,1,1,window,(HMENU)(INT_PTR)id,instance,nullptr);SendMessageW(control,WM_SETFONT,(WPARAM)uiFontFor(id==2120?22:14,id==2120),FALSE);return control;};
        for(int id:{2120,2121,2122,2123,2124,2112})make(id,L"STATIC",SS_LEFT);
        make(2101,L"EDIT",WS_TABSTOP|ES_AUTOHSCROLL);make(2102,L"COMBOBOX",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL);make(2103,L"EDIT",WS_TABSTOP|ES_AUTOHSCROLL);make(2104,L"EDIT",WS_TABSTOP|ES_AUTOHSCROLL);
        for(int id:{2110,2107,2108,2109,2111})make(id,L"BUTTON",WS_TABSTOP|BS_PUSHBUTTON);
        HWND list=make(2105,WC_LISTVIEWW,WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_BORDER);ListView_SetExtendedListViewStyle(list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);ListView_SetBkColor(list,RGB(12,31,44));ListView_SetTextBkColor(list,RGB(12,31,44));ListView_SetTextColor(list,RGB(230,244,246));
        LVCOLUMNW column{};column.mask=LVCF_TEXT|LVCF_WIDTH;column.cx=100;column.pszText=const_cast<LPWSTR>(L"");for(int i=0;i<3;++i)ListView_InsertColumn(list,i,&column);
        HWND detail=make(2106,L"EDIT",WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL);SendMessageW(detail,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(uiPx(12),uiPx(12)));
        gEventSearchWindow=window;refreshEventSearchLanguage();return 0;
    }
    if(message==WM_SIZE){layoutEventSearch(window);return 0;}
    if(message==WM_GETMINMAXINFO){auto* info=(MINMAXINFO*)b;uiMinimumWindow(window,info,720,520);return 0;}
    if(message==WM_COMMAND){int id=LOWORD(a),notification=HIWORD(a);
        if((notification==EN_CHANGE&&(id==2101||id==2103||id==2104))||(id==2102&&notification==CBN_SELCHANGE)){filterEventSearch(window);return 0;}
        if(id==2110){auto* state=(EventSearchState*)GetWindowLongPtrW(window,GWLP_USERDATA);state->refreshing=true;for(int control:{2101,2103,2104})SetWindowTextW(GetDlgItem(window,control),L"");SendDlgItemMessageW(window,2102,CB_SETCURSEL,0,0);state->refreshing=false;filterEventSearch(window);SetFocus(GetDlgItem(window,2101));return 0;}
        if(id==2111||id==IDCANCEL){DestroyWindow(window);return 0;}
        auto* event=selectedSearchEvent(window);HWND owner=GetWindow(window,GW_OWNER);
        if(event&&id==2107){int selected=-1;for(int i=0;i<HISTORY_YEAR_COUNT;++i)if(HISTORY_YEARS[i]==event->year){selected=i;break;}if(selected>=0){SendMessageW(gHistoryYearCombo,CB_SETCURSEL,selected,0);SendMessageW(owner,WM_COMMAND,MAKEWPARAM(1010,CBN_SELCHANGE),(LPARAM)gHistoryYearCombo);gShowHistory=true;SendMessageW(gHistoryToggleButton,BM_SETCHECK,BST_CHECKED,0);gNotice=(gEnglish?L"Event year: ":L"事件年份：")+eventYearLabel(*event);InvalidateRect(owner,nullptr,FALSE);}return 0;}
        if(event&&id==2108&&event->hasCoordinate){int row=std::max(0,std::min(GRID_ROWS-1,(int)std::floor((90-event->latitude)/GRID_CELL_DEGREES))),col=std::max(0,std::min(GRID_COLUMNS-1,(int)std::floor((event->longitude+180)/(360.0/GRID_COLUMNS))));selectSearchResultOnMap(SearchMatch(row,col,event->latitude,event->longitude));gMapCenterX=(event->longitude+180)/360;gMapCenterY=(90-event->latitude)/180;clampMapView();gInfoTab=0;layoutControls(owner);gNotice=(gEnglish?L"Event location reference: ":L"事件位置参考：")+event->heading(gEnglish);gMapSceneCacheValid=false;InvalidateRect(owner,nullptr,FALSE);return 0;}
        if(event&&id==2109){auto result=(INT_PTR)ShellExecuteW(window,L"open",event->source.c_str(),nullptr,nullptr,SW_SHOWNORMAL);if(result<=32)MessageBoxW(window,event->source.c_str(),gEnglish?L"Source link":L"资料链接",MB_OK);return 0;}
    }
    if(message==WM_NOTIFY){auto* note=(NMHDR*)b;if(note->idFrom==2105&&note->code==LVN_ITEMCHANGED){updateEventSearchDetail(window);return 0;}}
    if(message==WM_CTLCOLOREDIT||message==WM_CTLCOLORSTATIC||message==WM_CTLCOLORLISTBOX){HDC dc=(HDC)a;SetTextColor(dc,uiTheme::text);SetBkColor(dc,RGB(18,43,57));return (LRESULT)gUiControlBrush;}
    if(message==WM_ERASEBKGND){RECT rect{};GetClientRect(window,&rect);FillRect((HDC)a,&rect,gUiPanelBrush);return 1;}
    if(message==WM_CLOSE){DestroyWindow(window);return 0;}
    if(message==WM_NCDESTROY){delete (EventSearchState*)GetWindowLongPtrW(window,GWLP_USERDATA);SetWindowLongPtrW(window,GWLP_USERDATA,0);if(gEventSearchWindow==window)gEventSearchWindow=nullptr;}
    return DefWindowProcW(window,message,a,b);
}
static void showEventSearch(HWND owner){
    UiDpiScope dpi(owner);
    if(gEventSearchWindow&&IsWindow(gEventSearchWindow)){ShowWindow(gEventSearchWindow,SW_RESTORE);SetForegroundWindow(gEventSearchWindow);SetFocus(GetDlgItem(gEventSearchWindow,2101));return;}
    HINSTANCE instance=GetModuleHandleW(nullptr);WNDCLASSW cls{};cls.hIcon=(HICON)LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(101),IMAGE_ICON,0,0,LR_SHARED|LR_DEFAULTSIZE);cls.lpfnWndProc=eventSearchProc;cls.hInstance=instance;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.lpszClassName=L"WorldMapExplorerEventSearch";if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
    MONITORINFO monitor{sizeof(MONITORINFO)};GetMonitorInfoW(MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST),&monitor);int width=std::min(uiPx(1020),(int)(monitor.rcWork.right-monitor.rcWork.left)-uiPx(40)),height=std::min(uiPx(760),(int)(monitor.rcWork.bottom-monitor.rcWork.top)-uiPx(40));
    HWND window=CreateWindowExW(WS_EX_CONTROLPARENT,cls.lpszClassName,L"",WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPCHILDREN,monitor.rcWork.left+uiPx(20),monitor.rcWork.top+uiPx(20),width,height,owner,nullptr,instance,nullptr);if(window)SetFocus(GetDlgItem(window,2101));
}
