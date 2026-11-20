using System;using System.Diagnostics;using System.Drawing;using System.Drawing.Imaging;using System.IO;using System.Runtime.InteropServices;using System.Text;using System.Threading;
class DpiUiChecks{
 [StructLayout(LayoutKind.Sequential)]struct R{public int l,t,r,b;}
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)]struct FontSpec{public int height,width,escapement,orientation,weight;public byte italic,underline,strikeout,charset,outprecision,clipprecision,quality,pitch;[MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)]public string face;}
 delegate bool EnumProc(IntPtr w,IntPtr p);
 [DllImport("user32.dll")]static extern bool EnumWindows(EnumProc f,IntPtr p);
 [DllImport("user32.dll")]static extern bool EnumChildWindows(IntPtr w,EnumProc f,IntPtr p);
 [DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr w,out uint p);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern int GetClassName(IntPtr w,StringBuilder b,int n);
 [DllImport("user32.dll")]static extern IntPtr GetDlgItem(IntPtr w,int id);
 [DllImport("user32.dll")]static extern IntPtr SendMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll")]static extern bool PostMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)]static extern IntPtr SetText(IntPtr w,uint m,IntPtr a,string b);
 [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)]static extern IntPtr ReadText(IntPtr w,uint m,IntPtr a,StringBuilder b);
 [DllImport("user32.dll")]static extern bool IsWindowVisible(IntPtr w);
 [DllImport("user32.dll")]static extern bool IsWindow(IntPtr w);
 [DllImport("user32.dll")]static extern bool ShowWindow(IntPtr w,int command);
 [DllImport("user32.dll")]static extern bool GetClientRect(IntPtr w,out R r);
 [DllImport("user32.dll")]static extern bool GetWindowRect(IntPtr w,out R r);
 [DllImport("user32.dll")]static extern int MapWindowPoints(IntPtr a,IntPtr b,ref R r,uint n);
 [DllImport("user32.dll")]static extern uint GetDpiForWindow(IntPtr w);
 [DllImport("user32.dll")]static extern IntPtr GetWindowDpiAwarenessContext(IntPtr w);
 [DllImport("user32.dll")]static extern bool AreDpiAwarenessContextsEqual(IntPtr a,IntPtr b);
 [DllImport("user32.dll")]static extern bool PrintWindow(IntPtr w,IntPtr dc,uint flags);
 [DllImport("gdi32.dll",EntryPoint="GetObjectW",CharSet=CharSet.Unicode)]static extern int GetFontObject(IntPtr f,int n,out FontSpec s);
 static int checks;static Process process;static IntPtr main;static string folder,profile,app;
 static bool Wait(Func<bool> f,int ms=12000){var watch=Stopwatch.StartNew();while(watch.ElapsedMilliseconds<ms){if(f())return true;Thread.Sleep(40);}return f();}
 static void Check(bool ok,string label){if(!ok)throw new Exception(label);checks++;Console.WriteLine("PASS "+label);}
 static string Text(IntPtr w){var b=new StringBuilder(4096);ReadText(w,0xd,(IntPtr)b.Capacity,b);return b.ToString();}
 static IntPtr W(string name){IntPtr result=IntPtr.Zero;if(!Wait(()=>{EnumWindows((w,p)=>{uint id;GetWindowThreadProcessId(w,out id);var s=new StringBuilder(100);GetClassName(w,s,s.Capacity);if(id==(uint)process.Id&&s.ToString()==name&&IsWindowVisible(w)){result=w;return false;}return true;},IntPtr.Zero);return result!=IntPtr.Zero;}))throw new Exception("Missing own window "+name);return result;}
 static IntPtr C(IntPtr w,int id){return GetDlgItem(w,id);}
 static void Click(IntPtr w,int id){SendMessage(C(w,id),0xf5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(180);}
 static void Command(IntPtr w,int id,int notice){SendMessage(w,0x111,(IntPtr)((notice<<16)|id),C(w,id));}
 static void Close(IntPtr w){PostMessage(w,0x10,IntPtr.Zero,IntPtr.Zero);Check(Wait(()=>!IsWindow(w)),"close auxiliary window");}
 static R Bounds(IntPtr parent,IntPtr child){R r;GetWindowRect(child,out r);MapWindowPoints(IntPtr.Zero,parent,ref r,2);return r;}
 static int FontHeight(IntPtr w){FontSpec f;return GetFontObject(SendMessage(w,0x31,IntPtr.Zero,IntPtr.Zero),Marshal.SizeOf(typeof(FontSpec)),out f)>0?-f.height:0;}
 static void InBounds(IntPtr window,int id){R client;GetClientRect(window,out client);R r=Bounds(window,C(window,id));Check(IsWindowVisible(C(window,id))&&r.l>=0&&r.t>=0&&r.r<=client.r+1&&r.b<=client.b+1&&r.r>r.l&&r.b>r.t,"control "+id+" fits its window");}
 static void Shot(IntPtr w,string name){R r;GetWindowRect(w,out r);using(var b=new Bitmap(r.r-r.l,r.b-r.t))using(var g=Graphics.FromImage(b)){IntPtr dc=g.GetHdc();PrintWindow(w,dc,2);g.ReleaseHdc(dc);b.Save(Path.Combine(folder,name+".png"),ImageFormat.Png);}}
 static void Start(){var s=new ProcessStartInfo(app){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(app)};s.EnvironmentVariables["LOCALAPPDATA"]=profile;process=Process.Start(s);main=W("WorldMapExplorer");Check(Wait(()=>C(main,1086)!=IntPtr.Zero&&IsWindowVisible(C(main,1003))),"startup completes");}
 static void Stop(){if(process!=null&&!process.HasExited){PostMessage(main,0x10,IntPtr.Zero,IntPtr.Zero);if(!process.WaitForExit(10000))process.Kill();}process=null;}
 static void Scale(int mode,int dpi){SendMessage(C(main,1086),0x14e,(IntPtr)mode,IntPtr.Zero);Command(main,1086,1);Check(Wait(()=>FontHeight(C(main,1002))==(13*dpi+48)/96),"search font follows scale "+dpi);Thread.Sleep(200);}
 static void ValidateMain(){foreach(int id in new[]{1002,1003,1024,1023,1082,1070,1071,1072,1073,1074,1075,1018,1019,1066,1049,1050,1051,1052,1076,1077})InBounds(main,id);R a=Bounds(main,C(main,1002)),b=Bounds(main,C(main,1003)),language=Bounds(main,C(main,1082));Check(a.r<=b.l&&(b.r<=language.l||b.b<=language.t||language.b<=b.t),"search and header actions have separate space");}
 static void ValidateAuxiliaries(int dpi){
  Click(main,1082);IntPtr search=W("WorldMapExplorerUnifiedSearch");Check(Text(search).Length>0,"search window keeps a readable title");foreach(int id in new[]{2320,2301,2302,2304,2303,2305,2306,2307})InBounds(search,id);Check((int)SendMessage(C(search,2303),0x1a1,IntPtr.Zero,IntPtr.Zero)==(62*dpi+48)/96,"search rows scale with text");Check(FontHeight(C(search,2305))==(14*dpi+48)/96,"search reading font scales");Shot(search,"saved-"+dpi);Close(search);
  Click(main,1066);IntPtr events=W("WorldMapExplorerEventSearch");foreach(int id in new[]{2101,2102,2103,2104,2105,2106,2107,2108,2109,2111})InBounds(events,id);Check(FontHeight(C(events,2106))==(14*dpi+48)/96,"event reading font scales");Shot(events,"events-"+dpi);Close(events);
  Click(main,1071);PostMessage(C(main,1047),0xf5,IntPtr.Zero,IntPtr.Zero);IntPtr picker=W("WorldMapExplorerPicker");InBounds(picker,10);InBounds(picker,11);InBounds(picker,1);InBounds(picker,2);Check(FontHeight(C(picker,10))==(15*dpi+48)/96,"year chooser font scales");Shot(picker,"year-"+dpi);Click(picker,2);Check(Wait(()=>!IsWindow(picker)),"cancel year chooser");
  Click(main,1072);SendMessage(C(main,1041),0x14e,(IntPtr)1,IntPtr.Zero);Command(main,1041,1);Click(main,1044);IntPtr notes=W("WorldMapExplorerRouteNotes");InBounds(notes,1);InBounds(notes,2);Check(FontHeight(C(notes,1))==(14*dpi+48)/96,"notes reading font scales");Shot(notes,"notes-"+dpi);Close(notes);Click(main,1070);
 }
 static void SimulateDpiMessage(){R r;GetWindowRect(main,out r);IntPtr ptr=Marshal.AllocHGlobal(Marshal.SizeOf(typeof(R)));try{Marshal.StructureToPtr(r,ptr,false);int dpi=(int)GetDpiForWindow(main);SendMessage(main,0x2e0,(IntPtr)(dpi|(dpi<<16)),ptr);}finally{Marshal.FreeHGlobal(ptr);}ValidateMain();}
 static int Main(string[] args){app=Path.GetFullPath(args[0]);folder=Path.GetFullPath(args[1]);profile=Path.Combine(folder,"profile");Directory.CreateDirectory(profile);try{
  Start();uint monitorDpi=GetDpiForWindow(main);Console.WriteLine("Actual monitor DPI: "+monitorDpi);Check(AreDpiAwarenessContextsEqual(GetWindowDpiAwarenessContext(main),(IntPtr)(-4)),"application declares Per Monitor V2 awareness");Check(FontHeight(C(main,1002))==(13*(int)monitorDpi+48)/96,"fresh profile follows Windows automatically");ValidateMain();Shot(main,"automatic-zh");
  SetText(C(main,1002),0xc,IntPtr.Zero,"39.9042,116.4074");Click(main,1003);Click(main,1083);Check(Text(C(main,1083))=="★","map remains interactive at system scale");
  ValidateAuxiliaries((int)monitorDpi);Click(main,1075);foreach(int id in new[]{1086,1008,1014,1001,1020,1025,1085,1084,1053})InBounds(main,id);Shot(main,"display-automatic");Click(main,1075);
  Click(main,1082);IntPtr live=W("WorldMapExplorerUnifiedSearch");SetText(C(live,2301),0xc,IntPtr.Zero,"北京");SendMessage(C(live,2302),0x14e,(IntPtr)1,IntPtr.Zero);Command(live,2302,1);Thread.Sleep(200);Scale(1,96);Check(FontHeight(C(live,2305))==14&&Text(C(live,2301))=="北京","an open search rescales and retains the query");InBounds(live,2303);Scale(0,(int)monitorDpi);Check(FontHeight(C(live,2305))==(14*(int)monitorDpi+48)/96,"an open search returns to Windows scaling");Close(live);
  foreach(int mode in new[]{1,2,3,4}){int dpi=new[]{0,96,120,144,192}[mode];Scale(mode,dpi);ValidateMain();ValidateAuxiliaries(dpi);Shot(main,"main-"+dpi);}
  Scale(1,96);ShowWindow(main,3);Thread.Sleep(250);Scale(4,192);ValidateMain();ShowWindow(main,9);Thread.Sleep(250);ValidateMain();Shot(main,"restored-after-maximized-scale");Click(main,1075);foreach(int id in new[]{1086,1008,1014,1001,1020,1025,1085,1084,1053})InBounds(main,id);Shot(main,"display-narrow-192");Click(main,1075);
  SimulateDpiMessage();Click(main,1024);Shot(main,"main-en-192");Click(main,1082);IntPtr saved=W("WorldMapExplorerUnifiedSearch");PostMessage(C(saved,2308),0xf5,IntPtr.Zero,IntPtr.Zero);IntPtr rename=W("WorldMapExplorerRenameSaved");foreach(int id in new[]{7100,7101,7104,7102,7103})InBounds(rename,id);Check(FontHeight(C(rename,7101))==28,"rename input scales at 200 percent");SetText(C(rename,7101),0xc,IntPtr.Zero,"");Click(rename,7102);Check(IsWindow(rename)&&Text(C(rename,7104)).Contains("1–120"),"empty name remains editable with an explanation");Shot(rename,"rename-en-192");Click(rename,7103);Close(saved);Stop();string settings=Path.Combine(profile,"WorldMapExplorer","settings.ini");Check(File.ReadAllText(settings).Contains("UiScale=4"),"interface size is saved atomically");Start();Check(FontHeight(C(main,1002))==26&&Text(C(main,1024))=="中文","language and interface size survive relaunch");Scale(0,(int)GetDpiForWindow(main));Check((int)SendMessage(C(main,1086),0x147,IntPtr.Zero,IntPtr.Zero)==0,"Windows scaling can be restored");Console.WriteLine(checks+" DPI UI checks passed; one physical monitor, no multi-monitor hardware claim.");
  return 0;
 }catch(Exception e){Console.WriteLine("FAIL "+e);return 1;}finally{Stop();}}
}




