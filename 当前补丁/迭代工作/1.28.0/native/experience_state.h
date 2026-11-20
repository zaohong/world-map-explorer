#pragma once
#include "explorer_navigation.h"
#include "place_catalog.h"
#include "saved_places.h"
#include "saved_places_store.h"
enum class ExplorerWorkspace { Browse,History,Routes };
static ExplorerWorkspace gWorkspace=ExplorerWorkspace::Browse;
static int gExperiencePanel=0; // 0 closed, 1 thematic layers, 2 display preferences.
static HWND gWorkspaceButtons[3]{},gBasemapMenuButton=nullptr,gLayersMenuButton=nullptr,gDisplayMenuButton=nullptr;
static HWND gBackButton=nullptr,gForwardButton=nullptr,gPreviousYearButton=nullptr,gNextYearButton=nullptr,gHistoryScrub=nullptr,gRouteScrub=nullptr;
static HWND gSavedPlacesButton=nullptr,gBookmarkButton=nullptr,gRecommendedButtons[6]{};
static RECT gExperiencePanelRect{},gExperienceContextRect{},gExperienceStatusRect{};
static int gPendingHistorySelection=-1;
static const UINT HISTORY_SEEK_TIMER=1090,NAVIGATION_CAPTURE_TIMER=1091;
static std::wstring gSelectedPlaceTitle;
static explorerNavigation::History gViewHistory;
static placeCatalog::Catalog gPlaceCatalog;
static std::vector<savedPlaces::Place> gSavedPlaces;
static std::vector<savedPlaces::Undo> gSavedPlacesUndo;
static const UINT SAVED_PLACES_WATCH_TIMER=1093;
static bool syncSavedPlacesFromDisk(HWND);
static void refreshUnifiedSavedResults();
static void renameSavedPlace(HWND,const savedPlaces::Place&);
static void removeSavedPlace(HWND,const savedPlaces::Place&);
static void undoSavedPlace(HWND);
static bool applySavedPlaceChange(HWND,const savedPlaces::Request&,bool remember=true);
static bool gSavedPlacesUnreadable=false;
static std::wstring gSelectedPlaceOrigin;
static HWND gRestoreSessionButton=nullptr,gStartFreshButton=nullptr;
static bool gRestoreLastView=true,gSessionUnreadable=false,gSessionValidated=false,gSessionReady=false;
static void restoreLastView(HWND);
static void saveLastView();
static void startFreshExploration(HWND);
static void layoutControls(HWND);
static void syncExperienceControls();
static void rememberView();
static void showUnifiedSearch(HWND,const std::wstring& query=L"");
static void activateRecommendedPlace(HWND,int);
static void togglePlaceBookmark(HWND);
static void showSavedPlaces(HWND);
static void refreshUnifiedSearchLanguage();
static void refreshSelectedReferenceLanguage();
static void paintStartExperience(HDC,int,int,int,int);
static void activateWorkspace(HWND,ExplorerWorkspace);
static bool handleExperienceMessage(HWND,UINT,WPARAM,LPARAM);
static std::vector<HWND> experienceKeyboardControls(HWND);

static savedPlaces::Status gLastSavedPlaceStatus=savedPlaces::Status::Unchanged;
static void updateSelectedSavedLabel();
