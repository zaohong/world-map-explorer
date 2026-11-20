const fs=require('fs'),path=require('path');const root=path.resolve(__dirname,'..');
const all=JSON.parse(fs.readFileSync(path.join(__dirname,'development/route-geometry.json'),'utf8'));
const grid=JSON.parse(fs.readFileSync(path.resolve(root,'../../WorldMapExplorer-1.28.0/data/world-2026.json'),'utf8'));
const country=new Map(grid.countries.map(c=>[c.id,c.code]));
function cell(lat,lon){while(lon>180)lon-=360;while(lon< -180)lon+=360;let col=Math.max(0,Math.min(1119,Math.floor((lon+180)/360*1120))),row=Math.max(0,Math.min(559,Math.floor((90-lat)/180*560)));return grid.cells[row*1120+col];}
function distance(a,b){const rad=Math.PI/180;let la=(b[0]-a[0])*rad,lo=(b[1]-a[1])*rad;return 12742*Math.asin(Math.min(1,Math.sqrt(Math.sin(la/2)**2+Math.cos(a[0]*rad)*Math.cos(b[0]*rad)*Math.sin(lo/2)**2)));}
const audit=[];
for(const route of all.filter(r=>r.sea)){let runs=[];
for(let i=1;i<route.points.length;i++){let a=route.points[i-1],b=route.points[i];if(b[2])continue;let dx=b[1]-a[1];if(dx>180)dx-=360;if(dx< -180)dx+=360;let km=distance(a,b),n=Math.max(1,Math.ceil(km/10)),run=0,lastCodes=new Set(),runStart=null;
 for(let j=0;j<=n;j++){let t=j/n,lat=a[0]+(b[0]-a[0])*t,lon=a[1]+dx*t;const id=cell(lat,lon),inside=id&&t*km>25&&(1-t)*km>25;
  if(inside){if(!run)runStart=[+lat.toFixed(2),+lon.toFixed(2)];run+=km/n;lastCodes.add(country.get(id));}
  if((!inside||j===n)&&run){if(run>=60)runs.push({segment:i,landKm:Math.round(run),start:runStart,countries:[...lastCodes]});run=0;lastCodes=new Set();}
 }
}
audit.push({id:route.id,review:runs});}
fs.writeFileSync(path.join(__dirname,'development/sea-connection-audit.json'),JSON.stringify(audit,null,2));
for(const r of audit)console.log(r.id,JSON.stringify(r.review));
console.log('Diagnostic uses modern 627200-cell land mask, ~10 km samples, ignores 25 km at endpoints. It can flag coastal schematic approximations and does not establish a historical track.');
