const fs=require('fs'),path=require('path');const root=path.resolve(__dirname,'..'),release=path.resolve(root,'../../WorldMapExplorer-1.28.0');
const routes=JSON.parse(fs.readFileSync(path.join(root,'qa/development/route-geometry.json'),'utf8'));
if(routes.length!==42||!routes[30].zh)throw Error('Run the current RouteAccuracyChecks export first');
const history=JSON.parse(fs.readFileSync(path.join(release,'data/historical-boundaries-source.json'),'utf8'));
let sources=fs.readFileSync(path.resolve(root,'source-data/base-data-sources-1.24.md'),'utf8');
let start=sources.indexOf('## 世界历史政权'),end=sources.indexOf('## UNESCO',start);if(start<0||end<0)throw Error('Source documentation markers');
const historical=`## 历史疆域矢量（1.28.0）

- 运行文件为 \`data/historical-boundaries.wmh2\`，共 ${history.bytes.toLocaleString('en-US')} 字节；取代逐年保存的格心归属 JSON。文件通过只读映射访问，重复几何仅保存一次。显示和点击查询使用多边形，网格仅作为定位工具。
- 来源为 [Cliopatria / Seshat Global History Databank](https://github.com/Seshat-Global-History-Databank/cliopatria)，固定文件提交 \`${history.sourceCommit}\`、Git blob \`${history.sourceGitBlob}\`。归档名为 \`${history.sourceFile}\`；ZIP 和 GeoJSON 的 SHA-256 见 \`data/historical-boundaries-source.json\`。论文：[Bennett et al. (2025), Cliopatria, Scientific Data 12:247](https://doi.org/10.1038/s41597-025-04516-9)。
- 原始资料13797条；本地派生记录13800条，含13415条POLITY及385条RELATION。关系记录供资料页查阅，不作为主权领土填色。原始及补充数据共11317份不同几何、3422262个坐标点，保留多部件、内洞和日期变更线；坐标按float64存储，没有用格网中心代替边界。
- 可选择前3400年至2024年的5424个非零年份。每个年份依据来源的包含式FromYear / ToYear筛选；每年可选不表示每年都有边界观测，不在年份之间推演疆域。播放跳到来源边界变更、事件和辅助纪年节点，跨过的事件不会丢失。
- 来源存在概化、空白、重叠和命名错误。资料有效年段不等于王朝建立、灭亡日期。古代“Later Zhou”误名仅校订显示为东周，原几何及年段保留。中国秦汉等节点应结合独立事件资料理解，不把有效年段冒充历史定论。
- 六个2024年小国参考补充使用Natural Earth 5.1.1：摩纳哥、列支敦士登、圣马力诺、梵蒂冈、安道尔、卢森堡。它们替换该节点缺漏或粗糙的对应记录，标注“现代参考补充”；不回推早期年份，也不声称是2024年实测。Natural Earth为公有领域，1:10m指一千万分之一制图比例尺，不是10米精度。
- 历史边界按[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)保留Seshat / Cliopatria署名。许可见 \`licenses/CLIOPATRIA_RAW_LICENSE.md\`。1420个Wikidata标识的中英文标签快照用于名称补充，Wikidata结构化数据为CC0；无中文名时保留英文原名。
- 空白表示缺少对应来源记录；重叠记录按面积较小者优先列出。颜色只用于区分区域。图层不用于法律边界、导航或工程测绘。更细的绘图不消除史料本身的不确定性。

`;
sources=sources.slice(0,start)+historical+sources.slice(end);
sources=sources.replace('随数据页公开的当前版本为 5.1.1','本地采用的来源快照版本为 5.1.1').replace('历史年份分别保存在独立细格模块中，完整年份列表见下节，不写入现有父格档案。','历史层使用下节所述独立矢量档案，不写入父格档案。');
start=sources.indexOf('## 历史事件库（离线）');if(start<0)throw Error('Event source section');
sources=sources.slice(0,start)+`## 历史事件库（1.28.0）

- 主文件 \`data/world-events-v2.tsv\` 含346条事件、19个整理后的类别，覆盖前2560年至2024年；182条为去重后新增条目。旧六列 \`world-events.tsv\` 仅作兼容备份。摘要用于快速了解背景，不是完整史学论证。
- 新表保留中文标题、地点、项目编写的中文摘要、英文阅读字段、年份/年段、来源链接及年代说明。120条具有Wikidata提供的地球参考坐标。人物传记坐标、月球或火星表面坐标不会被当作地球事件位置。参考点不表示精确事件范围。
- 公开Wikidata快照提供CC0标签、说明、时间与坐标，并保留逐条QID链接。个人即位、著作出版、任务着陆等里程碑不强行套用实体的创建或出生年份；源年与里程碑年不一致的条目记录于源码 \`qa/development/event-source-audit.json\`。该核对不是对所有史学争论的穷尽验证。
- 校正旧表东汉建立的公元1年为公元25年，参考[哥伦比亚大学中国历史年表](https://afe.easia.columbia.edu/timelines/china_timeline.htm)；爱奥尼亚起义调整到通行的前499年。撒马利亚征服采用约年并说明史料分歧，参考[宾夕法尼亚大学ORACC材料](https://oracc2.museum.upenn.edu/saao/aebp/essentials/countries/israel/index.html)。天工开物、加纳独立及巴拿马运河交接等条目提供图书馆、档案或管理机构资料。
- 独立检索可搜索中英文标题、地点与摘要，按类别及包含式年份区间筛选。负数表示公元前，无公元0年。可查看事件年份、定位有坐标的条目，或由用户点击打开来源。所有筛选和地图查询均可离线使用。

## 历史路线（1.28.0）

- 42条路线保留原来的选择、逐步网格着色、播放、暂停、隐藏与定位功能。12条新增长途行程包含达尔文、库克、德雷克、塔斯曼、伊本·白图泰、玄奘、马可·波罗、阿蒙森、沙克尔顿、哥伦布、迪亚士与白令。
- 贝格尔号相关行程依据[Darwin Online整理的1740天行程](https://darwin-online.org.uk/converted/Ancillary/Rookmaaker_Beagle_Itinerary_A575.html)，提取596个不同位置，其中407点带FitzRoy海上日志标记，其他点为整理者定位的陆上/停泊参考点。含达尔文陆上活动，不是全程同船的船迹。334个日期缺少完整坐标或被排除，其中4条因明显经度异常排除；13处较长资料间隔断开。日期变更线导致的日历差异保留来源日期，不推算缺失位置。
- 其余路线以记载地点和明示的路径辅助点连接。修正旧航线在巴西、非洲南端、婆罗洲等处的明显大陆捷径；未复原的巨港河道断开。现代陆地掩膜检查是发现粗大穿陆连接的诊断，不能证明精确历史航迹。古道路、港口、水道与海岸可能已改变。
- 播放按参考点之间的距离推进，避免密集点与稀疏点出现不同的视觉速度；不模拟真实行进时长。说明中的连线长度不是实际走行或航行里程。辅助点不列作历史停靠地，争议点与资料间断单独说明。
- 逐条地点及来源见 \`路线地点与资料清单.md\`；编译目录的完整坐标参照导出为 \`data/route-catalog-reference.json\`，供审阅追溯。运行中的路线来自程序内置目录，直接编辑参照JSON不会改变程序。
`;
const places=JSON.parse(fs.readFileSync(path.join(release,'data/place-search-source.json'),'utf8'));
sources+=`\n## 离线地点搜索（1.28.0）\n\n${places.cities}个城市采用GeoNames来源参考坐标，${places.countries}个国家/地区采用Natural Earth地图标注点。快照日期${places.snapshot}。运行表为data/place-search.tsv，SHA-256为\`${places.sha256}\`。城市与国家参考点不改变周边基础分区统计的含义。\n\n`+fs.readFileSync(path.join(release,'licenses/PLACE_CATALOG_SOURCES.md'),'utf8').replace(/^# .+\n/,'');
fs.writeFileSync(path.join(release,'DATA_SOURCES.md'),sources);
fs.writeFileSync(path.join(release,'data/route-catalog-reference.json'),JSON.stringify({schemaVersion:1,version:'1.28.0',role:'Review-only export of the compiled route catalog; not runtime configuration',routes},null,2));
let doc='# 路线地点与资料清单 · 1.28.0\n\n共42条。\n\n历史地点、日志坐标、现代地点参考、辅助点和争议位置分别标注。辅助点用于连接示意，不作为曾经到访的证据；断开处没有画出未经重建的连线。完整596点贝格尔号日期位置可在程序“路线说明”或参照JSON中查看。\n\n';
for(const route of routes){const named=route.places.filter(p=>p.kind!=='路径辅助点'),gaps=route.points.filter(p=>p[2]).length;doc+=`## ${route.zh}\n\n${route.en} · ${route.period} · ${route.mode}\n\n${route.noteZh}\n\n${route.points.length}个参考点，${gaps}处断开。\n\n`;if(route.id==='beagle_darwin')doc+='逐日位置来自行程资料；407个海上日志坐标。为保持本清单易读，此处不重复596点。\n\n';else doc+=named.map(p=>'- '+p.zh+' · '+p.kind+(p.date?' · '+p.date:'')).join('\n')+'\n\n';doc+='来源：\n\n'+route.sources.map(url=>'- ['+new URL(url).hostname+']('+url+')').join('\n')+'\n\n';}
fs.writeFileSync(path.join(release,'路线地点与资料清单.md'),doc);
fs.writeFileSync(path.join(release,'licenses/HISTORY_EVENTS_DATA.md'),'# 历史事件资料说明 · 1.28.0\n\n中文浏览摘要由本项目编写；结构化标签、日期、坐标与部分英文短说明来自Wikidata公开CC0快照。每条保存来源链接，独立编写的英文里程碑摘要与来源实体的一般说明区分保存。新增事件和旧事件的年代校订见DATA_SOURCES.md及data/world-events-source.json。古代估年、传统纪年、发生与公布年份差异在条目中说明。来源不是对所有事实或争议的保证。\n\n旧world-events.tsv为兼容备份，主程序优先加载world-events-v2.tsv。\n');
fs.writeFileSync(path.join(release,'licenses/ROUTE_DATA_SOURCES.md'),'# 路线资料说明 · 1.28.0\n\n路线是本项目编写的参考地点与辅助几何，不是可导航轨迹。贝格尔号日期与坐标事实依据Darwin Online，原整理署名Kees Rookmaaker，编辑John van Wyhe，资料注明修订2020–2025；海上坐标引自FitzRoy《Narrative》(1839)气象附录，陆上及港口坐标为整理者的近似现代定位。保留来源署名，不复制日记叙事或现代研究文章全文。\n\n其余资料来自逐条链接的图书馆、博物馆、历史研究和遗产管理机构。不得把辅助点、争议位置或参考点连线声称为已考证的精确历史路线。完整逐条出处见“路线地点与资料清单.md”。\n');
console.log('Runtime source notes, 42-route reference catalog and reading list generated.');
