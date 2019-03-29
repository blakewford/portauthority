using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

public partial class Monitor
{
    private Int32 PreviousFill = 0;

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
