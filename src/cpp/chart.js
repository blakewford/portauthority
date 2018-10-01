var max_width = 480;
var width = screen.width > max_width ? max_width: screen.width;

var chartCanvas = null;
var fillPercentage = 0;
var percentAdjustment = Math.PI/50;

function addRange(percentage, color)
{
    var context = chartCanvas.getContext("2d");
    context.fillStyle = color;

    context.beginPath();
    context.moveTo(width/2, width/2);

    var previousFill = fillPercentage*percentAdjustment;
    context.arc(width/2, width/2, width/2, previousFill, previousFill + (percentage*percentAdjustment));
    context.closePath();

    context.fill();

    fillPercentage += percentage;
}

function initialize()
{
    var chart = document.getElementById('chart');
    var canvas = document.createElement('canvas');

    canvas.width = width;
    canvas.height = width;

    chartCanvas = canvas;

    fillPercentage = 0;

    chart.appendChild(canvas);
}

function wipe()
{
    fillPercentage = 0;
    var context = chartCanvas.getContext("2d");
    context.clearRect(0, 0, chartCanvas.width, chartCanvas.height);  

    addRange(100, "gray");
    fillPercentage = 0;
}

initialize();
wipe();
