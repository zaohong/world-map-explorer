using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

internal static class HistoryPlaybackRegression {
    [StructLayout(LayoutKind.Sequential)] struct RECT { public int left,top,right,bottom; }
    [DllImport("user32.dll")] static extern IntPtr GetDlgItem(IntPtr hwnd,int id);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr hwnd,uint message,IntPtr w,IntPtr l);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,EntryPoint="SendMessageW")] static extern IntPtr SendText(IntPtr hwnd,uint message,IntPtr w,string text);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,EntryPoint="SendMessageW")] static extern IntPtr ReadText(IntPtr hwnd,uint message,IntPtr w,StringBuilder text);
    [DllImport("user32.dll")] static extern bool PostMessage(IntPtr hwnd,uint message,IntPtr w,IntPtr l);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern IntPtr FindWindow(string cls,string title);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr hwnd,out RECT rect);
    [DllImport("user32.dll")] static extern bool PrintWindow(IntPtr hwnd,IntPtr dc,uint flags);
    static int failures;
    static void Check(bool value,string name){Console.WriteLine((value?"PASS: ":"FAIL: ")+name);if(!value)failures++;}
    static string Text(IntPtr hwnd){var buffer=new StringBuilder(512);ReadText(hwnd,0x000d,(IntPtr)buffer.Capacity,buffer);return buffer.ToString();}
    static string ComboText(IntPtr combo,int index){var buffer=new StringBuilder(256);ReadText(combo,0x0148,(IntPtr)index,buffer);return buffer.ToString();}
    static void Click(IntPtr window,int id){SendMessage(GetDlgItem(window,id),0x00f5,IntPtr.Zero,IntPtr.Zero);}
    static int Index(IntPtr combo){return (int)SendMessage(combo,0x0147,IntPtr.Zero,IntPtr.Zero);}
    static bool WaitFor(Func<bool> predicate,int timeout){var sw=Stopwatch.StartNew();while(sw.ElapsedMilliseconds<timeout){if(predicate())return true;Thread.Sleep(40);}return predicate();}
    static void Screenshot(IntPtr window,string path){RECT r;if(!GetWindowRect(window,out r))return;using(var bitmap=new Bitmap(r.right-r.left,r.bottom-r.top))using(var graphics=Graphics.FromImage(bitmap)){var dc=graphics.GetHdc();bool ok=PrintWindow(window,dc,2);graphics.ReleaseHdc(dc);if(ok)bitmap.Save(path,ImageFormat.Png);}}
    static void PickYear(IntPtr window,string year){
        PostMessage(GetDlgItem(window,1047),0x00f5,IntPtr.Zero,IntPtr.Zero);IntPtr picker=IntPtr.Zero;
        if(!WaitFor(()=>{picker=FindWindow("WorldMapExplorerPicker",null);return picker!=IntPtr.Zero&&IsWindowVisible(picker);},5000))throw new Exception("Year picker did not open");
        SendText(GetDlgItem(picker,10),0x000c,IntPtr.Zero,year);var list=GetDlgItem(picker,11);int count=(int)SendMessage(list,0x018b,IntPtr.Zero,IntPtr.Zero),row=-1;
        for(int i=0;i<count;i++){var text=new StringBuilder(256);ReadText(list,0x0189,(IntPtr)i,text);if(text.ToString()=="公元"+year+"年"){row=i;break;}}
        if(row<0)throw new Exception("Requested year missing from picker: "+year);SendMessage(list,0x0186,(IntPtr)row,IntPtr.Zero);PostMessage(picker,0x0111,(IntPtr)1,IntPtr.Zero);
        if(!WaitFor(()=>!IsWindowVisible(picker),5000))throw new Exception("Year picker did not close");Thread.Sleep(300);
    }
    static int Main(string[] args){
        if(args.Length!=2)return 2;string output=Path.GetFullPath(args[1]);Directory.CreateDirectory(output);
        var start=new ProcessStartInfo(Path.GetFullPath(args[0])){WorkingDirectory=Path.GetDirectoryName(Path.GetFullPath(args[0])),UseShellExecute=false};start.EnvironmentVariables["LOCALAPPDATA"]=Path.Combine(output,"profile");Process process=Process.Start(start);IntPtr window=IntPtr.Zero;
        try{
            if(!WaitFor(()=>{process.Refresh();window=process.MainWindowHandle;return window!=IntPtr.Zero&&IsWindowVisible(window)&&GetDlgItem(window,1015)!=IntPtr.Zero;},30000))throw new Exception("Application did not start");Thread.Sleep(1000);
            IntPtr combo=GetDlgItem(window,1010),play=GetDlgItem(window,1015),year=GetDlgItem(window,1047);int count=(int)SendMessage(combo,0x0146,IntPtr.Zero,IntPtr.Zero);Check(count==5424&&Index(combo)==0&&Text(year).Contains("2024"),"fresh startup selects 2024 and exposes 5424 years");
            Click(window,1071);Click(window,1016);Click(window,1016);Click(window,1015);int initial=Index(combo);Check(initial==count-1&&Text(year).Contains("3400")&&Text(play).Contains("暂停"),"starting from 2024 synchronizes oldest index, visible year, and playing state");
            bool moved=WaitFor(()=>{int v=Index(combo);return v>=0&&v<count-1&&v>0;},3000);Check(moved,"default playback advances beyond the oldest year");
            if(Text(play).Contains("暂停")){Click(window,1015);int paused=Index(combo);Thread.Sleep(600);Check(Index(combo)==paused&&Text(play).Contains("继续"),"pause holds the year and exposes resume");Click(window,1015);Check(WaitFor(()=>Index(combo)<paused&&Index(combo)>0,3000),"resume continues from the paused year");Click(window,1015);}else Check(false,"playback remained active for pause and resume");
            PickYear(window,"1600");int middle=Index(combo);Check(middle>0&&middle<count-1&&Text(year).Contains("1600")&&Text(play).Contains("播放"),"choosing a middle year while paused resets playback and synchronizes controls");
            Click(window,1015);Check(Index(combo)==middle&&Text(year).Contains("1600"),"starting from a middle year continues at that year");Check(WaitFor(()=>Index(combo)<middle&&Index(combo)>0,3000),"middle-year playback advances forward");
            if(Text(play).Contains("暂停")){Click(window,1015);int paused=Index(combo);Click(window,1024);Thread.Sleep(150);Check(Index(combo)==paused&&Text(year).Contains("CE")&&Text(play).Contains("Resume"),"language switch retains paused year and playback state");Click(window,1015);Check(WaitFor(()=>Index(combo)<paused&&Index(combo)>0,3000),"English-language resume continues from paused year");}else Check(false,"restart remained active for language switch");
            SendMessage(combo,0x014e,(IntPtr)1,IntPtr.Zero);SendMessage(window,0x0113,(IntPtr)1017,IntPtr.Zero);Thread.Sleep(300);Check(Index(combo)==0&&Text(year).Contains("2024")&&Text(play).Contains("Play"),"final timer step reaches 2024 and ends playback");Screenshot(window,Path.Combine(output,"history-playback-final.png"));
        }catch(Exception e){Console.WriteLine("ERROR: "+e.Message);failures++;}finally{if(window!=IntPtr.Zero)PostMessage(window,0x0010,IntPtr.Zero,IntPtr.Zero);if(!process.WaitForExit(5000))process.Kill();process.Dispose();}
        Console.WriteLine("RESULT failures="+failures);return failures==0?0:1;
    }
}
