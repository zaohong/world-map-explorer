const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const output = process.argv[2] || path.join(__dirname, 'world_map_explorer.ico');
const sizes = [256, 128, 64, 48, 32, 24, 16];

function polygonContains(x, y, points) {
  let inside = false;
  for (let i = 0, j = points.length - 1; i < points.length; j = i++) {
    const [xi, yi] = points[i], [xj, yj] = points[j];
    if ((yi > y) !== (yj > y) && x < ((xj - xi) * (y - yi)) / (yj - yi) + xi) inside = !inside;
  }
  return inside;
}

const landShapes = [
  [[.19,.35],[.25,.27],[.34,.25],[.40,.30],[.37,.37],[.32,.40],[.30,.47],[.25,.43]],
  [[.38,.46],[.43,.49],[.45,.57],[.42,.64],[.40,.72],[.36,.64],[.35,.55]],
  [[.48,.34],[.54,.31],[.58,.34],[.56,.39],[.51,.40],[.49,.37]],
  [[.52,.42],[.59,.39],[.63,.45],[.61,.53],[.58,.63],[.54,.61],[.51,.52]],
  [[.60,.34],[.68,.29],[.78,.32],[.82,.39],[.75,.42],[.69,.39],[.64,.43]],
  [[.72,.55],[.78,.54],[.81,.58],[.77,.62],[.72,.60]]
];

function sample(x, y) {
  const left=.035, right=.965, top=.035, bottom=.965, radius=.20;
  const qx=Math.max(left+radius,Math.min(right-radius,x));
  const qy=Math.max(top+radius,Math.min(bottom-radius,y));
  if ((x-qx)**2+(y-qy)**2 > radius**2) return [0,0,0,0];
  let c=[8+Math.round((1-y)*4),22+Math.round((1-y)*13),39+Math.round((1-y)*20),255];
  const glow=Math.max(0,1-Math.hypot(x-.29,y-.18)/.58)*.15;
  c=[c[0]+Math.round(20*glow),c[1]+Math.round(100*glow),c[2]+Math.round(125*glow),255];

  const dx=x-.50,dy=y-.50,r=Math.hypot(dx,dy),R=.365;
  if(r<=R){
    const light=Math.max(0,1-r/R);
    c=[Math.round(7+23*light),Math.round(56+91*light),Math.round(91+112*light),255];
    const gx=dx/R,gy=dy/R;
    const continent=landShapes.some(p=>polygonContains(x,y,p));
    if(continent)c=[83,180,141,255];
    const meridianA=.48*Math.sqrt(Math.max(0,1-gy*gy));
    const meridianB=.78*Math.sqrt(Math.max(0,1-gy*gy));
    const latitudeA=.43*Math.sqrt(Math.max(0,1-gx*gx));
    const latitudeB=.78*Math.sqrt(Math.max(0,1-gx*gx));
    const grid=Math.abs(gx)<.026 || Math.abs(Math.abs(gx)-meridianA)<.018 || Math.abs(Math.abs(gx)-meridianB)<.018 || Math.abs(gy)<.025 || Math.abs(Math.abs(gy)-latitudeA)<.018 || Math.abs(Math.abs(gy)-latitudeB)<.018;
    if(grid)c=[122,206,208,255];
    if(Math.abs(r-R)<.012)c=[177,239,228,255];
  }
  const marker=Math.hypot(x-.675,y-.335);
  if(marker<.040)c=marker<.020?[255,211,119,255]:[242,170,82,255];
  return c;
}

function renderPng(size) {
  const scale=3, n=size*scale, high=Buffer.alloc(n*n*4);
  for(let py=0;py<n;py++)for(let px=0;px<n;px++){
    const x=(px+.5)/n,y=(py+.5)/n,c=sample(x,y),at=(py*n+px)*4;
    high[at]=c[0];high[at+1]=c[1];high[at+2]=c[2];high[at+3]=c[3];
  }
  const pixels=Buffer.alloc(size*size*4);
  for(let y=0;y<size;y++)for(let x=0;x<size;x++){
    let a=0,r=0,g=0,b=0;
    for(let sy=0;sy<scale;sy++)for(let sx=0;sx<scale;sx++){
      const at=(((y*scale+sy)*n)+(x*scale+sx))*4;a+=high[at+3];r+=high[at];g+=high[at+1];b+=high[at+2];
    }
    const to=(y*size+x)*4, count=scale*scale;
    pixels[to]=Math.round(r/count);pixels[to+1]=Math.round(g/count);pixels[to+2]=Math.round(b/count);pixels[to+3]=Math.round(a/count);
  }
  const rows=Buffer.alloc(size*(1+size*4));
  for(let y=0;y<size;y++)pixels.copy(rows,y*(1+size*4)+1,y*size*4,(y+1)*size*4);
  const crcTable=new Uint32Array(256);
  for(let n=0;n<256;n++){let c=n;for(let k=0;k<8;k++)c=(c&1)?(0xedb88320^(c>>>1)):(c>>>1);crcTable[n]=c>>>0;}
  function crc32(buf){let c=0xffffffff;for(const v of buf)c=crcTable[(c^v)&255]^(c>>>8);return (c^0xffffffff)>>>0;}
  function chunk(type,data){const name=Buffer.from(type),out=Buffer.alloc(12+data.length);out.writeUInt32BE(data.length,0);name.copy(out,4);data.copy(out,8);out.writeUInt32BE(crc32(out.subarray(4,8+data.length)),8+data.length);return out;}
  const ihdr=Buffer.alloc(13);ihdr.writeUInt32BE(size,0);ihdr.writeUInt32BE(size,4);ihdr[8]=8;ihdr[9]=6;
  return Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',ihdr),chunk('IDAT',zlib.deflateSync(rows,{level:9})),chunk('IEND',Buffer.alloc(0))]);
}

const images=sizes.map(size=>({size,png:renderPng(size)}));
const header=Buffer.alloc(6+images.length*16);header.writeUInt16LE(0,0);header.writeUInt16LE(1,2);header.writeUInt16LE(images.length,4);
let offset=header.length;
images.forEach((entry,i)=>{const at=6+i*16;header[at]=entry.size===256?0:entry.size;header[at+1]=entry.size===256?0:entry.size;header[at+2]=0;header[at+3]=0;header.writeUInt16LE(1,at+4);header.writeUInt16LE(32,at+6);header.writeUInt32LE(entry.png.length,at+8);header.writeUInt32LE(offset,at+12);offset+=entry.png.length;});
fs.mkdirSync(path.dirname(output),{recursive:true});fs.writeFileSync(output,Buffer.concat([header,...images.map(entry=>entry.png)]));
if(process.argv[3])fs.writeFileSync(process.argv[3],images[0].png);
console.log(`Wrote ${images.length} icon sizes to ${output}`);
