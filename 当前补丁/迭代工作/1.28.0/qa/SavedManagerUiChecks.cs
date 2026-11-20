using System;using System.Diagnostics;using System.Drawing;using System.Drawing.Imaging;using System.IO;using System.Runtime.InteropServices;using System.Text;using System.Threading;
class SavedManagerUiChecks{
 delegate bool EnumProc(IntPtr w,IntPtr p);struct R{public int l,t,r,b;}
 [DllImport("user32.dll")]static extern bool EnumWindows(EnumProc f,IntPtr p);
 [DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr w,out uint p);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern int GetClassName(IntPtr w,StringBuilder b,int n);
 [DllImport("user32.dll")]static extern IntPtr GetDlgItem(IntPtr w,int id);
 [DllImport("user32.dll")]static extern IntPtr SendMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)]static extern IntPtr SetText(IntPtr w,uint m,IntPtr a,string b);
 [DllImport("user32.dll",EntryPoint="SendMessageW",CharSet=CharSet.Unicode)]static extern IntPtr ReadText(IntPtr w,uint m,IntPtr a,StringBuilder b);
 [DllImport("user32.dll")]static extern bool PostMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 [DllImport("user32.dll")]static extern bool IsWindowVisible(IntPtr w);
 [DllImport("user32.dll")]static extern bool GetWindowRect(IntPtr w,out R r);
 [DllImport("user32.dll")]static extern bool PrintWindow(IntPtr w,IntPtr dc,uint flags);
 static int checks;static void Check(bool ok,string label){if(!ok)throw new Exception(label);checks++;Console.WriteLine("PASS "+label);}
 static bool Wait(Func<bool> f,int ms=12000){var timer=Stopwatch.StartNew();while(timer.ElapsedMilliseconds<ms){if(f())return true;Thread.Sleep(40);}return f();}
 static IntPtr W(Process process,string name){IntPtr found=IntPtr.Zero;Wait(()=>{EnumWindows((w,p)=>{uint id;GetWindowThreadProcessId(w,out id);if(id!=(uint)process.Id)return true;var c=new StringBuilder(128);GetClassName(w,c,c.Capacity);if(c.ToString()==name&&IsWindowVisible(w)){found=w;return false;}return true;},IntPtr.Zero);return found!=IntPtr.Zero;});if(found==IntPtr.Zero)throw new Exception("Missing own window "+name);return found;}
 static string Text(IntPtr w){var b=new StringBuilder(16000);ReadText(w,0xd,(IntPtr)b.Capacity,b);return b.ToString();}
 static void Click(IntPtr w,int id){SendMessage(GetDlgItem(w,id),0xf5,IntPtr.Zero,IntPtr.Zero);Thread.Sleep(120);}
 static int Count(IntPtr w){return (int)SendMessage(GetDlgItem(w,2303),0x18b,IntPtr.Zero,IntPtr.Zero);}
 static void Select(IntPtr w,int index){SendMessage(GetDlgItem(w,2303),0x186,(IntPtr)index,IntPtr.Zero);SendMessage(w,0x111,(IntPtr)((1<<16)|2303),GetDlgItem(w,2303));Thread.Sleep(80);}
 static int Find(IntPtr w,string name){for(int i=0;i<Count(w);++i){var b=new StringBuilder(512);ReadText(GetDlgItem(w,2303),0x189,(IntPtr)i,b);if(b.ToString()==name)return i;}return -1;}
 static void Choose(IntPtr w,string name){int row=Find(w,name);if(row<0)throw new Exception("Missing result "+name);Select(w,row);}
 static IntPtr Search(Process p,IntPtr root,string query){SetText(GetDlgItem(root,1002),0xc,IntPtr.Zero,query);Click(root,1003);return W(p,"WorldMapExplorerUnifiedSearch");}
 static void Go(Process p,IntPtr root,string query){var w=Search(p,root,query);Click(w,2306);}
 static void Rename(Process p,IntPtr palette,string value){PostMessage(GetDlgItem(palette,2308),0xf5,IntPtr.Zero,IntPtr.Zero);var d=W(p,"WorldMapExplorerRenameSaved");SetText(GetDlgItem(d,7101),0xc,IntPtr.Zero,value);Click(d,7102);Wait(()=>!IsWindowVisible(d));Thread.Sleep(200);}
 static void Refresh(IntPtr root){SendMessage(root,6,(IntPtr)1,IntPtr.Zero);Thread.Sleep(250);}
 static void Shot(IntPtr w,string file){R r;GetWindowRect(w,out r);using(var b=new Bitmap(r.r-r.l,r.b-r.t))using(var g=Graphics.FromImage(b)){var dc=g.GetHdc();PrintWindow(w,dc,2);g.ReleaseHdc(dc);b.Save(file,ImageFormat.Png);}}
 static Process Start(string app,string profile){var si=new ProcessStartInfo(app){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(app)};si.EnvironmentVariables["LOCALAPPDATA"]=profile;var p=Process.Start(si);var w=W(p,"WorldMapExplorer");if(!Wait(()=>GetDlgItem(w,1065)!=IntPtr.Zero))throw new Exception("Incomplete window");return p;}
 static void Close(Process p){if(p!=null&&!p.HasExited){p.Refresh();PostMessage(p.MainWindowHandle,0x10,IntPtr.Zero,IntPtr.Zero);if(!p.WaitForExit(8000))p.Kill();}}
 static int Main(string[] args){Directory.CreateDirectory(args[1]);string profile=Path.Combine(args[1],"profile");Directory.CreateDirectory(profile);Process a=null,b=null;try{
  a=Start(args[0],profile);b=Start(args[0],profile);IntPtr ar=W(a,"WorldMapExplorer"),br=W(b,"WorldMapExplorer");Go(a,ar,"北京");Go(b,br,"Rome");Click(ar,1083);Click(br,1083);Click(ar,1082);var ap=W(a,"WorldMapExplorerUnifiedSearch");Check(Count(ap)==2,"two windows retain independent additions");Choose(ap,"罗马市");Rename(a,ap,"周末 · 罗马 / Roma");Check(Find(ap,"周末 · 罗马 / Roma")>=0,"saved place can be renamed without navigating away");
  Click(br,1082);var bp=W(b,"WorldMapExplorerUnifiedSearch");Check(Find(bp,"周末 · 罗马 / Roma")>=0,"another window observes the new name");Choose(bp,"北京");Click(bp,2309);Refresh(ar);Check(Count(ap)==1&&Text(GetDlgItem(ap,2305)).Contains("周末 · 罗马 / Roma"),"external removal retains the selected place by coordinates");Check(Text(GetDlgItem(ar,1083))=="☆","star updates when another window removes the current place");Click(bp,2310);Refresh(ar);Check(Count(ap)==2,"undo removal merges with another window's collection");
  Choose(bp,"周末 · 罗马 / Roma");Rename(b,bp,"更新名称 · Roma");Refresh(ar);Click(ap,2310);Check(Find(ap,"更新名称 · Roma")>=0&&Text(GetDlgItem(ap,2304)).Contains("较新"),"undo rename does not overwrite another window's newer name");Check(!IsWindowVisible(GetDlgItem(ap,2310)),"a superseded undo does not leave an unusable action");Choose(ap,"更新名称 · Roma");Click(ap,2309);Check(Count(ap)==1,"remove selected saved place");Click(ap,2310);Check(Count(ap)==2,"undo selected removal");Shot(ap,Path.Combine(args[1],"saved-manager-zh.png"));Click(ar,1024);Check(Text(GetDlgItem(ap,2309))=="Remove"&&Text(GetDlgItem(ap,2310))=="Undo change","saved actions update with the interface language");Shot(ap,Path.Combine(args[1],"saved-manager-en.png"));
  Choose(ap,"更新名称 · Roma");Click(ap,2306);Console.WriteLine("Opened saved target: "+Text(GetDlgItem(ar,1083))+" / "+Text(GetDlgItem(ar,1065)));Click(ar,1083);Click(ar,1082);ap=W(a,"WorldMapExplorerUnifiedSearch");Check(Find(ap,"更新名称 · Roma")<0,"a stable saved-result payload opens the intended coordinates");Choose(ap,"北京");Click(ap,2306);Close(a);a=null;Refresh(br);Click(br,1082);bp=W(b,"WorldMapExplorerUnifiedSearch");Choose(bp,"北京");Rename(b,bp,"跨窗口 · 北京");a=Start(args[0],profile);ar=W(a,"WorldMapExplorer");Check(Wait(()=>Text(GetDlgItem(ar,1083))=="★"),"restored saved location stays selected");Close(a);a=null;byte[] session=File.ReadAllBytes(Path.Combine(profile,"WorldMapExplorer","last-view-v1.bin")),name=Encoding.Unicode.GetBytes("跨窗口 · 北京");bool foundName=false;for(int i=0;i+name.Length<=session.Length&&!foundName;++i){int j=0;while(j<name.Length&&session[i+j]==name[j])++j;foundName=j==name.Length;}Check(foundName,"restored saved labels use the latest name from another window");Console.WriteLine(checks+" saved-manager UI checks passed");return 0;
 }catch(Exception e){Console.WriteLine("FAIL "+e);return 1;}finally{Close(a);Close(b);}}
}



