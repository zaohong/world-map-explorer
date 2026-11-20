const fs=require('fs'),path=require('path'),crypto=require('crypto');const root=path.resolve(__dirname,'..'),release=path.resolve(root,'../../WorldMapExplorer-1.28.0');
const input=path.join(root,'source-data/geonames'),sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const snapshot=JSON.parse(fs.readFileSync(path.join(input,'SOURCE_SNAPSHOT.json'),'utf8').replace(/^\uFEFF/,''));for(const entry of snapshot.files)if(sha(path.join(input,entry.file)).toUpperCase()!==entry.sha256.toUpperCase())throw Error('Source checksum mismatch: '+entry.file);
const world=JSON.parse(fs.readFileSync(path.join(release,'data/world-2026.json'),'utf8')),zones=JSON.parse(fs.readFileSync(path.join(release,'data/zones.json'),'utf8'));
const countryMap=new Map(world.countries.map(c=>[c.code,c])),countries=new Map(),admins=new Map(),chinese=new Map();
// Supplemental search aliases do not change source names, coordinates or sovereignty data.
const commonCountryNames={CHN:'中国,中国大陆',USA:'美国,美利坚',GBR:'英国,联合王国',RUS:'俄罗斯,俄国',KOR:'韩国,南韩',PRK:'朝鲜,北韩',IRN:'伊朗',VNM:'越南',LAO:'老挝',TZA:'坦桑尼亚',VEN:'委内瑞拉',BOL:'玻利维亚',SYR:'叙利亚',COG:'刚果布',COD:'刚果金',CZE:'捷克',BRN:'文莱',ARE:'阿联酋',TWN:'台湾',MKD:'北马其顿'};
for(const line of fs.readFileSync(path.join(input,'countryInfo.txt'),'utf8').split(/\r?\n/)){if(!line||line[0]==='#')continue;const c=line.split('\t');if(c.length>=17)countries.set(c[0],{en:c[4],a3:c[1],zh:countryMap.get(c[1])?.nameZh||''});}
for(const line of fs.readFileSync(path.join(input,'admin1CodesASCII.txt'),'utf8').split(/\r?\n/)){const c=line.split('\t');if(c.length>=3)admins.set(c[0],c[2]);}
for(const zone of zones.zones)for(const m of (zone.cities||'').matchAll(/([^；（）]+)（([A-Za-z][^（）]*)）/g)){let zh=m[1].trim(),en=m[2].trim();if(!/[0-9]/.test(en)&&zh.length<25&&!chinese.has(en.toLowerCase()))chinese.set(en.toLowerCase(),zh);}
const rows=[],ids=new Set(),invalid=[];
for(const line of fs.readFileSync(path.join(input,'cities15000.txt'),'utf8').split(/\r?\n/)){
 if(!line)continue;const c=line.split('\t');if(c.length!==19){invalid.push('column count');continue;}
 let lat=+c[4],lon=+c[5],population=+c[14];if(!Number.isFinite(lat)||Math.abs(lat)>90||!Number.isFinite(lon)||Math.abs(lon)>180||!Number.isFinite(population)||population<0){invalid.push(c[0]);continue;}
 const id='g:'+c[0];if(ids.has(id))throw Error('Duplicate GeoNames ID');ids.add(id);
 const aliases=c[3].split(','),zh=chinese.get(c[2].toLowerCase())||chinese.get(c[1].toLowerCase())||aliases.find(n=>/^[\u4e00-\u9fff·]{2,14}$/.test(n))||'';
 const country=countries.get(c[8])||{en:c[8],zh:''};
 rows.push(['city',id,c[2]||c[1],zh,country.en,country.zh,admins.get(c[8]+'.'+c[10])||'',lat,lon,population,c[7],[c[1],c[2],...aliases,c[8],country.a3,commonCountryNames[country.a3]||''].join(','),12]);
}
// Natural Earth country labels are explicit map reference points, not capitals.
const countrySource=path.join(root,'source-data/natural-earth/ne_10m_admin_0_countries.dbf');
const buffer=fs.readFileSync(countrySource),fields=[];let offset=1;for(let p=32;p<buffer.readUInt16LE(8)&&buffer[p]!==13;p+=32){let name=buffer.toString('ascii',p,p+11).replace(/\0/g,''),size=buffer[p+16];fields.push({name,size,offset});offset+=size;}
for(let i=0;i<buffer.readUInt32LE(4);++i){let p=buffer.readUInt16LE(8)+i*buffer.readUInt16LE(10),c={};for(const f of fields)c[f.name]=buffer.toString('utf8',p+f.offset,p+f.offset+f.size).trim();const known=countryMap.get(c.ADM0_A3);if(!known)continue;let lat=+c.LABEL_Y,lon=+c.LABEL_X;if(!Number.isFinite(lat)||!Number.isFinite(lon))continue;rows.push(['country','c:'+c.ADM0_A3,known.name,known.nameZh,known.name,known.nameZh,'',lat,lon,0,'COUNTRY',[c.ADMIN,c.NAME,c.NAME_LONG,c.NAME_ZH,c.ADM0_A3,commonCountryNames[c.ADM0_A3]||''].join(','),['RUS','CAN','CHN','USA','BRA','AUS'].includes(c.ADM0_A3)?3:(['MCO','VAT','LIE','SMR','AND'].includes(c.ADM0_A3)?24:6)]);}
const clean=v=>String(v??'').replace(/[\t\r\n\0]/g,' ').trim();
const text='# PLACES_V1\ttype\tid\tname_en\tname_zh\tcountry_en\tcountry_zh\tadmin_en\tlatitude\tlongitude\tpopulation_rank_only\tfeature_code\taliases\tfocus_zoom\n'+rows.map(r=>r.map(clean).join('\t')).join('\n')+'\n';
const file=path.join(release,'data/place-search.tsv');fs.writeFileSync(file,text);
const meta={schemaVersion:1,snapshot:'2026-11-20',cities:rows.filter(r=>r[0]==='city').length,countries:rows.filter(r=>r[0]==='country').length,invalidRows:invalid,bytes:Buffer.byteLength(text),sha256:sha(file),sources:[{name:'GeoNames cities15000, countryInfo, admin1CodesASCII',license:'CC BY 4.0',url:'https://download.geonames.org/export/dump/readme.txt',snapshotFiles:snapshot.files},{name:'Natural Earth Admin 0 5.1.1 label points',license:'Public domain',sourceSha256:sha(countrySource),url:'https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-countries/'}],method:'City coordinates are source reference points, not grid centres. Chinese display labels prefer existing GeoNames aliases where available; aliases are retained for bilingual matching. Population is used only for ranking homonyms, not shown as a current census count. Country coordinates are cartographic label points, not capitals.'};
fs.writeFileSync(path.join(release,'data/place-search-source.json'),JSON.stringify(meta,null,2));console.log(JSON.stringify({...meta,sources:meta.sources.map(s=>({name:s.name,license:s.license}))},null,2));
