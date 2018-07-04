/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : html_basic.c
** Description : these functions output HTML code for basic elements
**      Author : Alain BOUTE
**     Created : Feb 26 2002
**********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : put_html_table_header
** Description : output HTML header for a new table
*********************************************************************/
#define ERR_FUNCTION "put_html_table_header"
#define ERR_CLEANUP M_FREE(img)
int put_html_table_header(
	EVA_context *cntxt,				/* in/out : execution context data */
	char *align,					/* in : horizontal position (center / left / right) */
	char *width,					/* in : widht in pixels or % of window width */
	char *height,					/* in : widht in pixels or % of window width */
	char *bgcolor,					/* in : background color (RGB / 6 chars hexa) */
	char *background,				/* in : background image filename (no path) */
	int cellspacing,				/* in : cell spacing in pixels */
	int cellpadding,				/* in : cell padding in pixels */
	int border,						/* in : border width in pixels */
	char *rules,					/* in : cells border drawing rule */
	char *cls,						/* in : CSS class */
	char *extra						/* in : extra attributes for <table> tag */
){
	char *img = NULL;
	CHECK_HTML_STATUS;

	/* Output HTML table header with modifiers */
	DYNBUF_ADD_STR(html, "<table");
	if(cls  && !*cls) cls = NULL;
	DYNBUF_ADD3_INT(html," cellspacing=", cellspacing, "");
	if(!cls || cellpadding) DYNBUF_ADD3_INT(html, " cellpadding=", cellpadding, "");
	if(!cls || border) DYNBUF_ADD3_INT(html, " border=", border, "");
	if(!cls && !border && !rules) rules = "none";
	if(align && *align) DYNBUF_ADD3(html, " align=", align, 0, NO_CONV, "");
	if(width && *width) DYNBUF_ADD3(html, " width=", width, 0, NO_CONV, "");
	if(height && *height) DYNBUF_ADD3(html, " height=", height, 0, NO_CONV, "");
	if(bgcolor && *bgcolor && strlen(bgcolor) == 6) DYNBUF_ADD3(html, " bgcolor=#", bgcolor, 0, NO_CONV, "");
	if(rules && *rules) DYNBUF_ADD3(html, " rules=", rules, 0, NO_CONV, "");
	if(cls) DYNBUF_ADD3(html, " class=", cls, 0, NO_CONV, "");
	if(extra && *extra) DYNBUF_ADD3(html, " ", extra, 0, NO_CONV, "");
	img = get_image_file(cntxt, background, 0, 0);
	if(img) DYNBUF_ADD3(html, " background='", img, 0, NO_CONV, "'");
	DYNBUF_ADD_STR(html, "><tr>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_html_format_pos
** Description : output HTML table line / column header & text format
*********************************************************************/
#define ERR_FUNCTION "put_html_format_pos"
#define ERR_CLEANUP M_FREE(img)
int put_html_format_pos(
	EVA_context *cntxt,				/* in/out : execution context data */
	char *position,					/* in : position in table (NewLine, NewColumn, SameCell, ...) */
	char *align,					/* in : horizontal position (center / left / right) */
	char *valign,					/* in : verical position (middle / top / left) */
	char *width,					/* in : widht in pixels or % of window width */
	char *height,					/* in : height in pixels or % window height */
	char *bgcolor,					/* in : background color (RGB / 6 chars hexa) */
	char *background,				/* in : background image filename (no path) */
	int colspan, int rowspan,		/* in : # of row & columns the cell (if any) will occupy */
	char *fontface,					/* in : font face (a font name) */
	char *fontsize,					/* in : font size may be absolute (size) or relative (+size / -size) */
	char *fontcolor,				/* in : font color (RGB / 6 chars hexa) */
	char *style,					/* in : CSS class style */
	int bold,						/* in : use bold text if 1 */
	int italic,						/* in : use italic text if 1 */
	int underline,					/* in : use underlined text if 1 */
	int nobreak,					/* in : do not break lines if 1 */
	int b_head						/* in : output header if 1, footer else */
){
	char *tag = NULL;
	char *img = NULL;
	int b_font = (fontface && *fontface) || (fontsize && *fontsize) || (fontcolor && *fontcolor);
	CHECK_HTML_STATUS;

	/* Check parameters */
	if(!position) position = "";

	/* Handle position in table */
	if(!strcmp("_EVA_NewHeader", position))
	{
		/* NewColumn : new table cell */
		tag = "th";
	}
	else 	if(!strcmp("_EVA_NewColumn", position))
	{
		/* NewColumn : new table cell */
		tag = "td";
	}
	else if(!strcmp("_EVA_SameCell", position))
	{
		/* SameCell : use <span> if style given */
		if(style && *style)
		{
			tag = "span";
		}
	}
	else if(!strcmp("_EVA_SameColumn", position))
	{
		/* SameColumn : use <div> */
		tag = "div";
	}
	else
	{
		/* Default value : new table row */
		if(b_head)
		{
			char *txt = *html ? (*html)->data : NULL;
			size_t len = *html ? (*html)->cnt : 0;
			if(txt && len > 3 && strncmp(txt + len - 4, "<tr>", 4))
				DYNBUF_ADD_STR(html, "</tr><tr>");
		}
		tag = "td";
	}

	/* Output header */
	if(b_head)
	{
		if(tag)
		{
			/* Add cell modifiers */
			DYNBUF_ADD3(html, "<", tag, 0, NO_CONV, "");
			if(style && *style) DYNBUF_ADD3(html, " class='", style, 0, HTML_NO_QUOTE, "'");
			if(align && *align) DYNBUF_ADD3(html, " align=", align, 0, NO_CONV, "");
			if(valign && *valign) DYNBUF_ADD3(html, " valign=", valign, 0, NO_CONV, "");
			if(width && *width) DYNBUF_ADD3(html, " width=", width, 0, NO_CONV, "");
			if(height && *height) DYNBUF_ADD3(html, " height=", height, 0, NO_CONV, "");
			if(bgcolor && strlen(bgcolor) == 6) DYNBUF_ADD3(html, " bgcolor=#", bgcolor, 0, NO_CONV, "");
			img = get_image_file(cntxt, background, 0, 0);
			if(img) DYNBUF_ADD3(html, " background='", img, 0, NO_CONV, "'");
			if(colspan > 1) DYNBUF_ADD3_INT(html, " colspan=", colspan, "");
			if(rowspan > 1) DYNBUF_ADD3_INT(html, " rowspan=", rowspan, "");
			DYNBUF_ADD_STR(html, ">");
		}
		else
		{
			/* Add paragraph modifier */
			if(align && *align) DYNBUF_ADD3(html, "<", align, 0, NO_CONV, ">");
		}
		if(nobreak) DYNBUF_ADD_STR(html, "<nobr>");

		/* Add font modifiers */
		if(b_font) 
		{
			DYNBUF_ADD_STR(html, "<font");
			if(fontface && *fontface) DYNBUF_ADD3(html, " face='", fontface, 0, NO_CONV, "'");
			if(fontsize&& *fontsize) DYNBUF_ADD3(html, " size=", fontsize, 0, NO_CONV, "");
			if(fontcolor && strlen(fontcolor) == 6) DYNBUF_ADD3(html, " color=#", fontcolor, 0, NO_CONV, "");
			DYNBUF_ADD_STR(html, ">");
		}
		if(bold) DYNBUF_ADD_STR(html, "<b>");
		if(italic) DYNBUF_ADD_STR(html, "<i>");
		if(underline) DYNBUF_ADD_STR(html, "<u>");
	}
	else	/* Control footer */
	{
		/* Terminate font modifiers */
		if(underline) DYNBUF_ADD_STR(html, "</u>");
		if(italic) DYNBUF_ADD_STR(html, "</i>");
		if(bold) DYNBUF_ADD_STR(html, "</b>");
		if(b_font) DYNBUF_ADD_STR(html, "</font>");

		/* Terminate cell / paragraph */
		if(tag) 
		{
			DYNBUF_ADD3(html, "</", tag, 0, NO_CONV, ">");
		}
		else
		{
			if(align && *align) DYNBUF_ADD3(html, "</", align, 0, NO_CONV, ">");
		}
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP


/*********************************************************************
** Function : put_html_page_header
** Description : output HTML header code (direct to stdout)
*********************************************************************/
void put_html_page_header(
	EVA_context *cntxt,		/* in/out : execution context data */
	FILE *f,				/* in : output to this stream if not NULL */
	char *title,			/* in : page title */
	char *action,			/* in : params for action tag of <form> */
	int steps				/* in : max # of steps to run :
								1 = HTML header
								2 = <body> header
								3 = <form> header */
){
	if(!cntxt || cntxt->b_task) return;

	/* Output HTML header if applicable */
	if(!cntxt->b_header)
	{
		char tit[64] = {0};
		snprintf(add_sz_str(tit), "EVA - %s", cntxt->dbname);
		if(!f)
		{
			/* Read existing workstation Id in cookies */
			char *cook = getenv("HTTP_COOKIE");
			char *end = cook ? strchr(cook, ';') : NULL;
			if(end) *end = 0;
			if(cook) cook = strchr(cook, '=');
			if(cook) dynbuf_add(&cntxt->id_wks, cook + 1, 0, NO_CONV);

			/* Build new Id & set cookie if none found */
			if(!cntxt->id_wks)
			{
				char id[128];
				snprintf(add_sz_str(id), "%s-%x", datetxt_now(), rand());
				if(*id) cntxt->txsize += printf(
					"Set-Cookie: WKS=%s; expires=Monday, 15-Jun-2020 00:00:00 GMT;\n", id);
			}

			/* Put HTML Content-Type header */
			cntxt->txsize += printf("Content-Type:text/html;charset=iso-8859-1\r\n\n");
		}

		/* Put page title - disable cache */
		if(!title) title = dyntab_val(&cntxt->pagetitle, 0, 0);
		if(!*title) title = tit;
		cntxt->txsize += fprintf(f ? f : stdout,
			"<html><head><title>%s</title>", title);
		/* Disable cache for non administrator
		if(!cntxt->b_admin || f)
			cntxt->txsize += fprintf(f ? f : stdout,
				"<META HTTP-EQUIV='PRAGMA' CONTENT='NO-CACHE'>\n<META HTTP-EQUIV='CACHE-CONTROL' CONTENT='NO-CACHE'>");
*/

		/* Output link to eva javascript && CSS files */
		cntxt->txsize += fprintf(f ? f : stdout,
				"<script type=text/javascript src=../js/eva.js></script>"
				"<link href=../css/eva.css rel=stylesheet type=text/css>");

		/* Output link to site CSS file if present */
		{
			char cssname[256], *cssfile = DYNTAB_FIELD_VAL(&cntxt->cnf_data, IMGPATH);
			struct stat fs;
			if(!*cssfile) cssfile = cntxt->dbname;
			snprintf(add_sz_str(cssname), ".." DD "css" DD "%s.css", cssfile);
			if(!stat(cssname, &fs))
				cntxt->txsize += fprintf(f ? f : stdout, "<link href=../css/%s.css rel=stylesheet type=text/css>", cssfile);
		}

		cntxt->txsize += fprintf(f ? f : stdout, "</head>");
		cntxt->b_header = 1;
	}

	/* Output body header if applicable */
	if(steps > 1 && !cntxt->b_bodyheader)
	{
		/* Read applicable page background */
		char *bgcolor = DYNTAB_FIELD_VAL(&cntxt->user_data, PAGE_BGCOLOR);
		char *background = DYNTAB_FIELD_VAL(&cntxt->user_data, PAGE_BACKGROUND);
		if(!*bgcolor) bgcolor = DYNTAB_FIELD_VAL(&cntxt->cnf_data, PAGE_BGCOLOR);
		if(!*background) background = DYNTAB_FIELD_VAL(&cntxt->cnf_data, PAGE_BACKGROUND);
		background = get_image_file(cntxt, background, 0, 0);

		/* Put body header */
		cntxt->txsize += fprintf(f ? f : stdout, "%s", "<body leftmargin=0 topmargin=0 rightmargin=0 bottommargin=0");
		if(background) cntxt->txsize += fprintf(f ? f : stdout, " background='%s'", background);
		if(*bgcolor) cntxt->txsize += fprintf(f ? f : stdout, " bgcolor=#%s", bgcolor);
		cntxt->txsize += fprintf(f ? f : stdout, ">");
		if(cntxt->imgwait && !f) cntxt->txsize += fprintf(f ? f : stdout,	"%s", 
			"<script>document.write(\"<img src='../img/_eva_imgwait.gif'>\");</script>");
		cntxt->b_bodyheader = 1;
		M_FREE(background);
	}

	/* Output form header if applicable */
	if(steps > 2 && !cntxt->b_formheader)
	{
 
		/* Output links to specific javascript modules */
		if(cntxt->jsColorInput)
			cntxt->txsize += fprintf(f ? f : stdout, "\n<script type=text/javascript src=../js/color.js></script>");
		if(cntxt->jsenabled && cntxt->jsHTMLEditor)
		{
			/* Insert TinyMCE code & configuration */
			cntxt->txsize += fprintf(f ? f : stdout, "%s%s%s%s%s%s",
					"<script type=text/javascript src=../js/tiny_mce/tiny_mce.js></script>"
					"<script type=text/javascript>"
					"tinyMCE.init({"
						"mode : 'specific_textareas',"
						"language : 'fr',"
						"content_css : '/css/", cntxt->dbname, ".css',"
						"entity_encoding : 'raw',"
						"entities : '38,amp,60,lt,62,gt',"
						"cleanup_on_startup : true,"
						"apply_source_formatting : true,"
						"preformatted : false,"
						"theme_advanced_buttons1 : '",
						(cntxt->jsHTMLEditor == 1) ?
							"bold,italic,underline,separator,justifyleft,justifycenter,separator,bullist,numlist,separator,"
							"removeformat,undo,redo,separator,hr,charmap" :
						(cntxt->jsHTMLEditor == 2) ?
							"bold,italic,underline,sup,separator,justifyleft,justifycenter,separator,bullist,numlist,indent,outdent,separator,"
							"fontsizeselect,forecolor,separator,image,link,unlink,separator,removeformat,undo,redo,separator,hr,charmap" :
							"bold,italic,underline,sub,sup,strikethrough,separator,justifyleft,justifycenter,justifyright,justifyfull,separator,bullist,numlist,indent,outdent,separator,"
							"image,link,unlink,separator,fontselect,fontsizeselect,forecolor,backcolor,separator,removeformat,undo,redo,separator,hr,charmap,help",
						(cntxt->b_admin || cntxt->jsHTMLEditor == 3) ? ",code" : "",
						"',"
						"theme_advanced_buttons2 : '',"
						"theme_advanced_buttons3 : '',"
						"convert_urls : 'false',"
						"theme_advanced_fonts : 'Arial=arial,helvetica,sans-serif;Courier New=courier new,courier,monospace;Impact=impact,chicago;Verdana=verdana,geneva',"
						"theme_advanced_toolbar_align : 'left',"
						"theme_advanced_toolbar_location : 'top'"
					"});</script>");
		}
		if(cntxt->jsenabled && cntxt->jsCalendarInput)
		{
			/* Insert SCW calendar input code  */
			cntxt->txsize += fprintf(f ? f : stdout,
					"<script type=text/javascript src=../js/scw.js></script>");
		}

	
		if(!(cntxt->b_terminate & 64))
		{
			cntxt->txsize += fprintf(f ? f : stdout, "<form name='mainform'");
			if(!f)
			{
				cntxt->txsize += fprintf(f ? f : stdout, " action='/eva/%s.cgi", cntxt->dbname);
				if(action) cntxt->txsize += fprintf(f ? f : stdout, "?%s", action);
				cntxt->txsize += fprintf(f ? f : stdout, "%s", "' method=post");
				if(cntxt->cgiencodemultipart) cntxt->txsize += fprintf(f ? f : stdout, "%s", " enctype='multipart/form-data'");
			}
			cntxt->txsize += fprintf(f ? f : stdout, ">");
		}
		if(!f) cntxt->txsize += fprintf(stdout, 
			"<script>%s"
			"document.write(\"<input type=hidden name=JSINPUT>\");"
			"</script>",
			cntxt->imgwait ? "document.getElementsByTagName('body')[0].removeChild(document.images[0]);" : "");
		cntxt->b_formheader = 1;
	}
	fflush(stdout);
}

/*********************************************************************
** Function : put_html_page_trailer
** Description : output signup & HTML footer code (direct to stdout)
*********************************************************************/
void put_html_page_trailer(EVA_context *cntxt, FILE *f)
{
	int b_stats = 1;
	DynBuffer *focus = cntxt->focus1 ? cntxt->focus1 : cntxt->focus2 ? cntxt->focus2 : cntxt->focus3;
 	clock_t t;
	
	if(!(cntxt->b_terminate & 64))
	{
		/* Get trailer format */
		char *trailfmt = DYNTAB_FIELD_VAL(&cntxt->user_data, PAGE_BOTTOM);
		int b_usrbot = *trailfmt;
		if(!b_usrbot) trailfmt = dyntab_val(&cntxt->pagebottom, 0, 0);
		b_stats = !*trailfmt || !strcmp(trailfmt, "_EVA_NORMAL");
		
		t = clock();

		/* Handle JavaScript end script & SetFocus */
		cntxt->txsize += fprintf(f ? f : stdout, "%s", "<script>");
		if(cntxt->endjs) cntxt->txsize += fprintf(f ? f : stdout, "%s", cntxt->endjs->data);
		if(!f && focus) cntxt->txsize += printf("document.mainform[\"%s\"].focus();", focus->data);
		cntxt->txsize += fprintf(f ? f : stdout, "%s", "</script>");

#define TIME_CELL(txt, bg) "<td align=center bgcolor=#" bg "><nowrap><font size=-2>&nbsp;" txt "&nbsp;</font></td>"	
		if(cntxt->b_terminate & 16)
		{
			fprintf(f ? f : stdout, "<br>Durée : %1.2lf s", (double)t/CLOCKS_PER_SEC);
		}
		else if(!strcmp(trailfmt, "_EVA_HTML"))
		{
			DynBuffer *buf = NULL;
			unsigned long idbot = strtoul(b_usrbot ? DYNTAB_FIELD_VAL( &cntxt->user_data, PAGE_FOOTER) :
													 dyntab_val(&cntxt->pagebtctrl, 0, 0), NULL, 10);
			if(idbot)
			{
				if(form_get_html(cntxt, &buf, idbot, 0)) err_clear(cntxt, "put_html_page_trailer", __FILE__, __LINE__);
				if(buf) cntxt->txsize += fprintf(f ? f : stdout, "%s", buf->data);
				M_FREE(buf);
			}
		}
		else if(strcmp(trailfmt, "_EVA_NONE") && !cntxt->b_pda)
		{
			/* Output page end block with webmaster email & process statistics */
			cntxt->txsize += fprintf(f ? f : stdout, "%s%s%s%s%s%s",
					"<font face=Arial>"
					"<table cellspacing=0 cellpadding=0 width=100% border=0 bgcolor=#E0E0E0 rules=none><tr>\n"
					"<td>&nbsp;<font size=-1><a href='mailto:",
					dyntab_sz(&cntxt->mail_admin, 0, 0) ? dyntab_val(&cntxt->mail_admin, 0, 0) : "alain.boute@abing.fr",
					"'><img src='/img/letter_small.gif' border=0 align=absmiddle>Ecrire au webmaster</a></i></font>"
					"</td><td align=", b_stats ? "center" : "right", "><nobr><i>"
					"<font size=-2>Serveur EVA v4.0.1<br>&copy;<a href='http://abing.fr' target=_blank>ABI</a>"
					" " __DATE__ "</font></td>",
					!b_stats ? "" :
					"<td align=right>"
					"<table border=0 cellspacing=1 cellpadding=0><tr>"
					TIME_CELL("Durée", "BBBBBB")
					TIME_CELL("CPU", "BBBBBB")
					TIME_CELL("SQL", "BBBBBB")
					TIME_CELL("Emission", "BBBBBB")
					TIME_CELL("Réception", "BBBBBB")
					"</tr><tr>");
			if(b_stats)
			{
				char *rxsize, *rxrate, *txsize, *txrate;
#define COPY_HUMANFILESIZE(dest, _sz) { size_t sz = _sz; char *src = sz ? human_filesize(sz) : "";  size_t size = strlen(src); \
											dest = calloc(size + 1, 1); if((src) && size) memcpy(dest, src, size); }
				COPY_HUMANFILESIZE(rxsize, cntxt->rxsize); 
				COPY_HUMANFILESIZE(rxrate, cntxt->rxtime > 200 ? (size_t)(1.0 * cntxt->rxsize / cntxt->rxtime * CLOCKS_PER_SEC) : 0);
				COPY_HUMANFILESIZE(txsize, cntxt->txsize);
				COPY_HUMANFILESIZE(txrate, cntxt->txtime > 200 ? (size_t)(1.0 * cntxt->txsize / cntxt->txtime * CLOCKS_PER_SEC) : 0);
#undef COPY_HUMANFILESIZE

				fprintf(f ? f : stdout, TIME_CELL("%1.2lf s", "FFFFFF"), (double)t/CLOCKS_PER_SEC);
				fprintf(f ? f : stdout, TIME_CELL("%1.2lf s", "EEEEEE"), (double)(t - cntxt->rxtime - cntxt->txtime - cntxt->sqltime)/CLOCKS_PER_SEC);
				fprintf(f ? f : stdout, TIME_CELL("%1.2lf s", "EEEEEE"), (double)cntxt->sqltime/CLOCKS_PER_SEC);
				fprintf(f ? f : stdout, TIME_CELL("%1.2lf s - %s", "DDFFFF"), (double)cntxt->rxtime/CLOCKS_PER_SEC, rxsize);
				fprintf(f ? f : stdout, TIME_CELL("%1.2lf s - %s", "DDDDFF"), (double)cntxt->txtime/CLOCKS_PER_SEC, txsize);
#undef TIME_CELL
				fprintf(f ? f : stdout, "%s", "</tr></table></td>");
				free(rxsize); free(rxrate); free(txsize); free(txrate); 
			}
			fprintf(f ? f : stdout, "%s", "</tr></table>");
		}

		if(!f && cntxt->b_formheader) printf("</form>");

		put_debug_msg(cntxt, f);
	}

	fprintf(f ? f : stdout, "</body>");
	fprintf(f ? f : stdout, "</html>");
}

/*********************************************************************
** Function : put_html_bold_substring
** Description : output text with the given substring bold
*********************************************************************/
#define ERR_FUNCTION "put_html_bold_substring"
#define ERR_CLEANUP M_FREE(val); DYNTAB_FREE(words)
int put_html_bold_substring(
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer **html,					/* in/out : buffer to add text to */
	char *txtout,	 					/* in : text to output to buffer */
	size_t sz,							/* string length of txtout, or 0 if unknown */
	DynTable *substr					/* in : list of substrings to output as bold text */
){
	char *val = NULL, *txt, *txt0;
	DynTable words = {0};
	unsigned long i, j;
	size_t len = 0;
	if(!html) RETURN_OK;

	/* Uppercase val & substrings */
	if(!txtout || !*txtout) RETURN_OK;
	if(!sz) sz = strlen(txtout);
	M_STRDUP(val, txtout, sz);
	setcase(3, val, sz);
	if(substr) for(txt = 0, i = 0; i < substr->nbrows; i++)
	{
		size_t sz = dyntab_sz(substr, i, 0);
		DYNTAB_ADD_CELL(&words, i, 0, substr, i, 0);
		setcase(3, DYNTAB_VAL_SZ(&words, i, 0));
		if(len < sz) len = sz;
	}

	/* Output string to buffer & return if no substr */
	if(!len)
	{
		DYNBUF_ADD(html, txtout, sz, TO_HTML);
		RETURN_OK;
	}

	txt0 = val;
	while(txt0 && *txt0)
	{
		/* Search 1st occurence of all substrings */
		for(txt = 0, i = 0, j = 0; i < words.nbrows; i++)
		{
			char *txt2 = dyntab_val(&words, i, 0);
			char *txt1 = *txt2 ? strstr(txt0, txt2) : NULL;
			if(!txt || txt1 && txt1 < txt) 
			{
				txt = txt1;
				j = i;
			}
		}

		/* Output text with bold substr */
		len = txt ? txt - txt0 : (sz - (txt0 - val));
		if(len) DYNBUF_ADD(html, txtout + (txt0 - val), len, TO_HTML);
		len = dyntab_sz(substr, j, 0);
		if(txt && len)
			DYNBUF_ADD3(html, "<b><font color=#FF4444>", txtout + (txt - val), len, TO_HTML, "</font></b>");
		txt0 = txt ? txt + len : 0;
	}

 	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
