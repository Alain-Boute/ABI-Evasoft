// Check if browser is MSIE
var b_msie = navigator.userAgent.indexOf('MSIE') >= 0;
var comFRAME = null;

// Output iframe to handle msie <select> bug
if(b_msie)
{
	document.write("<iframe  style='position:absolute;z-index:1;visibility:hidden;' id=comFRAME frameborder=0></iframe>");
	comFRAME = document.getElementById("comFRAME");
}

// Numbering images for mouseover event
var imgnum = 1;
var imgaction = '';
		
// Nop function for inactive links
function Nop(x){return(x)};

// Transform clicks in form submit
function cb(name)
{
	document.mainform.JSINPUT.value=name;
	document.forms[0].submit();
}

// Output action for a link
function ol(opobj,opform,form,obj)
{
	cb('B'+form+'$'+obj+'#.OPENOBJ='+opobj+','+opform);
}

// Output a button
function ob(href,img,img1,label,tooltip,border,mode,w,h)
{
	if(href != imgaction) imgnum++;
	imgaction = href;
	var num = imgnum;
	if(mode & 128) href = "javascript:ChkBoxClick('"+href+"',"+((mode & 256)!=0)+")";
	else if(!(mode & 64)) href = "javascript:cb('"+href+"')";
	document.write('<a href="'+href+'" onMouseOver=ShowHelp(this) title="'+tooltip+'" num='+num+
					(((mode & 64) && !(mode & 128)) ? ' target=_blank' : '')+' tabindex=10000>');
	if(img != '')
	{
		document.write('<img name=I'+num+' border='+border+' src="'+img+'" img="'+img+'" img1="'+img1+'" action="'+imgaction+'"');
		if(mode & 4) document.write(' align=absmiddle');
		if(w) document.write(' width='+w);
		if(h) document.write(' height='+h);
		document.write('>');
	}
	if(img != '' && mode & 8) document.write('<img src=\'../img/_eva_nop.gif\' width=3 border=0>');
	if(img == '' || mode & 8) document.write(label);
	document.write('</a>');
}

// Check handling
function ChkBoxClick(n,b3)
{
	var name = 'D'+n.substr(1,n.length-1);
	var v = document.getElementsByName(name)[0];
	var p = imgSwap.src.substr(0,imgSwap.src.indexOf('/_eva_checkbox')+15);
	var i;
	switch(v.value){
	case '0': v.value = ''; i = 'nul'; break;
	case '1': v.value = b3 ? 0 : ''; i = 'no'; break;
	default: v.value = 1; i = 'yes'; break;
	}
	imgSwap.src = p+i+'_s.gif';
	imgSwap.setAttribute("img1",imgSwap.src);
	imgSwap.setAttribute("img",p+i+'.gif');
}
function ChkLblClick(name,i,submit,toggle)
{
	var v = document.getElementsByName(name)[i];
	if(!toggle && v.checked) b_submit = 0;
	v.checked = toggle ? !v.checked : 1;
	if(submit) cb(name)
}

// Automatic identification
function Login(id, pwd)
{
	var elts = document.getElementsByTagName("input");
	for(var i = 0; i < elts.length; i++)
	{
		var e = elts[i];
		if(e.name.substring(0, 1) != 'D') continue;
		if(e.name.indexOf("_EVA_LOGINSUBMITP") >= 0) e.value=pwd;
		else if(e.name.indexOf("_EVA_LOGINSUBMIT") >= 0) e.value=id;
	}
	document.mainform.submit();
}

// Table row highlight
var HL_el = null, HL_el1 = null, HL_c, HL_o;
function RowHL(el)
{
	var op = el.getAttribute('opname');
	if(el.previousSibling && el.previousSibling.getAttribute('opname') == op) HL_el1 = el.previousSibling;
	else if(el.nextSibling && el.nextSibling.getAttribute('opname') == op) HL_el1 = el.nextSibling;
	HL_el = el; HL_c = el.bgColor; HL_o = op;
	el.style.cursor = "pointer";
	el.onmouseout = RowDL;
	el.onclick = RowCL;
	el.bgColor = "#FFDDDD";
	if(HL_el1) HL_el1.bgColor = "#FFDDDD";
	var elts = document.getElementsByTagName("img");
	op = op.substring(0, op.length - 3);
	for(var i = 0; i < elts.length; i++)
	{
		var e = elts[i];
		if(e.getAttribute('action') && e.getAttribute('action').substring(0, op.length) == op) { ShowHelp(e.parentNode); break; }
	}
}
function RowDL() { HL_el.bgColor = HL_c; if(HL_el1) HL_el1.bgColor = HL_c; HideHelp(); HL_el = null; HL_el1 = null; }
function RowCL() { cb(HL_o); }

// Track mouse position
var mouseX=0,mouseY=0;
function mouseTrack(e) {
if(!e) e = window.event;
if (e.pageX) {mouseX=e.pageX;mouseY=e.pageY;}
else if (e.clientX){
   mouseX = e.clientX + (document.documentElement.scrollLeft ? document.documentElement.scrollLeft : document.body.scrollLeft);
   mouseY = e.clientY + (document.documentElement.scrollTop ? document.documentElement.scrollTop : document.body.scrollTop);
}}
document.onmousemove=mouseTrack;
function mousePos() {return [mouseX,mouseY];}

// Get screen position of an element
function findPosition(elt)
{
	if(elt.offsetParent)
	{
		var x; var y;
		for(x = 0, y = 0; elt.offsetParent; elt = elt.offsetParent)
		{
			x += elt.offsetLeft;
			y += elt.offsetTop;
		}
		return [ x, y ];
	}
	else
	{
		return [ elt.x, elt.y ];
	}
}

// Gestion affichage DIV unique entre les modules
var comDIV = null;
function showDIV(div)
{
	hideDIV();

	comDIV = div;
	if(comFRAME)
	{
		comFRAME.style.left = comDIV.style.left;
		comFRAME.style.top = comDIV.style.top;
		comFRAME.style.height = comDIV.offsetHeight;
		comFRAME.style.width = comDIV.offsetWidth;
		comFRAME.style.visibility = "visible";
	}
	if(comDIV) comDIV.style.visibility = 'visible';
}
function hideDIV()
{
	if(comDIV) comDIV.style.visibility = 'hidden';
	if(comFRAME) comFRAME.style.visibility = 'hidden';
	comDIV = null;
}

// Handle newlines in help text
function AddHelpText(p, str)
{
	var l = str.length;
	var lmax = 0;
	var s = '';
	for(i = 0; i < l; i++)
	{
		var s1 = str.substring(i, i + 1);
		switch(s1)
		{
		case '\r': break;
		case '\n':
			if(s.length > lmax) lmax = s.length;
			if(s != '') p.appendChild(document.createTextNode(s));
			if(i <= l) p.appendChild(document.createElement("br"));
			s = '';
			break;

		default:
			s = s + s1;
		}
	}
	if(s != '')
	{
		if(s.length > lmax) lmax = s.length;
		p.appendChild(document.createTextNode(s));
	}
	return lmax;
}

// Toggle menu contents visibility
function ToggleMenu(img,id)
{
	var elt = document.getElementById('Mnu'+id);
	var vis = elt.style.display == "none";
	var ud = vis ? "up" : "down";
	img.src = "/img/shrink_"+ud+"_s.gif";
	img.setAttribute("img1",img.src);
	img.setAttribute("img","/img/shrink_"+ud+".gif");
	img.title = vis ? "Cliquez pour réduire" : "Cliquez pour afficher";
	elt.style.display = vis ? "" : "none";
}

// Global variable to hold element used for help display
var pHelp = null;
var eHelp = null;
var neHelp = null;
var imgSwap = null;
var tHelp = null;

// Hide popup help message
function HideHelp(b_all)
{
	// Restore original image if applicable
	if(imgSwap) imgSwap.src = imgSwap.getAttribute('img');
	imgSwap = null;
	if(b_msie && !b_all) return;

	// Hide help text
	neHelp = null;
    hideDIV();
}

// Show rollover for an image
function SwapImg(img)
{
	var imgsrc = img ? img.getAttribute("img1") : null;
	if(img && imgsrc) imgSwap = img;
	if(imgSwap) imgSwap.src = imgsrc;
	img.onmouseout = HideHelp;
}

// Show help & rollover for an element
function ShowHelp(elt,b_all)
{
	// Rollover : swap image
	var num = elt.getAttribute("num");
	var img = num ? document.images['I'+num] : null;
	var imgsrc = img ? img.getAttribute("img1") : null;
	if(img && imgsrc) imgSwap = img;
	if(imgSwap) imgSwap.src = imgsrc;
	elt.onmouseout = HideHelp;

	// Display popup help as applicable - use timer
	if(b_msie && !b_all) return;
	if(pHelp && elt == eHelp) { showDIV(pHelp); return; }
	hideDIV();
	if(elt == neHelp && (tHelp || pHelp == comDIV)) return;
	neHelp = elt;
	if(tHelp) self.clearTimeout(tHelp);
	tHelp = self.setTimeout(ShowpHelp,300);
}

// Build & show popup help
function ShowpHelp(e)
{
	var mpos = mousePos(e)
	var elt = neHelp;
	if(!elt) return;

	// Remove existing popup
    if (pHelp)
	{
		hideDIV();
		document.getElementsByTagName("body")[0].removeChild(pHelp);
		pHelp = null;
	}

	// Get help message text
	helpmsg = elt.getAttribute("title");
	if(!helpmsg || helpmsg == '') helpmsg = elt.getAttribute("alt");
	if(!helpmsg || helpmsg == '') helpmsg = elt.getAttribute("helpmsg");
	if(!helpmsg || helpmsg == '') return;
	elt.removeAttribute("title");
	elt.removeAttribute("alt");
	elt.setAttribute("helpmsg", helpmsg)
    
	// Create paragraph to display help message
    var p = document.createElement("div");
	var w = AddHelpText(p, helpmsg) * 7;
	if(w > 400) w = 400;

	// Calc message position
	var h = 20;
	if(mpos == null)
	{
		h = elt.offsetHeight;
		if(h == undefined) h = elt.offsetParent.offsetHeight;
		mpos = findPosition(elt);
	}
    var x = mpos[0];
    var y = mpos[1] + h;
	var ws = window.innerWidth - 30;
	if(!ws) ws = document.body.scrollWidth;
    if((x + w) > ws) x = ws - w;

	// Set message style
    p.style.position = "absolute";
    p.style.top = y + 'px';
    p.style.left = x + 'px';
    p.style.width = w + 'px';
    p.style.background = "#FFFFE0";
    p.style.padding = "2px";
    p.style.border = "1px solid";
	p.style.font = "8pt Verdana, sans-serif";
	p.style.zIndex  = 2;
	p.onmouseout = HideHelp;
	p.onclick = HideHelp;
    
    document.getElementsByTagName("body")[0].appendChild(p);
    
    pHelp = p;
	showDIV(p);
	if(tHelp) self.clearTimeout(tHelp);
	tHelp = null;
}
