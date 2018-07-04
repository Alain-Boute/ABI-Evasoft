/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_title.c
** Description : handling fonctions for controls of type TITLE
**      Author : Alain BOUTE
**     Created : Feb 17 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_title
** Description : handles TITLE controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_title"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree); \
					DYNTAB_FREE(data); \
					M_FREE(label); \
					M_FREE(notes); \
					M_FREE(img); \
					M_FREE(imgsel); \
					M_FREE(title)
int ctrl_add_title(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable ctrltree = { 0 };
	DynTable data = { 0 };
	DynBuffer *title = NULL;
	DynBuffer *label = NULL;
	DynBuffer *notes = NULL;
	DynBuffer *img = NULL;
	DynBuffer *imgsel = NULL;
	DynBuffer **html = form->html;
	char *oldlabel = ctrl->LABEL;
	int b_data = DYNTAB_FIELD_CELL(&form->ctrl->attr, DISPLAYFIELDS) ||
									CTRL_ATTR_CELL(DISPLAYFIELDS);
	int b_title = 0;

	/* No output in print mode */
	if(form->step == HtmlPrint) RETURN_OK;

	/* Read control attributes */
	CTRL_ATTR(ctrltree, CTRLTREE);

	switch(form->step)
	{
	case HtmlEdit:
	case HtmlView:
	case HtmlPrint:
	case HtmlSaveDlg:
	case FormList: 
	case FormSearch:
	case FormStats: 
	case FormValues:
		/* Set title default attributes values */
		if(!ctrl->TABLERULES[0]) ctrl->TABLERULES = "none";
		if(!ctrl->TABLEWIDTH[0]) ctrl->TABLEWIDTH = "100%";
		ctrl->POSITION = "_EVA_NewColumn";
		ctrl->NOTES = form->ctrl->NOTES;
		if(CTRL_ATTR_VAL(TITLE_LABEL)[0] != '1') ctrl->LABEL = form->ctrl->LABEL;
		if(!ctrl->TABLEBGCOLOR[0]) ctrl->TABLEBGCOLOR = DYNTAB_FIELD_VAL(&cntxt->cnf_data, TITLE_BGCOLOR);
		if(!ctrl->TABLEBACKGROUND[0]) ctrl->TABLEBACKGROUND = DYNTAB_FIELD_VAL(&cntxt->cnf_data, TITLE_BACKGROUND);

		/* Build title label & value */
		if(form->step == FormList || form->step == FormSearch)
		{
			/* List or search mode - use form label as value */
 			DYNBUF_ADD_STR(&label, "Liste des fiches");
 			DYNBUF_ADD(&title, ctrl->LABEL, 0, NO_CONV);
		}
		else
		{
			if(qry_obj_label(cntxt, &label, &notes, NULL, &title, &notes, &img, &imgsel, NULL, 0, NULL, 0))
				STACK_ERROR;
			b_title = title != NULL;
			if(!dyntab_sz(&form->id_obj, 0, 0) && b_data) DYNBUF_ADD_STR(&title, " (Nouvelle fiche)");
		}

		/* Handle form title */
		DYNBUF_ADD_BUF(&form->title, label, NO_CONV);
		if(label && title) DYNBUF_ADD_STR(&form->title, " - ");
		DYNBUF_ADD_BUF(&form->title, title, NO_CONV);

		/* Switch output to html_tiltle & output HTML table header */
		form->html = &form->html_title;
		if(ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;

		/* Output title button */
		DYNBUF_ADD_STR(form->html, "<td><table align=left cellspacing=0 cellpadding=0 border=0><tr><td valign=top>");
		if(!img)
		{
			DYNBUF_ADD_STR(&img, "_eva_btn_helpbook.gif");
			DYNBUF_ADD_STR(&imgsel, "_eva_btn_helpbook_s.gif");
		}
		if(html_put_open_btn(cntxt, NULL, label, title, notes, img, imgsel, 0, 0, 0, 0, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, "</td>");

		/* Output title text */
		ctrl->LABELPOS = "_EVA_NONE";
		ctrl->OPTIONBUTTON = "_EVA_NONE";
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		if(b_data && b_title)
		{
			DYNBUF_ADD_STR(form->html, "<font size=-2>");
			DYNBUF_ADD_BUF(form->html, label, TO_HTML);
			if(label && !strstr(label->data, form->ctrl->LABEL))
				DYNBUF_ADD3(form->html, " - ", form->ctrl->LABEL, 0, TO_HTML, "");
			DYNBUF_ADD_STR(form->html, "</font><br>");
			DYNBUF_ADD_BUF(form->html, title, TO_HTML);
		}
		else
		{
			DYNBUF_ADD_BUF(form->html, label, TO_HTML);
			DYNBUF_ADD_STR(form->html, " ");
			DYNBUF_ADD_BUF(form->html, title, TO_HTML);
		}
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, "</tr></table></td>\n");
		ctrl->LABEL = oldlabel;

		/* Add lower level controls */
		DYNBUF_ADD_STR(form->html, "<td align=right><table cellspacing=0 cellpadding=0 border=0><tr>\n");
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
		DYNBUF_ADD_STR(form->html, "</tr></table></td>\n");

		/* Output HTML table footer */
		if(ctrl_put_table_footer(cntxt, ctrl)) STACK_ERROR;
		form->html = html;
		break;

	default:
		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
