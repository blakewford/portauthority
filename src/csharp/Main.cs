using System;
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

public class Monitor: Form
{
    Int32 SCREEN_WIDTH  = Convert.ToInt32(Screen.PrimaryScreen.Bounds.Width*.8);
    Int32 SCREEN_HEIGHT = Convert.ToInt32(Screen.PrimaryScreen.Bounds.Height*.9);
    const Int32 CHART_SIZE    = 480;
    const Int32 BUTTON_SIZE   = 48;

    struct Settings
    {
        public string authority;
        public string app;
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

    void ButtonClicked(object o, System.EventArgs e)
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
        Console.WriteLine(Cursor.Position.Y);
    }

    public Monitor()
    {
        Text = "Monitor"; 
        Width  = SCREEN_WIDTH;
        Height = SCREEN_HEIGHT;
        FormBorderStyle = FormBorderStyle.FixedDialog;

        Int32 BufferX = 448;
        Int32 BufferY = 4;

        Button SwitchButton = new Button();
        SwitchButton.Text = "Switch";
        SwitchButton.Location = new Point(BufferX, BufferY);
        Controls.Add(SwitchButton);

        Path.Width = 512;
        Path.Height = SwitchButton.Height;
        Path.Location = new Point(SwitchButton.Width + BufferX, BufferY);
        Controls.Add(Path);

        Int32 Count = SCREEN_HEIGHT/BUTTON_SIZE;
        Int32 Diff = SCREEN_HEIGHT - (Count*BUTTON_SIZE);

        while(Count-- > 0)
        {
            PictureBox ImageButton = new PictureBox();
            ImageButton.ClientSize = new Size(BUTTON_SIZE, BUTTON_SIZE*Count);
            ImageButton.Image = new Bitmap(BUTTON_SIZE, BUTTON_SIZE, PixelFormat.Format32bppPArgb);
            ImageButton.BackColor = Color.Gray;
            ImageButton.Click += ToolbarClicked;
            Controls.Add(ImageButton);
        }

        PictureBox Navigator = new PictureBox();
        Navigator.ClientSize = new Size(BUTTON_SIZE+384, SCREEN_HEIGHT-Diff-BUTTON_SIZE);
        Navigator.Image = new Bitmap(BUTTON_SIZE+384, SCREEN_HEIGHT-Diff-BUTTON_SIZE, PixelFormat.Format32bppPArgb);
        Navigator.BackColor = Color.Black;
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

        Graphics.FromImage(TrimBar.Image).Clear(Color.Blue);

        Control Window = new Control();
        Window.BackColor = Color.White;
        Window.ClientSize = new Size(SCREEN_WIDTH, SCREEN_HEIGHT);
        Controls.Add(Window);

        if(File.Exists("settings.json"))
        {
            var Deserializer = new JavaScriptSerializer();
            GLOBAL = Deserializer.Deserialize<Settings>(File.ReadAllText("settings.json"));
        }

        SwitchButton.Click += ButtonClicked;
        Application.ApplicationExit += OnExit;
        Path.Text = GLOBAL.app;

         // Create an empty MainMenu.
         MainMenu Main = new MainMenu();
         MenuItem FileItem = new MenuItem();

         FileItem.Text = "File";
         Main.MenuItems.Add(FileItem);
         Menu = Main;
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
