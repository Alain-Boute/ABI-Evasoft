/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_mode_search.c
** Description : handling function for for values mode
**      Author : Alain BOUTE
**     Created : Jan 21 2003
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : form_mode_search
** Description : outputs HTML code for the current form in search mode
*********************************************************************/
#define ERR_FUNCTION "form_mode_search"
#define ERR_CLEANUP DYNTAB_FREE(fields); \
					DYNTAB_FREE(selfields); \
					DYNTAB_FREE(values); \
					M_FREE(buf); \
					M_FREE(name)
int form_mode_search(
	EVA_context *cntxt		/* in/out : execution context data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl;
	DynTable fields = { 0 };
	DynTable values = { 0 };
	DynTable selfields = { 0 };
	DynBuffer *buf = NULL;
	DynBuffer *name = NULL;
	char sql[1024];
	unsigned long i, i_ctrl;

	/* Query fields in objects having this formstamp */
	sprintf(sql, 
		"-- form_mode_search : Query fields \n"
		"SELECT DISTINCT TLink.IdField,TField.TxtValue,COUNT(TLink.Pkey) \n"
		"FROM TLink INNER JOIN TLink AS TLink1 ON TLink1.IdObj = TLink.IdObj \n"
		"INNER JOIN TVal AS TField ON TLink.IdField = TField.IdValue \n"
		"WHERE TLink1.IdField=%lu AND TLink1.IdRelObj=%s AND TLink.DateDel IS NULL AND TLink1.DateDel IS NULL \n" 
		"GROUP BY TLink.IdField,TField.TxtValue \n", 
		cntxt->val_FORMSTAMP, dyntab_val(&form->id_form, 0, 0));
	if(sql_exec_query(cntxt, sql) || sql_get_table(cntxt, &fields, 2)) STACK_ERROR;

	/* Build fields list */
	for(i = 0; i < fields.nbrows; i++)
	{
		/* Search for control with same field */
		for(i_ctrl = 0; i_ctrl < form->nb_ctrl &&
			strcmp(form->ctrl[i_ctrl].FIELD, dyntab_val(&fields, i, 1)); i_ctrl++);
		ctrl = i_ctrl < form->nb_ctrl ? form->ctrl + i_ctrl : NULL;

		/* Set option label with control label, field name & nb of objects */
		if(ctrl) DYNBUF_ADD3(&buf, "", ctrl->LABEL, 0, NO_CONV, " ");
		DYNBUF_ADD_CELL(&buf, &fields, i, 1, NO_CONV);
		DYNBUF_ADD3_CELL(&buf, " (", &fields, i, 2, NO_CONV, ")");
		DYNTAB_ADD_BUF(&fields, i, 1, buf);
		M_FREE(buf);
	}
	
	/* Output fields list */
	CGI_VALUES_DONTKEEP(&selfields)
	form->html = &cntxt->html;
	DYNBUF_ADD_STR(form->html,
		"<td><font face=Arial>"
		"<table cellspacing=0 cellpadding=0 border=0 rules=none><tr>\n"
		"<td>Rechercher dans<br>\n");
	DYNBUF_ADD_BUF(&name, form->ctrl->cginame, NO_CONV);
	if(put_html_list(cntxt, name, &fields, &selfields, 30, 0, 1, 1, 0, NULL)) STACK_ERROR;
	DYNBUF_ADD_STR(form->html, "</td>");

	/* Query values for selected fields in objects having this formstamp */
	if(selfields.nbrows)
	{
		if(qry_values_list(cntxt, &selfields, 0, &buf)) STACK_ERROR;
		sprintf(sql, 
			"-- form_mode_search : Query values for selected fields \n"
			"SELECT DISTINCT TLink.IdValue,TVal.TxtValue,TLink.IdRelObj,COUNT(TLink.IdObj) \n"
			"FROM TLink INNER JOIN TLink AS TLink1 ON TLink1.IdObj = TLink.IdObj \n"
			"LEFT JOIN TVal ON TLink.IdValue = TVal.IdValue \n"
			"WHERE TLink1.IdField=%lu AND TLink1.IdRelObj=%s AND TLink1.DateDel IS NULL \n" 
				"AND TLink.IdField IN(%s) \n"
			"GROUP BY TLink.IdValue,TVal.TxtValue,TLink.IdRelObj \n", 
			cntxt->val_FORMSTAMP, dyntab_val(&form->id_form, 0, 0), buf->data);
		if(sql_exec_query(cntxt, sql) || sql_get_table(cntxt, &values, 2)) STACK_ERROR;
		M_FREE(buf);

		/* Output values list */
		dyntab_free(&selfields);
		DYNBUF_ADD_STR(form->html, "<td>Valeurs<br>\n");
		for(i = 0; i < values.nbrows; i++)
		{
			/* If value is a relation */
			if(dyntab_sz(&values, i, 2)) 
			{
				/* Add open button & object label */
				DYNTAB_SET_CELL(&selfields, 0, 0, &values, i, 2);
				if(obj_put_html_btn_label(cntxt, &selfields)) STACK_ERROR;
			}
			else
			{
				/* Display raw value */
				DYNBUF_ADD_CELL(form->html, &values, i, 1, TO_HTML);
			}
			DYNBUF_ADD3_CELL(form->html, " (", &values, i, 3, TO_HTML, ")<br>");
		}
		DYNBUF_ADD_STR(form->html, "</td>");
	}
	DYNBUF_ADD_STR(form->html, "</tr></table></font></td>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

