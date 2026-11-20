#pragma once

#include "route_geometry.h"
#include <string>
#include <vector>

namespace routes {
enum class WaypointKind { Milestone, Auxiliary, Uncertain, LoggedPosition, ReferenceLocality };
enum class TravelMode { Land, Sea, Mixed, Polar };
struct Stop {
    Point position;
    const wchar_t* zh;
    const wchar_t* en;
    WaypointKind kind=WaypointKind::Milestone;
    const wchar_t* date=L"";
};

struct Campaign {
    const char* id;
    const wchar_t* zh;
    const wchar_t* en;
    const wchar_t* period;
    const wchar_t* noteZh;
    const wchar_t* noteEn;
    std::vector<const char*> sources;
    std::vector<Stop> stops;
    TravelMode mode=TravelMode::Land;
    const char* category="campaign";
};

#include "beagle_stops.generated.h"
#include "route_additions.h"

inline std::vector<Point> pointsFor(const Campaign& campaign) {
    std::vector<Point> points;
    points.reserve(campaign.stops.size());
    for (const auto& stop : campaign.stops) points.push_back(stop.position);
    return points;
}

// Draft waypoint selections. Positions denote approximate modern locations;
// straight connections do not claim to be reconstructed historic roads.
inline const std::vector<Campaign>& catalog() {
    static const std::vector<Campaign> entries = {
        {
            "alexander_east", L"亚历山大东征", L"Alexander's eastern campaign", L"334–326 BCE",
            L"主要节点示意；中间道路和部分古战场位置存在不确定性。",
            L"Selected milestones; intervening roads and some ancient battle sites are uncertain.",
            {"https://alexander-ancien-atlas.com/", "https://stuff.mit.edu/afs/athena.mit.edu/course/21/21h.401/www/local/talbert_ach.Gr.2.pdf", "https://www.iranicaonline.org/articles/alexander-the-great-356-23-bc/", "https://www.cambridge.org/core/journals/classical-quarterly/article/athenian-archon-dates-in-arrian/10C431BEC10BED53AD052BE1A5FE8B24", "https://penelope.uchicago.edu/thayer/e/roman/texts/strabo/15b%2A.html"},
            {{{40.24,26.65},L"赫勒斯滂海峡",L"Hellespont"},
             {{40.28,27.35},L"格拉尼库斯河附近",L"Granicus area"},
             {{36.85,36.20},L"伊苏斯附近",L"Issus area"},
             {{33.27,35.20},L"推罗",L"Tyre"},
             {{31.50,34.47},L"加沙",L"Gaza"},
             {{31.04,32.55},L"埃及东部沿海地区（示意）",L"Eastern Egyptian coast (schematic)"},
             {{29.85,31.25},L"孟菲斯附近",L"Memphis area"},
             {{31.20,29.92},L"亚历山大城",L"Alexandria"},
             {{29.85,31.25},L"孟菲斯附近（返回）",L"Memphis area (return)"},
             {{31.04,32.55},L"埃及东部沿海地区（东返示意）",L"Eastern Egyptian coast (return, schematic)"},
             {{31.50,34.47},L"加沙地区（东返示意）",L"Gaza area (return east, schematic)"},
             {{33.27,35.20},L"推罗（东返）",L"Tyre (return east)"},
             {{35.98,38.10},L"幼发拉底河渡口地区（位置有争议）",L"Euphrates crossing area (disputed)"},
             {{36.36,43.25},L"高加米拉附近",L"Gaugamela area"},
             {{32.54,44.42},L"巴比伦",L"Babylon"},
             {{32.19,48.26},L"苏萨",L"Susa"},
             {{29.93,52.89},L"波斯波利斯",L"Persepolis"},
             {{34.81,48.52},L"埃克巴坦那",L"Ecbatana"},
             {{35.59,51.44},L"拉盖地区（示意）",L"Rhagae area (schematic)"},
             {{36.40,54.90},L"帕提亚地区（示意）",L"Parthia area (schematic)"},
             {{34.35,62.20},L"阿里亚（赫拉特地区）",L"Aria (Herat area)"},
             {{32.37,62.12},L"德兰吉亚纳地区（示意）",L"Drangiana area (schematic)"},
             {{31.63,65.74},L"阿拉霍西亚（坎大哈地区）",L"Arachosia (Kandahar area)"},
             {{34.53,69.17},L"兴都库什南麓地区（示意）",L"South of the Hindu Kush (schematic)"},
             {{36.76,66.90},L"巴克特拉",L"Bactra"},
             {{39.65,66.96},L"马拉坎达",L"Maracanda"},
             {{36.76,66.90},L"巴克特拉地区（南返示意）",L"Bactra area (return south, schematic)"},
             {{34.53,69.17},L"兴都库什南麓地区（示意）",L"South of the Hindu Kush (schematic)"},
             {{34.01,71.58},L"白沙瓦盆地（示意）",L"Peshawar valley (schematic)"},
             {{33.75,72.84},L"塔克西拉",L"Taxila"},
             {{32.81,73.65},L"希达斯皮斯河附近",L"Hydaspes area"}}
        },
        {
            "hannibal_italy", L"汉尼拔进军意大利", L"Hannibal's march into Italy", L"218–216 BCE",
            L"阿尔卑斯山穿越地点仍有争议；山地节点仅作区域示意。",
            L"The Alpine crossing is disputed; the mountain waypoint is schematic.",
            {"https://www.yorku.ca/bmahaney/books/html/maps.htm", "https://discovery.ucl.ac.uk/id/eprint/1547046/1/Mahaney_Biostratigraphic_Evidence_Relating_Hannibal_1.pdf"},
            {{{37.60,-0.98},L"新迦太基",L"New Carthage"},
             {{40.72,0.58},L"埃布罗河下游",L"Lower Ebro"},
             {{42.45,2.86},L"比利牛斯山地区（示意）",L"Pyrenees area (schematic)"},
             {{43.95,4.81},L"罗讷河下游",L"Lower Rhône"},
             {{45.10,6.85},L"阿尔卑斯山区域（示意）",L"Alps (schematic)"},
             {{45.19,7.65},L"意大利北部",L"Northern Italy"},
             {{45.07,9.63},L"特雷比亚河附近",L"Trebia area"},
             {{43.13,12.10},L"特拉西梅诺湖",L"Lake Trasimene"},
             {{41.30,16.15},L"坎尼附近",L"Cannae area"}}
        },
        {
            "long_march_first", L"中央红军长征", L"Central Red Army's Long March", L"1934–1935",
            L"概括中央红军主要地点及四渡赤水阶段；各部队分路、不同渡口和具体道路未逐线复原，安顺场至泸定桥沿大渡河两岸分路。",
            L"Selected locations and four Chishui River crossing stages. Separate columns, crossing points and roads are simplified; forces advanced on both banks from Anshunchang to Luding.",
            {"https://www.ynds.yn.gov.cn/html/dangshiyanjiu/2016/9/27/648c61d7-8203-4eff-a6d5-9b6ddb3d9915.html", "https://www.xingguo.gov.cn/xgxxxgk/xg10305/202506/4baafde86b27447ab8eedd0907522d64/files/6eeed8670367414ca7c7bf3d1feef45d.pdf", "https://www.getty.edu/vow/TGNFullDisplay?subjectid=8066741", "https://www.openstreetmap.org/node/2277373710", "https://www.mod.gov.cn/gfbw/gfjy_index/js_214151/4850116.html", "https://www.mod.gov.cn/gfbw/gfjy_index/4845760.html", "https://www.mod.gov.cn/gfbw/tp_214132/tsjs/4878950.html"},
            {{{25.88,116.03},L"瑞金",L"Ruijin"},
             {{25.95,115.41},L"于都",L"Yudu"},
             {{25.93,111.07},L"湘江战役地区",L"Xiang River battle area"},
             {{27.73,106.93},L"遵义",L"Zunyi"},
             {{28.2765,106.0026},L"一渡赤水·土城地区",L"First Chishui crossing: Tucheng area"},
             {{27.844,105.045},L"扎西地区",L"Zaxi area"},
             {{28.1412,106.0401},L"二渡赤水·太平渡附近",L"Second Chishui crossing: Taipingdu area"},
             {{27.73,106.93},L"遵义地区（再占）",L"Zunyi area (retaken)"},
             {{27.855,106.368},L"三渡赤水·茅台地区",L"Third Chishui crossing: Maotai area"},
             {{28.1412,106.0401},L"四渡赤水·太平渡附近",L"Fourth Chishui crossing: Taipingdu area"},
             {{26.292,102.382},L"皎平渡附近",L"Jiaopingdu area"},
             {{29.23,102.25},L"安顺场",L"Anshunchang"},
             {{29.91,102.23},L"泸定桥",L"Luding Bridge"},
             {{30.8605,102.6829},L"夹金山垭口附近",L"Jiajin Pass area"},
             {{31.00,102.36},L"懋功（今小金）",L"Maogong (Xiaojin)"},
             {{34.08,103.96},L"腊子口",L"Lazikou"},
             {{36.93,108.18},L"吴起镇",L"Wuqi"}}
        },
        {
            "napoleon_russia", L"拿破仑1812年俄国战役", L"Napoleon's 1812 Russian campaign", L"1812",
            L"展示主要进军与撤退节点；不代表所有军团的行军路线。",
            L"Selected advance and retreat milestones; corps followed different routes.",
            {"https://www.napoleon.org/en/history-of-the-two-empires/timelines/napoleons-russian-campaign-from-the-niemen-to-moscow/", "https://www.napoleon.org/en/history-of-the-two-empires/timelines/napoleons-russian-campaign-the-retreat/"},
            {{{54.90,23.90},L"涅曼河附近",L"Niemen crossing area"},
             {{54.69,25.28},L"维尔纽斯",L"Vilnius"},
             {{55.19,30.20},L"维捷布斯克",L"Vitebsk"},
             {{54.78,32.05},L"斯摩棱斯克",L"Smolensk"},
             {{55.53,35.82},L"博罗季诺",L"Borodino"},
             {{55.75,37.62},L"莫斯科",L"Moscow"},
             {{55.01,36.46},L"马洛亚罗斯拉韦茨",L"Maloyaroslavets"},
             {{54.78,32.05},L"斯摩棱斯克（撤退）",L"Smolensk (retreat)"},
             {{54.51,30.42},L"奥尔沙",L"Orsha"},
             {{54.30,28.45},L"别列津纳河附近",L"Berezina crossing area"},
             {{54.69,25.28},L"维尔纽斯（撤退）",L"Vilnius (retreat)"}}
        },
        {
            "first_crusade", L"第一次十字军东征（亚洲段）", L"First Crusade: Anatolia to Jerusalem", L"1097–1099",
            L"按一种重建示意君士坦丁堡至耶路撒冷的地点；安纳托利亚段有不同解释，分队与道路未逐一复原。",
            L"Selected places from Constantinople to Jerusalem follow one reconstruction; the Anatolian route is debated, and separate forces and roads are simplified.",
            {"https://www.animatedmaps.div.ed.ac.uk/Divinity2/first.html", "https://www.deremilitari.org/RESOURCES/SOURCES/firstcrusade.htm", "https://erenow.org/ww/victory-east-military-history-first-crusade/6.php", "https://ccel.org/ccel/schaff/hcc5/hcc5.ii.ix.iv.html"},
            {{{41.01,28.98},L"君士坦丁堡",L"Constantinople"},
             {{40.43,29.72},L"尼西亚",L"Nicaea"},
             {{39.78,30.52},L"多利莱乌姆附近",L"Dorylaeum area"},
             {{37.87,32.49},L"伊科尼乌姆",L"Iconium"},
             {{37.51,34.05},L"赫拉克利亚附近",L"Heraclea area"},
             {{38.73,35.49},L"卡帕多西亚凯撒利亚",L"Caesarea in Cappadocia"},
             {{37.57,36.93},L"马拉什地区",L"Marash area"},
             {{36.20,36.16},L"安条克",L"Antioch"},
             {{35.65,36.68},L"马阿拉特努曼",L"Ma'arrat al-Nu'man"},
             {{34.43,35.84},L"的黎波里地区",L"Tripoli area"},
             {{33.89,35.50},L"贝鲁特地区",L"Beirut area"},
             {{32.50,34.89},L"沿海凯撒利亚地区",L"Coastal Caesarea area"},
             {{31.93,34.87},L"拉姆拉地区",L"Ramla area"},
             {{31.78,35.23},L"耶路撒冷",L"Jerusalem"}}
        },
        {
            "napoleon_egypt", L"拿破仑埃及与叙利亚战役", L"Bonaparte in Egypt and Syria", L"1798–1799",
            L"陆上主要节点示意；省略船队航线和各分队行动。",
            L"Selected land campaign milestones; fleet movements and detachments are omitted.",
            {"https://www.napoleon.org/en/history-of-the-two-empires/articles/the-campaign-in-egypt/", "https://www.napoleon.org/en/young-historians/napodoc/bonaparte-in-egypt-1-the-military-campaign"},
            {{{31.20,29.92},L"亚历山大城",L"Alexandria"},
             {{30.01,31.21},L"金字塔战役地区",L"Battle of the Pyramids area"},
             {{30.04,31.24},L"开罗",L"Cairo"},
             {{31.13,33.80},L"阿里什",L"El Arish"},
             {{31.50,34.47},L"加沙",L"Gaza"},
             {{32.05,34.75},L"雅法",L"Jaffa"},
             {{32.93,35.08},L"阿卡",L"Acre"},
             {{32.05,34.75},L"雅法（撤退）",L"Jaffa (retreat)"},
             {{31.13,33.80},L"阿里什（撤退）",L"El Arish (retreat)"},
             {{30.04,31.24},L"开罗（返回）",L"Cairo (return)"}}
        },
        {
            "yorktown", L"约克镇战役：部分联军水路", L"Yorktown campaign: an Allied water route", L"1781",
            L"示意费城以南部分联军经切萨皮克湾运输与集结；不是华盛顿、罗尚博本人或全军的统一行程。",
            L"Schematic transport and assembly of some Allied troops south of Philadelphia via Chesapeake Bay; not the personal itinerary of Washington and Rochambeau or of all troops.",
            {"https://transportation.army.mil/history/yorktown.html", "https://history.army.mil/portals/143/Images/Publications/catalog/30-21.pdf", "https://www.nps.gov/waro/learn/management/foundation-document.htm"},
            {{{39.95,-75.16},L"费城",L"Philadelphia"},
             {{39.61,-75.83},L"埃尔克河口附近",L"Head of Elk area"},
             {{38.70,-76.30},L"切萨皮克湾（示意航段）",L"Chesapeake Bay (schematic)"},
             {{37.00,-76.45},L"詹姆斯河口附近",L"James River mouth area"},
             {{37.27,-76.71},L"威廉斯堡",L"Williamsburg"},
             {{37.24,-76.51},L"约克镇",L"Yorktown"}}
        },
        {
            "lewis_clark", L"刘易斯与克拉克远征（西行）", L"Lewis and Clark expedition: westbound", L"1804–1805",
            L"西行主要节点示意；河道弯曲、越山路线和返程分队未精确复原。",
            L"Westbound milestones; river bends, mountain trails and return detachments are simplified.",
            {"https://www.nps.gov/lewi/learn/historyculture/index.htm", "https://www.nps.gov/teachers/classrooms/upload/TwHP-Lessons_108lewisclark.pdf", "https://www.nps.gov/subjects/travellewisandclark/expedition-by-date.htm", "https://home.nps.gov/mnrr/learn/historyculture/the-lewis-and-clark-expedition.htm", "https://www.nps.gov/places/three-forks-of-the-missouri-mt.htm"},
            {{{38.80,-90.09},L"杜布瓦河营地",L"Camp River Dubois"},
             {{38.95,-92.56},L"罗什波特附近",L"Rocheport area"},
             {{39.11,-94.60},L"堪萨斯河汇合处附近",L"Kansas–Missouri confluence area"},
             {{41.46,-96.02},L"议会崖附近",L"Council Bluff area"},
             {{42.50,-96.40},L"苏城附近",L"Sioux City area"},
             {{44.37,-100.35},L"皮尔附近",L"Pierre area"},
             {{47.30,-101.00},L"曼丹堡附近",L"Fort Mandan area"},
             {{47.99,-104.04},L"黄石河与密苏里河汇合处附近",L"Yellowstone–Missouri confluence area"},
             {{47.82,-110.45},L"玛丽亚斯河与密苏里河汇合处附近",L"Marias–Missouri confluence area"},
             {{47.50,-111.30},L"密苏里河大瀑布",L"Great Falls of the Missouri"},
             {{45.93,-111.51},L"密苏里河三岔口",L"Three Forks of the Missouri"},
             {{44.98,-113.45},L"莱姆希山口附近",L"Lemhi Pass area"},
             {{46.63,-114.58},L"洛洛山口附近",L"Lolo Pass area"},
             {{46.42,-117.02},L"克利尔沃特河与斯内克河汇合处",L"Clearwater–Snake confluence"},
             {{46.21,-119.04},L"哥伦比亚河与斯内克河汇合处",L"Columbia–Snake confluence"},
             {{46.13,-123.88},L"克拉特索普堡",L"Fort Clatsop"}}
        },
        {
            "sherman_sea", L"谢尔曼向海洋进军", L"Sherman's March to the Sea", L"1864",
            L"亚特兰大至萨凡纳的主要地区示意；联邦军分多路推进。",
            L"Selected areas between Atlanta and Savannah; Union forces advanced in separate columns.",
            {"https://history.army.mil/Research/Reference-Topics/Army-Campaigns/Brief-Summaries/Civil-War/", "https://history.army.mil/portals/143/Images/Publications/catalog/75-13.pdf"},
            {{{33.75,-84.39},L"亚特兰大",L"Atlanta"},
             {{33.08,-83.23},L"米利奇维尔",L"Milledgeville"},
             {{32.98,-82.81},L"桑德斯维尔附近",L"Sandersville area"},
             {{32.08,-81.09},L"萨凡纳",L"Savannah"}}
        },
        {
            "waterloo_1815", L"拿破仑1815年比利时战役", L"Napoleon's 1815 Belgian campaign", L"1815",
            L"示意拿破仑从巴黎出发至滑铁卢的部分节点；各军团分路未逐一绘出。",
            L"Selected milestones from Paris to Waterloo; individual corps took different routes.",
            {"https://www.napoleon.org/histoire-des-2-empires/chronologies/vie-et-regne-de-napoleon/", "https://www.napoleon.org/magazine/lieux/la-route-napoleon-en-wallonie/", "https://napoleon-histoire.com/correspondance-de-napoleon-juin-1815/3/"},
            {{{48.86,2.35},L"巴黎",L"Paris"},
             {{50.24,4.24},L"博蒙",L"Beaumont"},
             {{50.41,4.44},L"沙勒罗瓦",L"Charleroi"},
             {{50.48,4.55},L"弗勒吕斯",L"Fleurus"},
             {{50.51,4.57},L"利尼",L"Ligny"},
             {{50.57,4.45},L"卡特尔布拉地区（6月17日）",L"Quatre Bras area (17 June)"},
             {{50.61,4.45},L"热纳普地区",L"Genappe area"},
             {{50.68,4.41},L"滑铁卢战场附近",L"Waterloo battlefield area"}}
        },
        {
            "joan_loire", L"贞德与卢瓦尔河战役", L"Joan of Arc and the Loire campaign", L"1429",
            L"展示从奥尔良至兰斯的主要事件地点；地点间连线不代表考证的全部行军道路。",
            L"Selected events from Orléans to Reims; connecting lines are schematic.",
            {"https://www.worldhistory.org/Joan_of_Arc/", "https://www.worldhistory.org/timeline/france/", "https://www.joanofarcsociety.org/wp-content/uploads/2023/11/joan.pdf"},
            {{{47.90,1.91},L"奥尔良",L"Orléans"},
             {{47.87,2.12},L"雅尔若",L"Jargeau"},
             {{47.83,1.70},L"默恩",L"Meung-sur-Loire"},
             {{47.78,1.63},L"博让西",L"Beaugency"},
             {{48.05,1.70},L"帕泰",L"Patay"},
             {{47.69,2.63},L"日安",L"Gien"},
             {{47.80,3.57},L"欧塞尔地区",L"Auxerre area"},
             {{48.30,4.08},L"特鲁瓦",L"Troyes"},
             {{48.95,4.36},L"沙隆",L"Châlons"},
             {{49.26,4.03},L"兰斯",L"Reims"}}
        },
        {
            "leclerc_paris", L"法国第2装甲师赴巴黎", L"French 2nd Armoured Division to Paris", L"1944",
            L"示意比洛特战斗群的部分地点；师内各战斗群分路行进，巴黎南郊又分为多个分队。",
            L"Selected locations on the Billotte group's advance; division groups took different routes and split again south of Paris.",
            {"https://www.defense.gouv.fr/chemins-memoire/histoire-memoires/ressources-historiques/seconde-guerre-mondiale/resister/france-libre/aout-1944", "https://sevres-92310.fr/doc/1939-1945_Occupation%20et%20Lib%C3%A9ration_VOLUME%202-Saclay.pdf", "https://museedelaresistanceenligne.org/expo.php?expo=84&theme=155"},
            {{{48.43,0.09},L"阿朗松",L"Alençon"},
             {{48.35,0.37},L"马梅尔",L"Mamers"},
             {{48.37,0.56},L"贝莱姆",L"Bellême"},
             {{48.32,0.82},L"诺让勒罗特鲁",L"Nogent-le-Rotrou"},
             {{48.45,1.49},L"沙特尔",L"Chartres"},
             {{48.52,1.84},L"阿布利",L"Ablis"},
             {{48.64,1.83},L"朗布依埃",L"Rambouillet"},
             {{48.65,2.08},L"利穆尔",L"Limours"},
             {{48.59,2.25},L"阿尔帕容",L"Arpajon"},
             {{48.70,2.30},L"隆瑞莫",L"Longjumeau"},
             {{48.75,2.30},L"安东尼",L"Antony"},
             {{48.86,2.35},L"巴黎",L"Paris"}}
        },
        {
            "magellan_elcano", L"麦哲伦—埃尔卡诺环球航行", L"Magellan–Elcano circumnavigation", L"1519–1522",
            L"海上航程按关键地点示意；麦哲伦在菲律宾遇难，埃尔卡诺率维多利亚号完成航行。",
            L"Sea voyage shown through selected milestones; Magellan died in the Philippines and Elcano completed the voyage aboard Victoria.",
            {"https://www.loc.gov/item/today-in-history/november-28/", "https://blogs.loc.gov/international-collections/2021/10/negotiating-empire-part-i-from-magellan-to-the-founding-of-manila-16th-18th-centuries/", "https://tile.loc.gov/storage-services/master/gdc/gdcebookspublic/20/20/71/95/78/2020719578/2020719578.pdf", "https://tile.loc.gov/storage-services/service/ndnp/prru/batch_prru_bacalao_ver04/data/2013201074/00295865313/1862042401/0169.pdf"},
            {{{36.78,-6.35},L"桑卢卡尔",L"Sanlúcar"},
             {{28.46,-16.25},L"加那利群岛",L"Canary Islands"},
             {{12,-23},L"海上路径辅助点",L"Sea connection guide"},
             {{0,-28},L"海上路径辅助点",L"Sea connection guide"},
             {{-10,-31},L"海上路径辅助点",L"Sea connection guide"},
             {{-18,-37},L"海上路径辅助点",L"Sea connection guide"},
             {{-23.5,-41.5},L"海上路径辅助点",L"Sea connection guide"},
             {{-22.91,-43.17},L"里约热内卢附近",L"Rio de Janeiro area"},
             {{-33,-51},L"海上路径辅助点",L"Sea connection guide"},
             {{-42,-61},L"海上路径辅助点",L"Sea connection guide"},
             {{-47,-65},L"海上路径辅助点",L"Sea connection guide"},
             {{-48.7,-66.8},L"海上路径辅助点",L"Sea connection guide"},
             {{-49.30,-67.72},L"圣胡利安港",L"Port San Julián"},
             {{-52.5,-68.0},L"麦哲伦海峡东口附近",L"Eastern Strait of Magellan"},
             {{-52.6,-69.6},L"海峡狭窄段（示意）",L"Strait narrows (schematic)"},
             {{-53.60,-70.90},L"麦哲伦海峡",L"Strait of Magellan"},
             {{-53.9,-71.3},L"海峡南段（示意）",L"Southern strait (schematic)"},
             {{-53.4,-72.8},L"海峡西段（示意）",L"Western strait (schematic)"},
             {{-52.8,-74.0},L"海峡西口附近（示意）",L"Western strait entrance (schematic)"},
             {{-52.8,-76.0},L"太平洋出口外（示意）",L"Off the Pacific exit (schematic)"},
             {{-30,-110},L"南太平洋（示意航段）",L"South Pacific (schematic)"},
             {{-10,-145},L"太平洋（示意航段）",L"Pacific (schematic)"},
             {{5,175},L"西太平洋（示意航段）",L"Western Pacific (schematic)"},
             {{13.44,144.79},L"关岛",L"Guam"},
             {{10.75,125.72},L"霍蒙洪岛附近",L"Homonhon area"},
             {{10.4,125.6},L"海上路径辅助点",L"Sea connection guide"},
             {{9.5,125.3},L"海上路径辅助点",L"Sea connection guide"},
             {{9.93,125.08},L"利马萨瓦附近",L"Limasawa area"},
             {{9.2,124.7},L"海上路径辅助点",L"Sea connection guide"},
             {{9.8,124.2},L"海上路径辅助点",L"Sea connection guide"},
             {{10.31,123.98},L"宿务与麦克坦附近",L"Cebu and Mactan area"},
             {{9.1,124.2},L"薄荷海（示意航段）",L"Bohol Sea (schematic)"},
             {{8.0,120.5},L"苏禄海（示意航段）",L"Sulu Sea (schematic)"},
             {{9.5,118.4},L"巴拉望附近",L"Palawan area"},
             {{4.9,114.9},L"文莱附近",L"Brunei area"},
             {{5.4,114.3},L"海上路径辅助点",L"Sea connection guide"},
             {{6.6,115.4},L"海上路径辅助点",L"Sea connection guide"},
             {{7.4,116.7},L"海上路径辅助点",L"Sea connection guide"},
             {{7,117.5},L"婆罗洲北端外（示意）",L"Off northern Borneo (schematic)"},
             {{5,122},L"西里伯斯海（示意航段）",L"Celebes Sea (schematic)"},
             {{3,125},L"苏拉威西以北（示意）",L"North of Sulawesi (schematic)"},
             {{0.68,127.40},L"马鲁古群岛",L"Maluku Islands"},
             {{0.8,126.5},L"海上路径辅助点",L"Sea connection guide"},
             {{-1,126.5},L"海上路径辅助点",L"Sea connection guide"},
             {{-3.5,127.8},L"海上路径辅助点",L"Sea connection guide"},
             {{-5,129},L"班达海（示意航段）",L"Banda Sea (schematic)"},
             {{-8,125},L"海上路径辅助点",L"Sea connection guide"},
             {{-9.5,123.4},L"海上路径辅助点",L"Sea connection guide"},
             {{-10,123.2},L"海上路径辅助点",L"Sea connection guide"},
             {{-10.17,123.59},L"帝汶附近",L"Timor area"},
             {{-22,95},L"海上路径辅助点",L"Sea connection guide"},
             {{-33,50},L"海上路径辅助点",L"Sea connection guide"},
             {{-36,25},L"海上路径辅助点",L"Sea connection guide"},
             {{-36,19},L"海上路径辅助点",L"Sea connection guide"},
             {{-34.36,18.47},L"好望角附近",L"Cape of Good Hope area"},
             {{-33.50,14.00},L"南非西部海域（示意航段）",L"Off western South Africa (schematic)"},
             {{-20.00,9.00},L"南大西洋东部（示意航段）",L"Eastern South Atlantic (schematic)"},
             {{-5.00,2.00},L"几内亚湾外海（示意航段）",L"Off the Gulf of Guinea (schematic)"},
             {{4.00,-4.00},L"西非近海（示意航段）",L"Off West Africa (schematic)"},
             {{3,-12},L"海上路径辅助点",L"Sea connection guide"},
             {{8,-20},L"海上路径辅助点",L"Sea connection guide"},
             {{15.00,-23.65},L"佛得角群岛",L"Cape Verde islands"},
             {{36.78,-6.35},L"桑卢卡尔（返回）",L"Sanlúcar (return)"}}
        },
        {
            "zheng_he_first", L"郑和第一次下西洋", L"Zheng He's first voyage", L"1405–1407",
            L"海上访问地点与航段示意；船队可能分航，地点间连线及访问顺序并非精确船迹。巨港河道未复原，港口参考点与外海分开绘制。",
            L"Selected ports and schematic sea legs; ships may have separated and the exact tracks and order are uncertain. Palembang river access is not reconstructed; the port is disconnected from the sea legs.",
            {"https://www.wti.ac.cn/wti/zhls/7351.jhtml", "https://www.cass.cn/xueshuchengguo/xscg_lsyjy/201912/t20191202_5051930.shtml", "https://www.gdwsw.gov.cn/shsy/content/post_22654.html"},
            {{{31.48,121.24},L"刘家港附近",L"Liujiagang area"},
             {{31.3,122.1},L"海上路径辅助点",L"Sea connection guide"},
             {{29.5,123.1},L"海上路径辅助点",L"Sea connection guide"},
             {{28,122},L"海上路径辅助点",L"Sea connection guide"},
             {{26.1,120.4},L"海上路径辅助点",L"Sea connection guide"},
             {{26.00,119.60},L"福州沿海",L"Fuzhou coast"},
             {{26.1,120.4},L"海上路径辅助点",L"Sea connection guide"},
             {{24,119.5},L"海上路径辅助点",L"Sea connection guide"},
             {{20,114.5},L"海上路径辅助点",L"Sea connection guide"},
             {{14.5,110.5},L"海上路径辅助点",L"Sea connection guide"},
             {{13.78,109.22},L"占城沿海地区（示意）",L"Champa coast (schematic)"},
             {{13,110.5},L"海上路径辅助点",L"Sea connection guide"},
             {{9.5,110},L"海上路径辅助点",L"Sea connection guide"},
             {{7,108.5},L"南海西部（路径辅助点）",L"Western South China Sea (guide)"},
             {{4,107.5},L"纳土纳群岛以西（路径辅助点）",L"West of Natuna (guide)"},
             {{0,107.5},L"卡里马塔海峡西侧（路径辅助点）",L"Western Karimata Strait (guide)"},
             {{-4.8,110.2},L"爪哇海（路径辅助点）",L"Java Sea (guide)"},
             {{-6.89,112.05},L"爪哇北岸附近",L"Northern Java area"},
             {{-2.98,104.75,true},L"旧港（今巨港）",L"Palembang"},
             {{-1.80,105.40,true},L"苏门答腊东岸外（示意航段）",L"Off eastern Sumatra (schematic)"},
             {{1.00,104.30},L"新加坡海峡附近（示意航段）",L"Singapore Strait area (schematic)"},
             {{1,103.5},L"海上路径辅助点",L"Sea connection guide"},
             {{1.8,102.5},L"海上路径辅助点",L"Sea connection guide"},
             {{2.19,102.25},L"满剌加",L"Malacca"},
             {{2.3,101.7},L"海上路径辅助点",L"Sea connection guide"},
             {{3.2,100.5},L"海上路径辅助点",L"Sea connection guide"},
             {{4.5,99.9},L"海上路径辅助点",L"Sea connection guide"},
             {{4.50,100.10},L"马六甲海峡（示意航段）",L"Malacca Strait (schematic)"},
             {{5.55,95.32},L"苏门答腊北部",L"Northern Sumatra"},
             {{6.03,80.22},L"锡兰南部沿海（示意）",L"Southern Sri Lanka (schematic)"},
             {{6.80,77.20},L"科摩林角以南（示意航段）",L"South of Cape Comorin (schematic)"},
             {{11.26,75.78},L"古里（今科泽科德）",L"Calicut (Kozhikode)"}}
        },
        {
            "mongol_bukhara", L"蒙古军进攻布哈拉与撒马尔罕", L"Mongol advance on Bukhara and Samarkand", L"1219–1220",
            L"仅示意成吉思汗与速不台部队经克孜勒库姆沙漠到布哈拉、撒马尔罕；沙漠内具体行进路线不确定。",
            L"Schematic main-force advance via the Kyzylkum to Bukhara and Samarkand; the desert track is uncertain.",
            {"https://www.armyupress.army.mil/Portals/7/military-review/Archives/English/Dir-Select/genghis-khan-july-1986.pdf"},
            {{{41.20,63.50},L"克孜勒库姆沙漠区域（示意）",L"Kyzylkum area (schematic)"},
             {{39.77,64.42},L"布哈拉",L"Bukhara"},
             {{39.65,66.96},L"撒马尔罕",L"Samarkand"}}
        },
        {
            "vasco_india", L"达·伽马首次赴印度航行", L"Vasco da Gama's first voyage to India", L"1497–1498",
            L"去程主要港口及海上示意点；部分船只停靠地点仅作附近区域示意，连线不代表确切航迹。",
            L"Outbound ports and schematic sea waypoints; some landfalls are approximate and lines do not show the exact track.",
            {"https://sahistory.org.za/article/vasco-da-gamas-voyage-discovery-1497", "https://www.cambridge.org/core/books/abs/worlds-of-the-indian-ocean/portuguese-in-the-indian-ocean/478AC3ACFBA6DE0F35F73C0A97A8D091", "https://www.cambridge.org/core/books/abs/how-transformative-innovations-shaped-the-rise-of-nations/portuguese-caravel-building-an-oceanic-empire/2D6B8F2B27B7E9812F5391FC5E651E2B"},
            {{{38.72,-9.14},L"里斯本",L"Lisbon"},
             {{15.00,-23.65},L"佛得角群岛",L"Cape Verde islands"},
             {{-25.00,-30.00},L"南大西洋航段（示意）",L"South Atlantic (schematic)"},
             {{-34.36,18.47},L"好望角附近",L"Cape of Good Hope area"},
             {{-35.40,20.00},L"非洲南端外海（示意航段）",L"Off southern Africa (schematic)"},
             {{-34.19,22.14},L"莫塞尔湾附近",L"Mossel Bay area"},
             {{-35.00,24.80},L"南非南岸外海（示意航段）",L"Off South Africa's south coast (schematic)"},
             {{-33.30,28.30},L"南非东南岸外海（示意航段）",L"Off southeastern South Africa (schematic)"},
             {{-30.50,31.80},L"纳塔尔海岸外（示意）",L"Off the Natal coast (schematic)"},
             {{-24.45,35.40},L"因哈里梅沿海附近（示意）",L"Inharrime coast area (schematic)"},
             {{-15.04,40.74},L"莫桑比克岛",L"Mozambique Island"},
             {{-10.00,42.00},L"东非近海（示意航段）",L"Off East Africa (schematic)"},
             {{-5.00,41.00},L"东非近海（示意航段）",L"Off East Africa (schematic)"},
             {{-4.05,39.67},L"蒙巴萨",L"Mombasa"},
             {{-3.22,40.12},L"马林迪",L"Malindi"},
             {{11.26,75.78},L"古里（科泽科德）",L"Calicut (Kozhikode)"}}
        },
        {
            "xerxes_greece", L"薛西斯进军希腊", L"Xerxes' invasion of Greece", L"480 BCE",
            L"陆军主要地点示意；进入色萨利的山地路线有不同解释，海军航线及具体道路未复原。",
            L"Selected land-army milestones; approaches to Thessaly are debated and the fleet and exact roads are omitted.",
            {"https://en.wikisource.org/wiki/Herodotus_The_Persian_Wars_%28Godley%29/Book_VII", "https://www.cambridge.org/core/journals/journal-of-hellenic-studies/article/abs/thessalian-expedition-of-480-bc/9D2CA4CDD8DA62B795BAB736893A6591"},
            {{{38.49,28.04},L"萨迪斯",L"Sardis"},
             {{40.24,26.65},L"赫勒斯滂海峡附近",L"Hellespont area"},
             {{40.90,26.00},L"多里斯科斯附近",L"Doriskos area"},
             {{40.82,23.84},L"斯特律蒙河口附近",L"Strymon mouth area"},
             {{40.64,22.94},L"塞尔迈（今塞萨洛尼基）",L"Therme (Thessaloniki)"},
             {{40.76,22.52},L"塞萨洛尼基湾西北侧（示意）",L"Northwestern Thermaic Gulf (schematic)"},
             {{40.12,22.15},L"奥林匹斯山西侧山地（示意）",L"Western Olympus highlands (schematic)"},
             {{39.87,22.47},L"贡努斯地区（示意）",L"Gonnus area (schematic)"},
             {{39.18,22.76},L"阿尔米罗斯地区",L"Almyros area"},
             {{38.80,22.54},L"温泉关",L"Thermopylae"},
             {{37.98,23.73},L"雅典",L"Athens"}}
        },
        {
            "cortes_mexico", L"科尔特斯进军特诺奇蒂特兰", L"Cortés' advance to Tenochtitlan", L"1519",
            L"从森波阿拉至墨西哥谷地的主要地点示意；托托纳克与特拉斯卡拉盟军参与了这次行动。",
            L"Selected places from Cempoala to the Valley of Mexico; Totonac and Tlaxcalan allies participated.",
            {"https://www.noticonquista.unam.mx/amoxtli/1631/1623", "https://e1.portalacademico.cch.unam.mx/alumno/historiademexico1/unidad3/conquistademexico/descubrimientodemexico"},
            {{{19.45,-96.38},L"森波阿拉",L"Cempoala"},
             {{19.45,-96.96},L"科阿特佩克",L"Coatepec"},
             {{19.47,-97.06},L"希科地区",L"Xico area"},
             {{19.82,-97.36},L"特休特兰",L"Teziutlán"},
             {{19.31,-98.24},L"特拉斯卡拉",L"Tlaxcala"},
             {{19.06,-98.30},L"乔卢拉",L"Cholula"},
             {{19.43,-99.13},L"特诺奇蒂特兰（今墨西哥城）",L"Tenochtitlan (Mexico City)"}}
        },
        {
            "pizarro_peru", L"皮萨罗进军印加地区", L"Pizarro's advance into the Inca realm", L"1532–1533",
            L"西班牙征服行动主要地点按一种研究示意；道路及各分队未逐一复原，古地点比定可能存在分歧。",
            L"Selected places of the Spanish conquest follow one reconstruction; roads, separate groups and ancient site identifications are uncertain.",
            {"https://repositorio.pucp.edu.pe/bitstreams/9a7ab74b-e4da-4956-9309-ee5a6e1fb688/download", "https://qhapaqnan.cultura.pe/sites/default/files/mi/archivo/QHAPAQNANSISTEMAVIALANDINO.pdf", "https://www.openstreetmap.org/node/4823760371"},
            {{{-3.56,-80.45},L"通贝斯",L"Tumbes"},
             {{-4.8855,-80.8249},L"坦加拉拉附近（现代地点示意）",L"Tangarará area (modern location, schematic)"},
             {{-5.20,-80.63},L"皮乌拉河谷附近",L"Piura valley area"},
             {{-5.98,-79.75},L"奥尔莫斯地区",L"Olmos area"},
             {{-6.15,-79.71},L"莫图佩地区",L"Motupe area"},
             {{-6.92,-79.59},L"萨尼亚地区",L"Saña area"},
             {{-7.16,-78.51},L"卡哈马卡",L"Cajamarca"},
             {{-7.62,-78.05},L"卡哈班巴地区",L"Cajabamba area"},
             {{-7.81,-78.05},L"瓦马丘科地区",L"Huamachuco area"},
             {{-9.72,-77.45},L"雷夸伊地区",L"Recuay area"},
             {{-10.47,-76.99},L"卡哈坦博地区",L"Cajatambo area"},
             {{-11.42,-75.69},L"塔尔马地区",L"Tarma area"},
             {{-11.78,-75.50},L"豪哈",L"Jauja"},
             {{-13.65,-73.95},L"比尔卡斯瓦曼地区",L"Vilcashuamán area"},
             {{-13.65,-73.39},L"安达瓦伊拉斯地区",L"Andahuaylas area"},
             {{-13.53,-71.97},L"库斯科",L"Cusco"}}
        },
        {
            "ten_thousand", L"希腊万人队撤退至黑海", L"Retreat of the Ten Thousand to the Black Sea", L"401–400 BCE",
            L"从库纳克萨至特拉布宗的主要区域示意；山地中的具体行程存在争议。",
            L"Major areas from Cunaxa to Trapezus are schematic; the mountain route is disputed.",
            {"https://www.history.ucsb.edu/wp-content/uploads/histpublications/files/00067-lee_2005.pdf", "https://www.iranicaonline.org/articles/anabasis/?generate_pdf=1"},
            {{{33.38,44.10},L"库纳克萨地区（示意）",L"Cunaxa area (schematic)"},
             {{36.10,43.33},L"拉里萨遗址附近",L"Larissa ruins area"},
             {{36.36,43.15},L"墨斯皮拉（尼尼微）",L"Mespila (Nineveh)"},
             {{37.20,42.50},L"卡杜奇山地（示意）",L"Carduchian mountains (schematic)"},
             {{39.70,41.30},L"亚美尼亚高地（示意）",L"Armenian highlands (schematic)"},
             {{41.00,39.72},L"特拉布宗",L"Trapezus (Trabzon)"}}
        },
        {
            "caesar_gaul_52", L"凯撒进军阿瓦里库姆与格尔戈维亚", L"Caesar's advance to Avaricum and Gergovia", L"52 BCE",
            L"选取凯撒本人的主要行动地点；部队分路及部分迂回未绘出，节点之间不是精确道路。",
            L"Selected places on Caesar's campaign; separate forces and detours are omitted, and links are not exact roads.",
            {"https://thelandmarkcaesar.com/LandmarkCaesarWebEssays_5Jan2018.pdf"},
            {{{48.20,3.28},L"阿格丁库姆（桑斯）",L"Agedincum (Sens)"},
             {{47.90,1.91},L"凯纳本（奥尔良）",L"Cenabum (Orléans)"},
             {{47.08,2.40},L"阿瓦里库姆（布尔日）",L"Avaricum (Bourges)"},
             {{45.72,3.13},L"格尔戈维亚遗址附近",L"Gergovia site area"}}
        },
        {
            "belisarius_italy", L"贝利撒留由意大利南部进军罗马", L"Belisarius' advance from southern Italy to Rome", L"536",
            L"仅示意由雷吉乌姆经那不勒斯至罗马的意大利本土阶段；海岸路与拉丁大道未精确复原。",
            L"Italian mainland phase from Rhegium via Naples to Rome; coastal roads and the Via Latina are not reconstructed precisely.",
            {"https://penelope.uchicago.edu/Thayer/E/Roman/Texts/secondary/BURLAT/18B%2A.html"},
            {{{38.11,15.65},L"雷吉乌姆（雷焦卡拉布里亚）",L"Rhegium (Reggio Calabria)"},
             {{38.68,16.10},L"卡拉布里亚西南部（示意）",L"Southwestern Calabria (schematic)"},
             {{38.96,16.31},L"拉梅齐亚地区（示意）",L"Lamezia area (schematic)"},
             {{39.13,16.07},L"阿曼泰亚地区（示意）",L"Amantea area (schematic)"},
             {{39.81,15.79},L"斯卡莱亚地区（示意）",L"Scalea area (schematic)"},
             {{40.07,15.62},L"萨普里地区（示意）",L"Sapri area (schematic)"},
             {{40.68,14.77},L"萨莱诺地区（示意）",L"Salerno area (schematic)"},
             {{40.85,14.27},L"那不勒斯",L"Naples"},
             {{41.49,13.83},L"拉丁大道沿线（示意）",L"Via Latina area (schematic)"},
             {{41.90,12.50},L"罗马",L"Rome"}}
        },
        {
            "third_crusade_coast", L"第三次十字军：阿卡至雅法", L"Third Crusade: Acre to Jaffa", L"1191",
            L"理查一世军队沿海行进的主要地点示意；海上补给部队和具体道路未绘出。",
            L"Selected locations on Richard I's coastal march; the supporting fleet and exact roads are omitted.",
            {"https://compass.onlinelibrary.wiley.com/doi/10.1111/hic3.12662", "https://www.gutenberg.org/cache/epub/71283/pg71283-images.html"},
            {{{32.93,35.08},L"阿卡",L"Acre"},
             {{32.82,34.99},L"海法地区（示意）",L"Haifa area (schematic)"},
             {{32.50,34.89},L"凯撒利亚地区（示意）",L"Caesarea area (schematic)"},
             {{32.20,34.81},L"阿尔苏夫",L"Arsuf"},
             {{32.05,34.75},L"雅法",L"Jaffa"}}
        },
        {
            "trajan_dacia", L"图拉真第一次达契亚战争", L"Trajan's first Dacian war", L"101–102",
            L"参考一种由多瑙河向塔佩推进的重建；渡河地点、行军道路与战场位置存在不同解释。",
            L"One reconstruction of the advance from the Danube to Tapae; crossings, roads and battlefield locations are debated.",
            {"https://assets.cambridge.org/97805218/72386/excerpt/9780521872386_excerpt.pdf", "https://www.museociviltaromana.it/it/didattica/didattica_per_le_scuole/visita_didattica_per_le_scuole_museo_della_civilta_romana_approfondimenti_le_campagne_militari_la_colonna_di_traiano_e_il_racconto_della_guerra_contro_i_daci"},
            {{{44.73,21.22},L"维米纳奇乌姆附近",L"Viminacium area"},
             {{44.82,21.34},L"莱德拉塔附近",L"Lederata area"},
             {{45.05,21.48},L"阿尔基达瓦附近",L"Arcidava area"},
             {{45.47,21.62},L"贝尔佐比斯附近",L"Berzobis area"},
             {{45.46,22.20},L"提比斯库姆附近",L"Tibiscum area"},
             {{45.51,22.50},L"塔佩地区（位置有争议）",L"Tapae area (location disputed)"}}
        },
        {
            "gao_xianzhi_bolor", L"高仙芝远征小勃律", L"Gao Xianzhi's Little Bolor campaign", L"747",
            L"主要区域按一种史地研究示意；古地名比定和山地道路并不确定。",
            L"Major regions follow one historical reconstruction; ancient place identifications and mountain tracks are uncertain.",
            {"https://www.academia.edu/44545611/The_Struggle_of_Tibet_in_the_West_from_8th_to_Early_9th_Century_8%E4%B8%96%E7%BA%AA%E8%87%B39%E4%B8%96%E7%BA%AA%E5%88%9D%E5%90%90%E8%95%83%E5%AF%B9%E8%A5%BF%E5%9F%9F%E7%9A%84%E7%BB%8F%E7%95%A5"},
            {{{41.72,82.94},L"安西（龟兹地区）",L"Anxi (Kucha area)"},
             {{39.47,75.99},L"疏勒地区",L"Shule (Kashgar area)"},
             {{37.78,75.23},L"葱岭守捉（塔什库尔干附近）",L"Congling garrison (Tashkurgan area)"},
             {{38.17,73.97},L"播密川地区（示意）",L"Murghab valley area (schematic)"},
             {{37.49,71.55},L"五识匿地区（示意）",L"Shighnan area (schematic)"},
             {{36.51,73.72},L"坦驹岭（Darkot 山口附近）",L"Darkot Pass area"},
             {{36.17,73.43},L"阿弩越城地区（示意）",L"Gupis area (schematic)"},
             {{35.92,74.31},L"小勃律（吉尔吉特地区）",L"Little Bolor (Gilgit area)"}}
        },
        {
            "tariq_iberia", L"塔里克进军伊比利亚半岛", L"Tariq's advance into Iberia", L"711",
            L"展示一项关于塔里克主力由直布罗陀经埃希哈至托莱多的重建；战场位置及具体路线有争议，分遣队未绘出。",
            L"One reconstruction of Tariq's main force from Gibraltar via Écija to Toledo; the battlefield and exact route are disputed, and detachments are omitted.",
            {"https://digibug.ugr.es/bitstream/handle/10481/71624/74140.pdf?isAllowed=y&sequence=4", "https://etheses.dur.ac.uk/15036/1/GruesoDominguez000224310.pdf"},
            {{{36.14,-5.35},L"直布罗陀地区",L"Gibraltar area"},
             {{36.80,-5.80},L"伊比利亚南部战场区域（位置有争议）",L"Southern Iberian battlefield area (disputed)"},
             {{37.54,-5.08},L"埃希哈",L"Écija"},
             {{39.86,-4.03},L"托莱多",L"Toledo"}}
        },
        {
            "batu_ryazan_vladimir", L"拔都进攻梁赞与弗拉基米尔", L"Batu's campaign against Ryazan and Vladimir", L"1237–1238",
            L"仅示意东北罗斯战役的主要地点；蒙古军多支部队的具体道路未复原。",
            L"Selected locations in the northeastern Rus' campaign; the routes of separate Mongol forces are not reconstructed.",
            {"https://cristoraul.org/english/readinghall/thirdmillenniumlibrary/Medieval-History-Chapters/PDF/NCMedH_6-1198-1300.pdf", "https://repository.lib.ncsu.edu/bitstreams/d2e7bf35-a759-408e-841b-8482054b0200/download"},
            {{{54.32,40.51},L"旧梁赞遗址附近",L"Old Ryazan area"},
             {{55.10,38.75},L"科洛姆纳",L"Kolomna"},
             {{55.75,37.62},L"莫斯科",L"Moscow"},
             {{56.13,40.41},L"弗拉基米尔",L"Vladimir"}}
        },
        {
            "timur_delhi", L"帖木儿进军德里", L"Timur's advance to Delhi", L"1398",
            L"去程主要地点示意；进军伴随劫掠及屠杀，其他分队与具体道路未绘出。",
            L"Selected outbound locations; the invasion involved plunder and massacres. Separate forces and exact roads are omitted.",
            {"https://www.ibiblio.org/britishraj/Jackson5/chapter09.html", "https://library.bjp.org/jspui/bitstream/123456789/1890/1/The%20Imperial%20gazetteer%20%20of%20India%20Vol-20%20%28Pardi%20to%20Pusad%29.pdf"},
            {{{30.53,72.24},L"图兰巴",L"Tulamba"},
             {{29.58,74.33},L"巴特纳尔（哈努芒加尔）",L"Bhatner (Hanumangarh)"},
             {{29.53,75.03},L"锡尔萨",L"Sirsa"},
             {{29.80,76.40},L"凯塔尔",L"Kaithal"},
             {{29.39,76.97},L"帕尼帕特",L"Panipat"},
             {{28.75,77.29},L"洛尼",L"Loni"},
             {{28.61,77.21},L"德里",L"Delhi"}}
        },
        {
            "mehmed_constantinople", L"穆罕默德二世赴君士坦丁堡", L"Mehmed II's advance to Constantinople", L"1453",
            L"示意从埃迪尔内至君士坦丁堡的主力行动；各部队具体行军道路和海军行动未复原。",
            L"Schematic main-army movement from Edirne to Constantinople; individual roads and naval movements are omitted.",
            {"https://istanbultarihi.ist/55-konstantinopolisin-son-kusatmasi-ve-dususu", "https://ttk.gov.tr/istanbulun-fethi/", "https://constantinople1453.org/en/history/"},
            {{{41.68,26.56},L"埃迪尔内",L"Edirne"},
             {{41.01,28.94},L"君士坦丁堡城墙附近",L"Constantinople walls area"}}
        },
        {
            "berlin_1945", L"苏军第8近卫集团军赴柏林", L"Soviet 8th Guards Army toward Berlin", L"1945",
            L"按主要战斗区域示意塞洛高地至柏林的推进；下属部队分路，连线不是具体行军道路。",
            L"Major battle areas from the Seelow Heights to Berlin; subordinate units advanced separately and lines are not exact roads.",
            {"https://www.bundesarchiv.de/themen-entdecken/online-entdecken/dokumente-zur-zeitgeschichte/lagekarte-zur-schlacht-um-berlin/", "https://erenow.org/ww/thefallofberlin1945antonybeevor/18.php", "https://erenow.org/ww/thefallofberlin1945antonybeevor/23.php"},
            {{{52.59,14.64},L"屈斯特林桥头堡地区",L"Küstrin bridgehead area"},
             {{52.53,14.38},L"塞洛高地地区",L"Seelow Heights area"},
             {{52.51,14.14},L"明歇贝格地区",L"Müncheberg area"},
             {{52.47,13.40},L"滕珀尔霍夫机场地区",L"Tempelhof airfield area"},
             {{52.51,13.38},L"柏林市中心地区",L"Central Berlin area"}}
        }
    };
    static const std::vector<Campaign> prepared=[](){
        auto all=entries;appendLongDistanceRoutes(all);
        for(auto& route:all){
            std::string id=route.id;
            if(id=="magellan_elcano"||id=="zheng_he_first"||id=="vasco_india"){route.mode=TravelMode::Sea;route.category="voyage";}
            if(id=="lewis_clark"){route.mode=TravelMode::Mixed;route.category="exploration";}
            for(auto& stop:route.stops)if(stop.kind==WaypointKind::Milestone){
                if(std::wstring(stop.zh).find(L"争议")!=std::wstring::npos)stop.kind=WaypointKind::Uncertain;
                else if(std::wstring(stop.zh).find(L"示意")!=std::wstring::npos||std::wstring(stop.zh).find(L"辅助")!=std::wstring::npos)stop.kind=WaypointKind::Auxiliary;
            }
        }
        return all;
    }();
    return prepared;
}
}
