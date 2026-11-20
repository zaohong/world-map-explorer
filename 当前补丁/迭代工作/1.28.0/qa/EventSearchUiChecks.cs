using System;using System.Diagnostics;using System.Drawing;using System.Drawing.Imaging;using System.IO;using System.Runtime.InteropServices;using System.Text;using System.Threading;
class EventSearchUiChecks {
 struct R{public int l,t,r,b;}
 [DllImport("user32.dll")]static extern IntPtr GetDlgItem(IntPtr w,int id);
 [DllImport("user32.dll")]static extern IntPtr SendMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern IntPtr SendMessage(IntPtr w,uint m,IntPtr a,string b);
 [DllImport("user32.dll")]static extern bool PostMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern IntPtr FindWindow(string cls,string title);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern int GetWindowText(IntPtr w,StringBuilder s,int n);
 [DllImport("user32.dll")]static extern bool GetWindowRect(IntPtr w,out R r);
 [DllImport("user32.dll")]static extern bool IsWindowEnabled(IntPtr w);
 [DllImport("user32.dll")]static extern bool PrintWindow(IntPtr w,IntPtr dc,uint f);
 [DllImport("user32.dll")]static extern bool SetWindowPos(IntPtr w,IntPtr z,int x,int y,int width,int height,uint flags);
 static int checks=0;static void Check(bool ok,string label){if(!ok)throw new Exception(label);checks++;Console.WriteLine("PASS "+label);}
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern IntPtr SendMessage(IntPtr w,uint m,IntPtr a,StringBuilder b);
 static string Text(IntPtr w){var b=new StringBuilder(14000);SendMessage(w,0xd,new IntPtr(b.Capacity),b);return b.ToString();}
 static void Input(IntPtr w,int id,string value){SendMessage(GetDlgItem(w,id),0xc,IntPtr.Zero,value);Thread.Sleep(80);}
 static void Click(IntPtr w,int id){SendMessage(GetDlgItem(w,id),0xf5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(200);}
 static int Rows(IntPtr w){return SendMessage(GetDlgItem(w,2105),0x1004,IntPtr.Zero,IntPtr.Zero).ToInt32();}
 static void Shot(IntPtr w,string path){R r;GetWindowRect(w,out r);using(var b=new Bitmap(r.r-r.l,r.b-r.t))using(var g=Graphics.FromImage(b)){var dc=g.GetHdc();PrintWindow(w,dc,2);g.ReleaseHdc(dc);b.Save(path,ImageFormat.Png);}}
 static int Main(string[] a){Directory.CreateDirectory(a[1]);string profile=Path.Combine(a[1],"profile");Directory.CreateDirectory(profile);var start=new ProcessStartInfo(a[0]){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(a[0])};start.EnvironmentVariables["LOCALAPPDATA"]=profile;var p=Process.Start(start);IntPtr w=IntPtr.Zero,search=IntPtr.Zero;try{
  for(int i=0;i<200;i++){p.Refresh();w=p.MainWindowHandle;if(w!=IntPtr.Zero&&GetDlgItem(w,1066)!=IntPtr.Zero)break;Thread.Sleep(100);}Check(w!=IntPtr.Zero,"application opens");Thread.Sleep(500);Click(w,1066);search=FindWindow("WorldMapExplorerEventSearch",null);Check(search!=IntPtr.Zero,"independent event search opens");Check(Rows(search)==346,"all 346 curated events are available");Shot(search,Path.Combine(a[1],"events-all-zh.png"));
  Input(search,2101,"蒙古 巴格达");Check(Rows(search)==1,"Chinese event/place search");Check(Text(GetDlgItem(search,2106)).Contains("1258"),"selected event detail has the correct year");Check(IsWindowEnabled(GetDlgItem(search,2108)),"mapped event offers location");Click(search,2107);int current=SendMessage(GetDlgItem(w,1010),0x147,IntPtr.Zero,IntPtr.Zero).ToInt32();Check(current==2024-1258,"event navigation selects 1258");Click(search,2108);Check(!p.HasExited,"map location action remains responsive");Shot(search,Path.Combine(a[1],"events-baghdad-zh.png"));
  Click(search,2110);Input(search,2101,"Beagle");Check(Rows(search)==2,"English search includes voyage and Darwin excursion");Input(search,2101,"Beagle voyage");Check(Rows(search)==1,"multiple English terms narrow the results");Check(Text(GetDlgItem(search,2106)).Contains("1831"),"Beagle detail uses voyage period");
  Click(search,2110);Input(search,2103,"-600");Input(search,2104,"-400");Check(Rows(search)>2&&Rows(search)<20,"BCE year range filters results");Input(search,2103,"0");Check(Rows(search)==0,"year zero rejected");Check(Text(GetDlgItem(search,2112)).Contains("公元0年"),"invalid year guidance shown");Input(search,2103,"500");Input(search,2104,"400");Check(Rows(search)==0,"reversed range rejected");
  Click(search,2110);SendMessage(GetDlgItem(search,2102),0x14e,new IntPtr(1),IntPtr.Zero);SendMessage(search,0x111,new IntPtr((1<<16)|2102),GetDlgItem(search,2102));Check(Rows(search)>0&&Rows(search)<346,"category selection filters the catalog");
  Click(search,2110);Input(search,2101,"天问一号");Check(Rows(search)==1,"space mission searchable");Check(!IsWindowEnabled(GetDlgItem(search,2108)),"non-Earth or unlocated event cannot be plotted on Earth");
  Click(search,2110);Click(w,1024);Check(Text(search).Contains("Explore"),"open search window changes language");Input(search,2101,"Darwin");Check(Rows(search)>0,"bilingual index retained after language change");Shot(search,Path.Combine(a[1],"events-en.png"));SetWindowPos(search,IntPtr.Zero,10,10,760,560,4);Thread.Sleep(150);Shot(search,Path.Combine(a[1],"events-small-en.png"));Click(search,2111);Check(FindWindow("WorldMapExplorerEventSearch",null)==IntPtr.Zero,"search window closes independently");Click(w,1066);search=FindWindow("WorldMapExplorerEventSearch",null);Check(search!=IntPtr.Zero&&Rows(search)==346,"search reopens with clean results");Console.WriteLine(checks+" checks passed");return 0;
 }catch(Exception e){Console.WriteLine("FAIL: "+e);return 1;}finally{if(search!=IntPtr.Zero)PostMessage(search,0x10,IntPtr.Zero,IntPtr.Zero);if(!p.HasExited){PostMessage(w,0x10,IntPtr.Zero,IntPtr.Zero);if(!p.WaitForExit(8000))p.Kill();}}}
}
