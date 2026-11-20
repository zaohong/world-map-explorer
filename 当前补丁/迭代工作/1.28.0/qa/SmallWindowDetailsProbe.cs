using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

internal static class SmallWindowDetailsProbe {
    delegate bool EnumProc(IntPtr h,IntPtr data);
    [StructLayout(LayoutKind.Sequential)] struct RECT { public int left,top,right,bottom; }
    [StructLayout(LayoutKind.Sequential)] struct POINT { public int x,y; }
    [DllImport("user32.dll")] static extern IntPtr GetDlgItem(IntPtr h,int id);
    [DllImport("user32.dll")] static extern int GetDlgCtrlID(IntPtr h);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc callback,IntPtr data);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr h,EnumProc callback,IntPtr data);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h,out uint process);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr h,StringBuilder name,int length);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern IntPtr FindWindow(string className,string title);
    [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
    [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)] static extern IntPtr SendText(IntPtr h,uint m,IntPtr w,string value);
    [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)] static extern IntPtr ReadText(IntPtr h,uint m,IntPtr w,StringBuilder value);
    [DllImport("user32.dll")] static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h,out RECT r);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h,out RECT r);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr h,ref POINT p);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr h,IntPtr after,int x,int y,int width,int height,uint flags);
    [DllImport("user32.dll")] static extern bool PrintWindow(IntPtr h,IntPtr dc,uint flags);
    static IntPtr Pack(int x,int y){return (IntPtr)((y<<16)|(x&65535));}
    static void Click(IntPtr w,int id){SendMessage(GetDlgItem(w,id),0x00f5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(800);}
    static void Search(IntPtr w,string text){SendText(GetDlgItem(w,1002),0x000c,IntPtr.Zero,text);Click(w,1003);}
    static void Wheel(IntPtr w,int x,int y,int delta){POINT p=new POINT{x=x,y=y};ClientToScreen(w,ref p);SendMessage(w,0x020a,(IntPtr)(delta<<16),Pack(p.x,p.y));Thread.Sleep(500);}
    static string Capture(IntPtr w,string path,Rectangle? crop=null){RECT r;if(!GetWindowRect(w,out r))throw new Exception("No window bounds");using(var bmp=new Bitmap(r.right-r.left,r.bottom-r.top))using(var g=Graphics.FromImage(bmp)){
        IntPtr dc=g.GetHdc();bool ok;try{ok=PrintWindow(w,dc,2);}finally{g.ReleaseHdc(dc);}if(!ok)throw new Exception("PrintWindow failed");bmp.Save(path,ImageFormat.Png);
        using(var sub=crop.HasValue?bmp.Clone(crop.Value,PixelFormat.Format32bppArgb):(Bitmap)bmp.Clone())using(var stream=new MemoryStream())using(var sha=SHA256.Create()){sub.Save(stream,ImageFormat.Png);return BitConverter.ToString(sha.ComputeHash(stream.ToArray()));}
    }}
    static void Check(bool condition,string label){if(!condition)throw new Exception(label);Console.WriteLine("PASS: "+label);}
    static IntPtr WaitPicker(){for(int i=0;i<100;i++){IntPtr p=FindWindow("WorldMapExplorerPicker",null);if(p!=IntPtr.Zero&&IsWindowVisible(p))return p;Thread.Sleep(50);}throw new Exception("Picker did not open");}
    static void CheckPicker(IntPtr owner,IntPtr picker){RECT outer,parent,client;POINT origin=new POINT();GetWindowRect(picker,out outer);GetWindowRect(owner,out parent);GetClientRect(picker,out client);ClientToScreen(picker,ref origin);
        Check(outer.left>=parent.left&&outer.top>=parent.top&&outer.right<=parent.right&&outer.bottom<=parent.bottom,"picker fits inside the small owner window");
        foreach(int id in new[]{10,11,1,2}){RECT r;GetWindowRect(GetDlgItem(picker,id),out r);Check(r.left>=origin.x&&r.top>=origin.y&&r.right<=origin.x+client.right&&r.bottom<=origin.y+client.bottom,"picker control "+id+" is inside its client area");}
    }
    static IntPtr SaveDialog(Process process,IntPtr owner){IntPtr found=IntPtr.Zero;EnumWindows((h,p)=>{uint id;GetWindowThreadProcessId(h,out id);var name=new StringBuilder(80);GetClassName(h,name,name.Capacity);if(id==process.Id&&h!=owner&&IsWindowVisible(h)&&name.ToString()=="#32770"){found=h;return false;}return true;},IntPtr.Zero);return found;}
    static IntPtr Filename(IntPtr dialog){IntPtr found=IntPtr.Zero;EnumChildWindows(dialog,(h,p)=>{var name=new StringBuilder(80);GetClassName(h,name,name.Capacity);if(GetDlgCtrlID(h)==1001&&name.ToString()=="Edit"){found=h;return false;}return true;},IntPtr.Zero);return found;}
    static void Export(IntPtr w,Process process,string file,int expectedWidth){PostMessage(GetDlgItem(w,1049),0x00f5,IntPtr.Zero,IntPtr.Zero);IntPtr dialog=IntPtr.Zero;for(int i=0;i<200&&dialog==IntPtr.Zero;i++){dialog=SaveDialog(process,w);Thread.Sleep(50);}Check(dialog!=IntPtr.Zero,"small-window save dialog opens");IntPtr edit=Filename(dialog);Check(edit!=IntPtr.Zero,"small-window save filename is available");SendText(edit,0x000c,IntPtr.Zero,file);PostMessage(GetDlgItem(dialog,1),0x00f5,IntPtr.Zero,IntPtr.Zero);
        bool valid=false;for(int i=0;i<300;i++){try{if(File.Exists(file)){using(var image=Image.FromFile(file))valid=image.RawFormat.Guid==ImageFormat.Png.Guid&&image.Width==expectedWidth&&image.Height>expectedWidth/2+50;if(valid)break;}}catch{}Thread.Sleep(50);}Check(valid,"small-window export contains a PNG at the map viewport width");}
    static int Main(string[] args){if(args.Length!=2)return 2;Directory.CreateDirectory(args[1]);var start=new ProcessStartInfo(args[0]){WorkingDirectory=Path.GetDirectoryName(args[0]),UseShellExecute=false};start.EnvironmentVariables["LOCALAPPDATA"]=Path.Combine(args[1],"profile");var process=Process.Start(start);IntPtr w=IntPtr.Zero;
        try{for(int i=0;i<120;i++){process.Refresh();w=process.MainWindowHandle;if(w!=IntPtr.Zero&&IsWindowVisible(w))break;Thread.Sleep(250);}if(w==IntPtr.Zero)throw new Exception("Window not found");Thread.Sleep(3000);
            foreach(var size in new[]{new{Width=640,Height=480},new{Width=800,Height=600}}){
                if(!SetWindowPos(w,IntPtr.Zero,0,0,size.Width,size.Height,0x0416))throw new Exception("Resize failed");Thread.Sleep(700);RECT client,outer;POINT origin=new POINT();GetClientRect(w,out client);GetWindowRect(w,out outer);ClientToScreen(w,ref origin);
                int side=client.right<820?240+Math.Max(0,client.right-760):300,infoLeft=client.right-side+8,infoTop=256,infoBottom=client.bottom-62;
                int availableWidth=client.right-side-56,availableHeight=client.bottom-108-150-32,mapWidth=availableWidth,mapHeight=mapWidth/2;if(mapHeight>availableHeight){mapHeight=availableHeight;mapWidth=availableHeight*2;}
                int mapLeft=origin.x-outer.left+24,mapTop=origin.y-outer.top+90+150+32+(availableHeight-mapHeight)/2;
                string metadata=Path.Combine(args[1],"viewport.csv");if(!File.Exists(metadata))File.WriteAllText(metadata,"window_width,map_left,map_top,map_width,map_height\n");File.AppendAllText(metadata,size.Width+","+mapLeft+","+mapTop+","+mapWidth+","+mapHeight+"\n");
                var panel=new Rectangle(origin.x-outer.left+infoLeft,origin.y-outer.top+infoTop,side-26,Math.Max(1,infoBottom-infoTop));
                Search(w,"39.9,116.4");string selected=Capture(w,Path.Combine(args[1],"selected-"+size.Width+".png"),panel);
                Wheel(w,infoLeft+side/2,infoTop+(infoBottom-infoTop)/2,-120);string scrolled=Capture(w,Path.Combine(args[1],"selected-scrolled-"+size.Width+".png"),panel);Check(selected!=scrolled,"region details scroll at width "+size.Width);
                Click(w,1019);Capture(w,Path.Combine(args[1],"events-"+size.Width+".png"));Check(IsWindowVisible(GetDlgItem(w,1049)),"export remains visible at width "+size.Width);Click(w,1018);
                if(size.Width==640){PostMessage(GetDlgItem(w,1047),0x00f5,IntPtr.Zero,IntPtr.Zero);IntPtr picker=WaitPicker();CheckPicker(w,picker);Capture(picker,Path.Combine(args[1],"year-picker-640.png"));SendText(GetDlgItem(picker,10),0x000c,IntPtr.Zero,"1600");Check((int)SendMessage(GetDlgItem(picker,11),0x018b,IntPtr.Zero,IntPtr.Zero)>0,"small year picker filters results");PostMessage(picker,0x0111,(IntPtr)1,IntPtr.Zero);Thread.Sleep(1000);var year=new StringBuilder(128);ReadText(GetDlgItem(w,1047),0x000d,(IntPtr)year.Capacity,year);Check(year.ToString().Contains("1600"),"small year picker applies the selection");
                    Click(w,1046);PostMessage(GetDlgItem(w,1048),0x00f5,IntPtr.Zero,IntPtr.Zero);picker=WaitPicker();CheckPicker(w,picker);Capture(picker,Path.Combine(args[1],"route-picker-640.png"));SendText(GetDlgItem(picker,10),0x000c,IntPtr.Zero,"长征");Check((int)SendMessage(GetDlgItem(picker,11),0x018b,IntPtr.Zero,IntPtr.Zero)>0,"small route picker filters results");PostMessage(picker,0x0111,(IntPtr)2,IntPtr.Zero);Thread.Sleep(500);}
                Export(w,process,Path.Combine(args[1],"small-"+size.Width+"-export.png"),mapWidth);
            }
            Click(w,1024);SetWindowPos(w,IntPtr.Zero,0,0,640,480,0x0416);Thread.Sleep(600);
            PostMessage(GetDlgItem(w,1047),0x00f5,IntPtr.Zero,IntPtr.Zero);IntPtr englishPicker=WaitPicker();CheckPicker(w,englishPicker);var select=new StringBuilder(64);ReadText(GetDlgItem(englishPicker,1),0x000d,(IntPtr)select.Capacity,select);Check(select.ToString()=="Select","small year picker uses English labels");Capture(englishPicker,Path.Combine(args[1],"year-picker-english-640.png"));SendText(GetDlgItem(englishPicker,10),0x000c,IntPtr.Zero,"not-a-year-xyz");Check((int)SendMessage(GetDlgItem(englishPicker,11),0x018b,IntPtr.Zero,IntPtr.Zero)==0,"small picker shows no matches for an unknown year");PostMessage(englishPicker,0x0111,(IntPtr)1,IntPtr.Zero);Thread.Sleep(300);Check(IsWindowVisible(englishPicker),"selecting with no match keeps the picker open");PostMessage(englishPicker,0x0111,(IntPtr)2,IntPtr.Zero);Thread.Sleep(500);
            PostMessage(GetDlgItem(w,1048),0x00f5,IntPtr.Zero,IntPtr.Zero);englishPicker=WaitPicker();CheckPicker(w,englishPicker);SendText(GetDlgItem(englishPicker,10),0x000c,IntPtr.Zero,"Long March");Check((int)SendMessage(GetDlgItem(englishPicker,11),0x018b,IntPtr.Zero,IntPtr.Zero)>0,"small English route picker filters results");Capture(englishPicker,Path.Combine(args[1],"route-picker-english-640.png"));PostMessage(englishPicker,0x0111,(IntPtr)2,IntPtr.Zero);Thread.Sleep(500);return 0;
        }catch(Exception e){Console.WriteLine("FAIL: "+e.Message);return 1;}
        finally{if(w!=IntPtr.Zero)PostMessage(w,0x0010,IntPtr.Zero,IntPtr.Zero);if(!process.WaitForExit(5000))process.Kill();}
    }
}
