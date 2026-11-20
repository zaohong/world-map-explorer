using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Threading;

internal static class SearchInputUiChecks {
    [StructLayout(LayoutKind.Sequential)] struct RECT { public int left,top,right,bottom; }
    [StructLayout(LayoutKind.Sequential)] struct POINT { public int x,y; }
    [DllImport("user32.dll")] static extern IntPtr GetDlgItem(IntPtr h,int id);
    [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
    [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)] static extern IntPtr SendText(IntPtr h,uint m,IntPtr w,string value);
    [DllImport("user32.dll")] static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h,out RECT r);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h,out RECT r);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr h,ref POINT p);
    [DllImport("user32.dll")] static extern bool PrintWindow(IntPtr h,IntPtr dc,uint flags);
    static void Search(IntPtr window,string text){SendText(GetDlgItem(window,1002),0x000c,IntPtr.Zero,text);SendMessage(GetDlgItem(window,1003),0x00f5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(800);}
    static string MapHash(IntPtr window,string path){
        RECT outer,client;POINT origin=new POINT();if(!GetWindowRect(window,out outer)||!GetClientRect(window,out client)||!ClientToScreen(window,ref origin))throw new Exception("Cannot locate map");
        int side=client.right<1200?300:360,toolbar=client.right<1200?150:116,aw=client.right-side-36-20,ah=client.bottom-108-toolbar,mw=aw,mh=mw/2;if(mh>ah){mh=ah;mw=mh*2;}
        var crop=new Rectangle(origin.x-outer.left+24,origin.y-outer.top+90+toolbar+(ah-mh)/2,mw,mh);
        using(var image=new Bitmap(outer.right-outer.left,outer.bottom-outer.top))using(var graphics=Graphics.FromImage(image)){
            IntPtr dc=graphics.GetHdc();bool ok;try{ok=PrintWindow(window,dc,2);}finally{graphics.ReleaseHdc(dc);}if(!ok)throw new Exception("PrintWindow failed");
            using(var map=image.Clone(crop,PixelFormat.Format32bppArgb))using(var bytes=new MemoryStream())using(var sha=SHA256.Create()){
                map.Save(path,ImageFormat.Png);map.Save(bytes,ImageFormat.Png);return BitConverter.ToString(sha.ComputeHash(bytes.ToArray()));
            }
        }
    }
    static int failures=0;
    static void Check(bool ok,string label){Console.WriteLine((ok?"PASS: ":"FAIL: ")+label);if(!ok)failures++;}
    static int Main(string[] args){
        if(args.Length!=2)return 2;Directory.CreateDirectory(args[1]);
        var start=new ProcessStartInfo(args[0]){WorkingDirectory=Path.GetDirectoryName(args[0]),UseShellExecute=false};start.EnvironmentVariables["LOCALAPPDATA"]=Path.Combine(args[1],"profile");
        var process=Process.Start(start);IntPtr window=IntPtr.Zero;
        try{
            for(int i=0;i<120;i++){process.Refresh();window=process.MainWindowHandle;if(window!=IntPtr.Zero&&IsWindowVisible(window)&&GetDlgItem(window,1065)!=IntPtr.Zero)break;Thread.Sleep(250);}
            if(window==IntPtr.Zero)throw new Exception("Application did not open");Thread.Sleep(3000);
            Search(window,"39.9,116.4");string beijing=MapHash(window,Path.Combine(args[1],"beijing-ascii.png"));
            Search(window,"51.5,-0.12");Check(MapHash(window,Path.Combine(args[1],"london.png"))!=beijing,"different valid coordinates select different views");
            Search(window,"39.9，116.4");Check(MapHash(window,Path.Combine(args[1],"beijing-chinese-comma.png"))==beijing,"Chinese comma selects the same coordinates");
            string[] invalid={"51.5,-0.12garbage","51.5,-0.12,2","Z9800-050-030-extra","Z12-34-56"};
            for(int i=0;i<invalid.Length;i++){Search(window,"39.9,116.4");Search(window,invalid[i]);Check(MapHash(window,Path.Combine(args[1],"invalid-"+i+".png"))==beijing,"invalid input leaves the selected map view unchanged: "+invalid[i]);}
            Search(window,"Z627200-0248-0591");Check((int)SendMessage(GetDlgItem(window,1008),0x0147,IntPtr.Zero,IntPtr.Zero)==3,"valid fine-grid ID selects its grid level");
            Search(window,"Z12-34");Check(MapHash(window,Path.Combine(args[1],"legacy.png"))!=beijing,"valid legacy ID still locates a different view");
            Console.WriteLine("RESULT failures="+failures);return failures==0?0:1;
        }catch(Exception e){Console.WriteLine("FAIL: "+e.Message);return 1;}
        finally{if(window!=IntPtr.Zero)PostMessage(window,0x0010,IntPtr.Zero,IntPtr.Zero);if(!process.WaitForExit(5000))process.Kill();}
    }
}
