using System;
using System.Collections.Generic;
using System.Drawing;
using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using System.Xml;

#pragma warning disable 618
public class Monitor: Form
{
    Int32 SCREEN_WIDTH  = Convert.ToInt32(Screen.PrimaryScreen.Bounds.Width*.8);
    Int32 SCREEN_HEIGHT = Convert.ToInt32(Screen.PrimaryScreen.Bounds.Height*.9);
    const Int32 CHART_SIZE    = 480;
    const Int32 BUTTON_SIZE   = 48;

    const string UNIVERSAL_FONT = "Courier";

    struct Settings
    {
        public string authority;
        public string workspace;
        public string app;
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

    private Int32 PreviousFill = 0;

    private TextBox Path       = new TextBox();
    private PictureBox Chart   = new PictureBox();
    public static void Main(String[] Args)
    {
        Application.Run(new Monitor());
    }

    void OnExit(object o, System.EventArgs e)
    {
        GLOBAL.app = Path.Text;

        var Serializer = new JavaScriptSerializer();
        string Json = Serializer.Serialize(GLOBAL);
        File.WriteAllText("settings.json", Json);
    }

    void WaitForTask(Thread WorkThread)
    {
        if(Cursor == Cursors.WaitCursor)
            return;

        Cursor = Cursors.WaitCursor;

        WorkThread.Start();
        new Thread(() => {
            WorkThread.Join();
            Refresh();
            Invoke((MethodInvoker)delegate
            {
                Cursor = Cursors.Default;

                // Hack to update Cursor
                Point Position = System.Windows.Forms.Cursor.Position;
                System.Windows.Forms.Cursor.Position = Position;
            });
        }).Start();
    }

    void Categorize()
    {
        Thread WorkThread = new Thread(() => {

            string Raw = String.Empty;

            ProcessStartInfo Info = new ProcessStartInfo(GLOBAL.authority + "authority", "--report " + Path.Text);
            Info.WorkingDirectory = GLOBAL.authority;
            Info.UseShellExecute = false;
            Info.RedirectStandardOutput = true;

            Process Debug = Process.Start(Info);
            Debug.WaitForExit();

            Raw = Debug.StandardOutput.ReadToEnd();

            //Sanitize
            Raw = Raw.Replace("</br>", "<br></br>");

            XmlDocument Doc = new XmlDocument();
            Doc.LoadXml(Raw);

            XmlNodeList NodeList = Doc.GetElementsByTagName("script");
            foreach(XmlElement Element in NodeList)
            {
                string[] Lines = Element.InnerXml.Split(new char[]{'\n'});

                Int32 Count = 0;
                while(Count < Lines.Length)
                {
                    string Line = Lines[Count];
                    if(!Line.Equals(String.Empty) &&
                       !Line.StartsWith("var "))
                    {
                        if(Line.StartsWith("function "))
                        {
                            while(!Line.Contains("}"))
                            {
                                Count++;
                                Line = Lines[Count];
                            }
                        }
                        else
                        {
                            string[] Components = Line.Split('(');
                            string MethodName = Components[0];
                            string Definition = Components[1].Split(')')[0];

                            Object[] Objects = null;
                            MethodInfo Method = GetType().GetMethod(MethodName);
                            if(Line.Contains("()"))
                            {
                                Objects = new Object[0];
                            }
                            else
                            {
                                string[] Parameters = Definition.Split(',');

                                Int32 Order = 0;
                                Objects = new Object[Parameters.Length];
                                foreach(string Parameter in Parameters)
                                {
                                    string Processed = Parameter.Trim();
                                    if(Parameter.Contains("\""))
                                    {
                                        Processed = Processed.Replace("\"", "");
                                        Objects[Order] = Processed;
                                    }
                                    else
                                    {
                                        Objects[Order] = Int32.Parse(Processed);
                                    }
                                    Order++;
                                }
                            }
                            Method.Invoke(this, Objects);
                        }
                    }
                    Count++;
                }
            }
        });

        WaitForTask(WorkThread);
    }

    void ToolbarClicked(object o, System.EventArgs e)
    {
        Int32 VerticalPosition = PointToClient(Cursor.Position).Y;
        if(Menu == null)
        {
            if(VerticalPosition < BUTTON_SIZE)
            {
                // Create an empty MainMenu.
                MainMenu Main = new MainMenu();
                MenuItem FileItem = new MenuItem();   
                FileItem.Text = "File";
                Main.MenuItems.Add(FileItem);
                Menu = Main;
            }
        }
        else
        {
            Menu = null;
        }
    }

    void ListDoubleClick(object sender, EventArgs e)
    {
        ListBox Navigator = (ListBox)Controls.Find("Navigator", true)[0];
        Int32 Selected = Navigator.IndexFromPoint(PointToClient(Cursor.Position));
        if(Selected >= 0)
        {
            Console.WriteLine(((string)Navigator.Items[Selected]).Trim());
            Categorize();
        }
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

        BackColor = Default.WorkspaceColor;

        Text = "Monitor"; 
        Width  = SCREEN_WIDTH;
        Height = SCREEN_HEIGHT;
        FormBorderStyle = FormBorderStyle.FixedDialog;

        Int32 BufferX = 448;
        Int32 BufferY = 4;

        Path.Width = 512;
        Path.Font = new Font(UNIVERSAL_FONT, 15, FontStyle.Regular, GraphicsUnit.Pixel);
        Path.Location = new Point(BufferX, BufferY);
        Controls.Add(Path);

        Int32 Count = SCREEN_HEIGHT/BUTTON_SIZE;
        Int32 Diff = SCREEN_HEIGHT - (Count*BUTTON_SIZE);

        PictureBox ImageButton = new PictureBox();
        ImageButton.ClientSize = new Size(BUTTON_SIZE, BUTTON_SIZE*Count);
        ImageButton.Image = new Bitmap(BUTTON_SIZE, BUTTON_SIZE*Count, PixelFormat.Format32bppPArgb);
        Graphics Panel = Graphics.FromImage(ImageButton.Image);

        Bitmap[] Icons = new Bitmap[Count];
        Icons[0] = new Bitmap("icons/menu.png");

        Int32 Icon = 0;
        while(Icon < Count)
        {
            if(Icons[Icon] != null)
            {
                Panel.DrawImage(Icons[Icon], 0.0f, 48.0f*Icon, new RectangleF(0.0f, 0.0f, 48.0f, 48.0f), GraphicsUnit.Pixel);
            }
            Icon++;
        }

        ImageButton.BackColor = Default.ButtonColor;
        ImageButton.Click += ToolbarClicked;
        Controls.Add(ImageButton);

        ListBox Navigator = new ListBox();
        Navigator.BorderStyle = BorderStyle.None;
        Navigator.Font = new Font(UNIVERSAL_FONT, 24, FontStyle.Bold, GraphicsUnit.Pixel);

        string Buffer = String.Empty;
        Int32 Spaces = Convert.ToInt32(Math.Ceiling(BUTTON_SIZE/GetAutoScaleSize(Navigator.Font).Width));
        while(Spaces-- > 0)
        {
            Buffer += " ";
        }

        string[] Targets = GetTargetList(GLOBAL.workspace);
        foreach(string Target in Targets)
        {
            Navigator.Items.Add(Buffer + Target);
        }

        Navigator.DoubleClick += ListDoubleClick;
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
        Path.Text = GLOBAL.app;
    }

    // Chart API
    public void initialize()
    {
    }

    public void wipe()
    {
        PreviousFill = 0;
        Chart.Image = new Bitmap(CHART_SIZE,CHART_SIZE, PixelFormat.Format32bppPArgb);

        addRange(100, "gray");
        PreviousFill = 0;
    }

    public void addRange(Int32 Percentage, string Fill)
    {
        GraphicsPath Path = new GraphicsPath();

        Int32 Angle = Convert.ToInt32(Math.Ceiling(360.0*(Percentage/100.0)));

        const Int32 MID_POINT = (CHART_SIZE/2);
        Path.AddLine(MID_POINT, MID_POINT, MID_POINT, MID_POINT);
        Path.AddArc(0, 0, CHART_SIZE, CHART_SIZE, 270 + PreviousFill, Angle);
        PreviousFill += Angle;

        Graphics.FromImage(Chart.Image).FillPath(new SolidBrush(Color.FromName(Fill)), Path);
    }
}
