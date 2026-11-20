using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

internal static class KeyboardUiChecks {
    [StructLayout(LayoutKind.Sequential)] struct RECT { public int left,top,right,bottom; }
    [StructLayout(LayoutKind.Sequential)] struct POINT { public int x,y; }
    [StructLayout(LayoutKind.Sequential)] struct GUITHREADINFO { public int cbSize,flags; public IntPtr active,focus,capture,menuOwner,moveSize,caret; public RECT caretRect; }
    [DllImport("user32.dll")] static extern IntPtr GetDlgItem(IntPtr h,int id);
    [DllImport("user32.dll")] static extern int GetDlgCtrlID(IntPtr h);
    [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
    [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)] static extern IntPtr SendText(IntPtr h,uint m,IntPtr w,string text);
    [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)] static extern IntPtr ReadText(IntPtr h,uint m,IntPtr w,StringBuilder text);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern IntPtr FindWindow(string name,string title);
    [DllImport("user32.dll")] static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] static extern bool IsWindowEnabled(IntPtr h);
    [DllImport("user32.dll")] static extern bool IsChild(IntPtr parent,IntPtr child);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern IntPtr SetFocus(IntPtr h);
    [DllImport("user32.dll")] static extern bool AttachThreadInput(uint a,uint b,bool attach);
    [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h,out uint process);
    [DllImport("user32.dll")] static extern bool GetGUIThreadInfo(uint thread,ref GUITHREADINFO info);
    [DllImport("user32.dll")] static extern bool GetKeyboardState(byte[] state);
    [DllImport("user32.dll")] static extern bool SetKeyboardState(byte[] state);
    [DllImport("user32.dll")] static extern uint MapVirtualKey(uint code,uint mapType);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h,out RECT rect);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h,out RECT rect);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr h,ref POINT point);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr h,IntPtr after,int x,int y,int width,int height,uint flags);
    [DllImport("user32.dll")] static extern bool PrintWindow(IntPtr h,IntPtr dc,uint flags);
    static uint uiThread,appProcess;static int failures;
    static void Check(bool ok,string label){Console.WriteLine((ok?"PASS: ":"FAIL: ")+label);if(!ok)failures++;}
    static IntPtr FocusHandle(){var info=new GUITHREADINFO{cbSize=Marshal.SizeOf(typeof(GUITHREADINFO))};if(!GetGUIThreadInfo(uiThread,ref info))throw new Exception("Cannot inspect keyboard focus");return info.focus;}
    static int FocusId(){return GetDlgCtrlID(FocusHandle());}
    static void Focus(IntPtr window,IntPtr control){
        uint current=GetCurrentThreadId();if(!AttachThreadInput(current,uiThread,true))throw new Exception("Cannot attach to the app input queue");
        try{SetForegroundWindow(window);SetFocus(control);}finally{AttachThreadInput(current,uiThread,false);}Thread.Sleep(250);
        // Tests post to the application's own input queue. Windows may decline foreground activation;
        // validate that queue's focus instead of depending on unrelated desktop focus.
        if(FocusId()!=GetDlgCtrlID(control))throw new Exception("Cannot focus target control "+GetDlgCtrlID(control)+"; actual focus "+FocusId());
    }
    static void Key(byte key,bool shift=false,bool control=false,bool fast=false){
        uint focusProcess;GetWindowThreadProcessId(FocusHandle(),out focusProcess);if(focusProcess!=appProcess)throw new Exception("Keyboard focus is outside the tested application");
        uint current=GetCurrentThreadId();if(!AttachThreadInput(current,uiThread,true))throw new Exception("Cannot attach input queue");
        var old=new byte[256];try{
            if(!GetKeyboardState(old))throw new Exception("Cannot read keyboard state");var state=(byte[])old.Clone();if(shift)state[0x10]|=0x80;if(control)state[0x11]|=0x80;
            if(!SetKeyboardState(state))throw new Exception("Cannot set keyboard modifiers");IntPtr target=FocusHandle();if(target==IntPtr.Zero)throw new Exception("No focused app control");
            uint scan=MapVirtualKey(key,0);int down=(int)(1|(scan<<16));if(!PostMessage(target,0x0100,(IntPtr)key,(IntPtr)down))throw new Exception("Cannot post keydown");
            Thread.Sleep(fast?100:250);PostMessage(target,0x0101,(IntPtr)key,(IntPtr)unchecked((int)(0xc0000001u|(scan<<16))));Thread.Sleep(fast?40:100);
        }finally{SetKeyboardState(old);AttachThreadInput(current,uiThread,false);}Thread.Sleep(fast?100:250);
    }
    static string MapHash(IntPtr window,string path){
        RECT outer,client;POINT origin=new POINT();if(!GetWindowRect(window,out outer)||!GetClientRect(window,out client)||!ClientToScreen(window,ref origin))throw new Exception("Cannot locate map");
        int side=client.right<1200?300:360,toolbar=client.right<1200?150:116,aw=client.right-side-36-20,ah=client.bottom-108-toolbar,mw=aw,mh=mw/2;if(mh>ah){mh=ah;mw=mh*2;}
        var crop=new Rectangle(origin.x-outer.left+24,origin.y-outer.top+90+toolbar+(ah-mh)/2,mw,mh);
        using(var image=new Bitmap(outer.right-outer.left,outer.bottom-outer.top))using(var graphics=Graphics.FromImage(image)){
            IntPtr dc=graphics.GetHdc();bool ok;try{ok=PrintWindow(window,dc,2);}finally{graphics.ReleaseHdc(dc);}if(!ok)throw new Exception("PrintWindow failed");
            using(var map=image.Clone(crop,PixelFormat.Format32bppArgb))using(var bytes=new MemoryStream())using(var sha=SHA256.Create()){map.Save(path,ImageFormat.Png);map.Save(bytes,ImageFormat.Png);return BitConverter.ToString(sha.ComputeHash(bytes.ToArray()));}
        }
    }
    static string ControlHash(IntPtr window,IntPtr control,string path){
        RECT outer,button;if(!GetWindowRect(window,out outer)||!GetWindowRect(control,out button))throw new Exception("Cannot locate control");
        var crop=new Rectangle(button.left-outer.left,button.top-outer.top,button.right-button.left,button.bottom-button.top);
        using(var image=new Bitmap(outer.right-outer.left,outer.bottom-outer.top))using(var graphics=Graphics.FromImage(image)){
            IntPtr dc=graphics.GetHdc();bool ok;try{ok=PrintWindow(window,dc,2);}finally{graphics.ReleaseHdc(dc);}if(!ok)throw new Exception("PrintWindow failed");
            using(var region=image.Clone(crop,PixelFormat.Format32bppArgb))using(var bytes=new MemoryStream())using(var sha=SHA256.Create()){region.Save(path,ImageFormat.Png);region.Save(bytes,ImageFormat.Png);return BitConverter.ToString(sha.ComputeHash(bytes.ToArray()));}
        }
    }
    static IntPtr WaitPicker(){for(int i=0;i<50;i++){IntPtr p=FindWindow("WorldMapExplorerPicker",null);if(p!=IntPtr.Zero&&IsWindowVisible(p))return p;Thread.Sleep(100);}return IntPtr.Zero;}
    static IntPtr WaitWindow(string className,string title){for(int i=0;i<50;i++){IntPtr p=FindWindow(className,title);if(p!=IntPtr.Zero&&IsWindowVisible(p))return p;Thread.Sleep(100);}return IntPtr.Zero;}
    static int Main(string[] args){
        if(args.Length!=2)return 2;Directory.CreateDirectory(args[1]);
        var start=new ProcessStartInfo(args[0]){WorkingDirectory=Path.GetDirectoryName(args[0]),UseShellExecute=false};start.EnvironmentVariables["LOCALAPPDATA"]=Path.Combine(args[1],"profile");var process=Process.Start(start);IntPtr window=IntPtr.Zero;
        try{
            for(int i=0;i<120;i++){process.Refresh();window=process.MainWindowHandle;if(window!=IntPtr.Zero&&IsWindowVisible(window)&&GetDlgItem(window,1065)!=IntPtr.Zero)break;Thread.Sleep(250);}if(window==IntPtr.Zero)throw new Exception("Application did not open");Thread.Sleep(3000);uiThread=GetWindowThreadProcessId(window,out appProcess);
            IntPtr edit=GetDlgItem(window,1002),search=GetDlgItem(window,1003),grid=GetDlgItem(window,1008),routePanel=GetDlgItem(window,1046),routePicker=GetDlgItem(window,1048),export=GetDlgItem(window,1049);
            Focus(window,edit);SendText(edit,0x000c,IntPtr.Zero,"");Key(0x41);var typed=new StringBuilder(32);ReadText(edit,0x000d,(IntPtr)typed.Capacity,typed);if(!typed.ToString().Equals("a",StringComparison.OrdinalIgnoreCase))throw new Exception("Queued keyboard input did not reach the focused search field");Console.WriteLine("PASS: queued keyboard input reaches focused search field");
            Focus(window,edit);Key(0x09);Check(FocusId()==1003,"Tab moves from search field to search button");
            string focused=ControlHash(window,search,Path.Combine(args[1],"focused-button.png"));Focus(window,edit);string unfocused=ControlHash(window,search,Path.Combine(args[1],"unfocused-button.png"));Check(focused!=unfocused,"keyboard focus is visible on the button");
            Focus(window,search);Key(0x09,shift:true);Check(FocusId()==1002,"Shift+Tab moves backwards");
            Focus(window,GetDlgItem(window,1053));Key(0x09);Check(FocusId()==1008,"Tab wraps from last control to first");
            SendText(edit,0x000c,IntPtr.Zero,"example");Focus(window,grid);Key(0x46,control:true);Check(FocusId()==1002,"Ctrl+F focuses search");Check((long)SendMessage(edit,0x00b0,IntPtr.Zero,IntPtr.Zero)==(7L<<16),"Ctrl+F selects existing search text");
            Focus(window,edit);SendText(edit,0x000c,IntPtr.Zero,"39.9,116.4");string before=MapHash(window,Path.Combine(args[1],"before-enter.png"));Key(0x0d);Thread.Sleep(700);Check(MapHash(window,Path.Combine(args[1],"after-enter.png"))!=before,"Enter in search locates the typed coordinates");
            Focus(window,grid);Key(0x30,control:true);string global=MapHash(window,Path.Combine(args[1],"shortcut-global.png"));
            Key(0xbb,control:true);Check(MapHash(window,Path.Combine(args[1],"shortcut-zoom.png"))!=global,"Ctrl+plus zooms the map");
            Key(0xbd,control:true);Check(MapHash(window,Path.Combine(args[1],"shortcut-zoom-out.png"))==global,"Ctrl+minus reverses the zoom step");
            Key(0xbb,control:true);Key(0x30,control:true);Check(MapHash(window,Path.Combine(args[1],"shortcut-reset.png"))==global,"Ctrl+0 restores the global view");
            Key(0x53,shift:true,control:true);Check(MapHash(window,Path.Combine(args[1],"shortcut-scale-off.png"))!=global,"Ctrl+Shift+S hides the scale bar");
            Key(0x53,shift:true,control:true);Check(MapHash(window,Path.Combine(args[1],"shortcut-scale-on.png"))==global,"Ctrl+Shift+S restores the scale bar");
            Key(0x70);IntPtr help=WaitWindow("#32770","地图使用提示");Check(help!=IntPtr.Zero,"F1 opens operation help");if(help!=IntPtr.Zero){Key(0x1b);Check(!IsWindowVisible(help),"Escape closes operation help");}
            Focus(window,routePanel);Key(0x0d);bool opened=IsWindowVisible(routePicker);Check(opened,"Enter opens route tools");if(!opened){SendMessage(routePanel,0x00f5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(300);}
            Focus(window,routePanel);Key(0x09);Check(FocusId()==1048,"Tab reaches visible route picker");
            Focus(window,routePicker);Key(0x09);Check(FocusId()==1018,"Tab skips disabled route controls");
            Focus(window,routePanel);SendMessage(routePanel,0x00f5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(300);Focus(window,routePanel);Key(0x09);Check(FocusId()==1018,"Tab skips collapsed route controls");
            Focus(window,grid);int level=(int)SendMessage(grid,0x0147,IntPtr.Zero,IntPtr.Zero);Key(0x28);Check((int)SendMessage(grid,0x0147,IntPtr.Zero,IntPtr.Zero)!=level,"arrow keys still change grid selection");
            Focus(window,grid);Key(0x73);bool dropped=SendMessage(grid,0x0157,IntPtr.Zero,IntPtr.Zero)!=IntPtr.Zero;Check(dropped,"F4 opens the grid drop-down");if(dropped){Key(0x09);Check(FocusId()==1002&&SendMessage(grid,0x0157,IntPtr.Zero,IntPtr.Zero)==IntPtr.Zero,"Tab closes drop-down and advances focus");}
            IntPtr lines=GetDlgItem(window,1014);int checkedBefore=(int)SendMessage(lines,0x00f0,IntPtr.Zero,IntPtr.Zero);Focus(window,lines);Key(0x20);Check((int)SendMessage(lines,0x00f0,IntPtr.Zero,IntPtr.Zero)!=checkedBefore,"Space toggles a focused layer button");Key(0x0d);Check((int)SendMessage(lines,0x00f0,IntPtr.Zero,IntPtr.Zero)==checkedBefore,"Enter toggles the focused layer button back");
            IntPtr opacity=GetDlgItem(window,1012);Focus(window,opacity);int opacityBefore=(int)SendMessage(opacity,0x0400,IntPtr.Zero,IntPtr.Zero);Key(0x25);Check((int)SendMessage(opacity,0x0400,IntPtr.Zero,IntPtr.Zero)!=opacityBefore,"arrow keys still adjust opacity");Key(0x27);
            Focus(window,grid);Key(0x7a);Check(!IsWindowVisible(edit),"F11 enters fullscreen from a focused control");Key(0x1b);Check(IsWindowVisible(edit),"Escape exits fullscreen");
            Focus(window,GetDlgItem(window,1047));Key(0x0d);IntPtr picker=WaitPicker();Check(picker!=IntPtr.Zero,"Enter opens year picker");if(picker!=IntPtr.Zero){Key(0x46,control:true);Check(FocusId()==10,"Ctrl+F does not escape the modal picker");Key(0x09);Check(FocusId()==11,"Tab reaches year-picker list");Key(0x1b);Thread.Sleep(300);Check(!IsWindowVisible(picker),"Escape closes year picker");Check(FocusId()==1047,"closing year picker restores prior focus");}
            Focus(window,routePanel);Key(0x0d);Focus(window,routePicker);Key(0x0d);picker=WaitPicker();Check(picker!=IntPtr.Zero,"Enter opens route picker");
            if(picker!=IntPtr.Zero){Key(0x0d);Thread.Sleep(500);Check(!IsWindowVisible(picker),"selecting a route closes picker and fits the route");Check(FindWindow("#32770","定位路线")==IntPtr.Zero,"route selection needs no confirmation");Check(FocusId()==1048,"route selection restores route control focus");}
            if(IsWindowEnabled(GetDlgItem(window,1044))){
                Focus(window,GetDlgItem(window,1044));Key(0x0d);IntPtr notes=WaitWindow("WorldMapExplorerRouteNotes",null);Check(notes!=IntPtr.Zero,"Enter opens route notes");
                if(notes!=IntPtr.Zero){Check(FocusId()==1,"route notes initially focus the text");Key(0x09);Check(FocusId()==2,"Tab reaches the route-notes close button");Key(0x0d);Thread.Sleep(300);Check(!IsWindowVisible(notes),"Enter closes route notes");Check(FocusId()==1044,"closing route notes restores prior focus");}
                Focus(window,GetDlgItem(window,1044));Key(0x0d);notes=WaitWindow("WorldMapExplorerRouteNotes",null);if(notes!=IntPtr.Zero){Key(0x1b);Thread.Sleep(300);Check(!IsWindowVisible(notes),"Escape closes route notes");Check(FocusId()==1044,"Escape restores route-notes focus");}else Check(false,"route notes reopen");
            }else Check(false,"route selection enables notes");
            IntPtr language=GetDlgItem(window,1024);var languageBefore=new StringBuilder(64);ReadText(language,0x000d,(IntPtr)languageBefore.Capacity,languageBefore);Focus(window,language);Key(0x0d);var languageAfter=new StringBuilder(64);ReadText(language,0x000d,(IntPtr)languageAfter.Capacity,languageAfter);Check(languageBefore.ToString()!=languageAfter.ToString()&&FocusId()==1024,"Enter switches language and keeps keyboard focus");Key(0x0d);
            Focus(window,export);Key(0x0d);IntPtr save=WaitWindow("#32770","导出地图图片");Check(save!=IntPtr.Zero,"Enter opens the system save dialog");if(save!=IntPtr.Zero){Key(0x09);Check(IsChild(save,FocusHandle()),"Tab remains inside the system save dialog");Key(0x1b);Thread.Sleep(300);Check(!IsWindowVisible(save)&&IsWindowEnabled(window),"Escape cancels the system save dialog");}
            int[] order={1008,1002,1003,1004,1001,1023,1005,1006,1007,1009,1024,1025,1047,1015,1016,1020,1011,1012,1014,1046,1048,1040,1043,1044,1045,1018,1019,1061,1062,1063,1064,1066,1049,1051,1052,1050,1053};
            Focus(window,grid);bool forward=true;for(int i=1;i<=order.Length;i++){Key(0x09,fast:true);if(FocusId()!=order[i%order.Length]){Console.WriteLine("Forward order mismatch at "+i+": "+FocusId());forward=false;break;}}Check(forward,"Tab reaches every enabled visible control in order and wraps");
            Focus(window,grid);bool backward=true;for(int i=1;i<=order.Length;i++){Key(0x09,shift:true,fast:true);if(FocusId()!=order[(order.Length-i)%order.Length]){Console.WriteLine("Backward order mismatch at "+i+": "+FocusId());backward=false;break;}}Check(backward,"Shift+Tab reaches every enabled visible control in reverse order and wraps");
            RECT outer;if(!GetWindowRect(window,out outer)||!SetWindowPos(window,IntPtr.Zero,0,0,1100,720,0x0016))throw new Exception("Cannot resize to compact layout");Thread.Sleep(1000);
            int[] compact={1008,1002,1003,1004,1001,1023,1005,1006,1007,1009,1047,1015,1016,1020,1011,1012,1014,1024,1025,1046,1048,1040,1043,1044,1045,1018,1019,1061,1062,1063,1064,1066,1049,1051,1052,1050,1053};
            Focus(window,grid);bool compactOrder=true;for(int i=1;i<=compact.Length;i++){Key(0x09,fast:true);if(FocusId()!=compact[i%compact.Length]){Console.WriteLine("Compact order mismatch at "+i+": "+FocusId());compactOrder=false;break;}}Check(compactOrder,"Tab follows the compact toolbar layout");
            SetWindowPos(window,IntPtr.Zero,0,0,outer.right-outer.left,outer.bottom-outer.top,0x0016);Thread.Sleep(500);Focus(window,GetDlgItem(window,1009));Key(0x09);Check(FocusId()==1024,"Tab order adapts after restoring the wide layout");
            if(!SetWindowPos(window,IntPtr.Zero,0,0,640,480,0x0416))throw new Exception("Cannot resize to small-screen layout");Thread.Sleep(800);
            Focus(window,grid);bool smallOrder=true;for(int i=1;i<=compact.Length;i++){Key(0x09,fast:true);if(FocusId()!=compact[i%compact.Length]){Console.WriteLine("Small-screen order mismatch at "+i+": "+FocusId());smallOrder=false;break;}}Check(smallOrder,"Tab follows the small-screen toolbar layout");
            Focus(window,GetDlgItem(window,1047));Key(0x0d);picker=WaitPicker();Check(picker!=IntPtr.Zero,"year picker opens by keyboard on a small screen");if(picker!=IntPtr.Zero){Key(0x1b);Thread.Sleep(300);Check(!IsWindowVisible(picker)&&FocusId()==1047,"small-screen picker closes and restores focus");}
            Console.WriteLine("RESULT failures="+failures);return failures==0?0:1;
        }catch(Exception e){Console.WriteLine("FAIL: "+e.Message);return 1;}
        finally{IntPtr picker=FindWindow("WorldMapExplorerPicker",null);if(picker!=IntPtr.Zero)PostMessage(picker,0x0010,IntPtr.Zero,IntPtr.Zero);if(window!=IntPtr.Zero)PostMessage(window,0x0010,IntPtr.Zero,IntPtr.Zero);if(!process.WaitForExit(5000))process.Kill();}
    }
}
