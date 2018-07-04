// Check if browser is MSIE
var comFRAME = null;

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
document.onmousemove = mouseTrack;

// Get screen position of an element
function findPosition(div)
{        
    var posX = 0, posY = 0;
    do
    {
        posX += div.offsetLeft;
        posY += div.offsetTop;
        div = div.offsetParent;
    }
    while( div != null );   
    var pos = [];
    pos[0] = posX;
    pos[1] = posY;
    
    return pos;
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
	var m = '';
	
	for(i = 0; i < l; i++)
	{
		var s1 = str.substring(i, i + 1);
		switch(s1)
		{
		case '\r': break;
		case '\n':
			if(s.length > lmax) lmax = s.length;
			if(s != '') p.appendChild(document.createTextNode(s));
			if(i <= l) m += "<br>";
			s = '';
			break;
		default:
			s = s + s1;
			m += s1;
		}
	}
	if(m != '')
	{
        if(s.length > lmax) lmax = s.length;
        /*
            Création de la table contenant la bulle d'aide
        */
		
		var x = mouseX;
		var ws = window.innerWidth - 30;
		var topLeft = "<img src='../img/helpText/tooltip-left-top.gif' />";
		var topRight = "<img src='../img/helpText/tooltip-right-top.gif' />";
		if(!ws) ws = document.body.scrollWidth;
		/*
			On regarde ou va se trouver la goutte d'eau
		*/
		if(lmax * 8 > document.body.scrollWidth * 45 / 100 ) lmax = document.body.scrollWidth * 45 / 800;
		/* Vérification que la largeur max de la bulle d'aide ne dépasse pas 45% de l'écran */
		if((x + lmax * 8) > ws)
		{
		/* 
			Si la longeur du texte + la position de la souris en x est plus grande que la dimention de l'écran
			alors la goutte sera à droite
		*/
			topRight = '<img src="../img/helpText/tooltip-right-drop.gif" />';
		}
		else
		{
			topLeft = '<img src="../img/helpText/tooltip-left-drop.gif" />';
		}
		
        var table;
        table = '<table id="evaTooltipPopUp" cellpadding="0px" cellspacing="0px">';
        table += '<tr>';
        table += '<td>' + topLeft +'</td>';
        table += '<td background=\'../img/helpText/tooltip-top.gif\';"></td>';
        table += '<td>' + topRight +'</td>';
        table += '</tr><tr>';
        table += '<td background=\'../img/helpText/tooltip-left-middle.gif\';"></td>';
        table += '<td bgcolor="white" class="fontTooltipPopUp">' + m +'</td>';
        table += '<td background=\'../img/helpText/tooltip-right-middle.gif\';"></td>';
        table += '</tr><tr>';
        table += '<td><img src="../img/helpText/tooltip-left-bottom.gif" /></td>';
        table += '<td background=\'../img/helpText/tooltip-bottom.gif\';"></td>';
        table += '<td><img src="../img/helpText/tooltip-right-bottom.gif" /></td>';
        table += '</tr></table>';
        
        p.innerHTML = table;
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
var helpmsg = null;
var mTrack = null;

// Hide popup help message
function HideHelp()
{
	// Restore original image if applicable
	if(imgSwap) imgSwap.src = imgSwap.getAttribute('img');
	imgSwap = null;

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
function ShowHelp(elt)
{
	if(mouseX==0 && mouseY==0) return;
	// Rollover : swap image
	elt.onmouseout = HideHelp;
	var num = elt.getAttribute("num");
	var img = num ? document.images['I'+num] : null;
	var imgsrc = img ? img.getAttribute("img1") : null;
	if(img && imgsrc) imgSwap = img;
	if(imgSwap) imgSwap.src = imgsrc;
		
	helpmsg = elt.getAttribute("title");
	if(elt.getAttribute("helpmsg") == "" || elt.getAttribute("helpmsg") == null)
	{
		elt.setAttribute("helpmsg", helpmsg);
	}
    elt.title = "";
	
    if(pHelp && elt == eHelp) { showDIV(pHelp); return; }
	hideDIV();
	if(elt == neHelp && (tHelp || pHelp == comDIV)) return;
	neHelp = elt;
	if(tHelp) self.clearTimeout(tHelp)
	tHelp = self.setTimeout(ShowpHelp,300);
}

// Build & show popup help
function ShowpHelp(e)
{
    var mpos = [mouseX , mouseY];
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
	if(!helpmsg || helpmsg == '') helpmsg = elt.getAttribute("alt");
	if(!helpmsg || helpmsg == '') helpmsg = elt.getAttribute("helpmsg");
	if(!helpmsg || helpmsg == '') return;
	elt.removeAttribute("title");
	elt.removeAttribute("alt");
	elt.setAttribute("helpmsg", helpmsg)
	
	// Create paragraph to display help message
    var p = document.createElement("div");
	var w = AddHelpText(p, helpmsg) * 8;
	if(w > document.body.scrollWidth * 45 / 100 ) w = document.body.scrollWidth * 45 / 100;

	// Calc message position
	var h = 8;
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
    if((x + w) > ws)
	{
		x -= w;
		p.align = "right";
	}

	// Set message style
    p.style.position = "absolute";
	p.style.top = y + 'px';
    p.style.left = x + 'px';
    p.style.width = w + 'px';
    p.style.padding = "2px";
	p.style.zIndex  = 2;

    /*
        Permet de faire disparaitre le block lorsqu'on bouge la souris
    */
    elt.onmouseout = function(){ hideDIV(); HideHelp();}
	p.onmouseout = function(){ hideDIV(); HideHelp();};
	p.onclick = function(){ hideDIV(); HideHelp();};
    
    document.getElementsByTagName("body")[0].appendChild(p);
    
    pHelp = p;
	showDIV(p);
    
	if(tHelp) self.clearTimeout(tHelp);
	tHelp = null;
}