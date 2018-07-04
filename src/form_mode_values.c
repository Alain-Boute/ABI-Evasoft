/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**form->stepform->stepFile : form_mode_values.c
** Description : handling function for for values mode
**form->step  Author : Alain BOUTE
**form->step Created : Jan 21 2003
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : form_mode_values
** Description : outputs HTML code for the given form
*********************************************************************/
#define ERR_FUNCTION "form_mode_values"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					DYNTAB_FREE(table); \
					DYNTAB_FREE(idobj); \
					DYNTAB_FREE(objdata); \
					M_FREE(buf); \
					table_free(&tbl)
int form_mode_values(
	EVA_context *cntxt		/* in/out : execution context data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl;
	DynTable data = { 0 };
	DynTable table = { 0 };
	DynTable idobj = { 0 };
	DynTable objdata = { 0 };
	DynBuffer *buf = NULL;
	char sql[2048];
	unsigned long i, j, k, l, i_ctrl, idfield;
	DynTableCell *val;
	char *clickbtn = cntxt->cgi ? cntxt->cgi[cntxt->cgibtn].name : NULL;
	char *dispfield = !clickbtn ? NULL : strncmp(clickbtn, add_sz_str("BFIELD=")) ? NULL : clickbtn + 7;
	int rawvalues = 0;
	ObjTableFormat tbl = {0};
	int b_ident_user = DYNTAB_TOUL(&form->id_obj) == DYNTAB_TOUL(&cntxt->id_user);

	/* Handle buttons */
	if(!clickbtn) clickbtn = "";
	if(!strncmp(clickbtn, add_sz_str("BCLR=")))
	{
		/* Delete value */
		snprintf(add_sz_str(sql),
		"-- form_mode_values : Delete selected value\n"
		"UPDATE TLink SET DateDel='%s',IdWhoDel=%s WHERE Pkey=%s",
		cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), clickbtn + 5);
		if(sql_exec_query(cntxt, sql)) STACK_ERROR;
	}

	/* Read all object data */
	snprintf(add_sz_str(sql),
		"-- form_mode_values : Read all object data\n"
		"SELECT \n"
			"TLink.IdField, \n"
			"TField.TxtValue, \n"
			"TLink.IdRelObj, \n"
			"TLink.IdValue, \n"
			"IF(TVal.TxtValue IS NULL,TLink.IdRelObj,TVal.TxtValue), \n"
			"TLink.Num, \n"
			"TLink.Line, \n"
			"TLink.Lang, \n"
#define _DateCr 8
			"TLink.DateCr, \n"
#define _IdWhoCr 9
			"TLink.IdWhoCr, \n"
#define _DateDel 10
			"TLink.DateDel, \n"
#define _IdWhoDel 11
			"TLink.IdWhoDel, \n"
			"TLink.Pkey	\n"
		"FROM TLink LEFT JOIN TVal ON TLink.IdValue = TVal.IdValue \n"
		"INNER JOIN TVal AS TField ON TLink.IdField = TField.IdValue \n"
		"WHERE TLink.IdObj=0%s \n"
		"ORDER BY TLink.IdField,TLink.Line,TLink.Num,TLink.Pkey DESC", 
		dyntab_val(&form->id_obj, 0, 0));
	if(sql_exec_query(cntxt, sql) ||
		sql_get_table(cntxt, &data, 2)) STACK_ERROR;

	/* Set columns titles */
	DYNTAB_SET(&table, 0, table.nbcols, "Libellé");
	DYNTAB_SET(&table, 0, table.nbcols, "Champ");
	DYNTAB_SET(&table, 0, table.nbcols, "Valeur");
	DYNTAB_SET(&table, 0, table.nbcols, "Index"); table.nbcols += 3;
	DYNTAB_SET(&table, 0, table.nbcols, "Date");
	DYNTAB_SET(&table, 0, table.nbcols, "Utilisateur");
	DYNTAB_SET(&table, 0, table.nbcols, "Effacé");
	DYNTAB_SET(&table, 0, table.nbcols, "Par");
	
	/* Prepare table of values */
	if(sql_id_value(cntxt, add_sz_str("_EVA_USERNAME"), &idfield)) STACK_ERROR;
	form->html = &buf;
	form->step = HtmlView;
	form->b_noctrltree = 1;
	for(i = 0; i < data.nbrows; i++)
	{
		char *field = dyntab_val(&data, i, 1);
		unsigned long Num = DYNTAB_TOULRC(&data, i, 5);
		unsigned long Line = DYNTAB_TOULRC(&data, i, 6);
		char *datedel = dyntab_val(&data, i, _DateDel);
		int b_changefield;

		/* Skip if field specified and different */
		if(dispfield && strcmp(dispfield, field)) continue;

		/* Skip if field unspecified and value is not last input */
		k = table.nbrows;
		val = dyntab_cell(&table, k - 1, 0);
		b_changefield = !val->field || strcmp(val->field, field);
		if(!dispfield && *datedel && !b_changefield) continue;

		/* Search for control with same field */
		for(i_ctrl = 0; i_ctrl < form->nb_ctrl && (
			strcmp(form->ctrl[i_ctrl].FIELD, dyntab_val(&data, i, 1)) ||
			strcmp(form->ctrl[i_ctrl].CONTROL, "_EVA_INPUT")); i_ctrl++);
		ctrl = i_ctrl < form->nb_ctrl ? form->ctrl + i_ctrl : NULL;

		/* Set label & field columns */
		if(ctrl) DYNTAB_ADD(&table, k, 0, ctrl->LABEL, 0, NO_CONV);
		DYNTAB_SET_CELL(&table, k, 1, &data, i, 1);

		/* Initialize value attribute for sorting */
		val = dyntab_cell(&table, k, 0);
		val->b_dontfreefield = 1;
		val->col = i_ctrl;
		val->field = field;
		val->Num = Num;
		val->Line = Line;
		val->Pkey = DYNTAB_TOULRC(&data, i, 12);

		/* If user credentials and not identified user or admin */
		if(!cntxt->b_admin && !b_ident_user && field && 
			(!strcmp(field, "_EVA_LOGIN") || !strcmp(field, "_EVA_PASSWORD"))) 
		{
			/* Hide credentials */
			DYNBUF_ADD_STR(form->html, "<font size=-1>******</font>");
		}
		/* If value is a relation */
		else if(dyntab_sz(&data, i, 2) && !rawvalues) 
		{
			/* Add open button & object label */
			DYNTAB_SET_CELL(&idobj, 0, 0, &data, i, 2);
			if(obj_put_html_btn_label(cntxt, &idobj)) STACK_ERROR;
		}
		/* If control is found */
		else if(ctrl && strcmp(ctrl->TYPE, "_EVA_RELATION") && !rawvalues) 
		{
			/* Use control for display in view mode */
			ctrl->LABELPOS = "_EVA_NONE";
			ctrl->POSITION = "_EVA_SameCell";
			ctrl->FONTCOLOR = dyntab_sz(&data, i, _DateDel) ? "888888" : "";
			ctrl->FONTSIZE = "-1";
			ctrl->CELL_STYLE = "";
			form->step = HtmlView;
			dyntab_free(&ctrl->val);
			dyntab_free(&ctrl->allval);
			dyntab_free(&ctrl->alldbval);
			DYNTAB_SET_CELL(&ctrl->val, 0, 0, &data, i, 4);
			CTRL_PRI_HDLR(i_ctrl);
		}
		else
		{
			/* Display raw value */
			DYNBUF_ADD3_CELL(form->html, "<font size=-1>", &data, i, 4, TO_HTML, "</font>");
		}
		DYNTAB_ADD_BUF(&table, k, 2, buf);
		M_FREE(buf);

		/* Set indexes */
		DYNTAB_SET_CELL(&table, k, 3, &data, i, 5);
		DYNTAB_SET_CELL(&table, k, 5, &data, i, 6);

		/* Add Clear value button */
		if(!*datedel && cntxt->user_data.cell && cntxt->b_admin)
		{
			snprintf(add_sz_str(sql), "BCLR=%lu", val->Pkey);
			if(put_html_button(cntxt, sql, NULL, "_eva_cancel.gif", "_eva_cancel_s.gif",
														"Efface cette valeur", 0, 0)) STACK_ERROR;
			DYNTAB_ADD_BUF(&table, k, 6, buf);
			M_FREE(buf);
		}

		/* Set create infos */
		if(datetxt_to_format(cntxt, sql, dyntab_val(&data, i, _DateCr), NULL)) 
			strcpy(sql, dyntab_val(&data, i, _DateCr));
		DYNTAB_ADD(&table, k, 7, sql, 0, NO_CONV);
		sprintf(sql, 
			"-- form_mode_values : Read create username\n"
			"SELECT TVal.TxtValue \n"
			"FROM TLink INNER JOIN TVal ON TLink.IdValue = TVal.IdValue \n"
			"WHERE TLink.DateDel is NULL AND TLink.IdField=%lu \n"
				"AND TLink.IdObj=0%s", idfield, dyntab_val(&data, i, _IdWhoCr));
		if(sql_exec_query(cntxt, sql) || sql_get_table(cntxt, &idobj, 2)) STACK_ERROR;
		DYNTAB_ADD_CELL(&table, k, 8, &idobj, 0, 0);

		/* Set delete infos if field history is displayed */
		if(dyntab_sz(&data, i, _DateDel))
		{
			if(datetxt_to_format(cntxt, sql, datedel, NULL)) 
				strcpy(sql, datedel);
			DYNTAB_ADD(&table, k, 9, sql, 0, NO_CONV);
			sprintf(sql, 
				"-- form_mode_values : Read delete username\n"
				"SELECT TVal.TxtValue \n"
				"FROM TLink INNER JOIN TVal ON TLink.IdValue = TVal.IdValue \n"
				"WHERE TLink.DateDel is NULL AND TLink.IdField=%lu \n"
					"AND TLink.IdObj=0%s", idfield, dyntab_val(&data, i, _IdWhoDel));
			if(sql_exec_query(cntxt, sql) || sql_get_table(cntxt, &idobj, 2)) STACK_ERROR;
			DYNTAB_ADD_CELL(&table, k, 10, &idobj, 0, 0);
		}

		/* Add history button if applicable */
		if(!dispfield && b_changefield)
		{
			for(b_changefield = 0, j = i + 1; j < data.nbrows; j++)
					if(strcmp(dyntab_val(&data, j, 1), field)) break;
					else if(dyntab_sz(&data, j, _DateDel)) { b_changefield = 1; break; }
			if(b_changefield)
			{
				snprintf(add_sz_str(sql), "BFIELD=%s", field);
				if(put_html_button(cntxt, sql, NULL, "_eva_history.gif", "_eva_history_s.gif",
															"Affiche l'historique de ce champ", 0, 0)) STACK_ERROR;
				DYNTAB_ADD_BUF(&table, k, 11, buf);
				M_FREE(buf);
			}
		}
	}

	/* Sort table without first row */
	qsort(table.cell + table.szcols, table.nbrows - 1, sizeof(table.cell[0])*table.szcols, 
		qsort_mode_values);
	
	/* Output table header */
	form->html = &cntxt->html;
	if(dispfield)
	{
		DYNBUF_ADD_STR(form->html, "<td align=center bgcolor=#FFFFFF>Historique de ");
		if(dyntab_sz(&table, 2, 0)) DYNBUF_ADD3_CELL(form->html, "<b>", &table, 1, 0, TO_XML, "</b> - ");
		DYNBUF_ADD3_CELL(form->html, "[<font face=Courier size=-1>", &table, 1, 1, TO_XML, "</font>]</td></tr><tr>");
	}
	DYNBUF_ADD_STR(form->html,
		"<td><table width=100% cellspacing=0 cellpadding=1 border=1 rules=rows><tr>\n");
	for(i = dispfield ? 2 : 0; i < table.nbcols; i++)
		DYNBUF_ADD3_CELL(form->html, 
			"<td bgcolor=#DDDDDD><font face=Arial size=-2>",
			&table, 0, i, NO_CONV, "</font></td>\n");
	DYNBUF_ADD_STR(form->html, "</tr>\n");

	/* Output table */
	for(i = 1, k = 0, l = 0; i < table.nbrows; i++)
	{
		int b_head = 0;
		char *bgcolor = table_row_bgcolor(cntxt, &tbl, k, NULL);
		if(!bgcolor || !*bgcolor) bgcolor = "FFFFFF";

		/* Handle fields cells grouping */
		if(l) l--;
		if(dyntab_cmp(&table, i, 1, &table, i + 1, 1)) k++;
		else if(!l)
		{
			do l++; while(!dyntab_cmp(&table, i, 1, &table, i + l, 1) && i + l < table.nbrows);
			b_head = 1;
		}
		DYNBUF_ADD_STR(form->html, "<tr>");
		for(j = dispfield ? 2 : 0; j < table.nbcols; j++)
		{
			if(!b_head && l && j < 2) continue;
			DYNBUF_ADD_STR(form->html, "<td bgcolor=#");
			DYNBUF_ADD(form->html, bgcolor, 6, NO_CONV);
			if(b_head && j < 2) DYNBUF_ADD3_INT(form->html, " rowspan=", l, "");
			if(dyntab_sz(&table, i, _DateDel) && j >= 2 && j <= 6)
				DYNBUF_ADD_STR(form->html, " background='../img/bg_rayures_grises.gif'");
			DYNBUF_ADD_STR(form->html, ">");
			if(dyntab_sz(&table, i, j)) DYNBUF_ADD3_CELL(form->html,
				"<font face=Arial size=-2>", &table, i, j, NO_CONV, "</font></td>\n")
			else DYNBUF_ADD_STR(form->html,
				"<img src='../img/_eva_spacer.gif' border=0></td>\n");
		}
		DYNBUF_ADD_STR(form->html, "</tr>\n");
	}
	DYNBUF_ADD_STR(form->html, "</table>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

