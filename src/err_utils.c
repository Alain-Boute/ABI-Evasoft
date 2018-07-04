/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : err_utils.c
** Description : error handling functions
**      Author : Alain BOUTE
**     Created : Feb 15 2002
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : err_add_context
** Description : report error context
*********************************************************************/
void err_add_context(
	EVA_context *cntxt,		/* in/out : execution context data */
	char *function, 
	char *file, 
	int line
){
	char tmp[1024];

	dyntab_add(&cntxt->err.stack, cntxt->err.stack.nbrows, 0,
				tmp,
				snprintf(tmp, sizeof(tmp), "%s - %s:%d", function, basename(file, 0), line),
				NO_CONV);
}

/*********************************************************************
** Function : err_clear
** Description : clear error status
*********************************************************************/
void err_clear(
	EVA_context *cntxt,		/* in/out : execution context data */
	char *function, 
	char *file, 
	int line
){
	/* Save error context */
	if(cntxt->err.text)
	{
		if(function) err_add_context(cntxt, function, file, line);
		dynbuf_add(&cntxt->debug_html, add_sz_str(
			"<table width=100% border=1 rules=none><tr><td bgcolor=#DDDDDD>*** Warning : "), NO_CONV);
		dynbuf_add(&cntxt->debug_html, cntxt->err.text, 0, TO_HTML);
		dynbuf_add(&cntxt->debug_html, add_sz_str("</td></tr><tr><td><font size=-1>"), NO_CONV);
		err_save_context(cntxt);
		dynbuf_add(&cntxt->debug_html, add_sz_str("</font></td></tr></table>"), NO_CONV);
	}

	/* Clear error data */
	cntxt->err.text = NULL;
	M_FREE(cntxt->err.detail);
	DYNTAB_FREE(cntxt->err.stack);
}

/*********************************************************************
** Function : err_save_context
** Description : save error status in cntxt->debug_html
*********************************************************************/
void err_save_context(EVA_context *cntxt)
{
	unsigned long i;
	EVA_form *form = cntxt->form;
	if(!cntxt->err.text) return;
	if(form && form->ctrl)
	{
		dynbuf_add(&cntxt->debug_html, add_sz_str("Form : "), NO_CONV);
		dynbuf_add(&cntxt->debug_html, form->ctrl->LABEL, 0, TO_HTML);
		dynbuf_add(&cntxt->debug_html, add_sz_str("<br>"), NO_CONV);
		if(form->i_ctrl && form->i_ctrl < form->nb_ctrl)
		{
			dynbuf_add(&cntxt->debug_html, add_sz_str("Control : "), NO_CONV);
			dynbuf_add(&cntxt->debug_html, form->ctrl[form->i_ctrl].LABEL, 0, TO_HTML);
			dynbuf_add(&cntxt->debug_html, add_sz_str("<br>"), NO_CONV);
		}
	}
	if(cntxt->err.detail) dynbuf_add(&cntxt->debug_html, DYNBUF_VAL_SZ(cntxt->err.detail), TO_HTML);
	for(i = 0; i < cntxt->err.stack.nbrows; i++)
	{
		dynbuf_add(&cntxt->debug_html, add_sz_str(
			"<table cellspacing=0 cellpadding=0 bgcolor=#DDDDDD width=100% border=0><tr>\n<td>"), NO_CONV);
		dynbuf_add(&cntxt->debug_html, DYNTAB_VAL_SZ(&cntxt->err.stack, i, 0), TO_HTML);
		dynbuf_add(&cntxt->debug_html, add_sz_str(
			"</td>\n</tr></table>"), NO_CONV);
	}
	if(cntxt->debug_msg) 
	{
		dynbuf_add(&cntxt->debug_html, add_sz_str(
			"<br><b><u>Debug info</u></b><br><pre>"), NO_CONV);
		dynbuf_add(&cntxt->debug_html, DYNBUF_VAL_SZ(cntxt->debug_msg), NO_CONV);
		dynbuf_add(&cntxt->debug_html, add_sz_str(
			"</pre></td>\n</tr></table>"), NO_CONV);
	}
	M_FREE(cntxt->debug_msg);
}

/*********************************************************************
** Function : put_html_error
** Description : output HTML error text to cntxt->html
*********************************************************************/
void put_html_error(EVA_context *cntxt)
{
	FILE *f;
	char *wd = getcwd(NULL, 0);

	/* Build errlog file name */
	snprintf(cntxt->err.file, sizeof(cntxt->err.file), "%s-%lu-%lu.htm", cntxt->err.text, time(NULL), getpid());
	file_compatible_name(cntxt->err.file);

	/* Output error message */
	cntxt->txsize += printf("<font face=Arial><hr><b><font size=+3>Erreur du serveur</font> : %s</b><hr>\n", cntxt->err.text);
	cntxt->txsize += printf("<br><p align=center>Le programme n'a pas pu exécuter votre commande.<br>\n"
		"<br>Retournez à la page précédente avec le bouton ou le menu de votre navigateur et essayez une autre commande.<br>\n"
		"<br>Contactez le <a href='mailto:alain.boute@abing.fr?subject=%s'>responsable du site</a> "
		"si cette erreur est bloquante (conservez le titre du message).<br>\n"
		"<br>Vous pouvez consulter des <a href='../errlog/%s'>informations détaillées sur l'erreur</a> (pour les développeurs)<br>\n"
		"</p>\n", cntxt->err.file, cntxt->err.file);

	/* Open file in errlog directory */
	chdir(cntxt->rootdir);
	mkdir("errlog");
	chdir("errlog");
	f = fopen(cntxt->err.file, "w");
	if(!f)
	{
		f = stdout;
		*cntxt->err.file = 0;
	}
	if(wd)
	{
		chdir(wd);
		free(wd);
	}

	/* Clear focus settings */
	M_FREE(cntxt->focus1);
	M_FREE(cntxt->focus2);
	M_FREE(cntxt->focus3);

	fprintf(f, "<font face=Arial><hr><b><font size=+3>Erreur du serveur</font> : %s</b><hr>\n", cntxt->err.text);
	err_save_context(cntxt);
	cntxt->err.text = NULL;
	err_clear(cntxt, NULL, NULL, 0);
	put_debug_msg(cntxt, f);
	M_FREE(cntxt->debug_html);
	cntxt->debug = DEBUG_CGI|DEBUG_ARGS|DEBUG_ENV;
	debug_put_info(cntxt);
	if(cntxt->err.file[0]) fclose(f);
}

/*********************************************************************
** Function : put_debug_msg
** Description : output debug message
*********************************************************************/
void put_debug_msg(EVA_context *cntxt, FILE *f)
{
	if(cntxt->debug_msg)
	{
		ReplaceTable lf_to_cr[] = { { add_sz_str("\n"), add_sz_str("\r") },
									{ NULL } };
		DynBuffer *tmp = NULL;
		DynBuffer *tmp1 = NULL;
		dynbuf_add(&tmp1, DYNBUF_VAL_SZ(cntxt->debug_msg), lf_to_cr, 1);
		dynbuf_add(&tmp, DYNBUF_VAL_SZ(tmp1), TO_HTML);
		if(tmp && tmp->data && tmp->cnt) fprintf(f ? f : stdout, "<pre>%s</pre><hr>", tmp->data);
		M_FREE(tmp);
		M_FREE(cntxt->debug_msg);
	}
	if(cntxt->debug_html && cntxt->debug_html->data && cntxt->debug_html->cnt)
	{
		fprintf(f ? f : stdout, "<font face=Courier size=-1>%s</font><hr>", cntxt->debug_html->data);
		M_FREE(cntxt->debug_html);
	}
}

/*********************************************************************
** Function : put_debug_sqltable
** Description : output debug message
*********************************************************************/
void put_debug_sqltable(EVA_context *cntxt, char *table)
{
	char printbuf[1024];
	unsigned long sql_trace = cntxt->sql_trace;
	DynTable id_obj = {0};
	snprintf(add_sz_str(printbuf), "SELECT * FROM %s ORDER BY IdObj", table);
	sql_exec_query(cntxt, printbuf);
	cntxt->sql_trace = 0;
	sql_get_table(cntxt, &id_obj, 3);
	cntxt->sql_trace = sql_trace;
	err_print_dyntab(&cntxt->debug_msg, &id_obj, 10, NO_CONV);
	dyntab_free(&id_obj);
}

/*********************************************************************
** Function : err_print_filter
** Description : output a filter to a buffer for debug purposes
*********************************************************************/
void err_print_filter(
	DynBuffer **buf,		/* in/out : destination buffer */
	QryBuild *flt			/* in : filter to output */ 
){
	unsigned long i, j;
	char printbuf[1024];

	if(!buf || !flt) return;

	/* Print filter names */
	dynbuf_print2(buf, "\n*** Filter : %lu nodes\nsrctable=%s\n", flt->nbnode, flt->srctable ? flt->srctable : "*");
	for(i = 0; i < flt->name.nbrows; i++) 
		dynbuf_print(buf, "[%s]\n", dyntab_val(&flt->name, i, 0));

	/* Print filter nodes */
	for(i = 0; i < flt->nbnode + 1; i++)
	{
		/* Print index, source node & join index*/
		QryBuilNode *node = flt->node + i;
		dynbuf_print(buf, "%2lu: ", i);
		if(i) dynbuf_print2(buf, "%2lu#%lu ", node->srcjoin, node->curjoin);

		/* Print fields */
		if(node->field.nbrows)
		{
			dynbuf_add(buf, add_sz_str("["), NO_CONV); 
			for(j = 0; j < node->field.nbrows; j++)
			{
				if(j) dynbuf_add(buf, add_sz_str(","), NO_CONV);
				dynbuf_add(buf, DYNTAB_VAL_SZ(&node->field, j, 0), NO_CONV); 
			}
			dynbuf_add(buf, add_sz_str("]"), NO_CONV); 
		}

		/* Print field modifier */
#define CP(n) node->modif == n ? #n :
		if(node->modif != F_None) dynbuf_print(buf, ".%s ", 
					CP(F_Pkey) CP(F_Obj) CP(F_IdField) CP(F_IdValue) CP(F_Field) CP(F_Value) CP(F_RelObj) CP(F_Num) CP(F_Line)
					CP(F_DateCr) CP(F_WhoCr) CP(F_DateDel) CP(F_WhoDel) "F_???");
#undef CP

		/* Print match operator */
#define CP(n) node->match == n ? #n :
		if(node->match != NoMatch) dynbuf_print(buf, " %s", 
					CP(ObjList) CP(NotObjList) CP(RelList) CP(NotRelList) CP(Like) CP(NotLike) CP(Contain) CP(NotContain) CP(IdValList)
					CP(Smaller) CP(SmallerEqual) CP(Larger) CP(LargerEqual)
					CP(Begin) CP(NotBegin) CP(Different) CP(Exact) CP(InBound) CP(NotInBound) "Match???");
#undef CP

		/* Print match values */
		if(node->value.nbrows)
		{
			dynbuf_add(buf, add_sz_str(" ("), NO_CONV); 
			for(j = 0; j < node->value.nbrows; j++)
			{
				if(j) dynbuf_add(buf, add_sz_str(","), NO_CONV);
				dynbuf_add(buf, DYNTAB_VAL_SZ(&node->value, j, 0), NO_CONV); 
			}
			dynbuf_add(buf, add_sz_str(")"), NO_CONV); 
		}

		/* Print node condition */
		if(node->cond && node->cond->cnt)
			dynbuf_add(buf, node->cond->data, node->cond->cnt, NO_CONV);

		/* Print relation to next node */
#define CP(x) node->rel == x ? #x :
		if(node->rel != RelNone) dynbuf_print(buf, " <%s>", CP(RelDirect) CP(RelReverse) CP(RelAny) CP(RelBoth) "Rel???");
#undef CP
		dynbuf_add(buf, add_sz_str("\n"), NO_CONV); 
	}
}

/*********************************************************************
** Function : err_print_dyntab_line
** Description : output a DynTable line to a buffer for debug purposes
*********************************************************************/
void err_print_dyntab_line(
	DynBuffer **buf,		/* in/out : destination buffer */
	DynTable *tab,			/* in : table to output */ 
	unsigned long i,		/* in : row to print */
	ReplaceTable *sr,		/* in : search & replace table */
	int sz_sr				/* in : # of elements in sr */
){
	unsigned long j;
	char col[25] = {0};

	dynbuf_add(buf, col, sprintf(col,"%lu:\t", i), sr, sz_sr); 
	for(j = 0; j < 20 && j < tab->nbcols; j++)
	{
		char *txt = dyntab_val(tab, i, j);
		size_t len = dyntab_sz(tab, i, j);
		memset(col, ' ', sizeof(col) - 1);
		memcpy(col, txt, len < sizeof(col) - 1 ? len : sizeof(col) - 2);
		dynbuf_add(buf, col, sizeof(col) - 1, sr, sz_sr); 
	}
	dynbuf_add(buf, add_sz_str("\n"), sr, sz_sr); 
}

/*********************************************************************
** Function : err_print_dyntab
** Description : output a DynTable to a buffer for debug purposes
*********************************************************************/
void err_print_dyntab(
	DynBuffer **buf,		/* in/out : destination buffer */
	DynTable *tab,			/* in : table to output */ 
	unsigned long rows,		/* in : max # of rows to print */
	ReplaceTable *sr,		/* in : search & replace table */
	int sz_sr				/* in : # of elements in sr */
){
	unsigned long i, j;

	if(!buf) return;

	if(!tab || !tab->nbrows) 
	{
		dynbuf_add(buf, add_sz_str("(null)"), sr, sz_sr); 
		return;
	}

	/* Output rows first lines */
	j = (!rows || rows > tab->nbrows) ? tab->nbrows : rows;
	for(i = 0; i < j; i++)
		err_print_dyntab_line(buf, tab, i, sr, sz_sr);

	/* Output rows last lines */
	j = (2 * rows > tab->nbrows) ? rows : tab->nbrows - rows;
	if(rows && rows < tab->nbrows)
	{
		if(j > rows) dynbuf_add(buf, add_sz_str("...\n"), sr, sz_sr); 
		for(i = j ;  i < tab->nbrows; i++)
			err_print_dyntab_line(buf, tab, i, sr, sz_sr);
	}
}

/*********************************************************************
** Function : err_print_data
** Description : output DynTable data for col 0 to a buffer for debug purposes
*********************************************************************/
void err_print_data(
	DynBuffer **buf,		/* in/out : destination buffer */
	DynTable *tab,			/* in : table to output */ 
	unsigned long beg,		/* in : 1st row to print */
	unsigned long end,		/* in : last row to print */
	ReplaceTable *sr,		/* in : search & replace table */
	int sz_sr				/* in : # of elements in sr */
){
	unsigned long i;
	char printbuf[1024];

	if(!buf) return;

	if(!tab || !tab->nbrows) 
	{
		dynbuf_add(buf, add_sz_str("(null)\r"), sr, sz_sr); 
		return;
	}
	dynbuf_add(buf, add_sz_str("      IdObj ValObj   N   L  C  R Field                  Value\r"), sr, sz_sr); 
	if(!end || end > tab->nbrows) end = tab->nbrows;
	for(i = beg; i < end; i++)
	{
		DynTableCell *c = dyntab_cell(tab, i, 0);
		dynbuf_print5(buf, "%3lu: %6lu %6lu %3lu %3lu", i, c->IdObj, c->IdValObj, c->Num, c->Line); 
		dynbuf_print4(buf, " %2lu %2lu %-20.20s %-50.50s\r", c->col, c->row, c->field ? c->field : "", c->txt); 
	}
}

/*********************************************************************
** Function : debug_put_cgi
** Description : output CGI inputs
*********************************************************************/
void debug_put_cgi(EVA_context *cntxt)
{
	unsigned long i;
	char buf[1024];

	/* Output CGI data if applicable */
	if(cntxt->debug & DEBUG_CGI)
	{
		dynbuf_add(&cntxt->debug_msg, add_sz_str("\n*** CGI input :"), NO_CONV);
		if(!cntxt->cgi) dynbuf_add(&cntxt->debug_msg, add_sz_str("(null)"), NO_CONV);
		dynbuf_add(&cntxt->debug_msg, add_sz_str("\n"), NO_CONV);
		for(i = 0; i < cntxt->nb_cgi; i++)
		{
			dynbuf_add(&cntxt->debug_msg, buf, snprintf(buf, sizeof(buf),"%lu:\t",i), NO_CONV);
			if(cntxt->cgi[i].name[0])
				dynbuf_add(&cntxt->debug_msg, cntxt->cgi[i].name, cntxt->cgi[i].name_sz, NO_CONV);
			dynbuf_add(&cntxt->debug_msg, add_sz_str("="), NO_CONV);
			dynbuf_add(&cntxt->debug_msg, cntxt->cgi[i].value, cntxt->cgi[i].value_sz, NO_CONV);
			dynbuf_add(&cntxt->debug_msg, add_sz_str("\n"), NO_CONV);
		}
	}
}

/*********************************************************************
** Function : debug_put_info
** Description : output basic debug info
*********************************************************************/
void debug_put_info(EVA_context *cntxt)
{
	int i;

	/* Output CGI data */
	debug_put_cgi(cntxt);

	if(cntxt->debug & DEBUG_ARGS)
	{
		time_t t;
		struct tm *lt;
		char buf[1024];

		t = time(0);
		lt = localtime(&t);
		dynbuf_add(&cntxt->debug_msg, buf, snprintf(buf, sizeof(buf),
				"*** Time : %s*** Duration : %1.2lf s\n*** CWD : ",
				asctime(lt), (double)clock()/CLOCKS_PER_SEC), NO_CONV);
		dynbuf_add(&cntxt->debug_msg, add_sz_str(""), NO_CONV);
		getcwd(buf, sizeof(buf));
		dynbuf_add(&cntxt->debug_msg, buf, 0, NO_CONV);
		sprintf(buf, "\n*** Args : %d\n", cntxt->argc);
		dynbuf_add(&cntxt->debug_msg, buf, 0, NO_CONV);
		for(i = 0; i < cntxt->argc; i++)
		{
			sprintf(buf, "%d:\t", i);
			dynbuf_add(&cntxt->debug_msg, buf, 0, NO_CONV);
			dynbuf_add(&cntxt->debug_msg, cntxt->argv[i], 0, NO_CONV);
			dynbuf_add(&cntxt->debug_msg, add_sz_str("\n"), NO_CONV);
		}
	}
	if(cntxt->debug & DEBUG_ENV)
	{
		dynbuf_add(&cntxt->debug_msg, add_sz_str("\n*** Env vars :\n"), NO_CONV);
		i = 0;
		while(cntxt->envp[i])
		{
			dynbuf_add(&cntxt->debug_msg, cntxt->envp[i], 0, NO_CONV);
			dynbuf_add(&cntxt->debug_msg, add_sz_str("\n"), NO_CONV);
			i++;
		}
	}
}
