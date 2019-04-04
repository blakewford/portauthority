using System;
using System.Collections.Generic;
using System.Drawing;
using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;

public partial class Monitor: Form
{
    Int32 SCREEN_WIDTH  = Convert.ToInt32(Screen.PrimaryScreen.Bounds.Width*.8);
    Int32 SCREEN_HEIGHT = Convert.ToInt32(Screen.PrimaryScreen.Bounds.Height*.9);
    const Int32 CHART_SIZE    = 480;
    const Int32 BUTTON_SIZE   = 48;

    const string UNIVERSAL_FONT = "Courier";

    struct Assignment
    {
        public string name;
        public string target;
    }

    struct Project
    {
        public Assignment[] assignments;
    }

    struct Settings
    {
        public string authority;
        public string workspace;
    }

    struct Theme
    {
        public Theme(Color ButtonPanel, Color NavigatorPanel, Color Workspace, Color Status)
        {
            ButtonColor = ButtonPanel;
            NavigatorColor = NavigatorPanel;
            WorkspaceColor = Workspace;
            StatusColor = Status;
        }

        public Color ButtonColor;
        public Color NavigatorColor;
        public Color WorkspaceColor;
        public Color StatusColor;
    }

    Settings GLOBAL;
    Project PROJECT;

    private TextBox Path       = new TextBox();
    private PictureBox Chart   = new PictureBox();
    public static void Main(String[] Args)
    {
        Application.Run(new Monitor());
    }

    void OnExit(object o, System.EventArgs e)
    {
        Int32 Count = 0;
        PROJECT.assignments = new Assignment[ApplicationMap.Count];
        foreach(var assignment in ApplicationMap)
        {
            PROJECT.assignments[Count].name = assignment.Key;
            PROJECT.assignments[Count].target = assignment.Value;
            Count++;
        }

        var Serializer = new JavaScriptSerializer();
        string Json = Serializer.Serialize(GLOBAL);
        File.WriteAllText("settings.json", Json);

        if(Directory.Exists(GLOBAL.workspace))
        {
            Directory.CreateDirectory(GLOBAL.workspace +"//.portauth");
            string ProjectJson = Serializer.Serialize(PROJECT);
            File.WriteAllText(GLOBAL.workspace +"//.portauth//"+"settings.json", ProjectJson);
        }
    }

    void ToolbarClicked(object Object, System.EventArgs e)
    {
        PictureBox Toolbar = (PictureBox)Controls.Find("Toolbar", true)[0];
        Graphics Panel = Graphics.FromImage(Toolbar.Image);

        Int32 VerticalPosition = PointToClient(Cursor.Position).Y;
        if(Menu == null)
        {
            if(VerticalPosition < BUTTON_SIZE)
            {
                // Create an empty MainMenu.
                MainMenu Main = new MainMenu();

                MenuItem FileItem = new MenuItem("File");
                MenuItem Workspace = new MenuItem("Open Workspace");
                Workspace.Click += delegate(object Item, EventArgs Args)
                {
                    FolderBrowserDialog Dialog = new FolderBrowserDialog();
                    DialogResult Result = Dialog.ShowDialog();
                    if(Result == DialogResult.OK)
                    {
                        OnExit(null, null);
                        GLOBAL.workspace = Dialog.SelectedPath;

                        if(File.Exists(GLOBAL.workspace + "//.portauth//" + "settings.json"))
                        {
                            var Deserializer = new JavaScriptSerializer();
                            PROJECT = Deserializer.Deserialize<Project>(File.ReadAllText(GLOBAL.workspace + "//.portauth//" + "settings.json"));
                            ListBox Navigator = (ListBox)Controls.Find("Navigator", true)[0];
                            Navigator.Items.Clear();
                            Path.Clear();
                            wipe();
                            AddTargets(Navigator);
                            ApplicationMap.Clear();
                            foreach(Assignment A in PROJECT.assignments)
                            {
                                ApplicationMap.Add(A.name, A.target);
                            }
                        }
                    }
                };

                FileItem.MenuItems.Add(Workspace);

                MenuItem ExitItem = new MenuItem("Exit");
                ExitItem.Click += delegate(object Item, EventArgs Args){ Close(); };
                FileItem.MenuItems.Add(ExitItem);
                Main.MenuItems.Add(FileItem);

                MenuItem Edit = new MenuItem("Edit");
                MenuItem Set = new MenuItem("Set Target Binary");
                Set.Name = ":Path";
                Set.Click += LinkedEvent;
                Edit.MenuItems.Add(Set);
                Main.MenuItems.Add(Edit);

                MenuItem Attach = new MenuItem("Attach to Running Process");
                MenuItem Start = new MenuItem("Start Profile");
                MenuItem Profile = new MenuItem("Profile");
                Profile.MenuItems.Add(Attach);
                Profile.MenuItems.Add(Start);
                Start.Click += delegate(object Item, EventArgs Args){ Categorize(); };
                Main.MenuItems.Add(Profile);

                Menu = Main;
            }
        }
        else
        {
            Menu = null;
            Panel.DrawImage(new Bitmap("icons/selected.png"), 0.0f, BUTTON_SIZE*1, new RectangleF(0.0f, 0.0f, BUTTON_SIZE, BUTTON_SIZE), GraphicsUnit.Pixel);
            Panel.DrawImage(new Bitmap("icons/category.png"), 0.0f, BUTTON_SIZE*1, new RectangleF(0.0f, 0.0f, BUTTON_SIZE, BUTTON_SIZE), GraphicsUnit.Pixel);
        }

        Refresh();
    }

    void ListDoubleClick(object Sender, EventArgs e)
    {
        ListBox Navigator = (ListBox)Controls.Find("Navigator", true)[0];
        Int32 Selected = Navigator.IndexFromPoint(PointToClient(Cursor.Position));
        if(Selected >= 0)
        {
            Categorize();
        }
    }

    void TargetSelectionChanged(object Sender, EventArgs e)
    {
        ListBox Navigator = (ListBox)Controls.Find("Navigator", true)[0];
        string Target = ApplicationMap[((string)Navigator.Items[Navigator.SelectedIndex]).Trim()];
        Path.Text = Target;
    }

    private static string[] GetTargetList(string WorkingDirectory)
    {
        List<string> Targets = new List<string>();
        ProcessStartInfo Info = new ProcessStartInfo("make", "-rRqp");
        Info.WorkingDirectory = WorkingDirectory;
        Info.UseShellExecute = false;
        Info.RedirectStandardOutput = true;

        Process Debug = Process.Start(Info);
        Debug.WaitForExit();

        string Database = Debug.StandardOutput.ReadToEnd();
        string[] Lines = Database.Split(new char[]{'\n'});

        Int32 Current = 0;
        while(Current < Lines.Length)
        {
            string Line = Lines[Current];
            if(Line.Contains("# automatic") || Line.Contains("# environment") || Line.Contains("# default") || Line.Contains("# makefile"))
            {
                Current++; // Skip variables
            }
            else if(Line.Contains("# Not a target"))
            {
                Current++;
            }
            else if(Line.StartsWith("#"))
            {
                Current++; // Comment
            }
            else if(Line.StartsWith("\t"))
            {
                Current++; // Comment
            }
            else
            {
                string Target = Line.Trim();
                if(Target != String.Empty)
                {
                    Targets.Add(Target.Split(new char[]{':'})[0]);
                }
            }
            Current++;
        }

        Targets.Remove("clean"); // Make this list selectable, 'x' in UI?
        Targets.Sort();

        return Targets.ToArray();
    }

    public Monitor()
    {
        Theme Default =
            new Theme
            (
                System.Drawing.ColorTranslator.FromHtml("#363636"),
                System.Drawing.ColorTranslator.FromHtml("#2A292B"),
                System.Drawing.ColorTranslator.FromHtml("#232323"), 
                Color.Blue
            );

        if(File.Exists("settings.json"))
        {
            var Deserializer = new JavaScriptSerializer();
            GLOBAL = Deserializer.Deserialize<Settings>(File.ReadAllText("settings.json"));
        }

        if(File.Exists(GLOBAL.workspace + "//.portauth//" + "settings.json"))
        {
            var Deserializer = new JavaScriptSerializer();
            PROJECT = Deserializer.Deserialize<Project>(File.ReadAllText(GLOBAL.workspace + "//.portauth//" + "settings.json"));
        }

        BackColor = Default.WorkspaceColor;

        Text = "Monitor"; 
        Width  = SCREEN_WIDTH;
        Height = SCREEN_HEIGHT;
        FormBorderStyle = FormBorderStyle.FixedDialog;

        Int32 BufferX = 448;
        Int32 BufferY = 4;

        Button FileButton = new Button();
        FileButton.Width = BUTTON_SIZE;
        FileButton.Location = new Point(BufferX, BufferY);
        FileButton.BackColor = Color.LightGray;
        Controls.Add(FileButton);

        Path.Width = 512;
        Path.Font = new Font(UNIVERSAL_FONT, 16, FontStyle.Regular, GraphicsUnit.Pixel);
        Path.Location = new Point(FileButton.Width + BufferX, BufferY);
        Path.BorderStyle = BorderStyle.None;
        Controls.Add(Path);

        LinkItems(new Control[]{FileButton, Path}, "Path");

        Int32 Count = SCREEN_HEIGHT/BUTTON_SIZE;
        Int32 Diff = SCREEN_HEIGHT - (Count*BUTTON_SIZE);

        PictureBox ImageButton = new PictureBox();
        ImageButton.Name = "Toolbar";
        ImageButton.ClientSize = new Size(BUTTON_SIZE, BUTTON_SIZE*Count);
        ImageButton.Image = new Bitmap(BUTTON_SIZE, BUTTON_SIZE*Count, PixelFormat.Format32bppPArgb);
        Graphics Panel = Graphics.FromImage(ImageButton.Image);

        Bitmap[] Icons = new Bitmap[Count];
        Icons[0] = new Bitmap("icons/menu.png");
        Icons[1] = new Bitmap("icons/category.png");
        Icons[2] = new Bitmap("icons/green.png");
        Icons[3] = new Bitmap("icons/coverage.png");

        Int32 Icon = 0;
        while(Icon < Count)
        {
            if(Icons[Icon] != null)
            {
                Panel.DrawImage(Icons[Icon], 0.0f, BUTTON_SIZE*Icon, new RectangleF(0.0f, 0.0f, BUTTON_SIZE, BUTTON_SIZE), GraphicsUnit.Pixel);
            }
            Icon++;
        }

        ImageButton.BackColor = Default.ButtonColor;
        ImageButton.Click += ToolbarClicked;
        Controls.Add(ImageButton);

        ListBox Navigator = new ListBox();
        Navigator.BorderStyle = BorderStyle.None;
        Navigator.Font = new Font(UNIVERSAL_FONT, 24, FontStyle.Bold, GraphicsUnit.Pixel);

        AddTargets(Navigator);

        if(PROJECT.assignments != null)
        {
            foreach(var assignment in PROJECT.assignments)
            {
                if(ApplicationMap.ContainsKey(assignment.name))
                {
                    ApplicationMap[assignment.name] = assignment.target;
                }
            }
        }
        else
        {
            PROJECT.assignments = new Assignment[0];
        }

        Navigator.DoubleClick += ListDoubleClick;
        Navigator.SelectedIndexChanged += TargetSelectionChanged;
        Navigator.ClientSize = new Size(BUTTON_SIZE+384, SCREEN_HEIGHT-Diff-BUTTON_SIZE);
        Navigator.BackColor = Default.NavigatorColor;
        Navigator.Name = "Navigator";
        Controls.Add(Navigator);

        Int32 OffsetX = BufferX;
        Int32 OffsetY = 32;
        Chart.ClientSize = new Size(CHART_SIZE+OffsetX, CHART_SIZE+OffsetY);
        Chart.Padding = new System.Windows.Forms.Padding(OffsetX, OffsetY, 0, 0);
        Controls.Add(Chart);

        PictureBox TrimBar = new PictureBox();
        TrimBar.ClientSize = new Size(SCREEN_WIDTH, SCREEN_HEIGHT);
        TrimBar.Image = new Bitmap(SCREEN_WIDTH, BUTTON_SIZE, PixelFormat.Format32bppPArgb);
        TrimBar.Padding = new System.Windows.Forms.Padding(0, SCREEN_HEIGHT-Diff-BUTTON_SIZE, 0, 0);
        Controls.Add(TrimBar);

        Graphics.FromImage(TrimBar.Image).Clear(Default.StatusColor);

        Control Window = new Control();
        Window.BackColor = Color.White;
        Window.ClientSize = new Size(SCREEN_WIDTH, SCREEN_HEIGHT);
        Controls.Add(Window);

        Application.ApplicationExit += OnExit;

        wipe();

        Panel.DrawImage(new Bitmap("icons/selected.png"), 0.0f, BUTTON_SIZE*1, new RectangleF(0.0f, 0.0f, BUTTON_SIZE, BUTTON_SIZE), GraphicsUnit.Pixel);
        Panel.DrawImage(new Bitmap("icons/category.png"), 0.0f, BUTTON_SIZE*1, new RectangleF(0.0f, 0.0f, BUTTON_SIZE, BUTTON_SIZE), GraphicsUnit.Pixel);
    }
}
