using System;using System.Text;using System.Diagnostics;using System.Drawing;using System.Drawing.Imaging;using System.IO;using System.Runtime.InteropServices;using System.Threading;using System.Security.Cryptography;
internal static class MapMemoryProbe {
 [StructLayout(LayoutKind.Sequential)]struct R{public int l,t,r,b;}
 [StructLayout(LayoutKind.Sequential)]struct P{public int x,y;}
 [StructLayout(LayoutKind.Sequential)]struct GUI{public int size,flags;public IntPtr active,focus,capture,menu,move,caret;public R caretRect;}
 [DllImport("user32.dll")]static extern IntPtr GetDlgItem(IntPtr w,int id);
 [DllImport("user32.dll")]static extern IntPtr SendMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)]static extern IntPtr SendText(IntPtr w,uint m,IntPtr a,string b);
 [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)]static extern IntPtr ReadText(IntPtr w,uint m,IntPtr a,StringBuilder b);
 [DllImport("user32.dll")]static extern bool PostMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern IntPtr FindWindow(string c,string t);
 [DllImport("user32.dll")]static extern bool IsWindowVisible(IntPtr w);
 [DllImport("user32.dll")]static extern bool IsWindowEnabled(IntPtr w);
 [DllImport("user32.dll")]static extern bool GetWindowRect(IntPtr w,out R r);
 [DllImport("user32.dll")]static extern bool GetClientRect(IntPtr w,out R r);
 [DllImport("user32.dll")]static extern int MapWindowPoints(IntPtr a,IntPtr b,ref R r,uint count);
 [DllImport("user32.dll")]static extern bool ClientToScreen(IntPtr w,ref P p);
 [DllImport("user32.dll")]static extern bool PrintWindow(IntPtr w,IntPtr dc,uint f);
 [DllImport("user32.dll")]static extern bool SetWindowPos(IntPtr w,IntPtr z,int x,int y,int width,int height,uint f);
 [DllImport("user32.dll")]static extern bool AttachThreadInput(uint a,uint b,bool attach);
 [DllImport("kernel32.dll")]static extern uint GetCurrentThreadId();
 [DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr w,out uint p);
 [DllImport("user32.dll")]static extern bool SetForegroundWindow(IntPtr w);
 [DllImport("user32.dll")]static extern IntPtr SetFocus(IntPtr w);
 [DllImport("user32.dll")]static extern bool GetGUIThreadInfo(uint t,ref GUI g);
 [DllImport("user32.dll")]static extern bool GetKeyboardState(byte[] s);
 [DllImport("user32.dll")]static extern bool SetKeyboardState(byte[] s);
 [DllImport("user32.dll")]static extern uint MapVirtualKey(uint c,uint t);
 static int checks,failures;static uint thread;static IntPtr main;static string folder,app,profile;static Process process;
 static void Check(bool ok,string text){checks++;Console.WriteLine((ok?"PASS: ":"FAIL: ")+text);if(!ok)failures++;}
 static IntPtr C(IntPtr w,int id){return GetDlgItem(w,id);}
 static string Text(IntPtr w){var b=new StringBuilder(24000);ReadText(w,0xd,(IntPtr)b.Capacity,b);return b.ToString();}
 static bool Visible(int id){return IsWindowVisible(C(main,id));}
 static void Click(IntPtr w,int id){if(!IsWindowVisible(C(w,id)))throw new Exception("Test attempted hidden control "+id);SendMessage(C(w,id),0xf5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(220);}
 static void Set(IntPtr w,int id,string value){SendText(C(w,id),0xc,IntPtr.Zero,value);Thread.Sleep(240);}
 static IntPtr Focused(){var g=new GUI{size=Marshal.SizeOf(typeof(GUI))};GetGUIThreadInfo(thread,ref g);return g.focus;}
 static void Focus(IntPtr w,IntPtr c){uint t=GetCurrentThreadId();if(!AttachThreadInput(t,thread,true))throw new Exception("Cannot attach focus");try{SetForegroundWindow(w);SetFocus(c);}finally{AttachThreadInput(t,thread,false);}Thread.Sleep(120);if(Focused()!=c)throw new Exception("Cannot focus control");}
 static void Key(byte key,bool ctrl=false,bool alt=false){uint t=GetCurrentThreadId();if(!AttachThreadInput(t,thread,true))throw new Exception("Cannot attach keyboard");byte[] old=new byte[256];try{GetKeyboardState(old);byte[] state=(byte[])old.Clone();if(ctrl)state[0x11]|=0x80;if(alt)state[0x12]|=0x80;SetKeyboardState(state);uint scan=MapVirtualKey(key,0);PostMessage(Focused(),alt?0x104u:0x100u,(IntPtr)key,(IntPtr)(1|(scan<<16)));Thread.Sleep(160);PostMessage(Focused(),alt?0x105u:0x101u,(IntPtr)key,(IntPtr)unchecked((int)(0xc0000001u|(scan<<16))));Thread.Sleep(100);}finally{SetKeyboardState(old);AttachThreadInput(t,thread,false);}Thread.Sleep(180);}
 static bool Wait(Func<bool> predicate,int ms=6000){var sw=Stopwatch.StartNew();while(sw.ElapsedMilliseconds<ms){if(predicate())return true;Thread.Sleep(50);}return predicate();}
 static IntPtr Window(string cls){IntPtr w=IntPtr.Zero;if(!Wait(()=>{w=FindWindow(cls,null);return w!=IntPtr.Zero&&IsWindowVisible(w);}))throw new Exception("Window missing: "+cls);return w;}
 static void Shot(IntPtr w,string name){R r;GetWindowRect(w,out r);using(var b=new Bitmap(r.r-r.l,r.b-r.t))using(var g=Graphics.FromImage(b)){var dc=g.GetHdc();bool ok=PrintWindow(w,dc,2);g.ReleaseHdc(dc);if(!ok)throw new Exception("Cannot capture window");b.Save(Path.Combine(folder,name+".png"),ImageFormat.Png);}}
 static string MapHash(){R r,button;P p=new P();GetWindowRect(main,out r);GetWindowRect(C(main,1076),out button);ClientToScreen(main,ref p);int left=button.l-p.x-10,top=button.t-p.y-10;R right;GetWindowRect(C(main,1051),out right);int width=right.l-p.x+118-left; // 1051 is zoom-out, the leftmost navigation button.
  if(width<80)throw new Exception("Cannot locate map");int h=width/2;using(var b=new Bitmap(r.r-r.l,r.b-r.t))using(var g=Graphics.FromImage(b)){var dc=g.GetHdc();PrintWindow(main,dc,2);g.ReleaseHdc(dc);using(var crop=b.Clone(new Rectangle(p.x-r.l+left+10,p.y-r.t+top+48,width-20,Math.Max(1,h-130)),PixelFormat.Format32bppArgb))using(var bytes=new MemoryStream())using(var sha=SHA256.Create()){crop.Save(bytes,ImageFormat.Png);return Convert.ToBase64String(sha.ComputeHash(bytes.ToArray()));}}
 }
 static IntPtr Search(string query){Set(main,1002,query);Click(main,1003);return Window("WorldMapExplorerUnifiedSearch");}
 static int Count(IntPtr w){return (int)SendMessage(C(w,2303),0x18b,IntPtr.Zero,IntPtr.Zero);}
 static void Select(IntPtr w,int index){SendMessage(C(w,2303),0x186,(IntPtr)index,IntPtr.Zero);SendMessage(w,0x111,(IntPtr)((1<<16)|2303),C(w,2303));Thread.Sleep(80);}
 static void Category(IntPtr w,int index){SendMessage(C(w,2302),0x14e,(IntPtr)index,IntPtr.Zero);SendMessage(w,0x111,(IntPtr)((1<<16)|2302),C(w,2302));Thread.Sleep(250);}
 static void Close(IntPtr w){SendMessage(w,0x10,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(150);}
 static void PickYear(string year){PostMessage(C(main,1047),0xf5,IntPtr.Zero,IntPtr.Zero);var w=Window("WorldMapExplorerPicker");Set(w,10,year);IntPtr list=C(w,11);int n=(int)SendMessage(list,0x18b,IntPtr.Zero,IntPtr.Zero),row=-1;for(int i=0;i<n;i++){var b=new StringBuilder(256);ReadText(list,0x189,(IntPtr)i,b);if(b.ToString()=="公元"+year+"年"){row=i;break;}}if(row<0)throw new Exception("Year not found");SendMessage(list,0x186,(IntPtr)row,IntPtr.Zero);PostMessage(w,0x111,(IntPtr)1,IntPtr.Zero);Wait(()=>!IsWindowVisible(w));Thread.Sleep(180);}
 static void Bounds(string label){R client;GetClientRect(main,out client);int[] ids={1002,1003,1024,1023,1018,1019,1070,1071,1072,1073,1074,1075,1082,1078,1047,1079,1015,1016,1080,1048,1040,1043,1044,1045,1081,1051,1050,1052,1076,1077,1061,1062,1063,1064,1065,1066,1083,1140,1141,1142,1143,1144,1145,1049};foreach(int id in ids){var c=C(main,id);if(c==IntPtr.Zero||!IsWindowVisible(c))continue;R r;GetWindowRect(c,out r);MapWindowPoints(IntPtr.Zero,main,ref r,2);Check(r.l>=0&&r.t>=0&&r.r<=client.r&&r.b<=client.b&&r.r>r.l&&r.b>r.t,label+" control "+id+" stays inside window");}}
 static void Start(){var s=new ProcessStartInfo(app){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(app)};s.EnvironmentVariables["LOCALAPPDATA"]=profile;process=Process.Start(s);main=IntPtr.Zero;if(!Wait(()=>{process.Refresh();main=process.MainWindowHandle;return main!=IntPtr.Zero&&C(main,1065)!=IntPtr.Zero&&!String.IsNullOrEmpty(Text(C(main,1070)))&&!IsWindowVisible(C(main,1008));},30000))throw new Exception("App did not finish initial layout");Thread.Sleep(250);uint p;thread=GetWindowThreadProcessId(main,out p);}
 static void Stop(){if(process==null)return;if(!process.HasExited){PostMessage(main,0x10,IntPtr.Zero,IntPtr.Zero);if(!process.WaitForExit(8000))process.Kill();}process.Dispose();process=null;}
 static bool SavedCoordinate(double latitude,double longitude){var files=Directory.GetFiles(profile,"saved-places-v1.bin",SearchOption.AllDirectories);if(files.Length!=1)return false;using(var f=new BinaryReader(File.OpenRead(files[0]),Encoding.Unicode)){if(Encoding.ASCII.GetString(f.ReadBytes(8))!="WMPLACE1")return false;uint n=f.ReadUInt32();for(int i=0;i<n;i++){uint length=f.ReadUInt32();f.ReadBytes((int)length*2);double lat=f.ReadDouble(),lon=f.ReadDouble();f.ReadDouble();if(Math.Abs(lat-latitude)<1e-7&&Math.Abs(lon-longitude)<1e-7)return true;}}return false;}
 static int Main(string[] args){app=Path.GetFullPath(args[0]);folder=Path.GetFullPath(args[1]);profile=Path.Combine(folder,"profile");Directory.CreateDirectory(profile);try{Start();process.Refresh();long initial=process.WorkingSet64;var w=Search("北京");Click(w,2306);SendMessage(main,0x111,(IntPtr)1122,IntPtr.Zero);Thread.Sleep(1800);Shot(main,"nasa-beijing");process.Refresh();File.WriteAllText(Path.Combine(folder,"memory.json"),"{\n  \"initialWorkingSet\": "+initial+",\n  \"workingSet\": "+process.WorkingSet64+",\n  \"peakWorkingSet\": "+process.PeakWorkingSet64+"\n}");Console.WriteLine("NASA WorkingSet="+process.WorkingSet64+" Peak="+process.PeakWorkingSet64);return 0;}catch(Exception e){Console.WriteLine("ERROR: "+e);return 1;}finally{Stop();}}}