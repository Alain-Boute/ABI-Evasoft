/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_contain.c
** Description : handing functions for container controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_tab
** Description : handles TAB controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_tab"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree)
int ctrl_add_tab(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable ctrltree = { 0 };
	DynBuffer **prev = form->html;

	/* Read control attributes */
	CTRL_ATTR(ctrltree, CTRLTREE);

	switch(form->step)
	{
	case CtrlRead:
		/* Set cginame */
		if(cgi_build_basename(cntxt, &ctrl->cginame, i_ctrl, 'T')) STACK_ERROR;

		/* Handle tabs list */
		M_REALLOC(unsigned long, form->tabs, form->nb_tabs);
		form->tabs[form->nb_tabs - 1] = i_ctrl;

		/* Set fixed values for tabs */
		ctrl->POSITION = "_EVA_NewLine";
		ctrl->COLSPAN = 0;
		ctrl->ROWSPAN = 0;
		if(!ctrl->TABLERULES[0] && !*ctrl->TABLE_STYLE) ctrl->TABLERULES = "none";
		if(!ctrl->CELL_STYLE[0]) ctrl->CELL_STYLE = !strcmp(ATTR_VAL(form->ctrl, TABS_POS), "_EVA_LEFT") ? "EVATabBorderH" : "EVATabBorderV";

		/* Handle tab optimization */
		if(form->opttabid)
		{
			unsigned long id = DYNTAB_TOUL(&ctrl->id);
			if(form->opttabid == ~0UL) form->opttabid = id;
			if(form->opttabid == id)
				form->seltab = i_ctrl;
			else 
				break;
		}

		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
		break;

	case HtmlPrint:
		/* Set tab title print format */
		ctrl->OPTIONBUTTON = "_EVA_NONE";
		ctrl->LABELPOS = "_EVA_TOP";
		ctrl->LABELFONTSIZE = "+1";
		ctrl->LABELBOLD = "1";
		ctrl->TABLEWIDTH = "100%";

		/* Position control & output HTML table header */
		form->html = &form->html_tab;
		if(ctrl_format_pos(cntxt, ctrl, 1) ||
			ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;

		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);

		/* Output HTML table footer */
		if(ctrl_put_table_footer(cntxt, ctrl) ||
			ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	case HtmlEdit:
	case HtmlView:

		/* Handle active tab */
		if(!form->seltab) form->seltab = i_ctrl;

		/* If tab is active */
		if(form->seltab == i_ctrl)
		{
			/* Output lower level controls */
			form->html = &form->html_tab;
			if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(form->html, "\n<!--- Start Tab ", ctrl->LABEL, 0, NO_CONV, " -->\n");
			CTRL_ADD_CHILD(i_ctrl, NULL);
			if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(form->html, "\n<!--- End Tab ", ctrl->LABEL, 0, NO_CONV, " -->\n");
		}
		else if(!form->opttabid)
		{
			/* Process lower level controls with no HTML output 
				This is needed because other tabs may be output by controls 
				located in hidden tabs */
			form->html = NULL;
			CTRL_ADD_CHILD(i_ctrl, &ctrltree);
		}
		ctrl->b_selected = 1;

		/* Switch to form bottom after first level tab */
		form->html = prev;
		break;

	default:
		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_tab_image
** Description : get image for a formated tab
*********************************************************************/
char *ctrl_tab_image(				/* return : image on success, NULL on error */
	char *pfx,						/* in : prefix (bg, bot or empty) */
	char *pos						/* in : image position (left, left_s, mid, mid_sl, mid_sr, right, right_s) */
){
	static char img[64];
	snprintf(add_sz_str(img), "_eva_tab_%s%s%s.gif", pfx ? pfx : "", pfx ? "_" : "", pos);

	return img;
}

/*********************************************************************
** Function : ctrl_add_tab_header
** Description : handles header for main TAB controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_tab_header"
#define ERR_CLEANUP M_FREE(tooltip)
int ctrl_add_tab_header(			/* return : 0 on success, other on error */
	EVA_context *cntxt				/* in/out : execution context data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl;
	DynBuffer *tooltip = NULL;
	unsigned long i, i_sel = 0;
	enum { TopBtns, LeftBtns, TopTabs, LeftTabs } dispmode = TopBtns;
	char *tabcolor;
	char *imgselbase_l = NULL;
	char *imgselbase_r = NULL;
	char *img, *imgsel, *imgbase;

	/* Read form tabs attributes */
	char *tabsheight = CTRL_ATTR_VAL(TABSHEIGHT);
	char *tabswidth = CTRL_ATTR_VAL(TABSWIDTH);
	char *tabsbgcolor = CTRL_ATTR_VAL(TABSBGCOLOR);
	char *tabsborder = CTRL_ATTR_VAL(TABSBORDER);
	char *tabspos = CTRL_ATTR_VAL(TABS_POS);
	int b_left = !strcmp(tabspos, "_EVA_LEFT");
	char *type = CTRL_ATTR_VAL(TABS_HEADER_MODE);
	int rightmenu = *DYNTAB_FIELD_VAL(&cntxt->user_data, MENUPOS) == '1';
	if(!*type) type = DYNTAB_FIELD_VAL(&cntxt->cnf_data, TABS_HEADER_MODE);
	if(!*tabsheight) tabsheight = ATTR_VAL(&cntxt->srvfmt, TABSHEIGHT);

	/* Remove hidden tabs */
	for(i = 0; i < form->nb_tabs; i++)
	{
		if(form->tabs[i] == form->seltab) i_sel = i;
		if(form->ctrl[form->tabs[i]].b_selected) continue;
		form->nb_tabs--;
		if(i < form->nb_tabs) memcpy(form->tabs + i, form->tabs + i + 1, (form->nb_tabs - i) * sizeof(form->tabs[0]));
		i--;
	}
	if(form->nb_tabs < 2) RETURN_OK;

	/* Set tabs display mode */
	dispmode = (!strcmp(type, "_EVA_TAB") && cntxt->jsenabled) ?
				b_left ? LeftTabs : TopTabs :
				b_left ? LeftBtns : TopBtns;

	/* Put tabs buttons bar header */
	ctrl = form->ctrl + form->seltab;
	if(put_html_format_pos(cntxt, "_EVA_NewColumn", rightmenu ? "right" : NULL,
			b_left ? "top" : "bottom", tabswidth,
			(dispmode == TopTabs || dispmode == TopBtns) ? tabsheight : NULL,
			(dispmode == TopTabs || !*tabsbgcolor) ? "FFFFFF" : tabsbgcolor,
			NULL, 0, 0, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 1))
		STACK_ERROR;
	DYNBUF_ADD3_INT(form->html, "<table border=", atoi(tabsborder), " rules=none cellspacing=0 cellpadding=0");
	if(b_left) DYNBUF_ADD_STR(form->html, " width=100%");
	DYNBUF_ADD_STR(form->html, "><tr>");
	tabcolor = ctrl->BGCOLOR;
	if(!*tabcolor) tabcolor = "FFFFFF";

	/* Output each tab */
	for(i = 0; i < form->nb_tabs; i++)
	{
		unsigned long i_ctrl = form->tabs[i];
		int scur = i == i_sel;
		int sprv = i && i - 1 == i_sel;
		int snxt = i < form->nb_tabs && i + 1 == i_sel;

		/* Build tooltip */
		ctrl = form->ctrl + i_ctrl;
		M_FREE(tooltip);
		DYNBUF_ADD3(&tooltip, "[", ctrl->LABEL, 0, NO_CONV, "]");
		if(ctrl->NOTES && ctrl->NOTES[0]) DYNBUF_ADD3(&tooltip, "\n\n", ctrl->NOTES, 0, NO_CONV, "");

		/* Get images */
		img = CTRL_ATTR_VAL(IMAGE);
		imgsel = CTRL_ATTR_VAL(IMAGESEL);

		/* Select display type */
		switch(dispmode)
		{
		case TopTabs:
			/* Display top tabs : select apropriate image */
			imgbase = scur ?
					!i ? "left_s" : "mid_sl" :
					!i ? "left" : sprv ? "mid_sr" : "mid";
			if(imgselbase_l && !imgselbase_r) imgselbase_r = imgbase;

			/* Output tab separator */
			DYNBUF_ADD_STR(form->html, "<td width=1 valign=top");
			if(scur || sprv || snxt)
				DYNBUF_ADD3(form->html, " bgcolor=#", tabcolor, 0, NO_CONV, "");
			DYNBUF_ADD3(form->html, " background='../img/_eva_tab_bg_", imgbase, 0, NO_CONV, ".gif'>");
			if(put_html_image(cntxt, NULL, ctrl_tab_image(NULL, imgbase), NULL, NULL, NULL, 0)) STACK_ERROR;
			DYNBUF_ADD_STR(form->html, "</td>");

			/* Output tab itself */
			if(scur)
			{
				/* Selected tab */
				imgselbase_l = imgbase;
				if(put_html_format_pos(cntxt, "_EVA_NewColumn", "center",
						NULL, NULL, NULL, tabcolor, "../img/_eva_tab_bg_s.gif", 
						0, 0, ctrl->FONTFACE, "+0", NULL, NULL, 1, 0, 0, 0, 1))
					STACK_ERROR;
				DYNBUF_ADD3(form->html, "<img src=../img/_eva_nop.gif height=2><br>", ctrl->LABEL, 0, TO_HTML, "");
				if(ctrl_add_opt_btn(cntxt, ctrl)) STACK_ERROR;
			}
			else
			{
				/* Other tabs */
				DYNBUF_ADD3_BUF(form->html, 
					"<td class=EVAUnselHTab background='../img/_eva_tab_bg.gif' align=center "
						"onClick=cb('", ctrl->cginame, NO_CONV, "')");
				DYNBUF_ADD3_BUF(form->html, " title='", tooltip, HTML_TOOLTIP, "'");
				if(put_showhelp(cntxt, form->html)) STACK_ERROR;
				DYNBUF_ADD3(form->html, ">", ctrl->LABEL, 0, TO_HTML, "");
			}
			DYNBUF_ADD_STR(form->html, "</td>");

			/* Output last tab separator */
			if(i == form->nb_tabs - 1)
			{
				imgbase = scur ? "right_s" : "right";
				DYNBUF_ADD_STR(form->html, "<td width=1 valign=top");
				if(scur)
					DYNBUF_ADD3(form->html, " bgcolor=#", tabcolor, 0, NO_CONV, "");
				DYNBUF_ADD3(form->html, " background='../img/_eva_tab_bg_", imgbase, 0, NO_CONV, ".gif'>");
				if(put_html_image(cntxt, NULL, ctrl_tab_image(NULL, imgbase), NULL, NULL, NULL, 0)) STACK_ERROR;
				DYNBUF_ADD_STR(form->html, "</td></tr><tr>");

				/* Output bottom black line */
				if(i_sel) DYNBUF_ADD3_INT(form->html, "<td bgcolor=#000000 colspan=", i_sel * 2, "></td>");
				DYNBUF_ADD3(form->html, "<td bgcolor=#", tabcolor, 0, NO_CONV, ">");
				if(put_html_image(cntxt, NULL, ctrl_tab_image("bot", imgselbase_l), NULL, NULL, NULL, 0)) STACK_ERROR;
				DYNBUF_ADD3(form->html, "</td><td bgcolor=#", tabcolor, 0, NO_CONV, "></td>");
				DYNBUF_ADD3(form->html, "<td bgcolor=#", tabcolor, 0, NO_CONV, ">");
				if(put_html_image(cntxt, NULL, ctrl_tab_image("bot", imgselbase_r ? imgselbase_r : imgbase), NULL, NULL, NULL, 0)) STACK_ERROR;
				DYNBUF_ADD_STR(form->html, "</td>");
				if(i_sel < form->nb_tabs - 1)
					DYNBUF_ADD3_INT(form->html, "<td bgcolor=#000000 colspan=", (form->nb_tabs - i_sel - 1) * 2, "></td>");
			}
			break;

		case LeftTabs:
			/* Output left tabs */
			if(i) DYNBUF_ADD_STR(form->html, "</tr><tr>");
			if(scur)
			{
				/* Selected tab */
				DYNBUF_ADD3(form->html, "<td class=EVASelVTab bgcolor=#", tabcolor, 0, TO_HTML, "");
				DYNBUF_ADD3(form->html, " height=", tabsheight, 0, NO_CONV, ">");
				DYNBUF_ADD(form->html, ctrl->LABEL, 0, TO_HTML);
				if(ctrl_add_opt_btn(cntxt, ctrl)) STACK_ERROR;
			}
			else
			{
				/* Other tabs */
				DYNBUF_ADD3_BUF(form->html, 
					"<td class=EVAUnselVTab onClick=cb('", ctrl->cginame, NO_CONV, "')");
				DYNBUF_ADD3_BUF(form->html, " title='", tooltip, HTML_TOOLTIP, "'");
				if(put_showhelp(cntxt, form->html)) STACK_ERROR;
				DYNBUF_ADD3(form->html, " height=", tabsheight, 0, NO_CONV, ">");
				if(put_html_image(cntxt, NULL, "_eva_item.gif", NULL, NULL, NULL, 0)) STACK_ERROR;
				DYNBUF_ADD(form->html, ctrl->LABEL, 0, TO_HTML);
			}
			DYNBUF_ADD_STR(form->html, "</td>");
			break;
		
		case LeftBtns:
			/* Output left buttons */
			if(scur)
			{
				/* Selected tab */
				if(put_html_format_pos(cntxt, i ? "_EVA_NewLine" : "_EVA_NewColumn", NULL,
						"middle", NULL, "30", tabcolor, NULL, 
						2, 0, ctrl->FONTFACE, "+0", NULL, NULL, 1, 0, 0, 0, 1))
					STACK_ERROR;
				if(put_html_button(cntxt, ctrl->cginame->data, ctrl->LABEL, imgsel, NULL, tooltip->data, 0, 1)) STACK_ERROR;
				if(ctrl_add_opt_btn(cntxt, cntxt->form->ctrl + form->seltab)) STACK_ERROR;
			}
			else
			{
				/* Other tabs */
				DYNBUF_ADD_STR(form->html, "<td>");
				if(put_html_button(cntxt, ctrl->cginame->data, ctrl->LABEL, img, imgsel, tooltip->data, 0, 0)) STACK_ERROR;
				DYNBUF_ADD_STR(form->html, "</td><td bgcolor=black width=1>");
			}
			DYNBUF_ADD_STR(form->html, "</td></tr><tr><td colspan=2 bgcolor=black height=1></td></tr><tr>");
			break;
		
		default:
			DYNBUF_ADD_STR(form->html, "<td>");
			if(put_html_button(cntxt, 
					ctrl->cginame->data,
					ctrl->LABEL,
					scur ? imgsel : img, 
					scur ? img : imgsel, 
					tooltip->data,
					0, scur))
				STACK_ERROR;
			DYNBUF_ADD_STR(form->html, "</td>");

			/* Put option button for current tab */
			if(i == form->nb_tabs - 1)
			{
				DYNBUF_ADD_STR(form->html, "<td>");
				if(ctrl_add_opt_btn(cntxt, cntxt->form->ctrl + form->seltab)) STACK_ERROR;
				DYNBUF_ADD_STR(form->html, "</td>");
			}
		}
	}
	DYNBUF_ADD_STR(form->html, "</tr></table></td>")

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_group
** Description : handles GROUP controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_group"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree)
int ctrl_add_group(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	char *br_style = CTRL_ATTR_VAL(BORDER_STYLE);
	DynTable ctrltree = { 0 };

	/* Read control attributes */
	CTRL_ATTR(ctrltree, CTRLTREE);

	/* Add lower level controls if no output */
	if(!form->html) 
	{
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
		RETURN_OK;
	}

	switch(form->step)
	{
	case HtmlEdit:
	case HtmlView:
	case HtmlPrint:

		/* Position control */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle border style : output surrounding table */
		if(!strcmp(br_style, "_EVA_RoundCorner"))
		{
			DYNBUF_ADD_STR(form->html, "<table cellpadding=0 cellspacing=0 border=0");
			if(!strcmp(ctrl->TABLEWIDTH, "100%")) DYNBUF_ADD_STR(form->html, " width=100%");
			DYNBUF_ADD_STR(form->html, "><tr><td width=10><img src=/img/bord_tl.gif></td>");
			DYNBUF_ADD_STR(form->html, "<td background=/img/bord_t.gif align=center></td>");
			DYNBUF_ADD_STR(form->html, "<td width=10><img src=/img/bord_tr.gif></td></tr>");
			DYNBUF_ADD_STR(form->html, "<tr><td background=/img/bord_l.gif></td><td>");
		}

		/* Set default values for groups */
		if(!ctrl->CELL_STYLE[0] && !ctrl->TABLE_STYLE[0])
		{
			if(!ctrl->TABLERULES[0]) ctrl->TABLERULES = "none";
			if(!CTRL_ATTR_CELL(BORDER)) ctrl->BORDER = 1;
		}

		/* Output HTML table header */
		if(ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;

		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);

		/* Output HTML table footer */
		if(ctrl_put_table_footer(cntxt, ctrl)) STACK_ERROR;

		/* Handle border style : output surrounding table */
		if(!strcmp(br_style, "_EVA_RoundCorner"))
		{
			DYNBUF_ADD_STR(form->html, "</td><td background=/img/bord_r.gif></td></tr>");
			DYNBUF_ADD_STR(form->html, "<tr><td><img src=/img/bord_bl.gif></td>");
			DYNBUF_ADD_STR(form->html, "<td background=/img/bord_b.gif></td>");
			DYNBUF_ADD_STR(form->html, "<td><img src=/img/bord_br.gif></td></tr></table>");
		}

		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	default:
		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_menubar
** Description : handles MENUBAR controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_menubar"
#define ERR_CLEANUP form->html = html
int ctrl_add_menubar(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer ** html = form->html;

	/* No output in print mode */
	if(form->step == HtmlPrint) RETURN_OK;

	/* Switch output buffer */
	if(form->html && (form->step == HtmlEdit || form->step == HtmlView)) form->html = &form->html_menu;

	/* Handle as group */
	if(!ctrl->ROWSPAN) ctrl->ROWSPAN = 3;
	if(ctrl_add_group(cntxt, i_ctrl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_pack
** Description : handles PACKAGE controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_pack"
#define ERR_CLEANUP DYNTAB_FREE(listobj); \
					DYNTAB_FREE(fieldexpr); \
					DYNTAB_FREE(ctrltree); \
					form->step = step; \
					cntxt->sql_trace = sql_trace
int ctrl_add_pack(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable listobj = { 0 };
	DynTable fieldexpr = { 0 };
	DynTable ctrltree = { 0 };
	char *srcobj = CTRL_ATTR_VAL(SRCOBJ);
 	char *dispmode = CTRL_ATTR_VAL(DISPLAY_MODE);
	int step = form->step;
	int sql_trace = cntxt->sql_trace;
	cntxt->sql_trace = atoi(CTRL_ATTR_VAL(DEBUG_ACCESS));

	/* Select controls to add in ctrl->val */
	switch(form->step)
	{
	case CtrlRead:
		if(!*srcobj)
		{
			/* List of predefined controls */
			CTRL_ATTR(ctrltree, CTRLTREE);
		}
		else if(!strcmp(srcobj, "_EVA_COND_CTRL"))
		{
			/* Condition controls - Process condition list */
			int res;
			CTRL_ATTR(listobj, COND_CTRLTREE);
			if(ctrl_check_condlist(cntxt, &res, &listobj, ~0UL)) STACK_ERROR;

			/* Handle matched condition */
			if(res) CTRL_ATTR(ctrltree, CTRLTREE)
			else CTRL_ATTR(ctrltree, CTRLTREE_ALT);
		}
		else if(!strcmp(srcobj, "_EVA_CONDSEQ"))
		{
			/* Conditions sequence - Process condition table */
			unsigned long lines, match;
			CTRL_ATTR_TAB(listobj, COND_CTRLTREE);
			CTRL_ATTR_TAB(fieldexpr, CTRLTREE);
			if(ctrl_check_condseq(cntxt, &listobj, &match)) STACK_ERROR;

			/* Handle matched condition */
			lines = fieldexpr.nbrows > listobj.nbrows ? fieldexpr.nbrows : listobj.nbrows;
			if((!lines || match < lines) &&
				dyntab_from_tab(&ctrltree, 0, 0, &fieldexpr, match, 0, match + 1, fieldexpr.nbcols, 17)) RETURN_ERR_MEMORY;
		}
		else
		{
			CTRL_ATTR(listobj, LISTOBJ);
			CTRL_ATTR(fieldexpr, SRCFIELD);
			if(ctrl_eval_valtyp(cntxt, ctrl, &ctrltree, srcobj, &listobj, &fieldexpr)) STACK_ERROR;
			dyntab_group(&ctrltree, "DISTINCT");
		}
		
		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
		break;

	default:
		/* Handle display mode */
		if(*dispmode)
		{
			form->opttabid = 0;
			if(form->step == HtmlView) form->step = HtmlPrint;
		}

		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, NULL);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_table_count_lines
** Description : prepare line indexes for all controls of a table
*********************************************************************/
#define ERR_FUNCTION "ctrl_table_count_lines"
#define ERR_CLEANUP 
int ctrl_table_count_lines(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : table control index in cntxt->form->ctrl */
	unsigned long *lines			/* out : # of lines in data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long j, i_child;
			  
	/* For each child control */
	for(i_child = ctrl->i_child; i_child; i_child = ctrl->i_brother)
	{
		unsigned long maxnum = 0, maxline = 0;
		ctrl = form->ctrl + i_child;
		if(!(ctrl->access & (AccessEdit|AccessView))) continue;

		/* Count # of lines in control */
		if(!strcmp(ctrl->CONTROL, "_EVA_INPUT"))
		{
			for(j = 0; j < ctrl->val.nbrows; j++)
			{
				/* Transform Num in Line if applicable */
				DynTableCell *val = dyntab_cell(&ctrl->val, j, 0);
				if(val->Num && !val->Line)
				{
					val->Line = val->Num;
					val->Num = 1;
				}
				if(val->Num > maxnum) maxnum = val->Num;
				if(val->Line > maxline) maxline = val->Line;
			}
			if(maxline > *lines) *lines = maxline;
		}
		if(ctrl_table_count_lines(cntxt, i_child, lines)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_table_null_lines
** Description : prepare line indexes for all controls of a table
*********************************************************************/
#define ERR_FUNCTION "ctrl_table_null_lines"
#define ERR_CLEANUP 
int ctrl_table_null_lines(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : table control index in cntxt->form->ctrl */
	unsigned long lines,			/* in : # of lines in data */
	int *b_null						/* out : set if null line index in data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long j, i_child;

	/* For each child control */
	for(i_child = ctrl->i_child; i_child; i_child = ctrl->i_brother)
	{
		ctrl = form->ctrl + i_child;
		if(!(ctrl->access & (AccessEdit|AccessView))) continue;

		/* Set values with null line index to last line */
		if(!strcmp(ctrl->CONTROL, "_EVA_INPUT"))
			for(j = 0; j < ctrl->val.nbrows; j++)
			{
				DynTableCell *val = dyntab_cell(&ctrl->val, j, 0);
				if(!val->Line)
				{
					val->Line = lines + 1;
					*b_null = 1;
				}
			}
		if(ctrl_table_null_lines(cntxt, i_child, lines, b_null)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_table_ins_line
** Description : insert a new line in a table
*********************************************************************/
void ctrl_table_ins_line(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	unsigned long line				/* in : line to insert before */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long ichild = ctrl->i_child, i;

	/* Insert line in control values */
	for(i = 0; i < ctrl->val.nbrows; i++)
	{
		DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
		if(c->Line >= line) c->Line++;
	}

	/* Process children */
	while(ichild)
	{
		ctrl_table_ins_line(cntxt, ichild, line);
		ichild = form->ctrl[ichild].i_brother;
	}
}

/*********************************************************************
** Function : ctrl_table_del_line
** Description : remove a line in a table
*********************************************************************/
void ctrl_table_del_line(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	unsigned long line				/* in : line to delete */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long ichild = ctrl->i_child, i;
	ObjTableFormat *tbl = ctrl->objtbl;

	/* Remove line in control values */
	for(i = 0; i < ctrl->val.nbrows; i++)
	{
		DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
		if(c->Line > line) c->Line--;
		else if(c->Line == line) 
		{
			dyntab_del_rows(&ctrl->val, i, 1);
			i--;
		}
	}
	if(tbl)
	{
		tbl->ctrlline = 0;
		tbl->status &= ~(TblCtrl_opensel | TblCtrl_opensearch);
	}

	/* Process children */
	while(ichild)
	{
		ctrl_table_del_line(cntxt, ichild, line);
		ichild = form->ctrl[ichild].i_brother;
	}
}

/*********************************************************************
** Function : ctrl_table_prepare_lines
** Description : prepare line indexes for all controls of a table
*********************************************************************/
#define ERR_FUNCTION "ctrl_table_prepare_lines"
#define ERR_CLEANUP 
int ctrl_table_prepare_lines(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl			/* in : table control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	int b_null = 0;
	char *btn = CGI_CLICK_BTN_SUBFIELD;
	unsigned long line = cntxt->cgi ? cntxt->cgi[cntxt->cgibtn].Line : 0;

	/* Handle control buttons clicks */
	if(line && DYNTAB_TOUL(&ctrl->id) == cntxt->cgi[cntxt->cgibtn].IdCtrl)
	{
		if(!strcmp(btn, "INS"))
			ctrl_table_ins_line(cntxt, i_ctrl, line);
		else if(!strcmp(btn, "DEL"))
			ctrl_table_del_line(cntxt, i_ctrl, line);
	}

	/* Count # of lines in data */
	if(ctrl_table_count_lines(cntxt, i_ctrl, &ctrl->lines)) STACK_ERROR;
	if(ctrl_table_null_lines(cntxt, i_ctrl, ctrl->lines, &b_null)) STACK_ERROR;
	if(b_null) ctrl->lines++;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_table
** Description : handles CONTAINER controls of type TABLE
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_table"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree); \
					M_FREE(name); \
					table_free(&tbl)
int ctrl_add_table(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable ctrltree = { 0 };
	DynBuffer *name = NULL;
	DynBuffer **html = form->html;
	unsigned long i, j, k, i_child;
	int b_first;
	int savedlg_outmode = form->savedlg_outmode;
	ObjTableFormat tbl = {0};
	unsigned long minval = strtoul(CTRL_ATTR_VAL(MINVAL), NULL, 10);
	unsigned long maxval = strtoul(CTRL_ATTR_VAL(MAXVAL), NULL, 10);
	unsigned long errnbval = strtoul(CTRL_ATTR_VAL(ERRNBVAL), NULL, 10);
	int mode_disp = atoi(CTRL_ATTR_VAL(NO_DELINS_BTN));
	int no_delins = mode_disp & 1;

	/* Return if a table is already beeing output (no table in table) */
	if(form->Line) RETURN_OK;

	/* Read control attributes */
	CTRL_ATTR(ctrltree, CTRLTREE);

	switch(form->step)
	{
	case CtrlRead:
		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);

		/* Prepare line indexes */
		if(ctrl_table_prepare_lines(cntxt, i_ctrl)) STACK_ERROR;
		break;

	case InputCheck:
		/* Process each line in table  */
		for(j = 0; j < minval || j < ctrl->lines; j++)
		{
			/* Process each control in line */
			int b_error = form->error;
			int b_modified = form->b_modified;
			int has_data = form->has_data;
			form->error = 0;
			form->b_modified = 0;
			form->has_data = 0;
			form->Line= j + 1;
			for(i = 0; i < ctrltree.nbrows; i++)
			{
				/* Get control index */
				i_child = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(&ctrltree, i, 0));
				if(!i_child) continue;
				
				/* Handle min # of lines */
				if(!i && errnbval && ctrl->lines < minval && i < minval)
					form->ctrl[i_child].ALLOWEMPTY = (errnbval == 1) ? "Warn" : "No";

				/* Call columns control handler for the line */
				CTRL_PRI_HDLR(i_child);
				
				/* Handle max # of lines */
				if(maxval && errnbval && j >= maxval && form->ctrl[i_child].error < 2 && form->ctrl[i_child].val.nbrows)
				{
					form->ctrl[i_child].error = errnbval;
					if(form->ctrl[i_child].errmsg) DYNBUF_ADD_STR(&form->ctrl[i_child].errmsg, "\n");
					DYNBUF_ADD3_INT(&form->ctrl[i_child].errmsg, "Vous devez saisir ", maxval, " lignes maximum");
					form->error |= form->ctrl[i_child].error;
				}

				 /* Store column error status in column 1 of ctrl->allval */
				DYNTAB_ADD_INT(&ctrl->allval, i , 1, form->ctrl[i_child].error);
			}

			/* If not last empty line */
			if(!ctrl->lines || j < ctrl->lines || form->has_data || j < minval)
			{
				/* Store line error & modified status in ctrl->val */
				DYNTAB_ADD_INT(&ctrl->val, j , 0, form->b_modified);
				DYNTAB_ADD_INT(&ctrl->val, j , 1, form->error);
				form->error |= b_error;
				form->b_modified |= b_modified;
				form->has_data |= has_data;

				/* Store column error status in column 0 of ctrl->allval */
				for(i = 0; i < ctrltree.nbrows; i++)
				{
					/* Keep most serious error condition */
					int err0 = atoi(dyntab_val(&ctrl->allval, i, 0));
					int err1 = atoi(dyntab_val(&ctrl->allval, i, 1));
					if(err1 > err0) DYNTAB_ADD_INT(&ctrl->allval, i, 0, err1);
				}
			}
			else
			{
				/* Restore form status */
				form->error = b_error;
				form->b_modified = b_modified;
				form->has_data = has_data;
			}
		}
		form->Line = 0;
		break;

	case HtmlSaveDlg:
		/* Process each line in table  */
		for(j = 0, b_first = 1; j < ctrl->val.nbrows; j++)
		{
			/* Output line in Edit mode if error, warning or modified */
			int b_modified = atoi(dyntab_val(&ctrl->val, j, 0));
			int b_error = atoi(dyntab_val(&ctrl->val, j, 1));
			if(!(
				(b_error & 1 && savedlg_outmode & 1) ||
				(b_error & 2 && savedlg_outmode & 2) ||
				(b_modified && savedlg_outmode & 4))) continue;

			/* Output label, line number & notes for each line */
			form->savedlg_outmode = 15;
			DYNBUF_ADD3(html, "<td align=right bgcolor=#DDDDDD><font size=-1><b><u>", ctrl->LABEL, 0, TO_HTML, " - Ligne n° ");
			DYNBUF_ADD_INT(html, j + 1);
			DYNBUF_ADD_STR(html, "</b></u></font></td>");
			if(b_first)
			{
				if(ctrl->NOTES) DYNBUF_ADD3(html, "<td bgcolor=#DDDDDD><font size=-1>", ctrl->NOTES, 0, TO_HTML, "</font></td>");
				b_first = 0;
			}
			else
				DYNBUF_ADD_STR(html, "<td></td>");
			DYNBUF_ADD_STR(html, "</tr><tr>");

			for(i = 0; i < ctrltree.nbrows; i++)
			{
				/* Add controls for the line */
				i_child = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(&ctrltree, i, 0));
				if(!i_child) continue;
				form->Line = j + 1;
				CTRL_PRI_HDLR(i_child);
			}
			form->savedlg_outmode = savedlg_outmode;
		}
		form->Line = 0;
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Set default values for tables */
		if(!CTRL_ATTR_CELL(BORDER)) ctrl->BORDER = 1;

		/* Position control & output HTML table header */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		if(ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;

		/* Output labels line */
		no_delins |= form->step != HtmlEdit || !(ctrl->access & AccessEdit);
		if(!(mode_disp & 6)) DYNBUF_ADD_STR(form->html, "<th>&nbsp;</th>\n");
		for(i = 0; i < ctrltree.nbrows; i++)
		{
			EVA_ctrl *child;
			i_child = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(&ctrltree, i, 0));
			if(!i_child) continue;
			child = form->ctrl + i_child;
			if(!(child->access & (AccessEdit|AccessView))) continue;
			child->error = (ctrl->lines || minval) ? atoi(dyntab_val(&ctrl->allval, i, 0)) : 0;
			if(!(mode_disp & 2))
			{
				child->LABELPOS = "_EVA_LEFT";
				if(!child->LABELALIGN[0]) child->LABELALIGN = "left";
				child->POSITION = "_EVA_NewColumn";
				if(ctrl_put_label(cntxt, child, "_EVA_NewHeader")) STACK_ERROR;
				child->LABELPOS = "_EVA_NONE";
			}
		}
		if(!no_delins) DYNBUF_ADD_STR(form->html, "<th>&nbsp;</th>");
		if(!(mode_disp & 2)) DYNBUF_ADD_STR(form->html, "</tr><tr>\n");

		/* Process each line in table  */
		k = ctrl->lines + (form->step == HtmlEdit ? 1 : 0);
		if(k < minval) k = minval;
		for(j = 0; j < k; j++)
		{
			char *bgcolor = table_row_bgcolor(cntxt, &tbl, j, NULL);
			if(!(mode_disp & 4))
			{
				DYNBUF_ADD_STR(form->html, "<td valign=top align=right");
				if(*bgcolor && strlen(bgcolor) == 6) DYNBUF_ADD3(form->html, " bgcolor=#", bgcolor, 6, NO_CONV, "");
				DYNBUF_ADD3_INT(form->html, ">", j + 1, "</td>\n")
			}

			/* Process each control in line */
			for(i = 0; i < ctrltree.nbrows; i++)
			{
				/* Add controls for the line */
				i_child = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(&ctrltree, i, 0));
				if(!i_child) continue;
				if(!(form->ctrl[i_child].access & (AccessEdit|AccessView))) continue;
				form->Line = j + 1;
				DYNBUF_ADD_STR(form->html, "<td valign=top");
				if(*bgcolor && strlen(bgcolor) == 6) DYNBUF_ADD3(form->html, " bgcolor=#", bgcolor, 6, NO_CONV, "");
				DYNBUF_ADD_STR(form->html, ">"
				"<table cellspacing=0 cellpadding=0 border=0 rules=none width=100%><tr>\n");
				CTRL_PRI_HDLR(i_child);
				DYNBUF_ADD_STR(form->html, "</tr></table></td>\n");
			}

			/* Output DEL/INS buttons if edit mode */
			if(!no_delins)
			{
				DYNBUF_ADD_STR(form->html, "<td valign=top");
				if(*bgcolor && strlen(bgcolor) == 6) DYNBUF_ADD3(form->html, " bgcolor=#", bgcolor, 6, NO_CONV, "");
				DYNBUF_ADD_STR(form->html, ">");
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("INS"));
				if(put_html_button_sz(cntxt, name->data, NULL, "_eva_plus.gif", "_eva_plus_s.gif",
					"Insérer une ligne au dessus", NULL, 0, 0, 0, 0)) STACK_ERROR;
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("DEL"));
				if(put_html_button_sz(cntxt, name->data, NULL, "_eva_cancel.gif", "_eva_cancel_s.gif",
					"Supprimer la ligne", NULL, 0, 0, 0, 0)) STACK_ERROR;
				DYNBUF_ADD_STR(form->html, "</td>");
			}

			if(j < k) DYNBUF_ADD_STR(form->html, "</tr><tr>\n");
		}
		form->Line = 0;

		/* Output HTML table footer */
		if(ctrl_put_table_footer(cntxt, ctrl)) STACK_ERROR;
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	default:
		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
