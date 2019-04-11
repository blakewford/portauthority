using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;
using System.Xml;

#pragma warning disable 618
public partial class Monitor
{
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

    void DrawChart(string Raw)
    {
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
        NodeList = Doc.GetElementsByTagName("div");
        foreach(XmlElement Element in NodeList)
        {
            for(Int32 Ndx = 0; Ndx < Element.Attributes.Count; Ndx++)
            {
                if(Element.Attributes[Ndx].Name.Equals("id") && Element.Attributes[Ndx].Value.Equals("coverage"))
                {
                    Coverage.Text = Element.FirstChild.InnerXml;
                    break;
                }
            }
        }
    }

    void Categorize()
    {
        if(!File.Exists(Path.Text))
        {
            MessageBox.Show("Invalid target binary selected!");
            return;
        }

        Thread WorkThread = new Thread(() => {

            string Raw = String.Empty;

            ProcessStartInfo Info = new ProcessStartInfo(GLOBAL.authority + "authority", "--report " + Path.Text);
            Info.WorkingDirectory = GLOBAL.authority;
            Info.UseShellExecute = false;
            Info.RedirectStandardOutput = true;

            Process Debug = Process.Start(Info);
            Debug.WaitForExit();

            Raw = Debug.StandardOutput.ReadToEnd();
            File.WriteAllText(GLOBAL.workspace +"/.portauth/"+"index.html", Raw);
            ListBox Navigator = (ListBox)Controls.Find("Navigator", true)[0];
            string Target = ((string)Navigator.Items[Navigator.SelectedIndex]).Trim();
            if(Directory.Exists(GLOBAL.workspace))
            {
                Directory.CreateDirectory(GLOBAL.workspace +"/.portauth/" + Target);
                File.WriteAllText(GLOBAL.workspace +"/.portauth/"+Target+"/index.html", Raw);
            }

            DrawChart(Raw);
        });

        WaitForTask(WorkThread);
    }

}
