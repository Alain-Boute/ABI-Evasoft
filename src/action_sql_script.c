/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_sql_script.c
** Description : handling fonctions for sql script buttons
**      Author : Alain BOUTE
**     Created : May 19 2004
*********************************************************************/

#include "eva.h"

char *mark_lab[] = {"", "", "FOR EACH FILE ", "FOR EACH VALUE ", "IF ", "ELSEIF ", "END ", "NEXT "};


#define END_STRUCT(a) (sm.mark[a].type == MrkEnd || sm.mark[a].type == MrkNext || sm.mark[a].type == MrkElse || sm.mark[a].type == MrkElseIf)
#define COM_STRUCT(a) (sm.mark[a].type == MrkUnknown || sm.mark[a].type == MrkComment)
#define COMSTRUCT(a) (sm->mark[a].type == MrkUnknown || sm->mark[a].type == MrkComment)
#define BEG_STRUCT(a) (sm.mark[a].type == MrkForFile || sm.mark[a].type == MrkForVal || sm.mark[a].type == MrkIf || sm.mark[a].type == MrkElse  || sm.mark[a].type == MrkElseIf)
#define FORSTRUCT(a) (sm->mark[a].type == MrkForFile || sm->mark[a].type == MrkForVal)

#define FOR_STRUCT(a) (sm.mark[a].type == MrkForFile || sm.mark[a].type == MrkForVal)
#define NEXT_STRUCT(a) (sm.mark[a].type == MrkNext)
#define IF_STRUCT(a)  (sm.mark[a].type == MrkIf || sm.mark[a].type == MrkElse || sm.mark[a].type == MrkElseIf)
#define ENDIF_STRUCT(a)  (sm.mark[a].type == MrkEnd || sm.mark[a].type == MrkElse|| sm.mark[a].type == MrkElseIf)

#define PARENT_PARSED_SUCCESS (cntstruc == 1 && ((sm.mark[i].type == MrkIf && ENDIF_STRUCT(k)) \
									|| (sm.mark[i].type == MrkElse && sm.mark[k].type == MrkEnd) \
									|| (sm.mark[i].type == MrkElseIf && (sm.mark[k].type == MrkElse || sm.mark[k].type == MrkElseIf))))

#define CHILD_PARSED_SUCCESS (((typstruc[cntstruc] == MrkForFile || typstruc[cntstruc] == MrkForVal) && NEXT_STRUCT(k)) \
							|| ((typstruc[cntstruc] == MrkIf || typstruc[cntstruc] == MrkElse || typstruc[cntstruc] == MrkElseIf) && sm.mark[k].type == MrkEnd))

#define NEXT_IF_CHILD_STEP (((typstruc[cntstruc] == MrkIf || typstruc[cntstruc] == MrkElseIf) && (sm.mark[k].type == MrkElse || sm.mark[k].type == MrkElseIf)) \
						|| (typstruc[cntstruc] == MrkElse && sm.mark[k].type == MrkElseIf))

#define STOP_EXE(txt) {sm->err = txt; RETURN_OK;}
#define BREAK_EXE(txt) {sm.err = txt; break;}

#define COLNAME 0
#define COLVAL 1
#define COLPATH 2


/*********************************************************************
** Typedef : ScriptMark
** Description : markers for a SQL script
*********************************************************************/
typedef struct _ScriptMark {
	unsigned long i_ctrl;			/* in : control index in cntxt->form->ctrl */
	DynBuffer *html;
	size_t outcnt;
	char *err;
	
	/* Markers table */
	struct _Mark {
		unsigned long line;			/* line # in script */
		char *txt;
		size_t len;
		enum {MrkUnknown,MrkComment,MrkForFile,MrkForVal,MrkIf,MrkElse,MrkElseIf,MrkEnd,MrkNext}
			type;					/* structure type */
		unsigned long end;			/* body end for structure (index in mark) */
		unsigned long qry;			/* enclosing query (~0UL for none) */
		unsigned long L;			/* line number  */
		unsigned long next;			/* next ELSE/ELSEIF mark to execute for IF structure */
	}
		*mark;
	unsigned long nbmarks;
	DynTable markvar;				/* var in mark
										col 0 : var name
										col 1 : var value */
	unsigned long nextmark;			/* next mark to execute */



	/* Queries table */
	struct _Qry {
		unsigned long beg;			/* mark preceding query (index in mark) */
		unsigned long end;			/* mark following query (index in mark) */
		char *txt;
		size_t len;
		unsigned long L0;			/* first line number  */
		unsigned long Ln;			/* last line number  */
	}
		*qry;
	unsigned long nbqry;
	unsigned long display;			/* display SQL qry result if #DISPLAY mark precedes query 
										1    : display sql result
										else : do not display result */

	int options;					/* execution options - bit mask
										bit 0 : display all commands if set
										bit 1 : display full SQL
										bit 2 : return error (do not display)
										bit 3 : check structure if set (no output, set sm->mark->end) */
}
	ScriptMark;

/*********************************************************************
** Function : exec_script_struct
** Description : execute a SQL script sctrure (FOR, IF)
*********************************************************************/
int exec_script_struct(			/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	ScriptMark *sm, 			/* in : script markers */
	unsigned long i				/* in : structure to execute (index in sm->mark) */
);

/*********************************************************************
** Function : flush_sm
** Description : flush a SQL script object
*********************************************************************/
void flush_sm(ScriptMark *sm)
{
	if(!sm || !sm->html || !sm->html->data) return;
	printf("%s", sm->html->data + sm->outcnt);
	sm->outcnt = sm->html->cnt;
	fflush(stdout);
}

/*********************************************************************
** Function : mark_eval_sqlexpr_var
** Description : replace variables by their values in sql queries & mark
*********************************************************************/
#define ERR_FUNCTION "mark_eval_sqlexpr_var"
#define ERR_CLEANUP 
int mark_eval_sqlexpr_var(
	EVA_context *cntxt,			/* in/out : execution context data */
	ScriptMark *sm,				/* in : script markers */
	DynBuffer **sqlexpr,		/* out : SQL expression with variables evaluated */
	char *expr					/* in : SQL expression to process */
){
	unsigned long i;
	char *beg, *next, *var, *val;
	unsigned long lenvar, lenexpr;

	if(!expr || !*expr || !sm) STACK_ERROR;

	/* No var to replace, copy expr in sqlexpr & exit */
	if(!sm->markvar.nbrows) 
	{
		DYNBUF_ADD(sqlexpr, expr, strlen(expr), NO_CONV)
		RETURN_OK;
	}

	lenexpr = strlen(expr);

	/* Scan variables in sm->marvar */
	for(i = 0; i < sm->markvar.nbrows; i++)
	{
		var = dyntab_val(&sm->markvar, i, COLNAME);
		lenvar = dyntab_sz(&sm->markvar, i, COLNAME);
		next = expr;
		beg = next;
		while(*next)
		{
			next = strstr(next, var);

			if(!next)
			{
				DYNBUF_ADD(sqlexpr, beg, strlen(beg), NO_CONV)
				break;
			}

			/* Replace only independant words (' xxxx ', '(xxx)', ' xxx.' ...) */
			if(!strchr("\"[('`.,;=<>-+ ", *next - 1) && !strchr("\"])'`.,;=<>-+ ", *next + lenvar))
			{
				val = dyntab_val(&sm->markvar, i, COLVAL);
				DYNBUF_ADD(sqlexpr, beg, next - beg, NO_CONV)

				DYNBUF_ADD_CELL(sqlexpr, &sm->markvar, i, COLVAL, NO_CONV)
				next = next + lenvar;
				beg = next;
			}
		}
	}
	

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : exec_sql_qry
** Description : execute a SQL script
*********************************************************************/
#define ERR_FUNCTION "exec_sql_qry"
#define ERR_CLEANUP	M_FREE(sql); \
					M_FREE(buf); \
					M_FREE(sqlexpr); \
					DYNTAB_FREE(res)
int exec_sql_qry(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	ScriptMark *sm,				/* in : script markers */
	unsigned long i				/* in : query to execute (index in sm->qry) */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + sm->i_ctrl;
	DynBuffer *sql = NULL;
	DynBuffer *buf = NULL;
	DynBuffer *sqlexpr = NULL;
	DynBuffer **html = &sm->html;
	DynTable res = {0};
	struct _Qry *qry = sm->qry + i;
	char printbuf[1024];

	/* Replace parameters in query */
	DYNBUF_ADD(&sql, qry->txt, qry->len, NO_CONV);
	if(qry_eval_sqlexpr_var(cntxt, &buf, sql->data, &ctrl->attr, NULL)) STACK_ERROR;

	/* Replace variables in query */
	if(mark_eval_sqlexpr_var(cntxt, sm, &sqlexpr, buf->data)) STACK_ERROR;


	/* Display & exec query */
	if(sm->options & 2)
	{
		DYNBUF_ADD3_INT(html, "<br><u>+++ Commande n° ", i + 1, "</u>");
		if(sm->options == 3) DYNBUF_ADD3_BUF(html, "<br><font face=Courier size=-1>", sqlexpr, TO_HTML, "</font>");
	}

	flush_sm(sm);
	if(sql_exec_query(cntxt, sqlexpr->data))
	{
		/* SQL error : display & stop processing */
		sm->err = cntxt->err.text;
		DYNBUF_ADD3_BUF(html, "<br><font face=Courier>", cntxt->err.detail, TO_HTML, "</font><br>");
		CLEAR_ERROR_NOWARN;
	}
	else if(cntxt->sql_result && sm->options && (sm->display == 1 || sm->options & 2))
	{
		/* If display allowed, there is a result to display : get result table */
		unsigned long i, j;
		if(sql_get_table(cntxt, &res, 3)) STACK_ERROR;

		/* Display as HTML table */
		M_FREE(buf);
		if(sm->options & 2) dynbuf_print2(html, "<br><u>=== Résultat : %lu lignes - %.2f s</u><br>", res.nbrows, cntxt->sql_restime);
		DYNBUF_ADD_STR(html, "<table border bgcolor=white>");
		for(i = 0; i < res.nbrows; i++)
		{
			DYNBUF_ADD_STR(html, "<tr>");
			for(j = 0; j < res.nbcols; j++)
				DYNBUF_ADD3_CELL(html, "<td>", &res, i, j, TO_HTML, "</td>");
			DYNBUF_ADD_STR(html, "</tr>");
		}
		DYNBUF_ADD_STR(html, "</table>");
	}
	else
	{
		/* No result to display : output affected rows */
		if(sm->options & 2) dynbuf_print2(html,"<br><u>### Résultat : %lu lignes modifiées - %.2f s</u><br>", cntxt->sql_nbrows, cntxt->sql_restime);
	}

	/* Output result */
	flush_sm(sm);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : eval_mark_expr
** Description : scan & execute queries in mark
*********************************************************************/
#define ERR_FUNCTION "eval_mark_expr"
#define ERR_CLEANUP	M_FREE(sql); \
					M_FREE(buf); \
					M_FREE(sqlexpr);
int eval_mark_expr(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	ScriptMark *sm,				/* in : script markers */
	DynTable *attr,
	DynTable *res,				/* out : result */
	unsigned long num,			/* in : structure to execute (index in sm->mark) */
	unsigned long type			/* enum type of mark structure */
){
	DynBuffer *sql = NULL;
	DynBuffer *buf = NULL;
	DynBuffer *bufpath = NULL;
	DynBuffer *sqlexpr = NULL;
	DynBuffer **html = &sm->html;
	DynTable result = {0};
	unsigned long len = 0, i = 0;
	DynTableCell *newvar, *var_i;
	struct _Mark *mark = sm->mark + num;
	char *txt = mark->txt;
	size_t txtlen = mark->len;


	char *beg = "", *end ="";
	len = strlen(mark_lab[type]) + 4;
	beg = txt + len;

	switch(type)
	{
	case MrkForFile:
	case MrkForVal:

		/* Search for the sql variable in mark 'FOR EACH XXX xxx IN xxxxxx' */
		end = strstr(beg, " IN ");
		if(!end) STOP_EXE("FOR EACH without IN")

		while(*beg == ' ') beg++;
		while(*end == ' ') end--;

		if(beg >= end || (end > beg + txtlen - len)) STOP_EXE("FOR EACH without variable")

		/* Save variable name  */
		DYNTAB_ADD(&sm->markvar, sm->markvar.nbrows, COLNAME, beg, end - beg + 1, NO_CONV)

		/* Search if the variable is already definite in the parent's structures */
		newvar = dyntab_cell(&sm->markvar, sm->markvar.nbrows - 1, COLNAME);
		if(sm->markvar.nbrows > 1)
			for(i = 0; i < sm->markvar.nbrows - 1; i++)
			{
				var_i = dyntab_cell(&sm->markvar, i, COLNAME);
				if(!DYNTAB_SAMEVAL(newvar, var_i)) STOP_EXE("Variable already definite")
			}

		/* Search for 'SELECT xxx' SQL expression */
		beg = strstr(end, " IN ") + 4;

	case MrkIf:
	case MrkElseIf:

		while(*beg == ' ') beg++;
		if(beg == txt + txtlen) STOP_EXE("Mark FOR/IF/ELSEIF without sql expression")
		if(strncmp(beg, add_sz_str("SELECT "))) DYNBUF_ADD_STR(&sql, "SELECT ")
		DYNBUF_ADD(&sql, beg, txtlen -(beg - txt), NO_CONV)

		/* Replace parameters in mark query */
		if(qry_eval_sqlexpr_var(cntxt, &sqlexpr, sql->data, attr, NULL)) STACK_ERROR;

		/* Replace variables in mark query */
		if(mark_eval_sqlexpr_var(cntxt, sm, &buf, sqlexpr->data)) STACK_ERROR;

		/* Display mark is applicable */
		if(sm->options & 2)
		{
			DYNBUF_ADD3(html, "<br><b>", mark_lab[type], 0, TO_HTML, "");
			DYNBUF_ADD3(html, " ", buf->data + 7, buf->cnt - 7, TO_HTML, "</b>\n");
		}


		/* Build & save variable value */
		if(type == MrkForFile)
		{
			/* Exec qry & find files */
			if(sql_exec_query(cntxt, buf->data) || sql_get_table(cntxt, &result, 2)) STACK_ERROR;

			/* Replace mysql path (../../) by cgi path (../) */
			beg = dyntab_val(&result, 0, 0);
			end = strstr(beg, "../");
			if(beg != end) DYNBUF_ADD(&bufpath, beg, end - beg, NO_CONV)
			DYNBUF_ADD(&bufpath, end + 3, strlen(end) - 3, NO_CONV)

			if(find_files(cntxt, res, bufpath->data, IncludeInfos)) CLEAR_ERROR;
			if(!res->nbrows) STOP_EXE("FOR EACH FILE : No such files")
			
			/* Save the file path for the later qry */
			beg = dyntab_val(&result, 0, 0);
			end = strrchr(beg,'/');
			DYNTAB_ADD(&sm->markvar, sm->markvar.nbrows - 1, COLPATH, beg, end - beg + 1, NO_CONV)

			DYNTAB_FREE(result);
			M_FREE(bufpath);
		}
		else
			if(sql_exec_query(cntxt, buf->data) || sql_get_table(cntxt, res, 2)) STACK_ERROR;

		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : exec_child_structures
** Description : execute children structures
*********************************************************************/
#define ERR_FUNCTION "exec_child_structures"
#define ERR_CLEANUP	
int exec_child_structures(
	EVA_context *cntxt,			/* in/out : execution context data */
	ScriptMark *sm,				/* in : script markers */
	unsigned long i				/* in : parent structure (index in sm->mark) */
){
	unsigned long j, k, endstruct, prev_child_struc;

	/* Run queries following mark 'i' if exist */
	k = sm->mark[i].qry;
	while(k != ~0UL && k < sm->nbqry && sm->qry[k].L0 > sm->mark[i].L && ((i == sm->nbmarks - 1) ? 1 :  sm->qry[k].L0 < sm->mark[i + 1].L))
	{
		if(exec_sql_qry(cntxt, sm, k)) STACK_ERROR;
		if(sm->err) RETURN_OK;
		k++;
	}

	if(COMSTRUCT(i)) RETURN_OK;

	/* Run children marks following this mark */
	prev_child_struc = i;
	j = i + 1;
	endstruct = sm->mark[i].end;
	while(j < endstruct && j < sm->nbmarks && !sm->err)
	{
		if(!sm->err && sm->mark[j].type != MrkNext && sm->mark[j].type != MrkEnd && j >= prev_child_struc)
		{
			if(exec_script_struct(cntxt, sm, j)) STACK_ERROR;

			if(COMSTRUCT(j)) prev_child_struc = j + 1;
			else prev_child_struc = (sm->mark[i].next == ~0UL) ? sm->mark[j].end : sm->mark[j].next;
		}
		j++;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : exec_script_struct
** Description : execute a SQL script sctrure (FOR, IF)
*********************************************************************/
#define ERR_FUNCTION "exec_script_struct"
#define ERR_CLEANUP	M_FREE(sql); \
					M_FREE(buf); \
					M_FREE(sqlmrk); \
					DYNTAB_FREE(res)
int exec_script_struct(			/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	ScriptMark *sm,				/* in : script markers */
	unsigned long i				/* in : structure to execute (index in sm->mark) */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + sm->i_ctrl;
	DynBuffer *sql = NULL;
	DynBuffer *buf = NULL;
	DynBuffer **html = &sm->html;
	DynTable res = {0};
	struct _Mark *mark = sm->mark + i;
	DynBuffer *sqlmrk = NULL;
	DynBuffer *file = NULL;
	unsigned long k, row;

	/* Switch on structure type */
	switch(sm->mark[i].type)
	{
	case MrkComment:
	case MrkUnknown:

		/* Comment : skip if applicable */
		sm->display = !strncmp(mark->txt + 3, add_sz_str("#DISPLAY")) || !strnicmp(mark->txt + 3, add_sz_str("display"));
		if(((sm->options & 2) || sm->display) && sm->options)
			DYNBUF_ADD3(html, "<br><b>", mark->txt + (sm->display ? 11 : 3), mark->len - (sm->display ? 11 : 3), TO_HTML, "</b>\n");

		/* Run queries following this mark */
		if(exec_child_structures(cntxt, sm, i)) STACK_ERROR;
		if(sm->err) RETURN_OK;
		sm->display = ~0UL;
		
		/* Next ancester mark to run */
		if(sm->nextmark == i)
			sm->nextmark = i + 1;

		break;

	case MrkForFile:
	case MrkForVal:

		/* Control & save mark variable (name & value) */
		if(eval_mark_expr(cntxt, sm, &ctrl->attr, &res, i, sm->mark[i].type)) STACK_ERROR;
		if(sm->err) RETURN_OK;

		/* Execute qry mark  */
		for(row = 0; row < res.nbrows; row++)
		{
			if(sm->mark[i].type == MrkForFile)
			{
				/* Copy actual file name in sm->markvar to be replaced in child queries */
				DYNBUF_ADD_STR(&file, "'")
				DYNBUF_ADD_CELL(&file, &sm->markvar, sm->markvar.nbrows - 1, COLPATH, NO_CONV)
				DYNBUF_ADD_CELL(&file, &res, row, 0, NO_CONV)
				DYNBUF_ADD_STR(&file, "'")

				DYNTAB_ADD_BUF(&sm->markvar, sm->markvar.nbrows - 1, COLVAL, file)
				M_FREE(file);
			}
			else
			{
				/* Save the current value of the variable in sm->markvar */
				DYNTAB_ADD_CELL(&sm->markvar, sm->markvar.nbrows - 1, COLVAL, &res, row, 0)
			}

			/* Run qry & children marks of the current mark */
			if(exec_child_structures(cntxt, sm, i)) STACK_ERROR;
			if(sm->err) RETURN_OK;

			/* Free variable or file name value */
			dyntab_free_cell(&sm->markvar, ((COLVAL) + ((sm->markvar.nbrows - 1) * sm->markvar.szcols)));
		}

		if(sm->options & 2)
			DYNBUF_ADD_STR(html, "<br><b>END</b>");

		/* Next parent mark to run */
		if(sm->nextmark == i)
			sm->nextmark = sm->mark[i].end + 1;

		/* Free variable */
		dyntab_del_rows(&sm->markvar, sm->markvar.nbrows - 1, 1);

		break;

	case MrkIf:
	case MrkElseIf:

		if(eval_mark_expr(cntxt, sm, &ctrl->attr, &res, i, sm->mark[i].type)) STACK_ERROR;
		if(sm->err) RETURN_OK;

		sm->mark[i].next = ~0UL;

		/* If Mark qry don't success, exec next mark */

		if(sm->options & 2)
			DYNBUF_ADD3_CELL(html, "<br><b>Résultat de la requête IF : ", &res, 0, 0, TO_HTML, "</b>")

		if(dyntab_txt_cmp("1", 1, &res, 0, 0)) 
		{
			if(sm->nextmark == i)
			{
				if(sm->mark[sm->mark[i].end].type != MrkEnd) sm->nextmark = sm->mark[i].end;
				else sm->nextmark = sm->mark[i].end + 1;
			}
			else sm->mark[i].next = (sm->mark[i].end != MrkEnd) ? sm->mark[i].end : sm->mark[i].end + 1;

			break;
		}

	case MrkElse:

		if(sm->options & 2 && sm->mark[i].type == MrkElse)
			DYNBUF_ADD3(html, "<br><b>", mark_lab[sm->mark[i].type], 0, TO_HTML, "</b>");

		/* Run queries & children marks following this mark */
		if(exec_child_structures(cntxt, sm, i)) STACK_ERROR;
		if(sm->err) RETURN_OK;

		if(sm->options & 2)
			DYNBUF_ADD_STR(html, "<br><b>ENDIF</b>");

		/* Next parent mark to run */
		k = sm->mark[i].end;
		while(sm->mark[k].type != MrkEnd) k++;
		if(sm->nextmark == i) sm->nextmark = k + 1;
		else sm->mark[i].next = k + 1;

		break;
	}

		/* Run queries following NEXT mark */
	if(sm->mark[i].type != MrkComment && sm->mark[i].type != MrkUnknown \
		&& (sm->mark[sm->mark[i].end].type == MrkEnd || sm->mark[sm->mark[i].end].type == MrkNext))
	{
		k = sm->mark[sm->mark[i].end].qry;
		while(sm->qry[k].L0 < sm->mark[sm->mark[i].end].L) k++;

		while(!sm->err && k != ~0UL && k < sm->nbqry && sm->qry[k].L0 > sm->mark[sm->mark[i].end].L && ((i == sm->nbmarks - 1) ? 1 :  sm->qry[k].L0 < sm->mark[sm->mark[i].end + 1].L))
		{
			if(exec_sql_qry(cntxt, sm, k)) STACK_ERROR;
			k++;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP


/*********************************************************************
** Function : action_sql_script
** Description : button click handler for SQL_SCRIPT
*********************************************************************/
#define ERR_FUNCTION "action_sql_script"
#define ERR_CLEANUP	M_FREE(sm.mark); \
					M_FREE(sm.qry); \
					M_FREE(sm.html); \
					M_FREE(script); \
					M_FREE(typstruc); \
					cntxt->sql_trace = sql_trace
int action_sql_script(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *script = NULL;
	ScriptMark sm = {0};
	unsigned long i, cnt = 0,j, k, cntstruc, curqry = ~0UL, endstruct;
	unsigned long *typstruc = NULL;
	char *next, *end, *scriptend, *cmp;
	int b_empty;
	int sql_trace = cntxt->sql_trace;
	char *dispmode = CTRL_ATTR_VAL(DISPLAY_MSG);

	/* Handle save before */
	int b_done = 1;
	if((form->b_modified || form->prevstep == HtmlSaveDlg || form->b_newobj) && 
		form_btn_do_save(cntxt, i_ctrl, &b_done)) STACK_ERROR;
	if(!b_done) RETURN_OK;
	form = cntxt->form;
	ctrl = form->ctrl + i_ctrl;

	/* Initialize script structure */

	sm.options |= !strcmp(dispmode, "_EVA_NONE") ? 0 :
				!strcmp(dispmode, "_EVA_MINIMAL") ? 1 :
				!strcmp(dispmode, "_EVA_SIMPLE") ? 2 :
				!strcmp(dispmode, "_EVA_DETAIL") ? 3 : 1;
	sm.display = ~0UL;


	/* Handle variables in script */
	if(qry_eval_sqlexpr_var(cntxt, &script, CTRL_ATTR_VAL(SQL_SCRIPT), &ctrl->attr, NULL)) STACK_ERROR;
	if(!script) RETURN_OK;

	/* Output page header */
	if(sm.options)
	{
		put_html_page_header(cntxt, NULL, "SQL Script", NULL, 3);
		cntxt->b_terminate = 15;
		form->html = &sm.html;
		DYNBUF_ADD3(form->html, "<font face=Arial><center><b>Exécution d'un script de commandes SQL : ", ctrl->LABEL, 0, TO_HTML, "</b></center><hr>");
	}


	/* Locate script markers in SQL comment */
	next = script->data;
	scriptend = script->data + script->cnt;
	while(next && *next && next < scriptend)
	{
		/* Scan lines */
		end = strstr(next, "\n");
		if(!end) end = scriptend;
		cnt++;
		b_empty = 1;
		cmp = next;
		while(b_empty && cmp < end) if(strchr("\t ", *cmp)) cmp++; else b_empty = 0;

		/* If not empty line */
		if(!b_empty) 
		{
			/* Mark comments */
			if(!strncmp(next, add_sz_str("-- ")))
			{
				cmp = next + 4;
				i = sm.nbmarks;
				M_REALLOC(struct _Mark, sm.mark, sm.nbmarks);
				sm.mark[i].txt = next;
				sm.mark[i].len = end - next;
				sm.mark[i].L = cnt;
				sm.mark[i].qry = ~0UL;
				sm.mark[i].end = ~0UL;
				if(next[3] == '#')
				{
					/* Structure keyword : get type */
					sm.mark[i].type = 
						!strncmp(cmp, add_sz_str("FOR EACH VALUE ")) ? MrkForVal :
						!strncmp(cmp, add_sz_str("FOR EACH FILE ")) ? MrkForFile :
						!strncmp(cmp, add_sz_str("NEXT")) ? MrkNext :
						!strncmp(cmp, add_sz_str("IF ")) ? MrkIf :
						!strncmp(cmp, add_sz_str("ELSE")) ? MrkElse :
						!strncmp(cmp, add_sz_str("ELSEIF ")) ? MrkElseIf :
						!strncmp(cmp, add_sz_str("END")) ? MrkEnd :
						MrkUnknown;
				}
				else
					sm.mark[i].type = MrkComment;

				sm.mark[i].qry = curqry;
			}

			/* Mark queries start */
			else if(curqry == ~0UL)
			{
				i = sm.nbqry;
				curqry = i;
				M_REALLOC(struct _Qry, sm.qry, sm.nbqry);
				sm.qry[i].txt = next;
				sm.qry[i].L0 = cnt;
				sm.qry[i].beg = sm.nbmarks;
			}

			/* Mark queries end */
			cmp = end - 1;
			while(cmp > next && strchr(" \t", *cmp)) cmp--;
			if(curqry != ~0UL && (*cmp == ';' || end == scriptend))
			{
				i = sm.nbqry - 1;
				if(end > sm.qry[i].txt) sm.qry[i].len = end - sm.qry[i].txt;
				sm.qry[i].end = sm.nbmarks;
				sm.qry[i].Ln = cnt;
				curqry = ~0UL;
			}
		}

		next = end + 1;
	}

	/* Buffer to scan embeded structures */
	M_ALLOC(typstruc, sm.nbmarks * sizeof(unsigned long))

	for(i = 0; i < sm.nbmarks; i++)
	{
		/* Search for first qry following current mark */
		j = (i == 0 || sm.nbqry == 0) ? 0 : (sm.mark[i - 1].qry != ~0UL) ? sm.mark[i - 1].qry : 0;
		if(sm.nbqry != 0)
		{
			while(j < sm.nbqry && sm.qry[j].L0 < sm.mark[i].L) j++;
			
			if (sm.qry[j].L0 > sm.mark[i].L) sm.mark[i].qry = j;
			else sm.mark[i].qry = ~0UL;
		}

		/* If The qry overlaps the mark */
		if(j != 0 && sm.qry[j - 1].Ln > sm.mark[i].L)
		{
			if(COM_STRUCT(i))
				sm.mark[i].qry = ~0UL;
			else RETURN_ERROR("Query error : query ride a mark structure", NULL)
		}

		/* If beginning of structure, seeks the mark.end */
		else if(BEG_STRUCT(i))
		{
			cntstruc = 1;
			typstruc[cntstruc] = sm.mark[i].type;
			for(k = i + 1; k < sm.nbmarks; k++)
			{
				if (sm.mark[k].type == MrkUnknown || sm.mark[k].type == MrkComment) continue;

				/* End/next match with IF/FOR : erase beginning mark from typstruc */
				if(PARENT_PARSED_SUCCESS || CHILD_PARSED_SUCCESS)
						cntstruc--;
				else if(NEXT_IF_CHILD_STEP)
				{
					/* ELSE/ELSEIF folowwing IF/ELSE/ELSEIF : replace beginning mark in typstruc */
					typstruc[cntstruc] = sm.mark[k].type;
				}
				else
				{
					/* New child structure registered in typstruc */
					cntstruc++;
					typstruc[cntstruc] = sm.mark[k].type;
				}
				
				/* Children structures successfully parsed */
				if(cntstruc == 0)
				{
					sm.mark[i].end = k;
					break;
				}
			}
			if(sm.mark[i].end == ~0UL && IF_STRUCT(i)) BREAK_EXE("IF without END")
			if(sm.mark[i].end == ~0UL && FOR_STRUCT(i)) BREAK_EXE("FOR without NEXT")
		}

		/* END/NEXT mark : Search for beginning mark */
		if (END_STRUCT(i)) 
		{
			k = i;
			while(k > 0 && sm.mark[k - 1].end != i) k--;
			if(sm.mark[k - 1].end != i && NEXT_STRUCT(i)) BREAK_EXE("NEXT without FOR")
			if(sm.mark[k - 1].end != i && ENDIF_STRUCT(i)) BREAK_EXE("ELSE/ELSEIF/END without IF")
		}
	}

	if(sm.err)
	{
		DYNBUF_ADD3(form->html, "<br><font face=Courier>", sm.err, 0, TO_HTML, "</font><br>");
	}

	/* Exec qry preceding first mark */
	cntxt->sql_trace = 0;
	if(sm.nbqry != 0 && !sm.err)
	{
		i = 0;
		endstruct = (sm.nbmarks != 0) ? sm.mark[0].L : sm.qry[sm.nbqry].L0 + 1;
		while(i < sm.nbqry && sm.qry[i].L0 < endstruct && !sm.err)
		{
			if(exec_sql_qry(cntxt, &sm, i)) STACK_ERROR;
			i++;
		}

		/* Exec parents marks including MrkUnknown et MrkComment */
		if(sm.nbmarks != 0 && !sm.err)
		{
			sm.nextmark = 0;

			while(sm.nextmark != sm.nbmarks && !sm.err)
				if(exec_script_struct(cntxt, &sm, sm.nextmark)) STACK_ERROR;
		}
	}

	/* Output footer */
	if(sm.options)
	{
		DYNBUF_ADD_STR(form->html, "<hr><center>");
		if(put_html_button(cntxt, "NOP", "Revenir", "_eva_btn_gobackobj_fr.gif", "_eva_btn_gobackobj_fr_s.gif",
								"Retourne à la page précédente", 0, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, "</center><hr>");
		form->html = NULL;
		flush_sm(&sm);
		if(sm.err) DYNBUF_ADD_STR(&form->html_tab, " ");
	}
	form->b_reload = 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP






