#include "../native/map_viewport.h"
#include <iostream>
#include <vector>
int main(){int checks=0;auto check=[&](bool ok,const char* reason){++checks;if(!ok){std::cerr<<"FAIL "<<reason<<"\n";exit(1);}};
 for(RECT r:std::vector<RECT>{{0,0,800,400},{18,124,966,768},{18,200,502,600},{0,0,720,1280},{0,0,1920,300}})for(double zoom:{1.,2.,4.,12.,64.}){
  mapViewport::Projection view{r,zoom,.5,.5};check(std::abs(view.width()/360-view.height()/180)<1e-12,"equal horizontal and vertical degrees per pixel");
  for(int row=0;row<=18;++row)for(int col=0;col<=36;++col){double x=col/36.,y=row/18.;check(std::abs(view.worldX(view.screenX(x))-x)<1e-12&&std::abs(view.worldY(view.screenY(y))-y)<1e-12,"forward/inverse roundtrip across world grid");}
  for(double x:{-.1,.2,.5,.9,1.1})for(double y:{-.1,.2,.5,.9,1.1}){auto p=view;p.centerX=x;p.centerY=y;p.clamp();check(p.centerX>=p.halfX()-1e-12&&p.centerX<=1-p.halfX()+1e-12&&p.centerY>=p.halfY()-1e-12&&p.centerY<=1-p.halfY()+1e-12,"clamp reserves full visible extent");}
  if(zoom>=4)check(view.worldY(r.top)<.5-.5/zoom||r.bottom-r.top<=view.height()+1e-8,"tall view reveals extra latitude rather than stretching the world");
 }
 mapViewport::Projection landscape{{0,0,1200,600},12,.5,.5},portrait{{0,0,300,900},12,.5,.5},medium{{0,0,720,1280},12,.5,.5};check(mapViewport::visibleTiles(landscape)==8&&mapViewport::detailFits(landscape,2400),"eight 500 m tiles fit the estimated pixel budget");check(mapViewport::visibleTiles(portrait)>mapViewport::visibleTiles(landscape)&&!mapViewport::detailFits(portrait,2400),"tall fields of view cannot silently multiply the detail budget");check(mapViewport::detailFits(medium,1200)&&!mapViewport::detailFits(medium,2400),"a wider latitude view can use the 1 km detail tier within budget");
 std::cout<<checks<<" viewport checks passed\n";
}
