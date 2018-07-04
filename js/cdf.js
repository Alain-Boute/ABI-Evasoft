/*
 *  CHEMIN DE FER
 */

// Chercher un input nommé, ou le créer
function FindOrCreateInput(name)
{
	var e = document.getElementsByName(name)[0];
	if (e) return e;
	var e = document.createElement("input");
	e.setAttribute("name", name);
	e.setAttribute("type", "hidden");
	document.mainform.appendChild(e);
	return e;
}

// Edition activée?
var edit = (true == edit ? true : false);
// Page portée
var ePage = null;
// La touche ctrl est-elle pressée?
var ctrl = false;
// Si le clic est rapide, ouvrir la page
var open = false;
// Variable du timeout pour ouvrir
var oTO = null;

function PageDown(el, evt)
{
	if(ePage)
	{
		ePage.className = "Page";
	}
    if(evt.button > 0) return true;
	ePage = el;
    if(!edit) return true;
	el.className = "Page PageClick";
	HideHelp();
    clearTimeout(oTO);
    open = true;
    oTO = setTimeout("open = false", 600);
	return false;
}

function PageUp(el)
{
    if(ePage != null && el == ePage && (open || !edit))
    {
        OpenPage(ePage.id, parseInt(ePage.getAttribute("np")));
        return;
    }
	if(!edit || ePage == null || el == null || el == ePage)
	{
		if (ePage) ePage.className = "Page";
		ePage = null;
		return;
	}

    if ((ePage.id == 0 || el.id == 0) && ctrl)
    {
        // Intervertir lorsqu'une page est vide...
        // Page source
        var i = FindOrCreateInput("D"+pageSrc+"/"+cForm+"$"+cObj+"#PAGE_SRC$1");
        i.setAttribute("value", ePage.id == 0 ? el.id : ePage.id);
        // Position destination
        var i = FindOrCreateInput("D"+nPageDest+"/"+cForm+"$"+cObj+"#NUM_PAGE_DEST$1");
        i.setAttribute("value", ePage.id == 0 ?
            parseInt(ePage.getAttribute("np")) : parseInt(el.getAttribute("np")));
        // Déplacer une page
        document.mainform.JSINPUT.value="B"+depPage+"/"+cForm+"$"+cObj+"#";
        document.mainform.submit();
        ePage = null;
        return;
    }

	// Source
	var i = FindOrCreateInput("D"+pageSrc+"/"+cForm+"$"+cObj+"#PAGE_SRC$1");
	if (ePage.id > 0)
		i.setAttribute("value", ePage.id);
	else
		i.setAttribute("value", "");
	var i = FindOrCreateInput("D"+nPageSrc+"/"+cForm+"$"+cObj+"#NUM_PAGE_SRC$1");
	i.setAttribute("value", parseInt(ePage.getAttribute("np")));

	// Destination
	var i = FindOrCreateInput("D"+pageDest+"/"+cForm+"$"+cObj+"#PAGE_DEST$1");
	if (el.id > 0)
		i.setAttribute("value", el.id);
	else
		i.setAttribute("value", "");
	var i = FindOrCreateInput("D"+nPageDest+"/"+cForm+"$"+cObj+"#NUM_PAGE_DEST$1");
	i.setAttribute("value", parseInt(el.getAttribute("np")));

	if (ctrl)
	{
		// Intervertir deux pages
		document.mainform.JSINPUT.value="B"+intPage+"/"+cForm+"$"+cObj+"#";
	}
	else if (parseInt(ePage.getAttribute("np"))
	  < parseInt(el.getAttribute("np")))
	{
		// Déplacer après
		document.mainform.JSINPUT.value="B"+depApr+"/"+cForm+"$"+cObj+"#";
	}
	else
	{
		// Déplacer avant
		document.mainform.JSINPUT.value="B"+depAvt+"/"+cForm+"$"+cObj+"#";
	}
	document.mainform.submit();
	ePage = null;
}

function PageOver(el)
{
	if(!ePage) ShowHelp(el);
	el.onmouseout = function(){ PageOut(el); };
	if(!edit || ePage == null || ePage == el) return;
	el.className = "Page PageHover";
}

function PageOut(el)
{
	HideHelp();
	if(!edit || ePage == null || ePage == el) return;
	el.className = "Page";
}

// Créer ou ouvrir une page
function OpenPage(page, num)
{
	if(page==0)
	{
        if(!edit) return;
		var i = FindOrCreateInput("D"+numPage+"$#NUMERO_PAGE$1");
		i.setAttribute("value", num);
		document.mainform.JSINPUT.value="B"+nouvPage+"/"+cForm+"$"+cObj+"#";
		document.mainform.submit();
		return;
	}
	document.mainform.JSINPUT.value="B"+cForm+"$"+cObj+"#.OPENOBJ="+page+",0,0,0";
	document.mainform.submit();
}


document.onmouseup=function(){PageUp(null)}

document.onkeydown=function(e)
{
	if(window.event) // IE
	{
		keynum = e.keyCode;
	}
	else if(e.which) // Netscape/Firefox/Opera
	{
		keynum = e.which;
	}
	//if (keynum == 16) maj
	if (keynum == 17) ctrl = true;
	//if (keynum == 18) alt
}

document.onkeyup=function(e)
{
	if(window.event) // IE
	{
		keynum = e.keyCode;
	}
	else if(e.which) // Netscape/Firefox/Opera
	{
		keynum = e.which;
	}
	if (keynum == 17) ctrl = false;
}
