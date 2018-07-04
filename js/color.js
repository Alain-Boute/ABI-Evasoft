// Color variables for current call
var colortd, colorinput, prev_color, prev_lum;

// Output hidden DIV for control display
document.write(
	"<div id=colorDIV style='border:1px solid black;background-color:#CCCCCC;position:absolute;visibility:hidden;z-index:2;'>" +
	"<table cellspacing=1 cellpadding=0>");

for(i = 0; i < 4; i++)
{
	// Output color sliders for RGB
	var c = i == 0 ? 'FF0000' : i == 1 ? '00FF00' : i == 2 ? '0000FF' : '888888';
	document.write(
		"<tr><td><img src=/img/cur_l.gif onclick=sliderChange("+i+",-1)></td>"+
		"<td bgcolor=#"+c+" onclick=sliderClick(event,this,"+i+") width=256 valign=top>" +
		"<div style='position:absolute'><img src=/img/curs.gif id=colcurs"+i+"></div></td>" +
		"<td><img src=/img/cur_r.gif onclick=sliderChange("+i+",1)></td></tr><tr><td height=2></td></tr>");
}
document.write(
	"<tr><td></td><td align=center><img src=/img/_eva_colortable.jpg onClick=colorClick(event,this)></td>"+
	"<td align=center><img src=/img/btn_ok.gif onclick=hideColor(0)><hr>"+
	"<img src=/img/_eva_cancel.gif onclick=hideColor(1)></tr></table></div>");
var colorDIV = document.getElementById("colorDIV")

// Clicks handling functions
function sliderChange(irgb, inc)
{
	var c = fromRGB();
	c[irgb] += inc;
	toRGB(c);
}
function sliderClick(event, elt, irgb)
{
	if (!event && window.event) event = window.event;
	var mpos = findPosition(elt);
	var c = fromRGB();
	c[irgb] = event.clientX - mpos[0];
	toRGB(c);
}
function colorClick(event, elt)
{
	if (!event && window.event) event = window.event;
	var mpos = findPosition(elt);
	var x = event.clientX - mpos[0], y = event.clientY - mpos[1];
	var c = new Array;
	if(y > 50)
	{
		// Gray scale click
		c[0] = Math.floor(Math.floor(x / elt.width * 26) / 25 * 256);
		c[1] = c[0]; c[2] = c[0];
	}
	else
	{
		// Color scale click
		c[0] = Math.floor((x % 50) / 10) * 64;
		c[1] = Math.floor(x / 50) * 64;
		c[2] = Math.floor(y / 10) * 64;
	}
	prev_lum = c[3] = LumfromRGB(c);
	toRGB(c);
}
function LumfromRGB(c)
{
	return Math.floor((c[0] + c[1] + c[2]) / 3);
}
// RGB convert functions
function fromRGB()
{
	var c = new Array, i, v = colorinput.value;
	c[3] = 0;
	for(i = 0; i < 3; i++) {c[i] = parseInt(v.substr(i * 2, 2), 16); c[3] += c[i];}
	c[3] = LumfromRGB(c);
	prev_lum = c[3];
	return c;
}
function toRGB(c)
{
	var i, v = "", h = "0123456789ABCDEF", inc = 0;
	
	if(!isNaN(c[3]) && c[3] != prev_lum) inc = c[3] - prev_lum;
	
	for(i = 0; i < 4; i++)
	{
		var curs = document.getElementById("colcurs" + i);
		var div = curs.parentNode;
		if(c)
		{
			if(inc) c[i] += inc;
			if(isNaN(c[i]) || c[i] < 0) c[i] = 0; else if(c[i] > 255) c[i] = 255;
			if(i != 3) v += h.charAt(Math.floor(c[i]/16)) + h.charAt(c[i]%16);
			else c[i] = LumfromRGB(c);
			div.style.visibility = 'visible';
			div.style.left = div.parentNode.offsetLeft + 1 + c[i] - Math.floor(curs.width / 2);
		}
		else
			div.style.visibility = 'hidden';
	}
	if(c)
	{
		colortd.style.backgroundImage = "url(/img/_eva_nop.gif)";
		colortd.bgColor = "#" + v;
	}
	else
	{
		colortd.style.backgroundImage = "url(/img/bg_rayures_grises.gif)";
		colortd.bgColor = "";
	}
	colorinput.value = v;
}

// Hide color control
function hideColor(cancel)
{
	if(cancel)
	{
		colorinput.value = prev_color;
		toRGB(prev_color == "" ? false : fromRGB());
	}
	var i;
	for(i = 0; i < 4; i++) document.getElementById("colcurs" + i).parentNode.style.visibility = "hidden";
	hideDIV();
}

// Main entry point
function showColor(coltd, name)
{
	colortd = coltd;
	colorinput = document.getElementsByName(name)[0];
	prev_color = colorinput.value;

	// Set color control position
	var mpos;
	mpos = findPosition(colortd);
    var x = mpos[0];
    var y = mpos[1] + colortd.offsetHeight;
	colorDIV.style.left = x;
	colorDIV.style.top = y;
	showDIV(colorDIV);
	
	// Set cursors position
	var c = colorinput.value == "" ? false : fromRGB();
	toRGB(c);
}
