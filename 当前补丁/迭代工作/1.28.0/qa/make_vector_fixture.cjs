const fs=require('fs'),path=require('path');
const rect=(x0,y0,x1,y1)=>[[x0,y0],[x1,y0],[x1,y1],[x0,y1],[x0,y0]];
const geometries=[[[rect(-10,-10,10,10),rect(-2,-2,2,2)]],[[rect(4,4,8,8)]],[[rect(170,-10,190,10)]],[[rect(-1,-1,1,1)]]];
const strings=[];for(const name of ['Outer with hole','Overlapping small state','Date line state','Alliance only'])for(const field of [name,'','','',name==='Alliance only'?'RELATION':'POLITY',name,'','']){const bytes=Buffer.from(field,'utf8'),length=Buffer.alloc(4);length.writeUInt32LE(bytes.length);strings.push(length,bytes);}
const entities=Buffer.concat(strings),records=Buffer.alloc(4*32),geo=Buffer.alloc(4*40),polygons=[],rings=[],points=[];
for(let i=0;i<4;i++){
 let p=i*32;records.writeUInt32LE(i,p);records.writeInt32LE(100,p+4);records.writeInt32LE(150,p+8);records.writeUInt32LE(i,p+12);records.writeDoubleLE(i===1?16:400,p+16);records.writeUInt32LE(i===3?1:0,p+24);
 let coords=geometries[i].flat(2);p=i*40;[Math.min(...coords.map(v=>v[0])),Math.min(...coords.map(v=>v[1])),Math.max(...coords.map(v=>v[0])),Math.max(...coords.map(v=>v[1]))].forEach((n,j)=>geo.writeDoubleLE(n,p+j*8));geo.writeUInt32LE(polygons.length,p+32);geo.writeUInt32LE(geometries[i].length,p+36);
 for(const polygon of geometries[i]){polygons.push([rings.length,polygon.length]);for(const ring of polygon){rings.push([points.length,ring.length]);points.push(...ring);}}
}
const header=Buffer.alloc(52);header.write('WMHIST2\0');[2,4,4,4,polygons.length,rings.length,points.length].forEach((v,i)=>header.writeUInt32LE(v,8+i*4));header.writeInt32LE(100,36);header.writeInt32LE(200,40);header.writeUInt32LE(entities.length,44);
const table=rows=>{const b=Buffer.alloc(rows.length*8);rows.forEach((r,i)=>{b.writeUInt32LE(r[0],i*8);b.writeUInt32LE(r[1],i*8+4);});return b;};const pointBytes=Buffer.alloc(points.length*16);points.forEach((p,i)=>{pointBytes.writeDoubleLE(p[0],i*16);pointBytes.writeDoubleLE(p[1],i*16+8);});
fs.writeFileSync(path.join(__dirname,'development/vector-fixture.wmh2'),Buffer.concat([header,entities,records,geo,table(polygons),table(rings),pointBytes]));console.log('Synthetic independent geometry fixture written');
