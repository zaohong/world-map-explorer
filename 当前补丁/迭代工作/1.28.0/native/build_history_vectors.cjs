'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),sourceDir=path.join(root,'source-data/cliopatria');
const outputDir=path.resolve(root,'../../WorldMapExplorer-1.28.0/data');
const expectedZip='e10a4e429fca708788ff9e7572a95fce801fae55852d217cffabc1c59cd4eed4';
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const strings=values=>Buffer.concat(values.map(value=>{const b=Buffer.from(String(value??''),'utf8'),n=Buffer.alloc(4);n.writeUInt32LE(b.length);return Buffer.concat([n,b]);}));
const knownZh={
 'Sumerian City-States':'苏美尔城邦','Elam':'埃兰','Shang Dynasty':'商朝','Zhou Dynasty':'周朝','Qin':'秦国','Qin Dynasty':'秦朝','Western Han Dynasty':'西汉','Eastern Han Dynasty':'东汉','Han Dynasty':'汉朝','Xin Dynasty':'新朝','Cao Wei':'曹魏','Shu Han':'蜀汉','Eastern Wu':'孙吴','Western Jin Dynasty':'西晋','Eastern Jin Dynasty':'东晋','Northern Wei':'北魏','Sui Dynasty':'隋朝','Tang Dynasty':'唐朝','Song Dynasty':'宋朝','Northern Song':'北宋','Southern Song':'南宋','Liao Dynasty':'辽朝','Great Jin':'金朝','Yuan Dynasty':'元朝','Ming Dynasty':'明朝','Qing Dynasty':'清朝','Republic of China':'中华民国','People\'s Republic of China':'中华人民共和国','Roman Republic':'罗马共和国','Roman Empire':'罗马帝国','Western Roman Empire':'西罗马帝国','Eastern Roman Empire':'东罗马帝国','Byzantine Empire':'拜占庭帝国','Ottoman Empire':'奥斯曼帝国','Achaemenid Empire':'阿契美尼德帝国','Sasanian Empire':'萨珊帝国','Mongol Empire':'蒙古帝国','British Empire':'大英帝国','Kingdom of France':'法兰西王国','Kingdom of England':'英格兰王国','Tokugawa Shogunate':'德川幕府','Heian Japan':'平安时代日本','Kamakura Shogunate':'镰仓幕府','Ashikaga Shogunate':'室町幕府','Maurya Empire':'孔雀帝国','Gupta Empire':'笈多帝国','Mughal Empire':'莫卧儿帝国','Inca Empire':'印加帝国','Aztec Empire':'阿兹特克帝国','Kingdom of Aksum':'阿克苏姆王国','Mali Empire':'马里帝国','Songhai Empire':'桑海帝国','Ghana Empire':'加纳帝国','United States of America':'美利坚合众国','Soviet Union':'苏联','Russian Empire':'俄罗斯帝国'
};
function ring(source,outerCenter){
 if(!Array.isArray(source)||source.length<4)throw new Error('Invalid polygon ring');
 const result=[];let previous;
 const limit=source.length-(source[0][0]===source.at(-1)[0]&&source[0][1]===source.at(-1)[1]?1:0);
 for(let i=0;i<limit;i++){
  let [x,y]=source[i];if(!Number.isFinite(x)||!Number.isFinite(y)||Math.abs(y)>90.000001||Math.abs(x)>1080)throw new Error('Invalid source coordinate');
  if(previous!==undefined)x+=Math.round((previous-x)/360)*360;
  result.push([x,y]);previous=x;
 }
 if(result.length<3)throw new Error('Degenerate ring');
 if(outerCenter!==undefined){const middle=result.reduce((sum,p)=>sum+p[0],0)/result.length,shift=Math.round((outerCenter-middle)/360)*360;for(const p of result)p[0]+=shift;}
 result.push(result[0].slice());return result;
}
function normalizedPolygons(geometry){
 const source=geometry.type==='Polygon'?[geometry.coordinates]:geometry.type==='MultiPolygon'?geometry.coordinates:null;
 if(!source)throw new Error('Only polygon and multipolygon histories are supported');
 return source.map(p=>{const outer=ring(p[0]),center=(Math.min(...outer.map(x=>x[0]))+Math.max(...outer.map(x=>x[0])))/2;return [outer,...p.slice(1).map(r=>ring(r,center))];});
}
function nameMetadata(p,labels){
 const original=p.Name;let en=original,zh=knownZh[en]||'',wiki=p.Wikipedia||'',noteZh='',noteEn='';
 const label=labels[p.Wikidata];if(!zh&&label)zh=label.zh||'';
 // The upstream name/QID conflates ancient Eastern Zhou with tenth-century Later Zhou.
 // Correct the display name only; the source years and polygon coordinates remain unchanged.
 if(original==='Later Zhou'&&p.ToYear<0){en='Eastern Zhou';zh='东周';wiki='Eastern Zhou';noteZh='名称校订：源文件误标为 Later Zhou（后周）；此处为先秦东周。边界与有效年份仍按源记录保留。';noteEn='Name correction: the source conflates ancient Eastern Zhou with tenth-century Later Zhou. Source geometry and validity years are retained.';}
 if(p.ReferenceCode){zh=p.NameZh;noteZh='现代边界参照补全：Natural Earth 5.1.1，仅用于2024年节点；不回推历史疆域，也不是2024年实测。';noteEn='Modern reference supplement: Natural Earth 5.1.1 at the 2024 node only. Not back-projected to earlier years or presented as a 2024 survey.';}
 return {en,zh,wiki,wikidata:p.Wikidata||'',type:p.Type||'POLITY',sourceName:p.ReferenceCode?'Natural Earth reference: '+p.ReferenceCode:original,noteZh,noteEn};
}
function main(){
 const zip=fs.readFileSync(path.join(sourceDir,'cliopatria.geojson.zip'));
 if(hash(zip)!==expectedZip)throw new Error('The pinned original dataset checksum does not match');
 const input=path.join(sourceDir,fs.readdirSync(sourceDir).find(f=>f.endsWith('.geojson')));
 const source=JSON.parse(fs.readFileSync(input,'utf8'));
 if(source.type!=='FeatureCollection'||!Array.isArray(source.features))throw new Error('Invalid source feature collection');
 const originalCount=source.features.length,microstates=require('./modern_microstates.cjs').apply(source.features,root);source.features=microstates.features;
 const labelPath=path.join(sourceDir,'wikidata-labels.json'),labels=fs.existsSync(labelPath)?JSON.parse(fs.readFileSync(labelPath,'utf8')):{};
 const entities=[],entityIndex=new Map(),records=[],geometries=[],geometryIndex=new Map(),polygons=[],rings=[],points=[];
 let earliest=Infinity,latest=-Infinity,polityRecords=0,relationRecords=0;
 for(const feature of source.features){
  const p=feature.properties;if(!p||!p.Name||!Number.isInteger(p.FromYear)||!Number.isInteger(p.ToYear)||p.FromYear>p.ToYear)throw new Error('Invalid source history interval');
  if(!['POLITY','RELATION'].includes(p.Type))throw new Error('Unknown source record type');
  earliest=Math.min(earliest,p.FromYear);latest=Math.max(latest,p.ToYear);
  const entity=nameMetadata(p,labels),entityKey=JSON.stringify(entity);let entityId=entityIndex.get(entityKey);
  if(entityId===undefined){entityId=entities.length;entityIndex.set(entityKey,entityId);entities.push(entity);}
  const geometryKey=hash(JSON.stringify(feature.geometry));let geometryId=geometryIndex.get(geometryKey);
  if(geometryId===undefined){
   geometryId=geometries.length;geometryIndex.set(geometryKey,geometryId);
   const sourcePolygons=normalizedPolygons(feature.geometry),g={minX:Infinity,minY:Infinity,maxX:-Infinity,maxY:-Infinity,firstPolygon:polygons.length,polygonCount:sourcePolygons.length};
   for(const pp of sourcePolygons){const poly={firstRing:rings.length,ringCount:pp.length};for(const rr of pp){rings.push({firstPoint:points.length,pointCount:rr.length});for(const xy of rr){points.push(xy);g.minX=Math.min(g.minX,xy[0]);g.maxX=Math.max(g.maxX,xy[0]);g.minY=Math.min(g.minY,xy[1]);g.maxY=Math.max(g.maxY,xy[1]);}}polygons.push(poly);}
   geometries.push(g);
  }
  records.push({entityId,from:p.FromYear,to:p.ToYear,geometryId,area:Number(p.Area)||0,flags:p.Type==='RELATION'?1:0});
  if(p.Type==='RELATION')relationRecords++;else polityRecords++;
 }
 const entityBytes=Buffer.concat(entities.map(e=>strings([e.en,e.zh,e.wiki,e.wikidata,e.type,e.sourceName,e.noteZh,e.noteEn])));
 const header=Buffer.alloc(52);header.write('WMHIST2\0',0,'ascii');[2,entities.length,records.length,geometries.length,polygons.length,rings.length,points.length].forEach((n,i)=>header.writeUInt32LE(n,8+i*4));header.writeInt32LE(earliest,36);header.writeInt32LE(latest,40);header.writeUInt32LE(entityBytes.length,44);header.writeUInt32LE(0,48);
 const recordBytes=Buffer.alloc(records.length*32);records.forEach((r,i)=>{let p=i*32;recordBytes.writeUInt32LE(r.entityId,p);recordBytes.writeInt32LE(r.from,p+4);recordBytes.writeInt32LE(r.to,p+8);recordBytes.writeUInt32LE(r.geometryId,p+12);recordBytes.writeDoubleLE(r.area,p+16);recordBytes.writeUInt32LE(r.flags,p+24);});
 const geometryBytes=Buffer.alloc(geometries.length*40);geometries.forEach((g,i)=>{let p=i*40;[g.minX,g.minY,g.maxX,g.maxY].forEach((n,j)=>geometryBytes.writeDoubleLE(n,p+j*8));geometryBytes.writeUInt32LE(g.firstPolygon,p+32);geometryBytes.writeUInt32LE(g.polygonCount,p+36);});
 const polygonBytes=Buffer.alloc(polygons.length*8);polygons.forEach((p,i)=>{polygonBytes.writeUInt32LE(p.firstRing,i*8);polygonBytes.writeUInt32LE(p.ringCount,i*8+4);});
 const ringBytes=Buffer.alloc(rings.length*8);rings.forEach((r,i)=>{ringBytes.writeUInt32LE(r.firstPoint,i*8);ringBytes.writeUInt32LE(r.pointCount,i*8+4);});
 const pointBytes=Buffer.alloc(points.length*16);points.forEach((p,i)=>{pointBytes.writeDoubleLE(p[0],i*16);pointBytes.writeDoubleLE(p[1],i*16+8);});
 const output=Buffer.concat([header,entityBytes,recordBytes,geometryBytes,polygonBytes,ringBytes,pointBytes]);fs.mkdirSync(outputDir,{recursive:true});
 const destination=path.join(outputDir,'historical-boundaries.wmh2'),staging=destination+'.new';fs.writeFileSync(staging,output);fs.renameSync(staging,destination);
 const years=[];for(let y=latest;y>=earliest;y--)if(y!==0)years.push(y);
 fs.writeFileSync(path.join(__dirname,'history_years.generated.h'),'// Generated from pinned Cliopatria source. Annual selection does not imply annual observations.\n#pragma once\nstatic const int HISTORY_YEARS[] = {'+years.join(',')+'};\nstatic const int HISTORY_YEAR_COUNT = (int)(sizeof(HISTORY_YEARS)/sizeof(HISTORY_YEARS[0]));\n');
 const upstream=JSON.parse(fs.readFileSync(path.join(sourceDir,'UPSTREAM_VERSION.json'),'utf8').replace(/^\uFEFF/,''));
 const manifest={schemaVersion:2,sourceCommit:upstream.sourceCommit,placeLabelSource:'Wikidata CC0, 2026-11-20 snapshot',placeLabelIds:Object.keys(labels).length,source:'Cliopatria / Seshat Global History Databank',sourceUrl:'https://github.com/Seshat-Global-History-Databank/cliopatria',sourceGitBlob:'a1e7093b64990bf97cd3c31bda96de1cafed822c',sourceZipSha256:expectedZip,sourceFile:path.basename(input),sourceGeoJsonSha256:hash(fs.readFileSync(input)),license:'CC BY 4.0',citation:'https://doi.org/10.1038/s41597-025-04516-9',attribution:'Bennett et al. (2025), Cliopatria, Scientific Data 12:247; Seshat Global History Databank',sourceRecords:originalCount,derivedRecords:source.features.length,polityRecords,relationRecords,entities:entities.length,uniqueGeometries:geometries.length,polygons:polygons.length,rings:rings.length,points:points.length,earliest,latest,selectableYears:years.length,bytes:output.length,outputSha256:hash(output),modernReferences:microstates.metadata,changes:['Original source polygon coordinates retained as float64 except the six explicitly marked current-node reference replacements; longitude unwrapped for date-line rendering.','POLITY records are filled; RELATION records are supplementary and are not treated as sovereign territory.','Annual selection uses inclusive source FromYear/ToYear intervals, without interpolating geometry.','Ancient Later Zhou display label corrected to Eastern Zhou; source intervals and geometry retained.','Six explicitly marked modern microstate references at 2024 only; no historical back-projection.'],errataSource:'https://www.metmuseum.org/toah/ht/04/eac.html'};
 fs.writeFileSync(path.join(outputDir,'historical-boundaries-source.json'),JSON.stringify(manifest,null,2)+'\n');fs.writeFileSync(path.join(sourceDir,'SOURCE_SNAPSHOT.json'),JSON.stringify(manifest,null,2)+'\n');
 console.log(JSON.stringify(manifest,null,2));
}
try{main();}catch(e){console.error('FAIL: '+String(e.message).slice(0,400));process.exitCode=1;}
