const fs=require('fs'),path=require('path'),crypto=require('crypto');const root=path.resolve(__dirname,'..'),source=path.join(root,'source-data/supplements/beagle-itinerary.html');
const html=fs.readFileSync(source,'utf8'),paragraphs=html.match(/<p\b[^>]*>[\s\S]*?<\/p>/gi).filter(p=>/<strong>Day<\/strong>/.test(p));
const clean=s=>s.replace(/<[^>]*>/g,' ').replace(/&nbsp;/g,' ').replace(/&amp;/g,'&').replace(/\s+/g,' ').trim();
const months={Jan:1,Feb:2,Mar:3,Apr:4,May:5,Jun:6,Jul:7,Aug:8,Sep:9,Oct:10,Nov:11,Dec:12};const missing=[],points=[];
const excluded=new Map([[207,'Source longitude 05°17 W creates a >4000 km daily jump between Brazilian coastal positions; omitted, not silently corrected.'],[1491,'Source Sydney longitude 117°51 E conflicts with the same locality and adjacent records; omitted.'],[1492,'Same Sydney coordinate discrepancy.'],[1493,'Same Sydney coordinate discrepancy.']]);
for(const paragraph of paragraphs){const text=clean(paragraph),day=+text.match(/^Day\s+(\d+)/)?.[1],date=text.match(/(\d{1,2})\s+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+(\d{4})/),mode=text.match(/\((Sea|Land|Harbour)\)/)?.[1];
    if(excluded.has(day)){missing.push({day,date:date?.[0],reason:excluded.get(day)});continue;}
    const match=text.match(/(\d{1,2})[º°]\s*(\d{1,2}(?:\.\d+)?)\s*['’′]?\s*([NS])\s*,?\s*(\d{1,3})[º°]\s*(\d{1,2}(?:\.\d+)?)\s*['’′]?\s*([EW])/i);
    if(!day||!date||!mode||!match||+match[2]>=60||+match[5]>=60){missing.push({day,date:date?.[0],reason:match?'Invalid degrees/minutes':'No complete coordinate pair'});continue;}
    const latitude=(+match[1]+match[2]/60)*(match[3].toUpperCase()==='S'?-1:1),longitude=(+match[4]+match[5]/60)*(match[6].toUpperCase()==='W'?-1:1);
    if(Math.abs(latitude)>90||Math.abs(longitude)>180){missing.push({day,reason:'Coordinate range'});continue;}
    const iso=date[3]+'-'+String(months[date[2]]).padStart(2,'0')+'-'+date[1].padStart(2,'0');
    const logged=mode==='Sea'&&/FitzRoy/i.test(text);const last=points.at(-1);
    if(last&&last.latitude===latitude&&last.longitude===longitude){last.toDate=iso;last.toDay=day;continue;}
    points.push({day,toDay:day,date:iso,toDate:iso,latitude,longitude,mode,kind:logged?'LoggedPosition':'ReferenceLocality',breakBefore:last?day-last.toDay>5:false});
}
function km(a,b){let p=Math.PI/180,dlat=(b.latitude-a.latitude)*p,dlon=(b.longitude-a.longitude)*p;return 12742*Math.asin(Math.min(1,Math.sqrt(Math.sin(dlat/2)**2+Math.cos(a.latitude*p)*Math.cos(b.latitude*p)*Math.sin(dlon/2)**2)));}
const jumps=points.slice(1).map((b,i)=>({day:b.day,from:points[i].toDate,to:b.date,kmPerDay:Math.round(km(points[i],b)/Math.max(1,b.day-points[i].toDay)),previous:points[i].day})).sort((a,b)=>b.kmPerDay-a.kmPerDay);
// Very large daily jumps indicate a transcription/coordinate problem. Preserve the points,
// but never draw an invented connecting segment across the discrepancy.
for(const jump of jumps)if(jump.kmPerDay>1200){points.find(p=>p.day===jump.day).breakBefore=true;}
const q=s=>'L'+JSON.stringify(s);const lines=points.map(p=>{
    const date=p.date+(p.toDate!==p.date?'–'+p.toDate:'');const zh=(p.mode==='Sea'?'海上记录':p.mode==='Harbour'?'停泊地参考点':'陆上考察参考点')+' · '+date;
    const en=(p.mode==='Sea'?'Sea position':p.mode==='Harbour'?'Harbour reference':'Land excursion reference')+' · '+date;
    return `    {{${p.latitude.toFixed(8)},${p.longitude.toFixed(8)},${p.breakBefore}},${q(zh)},${q(en)},WaypointKind::${p.kind},${q(date)}}`;
});
const header='#pragma once\n// Derived factual coordinates; see data/route-sources.json. Do not hand-edit.\ninline std::vector<Stop> beagleStops(){return {\n'+lines.join(',\n')+'\n};}\n';
fs.writeFileSync(path.join(root,'native/beagle_stops.generated.h'),header);
const manifest={source:'https://darwin-online.org.uk/converted/Ancillary/Rookmaaker_Beagle_Itinerary_A575.html',citation:'Kees Rookmaaker, Darwin itinerary, edited by John van Wyhe, Darwin Online; revision 2020–2025. At-sea noon coordinates refer to FitzRoy (1839); harbour/land positions are modern approximate localities.',retrieved:'2026-11-20',sourceSha256:crypto.createHash('sha256').update(html).digest('hex'),sourceDays:paragraphs.length,distinctPositions:points.length,loggedPositions:points.filter(p=>p.kind==='LoggedPosition').length,missingCoordinateDays:missing,breaks:points.filter(p=>p.breakBefore).map(p=>({day:p.day,date:p.date})),largestDailyJumps:jumps.slice(0,15),method:'Retain complete valid coordinate pairs, merge consecutive identical locations, break connections over gaps longer than five days or source jumps over 1200 km per day. No interpolation is asserted to be an observed ship track. Includes Darwin land excursions; not exclusively the ship track.'};
fs.writeFileSync(path.join(root,'qa/development/beagle-source-audit.json'),JSON.stringify(manifest,null,2));fs.writeFileSync(path.join(root,'source-data/supplements/beagle-points.json'),JSON.stringify(points,null,2));
console.log(JSON.stringify({...manifest,missingCoordinateDays:missing.length,largestDailyJumps:jumps.slice(0,6)},null,2));
