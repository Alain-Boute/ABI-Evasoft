/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_html.c
** Description : basic HTML output functions
**      Author : Alain BOUTE
**     Created : Sept 23 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : form_put_html
** Description : outputs HTML code for the given form
*********************************************************************/
#define ERR_FUNCTION "form_put_html"
#define ERR_CLEANUP M_FREE(name); \
					M_FREE(img)
int form_put_html(
	EVA_context *cntxt		/* in/out : execution context data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form ? form->ctrl : NULL;
	char *align;
	char *bgcolor;
	char *background;
	char *img = NULL;
	char *cellwidth;
	DynBuffer *name = NULL;
	int rightmenu = *DYNTAB_FIELD_VAL(&cntxt->user_data, MENUPOS) == '1';

	/* Build default title if applicable */
	if(!form->html_title && CTRL_ATTR_CELL(AUTO_TITLEBAR))
	{
		ctrl->LABELFONTSIZE = "+0";
		ctrl->LABELBOLD = "1";
		form->html = &form->html_title;
		DYNBUF_ADD_STR(form->html,
			"<table border=0 rules=none cellspacing=0 cellpadding=0 width=100%");
		bgcolor = DYNTAB_FIELD_VAL(&cntxt->cnf_data, TITLE_BGCOLOR);
		if(*bgcolor) DYNBUF_ADD3(form->html," bgcolor=#", bgcolor, 0, NO_CONV, "");
		img = get_image_file(cntxt, DYNTAB_FIELD_VAL(&cntxt->cnf_data, TITLE_BACKGROUND), 0, 0);
		if(img) DYNBUF_ADD3(form->html," background='", img, 0, NO_CONV, "'");
		M_FREE(img);
		DYNBUF_ADD_STR(form->html, "><tr>");
		if(ctrl_put_label(cntxt, ctrl, "_EVA_NewColumn")) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, 
			"<td align=right>");
		if(ctrl_cgi_name(cntxt, ctrl, NULL, 0, &name, 'B', add_sz_str("CLOSE")) ||
			put_html_button(cntxt, name->data, NULL, "_eva_close.gif", "_eva_close_s.gif", "Ferme le formulaire", 0, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html,
			"</td></tr></table></font>");
		form->html = NULL;
	}
	if(!form->title) DYNBUF_ADD(&form->title, ctrl->LABEL, 0, NO_CONV);

	/* Set form default attributes values */
	ctrl->POSITION = "_EVA_NewColumn";
	ctrl->LABELPOS = "_EVA_NONE";
	ctrl->OPTIONBUTTON = ctrl->LABELPOS;

	/* Read form attributes */
	align = CTRL_ATTR_VAL(TABLEALIGN);
	bgcolor = ctrl->TABLEBGCOLOR;
	background = ctrl->TABLEBACKGROUND;
	if(!*bgcolor) bgcolor = DYNTAB_FIELD_VAL(&cntxt->cnf_data, FORM_BGCOLOR);
	if(!*background) background = DYNTAB_FIELD_VAL(&cntxt->cnf_data, FORM_BACKGROUND);
	cellwidth = ctrl->WIDTH;

	/* Handle page title */
	if(!cntxt->title && !dyntab_cmp(&form->id_form, 0, 0, &cntxt->id_form, 0, 0))
		DYNBUF_ADD_BUF(&cntxt->title, form->title, NO_CONV);
	 
	/* Mark form context CGI data not to be reoutput */
	CGI_VALUES_DONTKEEP(&ctrl->cgival);

	/* Output HTML debug info if applicable */
	form->html = &cntxt->html;
	if(cntxt->debug & DEBUG_HTML)
		DYNBUF_ADD3(cntxt->form->html, "\n<!--- Start Form ", ctrl->LABEL, 0, NO_CONV, " -->\n");
 
	/* Put hidden input for form status */
	if(cgi_build_basename(cntxt, &ctrl->cginame, 0, 'F')) STACK_ERROR;
	DYNBUF_ADD3(&cntxt->htmlhidden, 
		"<input type=hidden name='", ctrl->cginame->data, ctrl->cginame->cnt, NO_CONV, "' value='");
	if(form_status_data(cntxt, &cntxt->htmlhidden)) STACK_ERROR;
	DYNBUF_ADD_STR(&cntxt->htmlhidden, "'>");
	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(&cntxt->htmlhidden, "\n");

	/* Output hidden input for selected tab */
	if(form->tabs && form_tab_selection(cntxt, 0)) STACK_ERROR;

	/* Output form main table header */
	if(ctrl->FONTFACE[0]) DYNBUF_ADD3(form->html, "<font face='", ctrl->FONTFACE, 0, NO_CONV, "'>");
	if(put_html_table_header(cntxt, align, ctrl->TABLEWIDTH, ctrl->TABLEHEIGHT, bgcolor, background,
								0, 0, 0, NULL, NULL, NULL))
		STACK_ERROR;

	/* Output form title */
	if(form->html_title) DYNBUF_ADD3_BUF(form->html, "<td colspan=2>",form->html_title, NO_CONV, "</td></tr><tr>")
	M_FREE(form->html_title);

	switch(form->step)
	{
	case FormValues:
		if(form_mode_values(cntxt)) STACK_ERROR;
		break;

	case FormSearch:
		if(form_mode_search(cntxt)) STACK_ERROR;
		break;

	case FormList:
		if(form_mode_list(cntxt)) STACK_ERROR;
		break;

	case HtmlSaveDlg:
		/* Output dialog */
		DYNBUF_ADD3_BUF(form->html, "<td><font face=Arial><b>", form->html_top, NO_CONV, "</font></td>");
		break;

	default:
		/* Output form menu */
		if(form->html_menu)
		{
			if(!rightmenu) DYNBUF_ADD_BUF(form->html, form->html_menu, NO_CONV);
			DYNBUF_ADD_STR(form->html, "<td valign=top>")
			if(put_html_table_header(cntxt, NULL, ctrl->TABLEWIDTH, 0, NULL, NULL, 0, 0, 0, NULL, NULL, NULL)) STACK_ERROR;
		}

		/* Output form top controls */
		if(form->html_top)
		{
			ctrl->BORDER = atoi(CTRL_ATTR_VAL(INNERBORDER));
			ctrl->WIDTH = "";
			ctrl->VALIGN = "top";
			if(ctrl_format_pos(cntxt, ctrl, 1) ||
				put_html_table_header(cntxt, NULL, cellwidth, ctrl->HEIGHT, NULL, NULL, 
								atoi(CTRL_ATTR_VAL(CELLSPACING)), 
								atoi(CTRL_ATTR_VAL(CELLPADDING)), ctrl->BORDER, ctrl->TABLERULES, ctrl->TABLE_STYLE, NULL))
				STACK_ERROR;
			DYNBUF_ADD3_BUF(form->html, "", form->html_top, NO_CONV, "</tr></table>")
			if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		}
 
		/* Output tabs buttons bar & contents if applicable */
		if(form->tabs)
		{
			/* Set control with form tab attributes */
			int b_left = !strcmp(CTRL_ATTR_VAL(TABS_POS), "_EVA_LEFT");
			EVA_ctrl *tab = form->ctrl + form->seltab;
			if(!tab->HEIGHT[0]) tab->HEIGHT = CTRL_ATTR_VAL(TABHEIGHT);
			if(!tab->BGCOLOR[0]) tab->BGCOLOR = tab->TABLEBGCOLOR;
			if(!tab->BGCOLOR[0]) tab->BGCOLOR = CTRL_ATTR_VAL(TABBGCOLOR);
			if(!tab->BGCOLOR[0]) tab->BGCOLOR = ATTR_VAL(&cntxt->srvfmt, TABBGCOLOR);
			tab->TABLEBGCOLOR = "";
			if(!tab->BACKGROUND[0]) tab->BACKGROUND = CTRL_ATTR_VAL(TABBACKGROUND);
			if(!tab->BACKGROUND[0]) tab->BACKGROUND = ATTR_VAL(&cntxt->srvfmt, TABBACKGROUND);
			tab->POSITION = b_left ? "_EVA_NewColumn" : "_EVA_NewLine";
			tab->VALIGN = "top";
			tab->LABELPOS = "_EVA_NONE";

			/* Output tabs header if not print mode */
			if(form->html_top) DYNBUF_ADD_STR(form->html, "</tr><tr>")
			if(form->step != HtmlPrint)
			{
				if(b_left) DYNBUF_ADD_STR(form->html, "<td colspan=2 bgcolor=black height=1></td>"
													"</tr><tr><td colspan=2><table cellpadding=0 cellspacing=0 border=0 width=100%><tr>");
				if(ctrl_add_tab_header(cntxt)) STACK_ERROR;
			}
			
			/* Put tab contents & footer */
			if(form->html_tab)
			{
				if(ctrl_format_pos(cntxt, tab, 1) || ctrl_put_table_header(cntxt, tab)) STACK_ERROR;
				DYNBUF_ADD3_BUF(form->html, "", form->html_tab, NO_CONV, "</tr></table>");
			}
			if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
			if(form->step != HtmlPrint && b_left) DYNBUF_ADD_STR(form->html, "</tr></table></td>");
		}
		if(form->html_menu)
		{
			DYNBUF_ADD_STR(form->html, "</tr></table></td>");
			if(rightmenu) DYNBUF_ADD_BUF(form->html, form->html_menu, NO_CONV);
		}
	}

	/* Output form table footer */
	{
		DynBuffer *b = *form->html;
		char *txt = b ? b->data : NULL;
		size_t len = b ? b->cnt : 0;
		if(txt && len > 3 && !strncmp(txt + len - 4, "<tr>", 4))
			b->cnt -= 4;
		else
			DYNBUF_ADD_STR(form->html, "</tr>");
	}
	DYNBUF_ADD_STR(form->html, "</table>");
	if(ctrl->FONTFACE[0]) DYNBUF_ADD_STR(form->html, "</font>");

	/* Output HTML debug info if applicable */
	if(cntxt->debug & DEBUG_HTML)
		DYNBUF_ADD3(cntxt->form->html, "\n<!--- EndOf Form ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_status_data
** Description : output encoded form status data
*********************************************************************/
#define ERR_FUNCTION "form_status_data"
#define ERR_CLEANUP 
int form_status_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **html				/* in/out : output buffer */
){
	EVA_form *form = cntxt->form;

	DYNBUF_ADD_CELL(html, &form->call_data, 0, 0, NO_CONV);
	DYNBUF_ADD3_BUF(html, "§", form->title, HTML_NO_QUOTE, "");
	DYNBUF_ADD3_INT(html, "§", form->step, "");
	DYNBUF_ADD3_CELL(html, "§", &form->dlg_ctrl, 0, 0, NO_CONV, "");
	DYNBUF_ADD3_INT(html, "§", (form->step <= HtmlStepEdit && form->step >= HtmlSaveDlg) ? form->b_modified : 0, "");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_tab_selection
** Description : handle CGI status for form main tab selection
*********************************************************************/
#define ERR_FUNCTION "form_tab_selection"
#define ERR_CLEANUP DYNTAB_FREE(cgival); \
					M_FREE(name)
int form_tab_selection(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	int mode						/* in : read if set, write else */
){
	EVA_form *form = cntxt->form;
	DynTable cgival = { 0 };
	DynBuffer *name = NULL;
	unsigned long idobj = DYNTAB_TOUL(&form->id_obj);
	unsigned long idform = DYNTAB_TOUL(&form->id_form);

	/* Build CGI name for hidden input */
	if(!form->tabs || form->nb_tabs < 2) RETURN_OK;
	if(cgi_build_name(cntxt, &name, 'D', 0, idform, idobj, add_sz_str("_EVA_SELTAB"), NULL, 0, 0, 0, 0)) STACK_ERROR;

	if(mode)
	{
		/* Read selected tab */
		if(cgi_get_values(cntxt, &cgival, name->data, 1)) STACK_ERROR;
		if(cgival.nbrows)
		{
			unsigned long i = 1;
			while(i < form->nb_ctrl && dyntab_cmp(&cgival, 0, 0, &form->ctrl[i].id, 0, 0)) i++;
			form->seltab = i < form->nb_ctrl ? i : 0;
			DYNTAB_ADD_CELL(&form->ctrl->cgival, 1, 0, &cgival, 0, 0);
		}
	}
	else
	{
		/* Output selected tab */
		if(form->seltab &&
			cgi_add_input(cntxt, NULL, DYNBUF_VAL_SZ(name), DYNTAB_VAL_SZ(&form->ctrl[form->seltab].id, 0, 0)))
			STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : process_form
** Description : handle form interaction
*********************************************************************/
#define ERR_FUNCTION "process_form"
#define ERR_CLEANUP DYNTAB_FREE(id_form); \
					DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(data)
int process_form(					/* return : 0 on success, other on error */
	EVA_context *cntxt				/* in/out : execution context data */
){
	DynTable id_form = { 0 };
	DynTable id_obj = { 0 };
	DynTable data = { 0 };

	/* Handle executable parameters */
	if(cntxt->qrystr)
	{
		unsigned long idform = 0, idobj = 0;
		if(sscanf(cntxt->qrystr, "OpenForm=%lu,%lu", &idform, &idobj) == 2 && idform)
		{
			/* Handle OpenForm parameters IdForm,IdObj */
			char *next = strchr(cntxt->qrystr, '&');
			DYNTAB_ADD_INT(&cntxt->id_form, 0, 0, idform);
			if(idobj) DYNTAB_ADD_INT(&cntxt->id_obj, 0, 0, idobj);

			/* Handle extra parameters as CGI input */
			if(next && next[1])
			{
				M_FREE(cntxt->input);
				while(next)
				{
					char *end = strchr(next + 1, '&');
					char *e = NULL;
					unsigned long idctrl = strtoul(next + 1, &e, 10);
					DYNTAB_FREE(data);
					DYNBUF_ADD_STR(&cntxt->input, "D");
					if(idctrl)
					{
						DYNBUF_ADD_INT(&cntxt->input, idctrl);
						DYNBUF_ADD_STR(&cntxt->input, "/");
						next = e - 1;
						if(qry_obj_field(cntxt, &data, idctrl, "_EVA_FIELD")) STACK_ERROR;
					}
					DYNBUF_ADD_INT(&cntxt->input, idform);
					DYNBUF_ADD_STR(&cntxt->input, "$");
					if(idobj) DYNBUF_ADD_INT(&cntxt->input, idobj);
					DYNBUF_ADD_STR(&cntxt->input, "#");
					if(data.nbrows) DYNBUF_ADD_CELL(&cntxt->input, &data, 0, 0, NO_CONV);
					DYNBUF_ADD(&cntxt->input, next + 1, end ? end - next : 0, NO_CONV);
					next = end;
				}
				if(cgi_read_urlencoded(cntxt)) STACK_ERROR;
			}
		}
	}

	/* If button clicked */
	if(cntxt->cgi)
	{
		CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
		if(cgi->IdForm && cgi->IdCtrl && cgi->name[0] != 'I' &&
			(cgi->subfield && strncmp(cgi->subfield, add_sz_str("OPENOBJ=")) ||
				cgi->IdCtrl != cgi->IdForm))
		{
			/* Read form & object ids in CGI data */
			if(cgi->IdObj) DYNTAB_ADD_INT(&id_obj, 0, 0, cgi->IdObj);
			DYNTAB_ADD_INT(&id_form, 0, 0, cgi->IdForm);

			/* Load form holding the button clicked & find button control index */
			if(form_load(cntxt, &id_form, &id_obj, 0)) STACK_ERROR;
			cgi = cntxt->cgi + cntxt->cgibtn;
			cntxt->form->i_ctrl_clic = ctrl_from_cginame(cntxt, cgi->name, cgi->name_sz);
			cntxt->log_clkbtn = cgi->IdCtrl;
			cntxt->log_clkform = cgi->IdForm;

			/* Call button handler */
			CTRL_ACT_HDLR(cntxt->form->i_ctrl_clic);

			/* Reset focus data if form closed */
			if(!cntxt->form || cntxt->form->b_reload)
			{
				M_FREE(cntxt->focus1);
				M_FREE(cntxt->focus2);
				M_FREE(cntxt->focus3);
			}
		}
		else
			if(action_default(cntxt, 0)) STACK_ERROR;
	}

	/* If loaded form is valid : output it */
	if(cntxt->form && !(cntxt->b_terminate & 1) && 	
			(	(	!dyntab_cmp(&cntxt->id_form, 0, 0, &cntxt->form->id_form, 0, 0) &&
					!dyntab_cmp(&cntxt->id_obj, 0, 0, &cntxt->form->id_obj, 0, 0)	) 
			|| (	!dyntab_cmp(&cntxt->alt_form, 0, 0, &cntxt->form->id_form, 0, 0) &&
					!dyntab_cmp(&cntxt->alt_obj, 0, 0, &cntxt->form->id_obj, 0, 0))) &&
		form_output(cntxt)) STACK_ERROR;

	/* Switch to homepage if no form selected */
	if(!cntxt->b_terminate && !dyntab_sz(&cntxt->id_form, 0, 0) && form_set_homepage(cntxt)) STACK_ERROR;

	/* Load & output menubar form if applicable */
	if(!(cntxt->b_terminate & 2) && dyntab_sz(&cntxt->menubar, 0, 0))
	{
		DynBuffer *html = cntxt->html;
		int b_terminate = cntxt->b_terminate;
		cntxt->b_terminate = 0;
		cntxt->html = NULL;
		if((!cntxt->form || dyntab_cmp(&cntxt->form->id_form, 0, 0, &cntxt->menubar, 0, 0)) &&
			form_load(cntxt, &cntxt->menubar, NULL, 0) ||
			form_output(cntxt)) STACK_ERROR;
		M_FREE(cntxt->html0);
		cntxt->html0 = cntxt->html;
		cntxt->html = html;
		cntxt->b_terminate = b_terminate;
	}

	/* Load & output main form if needed */
	if(!(cntxt->b_terminate & 4) && !cntxt->html &&
		(form_load(cntxt, &cntxt->id_form, &cntxt->id_obj, 0) ||
		form_output(cntxt))) STACK_ERROR;

	/* Load & output alternate form if applicable */
	if(!(cntxt->b_terminate & 8) && !cntxt->html1 && cntxt->alt_form.nbrows && (
		form_load(cntxt, &cntxt->alt_form, &cntxt->alt_obj, 0) ||
		form_output(cntxt))) STACK_ERROR;

	/* Ouput hidden input for CGI values not displayed as input */
	if(!(cntxt->b_terminate & 64) && cgi_put_hidden(cntxt)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

