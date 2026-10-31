using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

internal static class WorldMapExplorerUiSmoke
{
    private const uint WM_COMMAND = 0x0111, WM_HSCROLL = 0x0114, WM_TIMER = 0x0113, WM_CLOSE = 0x0010, WM_NULL = 0x0000, WM_SETTEXT = 0x000C, WM_KEYDOWN = 0x0100;
    private const uint WM_LBUTTONDOWN = 0x0201, WM_MOUSEMOVE = 0x0200, WM_LBUTTONUP = 0x0202, WM_LBUTTONDBLCLK = 0x0203, WM_MOUSEWHEEL = 0x020A;
    private const uint CBN_SELCHANGE = 1, BM_CLICK = 0x00F5, BM_GETCHECK = 0x00F0, CB_GETCOUNT = 0x0146, CB_GETLBTEXT = 0x0148, CB_SETCURSEL = 0x014E, TBM_GETPOS = 0x0400, TBM_SETPOS = 0x0405;
    private const uint PW_RENDERFULLCONTENT = 2, SMTO_ABORTIFHUNG = 0x0002, VK_F11 = 0x7a, VK_ESCAPE = 0x1b;
    private delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)] private struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] private struct POINT { public int X, Y; }
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumWindowsProc cb, IntPtr data);
    [DllImport("user32.dll")] private static extern bool EnumChildWindows(IntPtr hwnd, EnumWindowsProc cb, IntPtr data);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetClassName(IntPtr hwnd, StringBuilder text, int max);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetWindowText(IntPtr hwnd, StringBuilder text, int max);
    [DllImport("user32.dll")] private static extern int GetDlgCtrlID(IntPtr hwnd);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
    [DllImport("user32.dll")] private static extern IntPtr SendMessage(IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern IntPtr SendMessage(IntPtr hwnd, uint message, IntPtr wParam, StringBuilder lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern IntPtr SendMessage(IntPtr hwnd, uint message, IntPtr wParam, [MarshalAs(UnmanagedType.LPWStr)] string lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern bool SendMessageTimeout(IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam, uint flags, uint timeout, out IntPtr result);
    [DllImport("user32.dll")] private static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] private static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] private static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")] private static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] private static extern bool PostMessage(IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] private static extern int GetSystemMetrics(int index);
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)] private static extern uint ExtractIconEx(string file, int index, IntPtr large, IntPtr small, uint count);

    private static int checks, failures;
    private static void Check(bool condition, string name)
    {
        checks++;
        Console.WriteLine((condition ? "PASS: " : "FAIL: ") + name);
        if (!condition) failures++;
    }
    private static IntPtr FindMainWindow(uint pid)
    {
        IntPtr found = IntPtr.Zero;
        EnumWindows(delegate(IntPtr hwnd, IntPtr data) {
            uint owner; GetWindowThreadProcessId(hwnd, out owner);
            if (owner == pid && IsWindowVisible(hwnd)) { found = hwnd; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }
    private static IntPtr FindControl(IntPtr parent, int id, string className)
    {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, delegate(IntPtr hwnd, IntPtr data) {
            StringBuilder name = new StringBuilder(64); GetClassName(hwnd, name, name.Capacity);
            if (GetDlgCtrlID(hwnd) == id && String.Equals(name.ToString(), className, StringComparison.OrdinalIgnoreCase)) { found = hwnd; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }
    private static string ComboText(IntPtr combo, int index)
    {
        StringBuilder text = new StringBuilder(128);
        SendMessage(combo, CB_GETLBTEXT, (IntPtr)index, text);
        return text.ToString();
    }
    private static string WindowText(IntPtr hwnd)
    {
        StringBuilder text = new StringBuilder(128);
        GetWindowText(hwnd, text, text.Capacity);
        return text.ToString();
    }
    private static RECT ControlRectInClient(IntPtr window, IntPtr control)
    {
        RECT rect; POINT origin = new POINT { X = 0, Y = 0 };
        if (!GetWindowRect(control, out rect) || !ClientToScreen(window, ref origin)) return new RECT();
        return new RECT { Left = rect.Left - origin.X, Top = rect.Top - origin.Y, Right = rect.Right - origin.X, Bottom = rect.Bottom - origin.Y };
    }
    private static int FindHistoryIndex(IntPtr combo, int year)
    {
        string label = year < 0 ? "\u516c\u5143\u524d" + (-year).ToString(CultureInfo.InvariantCulture) + "\u5e74" : "\u516c\u5143" + year.ToString(CultureInfo.InvariantCulture) + "\u5e74";
        int count = (int)SendMessage(combo, CB_GETCOUNT, IntPtr.Zero, IntPtr.Zero);
        for (int i = 0; i < count; i++) if (String.Equals(ComboText(combo, i), label, StringComparison.Ordinal)) return i;
        return -1;
    }
    private static bool Capture(IntPtr hwnd, string path)
    {
        RECT rect; if (!GetWindowRect(hwnd, out rect)) return false;
        using (Bitmap bitmap = new Bitmap(rect.Right - rect.Left, rect.Bottom - rect.Top))
        using (Graphics graphics = Graphics.FromImage(bitmap)) {
            IntPtr hdc = graphics.GetHdc(); bool ok;
            try { ok = PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT); }
            finally { graphics.ReleaseHdc(hdc); }
            if (!ok) return false;
            bitmap.Save(path, ImageFormat.Png);
            return true;
        }
    }
    private static Color ControlFillColor(IntPtr window, IntPtr control)
    {
        RECT main, rect; POINT origin = new POINT { X = 0, Y = 0 };
        if (!GetWindowRect(window, out main) || !GetWindowRect(control, out rect) || !ClientToScreen(window, ref origin)) return Color.Empty;
        using (Bitmap bitmap = new Bitmap(main.Right - main.Left, main.Bottom - main.Top))
        using (Graphics graphics = Graphics.FromImage(bitmap)) {
            IntPtr hdc = graphics.GetHdc(); bool ok;
            try { ok = PrintWindow(window, hdc, PW_RENDERFULLCONTENT); }
            finally { graphics.ReleaseHdc(hdc); }
            if (!ok) return Color.Empty;
            int x = rect.Left - main.Left + 20, y = rect.Top - main.Top + 10;
            if (x < 0 || y < 0 || x >= bitmap.Width || y >= bitmap.Height) return Color.Empty;
            return bitmap.GetPixel(x, y);
        }
    }
    private static string CaptureFingerprint(IntPtr hwnd)
    {
        RECT rect; if (!GetWindowRect(hwnd, out rect)) return "";
        using (Bitmap bitmap = new Bitmap(rect.Right - rect.Left, rect.Bottom - rect.Top))
        using (Graphics graphics = Graphics.FromImage(bitmap))
        using (MemoryStream stream = new MemoryStream()) {
            IntPtr hdc = graphics.GetHdc(); bool ok;
            try { ok = PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT); }
            finally { graphics.ReleaseHdc(hdc); }
            if (!ok) return "";
            bitmap.Save(stream, ImageFormat.Png);
            using (SHA256 sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(stream.ToArray()));
        }
    }
    private static string CaptureClientRegionFingerprint(IntPtr hwnd, Rectangle clientRegion)
    {
        RECT window; if (!GetWindowRect(hwnd, out window)) return "";
        POINT clientOrigin = new POINT { X = 0, Y = 0 }; if (!ClientToScreen(hwnd, ref clientOrigin)) return "";
        using (Bitmap bitmap = new Bitmap(window.Right - window.Left, window.Bottom - window.Top))
        using (Graphics graphics = Graphics.FromImage(bitmap))
        using (MemoryStream stream = new MemoryStream()) {
            IntPtr hdc = graphics.GetHdc(); bool ok;
            try { ok = PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT); }
            finally { graphics.ReleaseHdc(hdc); }
            if (!ok) return "";
            Rectangle crop = new Rectangle(clientOrigin.X - window.Left + clientRegion.X, clientOrigin.Y - window.Top + clientRegion.Y, clientRegion.Width, clientRegion.Height);
            if (crop.X < 0 || crop.Y < 0 || crop.Right > bitmap.Width || crop.Bottom > bitmap.Height) return "";
            using (Bitmap region = bitmap.Clone(crop, PixelFormat.Format32bppArgb)) region.Save(stream, ImageFormat.Png);
            using (SHA256 sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(stream.ToArray()));
        }
    }
    private static string CaptureMapFingerprint(IntPtr hwnd)
    {
        RECT window, client; if (!GetWindowRect(hwnd, out window) || !GetClientRect(hwnd, out client)) return "";
        POINT clientOrigin = new POINT { X = 0, Y = 0 }; if (!ClientToScreen(hwnd, ref clientOrigin)) return "";
        using (Bitmap bitmap = new Bitmap(window.Right - window.Left, window.Bottom - window.Top))
        using (Graphics graphics = Graphics.FromImage(bitmap))
        using (MemoryStream stream = new MemoryStream()) {
            IntPtr hdc = graphics.GetHdc(); bool ok;
            try { ok = PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT); }
            finally { graphics.ReleaseHdc(hdc); }
            if (!ok) return "";
            int availW = client.Right - 360 - 36 - 20, availH = client.Bottom - 108 - 116;
            int mapWidth = availW, mapHeight = (int)(mapWidth / 2.0);
            if (mapHeight > availH) { mapHeight = availH; mapWidth = (int)(mapHeight * 2.0); }
            int mapLeft = 24, mapTop = 90 + 116 + (availH - mapHeight) / 2;
            int cropLeft = mapLeft + 320, cropTop = mapTop + 112;
            int cropWidth = mapWidth - 320 - 350, cropHeight = mapHeight - 120;
            Rectangle crop = new Rectangle(clientOrigin.X - window.Left + cropLeft, clientOrigin.Y - window.Top + cropTop, cropWidth, cropHeight);
            if (crop.X < 0 || crop.Y < 0 || crop.Right > bitmap.Width || crop.Bottom > bitmap.Height) return "";
            using (Bitmap map = bitmap.Clone(crop, PixelFormat.Format32bppArgb)) map.Save(stream, ImageFormat.Png);
            using (SHA256 sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(stream.ToArray()));
        }
    }
    private static IntPtr PackPoint(int x, int y)
    {
        return new IntPtr(unchecked((int)((((uint)y & 0xffffu) << 16) | ((uint)x & 0xffffu))));
    }
    private static void WaitForRedraw(IntPtr hwnd)
    {
        Thread.Sleep(900);
        IntPtr result;
        Check(SendMessageTimeout(hwnd, WM_NULL, IntPtr.Zero, IntPtr.Zero, SMTO_ABORTIFHUNG, 2500, out result), "application window remains responsive");
    }
    private static void SelectHistoryYear(IntPtr window, IntPtr combo, int index)
    {
        SendMessage(combo, CB_SETCURSEL, (IntPtr)index, IntPtr.Zero);
        SendMessage(window, WM_COMMAND, (IntPtr)((CBN_SELCHANGE << 16) | 1010), combo);
        WaitForRedraw(window);
    }
    private static void SetFillOpacity(IntPtr window, IntPtr trackbar, int opacity)
    {
        SendMessage(trackbar, TBM_SETPOS, (IntPtr)1, (IntPtr)opacity);
        SendMessage(window, WM_HSCROLL, (IntPtr)(4 | (opacity << 16)), trackbar);
        WaitForRedraw(window);
    }
    private static int Main(string[] args)
    {
        if (args.Length != 2) { Console.Error.WriteLine("Usage: WorldMapExplorerUiSmoke.exe <app.exe> <qa-directory>"); return 2; }
        Process process = Process.Start(new ProcessStartInfo(args[0]) { WorkingDirectory = System.IO.Path.GetDirectoryName(args[0]), UseShellExecute = true });
        if (process == null) return 3;
        try {
            process.WaitForInputIdle(15000);
            IntPtr window = IntPtr.Zero;
            for (int attempt = 0; attempt < 50 && window == IntPtr.Zero; attempt++) { process.Refresh(); window = FindMainWindow((uint)process.Id); Thread.Sleep(200); }
            Check(window != IntPtr.Zero, "application launches with a visible main window");
            if (window == IntPtr.Zero) return 4;
            Check(ExtractIconEx(args[0], -1, IntPtr.Zero, IntPtr.Zero, 0) >= 1, "executable contains an embedded multi-size application icon");
            IntPtr combo = FindControl(window, 1008, "ComboBox");
            IntPtr edit = FindControl(window, 1002, "Edit");
            IntPtr search = FindControl(window, 1003, "Button");
            IntPtr hd = FindControl(window, 1004, "Button");
            IntPtr nasa = FindControl(window, 1006, "Button");
            IntPtr landCover = FindControl(window, 1005, "Button");
            IntPtr population = FindControl(window, 1007, "Button");
            IntPtr world2026 = FindControl(window, 1009, "Button");
            IntPtr historyYearCombo = FindControl(window, 1010, "ComboBox");
            IntPtr historyToggle = FindControl(window, 1011, "Button");
            IntPtr opacity = FindControl(window, 1012, "msctls_trackbar32");
            IntPtr opacityLabel = FindControl(window, 1013, "Static");
            IntPtr gridLines = FindControl(window, 1014, "Button");
            IntPtr historyPlayback = FindControl(window, 1015, "Button");
            IntPtr historySpeed = FindControl(window, 1016, "Button");
            IntPtr historyAnimation = FindControl(window, 1020, "Button");
            IntPtr geoTab = FindControl(window, 1018, "Button");
            IntPtr eventsTab = FindControl(window, 1019, "Button");
            IntPtr fullscreen = FindControl(window, 1023, "Button");
            Check(combo != IntPtr.Zero && edit != IntPtr.Zero && search != IntPtr.Zero && hd != IntPtr.Zero && nasa != IntPtr.Zero && landCover != IntPtr.Zero && population != IntPtr.Zero && world2026 != IntPtr.Zero && historyYearCombo != IntPtr.Zero && historyToggle != IntPtr.Zero && opacity != IntPtr.Zero && opacityLabel != IntPtr.Zero && gridLines != IntPtr.Zero && historyPlayback != IntPtr.Zero && historySpeed != IntPtr.Zero && historyAnimation != IntPtr.Zero && geoTab != IntPtr.Zero && eventsTab != IntPtr.Zero && fullscreen != IntPtr.Zero, "map controls, fullscreen mode, trial animation switch and information tabs exist");
            if (combo == IntPtr.Zero || edit == IntPtr.Zero || search == IntPtr.Zero || hd == IntPtr.Zero || nasa == IntPtr.Zero || landCover == IntPtr.Zero || population == IntPtr.Zero || world2026 == IntPtr.Zero || historyYearCombo == IntPtr.Zero || historyToggle == IntPtr.Zero || opacity == IntPtr.Zero || opacityLabel == IntPtr.Zero || gridLines == IntPtr.Zero || historyPlayback == IntPtr.Zero || historySpeed == IntPtr.Zero || historyAnimation == IntPtr.Zero || geoTab == IntPtr.Zero || eventsTab == IntPtr.Zero || fullscreen == IntPtr.Zero) return 5;
            SendMessage(combo, CB_SETCURSEL, (IntPtr)3, IntPtr.Zero);
            SendMessage(window, WM_COMMAND, (IntPtr)((CBN_SELCHANGE << 16) | 1008), combo);
            WaitForRedraw(window);
            RECT client; GetClientRect(window, out client);
            IntPtr[] toolbarControls = { landCover, nasa, population, world2026, historyYearCombo, historyPlayback, historySpeed, historyAnimation, historyToggle, opacityLabel, opacity, gridLines };
            bool toolbarOutsideMap = true;
            foreach (IntPtr control in toolbarControls) {
                RECT controlRect = ControlRectInClient(window, control);
                toolbarOutsideMap = toolbarOutsideMap && controlRect.Left >= 0 && controlRect.Right <= client.Right && controlRect.Bottom <= 254;
            }
            Check(toolbarOutsideMap, "all map controls, including the trial animation switch, remain above the map image");
            Check((int)SendMessage(historyAnimation, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 0, "approximate transition mode is off by default");
            Color activeGeoColor = ControlFillColor(window, geoTab);
            SendMessage(eventsTab, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Color activeEventColor = ControlFillColor(window, eventsTab), inactiveGeoColor = ControlFillColor(window, geoTab);
            Check(activeGeoColor != Color.Empty && activeEventColor != Color.Empty && inactiveGeoColor != Color.Empty && activeGeoColor.G > inactiveGeoColor.G && activeEventColor.G > inactiveGeoColor.G && activeGeoColor == activeEventColor, "information tabs keep a distinct color for the selected page");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-history-events.png")), "history event tab captured while no map cell is selected");
            Rectangle eventContentRegion = new Rectangle(client.Right - 340, client.Top + 170, 320, client.Bottom - 190);
            string eventPanelTop = CaptureClientRegionFingerprint(window, eventContentRegion);
            Thread.Sleep(260);
            string eventPanelAutoScrolled = CaptureClientRegionFingerprint(window, eventContentRegion);
            Check(eventPanelTop.Length > 0 && eventPanelAutoScrolled.Length > 0 && eventPanelAutoScrolled != eventPanelTop, "historical events advance smoothly in a continuous loop");
            POINT eventPanelPoint = new POINT { X = client.Right - 70, Y = client.Bottom / 2 }; ClientToScreen(window, ref eventPanelPoint);
            SendMessage(window, WM_MOUSEWHEEL, new IntPtr(unchecked((short)-120 << 16)), PackPoint(eventPanelPoint.X, eventPanelPoint.Y));
            WaitForRedraw(window);
            string eventPanelScrolled = CaptureClientRegionFingerprint(window, eventContentRegion);
            List<string> eventFrames = new List<string>();
            for (int frame = 0; frame < 5; frame++) { eventFrames.Add(CaptureClientRegionFingerprint(window, eventContentRegion)); Thread.Sleep(80); }
            Check(eventPanelAutoScrolled.Length > 0 && eventPanelScrolled.Length > 0 && eventPanelScrolled != eventPanelAutoScrolled && Capture(window, System.IO.Path.Combine(args[1], "ui-history-events-scrolled.png")), "mouse-wheel input scrolls the event list and temporarily pauses automation");
            Check(eventFrames[0].Length > 0 && eventFrames.TrueForAll(hash => hash == eventFrames[0]), "manually paused event panel remains visually stable with no flashing");
            SendMessage(geoTab, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(gridLines, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1, "grid lines are visible by default");
            Color gridEnabledColor = ControlFillColor(window, gridLines);
            string gridVisible = CaptureMapFingerprint(window);
            SendMessage(gridLines, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Color gridDisabledColor = ControlFillColor(window, gridLines);
            Check(gridEnabledColor != Color.Empty && gridDisabledColor != Color.Empty && gridEnabledColor.G > gridDisabledColor.G, "map layer buttons change color when toggled");
            string gridHidden = CaptureMapFingerprint(window);
            Check((int)SendMessage(gridLines, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 0 && gridVisible.Length > 0 && gridHidden.Length > 0 && gridVisible != gridHidden, "grid-line switch hides only the drawn lines");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-gridlines-hidden.png")), "map screenshot captured with grid lines hidden");
            SendMessage(gridLines, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(gridLines, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && CaptureMapFingerprint(window) == gridVisible, "turning grid lines back on restores the same map appearance");
            SendMessage(gridLines, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            int hiddenCellX = client.Right * 366 / 1000, hiddenCellY = client.Bottom * 55 / 100;
            SendMessage(window, WM_LBUTTONDOWN, new IntPtr(1), PackPoint(hiddenCellX, hiddenCellY));
            SendMessage(window, WM_LBUTTONUP, IntPtr.Zero, PackPoint(hiddenCellX, hiddenCellY));
            WaitForRedraw(window);
            string selectedWithoutLines = CaptureMapFingerprint(window);
            Check((int)SendMessage(gridLines, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 0 && (int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3 && selectedWithoutLines.Length > 0 && selectedWithoutLines != gridHidden, "map cells remain clickable and selectable while grid lines are hidden");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-gridlines-hidden-cell-selected.png")), "selected fine cell remains visible with grid lines hidden");

            Check(combo != IntPtr.Zero && edit != IntPtr.Zero && search != IntPtr.Zero && hd != IntPtr.Zero && nasa != IntPtr.Zero && landCover != IntPtr.Zero && population != IntPtr.Zero && world2026 != IntPtr.Zero && historyYearCombo != IntPtr.Zero && historyToggle != IntPtr.Zero && opacity != IntPtr.Zero && opacityLabel != IntPtr.Zero && historyPlayback != IntPtr.Zero && historySpeed != IntPtr.Zero && historyAnimation != IntPtr.Zero, "grid, search, thematic layers, history playback, trial animation and opacity controls exist");
            if (combo == IntPtr.Zero || edit == IntPtr.Zero || search == IntPtr.Zero || hd == IntPtr.Zero || nasa == IntPtr.Zero || landCover == IntPtr.Zero || population == IntPtr.Zero || world2026 == IntPtr.Zero || historyYearCombo == IntPtr.Zero || historyToggle == IntPtr.Zero || opacity == IntPtr.Zero || opacityLabel == IntPtr.Zero || historyPlayback == IntPtr.Zero || historySpeed == IntPtr.Zero || historyAnimation == IntPtr.Zero) return 5;
            Check((int)SendMessage(combo, CB_GETCOUNT, IntPtr.Zero, IntPtr.Zero) == 4, "four selectable grid levels are present");
            for (int i = 0; i < 4; i++) Check(!String.IsNullOrWhiteSpace(ComboText(combo, i)), "grid option text " + i.ToString(CultureInfo.InvariantCulture) + " is readable");
            for (int level = 0; level < 4; level++) {
                SendMessage(combo, CB_SETCURSEL, (IntPtr)level, IntPtr.Zero);
                SendMessage(window, WM_COMMAND, (IntPtr)((CBN_SELCHANGE << 16) | 1008), combo);
                WaitForRedraw(window);
                Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == level, "grid option selection " + level.ToString(CultureInfo.InvariantCulture));
                string file = System.IO.Path.Combine(args[1], "ui-standard-level-" + level.ToString(CultureInfo.InvariantCulture) + ".png");
                Check(Capture(window, file), "standard map screenshot captured for level " + level.ToString(CultureInfo.InvariantCulture));
            }
            List<string> frameHashes = new List<string>();
            for (int frame = 0; frame < 5; frame++) frameHashes.Add(CaptureFingerprint(window));
            Check(frameHashes[0].Length > 0 && frameHashes.TrueForAll(hash => hash == frameHashes[0]), "five repeated maximum-detail frames remain visually stable (no flashing) ");
            SendMessage(edit, WM_SETTEXT, IntPtr.Zero, "Z627200-0248-0591");
            SendMessage(search, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3, "searching a fine-cell ID preserves its grid level");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-search-fine-cell.png")), "fine-cell search screenshot captured with parent-cell data visible");
            GetClientRect(window, out client);
            int mapX = client.Right * 366 / 1000, mapY = client.Bottom * 55 / 100;
            POINT wheelPoint = new POINT { X = mapX, Y = mapY };
            ClientToScreen(window, ref wheelPoint);
            string beforeWheel = CaptureFingerprint(window);
            SendMessage(window, WM_MOUSEWHEEL, new IntPtr(120 << 16), PackPoint(wheelPoint.X, wheelPoint.Y));
            WaitForRedraw(window);
            string afterWheel = CaptureFingerprint(window);
            Check(beforeWheel.Length > 0 && afterWheel.Length > 0 && beforeWheel != afterWheel && (int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3, "mouse-wheel zoom redraws the fine grid and retains its level");
            SendMessage(window, WM_MOUSEWHEEL, new IntPtr(unchecked((short)-120 << 16)), PackPoint(wheelPoint.X, wheelPoint.Y));
            WaitForRedraw(window);
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-wheel-zoom-fine.png")), "fine grid remains visible after zooming back");
            IntPtr panStart = PackPoint(mapX, mapY), panEnd = PackPoint(mapX + 35, mapY + 20);
            SendMessage(window, WM_LBUTTONDOWN, new IntPtr(1), panStart);
            SendMessage(window, WM_MOUSEMOVE, new IntPtr(1), panEnd);
            SendMessage(window, WM_LBUTTONUP, IntPtr.Zero, panEnd);
            WaitForRedraw(window);
            Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3 && Capture(window, System.IO.Path.Combine(args[1], "ui-pan-fine.png")), "drag panning retains the selected fine-grid level");
            SendMessage(window, WM_LBUTTONDBLCLK, IntPtr.Zero, panStart);
            WaitForRedraw(window);
            Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3 && Capture(window, System.IO.Path.Combine(args[1], "ui-reset-fine.png")), "double-click reset retains the selected fine-grid level");
            SendMessage(landCover, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3 && Capture(window, System.IO.Path.Combine(args[1], "ui-landcover-fine.png")), "fine grid remains visible over the land-cover layer");
            SendMessage(population, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3 && Capture(window, System.IO.Path.Combine(args[1], "ui-population-fine.png")), "fine grid remains visible over the population layer");
            SendMessage(population, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string thematicBaseFingerprint = CaptureMapFingerprint(window);
            int historyCount = (int)SendMessage(historyYearCombo, CB_GETCOUNT, IntPtr.Zero, IntPtr.Zero);
            Check(historyCount > 165 && historyCount <= 1000, "historical timeline has more than 165 nodes and does not exceed 1,000");
            HashSet<string> labels = new HashSet<string>();
            for (int i = 0; i < historyCount; i++) { string label = ComboText(historyYearCombo, i); Check(!String.IsNullOrWhiteSpace(label), "historical year option " + i.ToString(CultureInfo.InvariantCulture) + " is readable"); labels.Add(label); }
            Check(labels.Count == historyCount && ComboText(historyYearCombo, 0).Contains("2024") && ComboText(historyYearCombo, historyCount - 1).Contains("\u524d3400"), "historical years are unique and span 3400 BCE through 2024 CE");
            Check((int)SendMessage(opacity, TBM_GETPOS, IntPtr.Zero, IntPtr.Zero) == 72, "fill opacity starts at a readable 72 percent");
            SendMessage(historyToggle, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string history2024Fingerprint = CaptureMapFingerprint(window);
            Check((int)SendMessage(historyToggle, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && history2024Fingerprint.Length > 0 && history2024Fingerprint != thematicBaseFingerprint, "World 2024 opens as an independent historical layer");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world2024-enabled.png")), "World 2024 screenshot captured at 627,200 cells");
            int year1800Index = FindHistoryIndex(historyYearCombo, 1800);
            SelectHistoryYear(window, historyYearCombo, year1800Index);
            string history1800Fingerprint = CaptureMapFingerprint(window);
            Check((int)SendMessage(historyToggle, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && history1800Fingerprint.Length > 0 && history1800Fingerprint != history2024Fingerprint, "switching from 2024 to 1800 replaces the visible historical fill");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world1800-enabled.png")), "World 1800 screenshot captured after changing the year");
            int year2024Index = FindHistoryIndex(historyYearCombo, 2024);
            SelectHistoryYear(window, historyYearCombo, year2024Index);
            string history2024ReturnedFingerprint = CaptureMapFingerprint(window);
            Check((int)SendMessage(historyToggle, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && history2024ReturnedFingerprint == history2024Fingerprint, "switching back to 2024 restores the 2024 fill raster");
            List<string> historyFrames = new List<string>();
            for (int frame = 0; frame < 5; frame++) historyFrames.Add(CaptureFingerprint(window));
            Check(historyFrames[0].Length > 0 && historyFrames.TrueForAll(hash => hash == historyFrames[0]), "historical layer remains visually stable with no flashing");
            SetFillOpacity(window, opacity, 0);
            Check(CaptureMapFingerprint(window) == thematicBaseFingerprint, "zero fill opacity restores the unobscured base map");
            SetFillOpacity(window, opacity, 25);
            string lowOpacityFingerprint = CaptureMapFingerprint(window);
            SetFillOpacity(window, opacity, 100);
            string fullOpacityFingerprint = CaptureMapFingerprint(window);
            Check(lowOpacityFingerprint.Length > 0 && lowOpacityFingerprint != thematicBaseFingerprint && fullOpacityFingerprint != lowOpacityFingerprint, "opacity slider changes map fill continuously from transparent to opaque");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world2024-opacity-100.png")), "historical map screenshot captured at full opacity");
            int[] yearsToCheck = { 2024, 1914, 1800, 1600, 1400, 1200, 1000, 476, -1000, -3400 };
            string previousYearFingerprint = fullOpacityFingerprint;
            for (int i = 0; i < yearsToCheck.Length; i++) {
                int yearIndex = FindHistoryIndex(historyYearCombo, yearsToCheck[i]);
                Check(yearIndex >= 0, "historical year is selectable: " + yearsToCheck[i].ToString(CultureInfo.InvariantCulture));
                if (yearIndex < 0) continue;
                SelectHistoryYear(window, historyYearCombo, yearIndex);
                string yearFingerprint = CaptureMapFingerprint(window);
                bool fillChanged = i == 0 ? yearFingerprint == fullOpacityFingerprint : yearFingerprint != previousYearFingerprint;
                Check((int)SendMessage(historyYearCombo, 0x0147, IntPtr.Zero, IntPtr.Zero) == yearIndex && (int)SendMessage(historyToggle, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && yearFingerprint.Length > 0 && fillChanged, "World " + yearsToCheck[i].ToString(CultureInfo.InvariantCulture) + " loads its own visible fill raster");
                if (yearsToCheck[i] == 1600 || yearsToCheck[i] == 1400 || yearsToCheck[i] == 1200 || yearsToCheck[i] == 1000) Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world" + yearsToCheck[i].ToString(CultureInfo.InvariantCulture) + "-enabled.png")), "World " + yearsToCheck[i].ToString(CultureInfo.InvariantCulture) + " screenshot captured after switching years");
                previousYearFingerprint = yearFingerprint;
            }
            SendMessage(historySpeed, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            SendMessage(historySpeed, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Check(WindowText(historySpeed).Contains("4"), "history playback speed cycles up to 4x");
            SendMessage(eventsTab, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            SendMessage(historyAnimation, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Check((int)SendMessage(historyAnimation, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1, "trial transition mode can be enabled independently");
            SendMessage(historySpeed, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Check(WindowText(historySpeed).Contains("0.5"), "the slow playback speed gives the trial transition time to render");
            SendMessage(historyPlayback, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            int playbackStartIndex = (int)SendMessage(historyYearCombo, 0x0147, IntPtr.Zero, IntPtr.Zero);
            Check(playbackStartIndex == historyCount - 1, "playback begins at the oldest year before advancing");
            int year1914Index = FindHistoryIndex(historyYearCombo, 1914);
            Check(year1914Index >= 0, "1914 is available as an event-bearing playback relay node");
            SendMessage(historyYearCombo, CB_SETCURSEL, (IntPtr)(year1914Index + 1), IntPtr.Zero);
            SendMessage(window, WM_TIMER, (IntPtr)1017, IntPtr.Zero);
            Thread.Sleep(70);
            List<string> animationFrames = new List<string>();
            for (int frame = 0; frame < 7; frame++) { animationFrames.Add(CaptureMapFingerprint(window)); Thread.Sleep(65); }
            HashSet<string> animationFrameSet = new HashSet<string>(animationFrames);
            Check(animationFrameSet.Count >= 3, "enabled trial mode renders multiple intermediate fill frames during playback");
            int firstPlaybackIndex = (int)SendMessage(historyYearCombo, 0x0147, IntPtr.Zero, IntPtr.Zero);
            Check(firstPlaybackIndex == year1914Index && WindowText(historyPlayback).Contains("\u6682\u505c"), "playback relays the 1914 event interval while the transition runs");
            Check(ControlFillColor(window, eventsTab).G > ControlFillColor(window, geoTab).G && Capture(window, System.IO.Path.Combine(args[1], "ui-playback-events.png")), "historical event tab stays visible while the timeline is playing");
            SendMessage(historyPlayback, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            int pausedPlaybackIndex = (int)SendMessage(historyYearCombo, 0x0147, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(500);
            Check(WindowText(historyPlayback).Contains("\u7ee7\u7eed") && (int)SendMessage(historyYearCombo, 0x0147, IntPtr.Zero, IntPtr.Zero) == pausedPlaybackIndex, "pause freezes the selected year and offers resume");
            SendMessage(historyAnimation, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Check((int)SendMessage(historyAnimation, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 0, "trial animation can be turned off while paused");
            SendMessage(historySpeed, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            SendMessage(historyPlayback, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(1000);
            int resumedPlaybackIndex = (int)SendMessage(historyYearCombo, 0x0147, IntPtr.Zero, IntPtr.Zero);
            SendMessage(historyPlayback, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Check(resumedPlaybackIndex >= 0 && resumedPlaybackIndex < pausedPlaybackIndex, "resuming playback continues forward from the paused year");
            SendMessage(historySpeed, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            SendMessage(historySpeed, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            SelectHistoryYear(window, historyYearCombo, 0);
            SendMessage(historyToggle, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(historyToggle, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 0 && CaptureMapFingerprint(window) == thematicBaseFingerprint, "history layer closes cleanly and restores the base map");
            SetFillOpacity(window, opacity, 72);
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string worldAt72 = CaptureMapFingerprint(window);
            SendMessage(historyToggle, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string combinedFillFingerprint = CaptureMapFingerprint(window);
            Check((int)SendMessage(historyToggle, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && combinedFillFingerprint.Length > 0 && combinedFillFingerprint != worldAt72, "World 2026 and the historical layer render together");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world2024-and-2026.png")), "both map legends remain visible at separate lower corners");
            SendMessage(historyToggle, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check(CaptureMapFingerprint(window) == worldAt72, "closing history preserves the World 2026 layer");
            SetFillOpacity(window, opacity, 0);
            Check(CaptureMapFingerprint(window) == thematicBaseFingerprint, "shared fill opacity also hides World 2026 at zero percent");
            SetFillOpacity(window, opacity, 72);
            Check(CaptureMapFingerprint(window) == worldAt72, "restoring opacity restores the unchanged World 2026 appearance");
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            SendMessage(population, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string populationAt72 = CaptureMapFingerprint(window);
            SetFillOpacity(window, opacity, 0);
            Check(CaptureMapFingerprint(window) == populationAt72, "fill opacity does not change population data");
            SetFillOpacity(window, opacity, 72);
            SendMessage(population, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            SendMessage(landCover, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string landCoverAt72 = CaptureMapFingerprint(window);
            SetFillOpacity(window, opacity, 0);
            Check(CaptureMapFingerprint(window) == landCoverAt72, "fill opacity does not change land-cover data");
            SetFillOpacity(window, opacity, 72);
            SendMessage(landCover, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-base-map-fine.png")), "thematic overlays can be disabled while the fine grid remains selected");
            string baseLayerFingerprint = CaptureMapFingerprint(window);
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string world2026Fingerprint = CaptureMapFingerprint(window);
            Check((int)SendMessage(world2026, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && world2026Fingerprint.Length > 0 && world2026Fingerprint != baseLayerFingerprint, "World 2026 opens as its own country-fill layer");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world2026-enabled.png")), "World 2026 country-fill screenshot captured at 627,200 cells");
            List<string> worldFrames = new List<string>();
            for (int frame = 0; frame < 5; frame++) worldFrames.Add(CaptureMapFingerprint(window));
            Check(worldFrames[0].Length > 0 && worldFrames.TrueForAll(hash => hash == worldFrames[0]), "World 2026 layer remains visually stable with no flashing");
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(world2026, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 0 && CaptureMapFingerprint(window) == baseLayerFingerprint, "World 2026 closes cleanly and restores the unchanged base map");
            SendMessage(landCover, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string landCoverOnlyFingerprint = CaptureMapFingerprint(window);
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(world2026, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && (int)SendMessage(landCover, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1, "World 2026 toggles independently alongside land cover");
            string combinedLandCoverFingerprint = CaptureMapFingerprint(window);
            Check(combinedLandCoverFingerprint.Length > 0 && combinedLandCoverFingerprint != landCoverOnlyFingerprint, "World 2026 adds its fill over the enabled land-cover layer");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world2026-with-landcover.png")), "World 2026 and land-cover layers render together");
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(landCover, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && CaptureMapFingerprint(window) == landCoverOnlyFingerprint, "closing World 2026 preserves the previous land-cover layer");
            SendMessage(landCover, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            SendMessage(population, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string populationOnlyFingerprint = CaptureMapFingerprint(window);
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            string combinedPopulationFingerprint = CaptureMapFingerprint(window);
            Check((int)SendMessage(world2026, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && (int)SendMessage(population, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && combinedPopulationFingerprint != populationOnlyFingerprint, "World 2026 adds its fill over the enabled population layer");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-world2026-with-population.png")), "World 2026 and population layers render together");
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check((int)SendMessage(population, BM_GETCHECK, IntPtr.Zero, IntPtr.Zero) == 1 && CaptureMapFingerprint(window) == populationOnlyFingerprint, "closing World 2026 preserves the previous population layer");
            SendMessage(population, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            SendMessage(hd, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(3500); WaitForRedraw(window);
            Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3, "Natural Earth high-resolution map retains the selected fine-grid level");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-natural-earth-hd-level-3.png")), "Natural Earth original high-resolution map screenshot captured");
            string hdMapFingerprint = CaptureMapFingerprint(window);
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check(CaptureMapFingerprint(window) != hdMapFingerprint && Capture(window, System.IO.Path.Combine(args[1], "ui-world2026-natural-earth-hd.png")), "World 2026 fill aligns with the high-resolution Natural Earth base map");
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check(CaptureMapFingerprint(window) == hdMapFingerprint, "World 2026 closes cleanly over the high-resolution base map");
            SendMessage(nasa, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(3500); WaitForRedraw(window);
            Check((int)SendMessage(combo, 0x0147, IntPtr.Zero, IntPtr.Zero) == 3, "NASA map retains the selected fine-grid level");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-nasa-level-3.png")), "NASA map screenshot captured with fine-grid level retained");
            string nasaMapFingerprint = CaptureMapFingerprint(window);
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check(CaptureMapFingerprint(window) != nasaMapFingerprint && Capture(window, System.IO.Path.Combine(args[1], "ui-world2026-nasa.png")), "World 2026 fill aligns with the NASA base map");
            SendMessage(world2026, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
            WaitForRedraw(window);
            Check(CaptureMapFingerprint(window) == nasaMapFingerprint, "World 2026 closes cleanly over the NASA base map");
            RECT windowedBounds; GetWindowRect(window, out windowedBounds);
            SendMessage(fullscreen, BM_CLICK, IntPtr.Zero, IntPtr.Zero); Thread.Sleep(350); WaitForRedraw(window);
            RECT fullscreenBounds; GetWindowRect(window, out fullscreenBounds);
            Check(!IsWindowVisible(fullscreen) && fullscreenBounds.Right-fullscreenBounds.Left>windowedBounds.Right-windowedBounds.Left && fullscreenBounds.Bottom-fullscreenBounds.Top>windowedBounds.Bottom-windowedBounds.Top, "fullscreen button hides controls and expands the map to the display");
            Check(Capture(window, System.IO.Path.Combine(args[1], "ui-map-fullscreen.png")), "fullscreen map screenshot captured");
            PostMessage(window, WM_KEYDOWN, (IntPtr)VK_ESCAPE, IntPtr.Zero); Thread.Sleep(350); WaitForRedraw(window);
            RECT restoredBounds; GetWindowRect(window, out restoredBounds);
            Check(IsWindowVisible(fullscreen) && Math.Abs((restoredBounds.Right-restoredBounds.Left)-(windowedBounds.Right-windowedBounds.Left))<8 && Math.Abs((restoredBounds.Bottom-restoredBounds.Top)-(windowedBounds.Bottom-windowedBounds.Top))<8, "Escape exits fullscreen and restores the previous window size");
            PostMessage(window, WM_KEYDOWN, (IntPtr)VK_F11, IntPtr.Zero); WaitForRedraw(window);
            Check(!IsWindowVisible(fullscreen), "F11 enters fullscreen from the keyboard");
            PostMessage(window, WM_KEYDOWN, (IntPtr)VK_ESCAPE, IntPtr.Zero); WaitForRedraw(window);
            Check(IsWindowVisible(fullscreen), "Escape exits fullscreen after keyboard activation");
        } catch (Exception ex) {
            Console.Error.WriteLine("UI smoke exception: " + ex.GetType().FullName + ": " + ex.Message);
            Console.Error.WriteLine(ex.StackTrace);
            failures++;
        } finally {
            if (!process.HasExited) { IntPtr hwnd = FindMainWindow((uint)process.Id); if (hwnd != IntPtr.Zero) PostMessage(hwnd, WM_CLOSE, IntPtr.Zero, IntPtr.Zero); if (!process.WaitForExit(10000)) process.Kill(); }
            process.Dispose();
        }
        Console.WriteLine("TOTAL: " + (checks - failures) + " passed, " + failures + " failed (" + checks + " checks)");
        return failures == 0 ? 0 : 1;
    }
}
