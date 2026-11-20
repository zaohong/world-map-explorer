const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),release=path.resolve(root,'../../WorldMapExplorer-1.28.0'),input=path.resolve(root,'source-data/events-legacy.tsv');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const mappings=read(path.join(root,'source-data/supplements/event-title-map.json')),entities=read(path.join(root,'source-data/supplements/event-entities.json'));
const overrides=require('./event_overrides.cjs');
const englishPlace=require('./event_place_labels.cjs')(release);
const categories={政治:'Politics',战争:'War and conflict',宗教:'Religion',科技:'Science and technology',科学:'Science and technology',灾害:'Disasters',起义:'Uprisings',革命:'Revolutions',外交:'Diplomacy',航海:'Voyages',经济:'Economy',思想:'Ideas',工程:'Engineering',建城:'Cities',考古与建筑:'Archaeology',人物与思想:'People and ideas',体育与文化:'Sport and culture',探索与交流:'Exploration and exchange',社会运动:'Social movements',建筑与工程:'Buildings and engineering',外交与法律:'Diplomacy and law',社会与制度:'Society and institutions',宗教与思想:'Religion and ideas',贸易与经济:'Trade and economy',贸易与交流:'Trade and exchange',航海与探索:'Voyages and exploration',科学与技术:'Science and technology',社会与文化:'Society and culture',迁徙与殖民:'Migration and colonization',政治与社会:'Politics and society',革命与独立:'Revolution and independence',自然保护:'Environment and conservation',公共卫生:'Public health',国际组织:'International organizations',战争与外交:'War and diplomacy'};
const normalize={科技:'科学与技术',科学:'科学与技术',灾害与公共卫生:'公共卫生',战争与科技:'战争',政治与战争:'战争',迁徙与战争:'战争',战争与科技:'战争',航海:'航海与探索',工程:'建筑与工程',革命:'革命与独立',革命与政治:'革命与独立',宗教与社会:'宗教与思想',宗教与政治:'宗教与思想',宗教与战争:'战争',战争与外交:'外交与法律',人权与法律:'外交与法律',法律与社会:'外交与法律',贸易与政治:'贸易与经济',政治与经济:'贸易与经济',外交流殖民:'外交与法律',外交流民:'外交与法律',外交与殖民:'迁徙与殖民',政治与安全:'政治与社会',科技与文化:'科学与技术',科技与社会:'科学与技术',起义与政治:'社会运动'};
const corrections=[];const old=fs.readFileSync(input,'utf8').split(/\r?\n/).filter(x=>x&&!x.startsWith('#')).map((line,index)=>{
  const [year,category,place,title,description,source]=line.split('\t');let e={year:+year,category:normalize[category]||category,place,title,description,source,origin:'legacy',id:'legacy-'+(index+1)};
  if(e.year===1&&e.title.includes('东汉')){corrections.push({oldYear:1,newYear:25,title:e.title,source:'https://afe.easia.columbia.edu/timelines/china_timeline.htm'});e.year=25;e.title='刘秀即位，东汉建立';e.description='刘秀即帝位，后定都洛阳。东汉延续至公元220年，建立年份为公元25年。';e.titleEn='Eastern Han established';e.descriptionEn='Liu Xiu became emperor in 25 CE. The Eastern Han later ruled from Luoyang until 220 CE.';}
  if(e.year===-500&&e.title.includes('爱奥尼亚')){corrections.push({oldYear:-500,newYear:-499,title:e.title,source:'https://www.wikidata.org/wiki/Q208261'});e.year=-499;e.description='小亚细亚希腊城邦反抗波斯统治，起义通常纪为前499年至前493年，是希波战争的前奏。';}
  return e;
});
const seeds=fs.readFileSync(path.join(root,'source-data/event-seeds.tsv'),'utf8').split(/\r?\n/).filter(x=>x&&!x.startsWith('#')).map((line,index)=>{let [year,category,article,title,place,description]=line.split('|');if(!description)throw Error('Incomplete event seed '+index);return {year:+year,category,article,title,place,description,source:(mappings[article]?.id?'https://www.wikidata.org/wiki/'+mappings[article].id:'https://en.wikipedia.org/wiki/'+encodeURIComponent(article.replace(/ /g,'_'))),origin:'curated',id:'curated-'+(index+1)};});
const dates=e=>['P585','P580','P582','P571','P577','P619'].flatMap(prop=>(e?.claims?.[prop]||[]).filter(v=>v.rank!=='deprecated').map(v=>{const d=v.mainsnak?.datavalue?.value;return d&&d.time?{property:prop,year:parseInt(d.time.match(/^[+-]?\d+/)?.[0]),precision:d.precision,calendar:d.calendarmodel}:null;}).filter(Boolean));
const spans=new Map([['Haitian Revolution',1804],['Congress of Vienna',1815],['First voyage of James Cook',1771],['Second voyage of HMS Beagle',1836],['Great Famine (Ireland)',1852],['Spanish Civil War',1939],['English Civil War',1651],['Battle of Stalingrad',1943],['World War I',1918],['World War II',1945],['Mexican War of Independence',1821],['Korean War',1953]]);
const audit=[],dedup=new Set(),output=[];let located=0,matched=0,missing=0;
for(let event of [...old,...seeds]){
    if(event.origin==='curated'&&event.year===25)continue;
    const article=event.article||decodeURIComponent((event.source.split('/wiki/')[1]||'').replace(/_/g,' '));let mapping=mappings[article];const id=mapping?.id,entity=entities[id];
    const key=(id||event.title)+'@'+event.year;if(dedup.has(key))continue;dedup.add(key);if(!entity)missing++;
    const time=dates(entity),same=time.filter(t=>t.year===event.year&&t.precision>=9);if(same.length)matched++;
    event.endYear=event.year;if(spans.has(article)&&(/开始|爆发|起义|革命|战争|召开|启程|大饥荒/.test(event.title)))event.endYear=spans.get(article);
    const label=entity?.labels?.en?.value||mapping?.title||article;const description=entity?.descriptions?.en?.value;
    event.titleEn=event.titleEn||label||event.title;
    event.descriptionEn=event.descriptionEn||description||(event.origin==='curated'?'Historical milestone; the Chinese summary includes the context.':'A historical milestone in the offline event collection.');
    if(event.titleEn!==event.title&&event.year!==time.find(t=>t.property==='P585')?.year&&event.origin==='curated')event.titleEn+=' · '+(event.year<0?(-event.year)+' BCE':event.year);
    const human=(entity?.claims?.P31||[]).some(c=>c.mainsnak?.datavalue?.value?.id==='Q5');
    const coords=(entity?.claims?.P625||[]).filter(c=>c.rank!=='deprecated').map(c=>c.mainsnak?.datavalue?.value).find(c=>c&&(!c.globe||c.globe==='http://www.wikidata.org/entity/Q2')&&Number.isFinite(c.latitude)&&Number.isFinite(c.longitude));
    // Person coordinates and astronomical-body coordinates are never treated as event locations.
    if(coords&&!human){event.latitude=coords.latitude;event.longitude=coords.longitude;located++;}
    event.placeEn=englishPlace(event.place,event.latitude,event.longitude);
    const kinds={'人物与思想':'思想与宗教','体育与文化':'考古与文化','宗教':'思想与宗教','宗教与思想':'思想与宗教','思想':'思想与宗教','政治':'政治与国家','政治与社会':'政治与国家','政治与法律':'外交与法律','外交':'外交与法律','国际协定':'外交与法律','外交与战争':'外交与法律','战争与外交':'外交与法律','战争与航海':'战争与冲突','战争':'战争与冲突','国际会议':'国际合作','国际组织':'国际合作','建城':'建筑与工程','考古与建筑':'考古与文化','社会与文化':'考古与文化','灾害与社会':'灾害','灾害与科技':'灾害','起义':'社会运动','经济':'贸易与经济'};
    event.category=kinds[event.category]||event.category;
    event.categoryEn=({'思想与宗教':'Ideas and religion','考古与文化':'Archaeology and culture','政治与国家':'Politics and states','战争与冲突':'War and conflict','国际合作':'International cooperation'})[event.category]||categories[event.category]||event.category;
    event.dateNoteZh=(/约|传统|通行纪年/.test(event.title)?'年代为估计或传统纪年。':'年份表示标题所指的历史节点。')+(event.endYear!==event.year?'此条包含一个年段。':'')+'相关地点仅作地理参考。';
    event.dateNoteEn=(/约|传统|通行纪年/.test(event.title)?'Approximate or traditional dating. ':'The year refers to the milestone in the title. ')+(event.endYear!==event.year?'This entry covers a period. ':'')+'Locations are references for orientation.';
    if(id)event.source='https://www.wikidata.org/wiki/'+id;
    Object.assign(event,overrides[article+'@'+event.year]||{});
    if(event.origin==='legacy'&&event.title.includes('东汉建立'))event.source='https://afe.easia.columbia.edu/timelines/china_timeline.htm';
    if(!event.source.endsWith('/wiki/'))output.push(event);else throw Error('No source mapping for '+article);
    if(!same.length)audit.push({year:event.year,title:event.title,article,id:id||'',sourceYears:time,review:human?'Biography: milestone year curated independently':'Milestone year curated independently; do not silently substitute entity lifetime'});
}
output.sort((a,b)=>a.year-b.year||a.title.localeCompare(b.title));
const clean=s=>String(s??'').replace(/[\t\r\n]/g,' ').trim();
const header='# WORLD_EVENTS_V2\tyear\tcategory\tplace\ttitle\tsummary\tsource_url\tend_year\tlatitude\tlongitude\ttitle_en\tsummary_en\tplace_en\tcategory_en\tdating_note_zh\tdating_note_en\tid\n';
const rows=output.map(e=>[e.year,e.category,e.place,e.title,e.description,e.source,e.endYear,e.latitude,e.longitude,e.titleEn,e.descriptionEn,e.placeEn,e.categoryEn,e.dateNoteZh,e.dateNoteEn,e.id].map(clean).join('\t'));
const text=header+rows.join('\n')+'\n',target=path.join(release,'data/world-events-v2.tsv');fs.writeFileSync(target,text);
const info={schema:2,retrieved:'2026-11-20',events:output.length,withEarthCoordinates:located,metadataYearMatches:matched,withoutEntitySnapshot:missing,categories:[...new Set(output.map(e=>e.category))].sort(),startYear:output[0].year,endYear:output.at(-1).year,sha256:crypto.createHash('sha256').update(text).digest('hex'),sources:['Wikidata CC0 factual labels, descriptions, dates and reference coordinates','Previous local curated collection; independently authored new Chinese summaries'],corrections,datesRequiringIndependentMilestoneReview:audit};
fs.writeFileSync(path.join(root,'qa/development/event-source-audit.json'),JSON.stringify(info,null,2));
fs.writeFileSync(path.join(release,'data/world-events-source.json'),JSON.stringify({...info,datesRequiringIndependentMilestoneReview:undefined},null,2));
console.log(JSON.stringify({...info,datesRequiringIndependentMilestoneReview:audit.length},null,2));
