using System;
using System.Collections.Generic;
using System.Windows.Forms;

#pragma warning disable 618
public partial class Monitor
{
    Dictionary<string, string> ApplicationMap = new Dictionary<string, string>();

    void LinkedEvent(object Sender, EventArgs e)
    {
        OpenFileDialog Dialog = new OpenFileDialog();
        DialogResult Result = Dialog.ShowDialog();
        if(Result == DialogResult.OK)
        {
            string LinkedControl = ((Button)Sender).Name.Split(':')[1];
            TextBox Box = (TextBox)Controls.Find(LinkedControl, true)[0];
            Box.Text = Dialog.FileName;

            ListBox Navigator = (ListBox)Controls.Find("Navigator", true)[0];
            ApplicationMap[((string)Navigator.Items[Navigator.SelectedIndex]).Trim()] = Box.Text;
        }
    }

    void LinkItems(Control[] Controls, string Tag)
    {
        Int32 Index = 0;
        while(Index < Controls.Length-1)
        {
            Controls[Index].Click += LinkedEvent;
            Controls[Index].Name = ":" + Tag;
            Index++;
        }

        Controls[Index].Name = Tag;
    }

    void AddTargets(ListBox Ctrl)
    {
        string Buffer = String.Empty;
        Int32 Spaces = Convert.ToInt32(Math.Ceiling(BUTTON_SIZE/GetAutoScaleSize(Ctrl.Font).Width));
        while(Spaces-- > 0)
        {
            Buffer += " ";
        }

        string[] Targets = GetTargetList(GLOBAL.workspace);
        foreach(string Target in Targets)
        {
            Ctrl.Items.Add(Buffer + Target);
            if(!ApplicationMap.ContainsKey(Target))
            {
                ApplicationMap.Add(Target, "");
            }
        } 
    }

}
