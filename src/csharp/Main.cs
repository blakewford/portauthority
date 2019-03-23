using System;
using System.Drawing;
using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using System.Xml;

public class Monitor: Form
{
    const Int32 SCREEN_WIDTH  = 960;
    const Int32 SCREEN_HEIGHT = 1080;
    const Int32 CHART_SIZE    = 480;

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

            wipe();
            Random Angle = new Random();
            addRange(Angle.Next(1, 50), Brushes.Red);
            addRange(Angle.Next(1, 50), Brushes.Blue);
        });

        WaitForTask(WorkThread);
    }

    public Monitor()
    {
        Text = "Monitor"; 
        Width  = SCREEN_WIDTH;
        Height = SCREEN_HEIGHT;
        FormBorderStyle = FormBorderStyle.FixedDialog;

        Int32 Buffer = 4;

        Button SwitchButton = new Button();
        SwitchButton.Text = "Switch";
        SwitchButton.Location = new Point(Buffer, Buffer);
        Controls.Add(SwitchButton);

        Path.Width = 512;
        Path.Location = new Point(80, Buffer);
        Controls.Add(Path);

        Chart.BackColor = Color.White;

        Int32 OffsetX = 4;
        Int32 OffsetY = 32;
        Chart.ClientSize = new Size(CHART_SIZE+OffsetX, CHART_SIZE+OffsetY);
        Chart.Padding = new System.Windows.Forms.Padding(OffsetX, OffsetY, 0, 0);
        Controls.Add(Chart);

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
    void initialize()
    {
    }

    void wipe()
    {
        PreviousFill = 0;
        Chart.Image = new Bitmap(CHART_SIZE,CHART_SIZE, PixelFormat.Format32bppPArgb);

        addRange(100, Brushes.Gray);
        PreviousFill = 0;
    }

    void addRange(Int32 Percentage, Brush Fill)
    { 
        GraphicsPath Path = new GraphicsPath();

        Int32 Angle = Convert.ToInt32(360.0*(Percentage/100.0));

        const Int32 MID_POINT = (CHART_SIZE/2);
        Path.AddLine(MID_POINT, MID_POINT, MID_POINT, MID_POINT);
        Path.AddArc(0, 0, CHART_SIZE, CHART_SIZE, 270 + PreviousFill, Angle);
        PreviousFill += Angle;

        Graphics.FromImage(Chart.Image).FillPath(Fill, Path);
    }
}
