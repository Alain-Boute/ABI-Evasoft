/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
** File : html_table.c
** Description : utility functions for objects tables
** Author : Alain BOUTE
** Created : Feb 26 2002
**********************************************************************/

#include "eva.h"

# define LABEL_OR_FIELD_CELL(i) table_column_label(tbl, i)

/*********************************************************************
** Function : table_column_label
** Description : return label for a table column
*********************************************************************/
DynTableCell *table_column_label(		/* return : label cell */
	ObjTableFormat *tbl,				/* in : table to use */
	unsigned long i						/* in : table column index */
){
	static DynTableCell lbl = {0};
	static char txt[16];
	DynTableCell *c = dyntab_cell(&tbl->label, i, 0);
	if(c && c->len) return c;
	lbl.txt = txt;
	lbl.len = snprintf(add_sz_str(txt), "Colonne %lu", i);
	lbl.b_dontfreetxt = 1;
	return &lbl;
}

/*********************************************************************
** Function : table_init_srchcol
** Description : initialize search columns for a table
*********************************************************************/
void table_init_srchcol(				/* return : 0 on success, other on error */
	ObjTableFormat *tbl					/* in : table to initialize */
){
	unsigned long i;

	/* Initialize search columns with default fields if none */
	if(!tbl->srchcol)
	{
		DynTableCell *srchcol = DYNTAB_FIELD_CELL(tbl->attr, SEARCHCOL);
		if(srchcol)
		{
			char *num = srchcol->txt;
			while(num && *num)
			{
				unsigned long col = strtoul(num, &num, 10);
				tbl->srchcol |= 1 << (col ? (col - 1) : tbl->field.nbrows);
				if(*num) num++;
			}
		}
		if(!tbl->srchcol) tbl->srchcol = 1 << tbl->field.nbrows;
	}
	if(!tbl->srchcol) tbl->srchcol = 1 << tbl->field.nbrows;
	i = unique_bit_index(tbl->srchcol);
	tbl->srchfmt = i ? dyntab_val(&tbl->format, i - 1, 0) : "";
}

/*********************************************************************
** Function : table_process_controls
** Description : process button clicks & control inputs for list & relations tables
*********************************************************************/
#define ERR_FUNCTION "table_process_controls"
#define ERR_CLEANUP DYNTAB_FREE(list); \
					DYNTAB_FREE(formstamp); \
					DYNTAB_FREE(fields); \
					DYNTAB_FREE(values); \
					DYNTAB_FREE(id_obj); \
					M_FREE(name)
int table_process_controls(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable list = { 0 };
	DynTable formstamp = { 0 };
	DynTable fields = { 0 };
	DynTable values = { 0 };
	DynTable id_obj = { 0 };
	DynBuffer *name = NULL;
	unsigned long i, row;

	/* If a control button was clicked */
	if(form->i_ctrl_clic == i_ctrl)
	{
		/* Add new object if add button */
		char *btn = CGI_CLICK_BTN_SUBFIELD;
		tbl->ctrlline = cntxt->cgi[cntxt->cgibtn].Line;
		if(!tbl->input) tbl->input = dyntab_val(&tbl->cgiinput, 0, 0);

		if(!strcmp(btn, "ADDNEWOBJ"))
		{
			DynTableCell *val;
			DynTableCell *inputfield = DYNTAB_FIELD_CELL(tbl->attr, DISPLAYFIELDS);

			/* Get create parameters */
			CTRL_ATTR(formstamp, LIST_CREATE_FORM);
			if(!formstamp.nbrows) CTRL_ATTR(formstamp, BASE_FORMS);
			CTRL_ATTR_TAB(fields, LIST_CREATE_FIELD);
			CTRL_ATTR_TAB(values, LIST_CREATE_VALUE);
			if((tbl->status & TblCtrl_openaddnew) && inputfield && formstamp.nbrows &&
				tbl->input && tbl->input[0])
			{
				/* Prepare object data */
				DYNTAB_SET(&list, 0, 2, "V");
				DYNTAB_ADD(&list, 0, 1, tbl->input, 0, NO_CONV);
				DYNTAB_SET_CELLP(&list, 0, 0, inputfield);
				for(i = 0; i < fields.nbrows; i++)
				{
					row = list.nbrows;
					DYNTAB_SET(&list, row, 2, "V");
					DYNTAB_SET_CELL(&list, row, 1, &values, i, 0);
					DYNTAB_SET_CELL(&list, row, 0, &fields, i, 0);
				}

				/* Create new object with given formstamp */
				if(qry_add_obj_data(cntxt, &id_obj, &list, &formstamp, 0)) STACK_ERROR;

				/* Add new object to the relation at end of list */
				row = ctrl->val.nbrows;
				DYNTAB_ADD_CELL(&ctrl->val, row, 0, &id_obj, 0, 0);
				val = dyntab_cell(&ctrl->val, row, 0);
				val->b_modified = 1;
				val->i_cgi = cntxt->nb_cgi;
				ctrl->b_modified = 1;

				/* Clear input value in CGI data to avoid multiple creation */
				tbl->input [0] = 0;
			}
		}
		
		/* Handle status buttons */
		else if(!strcmp(btn, "SEARCHBEGIN")) tbl->srchmode = Begin; 
		else if(!strcmp(btn, "SEARCHEXACT")) tbl->srchmode = Exact; 
		else if(!strcmp(btn, "SEARCHCONTAIN")) tbl->srchmode = Contain; 

		/* Handle status buttons */
		else if(!strcmp(btn, "OPENSEARCH")) 
		{
			if(tbl->status & TblCtrl_search) tbl->status |= TblCtrl_opensearch; 
			if(tbl->status & TblCtrl_sel) tbl->status |= TblCtrl_opensel; 
			tbl->status &= ~TblCtrl_openaddnew;
		}
		else if(!strcmp(btn, "CLOSESEARCH")) 
		{
			tbl->status &= ~(TblCtrl_opensearch | TblCtrl_opensel); 
		}
		else if(!strcmp(btn, "OPENADDNEW"))
		{
			tbl->status ^= TblCtrl_openaddnew;
			if(tbl->status & TblCtrl_openaddnew) tbl->status &= ~(TblCtrl_opensearch | TblCtrl_opensel);
		}
		else if(!strcmp(btn, "TOP")) tbl->line = 0;
		else if(!strcmp(btn, "PAGEUP")) tbl->line = tbl->line > tbl->lines ? tbl->line - tbl->lines : 0;
		else if(!strcmp(btn, "LINEUP")) tbl->line = tbl->line ? tbl->line - 1 : 0;
		else if(!strcmp(btn, "LINEDOWN")) tbl->line++;
		else if(!strcmp(btn, "PAGEDOWN")) tbl->line += tbl->lines;
		else if(!strcmp(btn, "BOTTOM")) tbl->line = ~0UL;
		else if(!strcmp(btn, "SHRINK")) tbl->lines /= 2;
		else if(!strcmp(btn, "EXPAND")) tbl->lines *= 2;
		else if(!strncmp(btn, add_sz_str("PAGESIZE="))) tbl->lines = atoi(btn + 9);

		/* Handle Copy button */
		else if(!strcmp(btn, "COPY"))
		{
			/* Clear current clipboard values */
			unsigned long line = cntxt->cgi[cntxt->cgibtn].Line;
			if(cgi_filter_values(cntxt, &values, 'D', 0, 0, 0, "CB", NULL, 0, 0)) STACK_ERROR;
			for(i = 0; i < values.nbrows; i++) cgi_value_setkeep(cntxt, &values, i, 2);

			/* Add new values */
			for(i = 0; i < ctrl->val.nbrows; i++)
			{
				DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
				if(!line || line == c->Line)
				{
					M_FREE(name);
					DYNBUF_ADD3_INT(&name, "D0$0#CB$", c->Num, "");
					if(cgi_add_input(cntxt, NULL, DYNBUF_VAL_SZ(name), DYNTAB_VAL_SZ(&ctrl->val, i, 0))) STACK_ERROR;
				}
			}
		}

		/* Handle Paste button */
		else if(!strcmp(btn, "PASTE"))
		{
			unsigned long line = cntxt->cgi[cntxt->cgibtn].Line, num = 1;
			for(i = 0; i < ctrl->val.nbrows; i++)
			{
				DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
				if(!line || line == c->Line && num <= c->Num) num = c->Num + 1;
			}
			if(cgi_filter_values(cntxt, &values, 'D', 0, 0, 0, "CB", NULL, 0, 0)) STACK_ERROR;
			for(i = 0; i < values.nbrows; i++)
			{
				DynTableCell *c = dyntab_cell(&values, i, 0);
				DYNTAB_ADD_NL(&ctrl->val, ctrl->val.nbrows, 0, c->txt, c->len, i + num, line, 1);
				ctrl->b_modified = 1;
			}
			tbl->status &= ~(TblCtrl_opensearch | TblCtrl_opensel); 
		}

		/* Handle PRINT mode button */
		else if(!strcmp(btn, "PRINT"))
		{
			tbl->status ^= TblCtrl_printmode; 
		}

		/* Handle Export button */
		else if(!strncmp(btn, add_sz_str("EXPORT")))
		{
			table_init_srchcol(tbl);
			if(table_export_list(cntxt, i_ctrl)) STACK_ERROR;
		}

		/* Handle Clear button */
		else if(!strncmp(btn, add_sz_str("CLEAR")))
		{
			unsigned long line = cntxt->cgi[cntxt->cgibtn].Line;
			for(i = ctrl->val.nbrows; i; i--)
				if(!line || line == dyntab_cell(&ctrl->val, i - 1, 0)->Line)
				{
					cgi_setkeep(cntxt, dyntab_cell(&ctrl->val, i - 1, 0)->i_cgi, 1);
					dyntab_del_rows(&ctrl->val, i - 1, 1);
				}
			ctrl->b_modified = 1;
		}

		/* Handle user action button */
		else if(!strncmp(btn, add_sz_str("ACTBTN")))
		{
			EVA_form f = {0}, *f0 = cntxt->form;
			EVA_ctrl ctl = {0};
			cntxt->form = &f;
			f.ctrl = &ctl;
			f.nb_ctrl = 1;
			DYNTAB_ADD_INT(&f.id_form, 0, 0, dyntab_cell(&tbl->field, cntxt->cgi[cntxt->cgibtn].Num, 0)->IdObj);
			DYNTAB_ADD_INT(&f.id_obj, 0, 0, cntxt->cgi[cntxt->cgibtn].Line);
			if(qry_obj_data(cntxt, &ctl.attr, &f.id_form, NULL) ||
				qry_obj_data(cntxt, &f.objdata, &f.id_obj, NULL) ||
				ctrl_set_common_attr(cntxt, &ctl) ||
				ctrl_call_act_handler(cntxt, 0)) CLEAR_ERROR;
			DYNTAB_FREE(f.objdata);
			DYNTAB_FREE(ctl.attr);
			cntxt->form = f0;
		}

		/* Handle move buttons */
		else if(ctrl->val.nbrows &&! strncmp(btn, add_sz_str("MOVE")))
		{
			unsigned long dest;
			unsigned long src = cntxt->cgi[cntxt->cgibtn].Num - 1;

			/* Choose destination row */
			dest = src;
			if(!strcmp(btn, "MOVEUP")) dest = src ? src-1 : 0;
			else if(!strcmp(btn, "MOVETOP")) dest = 0;
			else if(!strcmp(btn, "MOVEDOWN")) dest = src < ctrl->val.nbrows-1 ? src+1 : src;
			else if(!strcmp(btn, "MOVEBOTTOM")) dest = ctrl->val.nbrows-1;

			/* Renumber & sort values */
			for(i = 0; i < ctrl->val.nbrows; i++)
			{
				DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
				unsigned long num = c->Num;
				c->Num = 1 + 
					(i == src ? dest : src > dest ? ((i >= dest && i < src) ? i+1 : i) : 
													((i <= dest && i > src) ? i-1 : i));
				if(num != c->Num) c->b_modified = 1;
			}
			dyntab_sort(&ctrl->val, qsort_ctrlval);
			ctrl->b_modified = 1;
		}

		/* Handle sort column buttons */
		else if(!strncmp(btn, add_sz_str("SORT")))
		{
			unsigned long sortcol = tbl->sortcol;
			int b_sortdesc = tbl->b_sortdesc;
			tbl->b_sortdesc = !strncmp(btn+4, add_sz_str("DESC"));
			tbl->sortcol = atoi(btn+(tbl->b_sortdesc ? 8 : 7));
			if(sortcol == tbl->sortcol && b_sortdesc == tbl->b_sortdesc)
				tbl->sortcol = ~0UL;
		}

		/* Handle search column buttons */
		else if(!strncmp(btn, add_sz_str("SEARCHCOL")))
		{
			unsigned long col = 1 << atoi(btn+9);
			tbl->srchcol = col == tbl->srchcol ? 0 : col;
		}
	}
	/* Close search if applicable */
	else
	{
		if(tbl->status & TblCtrl_sel &&	ctrl->val.nbrows &&
			CTRL_ATTR_CELL(AUTOCLOSE) && cntxt->cgi && cntxt->cgi[cntxt->cgibtn].name &&
				!strstr(cntxt->cgi[cntxt->cgibtn].name, "NOP"))
			tbl->status &= ~(TblCtrl_opensearch | TblCtrl_opensel);
	}

	/* Disable unused functions */
	if(!(tbl->status & TblCtrl_search)) tbl->status &= ~TblCtrl_opensearch;
	if(!(tbl->status & TblCtrl_addnew)) tbl->status &= ~TblCtrl_openaddnew;
	if(!(tbl->status & TblCtrl_sel)) tbl->status &= ~TblCtrl_opensel;

	/* Initialize search columns */
	table_init_srchcol(tbl);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_add_sel_chkbox
** Description : output a selection checkbox for an object
*********************************************************************/
#define ERR_FUNCTION "table_add_sel_chkbox"
#define ERR_CLEANUP M_FREE(name)
int table_add_sel_chkbox(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl,						/* in : related control */
	unsigned long i_val,				/* in : associated control value index - checkbox is selected if not empty */
	char *selval, size_t selval_sz		/* in : checkbox value when selected */
){
	DynBuffer *name = NULL;
	DynTableCell *val = dyntab_cell(&ctrl->val, i_val, 0);
	CHECK_HTML_STATUS;

	/* Build checkbox name */
	if(ctrl_cgi_name(cntxt, ctrl, val, 0, &name, 0, NULL, 0)) STACK_ERROR;

	/* Output selection checkbox */
	DYNBUF_ADD_STR(html, "<td width=1");
	if(ctrl->objtbl && ctrl->objtbl->colbrk && ctrl->objtbl->colbrk <= ctrl->objtbl->field.nbrows)
		DYNBUF_ADD_STR(html, " rowspan=2 valign=top");
	DYNBUF_ADD_STR(html, ">");
	if(put_html_chkbox(cntxt,DYNBUF_VAL_SZ(name), selval, selval_sz, (val ? 1 : 0), 0) ||
		val && ctrl_put_hidden_old(cntxt, ctrl, i_val, name, NULL, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_add_searchsort_btn
** Description : output a search button for a column
*********************************************************************/
#define ERR_FUNCTION "table_add_searchsort_btn"
#define ERR_CLEANUP M_FREE(name); \
					M_FREE(subfield)
int table_add_searchsort_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl,						/* in : related control */
	unsigned long i,					/* in : column index */
	int b_search,						/* in : add search button if not 0 */
	int b_sort,							/* in : add sort buttons if not 0 */
	int b_invsort						/* in : invert sort buttons if not 0 */
){
	DynBuffer *val = NULL;
	DynBuffer *name = NULL;
	DynBuffer *subfield = NULL;
	ObjTableFormat *tbl = ctrl->objtbl;
	int j = unique_bit_index(tbl->srchcol);
	int b_sel = tbl->srchcol & (1 << i);
	CHECK_HTML_STATUS;

	/* Output search button if applicable */
	if(b_search)
	{
		DYNBUF_ADD3_INT(&subfield, "SEARCHCOL", i, "");
		CTRL_CGINAMEBTN(&name, NULL, DYNBUF_VAL_SZ(subfield));
		M_FREE(subfield);
		if(i < tbl->field.nbrows)
			DYNBUF_ADD3_CELLP(&val, "Chercher dans [", LABEL_OR_FIELD_CELL(i), NO_CONV, "]\n\n")
		else
			DYNBUF_ADD_STR(&val, "Chercher dans toutes les données des fiches\n\n");
		if(j && b_sel)
		{
			DYNBUF_ADD_STR(&val, "Recherche activée sur cette colonne\n\n"
				"Cliquez pour chercher dans les colonnes par défaut");
		}
		else if(b_sel)
		{
			DYNBUF_ADD_STR(&val, "Recherche activée sur cette colonne et sur d'autres colonnes\n\n"
				"Cliquez pour chercher uniquement dans cette colonne");
		}
		else
		{
			DYNBUF_ADD_STR(&val, "Recherche non activée sur cette colonne\n\n"
				"Cliquez pour chercher dans cette colonne");
		}
		if(b_sel ?
			put_html_button(cntxt, name->data, NULL, "_eva_search2_s.gif", "_eva_search2.gif", val->data, 0, 0) :
			put_html_button(cntxt, name->data, NULL, "_eva_search2.gif", "_eva_search2_s.gif", val->data, 0, 0))
				STACK_ERROR;
	}

	/* Output sort buttons */
	if(b_sort)
	{
		int b_sortdesc = b_invsort ? !tbl->b_sortdesc : tbl->b_sortdesc;
		M_FREE(val);
		if(!b_invsort) DYNBUF_ADD3_INT(&val, "SORTASC", i, "")
		else  DYNBUF_ADD3_INT(&val, "SORTDESC", i, "");
		CTRL_CGINAMEBTN(&name, NULL, DYNBUF_VAL_SZ(val));
		M_FREE(val);
		if(i < tbl->field.nbrows)
			DYNBUF_ADD3_CELLP(&val, "Trier [", LABEL_OR_FIELD_CELL(i), NO_CONV, "] par ordre croissant")
		else
			DYNBUF_ADD_STR(&val, "Trier par ordre de création des fiches (+ anciennes en 1er)\n\n");
		if(((tbl->sortcol >= tbl->field.nbrows ? tbl->field.nbrows : tbl->sortcol) == i && !b_sortdesc) ?
			put_html_button(cntxt, name->data, NULL, "_eva_sort_asc_s.gif", "_eva_sort_asc.gif", val->data, 0, 0) :
			put_html_button(cntxt, name->data, NULL, "_eva_sort_asc.gif", "_eva_sort_asc_s.gif", val->data, 0, 0))
				STACK_ERROR;
		M_FREE(val);
		if(b_invsort) DYNBUF_ADD3_INT(&val, "SORTASC", i, "")
		else  DYNBUF_ADD3_INT(&val, "SORTDESC", i, "");
		CTRL_CGINAMEBTN(&name, NULL, DYNBUF_VAL_SZ(val));
		M_FREE(val);
		if(i < tbl->field.nbrows)
			DYNBUF_ADD3_CELLP(&val, "Trier [", LABEL_OR_FIELD_CELL(i), NO_CONV, "] par ordre décroissant")
		else
			DYNBUF_ADD_STR(&val, "Trier par ordre de création des fiches (+ récentes en 1er)\n\n");
		if(((tbl->sortcol >= tbl->field.nbrows ? tbl->field.nbrows : tbl->sortcol) == i && b_sortdesc) ?
			put_html_button(cntxt, name->data, NULL, "_eva_sort_desc_s.gif", "_eva_sort_desc.gif", val->data, 0, 0) :
			put_html_button(cntxt, name->data, NULL, "_eva_sort_desc.gif", "_eva_sort_desc_s.gif", val->data, 0, 0))
				STACK_ERROR;
		M_FREE(val);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_add_updown_btns
** Description : output an open button for an object
*********************************************************************/
#define ERR_FUNCTION "table_add_updown_btns"
#define ERR_CLEANUP M_FREE(tooltip); \
					M_FREE(name)
int table_add_updown_btns(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl,						/* in : related control */
	unsigned long i_val,				/* in : control value */
	char *label, size_t sz_label,		/* in : object label for buttons tooltip */
	char *bg_color						/* in : output cell separators with given color if not NULL */
){
	DynTableCell *val = dyntab_cell(&ctrl->val, i_val, 0);
 	DynBuffer *tooltip = NULL;
	DynBuffer *name = NULL;
	int b_updown = (i_val ? 1 : 0) | (i_val < ctrl->val.nbrows - 1 ? 2 : 0);
	CHECK_HTML_STATUS;

	DYNBUF_ADD_STR(html, "<td");
	if(*bg_color && strlen(bg_color) == 6) DYNBUF_ADD3(html, " bgcolor=#", bg_color, 6, NO_CONV,"");
	DYNBUF_ADD_STR(html, " width=1 align=center valign=middle>");
	if(b_updown & 1 && cntxt->form->step != HtmlPrint)
	{
		/* Output top arrow button */
		M_FREE(name); M_FREE(tooltip);
		CTRL_CGINAMEBTN(&name, val, add_sz_str("MOVETOP"));
		DYNBUF_ADD3(&tooltip, "Monter [", label, sz_label, NO_CONV, "] en 1ère ligne")
		DYNBUF_ADD_STR(html, "<table cellspacing=0 cellpadding=0 border=0><tr><td>");
		if(put_html_button(cntxt, name->data, NULL,
				"_eva_btn_arrowt_small.gif", "_eva_btn_arrowt_s_small.gif",
				tooltip->data, 0, 0))
			STACK_ERROR;
		DYNBUF_ADD_STR(html, "</td>\n<td>");

		/* Output up arrow button */
		M_FREE(name); M_FREE(tooltip);
		CTRL_CGINAMEBTN(&name, val, add_sz_str("MOVEUP"));
		DYNBUF_ADD3(&tooltip, "Monter [", label, sz_label, NO_CONV, "] d'une ligne")
		if(put_html_button(cntxt, name->data, NULL,
				"_eva_btn_arrowu_small.gif", "_eva_btn_arrowu_s_small.gif",
				tooltip->data, 0, 0))
			STACK_ERROR;
		if(b_updown & 2) DYNBUF_ADD_STR(html, "</td>\n</tr><tr><td>")
		else DYNBUF_ADD_STR(html, "</td></tr></table>")
	}

	if(b_updown & 2 && cntxt->form->step != HtmlPrint)
	{
		/* Output bottom arrow button */
		M_FREE(name); M_FREE(tooltip);
		CTRL_CGINAMEBTN(&name, val, add_sz_str("MOVEBOTTOM"));
		DYNBUF_ADD3(&tooltip, "Descendre [", label, sz_label, NO_CONV, "] en dernière ligne")
		if(!(b_updown & 1)) DYNBUF_ADD_STR(html, "<table cellspacing=0 cellpadding=0 border=0><tr><td>");
		if(put_html_button(cntxt, name->data, NULL,
				"_eva_btn_arrowb_small.gif", "_eva_btn_arrowb_s_small.gif",
				tooltip->data, 0, 0))
			STACK_ERROR;
		M_FREE(name); M_FREE(tooltip);
		DYNBUF_ADD_STR(html, "</td>\n<td>");

		/* Output down arrow button */
		M_FREE(name); M_FREE(tooltip);
		CTRL_CGINAMEBTN(&name, val, add_sz_str("MOVEDOWN"));
		DYNBUF_ADD3(&tooltip, "Descendre [", label, sz_label, NO_CONV, "] d'une ligne")
		if(put_html_button(cntxt, name->data, NULL,
				"_eva_btn_arrowd_small.gif", "_eva_btn_arrowd_s_small.gif",
				tooltip->data, 0, 0))
			STACK_ERROR;
		M_FREE(name); M_FREE(tooltip);
		DYNBUF_ADD_STR(html, "</td></tr></table>");
	}
	DYNBUF_ADD_STR(html, "</td>\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_closesearch_btn
** Description : output close search button
*********************************************************************/
#define ERR_FUNCTION "table_put_closesearch_btn"
#define ERR_CLEANUP	M_FREE(name)
int table_put_closesearch_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *name = NULL;
	char *btn = tbl->status & TblCtrl_sel ? "_eva_btn_opensel.gif" : "_eva_searchoff.gif";
	char *btn1 = tbl->status & TblCtrl_sel ? "_eva_btn_opensel_s.gif" : "_eva_searchoff_s.gif";
	char *help = tbl->status & TblCtrl_sel ?
		"Cliquez pour voir les fiches sélectionnées et masquer la recherche" : 
		"Cliquez pour masquer la recherche et voir toutes les fiches";
	if(cntxt->form->step == HtmlPrint) RETURN_OK;

	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("CLOSESEARCH"));
	if(put_html_button(cntxt, name->data, "Fiches sélectionnées", btn, btn1, help, 0, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_opensearch_btn
** Description : output open search button
*********************************************************************/
#define ERR_FUNCTION "table_put_opensearch_btn"
#define ERR_CLEANUP	M_FREE(name)
int table_put_opensearch_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *tooltip						/* in : button tooltip */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;
	if(cntxt->form->step == HtmlPrint) return 0;

	if(ctrl->objtbl && !(ctrl->objtbl->status & TblCtrl_search)) RETURN_OK;
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("OPENSEARCH"));
	if(put_html_button(cntxt, name->data, "Chercher", "_eva_search.gif", "_eva_search_s.gif", 
		tooltip ? tooltip : "Cliquez pour chercher dans la liste des fiches", 0, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_export_btn
** Description : output export list data button
*********************************************************************/
#define ERR_FUNCTION "table_put_export_btn"
#define ERR_CLEANUP	M_FREE(name)
int table_put_export_btn(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *name = NULL;
	char *b_export = DYNTAB_FIELD_VAL(tbl->attr, EXPORTLIST);
	CHECK_HTML_STATUS;

	/* If export available */
	if(cntxt->form->step == HtmlPrint || !(tbl && tbl->attr && *b_export && *b_export != '0')) RETURN_OK;

	/* Output export buttons */
	DYNBUF_ADD_STR(html, "<td align=right>");
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("EXPORT_EXCEL"));
	if(put_html_button(cntxt, name->data, "Exporter", "_eva_excel_icon_small.gif", "_eva_excel_icon_small_s.gif", 
		"Cliquez pour obtenir la liste des fiches au format Excel", 0, 0)) STACK_ERROR;
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("EXPORT_WORD"));
	if(put_html_button(cntxt, name->data, "Exporter", "_eva_word_icon_small.gif", "_eva_word_icon_small_s.gif", 
		"Cliquez pour obtenir la liste des fiches au format Word", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td>");

	/* Output print mode button
	DYNBUF_ADD_STR(html, "<td align=right>");
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("PRINT"));
	if(tbl->status & TblCtrl_printmode ?
		put_html_button(cntxt, name->data, NULL, "_eva_print_s.gif", "_eva_print.gif", 
			"Mode impression en cours\n\nCliquez pour afficher la liste en mode normal", 0, 0) : 
		put_html_button(cntxt, name->data, NULL, "_eva_print.gif", "_eva_print_s.gif", 
			"Mode impression\n\nCliquez pour copier / coller directement la liste depuis la page", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td>"); */

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_clear_btn
** Description : output clear values button
*********************************************************************/
#define ERR_FUNCTION "table_put_clear_btn"
#define ERR_CLEANUP	M_FREE(name)
int table_put_clear_btn(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;
	if(!ctrl->val.nbrows) RETURN_OK;
	CTRL_CGINAMEBTN(&name, dyntab_cell(&ctrl->val, 0, 0), add_sz_str("CLEAR"));
	if(put_html_button(cntxt, name->data,
		"Effacer", "_eva_cancel.gif", "_eva_cancel_s.gif",
		"Efface les valeurs de cette relation", 0, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_addnew_btn
** Description : output insert button
*********************************************************************/
#define ERR_FUNCTION "table_put_addnew_btn"
#define ERR_CLEANUP	M_FREE(name); \
					M_FREE(val)
int table_put_addnew_btn(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *bef, size_t bef_sz,			/* in : insert this text before button */
	char *aft, size_t aft_sz,			/* in : insert this text after button */
	int mode							/* in : button output mode */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *name = NULL;
	DynBuffer *val = NULL;
	char *insert = CTRL_ATTR_VAL(TABLEINSERT);
	unsigned long id_form = strtoul(DYNTAB_FIELD_VAL(tbl->attr, LIST_CREATE_FORM), NULL, 10);
	char *field = dyntab_val(&tbl->field, 0, 0);
	int b_oper = strpbrk(field, ".-()[] ,") != NULL;
	int b_direct = !strcmp(insert, "1");

	/* Return if no insert button */
	if(!*insert || !strcmp(insert, "0") || form->step == HtmlPrint) RETURN_OK;

	/* Return if insert button not applicable */
	if(!id_form) id_form = strtoul(DYNTAB_FIELD_VAL(tbl->attr, BASE_FORMS), NULL, 10);
	if(tbl->status & TblCtrl_sel && (form->step != HtmlEdit || !(ctrl->access & AccessEdit)) ||
			ctrl->val.nbrows && (!ctrl->MULTIPLE[0] || !strcmp("No", ctrl->MULTIPLE)) || 
			!id_form || b_direct && (!*field || b_oper)) RETURN_OK;
	
	/* Output separator */
	DYNBUF_ADD(form->html, bef, bef_sz, NO_CONV);

	if(b_direct && !mode)
	{
		/* Insert from table : add OPENADDNEW button */
		CTRL_CGINAMEBTN(&name, NULL, add_sz_str("OPENADDNEW"));
		if(put_html_button(cntxt, name->data, "Ajouter", "_eva_plus.gif", "_eva_plus_s.gif", 
						"Cliquez pour ajouter une nouvelle fiche à la liste", 0, 0)) STACK_ERROR;
	}
	else
	{
		/* Insert with new form : add OPENOBJ button */
		DYNBUF_ADD3_INT(&val, "OPENOBJ=0,", id_form, ",0");
		CTRL_CGINAMEBTN(&name, dyntab_cell(&ctrl->val, 0, 0), DYNBUF_VAL_SZ(val));
		if(put_html_button(cntxt, name->data, "Nouvelle fiche", "_eva_plus.gif", "_eva_plus_s.gif",
						"Ouvre une nouvelle fiche à ajouter à la liste", 0, mode)) STACK_ERROR;
	}
	
	/* Output separator */
	DYNBUF_ADD(form->html, aft, aft_sz, NO_CONV);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_action_button
** Description : output user action button
*********************************************************************/
#define ERR_FUNCTION "table_put_action_button"
#define ERR_CLEANUP	M_FREE(name)
int table_put_action_button(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	unsigned long idobj,				/* in : object to apply button on */
	unsigned long ifld					/* in : button column position */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *name = NULL;
	DynTableCell v = {0};
	if(!idobj) RETURN_OK;
	v.Num = ifld;
	v.Line = idobj;
	CTRL_CGINAMEBTN(&name, &v, add_sz_str("ACTBTN"));
	if(put_html_button(cntxt, name->data,
		dyntab_val(&tbl->label, ifld, 0), NULL, NULL,
		dyntab_val(&tbl->notes, ifld, 0), 0, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_functions_btn
** Description : output copy, paste, insert, fullscreen & other buttons
*********************************************************************/
#define ERR_FUNCTION "table_put_functions_btn"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					M_FREE(name)
int table_put_functions_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *separ, size_t separ_sz		/* in : buttons separator */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable cgival = {0};
	DynBuffer *name = NULL;
	if(form->step == HtmlPrint) RETURN_OK;

	/* Output copy / paste buttons */
	if(*CTRL_ATTR_VAL(COPYPASTE) == '1')
	{
		if(ctrl->val.nbrows)
		{
			CTRL_CGINAMEBTN(&name, dyntab_cell(&ctrl->val, 0, 0), add_sz_str("COPY"));
			DYNBUF_ADD(form->html, separ, separ_sz, NO_CONV);
			if(put_html_button(cntxt, name->data,
				NULL, "btn_copy_small1.gif", "btn_copy_small1_s.gif",
				"Copier\n\nPlace les valeurs de cette relation dans le presse papier", 0, 0)) STACK_ERROR;
		}
		if(tbl->status & TblCtrl_sel && cntxt->form->step == HtmlEdit &&
			cgi_filter_values(cntxt, &cgival, 'D', 0, 0, 0, "CB", NULL, 0, 0)) STACK_ERROR;
		if(cgival.nbrows) 
		{
			CTRL_CGINAMEBTN(&name, dyntab_cell(&ctrl->val, 0, 0), add_sz_str("PASTE"));
			DYNBUF_ADD(form->html, separ, separ_sz, NO_CONV);
			if(put_html_button(cntxt, name->data,
				NULL, "btn_paste_small1.gif", "btn_paste_small1_s.gif",
				"Coller\n\nAjoute le contenu du presse papier aux valeurs de cette relation", 0, 0)) STACK_ERROR;
		}
	}

	/* Output insert button if applicable */
	if(table_put_addnew_btn(cntxt, i_ctrl, separ, separ_sz, NULL, 0, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_search_input
** Description : output text input for search & new object
*********************************************************************/
#define ERR_FUNCTION "table_put_search_input"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					M_FREE(name); \
					M_FREE(label); \
					M_FREE(tooltip); \
					M_FREE(subfield)
int table_put_search_input(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int *b_label						/* in : output label if not 0
										  out : 1 if HTML input was output */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable cgival = {0};
	DynBuffer *name = NULL;
	DynBuffer *label = NULL;
	DynBuffer *tooltip = NULL;
	DynBuffer *subfield = NULL;
	unsigned long srchcol1 = unique_bit_index(tbl->srchcol);
	unsigned long i;
	int blabel = !b_label || *b_label, b_onecol = srchcol1;
	CHECK_HTML_STATUS;
	if(b_label) *b_label = 0;

	/* Return if external controls or print mode with no search string */
	if(tbl->status & TblCtrl_extsearch || !*tbl->input && form->step == HtmlPrint) RETURN_OK;

	/* Build text label & help button tooltip */
	if(tbl->status & TblCtrl_openaddnew)
	{
		/* Add new object button */
		DYNBUF_ADD_STR(&label, "Ajouter");
		DYNBUF_ADD_STR(&subfield, "ADDNEWOBJ");
		DYNBUF_ADD3_CELLP(&tooltip, "Entrez une valeur pour le champ [",
			LABEL_OR_FIELD_CELL(0), NO_CONV, 
			"] et cliquez sur Ok pour créer une nouvelle fiche et l'ajouter à la liste");
	}
	else 
	{
		int b_words = 1;
		DYNBUF_ADD_STR(&subfield, "SEARCH");
		if(b_onecol)
		{
			srchcol1--;
			if(srchcol1 < tbl->field.nbrows)
			{
				/* Search on single field */
				DYNBUF_ADD_CELLP(&label, dyntab_cell(&tbl->label, srchcol1, 0), NO_CONV);
				if(!strcmp(dyntab_val(&tbl->format, srchcol1, 0), "_EVA_AGE"))
				{
					/* Search on single age field */
					b_words = 0;
					DYNBUF_ADD_STR(&tooltip, "Entrer un age ou une tranche d'age (2 nombres séparés par un tiret)\n\n"
						"exemples :\n"
						"15-18 : 15 à 18 ans inclus\n"
						"15 : 15 ans\n"
						"15- : 15 ans et +\n"
						"-15 : 15 ans et moins");
				}
				else if(!strncmp(dyntab_val(&tbl->format, srchcol1, 0), add_sz_str("_EVA_DATE")))
				{
					/* Search on single date field */
					b_words = 0;
					DYNBUF_ADD_STR(&tooltip, "Recherche de dates\n"
											" Entrer une date ou une période (2 dates séparées par un tiret)\n"
											"Exemple : 1/1-1/4 pour le 1er trimestre de l'année en cours")
				}
				else
					DYNBUF_ADD_STR(&tooltip, "Entrer le texte à chercher")
			}
			else 
			{
				/* Search on all object fields */
				DYNBUF_ADD_STR(&label, "Chercher");
				DYNBUF_ADD_STR(&tooltip, "Cherche sur tous les champs des fiches");
			}
		}
		else
		{
			/* Search on multiple fields */
			int b_first = 1;
			unsigned long i;
			DYNBUF_ADD_STR(&label, "Chercher");
			DYNBUF_ADD_STR(&tooltip, "Cherche sur les champs [");
			for(i = 0; i < tbl->field.nbrows; i++) if(tbl->srchcol & ( 1 << i))
			{
				if(!b_first) DYNBUF_ADD_STR(&tooltip, ",")
				else b_first = 0;
				DYNBUF_ADD_CELLP(&tooltip, LABEL_OR_FIELD_CELL(i), NO_CONV);
			}
			DYNBUF_ADD_STR(&tooltip, "]");
		}
		if(b_words) DYNBUF_ADD_STR(&tooltip,
			"\n\n"
			"Si vous entrez plusieurs mots, ils sont recherchés individuellement et seules les fiches contenant tous les mots sont affichées\n\n"
			"Pour chercher un texte contenant un espace, entourez le avec des doubles guillements");
	}

	/* Output label, text input & Ok button */
	if(b_label) *b_label = 1;
	if(blabel) DYNBUF_ADD3_BUF(html, "<td align=right valign=middle>&nbsp;", label, TO_XML, "</td><td valign=middle><nobr>");
	CTRL_CGINAMESUBFIELD(&name, NULL, "INPUT");
	if(put_html_text_input(cntxt, DYNBUF_VAL_SZ(name), tbl->input, strlen(tbl->input), 0, 0, 1, tbl->inputwidth, 0)) STACK_ERROR;
	if(ctrl_cgi_name(cntxt, ctrl, NULL, 0, &name, 'B', DYNBUF_VAL_SZ(subfield))) RETURN_ERR_MEMORY;
	if(put_html_button(cntxt, name->data, "Ok", NULL, NULL, tooltip->data, 0, 0)) STACK_ERROR;
	for(i = 0; i < tbl->cgiinput.nbrows; i++) cgi_value_setkeep(cntxt, &tbl->cgiinput, i, 1);

	/* Output exact search button if applicable */
	if(tbl->status & TblCtrl_opensearch && 
		*tbl->input && (!srchcol1 ||
						strncmp(dyntab_val(&tbl->format, srchcol1, 0), add_sz_str("_EVA_DATE")) &&
						strcmp(dyntab_val(&tbl->format, srchcol1, 0), "_EVA_AGE")))
	{
		if(tbl->srchmode == Exact)
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("SEARCHBEGIN"));
			if(put_html_button(cntxt, name->data, NULL, "_eva_srchmode_exact.gif", "_eva_srchmode_begin.gif", 
									"Augmente le nombre de réponses en recherchant les valeurs qui commencent par les mots donnés", 0, 4)) STACK_ERROR;
		}
		else if(tbl->srchmode == Begin)
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("SEARCHCONTAIN"));
			if(put_html_button(cntxt, name->data, NULL, "_eva_srchmode_begin.gif", "_eva_srchmode_contain.gif", 
									"Augmente le nombre de réponses en recherchant les valeurs qui contiennent les mots donnés", 0, 4)) STACK_ERROR;
		}
		else
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("SEARCHEXACT"));
			if(put_html_button(cntxt, name->data, NULL, "_eva_srchmode_contain.gif", "_eva_srchmode_exact.gif", 
									"Diminue le nombre de réponses en recherchant exactement les mots donnés", 0, 4)) STACK_ERROR;
		}
	}

	if(blabel) DYNBUF_ADD_STR(html, "</td>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_filter_list
** Description : output extra filters list
*********************************************************************/
#define ERR_FUNCTION "table_put_filter_list"
#define ERR_CLEANUP	M_FREE(name); \
					DYNTAB_FREE(optlist); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(cgival)
int table_put_filter_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int *b_label						/* in : output label if not 0
										  out : 1 if HTML input was output */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *name = NULL;
	DynTable optlist = { 0 };
	DynTable data = { 0 };
	DynTable cgival = { 0 };
	unsigned long val_LABEL;
	unsigned long i, row = 0;
	char *lbl_noflt = CTRL_ATTR_VAL(LABEL_NOFILTER);
	int blabel = !b_label || *b_label;
	CHECK_HTML_STATUS;
	if(b_label) *b_label = 0;

	/* User defined [No filter] option if present */
	if(!lbl_noflt || !*lbl_noflt) lbl_noflt = "Pas de filtre";
	if(dyntab_set_txt(&optlist, 0, 1, lbl_noflt, 0)) RETURN_ERR_MEMORY;

	/* Get filters selected by user & prepare list of user selectable filters */
	DYNTAB_SET(&optlist, 0, 0, "");
	val_LABEL = get_id_value(cntxt, add_sz_str("_EVA_LABEL"));
	for(i = 0; i < tbl->altfilter.nbrows; i++) if(!dyntab_sz(&tbl->typfilter, i, 0))
	{
		row = optlist.nbrows;
		if(qry_obj_idfield(cntxt, &data, DYNTAB_TOULRC(&tbl->altfilter, i, 0), val_LABEL)) STACK_ERROR;
		DYNTAB_ADD_CELL(&optlist, row, 1, &data, 0, 0);
		DYNTAB_ADD_CELL(&optlist, row, 0, &tbl->altfilter, i, 0);
	}

	/* Return if list must not be output */
	if(!tbl->altfilter.nbrows ||
		(!(tbl->status & TblCtrl_opensel) && tbl->status & TblCtrl_sel) ||
		!tbl->idobj.nbrows && !tbl->selfilter.nbrows ||
		optlist.nbrows < 2) RETURN_OK;

	/* Output filters list */
	CTRL_GET_CGI_SUBFIELD("SELFILTER")
	CTRL_CGINAMESUBFIELD(&name, NULL, "SELFILTER");
	if(blabel) DYNBUF_ADD_STR(html, "<td align=right valign=middle>&nbsp;Filtrer</td><td valign=middle>");
	if(put_html_list(cntxt, name, &optlist, &tbl->selfilter, 1, 0, 0, 1, 1, NULL)) STACK_ERROR;
	if(blabel) DYNBUF_ADD_STR(html, "</td>");
	if(b_label) *b_label = 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_page_navigator
** Description : output a table's page navigation buttons
*********************************************************************/
#define ERR_FUNCTION "table_put_page_navigator"
#define ERR_CLEANUP M_FREE(name)
int table_put_page_navigator(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int options							/* in : display options - bit mask
												bit 0 : output partial list indicator if set */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *name = NULL;
	unsigned long nbpages = !tbl->totlines ? 1 : ((tbl->totlines - 1) / tbl->lines) + 1;
	unsigned long page = !tbl->line ? 1 : (tbl->line + tbl->lines - 1) / tbl->lines + 1;
	char *counttype = "";
	DynTableCell *lbl = NULL;
	char *txtpagectrl = CTRL_ATTR_VAL(PAGE_CTRL);
	unsigned long pagectrl = *txtpagectrl ? strtoul(txtpagectrl, NULL, 10) : 7;
	CHECK_HTML_STATUS;

	/* Handle no page control */
	if(!pagectrl) RETURN_OK;

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- Start TablePageNavigator ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	/* Handle search / select labels */
	if(!strcmp(ctrl->CONTROL, "_EVA_INPUT") && !(tbl->status & TblCtrl_opensel))
	{
		lbl = !tbl->totlines ? CTRL_ATTR_CELL(LABEL_NOSEL) : NULL;
		counttype = "fiche";
	}
	else
	{
		lbl = !tbl->totlines ? CTRL_ATTR_CELL(LABEL_NOMATCH) : NULL;
		counttype = "fiche";
	}

	/* Output enclosing table header */
	DYNBUF_ADD3_INT(html,
		"<td colspan=99><table cellspacing=0 cellpadding=0 border=", 0, " rules=none width=100%><tr>");

	/* Output partial list indicator */
	if(nbpages > 1 && (options & 1) && pagectrl) 
	{
		DYNBUF_ADD_STR(html, "<td>");
		if(put_html_image(cntxt, NULL, "_eva_morepage.gif",
			"La liste n'affiche pas toutes les fiches\n"
			"Utilisez les boutons à droite pour changer de page ou le nombre de fiches par page", 
			NULL, NULL, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</td>");
	}

	/* Output record & page count */
	if(!(options & 1) || (pagectrl & 6))
	{
		DYNBUF_ADD_STR(html, "<td><font size=-1><b><nobr>");
		switch(tbl->totlines) 
		{
		case 0:
			if((!tbl->input || !tbl->input[0]) && tbl->minsearchlen && (tbl->status & TblCtrl_opensearch) )
			{
				/* Opened search with no input and min search len */
				DYNBUF_ADD_STR(html, "?");
				if(tbl->minsearchlen > 1)
					DYNBUF_ADD3_INT(html, " (", tbl->minsearchlen, " car)")
			}
			else if(lbl) DYNBUF_ADD_CELLP(html, lbl, TO_HTML)
			else DYNBUF_ADD3(html, "0 ", counttype, 0, NO_CONV, "");
			break;

		case 1:
			DYNBUF_ADD3(html, "1 ", counttype, 0, NO_CONV, "")
			break;

		default:
			DYNBUF_ADD_INT(html, tbl->totlines);
			DYNBUF_ADD_STR(html, " ");
			if(*counttype) DYNBUF_ADD3(html, " ", counttype, 0, NO_CONV, "s");
			if(nbpages > 1) 
			{
				DYNBUF_ADD3_INT(html, " -</nobr> <nobr><small>p ", page, " / ");
				DYNBUF_ADD_INT(html, nbpages);
			DYNBUF_ADD_STR(html, "</small>");
			}
		}
		DYNBUF_ADD_STR(html, "</nobr></b></font></td>");
	}


	/* Output navigator buttons */
	if(nbpages > 1 && ((options & 1) || (pagectrl & 6)))
	{
		/* Output previous page buttons */
		DYNBUF_ADD_STR(html, "<td><nobr>");
		if(tbl->line)
		{
			/* Top of table */
			if(nbpages > 2) 
			{
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("TOP"));
				if(put_html_button(cntxt, name->data, "Début", "_eva_btn_arrowl1_small.gif", "_eva_btn_arrowl1_small_s.gif",
													"Remonte en haut de la table", 0, 4)) STACK_ERROR;
			}

			/* Previous line */
			if(pagectrl & 8)
			{
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("LINEUP"));
				if(put_html_button(cntxt, name->data, "Ligne précédente", "_eva_btn_arrowl_small.gif", "_eva_btn_arrowl_small_s.gif",
													"Remonte d'une ligne dans la table", 0, 4)) STACK_ERROR;
			}

			/* Previous page */
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("PAGEUP"));
			if(put_html_button(cntxt, name->data, "Page précédente",
					(pagectrl & 4) ? "_eva_pgprev.gif" : "_eva_pgprv.gif",
					(pagectrl & 4) ? "_eva_pgprev_s.gif" : "_eva_pgprv_s.gif",
					"Remonte d'une page dans la table", 0, 4)) STACK_ERROR;
		}
		else
		{
			/* Disabled previous page */
			if(put_html_image(cntxt, NULL,
							(pagectrl & 4) ? "_eva_pgprev_d.gif" : "_eva_pgprv_d.gif",
							"Page précédente désactivée : début de liste", NULL, NULL, 0)) STACK_ERROR;
		}

		/* Allow break between previon & next controls if large images */
		if(pagectrl & 4) DYNBUF_ADD_STR(html, "</nobr> <nobr>");

		/* Output next page buttons */
		if(tbl->line < tbl->totlines - tbl->lines)
		{
			/* Next page */
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("PAGEDOWN"));
			if(put_html_button(cntxt, name->data, "Page suivante",
					(pagectrl & 4) ? "_eva_pgnext.gif" : "_eva_pgnxt.gif",
					(pagectrl & 4) ? "_eva_pgnext_s.gif" : "_eva_pgnxt_s.gif",
					"Descend d'une page dans la table", 0, 4)) STACK_ERROR;

			/* Next line */
			if(pagectrl & 8)
			{
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("LINEDOWN"));
				if(put_html_button(cntxt, name->data, "Ligne suivante", "_eva_btn_arrowr_small.gif", "_eva_btn_arrowr_small_s.gif",
													"Descend d'une ligne dans la table", 0, 4)) STACK_ERROR;
			}

			/* Bottom of table */
			if(nbpages > 2) 
			{
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("BOTTOM"));
				if(put_html_button(cntxt, name->data, "Fin", "_eva_btn_arrowr1_small.gif", "_eva_btn_arrowr1_small_s.gif",
													"Descend en bas de la table", 0, 4)) STACK_ERROR;
			}
		}
		else
		{
			/* Disabled next page */
			if(put_html_image(cntxt, NULL,
							(pagectrl & 4) ? "_eva_pgnext_d.gif" : "_eva_pgnxt_d.gif",
							"Page suivante désactivée : fin de liste", NULL, NULL, 0)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "</nobr></td>");
	}

	/* Output page size buttons */
#define PAGESIZE_BTN(n) { \
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("PAGESIZE=" #n)); \
	if(put_html_button(cntxt, name->data, NULL, (n == tbl->lines ? "_eva_btn_" #n "_s.gif" : "_eva_btn_" #n ".gif"), \
												(n == tbl->lines ? "_eva_btn_" #n ".gif" : "_eva_btn_" #n "_s.gif"), \
												tbl->lines == n ? #n " lignes affichées par page" : "Affiche " #n " lignes par page", 0, \
												tbl->lines == n ? 1 : 4)) STACK_ERROR; }
	if((options & 1) || (pagectrl & 6))
	{
		DYNBUF_ADD_STR(html, "<td align=right>");
		if(tbl->totlines > 2 && tbl->lines > 2) 
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("SHRINK"));
			if(put_html_button(cntxt, name->data, NULL, "_eva_btn_shrink.gif", "_eva_btn_shrink_s.gif",
												"Réduit le nombre de lignes affichées", 0, 4)) STACK_ERROR;
		}
		if(pagectrl & 6)
		{
			if(tbl->totlines > 2) 
			{
				PAGESIZE_BTN(2)
				PAGESIZE_BTN(5)
			}
			if(tbl->totlines > 5) PAGESIZE_BTN(10)
			if(tbl->totlines > 10) PAGESIZE_BTN(30)
			if(tbl->totlines > 30) PAGESIZE_BTN(100)
		}
		if(nbpages > 1) 
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("EXPAND"));
			if(put_html_button(cntxt, name->data, NULL, "_eva_btn_expand.gif", "_eva_btn_expand_s.gif",
												"Augmente le nombre de lignes affichées", 0, 4)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "</td>");
	}
#undef PAGESIZE_BTN

	/* Output export buttons */
	if(tbl->totlines && table_put_export_btn(cntxt, i_ctrl)) STACK_ERROR;

	/* Output table footer */
	DYNBUF_ADD_STR(html, "</tr></table></td>\n");

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- EndOf TablePageNavigator ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_controls
** Description : output search, add new object buttons & text input control
*********************************************************************/
#define ERR_FUNCTION "table_put_controls"
#define ERR_CLEANUP 
int table_put_controls(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int b_shrink						/* in : output in shrinked mode if not 0 */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	int b_opensearch = !(tbl->status & (TblCtrl_opensearch | TblCtrl_opensel)) && tbl->status & (TblCtrl_search | TblCtrl_sel);
	CHECK_HTML_STATUS;

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- Start TableControls ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	if(b_shrink)
	{
		/* Output enclosing table header & Add OPENSEARCH button */
		DYNBUF_ADD_STR(html, "<td width=10><table cellspacing=0 cellpadding=0 border=0 rules=none width=100%><tr><td width=10>");
		if(b_opensearch && table_put_opensearch_btn(cntxt, i_ctrl, NULL)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</td>");
		if(!tbl->totlines && (table_put_filter_list(cntxt, i_ctrl, NULL)) ||
								table_put_page_navigator(cntxt, i_ctrl, 0)) STACK_ERROR;
	}
	else
	{
		int b_label = 0, nblabel = 0;
		char *pagectl = CTRL_ATTR_VAL(PAGE_CTRL);

		/* Output enclosing table header */
		DYNBUF_ADD3_INT(html, "<td><table cellspacing=0 cellpadding=0 border=", 0, " rules=none width=100%><tr><td>");

		/* Add CLOSESEARCH / OPENSEARCH button */
		if(!(tbl->status & TblCtrl_extsearch) || (tbl->status & TblCtrl_sel)) 
			if(b_opensearch ? 
				table_put_opensearch_btn(cntxt, i_ctrl, NULL) :
				(tbl->status & (TblCtrl_search | TblCtrl_sel) &&
					table_put_closesearch_btn(cntxt, i_ctrl))) STACK_ERROR

		/* Add function buttons */
		if(table_put_functions_btn(cntxt, i_ctrl, NULL, 0)) STACK_ERROR;

		/* Add CLEAR button */
		if(tbl->status & TblCtrl_sel && form->step == HtmlEdit &&
			table_put_clear_btn(cntxt, i_ctrl)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</td>");

		/* Output extra filters list if applicable */
		if(tbl->altfilter.nbrows)
		{
			b_label = 1;
			if(table_put_filter_list(cntxt, i_ctrl, &b_label)) STACK_ERROR;
			if(b_label) nblabel++;
		}

		/* Add text input for seach & add functions */
		if(tbl->status & (TblCtrl_opensearch | TblCtrl_openaddnew))
		{
			b_label = 1;
			if(table_put_search_input(cntxt, i_ctrl, &b_label)) STACK_ERROR;
			if(b_label) nblabel++;
		}
		
		/* Output navigation controls */
		if(tbl->totlines > tbl->lines && nblabel && (*pagectl == '7' || !*pagectl))
			DYNBUF_ADD_STR(html, "</tr><tr>");
		if(table_put_page_navigator(cntxt, i_ctrl, 0)) STACK_ERROR;
	}

	/* Output table footer */
	DYNBUF_ADD_STR(html, "</tr></table></td>");
	if(!b_shrink) DYNBUF_ADD_STR(html, "</tr><tr>");

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- EndOf TableControls ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_title_row
** Description : output a table title row (1st row)
*********************************************************************/
#define ERR_FUNCTION "table_put_title_row"
#define ERR_CLEANUP M_FREE(val); \
					M_FREE(name)
int table_put_title_row(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int titlemode						/* in : output mode for controls */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *val = NULL;
	DynBuffer *name = NULL;
	unsigned height = strtoul(DYNTAB_FIELD_VAL(tbl->attr, TITLE_HEIGHT), NULL, 10);
	unsigned long i, k;
	int b_search = tbl->status & TblCtrl_opensearch && form->step != HtmlPrint;
	int b_sort = (tbl->b_sortsel || tbl->status & TblCtrl_sort) && cntxt->form->step != HtmlPrint;
	CHECK_HTML_STATUS;
	if(!tbl->totlines && !(tbl->status & TblCtrl_opensearch)) RETURN_OK;

	/* Do not output if not applicable */
	if(!(tbl->label.nbrows || b_sort) || !strcmp(DYNTAB_FIELD_VAL(tbl->attr, TABLE_NOTITLESHRINK), "2"))
	{
		DYNBUF_ADD_STR(html, "</tr>");
		RETURN_OK; 
	}

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- Start TableTitleRow ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	/* Leave blank cells for selection checkbox & open button */
	if(tbl->status & TblCtrl_sel) DYNBUF_ADD_STR(html, "<th>&nbsp;</th>");
	if(tbl->status & TblCtrl_openbtn)
	{
		DYNBUF_ADD_STR(html, "<th><nobr>&nbsp;");
		if(titlemode != 4 && table_add_searchsort_btn(cntxt, ctrl, tbl->field.nbrows, b_search, b_sort, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</th>");
	}

	/* For each display field */
	k = (tbl->colbrk > 0 && tbl->colbrk <= tbl->field.nbrows) ? tbl->colbrk - 1 : tbl->field.nbrows;
	for(i = 0; i < k; i++)
	{
		char * style = dyntab_val(&tbl->lblstyle, i, 0);
		char * align = dyntab_val(&tbl->align, i, 0);

		/* Output title label */
		DYNBUF_ADD_STR(html, "<th");
		if(!strcmp(align, "center") || !strcmp(align, "right"))
			DYNBUF_ADD3(html, " align=", align, 0, NO_CONV, "");
		if(*style) DYNBUF_ADD3(html, " class=", style, 0, NO_CONV, ">")
		else DYNBUF_ADD_STR(html, "><font size=-1>");
		if(height < 3 && !*style) DYNBUF_ADD_STR(html, "<nobr>");
		DYNBUF_ADD_CELL(html, &tbl->label, i, 0, TO_HTML);
		if(height == 2) DYNBUF_ADD_STR(html, "<br><center>")
		else DYNBUF_ADD_STR(html, " ");

		/* Output search & sort buttons as applicable */
		if(titlemode != 4 && table_add_searchsort_btn(cntxt, ctrl, i, b_search, b_sort, !strcmp("_EVA_AGE", dyntab_val(&tbl->format, i, 0)))) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</th>\n");
	}
	DYNBUF_ADD_STR(html, "</tr>");

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- EndOf TableTitleRow ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP


/*********************************************************************
** Function : table_init_bgseq
** Description : initialize lines backgroung sequence data for table display
*********************************************************************/
#define ERR_FUNCTION "table_init_bgseq"
#define ERR_CLEANUP
int table_init_bgseq(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	ObjTableFormat *tbl					/* in : table to output */
){
	char *bgseq;
	int nbrows;
	if(!tbl) return 0;

	DYNTAB_FIELD_TABDEF(&tbl->bgcol, tbl->attr, TABLEROWCOLORS);
	DYNTAB_FIELD_TABDEF(&tbl->bgimg, tbl->attr, TABLEROWTRANSP);
	bgseq = dyntab_val(&tbl->bgimg, 0, 0);
	DYNTAB_FIELD_TABDEF(&tbl->bgimg, tbl->attr, TABLEROWBG);
	nbrows = max(tbl->bgcol.nbrows, tbl->bgimg.nbrows);

	/* Handle full automatic mode */
	if(!*bgseq && !tbl->bgcol.nbrows && !tbl->bgimg.nbrows)
	{
		/* Light green / white sequence */
		DYNTAB_SET(&tbl->bgcol, 0, 0, "DDFFEE");
		DYNTAB_SET(&tbl->bgcol, 1, 0, "FFFFFF");
	}
	else if(!strcmp(bgseq, "_EVA_LAST") || !*bgseq && nbrows <= 1)
	{
		DYNTAB_ADD_CELLP(&tbl->bgcol, max(tbl->bgcol.nbrows, tbl->bgimg.nbrows), 0, NULL);
	}
	else if(!strcmp(bgseq, "_EVA_FIRST"))
	{
		tbl->bgseqstart = nbrows;
		DYNTAB_SET(&tbl->bgcol, nbrows, 0, "");
	}
	if(!tbl->bgcol.nbrows) DYNTAB_SET(&tbl->bgcol, 0, 0, "");
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_row_bgcolor
** Description : return row background color for a given row
*********************************************************************/
char *table_row_bgcolor(				/* return : RGB color code - empty string for transparent */
	EVA_context *cntxt,					/* in/out : execution context data */
	ObjTableFormat *tbl,				/* in : table to output */
	unsigned long i,					/* in : row index to get color for */
	char *colorfield					/* in : color expression */
){
	int nbrows;
	DynTable data = {0};
	static char color[16] = {0};
	char *res;
	if(!tbl) return "";

	if(!tbl->bgcol.nbrows && !i) table_init_bgseq(cntxt, tbl);
	nbrows = max(tbl->bgcol.nbrows, tbl->bgimg.nbrows);
	res = nbrows ? dyntab_val(&tbl->bgcol, (i + tbl->bgseqstart) % nbrows,0) : "FFFFFF";

	if(colorfield && *colorfield)
	{
		form_eval_fieldexpr(cntxt, &data, 0, DYNTAB_TOULRC(&tbl->idobj, i, 0), colorfield, strlen(colorfield), NULL, 0);
		if(data.nbrows && data.cell && data.cell->len == 6)
		{
			snprintf(add_sz_str(color)-1, "%s", data.cell->txt);
			res = color;
		}
		DYNTAB_FREE(data);
	}
	return res;
}

/*********************************************************************
** Function : table_put_rows
** Description : output the body of a table of objects
*********************************************************************/
#define ERR_FUNCTION "table_put_rows"
#define ERR_CLEANUP	M_FREE(buf); \
					M_FREE(buf1); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(objdata); \
					DYNTAB_FREE(ctrldata); \
					DYNTAB_FREE(ctrldata1); \
					DYNTAB_FREE(colorfld)
int table_put_rows(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *buf = NULL;
	DynBuffer *buf1 = NULL;
	DynTable data = {0};
	DynTable objdata = {0};
	DynTable ctrldata = {0};
	DynTable ctrldata1 = {0};
	DynTable colorfld = {0};
	ObjTableFormat *tbl = ctrl->objtbl;
	char *openbtn = DYNTAB_FIELD_VAL(tbl->attr, OPENBUTTON);
	char *imgbtnfield = NULL, *imgdir = NULL;
	unsigned long imgbtn_maxh = 0, imgbtn_maxw = 0;
	char imgbtnpath[1024];
	unsigned long idcolor = strtoul(DYNTAB_FIELD_VAL(tbl->attr, BG_LINE_SPEC), NULL, 10);
	char *colorfield = NULL;
	int b_rowhilite = *CTRL_ATTR_VAL(DYNLINES) == '1' &&
						!(form->step == HtmlEdit && !strcmp(ctrl->CONTROL, "_EVA_INPUT"));
	char *alt = CTRL_ATTR_VAL(ALTFORM);
	int loc = strcmp(alt, "_EVA_REUSE") ? 0 : 3;
	size_t sz_imgbtnpath = 0;
	char opname[64] = {0};
	unsigned long fixform = strtoul(CTRL_ATTR_VAL(LIST_OPENFORM), NULL, 10);
	unsigned long baseform = strtoul(CTRL_ATTR_VAL(BASE_FORMS), NULL, 10);
	unsigned long opform = CTRL_ATTR_CELL(LIST_OPENFIXED) ? fixform ? fixform : baseform : 0;
	unsigned long i, j, k;
	unsigned long rowstart;
	unsigned long nbwords = tbl->srchwords.nbrows;
	int b_arrows = form->step == HtmlEdit && !tbl->b_sortsel && !(tbl->status & TblCtrl_opensel) && (tbl->status & TblCtrl_sel) && ctrl->val.nbrows > 1;
	CHECK_HTML_STATUS;

	/* Return if no rows */
	if(!tbl->idobj.nbrows) RETURN_OK;

	/* Remove previous <tr> if applicable */
	if(*html)
	{
		DynBuffer *h = *html;
		if(h->cnt > 4 && !strncmp(h->data + h->cnt - 4, add_sz_str("<tr>"))) h->cnt -= 4;
	}

	/* Output HTML comment if applicable */
	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- Start TableRows ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	/* Read color field for line background */
	if(idcolor)
	{
		if(qry_obj_field(cntxt, &ctrldata1, idcolor, NULL) || ctrl_read_baseobj(cntxt, &ctrldata1)) CLEAR_ERROR;
		DYNTAB_FIELD_TAB(&colorfld, &ctrldata1, FIELD);
		if(!colorfld.nbrows) DYNTAB_FIELD_TAB(&colorfld, &ctrldata1, SRCFIELD);
		if(qry_eval_var_expr_table(cntxt, &colorfld, tbl->attr)) CLEAR_ERROR;
		colorfield = dyntab_val(&colorfld, 0, 0);
	}

	/* Read image field for open button if applicable */
	if(!strcmp(openbtn, "_EVA_IMAGE"))
	{
		if(qry_obj_field(cntxt, &ctrldata, strtoul(DYNTAB_FIELD_VAL(tbl->attr, IMGBTN_CTRL), NULL, 10), NULL) || ctrl_read_baseobj(cntxt, &ctrldata)) CLEAR_ERROR;
		imgdir = DYNTAB_FIELD_VAL(&ctrldata, IMAGE_DIR);
		imgbtnfield = DYNTAB_FIELD_VAL(&ctrldata, FIELD);
		if(!*imgbtnfield) imgbtnfield = DYNTAB_FIELD_VAL(&ctrldata, SRCFIELD);

		/* Disable image open button if no field */
		if(!*imgbtnfield)
			openbtn = "";
		else
		{
			imgbtn_maxw = strtoul(DYNTAB_FIELD_VAL(&ctrldata, WIDTH), NULL, 10);
			imgbtn_maxh = strtoul(DYNTAB_FIELD_VAL(&ctrldata, HEIGHT), NULL, 10);
		}
	}

	for(i = 0; i < tbl->idobj.nbrows; i++)
	{
		char *bg_color = table_row_bgcolor(cntxt, tbl, i, colorfield);
		char *idend;
		unsigned long idobj = strtoul(dyntab_val(&tbl->idobj, i, 0), &idend, 10);
		unsigned long i_label = tbl->data.nbrows;
		unsigned long i_val = ctrl->val.nbrows;

		/* Check selection status */
		if(ctrl->val.nbrows && tbl->status & TblCtrl_sel)
		{  
			/* Search current line in selection */
			for(i_val = 0; i_val < ctrl->val.nbrows && dyntab_cmp(&ctrl->val, i_val, 0, &tbl->idobj, i, 0); i_val++);
			
			/* If found : mark cell with col = 1 */
			if(i_val < ctrl->val.nbrows) dyntab_cell(&ctrl->val, i_val, 0)->col = 1;
		}

		/* Output line start */
		DYNBUF_ADD_STR(html, "<tr");
		if(b_rowhilite)
		{
			build_open_btn_name(cntxt, opname, opform, idobj, loc, atoi(CTRL_ATTR_VAL(OPEN_MODE)));
			DYNBUF_ADD3(html, " onMouseOver=RowHL(this) opname='", opname, 0, NO_CONV, "'");
		}
		if(*bg_color && strlen(bg_color) == 6) DYNBUF_ADD3(html, " bgcolor=#", bg_color, 6, NO_CONV,"")
		DYNBUF_ADD_STR(html, ">");

		/* Output selection checkbox where applicable */
		if(tbl->status & TblCtrl_sel &&
				table_add_sel_chkbox(cntxt, ctrl, i_val, DYNTAB_VAL_SZ(&tbl->idobj, i, 0)))STACK_ERROR;


		/* If object is not a relation */
		if(!idobj || *idend)
		{
			DYNBUF_ADD3_INT(html, "<td>#NAR#</td><td colspan=", tbl->field.nbrows, ">");
			DYNBUF_ADD_CELL(html, &tbl->idobj, i, 0, TO_HTML);
			DYNBUF_ADD_STR(html, "</td>");
		}
		else
		{
			int b_firstcolbrk = 1;

			/* Find object in objects data */
			for(j = 0; j < tbl->data.nbrows && dyntab_cell(&tbl->data, j, 0)->IdObj != idobj; j++);
			rowstart = j;

			/* Output open button where applicable */
			if(tbl->status & TblCtrl_openbtn && strcmp(openbtn, "_EVA_NONE"))
			{
				unsigned long w = 0, h = 0;
				char *img = NULL;
				char *thumb = NULL;
				char *opbtn = openbtn;

				/* Output cell header */
				DYNBUF_ADD_STR(html, "<td");
				if(!*openbtn || !strcmp(openbtn, "_EVA_SYMBOL") || !strcmp(openbtn, "_EVA_IMAGE"))
					DYNBUF_ADD_STR(html, " align=right width=1%")
				if(tbl->colbrk && tbl->colbrk <= tbl->field.nbrows) DYNBUF_ADD_STR(html, " rowspan=2 valign=top");
				DYNBUF_ADD_STR(html, ">");

				/* Handle dynamic image button */
				if(!strcmp(openbtn, "_EVA_IMAGE"))
				{
					char *end = NULL;
					unsigned long idimg = 0;

					/* Compute image from field expression */
					if(form_eval_fieldexpr(cntxt, &data, 0, idobj, imgbtnfield, strlen(imgbtnfield), &tbl->data, rowstart)) CLEAR_ERROR;
					if(data.nbrows) img = dyntab_val(&data, 0, 0);
					
					/* Handle file link : (image file name is a number with no extension) */
					if(img) idimg = strtoul(img, &end, 10);
					if(idimg && end && !*end)
					{
						/* Get file path */
						if(qry_obj_field(cntxt, &data, idimg, "_EVA_FILE_SERVER_NAME")) STACK_ERROR;
						img = dyntab_val(&data, 0, 0);
						sz_imgbtnpath = snprintf(add_sz_str(imgbtnpath), "../docs/%s/", cntxt->dbname); 
					}
					else if(*imgdir)
						sz_imgbtnpath = snprintf(add_sz_str(imgbtnpath), "../docs/%s/%s/", cntxt->dbname, imgdir); 
					else
						sz_imgbtnpath = 0;

					/* Handle image if present */
					if(img)
					{
						/* Build image path */
						if(sz_imgbtnpath)
							snprintf(imgbtnpath + sz_imgbtnpath, sizeof(imgbtnpath) - sz_imgbtnpath, "%s", img);
						else
						{
							char *s = get_image_file(cntxt, img, NULL, NULL);
							strncpy(imgbtnpath, s, sizeof(imgbtnpath));
							M_FREE(s);
						}

						/* Handle image resizing */
						if(imgbtn_maxh || imgbtn_maxw)
						{
							/* Get image thumbnail */
							thumb = get_image_thumb(cntxt, imgbtnpath, sz_imgbtnpath, imgbtn_maxw, imgbtn_maxh, &w, &h);
						}
					}
					if(!(thumb || img)) opbtn = "SYMBOL";
				}

				/* If thumbnail or image available */
				if(thumb || img)
				{
					/* Build open button name */
					char name[64];
					DynTable *d = &tbl->data;
					unsigned long i_fc;
					M_FREE(buf); 
					M_FREE(buf1); 
					if(qry_obj_label(cntxt, NULL, NULL, &buf, NULL, &buf1, NULL, NULL, &i_fc, 0, d, rowstart)) CLEAR_ERROR;
					if(buf1) DYNBUF_ADD3_BUF(&buf, "\n", buf1, NO_CONV, "");
					build_open_btn_name(cntxt, name, opform ? opform : cntxt->fc[i_fc].idform, dyntab_cell(d, rowstart, 0)->IdObj, loc,
												atoi(CTRL_ATTR_VAL(OPEN_MODE)));
					/* Output image button */
					if(put_html_button_sz(cntxt, name, NULL, thumb ? thumb : imgbtnpath, NULL, buf ? buf->data : NULL, NULL, w, h, 0, 2))
						STACK_ERROR;
				}
				else
				{
					/* Output object symbol button */
					DYNBUF_ADD_STR(html, "<table border=0 cellspacing=0 cellpadding=0><tr>");
					if(ctrl_add_symbol_btn(cntxt, ctrl, dyntab_cell(&ctrl->val, i_val, 0),
											&tbl->data, j, "", opbtn))
						STACK_ERROR;
					DYNBUF_ADD_STR(html, "</tr></table>");
				}
				DYNBUF_ADD_STR(html, "</td>");
				
				/* Free alloc-ed path */
				if(thumb) free(thumb);
			}

			/* Output line data in tbl->field order */
			for(j = 0; j < tbl->field.nbrows; j++) 
			{
				char *fmt = dyntab_val(&tbl->format, j, 0);
				char *s_strip = dyntab_val(&tbl->strip, j, 0);
				unsigned long strip = atoi(s_strip);
				int b_stripped = 0;
				size_t tot_char = 0;
				char *valign = NULL;
				char *align = dyntab_val(&tbl->align, j, 0);
				char *separ = ", ";
				int b_index = 0;
				unsigned long line = 1;
				unsigned long i0;
				int b_distinct = dyntab_sz(&tbl->distinctval, j, 0);
				char *pos =	(!tbl->colbrk || j + 1 < tbl->colbrk) ? "_EVA_NewColumn" : "_EVA_SameCell";
				DynTableCell *cell = dyntab_cell(&tbl->data, rowstart, 0);
				int b_colbrk = tbl->colbrk && j + 1 >= tbl->colbrk;
				int b_colbrknoval = b_colbrk && !(cell && cell->col == j);
				int b_sel = tbl->srchcol & (1 << j);

				if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3_INT(cntxt->form->html, "\n<!--- Start TableCell ", i, " -->\n");

				/* Set alignment & value separator */
				if(!*align && !b_colbrk)
				{
					if(!stricmp(fmt, "_EVA_MONEY") || !strncmp(fmt, add_sz_str("_EVA_DATE"))) align = "right";
					else if(!strcmp(fmt, "_EVA_NUMBER")) align = "center";
					else if(!strncmp(fmt, add_sz_str("_EVA_RELATION_BTN"))) align = "space";
				}
				if(!strcmp(align, "newline")) { align = ""; separ = "<br>"; }
				else if(!strcmp(align, "space")) { align = ""; separ = " "; }
				else if(!strcmp(align, "+")) { align = "center"; separ = " + "; }
				else if(!strcmp(align, "index")) { align = ""; b_index = 1; valign = "top"; }

				/* Handle column break mode */
				if(b_colbrk && j + 1 == tbl->colbrk)
				{
					DYNBUF_ADD_STR(html, "</tr><tr");
					if(b_rowhilite) DYNBUF_ADD3(html, " onMouseOver=RowHL(this) opname='", opname, 0, NO_CONV, "'");
					if(*bg_color && strlen(bg_color) == 6) DYNBUF_ADD3(html, " bgcolor=#", bg_color, 6, NO_CONV,"")
					DYNBUF_ADD3_INT(html, "><td colspan=", tbl->colbrk + 2, ">");
				}
  				
				/* Output cell header */
				if(!b_colbrknoval && put_html_format_pos(cntxt, pos,
					align, valign, NULL, NULL, NULL, NULL, 1, 1, NULL,
					dyntab_sz(&tbl->fontsize, j, 0) ? dyntab_val(&tbl->fontsize, j, 0) : ctrl->FONTSIZE, NULL,
					dyntab_val(&tbl->celstyle, j, 0),
					dyntab_val(&tbl->bold, j, 0)[0] == '1', 0, dyntab_val(&tbl->underline, j, 0)[0] == '1',
					!strip && *s_strip,
					1)) STACK_ERROR;

				/* Handle column break mode */
				if(b_colbrk && !b_colbrknoval)
				{
					if(!b_firstcolbrk) DYNBUF_ADD_STR(html, "<br>");
					b_firstcolbrk = 0;
					if(dyntab_sz(&tbl->label, j, 0))
					{
						DYNBUF_ADD3_CELL(html, "<u>", &tbl->label, j, 0, TO_HTML, "</u> : ");
						if(cell && cell->txt && strchr(cell->txt, '\n')) DYNBUF_ADD_STR(html, "<br>");
					}
				}

				/* Handle buttons */
				if(!strcmp(dyntab_val(&tbl->field, j, 0), "_EVA_BUTTON"))
				{
					if(table_put_action_button(cntxt, i_ctrl, idobj, j)) STACK_ERROR;
				}
				/* Data field : read cell values & output formated */
				else for(k = 0; rowstart < tbl->data.nbrows; k++, rowstart++)
				{
					/* Insert space & break if column changes */
					int b_found = 0;
					cell = dyntab_cell(&tbl->data, rowstart, 0);
					if(cell->col != j)
					{
						if(!k && !b_colbrk) DYNBUF_ADD_STR(html, "&nbsp;");
						break;
					}

					/* Ignore values after stripping */
					if(b_stripped) continue;

					/* If max strip count is reached : strip */
					if(strip && tot_char >= strip)
					{
						DYNBUF_ADD_STR(html, " ...");
						b_stripped = 1;
						continue;
					}

					/* Distinct values required : search for same previous value - ignore if found */
					if(b_distinct) for(i0 = 0; i0 < k && !b_found; i0++)
						if(!dyntab_cmp(&tbl->data, rowstart, 0, &tbl->data, rowstart - i0 - 1, 0))
							b_found = 1;
					if(b_found) continue;

					/* Output value separator if applicable */
					if(b_index || cell->Line && !dyntab_sz(&tbl->align, j, 0))
					{
						if(line >= cell->Line && k) DYNBUF_ADD(html, separ, 0, NO_CONV)
						else while(line < cell->Line) { DYNBUF_ADD_STR(html, "<br>"); line++; }
					}
					else if(k) DYNBUF_ADD(html, separ, 0, NO_CONV);

					/* Memorize first value for line buttons tooltip */
					if(i_label == tbl->data.nbrows) i_label = rowstart;

					/* Put formated value */
					tbl->srchwords.nbrows = b_sel ? nbwords : 0;
					if(html_put_value_fmt(cntxt, ctrl, &tbl->data, rowstart, fmt, strip, &tot_char, &b_stripped))
						CLEAR_ERROR;

					/* Add unit if column break */
					if(b_colbrk)
					{
						DynTableCell *lbl = dyntab_cell(&tbl->lblunit, j, 0);
						if(lbl && lbl->len) DYNBUF_ADD3_CELLP(html, " ", lbl, TO_HTML, "");
					}
				}

				/* Output cell footer */
				if(put_html_format_pos(cntxt, pos,
					align, valign, NULL, NULL, NULL, NULL, 1, 1, NULL,
					dyntab_sz(&tbl->fontsize, j, 0) ? dyntab_val(&tbl->fontsize, j, 0) : ctrl->FONTSIZE, NULL, NULL,
					dyntab_val(&tbl->bold, j, 0)[0] == '1', 0, dyntab_val(&tbl->underline, j, 0)[0] == '1',
					!strip && s_strip,
					0)) STACK_ERROR;

				/* Handle column break mode */
				if(b_colbrk && j + 1 == tbl->field.nbrows)
					DYNBUF_ADD_STR(html, "</td>");

				if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3_INT(cntxt->form->html, "\n<!--- EndOf TableCell ", i, " -->\n");

				/* Output arrows where applicable */
				if(b_arrows && j + 1 == (tbl->colbrk ? tbl->colbrk - 1 : tbl->field.nbrows) &&
					table_add_updown_btns(cntxt, ctrl, i + tbl->line, DYNTAB_VAL_SZ(&tbl->data, i_label, 0), bg_color))
					STACK_ERROR;
			}
		}

		/* Output line end */
		DYNBUF_ADD_STR(html, "</tr>");
	}

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(cntxt->form->html, "\n<!--- EndOf TableRows ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_status
** Description : output table status as hidden input
*********************************************************************/
#define ERR_FUNCTION "table_put_status"
#define ERR_CLEANUP DYNTAB_FREE(cgival)
int table_put_status(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable cgival = { 0 };
	char status[64];

	if(tbl->b_no_status) RETURN_OK;
	CTRL_GET_CGI_SUBFIELD("STATUS");
	if(!tbl->status) RETURN_OK;

	sprintf(status, "%x,%lu,%lu,%lu,%lu,%lu,%lu,%lu", 
		tbl->status, tbl->line, tbl->lines, tbl->sortcol, tbl->b_sortdesc, tbl->srchcol, tbl->ctrlline, tbl->srchmode);
	CTRL_PUT_CGI_SUBFIELD_VAL("STATUS", status, 0);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_init_obj_list
** Description : read unsorted objects list for a table of objects
*********************************************************************/
#define ERR_FUNCTION "table_init_obj_list"
#define ERR_CLEANUP 
int table_init_obj_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int *selobj							/* out : table displays control values if not 0 */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	int b_selobj = 0;

	/* Set default table attributes	*/
	if(tbl->b_ctrl) tbl->attr = &ctrl->attr;
	if(!tbl->input) tbl->input = dyntab_val(&tbl->cgiinput, 0, 0);


	/* Handle unsorted list of objects */
	if(strcmp(ctrl->CONTROL, "_EVA_INPUT") && *CTRL_ATTR_VAL(NOTABLESORT) == '1' && !strcmp(CTRL_ATTR_VAL(FILTER_TYPE), "_EVA_FILTER_LISTOBJ"))
	{
		tbl->status &= ~(TblCtrl_opensearch | TblCtrl_search | TblCtrl_openaddnew | TblCtrl_addnew | TblCtrl_sel | TblCtrl_opensel | TblCtrl_sort);
		CTRL_ATTR(ctrl->val, LISTOBJ);
		b_selobj = 1;
	}
	/* Handle relation control selection status */
	else if(!strcmp(ctrl->CONTROL, "_EVA_INPUT") && !(form->step == HtmlEdit && ctrl->access & AccessEdit))
	{
		tbl->status &= ~(TblCtrl_opensearch | TblCtrl_search | TblCtrl_openaddnew | TblCtrl_addnew | TblCtrl_sel | TblCtrl_opensel);
		b_selobj = 1;
	}
	else if(!strcmp(ctrl->CONTROL, "_EVA_INPUT") && strlen(tbl->input) < tbl->minsearchlen )
	{
		b_selobj = 1;
	}
	else
		b_selobj = tbl->status & TblCtrl_sel && !(tbl->status & TblCtrl_opensel);

	/* Fetch list of objects to display */
	dyntab_free(&tbl->idobj);
	if(b_selobj)
		/* Selection mode : use control values */
		tbl->totlines = ctrl->val.nbrows;
	else
	{
		/* Search mode : execute search */
		if(table_word_search(cntxt, i_ctrl)) CLEAR_ERROR;

		/* Handle auto selection */
		if((tbl->status & TblCtrl_opensel) && tbl->totlines == 1 && tbl->input && *tbl->input)
		{
			/* Check if control allows multiple values */
			int b_1val = !ctrl->MULTIPLE || !*ctrl->MULTIPLE || !strcmp("No", ctrl->MULTIPLE);
			unsigned long i;
			DynTableCell *c;

			/* Read found IdObj */
			if(sql_exec_query(cntxt, "SELECT DISTINCT IdObj FROM IdList") ||
				sql_get_table(cntxt, &tbl->idobj, 2)) STACK_ERROR;
			c = tbl->idobj.cell;
			if(c)
			{
				/* If control does not allows multiple values */
				if(b_1val)
				{
					/* Clear old values & set table status to closed search */
					for(i = 0; i < ctrl->val.nbrows; i++) ctrl_del_value(cntxt, i_ctrl, i);
					tbl->status &= ~(TblCtrl_opensel | TblCtrl_opensearch);
					tbl->ctrlline = 0;
					b_selobj = 1;

					/* Add found object to control values */
					if(ctrl_add_value(cntxt, i_ctrl, c->txt, c->len, 1, tbl->ctrlline, 1)) STACK_ERROR;
				}
				else
				{
					/* Multiple values allowed : Search for found IdObj & max Num for the control line */
					unsigned long num = 0;
					int b_found = 0;
 					for(i = 0; i < ctrl->val.nbrows; i++)
					{
						DynTableCell *c1 = dyntab_cell(&ctrl->val, i, 0);
						b_found |= !strcmp(c->txt, c1->txt);
						if(c->Line == tbl->ctrlline && c->Num > num) num = c->Num;
					}


					/* If duplicates allowed or IdObj not in control values : add IdObj */
					if(strcmp("Unique", ctrl->MULTIPLE) || !b_found)
						if(ctrl_add_value(cntxt, i_ctrl, c->txt, c->len, num, tbl->ctrlline, 1)) STACK_ERROR;
				}
			}

			/* Clear search string */
			tbl->input = "";
			cgi_value_setkeep(cntxt, &tbl->cgiinput, 0, 2);
			DYNTAB_SET(&tbl->cgiinput, 0, 0, "");
		}
	}

	if(selobj) *selobj = b_selobj;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_export_list
** Description : export a table of objects
*********************************************************************/
#define ERR_FUNCTION "table_export_list"
#define ERR_CLEANUP M_FREE(label); \
					form->step = step;
int table_export_list(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *label = NULL;
	int step = form->step;
	int b_selobj = 0;
	unsigned long i;
	char *btn = CGI_CLICK_BTN_SUBFIELD;
	int b_xl = strstr(btn, "EXCEL") ? 1 : 0;
	char *ext = b_xl ? "xls" : "doc";
	char *exe = b_xl ? "EXCEL" : "WINWORD";
	char name[4090] = {0}; size_t sz_name;
	char fname[4096] = {0}; size_t sz_fname;
	char cmd[4096];
	char cwd[4096];
	if(!tbl) RETURN_OK;

	/* Set temp directory for user */
	if(chdir_user_doc(cntxt)) STACK_ERROR;
	getcwd(add_sz_str(cwd));

	/* Prepare text file with table data */
	form->step = form->prevstep;
	if(table_init_obj_list(cntxt, i_ctrl, &b_selobj)) STACK_ERROR;
	tbl->line = 0;
	tbl->lines = b_xl ? 65000 : tbl->totlines;
	if(table_sort_obj_list(cntxt, i_ctrl, b_selobj)) STACK_ERROR;
	if(table_read_obj_list(cntxt, i_ctrl, b_selobj)) STACK_ERROR;
	if(table_prepare_rows(cntxt, i_ctrl, "")) STACK_ERROR;
	if(file_write_tabrc(cntxt, &tbl->cellval, "dump.txt")) STACK_ERROR;

	/* Prepare output format file */
	if(b_xl)
	{
		dyntab_free(&tbl->cellval);
		DYNTAB_SET(&tbl->cellval, 0, 0, "Fields");
		DYNTAB_SET(&tbl->cellval, 1, 0, "Formats");
		DYNTAB_SET(&tbl->cellval, 2, 0, "Fixed");
		for(i = 0; i < tbl->field.nbrows; i++)
		{
			if(!strcmp("_EVA_RELATION_BTN0", dyntab_val(&tbl->format, i, 0))) continue;
			DYNTAB_SET_CELL(&tbl->cellval, 0, i + 1, &tbl->field, i, 0);
			DYNTAB_SET_CELL(&tbl->cellval, 1, i + 1, &tbl->format, i, 0);
			if(!strcmp("0", dyntab_val(&tbl->strip, i, 0))) DYNTAB_SET(&tbl->cellval, 2, i + 1, "X");
		}
		if(file_write_tabrc(cntxt, &tbl->cellval, "dumpfmt.txt")) STACK_ERROR;
	}

	/* Copy & launch template */
	snprintf(add_sz_str(cmd), "copy %stemplates\\TableList.%s %s > msg.txt 2>err.txt", cntxt->rootdir , ext, cwd);
	system(cmd);
	snprintf(add_sz_str(cmd), "call \"%s\\Office\\%s.EXE\" %s\\TableList.%s", dyntab_val(&cntxt->cnf_server, 0, 0), exe, cwd, ext);
	system(cmd);

	/* Clear temp files */
	remove("dump.txt");
	remove("dumpfmt.txt");
	remove("TableList.xls");
	remove("err.txt");
	remove("msg.txt");

	/* Build destination file name */
	qry_obj_label(cntxt, NULL, NULL, NULL, &label, NULL, NULL, NULL, NULL, 0, &form->objdata, 0);
	sz_name = label ?
		snprintf(add_sz_str(name), "%s %s", ctrl->LABEL, label->data) :
		snprintf(add_sz_str(name), "%s", ctrl->LABEL);
	snprintf(add_sz_str(fname), "%s.%s", name, ext);
	sz_fname = file_compatible_name(fname);
	remove(fname);
	rename(b_xl ? "List.xls" : "TableList.doc", fname);

	/* Output download dialog header */
	DYNBUF_ADD3(&cntxt->html,
		"<font face=Arial><center><hr>"
		"<b>Contenu de [", name, sz_name, TO_HTML, "]</b><hr>");

	/* Output link to file */
	if(file_output_link(cntxt, &cntxt->html,
				cmd, snprintf(add_sz_str(cmd), "Fichier %s : ", b_xl ? "Excel" : "Word"),
				name, sz_name, fname, sz_fname, "#user", NULL, 0, NULL, 0,
				7 | (form->b_modified ? 0 : 8))) STACK_ERROR;

	DYNBUF_ADD_STR(&cntxt->html, "</center></font><br>");
	cntxt->b_terminate = 15;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_sort_obj_list
** Description : sort objects list for a table of objects
*********************************************************************/
#define ERR_FUNCTION "table_sort_obj_list"
#define ERR_CLEANUP qry_build_free(&flt); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(sort); \
					M_FREE(sql)
int table_sort_obj_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int b_selobj						/* in : use control values if not 0 */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	QryBuild flt = { 0 };
	DynTable data = {0};
	DynTable sort = {0};
	DynBuffer *sql = NULL;
	DynTableCell *field = dyntab_cell(&tbl->field, tbl->sortcol, 0);
	char printbuf[1024] = {0};
	char *fmt = dyntab_val(&tbl->format, tbl->sortcol, 0);
	int b_num = !strcmp(fmt, "_EVA_NUMBER") ||
				!stricmp(fmt, "_EVA_MONEY")||
				!strcmp(fmt, "_EVA_FILESIZE");
	if(!field || !field->txt) RETURN_OK;

	/* Check sort column type */
	if(strchr(field->txt, '[') || !strncmp(field->txt, add_sz_str("SELECT ")))
	{
		/* Handle SQL expression - check optimization */
		if(*dyntab_val(&tbl->notable, tbl->sortcol, 0) == '1' || b_selobj)
		{
			/* SQL expression without tables - loop on objects */
			unsigned long j;
			if(b_selobj)
			{
				if(dyntab_from_tab(&tbl->idobj, 0, 0, &ctrl->val, 0, 0, ctrl->val.nbrows, 1, 1)) RETURN_ERR_MEMORY;
			}
			else if(sql_exec_query(cntxt, "SELECT IdObj FROM IdList") || sql_get_table(cntxt, &tbl->idobj, 2)) STACK_ERROR
			if(dyntab_resize(&sort, tbl->idobj.nbrows - 1, 0)) RETURN_ERR_MEMORY;
			if(qry_eval_sqlexpr_var(cntxt, &sql, field->txt, tbl->attr, NULL)) STACK_ERROR
			for(j = 0; j < tbl->idobj.nbrows; j++)
			{
				/* Evaluate expression on object & set result in sort table */
				unsigned long idobj = DYNTAB_TOULRC(&tbl->idobj, j, 0);
				DynTableCell *s, *d = dyntab_cell(&sort, j, 0);
				if(form_eval_fieldexpr(cntxt, &data, 0, idobj, sql->data, sql->cnt, NULL, 0)) STACK_ERROR;
				if(data.nbrows > 1)
					dyntab_sort(&data, 	tbl->b_sortdesc ?
						b_num ?	qsort_col0idesc : qsort_col0desc :
						b_num ?	qsort_col0i : qsort_col0);
				s = dyntab_cell(&data, 0, 0);
				if(s)
				{
					if(!b_num) setcase(3, s->txt, s->len);
					memcpy(d, s, sizeof(d[0]));
					memset(s, 0, sizeof(d[0]));
				}
				d->IdObj = idobj;
			}

			/* Sort table & replace tbl->idobj */
			dyntab_sort(&sort, 	tbl->b_sortdesc ?
				b_num ?	qsort_col0idesc : qsort_col0desc :
				b_num ?	qsort_col0i : qsort_col0);
			dyntab_free(&tbl->idobj);
			for(j = 0; j < tbl->lines; j++)
			{
				DynTableCell *c = dyntab_cell(&sort, j + tbl->line, 0);
				if(!c) break;
				DYNTAB_ADD_INT(&tbl->idobj, j, 0, c->IdObj);
			}
			RETURN_OK;
		}
		else
		{
			/* SQL expression with tables - eval expression on IdList to ValList */
			if(qry_eval_sql_fieldexpr(cntxt, "ValList", field->txt, "IdList", tbl->attr)) STACK_ERROR;
		}
	}
	else
	{
		/* Sort on 1st field only if applicable */
		char *fmt = dyntab_val(&tbl->format, tbl->sortcol, 0);
		flt.b_single_last_field = field->txt && !strchr(field->txt, '(') &&
								(!*fmt || 
									!strcmp(fmt, "_EVA_TXTLINK") ||
									!strcmp(fmt, "_EVA_EMAIL"));

		/* Build temporary table with sort values */
		if(qry_parse_fieldexpr(cntxt, field->txt, field->len, &flt) ||
			sql_get_listcol(cntxt, NULL, b_selobj ? &ctrl->val : NULL, &flt, 0)) STACK_ERROR;
	}

	/* Handle numerical sorting */
	if(b_num && sql_exec_query(cntxt, "ALTER TABLE ValList MODIFY Val FLOAT")) STACK_ERROR;

	/* Add indexes */
	if(sql_exec_query(cntxt, "ALTER TABLE ValList MODIFY IdObj INT NOT NULL") ||
		sql_exec_query(cntxt, "ALTER TABLE ValList ADD INDEX (IdObj)")) STACK_ERROR;

	/* Handle numerical / case insensitive sorting */
	if(dynbuf_print(&sql, 
		"SELECT IdList.IdObj,MAX(%s) AS OrdVal FROM ",
		b_num ? "ValList.Val" : "UCASE(ValList.Val)")) RETURN_ERR_MEMORY;
	if(b_selobj) DYNBUF_ADD_STR(&sql, "TLink AS IdList")
	else DYNBUF_ADD_STR(&sql, "IdList");
	DYNBUF_ADD_STR(&sql, 
		"\nLEFT JOIN ValList ON IdList.IdObj=ValList.IdObj \n");
	if(b_selobj)
	{
		DYNBUF_ADD_STR(&sql, "WHERE IdList.IdObj IN(");
		if(qry_values_list(cntxt, &ctrl->val, 0, &sql)) STACK_ERROR;
		DYNBUF_ADD_STR(&sql, ")\n");
	}

	/* Ordered & clipped list of objects */
	if(dynbuf_print3(&sql, 
		"GROUP BY IdList.IdObj ORDER BY OrdVal %s, IdList.IdObj DESC LIMIT %lu,%lu",
		tbl->b_sortdesc ? "DESC" : "",
		tbl->line, tbl->lines)) RETURN_ERR_MEMORY;

	/* Exec query & place it in table list of objects */
	if(sql_exec_query(cntxt, sql->data) ||
		sql_get_table(cntxt, &tbl->idobj, 2)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_prepare_obj_list
** Description : prepare objects list for a table of objects
*********************************************************************/
#define ERR_FUNCTION "table_prepare_obj_list"
#define ERR_CLEANUP 
int table_prepare_obj_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	int b_sort, b_selobj = 0;
	char *btn = form->i_ctrl_clic == i_ctrl ? CGI_CLICK_BTN_SUBFIELD : "";

	/* Initialize table	*/
	if(table_init_obj_list(cntxt, i_ctrl, &b_selobj)) STACK_ERROR;

	/* Handle automatic setting of display parameters */
	if(tbl->lines < 2) tbl->lines = 2;
	if(tbl->totlines <= tbl->lines && !strcmp(btn, "SHRINK"))
		tbl->lines = tbl->totlines > 4 ? tbl->totlines / 2 : 2;
	if(tbl->totlines <= tbl->lines) tbl->line = 0;
	else if(tbl->line == ~0UL) tbl->line = tbl->totlines - tbl->lines;
	else if(tbl->line > tbl->totlines) tbl->line = tbl->totlines - 1;
	b_sort = (tbl->b_sortsel  || !b_selobj) && tbl->status & TblCtrl_sort;
	if(!b_sort)
		tbl->status &= ~TblCtrl_sort;
	else if(tbl->totlines > 1 &&
		table_sort_obj_list(cntxt, i_ctrl, b_selobj)) CLEAR_ERROR;

	/* Read displayed objects data */
	if(table_read_obj_list(cntxt, i_ctrl, b_selobj)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_put_html_obj_list
** Description : output a table of objects
*********************************************************************/
#define ERR_FUNCTION "table_put_html_obj_list"
#define ERR_CLEANUP form->step = step;
int table_put_html_obj_list(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	unsigned long i;
	int b_shrink, titlemode;
	int step = form->step;
	char *tablesearch;
	char *pagectrl = CTRL_ATTR_VAL(PAGE_CTRL);
	CHECK_HTML_STATUS;
	if(!tbl) RETURN_OK;

	/* Handle edit mode if no title */
	if(tbl->b_ctrl) tbl->attr = &ctrl->attr;
	tablesearch = DYNTAB_FIELD_VAL(tbl->attr, TABLESEARCH);
	titlemode = atoi(DYNTAB_FIELD_VAL(tbl->attr, TABLE_NOTITLESHRINK));
	if(titlemode == 2 && !*tablesearch && !strcmp(ctrl->CONTROL, "_EVA_INPUT") && form->step == HtmlEdit && (ctrl->access & AccessEdit))
		tbl->status |= TblCtrl_opensel;

	/* Prepare list of objects	*/
	if(table_prepare_obj_list(cntxt, i_ctrl)) STACK_ERROR;

	/* Set default table attributes	*/
	if(!DYNTAB_FIELD_CELL(tbl->attr, BORDER) && !ctrl->TABLE_STYLE[0])
		ctrl->BORDER = 1;
	if(!ctrl->TABLERULES[0] && !ctrl->TABLE_STYLE[0])
		ctrl->TABLERULES = ctrl->BORDER ? (form->step == HtmlPrint || tbl->status & TblCtrl_printmode) ? "all" : "cols" : "none";

	/* Handle search output in logs */
	if(tbl->input[0] && tbl->status & TblCtrl_opensearch && (!cntxt->log_srchtxt || form->i_ctrl_clic == i_ctrl))
	{
		M_FREE(cntxt->log_srchtxt);
		DYNBUF_ADD(&cntxt->log_srchtxt, tbl->input, 0, NO_CONV);
	}

	/* Handle minimal table display (closed & 2 or less values) */
	b_shrink = (!titlemode || !tbl->totlines) && tbl->totlines < 3 && 
		!(tbl->status & (TblCtrl_opensearch | TblCtrl_opensel)) && !(tbl->status & TblCtrl_openaddnew);

	/* Handle print mode */
	if(tbl->status & TblCtrl_printmode) form->step = HtmlPrint;

	/* Output global table header */
	if(put_html_table_header(cntxt, 
			DYNTAB_FIELD_VAL(tbl->attr, TABLEALIGN),
			ctrl->TABLEWIDTH,
			ctrl->TABLEHEIGHT,
			NULL, NULL,
			0, 0, 0, NULL, NULL, NULL)) STACK_ERROR;

	/* Add input controls for table functions */
	if((!tbl->idobj.nbrows || tbl->lines <= tbl->idobj.nbrows || pagectrl[0] != '0' || tbl->status & TblCtrl_search) && 
		table_put_controls(cntxt, i_ctrl, b_shrink)) STACK_ERROR;

	/* Output status, table header &  rows */
	DYNBUF_ADD_STR(html, "<td>");
	if(table_put_status(cntxt, i_ctrl) ||
		put_html_table_header(cntxt,
			NULL, "100%", NULL, 
			ctrl->TABLEBGCOLOR,
			ctrl->TABLEBACKGROUND,
			atoi(DYNTAB_FIELD_VAL(tbl->attr, CELLSPACING)),
			atoi(DYNTAB_FIELD_VAL(tbl->attr, CELLPADDING)),
			ctrl->BORDER,
			ctrl->TABLERULES, ctrl->TABLE_STYLE, NULL) ||
		!b_shrink && titlemode != 2 && table_put_title_row(cntxt, i_ctrl, titlemode) ||
		table_put_rows(cntxt, i_ctrl)) STACK_ERROR;

	/* Output table footer */
	DYNBUF_ADD_STR(html, "</table>\n</td></tr>");
	if(tbl->totlines > tbl->lines)
	{
		if(table_put_page_navigator(cntxt, i_ctrl, 1)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</tr>");
	}
	DYNBUF_ADD_STR(html, "</table>\n");

	/* Output non visible values as hidden input */
	if(form->step == HtmlEdit && tbl->status & TblCtrl_sel)
		for(i = 0; i < (ctrl->val.nbrows ? ctrl->val.nbrows : 1); i++)
		{
			DynTableCell *val = dyntab_cell(&ctrl->val, i, 0);
			if(!val || !val->col)
				if(ctrl_put_hidden(cntxt, ctrl, i)) STACK_ERROR;
		}

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD3(html, "\n<!--- End   ObjTable ", ctrl->LABEL, 0, NO_CONV, " -->\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
