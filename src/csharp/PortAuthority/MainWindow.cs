using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Web.Script.Serialization;

using PortAuthority;


public partial class MainWindow : Gtk.Window
{
    public MainWindow() : base(Gtk.WindowType.Toplevel)
    {
        Build();
        Graph.ExposeEvent += OnExpose;
    }

    protected void OnDeleteEvent(object sender, Gtk.DeleteEventArgs a)
    {
        Gtk.Application.Quit();
        a.RetVal = true;
    }

    private const int GUI_PORT = 0xBFF;
    private const double ADJUSTMENT = Math.PI/50;

    private bool Initialized = false;

    private Dictionary<int, string> Ranges = new Dictionary<int, string>();

    protected void OnRunClicked(object sender, EventArgs e)
    {
        Process native = null;
        string path = Environment.GetEnvironmentVariable("AUTHORITY");

        byte[] receiveBuffer = new byte[1024];
        byte[] sendBuffer = Encoding.ASCII.GetBytes("{}");

        IPHostEntry info = Dns.GetHostEntry(Dns.GetHostName());
        IPAddress address = info.AddressList[0];
        IPEndPoint endpoint = new IPEndPoint(address, GUI_PORT);

        Socket socket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

        bool connected = false;
        while(!connected)
        {
            if(native == null || native.HasExited)
                native = Process.Start(path + "cluster-profile");

            try
            {
                socket.Connect(endpoint);
                connected = true;

            }catch(Exception)
            {
            }
        }

        int sent = socket.Send(sendBuffer);
        int received = socket.Receive(receiveBuffer);
        string json = Encoding.ASCII.GetString(receiveBuffer, 0, received);

        new JavaScriptSerializer().Deserialize<Response>(json);
                                
        socket.Shutdown(SocketShutdown.Both);
        socket.Close();

        native.WaitForExit();
        Graph.QueueDraw();
    }

    private void OnExpose(object sender, Gtk.ExposeEventArgs e)
    {
        if(!Initialized)
        {
            Initialize();
            return;
        }

        Wipe();
        AddRange(8, "red");
        AddRange(50, "blue");
    }

    private void Initialize()
    {
        Cairo.Context cr = Gdk.CairoHelper.Create(Graph.GdkWindow);

        int width = (int)(Allocation.Width * 0.8);
        int height = (int)(Allocation.Height * 0.8);
        cr.Translate(width/2, height/2);

        cr.Arc(0, 0, width/2, 0, 2 * Math.PI);

        cr.SetSourceColor(new Cairo.Color(Color.Gray.R/256.0, Color.Gray.G/256.0, Color.Gray.B/256.0));
        cr.Fill();

        ((IDisposable)cr.GetTarget()).Dispose();
        ((IDisposable)cr).Dispose();

        Initialized = true;
    }

    private void Wipe()
    {
        Ranges.Clear();
    }

    private void AddRange(int percentage, string color)
    {
        Cairo.Context cr = Gdk.CairoHelper.Create(Graph.GdkWindow);

        int width = (int)(Allocation.Width * 0.8);
        int height = (int)(Allocation.Height * 0.8);
        cr.Translate(width/2, height/2);

        // Initialize
        cr.Arc(0, 0, width/2, 0, 2 * Math.PI);
        cr.SetSourceColor(new Cairo.Color(Color.Gray.R/256.0, Color.Gray.G/256.0, Color.Gray.B/256.0));
        cr.Fill();

        Ranges.Add(percentage, color);

        int fillPercentage = 0;
        foreach(KeyValuePair<int, string> range in Ranges)
        {
            Color fillColor = Color.FromName(range.Value);
            cr.SetSourceColor(new Cairo.Color(fillColor.R/256.0, fillColor.G/256.0, fillColor.B/256.0));

            cr.NewPath();
            cr.LineTo(new Cairo.PointD(0.0, 0.0));

            var previousFill = fillPercentage * ADJUSTMENT;
            cr.Arc(0, 0, width/2, previousFill, previousFill + (range.Key * ADJUSTMENT));
            cr.ClosePath();
            cr.StrokePreserve();

            cr.Fill();
            fillPercentage += range.Key;
        }

        ((IDisposable)cr.GetTarget()).Dispose();
        ((IDisposable)cr).Dispose();
    }
}
