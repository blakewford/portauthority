using System;
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
    }

    protected void OnDeleteEvent(object sender, Gtk.DeleteEventArgs a)
    {
        Gtk.Application.Quit();
        a.RetVal = true;
    }

    const int GUI_PORT = 0xBFF;

    protected void OnRunClicked(object sender, EventArgs e)
    {
        try
        {
            byte[] receiveBuffer = new byte[1024];            
            byte[] sendBuffer = Encoding.ASCII.GetBytes("{}");

            IPHostEntry info = Dns.GetHostEntry(Dns.GetHostName());  
            IPAddress address = info.AddressList[0];  
            IPEndPoint endpoint = new IPEndPoint(address, GUI_PORT);

            Socket socket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
            socket.Connect(endpoint);
            int sent = socket.Send(sendBuffer);
            int received = socket.Receive(receiveBuffer);  
            string json = Encoding.ASCII.GetString(receiveBuffer, 0, received);

            new JavaScriptSerializer().Deserialize<Response>(json);
                                
            socket.Shutdown(SocketShutdown.Both);  
            socket.Close();

        }catch(Exception)
        {  
        }  
    }
}
