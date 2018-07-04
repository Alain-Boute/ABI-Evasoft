/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_utils.c
** Description : utility fonctions for buttons & action handlers
**      Author : Alain BOUTE
**     Created : July 1 2003
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : form_get_field_values
** Description : read field values for an object (search in CGI & form data)
*********************************************************************/
#define ERR_FUNCTION "form_get_field_values"
#define ERR_CLEANUP DYNTAB_FREE(fields)
int form_get_field_values(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : copied values */
	char *fieldlist,							/* in : fields to search for (comma separated) */
	unsigned long idform,						/* in : source form id (use current form if 0 and idobj = 0) */
	unsigned long idobj							/* in : source object id (use current obj if 0 and idform = 0) */
){
	DynTable fields = {0};

	/* Check params & prepare fields list */
	if(!fieldlist || !*fieldlist) return 0;
	if(dyntab_from_list(&fields, fieldlist, strlen(fieldlist), ",", 0, 0)) RETURN_ERR_MEMORY;

	/* Call root function */
	if(form_get_fields_values(cntxt, res, &fields, idform, idobj)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_get_fields_values
** Description : read field values for an object (search in CGI & form data)
*********************************************************************/
#define ERR_FUNCTION "form_get_fields_values"
#define ERR_CLEANUP DYNTAB_FREE(values)
int form_get_fields_values(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : copied values */
	DynTable *fields,							/* in : fields to search for */
	unsigned long idform,						/* in : source form id (use current form if 0 and idobj = 0) */
	unsigned long idobj							/* in : source object id (use current obj if 0 and idform = 0) */
){
	EVA_form *form = cntxt->form;
	DynTable values = {0};
	unsigned long i = form ? form->nb_ctrl : 0, j, k;
	unsigned long curform = form ? DYNTAB_TOUL(&form->id_form) : 0;
	unsigned long curobj = form ? DYNTAB_TOUL(&form->id_obj) : 0;

	/* Check params & prepare fields list */
	if(res) dyntab_free(res); else return 0;
	if(!idobj && !idform)
	{
		idobj = curobj;
		idform = curform;
	}
	
	/* For each field */
	for(k = 0; k < fields->nbrows; k++)
	{
		/* If working on current form : use control values first */
		char *field = dyntab_val(fields, k, 0);
		if(form && idform == curform && idobj == curobj)
		{
			/* Search for first control with same field that has values */
			DynTable *val = NULL;
			for(i = 0; i < form->nb_ctrl; i++)
			{
				val = form->ctrl[i].allval.nbrows ? &form->ctrl[i].allval : &form->ctrl[i].val;
				if(!strcmp(form->ctrl[i].FIELD, field) && (val->nbrows || form->ctrl[i].b_modified)) break;
			}
			if(i < form->nb_ctrl) 
			{
				/* Control found : transfer values to result & return */
				if(form->Num || form->Line) val = &form->ctrl[i].val;
				for(j = 0; j < val->nbrows; j++) if(dyntab_sz(val, j, 0))
					DYNTAB_SET_CELL(res, res->nbrows, 0, val, j, 0);
				continue;
			}
		}

		/* Read from CGI data or DB if no cgi value */
		if(cgi_filter_values(cntxt, &values, 'D', ~0UL, idobj ? ~0UL : idform, idobj, field, "", 0, 0) ||
			!values.nbrows && idobj && qry_obj_field(cntxt, &values, idobj, field))
			STACK_ERROR;
		for(j = 0; j < values.nbrows; j++) 
			if(dyntab_sz(&values, j, 0)) DYNTAB_ADD_CELL(res, res->nbrows, 0, &values, j, 0);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_set_field_values
** Description : define field values in CGI data for given form & obj
*********************************************************************/
#define ERR_FUNCTION "cgi_set_field_values"
#define ERR_CLEANUP DYNTAB_FREE(cgival); \
					M_FREE(name); \
					M_FREE(dstfld)
int cgi_set_field_values(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	unsigned long idform,						/* in : destination form id */
	unsigned long idobj,						/* in : destination object id */
	char *dstfield, size_t dstfield_sz,			/* in : destination field */
	DynTable *values,							/* in : source values */
	char *setmode,								/* in : set mode (replace, add, ...) */
	int b_old									/* in : also set corresponding old CGI input value if non zero */
){
	DynTable cgival = { 0 };
	DynBuffer *name = NULL;
	DynBuffer *dstfld = NULL;
	unsigned long i, j;
	EVA_form *form = cntxt->form;
	unsigned long num = 0, line = 0;
	unsigned long curform = form ? DYNTAB_TOUL(&form->id_form) : 0;
	unsigned long curobj = form ? DYNTAB_TOUL(&form->id_obj) : 0;

	/* Read current CGI or DB values of destination field */
	DYNBUF_ADD(&dstfld, dstfield, dstfield_sz, NO_CONV);
	if(!setmode) setmode = "";
	if(cgi_filter_values(cntxt, &cgival, 'D', ~0UL, idform, idobj, dstfld->data, "", 0, 0) ||
		!cgival.nbrows && qry_obj_field(cntxt, &cgival, idobj, dstfld->data)) STACK_ERROR;

	/* Return if nothing to do */
	if(cgival.nbrows ? !strcmp("_EVA_ADD_IFEMPTY", setmode) : !strcmp("_EVA_REMOVE", setmode)) RETURN_OK;

	/* Delete applicable values & check indexes */
	for(i = 0; i < cgival.nbrows; i++)
	{
		DynTableCell *val = dyntab_cell(&cgival, i, 0);
		int del = !strcmp("_EVA_REPLACE", setmode);
		if(!del && !strcmp("_EVA_REMOVE", setmode))
			for(j = 0; j < values->nbrows && !del;
				del = !dyntab_cmp(values, j, 0, &cgival, i, 0), j++);
		if(del)
		{
			if(val->i_cgi && cntxt->cgi[val->i_cgi].value)
			{
				cntxt->cgi[val->i_cgi].name[0] = 0;
				cntxt->cgi[val->i_cgi].name_sz = 0;
			}
		}
		else
		{
			if(!val->i_cgi && (
				cgi_build_name(cntxt, &name, 'D', 0, idform, idobj, dstfield, dstfield_sz, NULL, 0, val->Num, val->Line, 0) ||
				(b_old ? cgi_add_inputold : cgi_add_input)(cntxt, NULL, DYNBUF_VAL_SZ(name), val->txt, val->len)))
				STACK_ERROR;
			if(num < val->Num) num = val->Num;
			if(line < val->Line) line = val->Line;
		}
	}

	/* Add apropriate values */
	if(strcmp("_EVA_REMOVE", setmode)) 
		for(i = 0; !i || i < values->nbrows; i++)
		{
			/* Fake CGI input with proper name & indexes */
			DynTableCell *c = dyntab_cell(values, i, 0);
			if(c && c->Num <= 1 && !c->Line)
			{
				if(line) line++; else num++;
				c->Num = num;
				c->Line = line;
			}
			if(cgi_build_name(cntxt, &name, 'D', 0, idform, idobj, dstfield, dstfield_sz, NULL, 0, c ? c->Num : 1, c ? c->Line : 0, 0) ||
				(b_old ? cgi_add_inputold : cgi_add_input)(cntxt, NULL, DYNBUF_VAL_SZ(name), DYNTAB_VAL_SZ(values, i, 0)))
				STACK_ERROR;
		}

	/* Handle control values if current form & object */
	if(form && idform == curform && idobj == curobj)
	{
		/* Search for first control with same field that has values */
		DynTable *val = NULL;
		for(i = 0; i < form->nb_ctrl; i++)
		{
			val = form->ctrl[i].allval.nbrows ? &form->ctrl[i].allval : &form->ctrl[i].val;
			if(!strcmp(form->ctrl[i].FIELD, dstfld->data) && (val->nbrows || form->ctrl[i].b_modified)) break;
		}
		if(i < form->nb_ctrl) 
		{
			/* Control found : transfer values to control */
			dyntab_free(&form->ctrl[i].allval);
			if(cgi_filter_values(cntxt, &form->ctrl[i].val, 'D', ~0UL, idform, idobj, dstfld->data, "", 0, 0)) STACK_ERROR;
			form->ctrl[i].b_modified = 1;
		}
	}

	/* Set form reload if current form */
	if(form && idform == DYNTAB_TOUL(&form->id_form) && idobj == DYNTAB_TOUL(&form->id_obj))
		form->b_reload = 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_set_values
** Description : define values in current form
*********************************************************************/
#define ERR_FUNCTION "form_set_values"
#define ERR_CLEANUP DYNTAB_FREE(values)
int form_set_values(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	unsigned long idform,						/* in : destination form id */
	unsigned long idobj,						/* in : destination object id */
	char *destfield, size_t destfield_sz,		/* in : destination field */
	char *srctype,								/* in : source type */
	DynTable *srcobj,							/* in : source objects */
	DynTable *srcval,							/* in : source values / field expression */
	char *setmode								/* in : set mode (replace, add, ...) */
){
	DynTable values = {0};
	int b_old = !idobj;
	
	/* Handle expression on destination object */
	if(!strcmp(srctype, "_EVA_DESTOBJ"))
	{
		if(form_eval_fieldexpr(cntxt, &values, idform, idobj, DYNTAB_VAL_SZ(srcval, 0, 0), NULL, 0)) STACK_ERROR;
	}
	else
	{
		if(form_eval_valtype(cntxt, &values, srctype, srcobj, srcval)) STACK_ERROR;
	}

	/* Set values */
	if(cgi_set_field_values(cntxt, idform, idobj, destfield, destfield_sz, &values, setmode, b_old)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_eval_fieldexpr
** Description : evaluate a field expression in a control
*********************************************************************/
#define ERR_FUNCTION "ctrl_eval_fieldexpr"
#define ERR_CLEANUP M_FREE(buf)
int ctrl_eval_fieldexpr(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	EVA_ctrl *ctrl,								/* in : control to process */
	DynTable *res,								/* out : resulting values - always freed before processing */
	char *expr									/* in : field expression to evaluate in control */
){
	EVA_form *form = cntxt->form;
	DynBuffer *buf = NULL;
	if(qry_eval_sqlexpr_var(cntxt, &buf, expr, &ctrl->attr, NULL)) STACK_ERROR;
	if(form_eval_fieldexpr(cntxt, res, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj),
										DYNBUF_VAL_SZ(buf), NULL, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

int TarifFAP(EVA_context *cntxt, DynTable *res, unsigned long idobj, char *func);

/*********************************************************************
** Function : form_eval_fieldexpr
** Description : evaluate a field expression in current form context
*********************************************************************/
#define ERR_FUNCTION "form_eval_fieldexpr"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(restbl); \
					DYNTAB_FREE(resgroup); \
					qry_build_free(&flt); \
					M_FREE(pfield); \
					M_FREE(expr); \
					M_FREE(srchfield); \
					M_FREE(fieldexpr); \
					M_FREE(_groupfn)
int form_eval_fieldexpr(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : resulting values - always freed before processing */
	unsigned long idform,						/* in : source form id (read from CGI if not 0 & available, else from DB) */
	unsigned long idobj,						/* in : source object id */
	char *_field, size_t field_sz,				/* in : field expression to evaluate in current form context */
	DynTable *data,								/* in : object data - for optimisation purposes */
	unsigned long beg							/* in : first line to process in data */
){
	char *pfield = mystrdup(_field, field_sz);
	char *field = pfield;
	char *bracket = !field ? NULL : strchr(field, '[');
	char *dotsep = !field ? NULL : strchr(field, '.');
	char *reldir = !field ? NULL : strstr(field, "->");
	char *relrev = !field ? NULL : strstr(field, "<-");
	char *relop = (reldir && (!relrev || reldir < relrev)) ? reldir : relrev;
	char *paropn = !field ? NULL : strchr(field, '(');
	DynTable id_obj = { 0 };
	DynTable restbl = { 0 };
	DynTable resgroup = { 0 };
	QryBuild flt = { 0 };
	DynBuffer *expr = NULL;
	char *srchfield = NULL;
	char *fieldexpr = NULL;
	char *fieldgrp;
	size_t fieldgrp_sz;
	struct _Brackets 
	{
		size_t beg;
		size_t end;
	} p[50] = {0};
	int b_expr;
	char *_groupfn = NULL;
	char *groupfn = NULL;
	unsigned long i, j, nbp = 0;

	/* Check params - return if applicable */
	if(res) dyntab_free(res); else RETURN_OK;
	if(!idform && !idobj)
	{
		idform = DYNTAB_TOUL(&cntxt->form->id_form);
		idobj = DYNTAB_TOUL(&cntxt->form->id_obj);
	}
	if(!field || !*field || !field_sz || (!idform && !idobj)) RETURN_OK;

	/* Handle raw SELECT statement */
	if(!strncmp(field, add_sz_str("SELECT ")))
	{
		if(qry_eval_sqlexpr_var(cntxt, &expr, field, NULL, NULL) ||
			sql_exec_query(cntxt, expr->data) ||
			sql_get_table(cntxt, res, 4)) CLEAR_ERROR;
		RETURN_OK;
	}

	/* Handle forced SQL, string litterals & numbers */
	if(*field == '=') { field++; field_sz--; b_expr = 1; }
	else b_expr = *field == '\'' || isdigit(*field) || bracket;
	if(!field || !*field || !field_sz) RETURN_OK;

	/* Handle grouping */
	fieldgrp = field;
	fieldgrp_sz = field_sz;
	if(!b_expr && paropn)
	{
		size_t sz = paropn - field;
		M_STRDUP(_groupfn, field, sz);
		groupfn = _groupfn;
		fieldgrp += sz + 1;
		fieldgrp_sz -= sz + 2;
	}

	/* Handle brackets or expression : mixed SQL / fields expression */
	if(b_expr)
	{
		/* Process field expressions & collect results in restbl */
		unsigned long line = 0, num = 0;
		while(bracket)
		{
			DynTableCell *c;
			unsigned long line0 = 0, num0 = 0;
			char *end = strchr(bracket + 1, ']');
			size_t sz = end - bracket - 1;
			p[nbp].beg = bracket - fieldgrp;
			p[nbp].end = end - fieldgrp;
			if(!end) RETURN_OK;
			M_STRDUP(fieldexpr, bracket + 1, sz);

			/* Handle functions reference */
#ifndef _EVA_DLL
			if(!strncmp(fieldexpr, add_sz_str("$TarifFap")))
			{
				if(TarifFAP(cntxt, res, idobj, fieldexpr + 9)) CLEAR_ERROR;
			}
			else
#endif
				/* Normal case : evaluate bracket contents */
				if(form_eval_fieldexpr(cntxt, res, idform, idobj, fieldexpr, sz, data, beg)) STACK_ERROR;
			M_FREE(fieldexpr);

			/* Copy values to restbl */
			for(i = 0; !i || i < res->nbrows; i++) 
			{
				c = dyntab_cell(res, i, 0);
				DYNTAB_ADD_CELLP(&restbl, i, nbp, c);
				if(!c) continue;
				if(line0 < c->Line) line0 = c->Line;
				if(num0 < c->Num) num0 = c->Num;
			}
			c = dyntab_cell(&restbl, 0, nbp);
			c->i_cgi = res->nbrows;

			/* Handle result mode : do not use indexes if IN () operator*/
			if(bracket < field + 5 || strncmp(bracket - 5, " IN (", 5))
			{
				c->row = num0;
				c->col = line0;
			}
			else
			{
				c->row = 0;
				c->col = 0;
			}

			if(line < c->col) line = c->col;
			if(num < c->row) num = c->row;
			nbp++;
			bracket = strchr(end + 1, '[');
		}
		dyntab_free(res);

		/* If a set of values was evaluated */
		if(restbl.nbrows)
		{
			/* Exec SQL query for each line / num set of values */
			unsigned long L, N;
			for(L = line ? (restbl.nbcols == 1 && restbl.nbrows == 1) ? dyntab_cell(&restbl,0,0)->Line : 1 : 0;
					!L || line && L <= line; L++) 
				for(N = 1; (line || !num) ? N < 2 : N <= num; N++)
			{
				/* Prepare query build */
				char *txt = fieldgrp;
				unsigned long valcnt = 0;
				size_t len = !p[0].end ? fieldgrp_sz : p[0].beg;
				M_FREE(expr);
				DYNBUF_ADD_STR(&expr, "SELECT ");
				if(len) DYNBUF_ADD(&expr, txt, len, NO_CONV);

				/* Substitue each bracket result in SQL query */
				for(j = 0; j < restbl.nbcols; j++)
				{
					/* Loop on values */
					DynTableCell *c0 = dyntab_cell(&restbl, 0, j);
					int b_empty = 1;
					for(i = 0; i < restbl.nbrows; i++)
					{
						DynTableCell *c = dyntab_cell(&restbl, i, j);
						if(((line ? !c->Line || c->Line == L :
								!num || c->Num == N || c0->i_cgi <= 1 && c->Num <= 1) &&
							c->txt && c->txt[0] && c->len))
						{
							if(!b_empty) DYNBUF_ADD_STR(&expr, ",");
							DYNBUF_ADD3_CELLP(&expr, "'", c, SQL_NO_QUOTE, "'")
							b_empty = 0;
							if(line ? c->Line : num <= 1 || c->Num > 1 || N == 1) valcnt++;
						}
					}

					/* Add empty value if no matching value found */
					if(b_empty) DYNBUF_ADD_STR(&expr, "''");

					/* Add out of brackets SQL code piece */
					txt = fieldgrp + p[j].end + 1;
					len = (j < nbp - 1 ? p[j + 1].beg : fieldgrp_sz) - p[j].end - 1;
					if(len) DYNBUF_ADD(&expr, txt, len, NO_CONV);
				}
				/* Ignore value set if no value found (except when 1 row without line */
				if(!valcnt && (restbl.nbrows > 1 || line)) continue;

				/* Exec Query */
				if(sql_exec_query(cntxt, expr->data) || sql_get_table(cntxt, &resgroup, 2)) STACK_ERROR;

				/* Add results to res */
				M_FREE(expr);
				for(j = 0; j < resgroup.nbrows; j++)
				{
					if(expr && expr->cnt) DYNBUF_ADD_STR(&expr, ",");
					DYNBUF_ADD_CELL(&expr, &resgroup, j, 0, NO_CONV);
				}
				if(expr && expr->cnt)
				{
					unsigned long r = res->nbrows;
					DYNTAB_ADD_BUF(res, r, 0, expr);
					dyntab_cell(res, r, 0)->Line = L;
				}
			}
		}
		else
		{
			DYNBUF_ADD3(&expr, "SELECT ", fieldgrp, fieldgrp_sz, NO_CONV, "");
			if(sql_exec_query(cntxt, expr->data) || sql_get_table(cntxt, res, 6)) STACK_ERROR;
		}
	}
	/* Handle trivial case : simple field */
	else if(!relop && !dotsep && (!paropn || groupfn))
	{
		if(data)
		{
			if(dyntab_from_list(&restbl, fieldgrp, fieldgrp_sz, ",", 0, 0)) RETURN_ERR_MEMORY;
			for(i = 0; i < restbl.nbrows; i++)
			{
				j = beg;
				if(dyntab_filter_field(res, 0, data, dyntab_val(&restbl, i, 0), ~0UL, &j)) RETURN_ERR_MEMORY;
			}
		}
		else
		{
			M_STRDUP(srchfield, fieldgrp, fieldgrp_sz);
			if(form_get_field_values(cntxt, res, srchfield, idform, idobj)) STACK_ERROR;
		}
	}
	/* Handle first field if it may be read from CGI or data */
	else if(cntxt->nb_cgi && relop > fieldgrp && (!dotsep || relop && dotsep > relop) && (!paropn || groupfn))
	{
		/* Read first field values */
		size_t sz = fieldgrp_sz - (relop - fieldgrp) - (relop == reldir ? 2 : 0);
		size_t sz1 = relop - fieldgrp;
		M_STRDUP(srchfield, fieldgrp, sz1);
		M_STRDUP(fieldexpr, relop + (relop == reldir ? 2 : 0), sz);
		if(data)
		{
			if(dyntab_from_list(&restbl, srchfield, sz1, ",", 0, 0)) RETURN_ERR_MEMORY;
			for(i = 0; i < restbl.nbrows; i++)
			{
				j = beg;
				if(dyntab_filter_field(&id_obj, 0, data, dyntab_val(&restbl, i, 0), ~0UL, &j)) RETURN_ERR_MEMORY;
			}
		}
		else
			if(form_get_field_values(cntxt, &id_obj, srchfield, idform, idobj)) STACK_ERROR;
		
		/* Evaluate expression on first field value */
		flt.b_recdata = 1;
		if(dyntab_sz(&id_obj, 0, 0) && (
				qry_parse_fieldexpr(cntxt, fieldexpr, sz, &flt) ||
				sql_get_listcol(cntxt, res, &id_obj, &flt, 0))) STACK_ERROR;

		/* Set indexes for values - TODO : this should be explained better in documentation 
											it allows some interesting table calculations but is some how obscure */
		if(dyntab_sz(&id_obj, 0, 0) && id_obj.nbrows > 1)
			for(i = 0; i < res->nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(res, i, 0);
			for(j = 0; j < id_obj.nbrows; j++)
				if(c->IdObj == DYNTAB_TOULRC(&id_obj, j, 0))
				{
					DynTableCell *c1 = dyntab_cell(&id_obj, j, 0);
					c->Num = c1->Num;
					c->Line = c1->Line;
					break;
				}
		}
	}
	/* Other cases : read from DB if object exists */
	else if(idobj)
	{
		groupfn = NULL;
		DYNTAB_ADD_INT(&id_obj, 0, 0, idobj);
		flt.b_recdata = 1;
		if(qry_parse_fieldexpr(cntxt, field, field_sz, &flt) ||
			sql_get_listcol(cntxt, res, &id_obj, &flt, 0)) STACK_ERROR;
	}
	
	/* Handle grouping */
	if(groupfn)
	{
		/* Handle MAX / MIN auto number mode */
		if(*groupfn == 'M')
		{
			/* Number mode if all values are full numbers and not date */
			int b_num = 1;
			for(i = 0; i < res->nbrows && b_num; i++)
			{
				char *e, *t = dyntab_val(res, i, 0);
				double n = strtod(t, &e);
				b_num = !*e && (n <= 10000 || datetxt_invalid(t));
			}
			if(b_num) groupfn = groupfn[1] == 'I' ? "NMIN" : "NMAX";
		}

		/* Group with selected function */
		if(dyntab_group(res, groupfn)) CLEAR_ERROR;;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_parse_valtype
** Description : evaluate a value type / expression pair
*********************************************************************/
int qry_parse_valtype(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in : execution context data */
	char *srctype,								/* in : value type */
	DynTable *listobj,							/* int : list of objects */
	DynTable **id_form,							/* out : selected form */
	DynTable **id_obj							/* out : selected object */
){
	*id_form = 0;
	*id_obj = 0;

	/* Set base object & form depending on source type */
	if(!strcmp("_EVA_MAIN_FORM", srctype))
	{
		*id_obj = &cntxt->id_form;
	}
	else if(!strcmp("_EVA_MAIN_OBJ", srctype)) 
	{
		*id_form = &cntxt->id_form;
		*id_obj = &cntxt->id_obj;
	}
	else if(!strcmp("_EVA_ALT_FORM", srctype)) 
	{
		*id_obj = &cntxt->alt_form;
	}
	else if(!strcmp("_EVA_ALT_OBJ", srctype)) 
	{
		*id_form = &cntxt->alt_form;
		*id_obj = &cntxt->alt_obj;
	}
	else if(!strcmp("_EVA_SERVERCONFIG", srctype)) 
	{
		*id_obj = &cntxt->id_cnf;
	}
	else if(!strcmp("_EVA_CURRENTSESSION", srctype)) 
	{
		*id_obj = &cntxt->session;
	}
	else if(!strcmp("_EVA_USERID", srctype) ||
			!strcmp("_EVA_CURRENTUSER", srctype) ||
			!strcmp("_EVA_CURRENT_USER", srctype)) 
	{
		*id_obj = &cntxt->id_user;
	}
	else if(!strcmp("_EVA_CURRENTFORM", srctype)) 
	{
		*id_obj = &cntxt->form->id_form;
	}
	else if(!strcmp("_EVA_CURRENT", srctype) ||
			!strcmp("_EVA_CURRENTOBJ", srctype) ||
			!strcmp("_EVA_CURRENTOBJ_FIELD", srctype) ||
			!strcmp("_EVA_FIELD", srctype)) 
	{
		*id_form = &cntxt->form->id_form;
		*id_obj = &cntxt->form->id_obj;
	}
	else if(!strcmp("_EVA_LISTOBJ", srctype) ||
			!strcmp("_EVA_FILTER_LISTOBJ", srctype)) 
	{
		*id_obj = listobj;
	}
	else return 1;
	return 0;
}

/*********************************************************************
** Function : form_eval_valtype
** Description : evaluate a value type / expression pair
*********************************************************************/
#define ERR_FUNCTION "form_eval_valtype"
#define ERR_CLEANUP
int form_eval_valtype(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : resulting values */
	char *srctype,								/* in : value type */
	DynTable *listobj,							/* in : objects id for srctype=LISTOBJ or srctype=FILTER */
	DynTable *srcval							/* in : source values / field expression */
){
	if(ctrl_eval_valtyp(cntxt, NULL, res, srctype, listobj, srcval)) STACK_ERROR;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_eval_valtyp
** Description : evaluate a value type / expression pair
*********************************************************************/
#define ERR_FUNCTION "ctrl_eval_valtyp"
#define ERR_CLEANUP DYNTAB_FREE(selobj); \
					DYNTAB_FREE(fields); \
					DYNTAB_FREE(values); \
					M_FREE(buf); \
					qry_build_free(&flt)
int ctrl_eval_valtyp(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	EVA_ctrl *ctrl,								/* in : control to process */
	DynTable *res,								/* out : resulting values */
	char *srctype,								/* in : value type */
	DynTable *listobj,							/* in : objects id for srctype IN (LISTOBJ,FILTER,CALC) */
	DynTable *srcval							/* in : source values / field expression */
){
	DynTable *id_form = NULL;
	DynTable *id_obj = NULL;
	DynTable selobj = {0};
	DynTable fields = {0};
	DynTable values = {0};
	DynBuffer *buf = NULL;
	QryBuild flt = {0};

	/* Check params */
	dyntab_free(res);
	if(!srctype || !strcmp(srctype, "_EVA_VALUE")) srctype = "";

	/* Set base object & form depending on source type */
	if(!strcmp(srctype, "_EVA_SRC_FILTER") ||
		!strcmp(srctype, "_EVA_FILTER"))
	{
		/* Source = filter : read matching objects - return empty result if none */
		if(qry_add_filter_forms(cntxt, &flt, NULL, listobj) ||
			qry_filter_objects(cntxt, &selobj, &flt)) STACK_ERROR;
		if(!selobj.nbrows) RETURN_OK;
		id_obj = &selobj;

		/* Sort objects if applicable */
		if(ctrl) CTRL_ATTR(fields, SORT_VALUE);
		if(fields.nbrows)
		{
			char *sortmode = CTRL_ATTR_VAL(SORT_MODE);
			unsigned long idsort = DYNTAB_TOUL(&fields);
			if(qry_obj_field(cntxt, &values, idsort, "_EVA_CONTROL") ||
				qry_obj_field(cntxt, &values, idsort, 
					!strcmp(dyntab_val(&values, 0, 0), "_EVA_INPUT") ? "_EVA_FIELD" : "_EVA_SRCFIELD") ||
				qry_add_table_col(cntxt, id_obj, DYNTAB_VAL_SZ(&values, 0, 0))) CLEAR_ERROR; 
			else
				dyntab_sort(id_obj, !strcmp(sortmode, "_EVA_NUMASC") ? qsort_col1f :
									!strcmp(sortmode, "_EVA_NUMDESC") ? qsort_col1fdesc :
									!strcmp(sortmode, "_EVA_TXTDESC") ? qsort_col1desc :qsort_col1);
		}
	}
	else if(!strcmp(srctype, "_EVA_EXPRVAL"))
	{
		unsigned long i;
		EVA_ctrl exprval = {0};
		if(qry_cache_idobj(&exprval.attr, DYNTAB_TOUL(listobj)) ||
			ctrl_read_baseobj(cntxt, &exprval.attr) ||
			ctrl_output_exprval(cntxt, &exprval, 3)) CLEAR_ERROR;
		for(i = 0; i < exprval.val.nbrows; i++) if(dyntab_sz(&exprval.val, i, 0))
			DYNTAB_ADD_CELL(res, res->nbrows, 0, &exprval.val, i, 0);
		RETURN_OK;
	}
	else if(!strcmp(srctype, "_EVA_SRC_CTRL"))
	{
		if(qry_listobj_field(cntxt, &fields, listobj, "_EVA_FIELD") ||
			form_get_fields_values(cntxt, &selobj, &fields, 0, 0)) STACK_ERROR;
		id_obj = &selobj;
	}
	else
		qry_parse_valtype(cntxt, srctype, listobj, &id_form, &id_obj);

	/* Set result depending on base object & values */
	if(!id_obj)
	{
		/* Copy fixed values to result */
		unsigned long i;
		if(srcval) for(i = 0; i < srcval->nbrows; i++)
		{
			DYNTAB_ADD_CELL(res, i, 0, srcval, i, 0);
			dyntab_cell(res, i, 0)->Num = 1;
			dyntab_cell(res, i, 0)->Line = 0;
		}
	}
	else if(!dyntab_sz(srcval, 0, 0))
	{
		/* Copy object id to result */
		unsigned long i;
		for(i = 0; i < id_obj->nbrows; i++)
		{
			DYNTAB_ADD_CELL(res, i, 0, id_obj, i, 0);
			dyntab_cell(res, i, 0)->b_relation = 1;
		}
	}
	else if(srcval && srcval->nbrows)
	{
		/* Evaluate each expression & add to result */
		unsigned long i, j, k, idform = id_form ? DYNTAB_TOUL(id_form) : 0;
		for(k = 0; !k || k < id_obj->nbrows; k++) for(i = 0; i < srcval->nbrows; i++)
		{
			char *expr = dyntab_val(srcval, i, 0);
			size_t sz = dyntab_sz(srcval, i, 0);
			M_FREE(buf);
			DYNBUF_ADD_STR(&buf, "=");
			if(qry_eval_sqlexpr_var(cntxt, &buf, expr, ctrl ? &ctrl->attr : NULL, NULL)) STACK_ERROR;
			if(strcmp(buf->data + 1, expr))
			{
				j = !strchr(buf->data, '[') ? 0 : 1;
				expr = buf->data + j;
				sz = buf->cnt - j;
				if(!sz) expr = "";
			}
			if(form_eval_fieldexpr(cntxt, &values, idform, DYNTAB_TOULRC(id_obj, k, 0), expr, sz, NULL, 0)) STACK_ERROR;
			for(j = 0; j < values.nbrows; j++) if(dyntab_sz(&values, j, 0))
				DYNTAB_ADD_CELL(res, res->nbrows, 0, &values, j, 0);
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
