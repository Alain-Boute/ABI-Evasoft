/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_access.c
** Description : access rights handling utilities
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : ctrl_parse_access
** Description : parse access check keywords
*********************************************************************/
void ctrl_parse_access(
	int *access,					/* in/out : access bits are or-ed to the given value */
	char *exec,						/* in : control execution mode */
	char *objcreat,					/* in : create object access */
	char *objdel					/* in : delete object access */
){
	/* Control view / edit access */
	*access = AccessNone;
	if(!strcmp("_EVA_ACCES_NONE", exec)) return;
	if(!strcmp("_EVA_ACCES_VIEW", exec)) *access |= AccessView;
	else *access |= AccessView | AccessEdit;

	/* Form create / delete access */
	if(objcreat && !*objcreat) *access |= AccessCreate;
	if(objdel && !*objdel) *access |= AccessDelete;
 }

/*********************************************************************
** Function : obj_check_user_access
** Description : get access status of an object for the identified user
*********************************************************************/
#define ERR_FUNCTION "obj_check_user_access"
#define ERR_CLEANUP DYNTAB_FREE(usergroup)
int obj_check_user_access(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *data,					/* in : object data */
	unsigned long beg,				/* in : 1st row to use in data */
	int *acc						/* out : access rights */
){
	DynTable usergroup = { 0 };
	unsigned long i, j;

	/* Read access grants - return full access if no restrictions */
	i = beg;
	if(dyntab_filter_field(&usergroup, 0, data, "_EVA_GRANT_EDIT", ~0UL, &i)) RETURN_ERR_MEMORY;
	i = beg;
	if(dyntab_filter_field(&usergroup, 0, data, "_EVA_GRANT_VIEW", ~0UL, &i)) RETURN_ERR_MEMORY;
	if(!usergroup.nbrows)
	{
		*acc = AccessView | AccessEdit;
		RETURN_OK;
	}

	/*  Select access on user */
	for(i = 0; i < usergroup.nbrows; i++)
		if(!dyntab_cmp(&usergroup, i, 0, &cntxt->id_user, 0, 0))
		{
			DynTableCell *c = dyntab_cell(&usergroup, i, 0);
			*acc |= (c->field && strcmp(c->field, "_EVA_GRANT_EDIT")) ? AccessView : AccessEdit;
		}

	/*  Select access on user groups */
	for(i = 0; i < usergroup.nbrows; i++)
	{
		DynTableCell *c = dyntab_cell(&usergroup, i, 0);
		char *txt = c->txt;
		size_t sz = c->len;
		for(j = 0; j < cntxt->user_groups.nbrows; j++)
			if(!dyntab_txt_cmp(txt, sz, &cntxt->user_groups, j, 0))
			{
				*acc |= (c->field && strcmp(c->field, "_EVA_GRANT_EDIT")) ? AccessView : AccessEdit;
			}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_user_access
** Description : get access status of a control for the identified user
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_user_access"
#define ERR_CLEANUP DYNTAB_FREE(access); \
					DYNTAB_FREE(ctrlgroup); \
					DYNTAB_FREE(objcreat); \
					DYNTAB_FREE(objdel)
int ctrl_check_user_access(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *attr,					/* in : control attributes */
	DynTable *obj_data,				/* in : object data */
	unsigned long beg,				/* in : 1st row to use in obj_data */
	int *acc						/* out : access rights */
){
	DynTable access = { 0 };
	DynTable ctrlgroup = { 0 };
	DynTable objcreat = { 0 };
	DynTable objdel = { 0 };
	unsigned long line, lines, matchline;
	unsigned long i, j;
	DynTableCell *objbeg = dyntab_cell(obj_data, beg, 0);
	int b_form = !strcmp(DYNTAB_FIELD_VAL(attr, CONTROL), "_EVA_FORM");
	if(!acc) return 0;

	/* Check object access if control is a form */
	if(b_form)
	{
		if(obj_check_user_access(cntxt, obj_data, beg, acc)) STACK_ERROR;
		if(!(*acc & AccessEdit)) RETURN_OK;
	}

	DYNTAB_FIELD_TAB(&ctrlgroup, attr, USERGROUP);
	if(!cntxt->b_identified)
	{
		/* Select access for non identified users */
		DYNTAB_FIELD(&access, attr, ACCES_NOT_IDENT);
		DYNTAB_FIELD(&objcreat, attr, CREATE_NOT_IDENT);
		DYNTAB_FIELD(&objdel, attr, DELETE_NOT_IDENT);
		if(!access.nbrows && ctrlgroup.nbrows)
			/* No access if unspecified & control has usergroups */
			*acc = AccessNone;
		else
			ctrl_parse_access(acc, dyntab_val(&access, 0, 0),
											dyntab_val(&objcreat, 0, 0),
											dyntab_val(&objdel, 0, 0));
	}
	else
	{
		/* Always allow acces to identified user account form */
		if(objbeg && objbeg->IdObj == DYNTAB_TOUL(&cntxt->id_user) && b_form)
		{
			*acc = AccessView | AccessEdit;
			RETURN_OK;
		}

		/*  Select acces on user groups */
		DYNTAB_FIELD_TAB(&access, attr, ACCES_IDENT);
		DYNTAB_FIELD_TAB(&objcreat, attr, CREATE_IDENT);
		DYNTAB_FIELD_TAB(&objdel, attr, DELETE_IDENT);

		/* For each line of control acces / users groups table */
		lines = (access.nbrows > ctrlgroup.nbrows) ? access.nbrows : ctrlgroup.nbrows;
		matchline = lines;
		for(line = 0; line < lines && matchline == lines; line++)
		{
			/* Match line if no user group selected */
			if(!dyntab_sz(&ctrlgroup, line, 0))
			{
				matchline = line;
				break; 
			}

			/* Search for user groups in control */
			for(i = 0; i < ctrlgroup.nbcols && matchline == lines; i++)
				for(j = 0; j < cntxt->user_groups.nbrows && matchline == lines; j++)
					if(!dyntab_cmp(&ctrlgroup, line, i, &cntxt->user_groups, j, 0))
						matchline = line;
		}
			
		/* Select access of 1st line matching user group */
		if(!lines || matchline < lines)
		{
			ctrl_parse_access(acc, dyntab_val(&access, matchline, 0),
									dyntab_val(&objcreat, matchline, 0),
									dyntab_val(&objdel, matchline, 0));
		}
		else
			*acc = AccessNone;
	}

	/* Select access on new object */
	if(*acc != AccessNone && !dyntab_sz(obj_data, beg, 0))
	{
		int acc1;
		ctrl_parse_access(&acc1, DYNTAB_FIELD_VAL(attr, ACCES_NEWOBJ), "", "");
		*acc &= acc1;
	}

	/* Select access on existing object */
	else if(acc != AccessNone)
	{
		int acc1;
		ctrl_parse_access(&acc1, DYNTAB_FIELD_VAL(attr, ACCES_EXISTOBJ), "", "");
		*acc &= acc1;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_access_v0
** Description : get access status of a control for the identified user - old method
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_access_v0"
#define ERR_CLEANUP DYNTAB_FREE(access); \
					DYNTAB_FREE(fltfield); \
					DYNTAB_FREE(fltval); \
					DYNTAB_FREE(fltvaltyp); \
					DYNTAB_FREE(fltop); \
					DYNTAB_FREE(values); \
					DYNTAB_FREE(values1); \
					DYNTAB_FREE(debug); \
					cntxt->sql_trace = sql_trace
int ctrl_check_access_v0(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl					/* in : control to check for access status
									  out : access status is set in ctrl->access */
){
	EVA_form *form = cntxt->form;
	DynTable access = { 0 };
	DynTable fltfield = { 0 };
	DynTable fltval = { 0 };
	DynTable fltvaltyp = { 0 };
	DynTable fltop = { 0 };
	DynTable values = { 0 };
	DynTable values1 = { 0 };
	DynTable debug = { 0 };
	unsigned long line, lines, matchline;
	unsigned long i, j;
	int acc, sql_trace = cntxt->sql_trace;

	/* Check access once during read for user rights */
	if(form->step == CtrlRead)
	{
		ctrl_check_user_access(cntxt, &ctrl->attr, &form->objdata, 0, &ctrl->access);
		ctrl->access0 = ctrl->access;

		/* No other checking during read process */
		RETURN_OK;
	}

	/* Return if no access */
	ctrl->access = ctrl->access0;
	if(ctrl->access == AccessNone) RETURN_OK;

	/* Check access for edit mode */
	CTRL_ATTR(access, ACCES_EDIT);
	if(access.nbrows)
	{
		char *acc = dyntab_val(&access, 0, 0);
		int b_edit = (form->prevstep == HtmlEdit || form->prevstep == HtmlSaveDlg || form->nextstep == HtmlEdit)
					&& form->step != HtmlView;
		if(b_edit ? !strcmp(acc, "_EVA_VIEWONLY") : !strcmp(acc, "_EVA_EDITONLY"))
		{
			/* No access if edit mode does not match requested mode */
			ctrl->access = AccessNone;
			RETURN_OK;
		}
		if(!strcmp(acc, "_EVA_ALWAYS"))
		{
			/* Edit access if forced */
			ctrl->access = AccessEdit;
			RETURN_OK;
		}
	}

	/* Select access on current object data */
	CTRL_ATTR_TAB(fltfield, ACCESS_FILTER_SIMPLE_FIELD);
	CTRL_ATTR_TAB(fltop, ACCESS_FILTER_SIMPLE_OP);
	CTRL_ATTR_TAB(fltvaltyp, ACCESS_FILTER_SIMPLE_VALTYPE);
	CTRL_ATTR_TAB(fltval, ACCESS_FILTER_SIMPLE_VALUE);
	CTRL_ATTR_TAB(access, ACCESS_FILTER_SIMPLE);
	CTRL_ATTR_TAB(debug, DEBUG_SQL_EXPR);

	/* For each line of control acces / field condition table */
	lines = access.nbrows > fltfield.nbrows ?	access.nbrows : fltfield.nbrows;
	matchline = lines;
	for(line = 0; line < lines && matchline == lines; line++)
	{
		/* Check for field presence - match line if no field */
		char *op = dyntab_val(&fltop, line, 0);
		int b_equal = !*op || !strcmp(op, "_EVA_EQUAL");
		char *field = dyntab_val(&fltfield, line, 0);
		if(!*field)
		{
			matchline = line;
			break;
		}

		/* Evaluate value expression */
		dyntab_free(&values);
		DYNTAB_SET_CELL(&values, 0, 0, &fltval, line, 0);
		if(dyntab_sz(&debug, line, 0))
			cntxt->sql_trace = DEBUG_SQL_RES;
		if(ctrl_eval_valtyp(cntxt, ctrl, &values1, dyntab_val(&fltvaltyp, line, 0), NULL, &values)) CLEAR_ERROR;

		/* Evaluate field expression */
		dyntab_free(&values);
		if(ctrl_eval_fieldexpr(cntxt, ctrl, &values, field)) CLEAR_ERROR;
		cntxt->sql_trace = sql_trace;

		/* Set empty values if applicable & check each value */
		if(!values.nbrows) DYNTAB_ADD_CELLP(&values, 0, 0, NULL);
		if(!values1.nbrows) DYNTAB_ADD_CELLP(&values1, 0, 0, NULL);
		for(i = 0; i < values.nbrows && matchline == lines; i++)
			for(j = 0; j < values1.nbrows && matchline == lines; j++)
				if(!dyntab_cmp(&values, i, 0, &values1, j, 0))
					matchline = line;

		/* Handle different operator */
		if(!b_equal)
		{
			if(matchline < lines) matchline = lines;
			else matchline = line;
		}
	}
		
	/* Select access of 1st line matching user group */
	if(!lines || matchline < lines)
	{
		ctrl_parse_access(&acc, dyntab_val(&access, matchline, 0), "", "");
		ctrl->access &= acc;
	}
	else
		ctrl->access = AccessNone;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : debug_compare_res
** Description : return result & handle desbug messages for values comparison
*********************************************************************/
int debug_compare_res(					/* return : comparison result - 0 if not Ok */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *val1,					/* in : values set 1 */
	unsigned long i1,				/* in : matched value index in val1 */
	DynTable *val2,					/* in : values set 2 */
	unsigned long i2,				/* in : matched value index in val1 */
	MatchMode match,				/* in : comparison operator */
	int res							/* in : comparison result */
){
	/* Return if not debuging */
	if(!cntxt->sql_trace) return res;
	{
		char *op = "";
		size_t sz = 0;
		char buf[128];

		/* Output comparison value 1 */
		dynbuf_add(&cntxt->debug_msg, add_sz_str("\n*** compare_values : ('"), NO_CONV);
		dyntab_to_dynbuf(val1, &cntxt->debug_msg, add_sz_str(","), add_sz_str("','"), NO_CONV);
		dynbuf_add(&cntxt->debug_msg, add_sz_str("') "), NO_CONV);

		/* Output comparison query */
#define PRINT(x) op = x; sz = sizeof(x) - 1; break
		switch(match)
		{
		case Smaller:		PRINT("<");
		case SmallerEqual:	PRINT("<=");
		case Larger:		PRINT(">");
		case LargerEqual:	PRINT("=>");
		case Different:		PRINT("!=");
		default:			PRINT("==");
		}
#undef PRINT
		dynbuf_add(&cntxt->debug_msg, op, sz, NO_CONV);

		/* Output comparison value 2 */
		dynbuf_add(&cntxt->debug_msg, add_sz_str(" ('"), NO_CONV);
		dyntab_to_dynbuf(val2, &cntxt->debug_msg, add_sz_str(","), add_sz_str("','"), NO_CONV);
		dynbuf_add(&cntxt->debug_msg, add_sz_str("')"), NO_CONV);

		/* Output comparison result */
		dynbuf_add(&cntxt->debug_msg, buf,
					snprintf(add_sz_str(buf), "IS %s on '%s' %s '%s'", res ? "TRUE" : "FALSE",
					i1 < val1->nbrows ? dyntab_val(val1, i1, 0) : "NoMatch" , op,
					i2 < val2->nbrows ? dyntab_val(val2, i2, 0) : "NoMatch"), NO_CONV);
	}
	return res;
}


/*********************************************************************
** Function : compare_values
** Description : compare a couple of values	sets
*********************************************************************/
int compare_values(					/* return : comparison result - 0 if not Ok */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *val1,					/* in : values set 1 */
	char *oper,						/* in : comparison operator */
	DynTable *val2,					/* in : values set 2 */
	char *mode,						/* in : comparison mode */
	char *multiple					/* in : multiple values handling mode */
){
	MatchMode match = Exact;
	int b_num = mode && !strcmp(mode, "_EVA_NUMERIC");
	int b_all = multiple && *multiple == '0';
	unsigned long i, j = 0;
	int res = 0;

	/* Set match mode & values depending on comparison type */
#define USE_STD_VAL(tag, mode) if(!strcmp("_EVA_"#tag, oper)) match = mode;
	USE_STD_VAL(DIFFERENT, Different) else
	USE_STD_VAL(SMALLER, Smaller) else
	USE_STD_VAL(SMALLEREQUAL, SmallerEqual) else
	USE_STD_VAL(GREATER, Larger) else
	USE_STD_VAL(GREATEREQUAL, LargerEqual) else
	USE_STD_VAL(BEGINWITH, Begin) else
	USE_STD_VAL(NOT_BEGINWITH, NotBegin) else
	USE_STD_VAL(CONTAIN, Contain) else
	USE_STD_VAL(NOT_CONTAIN, NotContain)
#undef USE_STD_VAL

	/* Always check all values for Different operator */
	if(match == Different) b_all = 1;

	for(i = 0; !i || i < val1->nbrows; i++)
	{
		double v1 = b_num ? atof(dyntab_val(val1, i, 0)) : 0;
		for(j = 0; !j || j < val2->nbrows; j++)
		{
			double v2 = b_num ? atof(dyntab_val(val2, j, 0)) : 0;
			switch(match)
			{
			case Smaller:
				res = b_num ? v1 < v2 : dyntab_cmp(val1, i, 0, val2, j, 0) < 0;
				break;
			case SmallerEqual:
				res = b_num ? v1 <= v2 : dyntab_cmp(val1, i, 0, val2, j, 0) <= 0;
				break;
			case Larger:
				res = b_num ? v1 > v2 : dyntab_cmp(val1, i, 0, val2, j, 0) > 0;
				break;
			case LargerEqual:
				res = b_num ? v1 >= v2 : dyntab_cmp(val1, i, 0, val2, j, 0) >= 0;
				break;
			case Different:
				res = b_num ? v1 != v2 : dyntab_cmp(val1, i, 0, val2, j, 0) != 0;
				break;
			default:
				res = b_num ? v1 == v2 : !dyntab_cmp(val1, i, 0, val2, j, 0);
			}

			if(res ^ b_all) return debug_compare_res(cntxt, val1, i, val2, j, match, res);
		}
	}
	return debug_compare_res(cntxt, val1, i, val2, j, match, res);
}

/*********************************************************************
** Function : qry_values
** Description : evaluate a combination of value type / field / expression
*********************************************************************/
#define ERR_FUNCTION "qry_values"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					DYNTAB_FREE(obj); \
					DYNTAB_FREE(val)
int qry_values(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result values */
	char *valtyp,					/* in : type of source object */
	DynTable *fields,				/* in : list of objects to use as input fields our calc-ed values */
	DynTable *expr					/* in : list of sql / relational expression to evaluate */
){
	DynTable data = { 0 };
	DynTable obj = { 0 };
	DynTable val = { 0 };
	unsigned long i, j;

	/* Initialize */
	if(!valtyp || !*valtyp) valtyp = "_EVA_CURRENTOBJ";
	dyntab_free(res);

	/* Handle fields if present */
	if(fields && fields->nbrows)
	{
		/* Read control type */
		char *ctrltyp;
		EVA_ctrl ctrl = {0};
		if(qry_cache_idobj_field(cntxt, &data, DYNTAB_TOUL(fields), "_EVA_CONTROL", 0)) STACK_ERROR;
		ctrltyp = dyntab_val(&data, 0, 0);

		/* Calc-ed value : compute & add to result */
		if(!strcmp(ctrltyp, "_EVA_CALC"))
		{
			/* Calc-ed value : compute each value & add to result */
			for(i = 0; i < fields->nbrows; i++)
			{
				if(qry_cache_idobj(&ctrl.attr, DYNTAB_TOUL(fields)) ||
					ctrl_read_baseobj(cntxt, &ctrl.attr) ||
					ctrl_output_exprval(cntxt, &ctrl, 3)) CLEAR_ERROR;
				if(dyntab_from_tab(res, res->nbrows, 0, &ctrl.val, 0, 0, ctrl.val.nbrows, ctrl.val.nbcols, 33)) STACK_ERROR;
			}
			RETURN_OK;
		}
		else if(!strcmp(ctrltyp, "_EVA_INPUT"))
		{
			
			/* Input : Check input type */
			if(qry_cache_idobj_field(cntxt, &data, DYNTAB_TOUL(fields), "_EVA_TYPE", 2)) STACK_ERROR;
			ctrltyp = dyntab_val(&data, 0, 0);
			if((!strcmp(ctrltyp, "_EVA_RELATION") || !strcmp(ctrltyp, "_EVA_FILE_UPLOAD")) && expr && expr->nbrows)
			{
				/* Relation with expressions : read inputs values */
				if(qry_cache_objlist_field(cntxt, &data, fields, "_EVA_FIELD", 2)) STACK_ERROR;
				if(form_eval_valtype(cntxt, &val, valtyp, NULL, &data)) CLEAR_ERROR;

				/* Evaluate expressions on each relation object & add to result */
				if(expr) for(i = 0; i < val.nbrows; i++)
					for(j = 0; j < expr->nbrows; j++)
					{
						if(form_eval_fieldexpr(cntxt, &data, 0, DYNTAB_TOULRC(&val, i, 0), DYNTAB_VAL_SZ(expr, j, 0), NULL, 0))
							CLEAR_ERROR_CONT;
						if(dyntab_from_tab(res, res->nbrows, 0, &data, 0, 0, data.nbrows, data.nbcols, 33)) STACK_ERROR;
					}
			}
			else
			{
				/* Text input or relation without expressions : read inputs values */
				if(qry_cache_objlist_field(cntxt, &data, fields, "_EVA_FIELD", 2)) STACK_ERROR;
				if(form_eval_valtype(cntxt, res, valtyp, NULL, &data)) CLEAR_ERROR;
			}
			RETURN_OK;
		}
	}

	/* Evaluate expressions */
	if(form_eval_valtype(cntxt, res, valtyp, NULL, expr)) CLEAR_ERROR;


	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_cond
** Description : evaluate a condition
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_cond"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					DYNTAB_FREE(obj); \
					DYNTAB_FREE(expr); \
					DYNTAB_FREE(val); \
					DYNTAB_FREE(obj1); \
					DYNTAB_FREE(expr1); \
					DYNTAB_FREE(val1); \
					M_FREE(buf)
int ctrl_check_cond(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	int *res,						/* out : 0 if contion false, non zero else */
	unsigned long idcond,			/* in : condition to process (IdObj) */
	DynTable *altdata				/* in : list of objects to use if idcond is 0 */
){
	EVA_form *form = cntxt->form;
	DynTable data = { 0 };
	DynTable obj = { 0 };
	DynTable expr = { 0 };
	DynTable val = { 0 };
	DynTable obj1 = { 0 };
	DynTable expr1 = { 0 };
	DynTable val1 = { 0 };
	DynTable *idobj = idcond ? &obj : altdata;
	DynBuffer *buf = NULL;
	unsigned long i, j;
	char *typ = "";
	int b_edit = form->prevstep == HtmlEdit || form->prevstep == HtmlSaveDlg || form->nextstep == HtmlEdit;

	/* Initialize */
	*res = 0;

	/* Read condition definition */
	if(idcond)
	{
		if(qry_cache_idobj(&data, idcond)) STACK_ERROR;
		typ = DYNTAB_FIELD_VAL(&data, TYPE_COND);
	}
	else if(altdata && altdata->cell && altdata->cell->field)
		typ = altdata->cell->field;
	
	/* Check condition type */
	if(!strcmp(typ, "_EVA_USERINGROUP") || !strcmp(typ, "_EVA_USERNOTINGROUP"))
	{
		/* Check user group */
		if(idcond) DYNTAB_FIELD(&obj, &data, USERGROUP_COND);
		for(i = 0; i < idobj->nbrows; i++)
		{
			/* Search selected group in user groups */
			DynTableCell *c = dyntab_cell(idobj, i, 0);
			for(j = 0; j < cntxt->user_groups.nbrows && dyntab_txt_cmp(c->txt, c->len, &cntxt->user_groups, j, 0); j++);
			if(j < cntxt->user_groups.nbrows) break;
		}
		/* Matched if INGROUP and found or NOTINGROUP and not found */
		*res = !strcmp(typ, "_EVA_USERINGROUP") ^ (i == idobj->nbrows);
	}
	else if(!strcmp(typ, "_EVA_CURCONDTRUE") || !strcmp(typ, "_EVA_CURCONDFALSE"))
	{
		/* Check condition on current object */
		if(idcond) DYNTAB_FIELD(&obj, &data, FILTER_CONDSEQ);
		DYNBUF_ADD_STR(&buf, "=");
		if(idobj->nbrows && qry_filterlist_to_sql(cntxt, &buf, idobj, "AND")) CLEAR_ERROR_RETURN;
		if(buf && form_eval_fieldexpr(cntxt, &obj, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), DYNBUF_VAL_SZ(buf), NULL, 0)) CLEAR_ERROR_RETURN;

		/* Matched if CONDTRUE and 1 or CONDFALSE and not 1 */
		*res = !strcmp(typ, "_EVA_CURCONDTRUE") ^ (strcmp(dyntab_val(&obj, 0, 0), "1") != 0);
	}
	else if(!strcmp(typ, "_EVA_CONDTRUE") || !strcmp(typ, "_EVA_CONDFALSE"))
	{
		/* Check condition on all objects */
		DYNTAB_FIELD(&obj, &data, FILTER_CONDSEQ);
		if(qry_to_listobj(cntxt, &buf, NULL, &obj)) CLEAR_ERROR_RETURN;

		/* Matched if CONDTRUE and objects found or CONDFALSE and no objects found */
		*res = !strcmp(typ, "_EVA_CONDTRUE") ^ (buf == NULL);
	}
	else if(!strcmp(typ, "_EVA_ISNOTEMPTY") || !strcmp(typ, "_EVA_ISEMPTY"))
	{
		/* Check value presence */
		if(idcond)
		{
			DYNTAB_FIELD(&obj, &data, CTRL_VAL1);
			DYNTAB_FIELD(&expr, &data, EXPR_VAL1);
			if(qry_values(cntxt, &val, DYNTAB_FIELD_VAL(&data, TYPE_VAL1), &obj, &expr)) CLEAR_ERROR_RETURN;
		}
		else
			if(qry_values(cntxt, &val, NULL, altdata, NULL)) CLEAR_ERROR_RETURN;

		/* Matched if ISEMPTY and no values or ISNOTEMPTY and values */
		*res = !strcmp(typ, "_EVA_ISEMPTY") ^ (val.nbrows != 0);
	}
	else if(!strcmp(typ, "_EVA_COMPARE_STATIC"))
	{
		/* Compare values with fixed values */
		DYNTAB_FIELD(&obj1, &data, CTRL_VAL1);
		DYNTAB_FIELD(&expr1, &data, EXPR_VAL1);
		DYNTAB_FIELD(&val, &data, COMPARE_VALUE);
		if(qry_values(cntxt, &val1, DYNTAB_FIELD_VAL(&data, TYPE_VAL1), &obj1, &expr1)) CLEAR_ERROR_RETURN;

		/* Match if comparison succeed */
		*res = compare_values(cntxt, &val1, DYNTAB_FIELD_VAL(&data, TYPE_COMPARE), &val,
										DYNTAB_FIELD_VAL(&data, MODE_COMPARE),
										DYNTAB_FIELD_VAL(&data, MULTIPLE_COMPARE));
	}
	else if(!strcmp(typ, "_EVA_COMPARE_VALUES"))
	{
		/* Compare 2 values */
		DYNTAB_FIELD(&obj1, &data, CTRL_VAL1);
		DYNTAB_FIELD(&expr1, &data, EXPR_VAL1);
		DYNTAB_FIELD(&obj, &data, CTRL_VAL2);
		DYNTAB_FIELD(&expr, &data, EXPR_VAL2);
		if(qry_values(cntxt, &val1, DYNTAB_FIELD_VAL(&data, TYPE_VAL1), &obj1, &expr1) ||
			qry_values(cntxt, &val, DYNTAB_FIELD_VAL(&data, TYPE_VAL2), &obj, &expr)) CLEAR_ERROR_RETURN;

		/* Match if comparison succeed */
		*res = compare_values(cntxt, &val1, DYNTAB_FIELD_VAL(&data, TYPE_COMPARE), &val,
										DYNTAB_FIELD_VAL(&data, MODE_COMPARE),
										DYNTAB_FIELD_VAL(&data, MULTIPLE_COMPARE));
	}
	else
		/* Check other condition */
		*res = 
			/* Empty or always true */
			!*typ || !strcmp(typ, "_EVA_ALWAYS") ||
			/* New object */
			!strcmp(typ, "_EVA_NEWOBJ") && !dyntab_sz(&form->id_obj, 0, 0) ||
			/* Existing object */
			!strcmp(typ, "_EVA_EXISTOBJ") && dyntab_sz(&form->id_obj, 0, 0) ||
			/* User identified */
			!strcmp(typ, "_EVA_USERIDENT") && cntxt->b_identified ||
			/* User not identified */
			!strcmp(typ, "_EVA_USERUNKNOWN") && !cntxt->b_identified ||
			/* Form in edit mode */
			!strcmp(typ, "_EVA_FORMEDIT") && b_edit ||
			/* Form in display mode */
			!strcmp(typ, "_EVA_FORMDISPLAY") && !b_edit;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_condlist
** Description : evaluate a condition table line
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_condlist"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					DYNTAB_FREE(conds); \
					DYNTAB_FREE(groups); \
					DYNTAB_FREE(filters); \
					DYNTAB_FREE(controls); \
					M_FREE(sql)
int ctrl_check_condlist(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	int *res,						/* out : 0 if contion false, non zero else */
	DynTable *cond,					/* in : table of conditions to process (IdObj) */
	unsigned long line				/* in : table line to process (AND combination on columns of line)
											if ~0UL : process all lines (AND combination on column 0) */
){
	DynTable data = { 0 };
	DynTable conds = { 0 };
	DynTable groups = { 0 };
	DynTable filters = { 0 };
	DynTable controls = { 0 };
	DynTable *tab;
	DynBuffer *sql = NULL;
	unsigned long i, nb = line == ~0UL ? cond->nbrows : cond->nbcols;

	/* Initialize */
	*res = 1;

	/* For each condition to process */
	for(i = 0; i < nb; i++)
	{
		/* Read condition type */
		unsigned long r = line == ~0UL ? i : line;
		unsigned long c = line == ~0UL ? 0 : i;
		unsigned long idcond = DYNTAB_TOULRC(cond, r, c);
		if(!idcond) continue;
		if(qry_cache_idobj(&data, idcond)) STACK_ERROR;
		
		/* Split conditions, user groups & filters */
		tab =	DYNTAB_FIELD_CELL(&data, TYPE_COND) ? &conds :
				DYNTAB_FIELD_CELL(&data, CONTROL) ? &controls :
				DYNTAB_FIELD_CELL(&data, FILTER_TYPE) ? &filters : &groups;
		DYNTAB_ADD_CELL(tab, tab->nbrows, 0, cond, r, c);
	}

	/* Check user groups */
	if(groups.nbrows)
	{
		groups.cell->field = "_EVA_USERINGROUP";
		groups.cell->b_dontfreefield = 1;
		if(ctrl_check_cond(cntxt, res, 0, &groups)) STACK_ERROR;
	}

	/* Check controls */
	if(*res && controls.nbrows)
	{
		controls.cell->field = "_EVA_ISNOTEMPTY";
		controls.cell->b_dontfreefield = 1;
		if(ctrl_check_cond(cntxt, res, 0, &controls)) STACK_ERROR;
	}
		
	/* Check conditions */
	if(*res) for(i = 0; i < conds.nbrows && *res; i++)
		if(ctrl_check_cond(cntxt, res, DYNTAB_TOULRC(&conds, i, 0), NULL)) STACK_ERROR;

	/* Check filters */
	if(*res && filters.nbrows)
	{
		filters.cell->field = "_EVA_CURCONDTRUE";
		filters.cell->b_dontfreefield = 1;
		if(ctrl_check_cond(cntxt, res, 0, &filters)) STACK_ERROR;
	}

	/* Handle debug */
	if(cntxt->sql_trace)
	{
		DYNBUF_ADD3_INT(&cntxt->debug_msg, "\n*** Access - line ", line, " res=");
		DYNBUF_ADD_INT(&cntxt->debug_msg, *res);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_condseq
** Description : evaluate a condition sequence - return matched line
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_condseq"
#define ERR_CLEANUP 
int ctrl_check_condseq(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *cond,					/* in : table of conditions to process (IdObj) */
	unsigned long *match			/* out : index of first true condition - cond->nbrows if no match */
){
	int res = 0;

	/* For each line of condition table */
	for(*match = 0; *match < cond->nbrows; (*match)++)
	{
		/* Check condition line */
		if(ctrl_check_condlist(cntxt, &res, cond, *match)) STACK_ERROR;
		if(res) break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_access_condseq
** Description : get access status of a control for the identified user	- condition sequence
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_access_condseq"
#define ERR_CLEANUP DYNTAB_FREE(access); \
					DYNTAB_FREE(cond)
int ctrl_check_access_condseq(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl					/* in : control to check for access status
									  out : access status is set in ctrl->access */
){
	DynTable access = { 0 };
	DynTable cond = { 0 };
	unsigned long lines, match;

	/* Read condition sequence definition */
	CTRL_ATTR_TAB(cond, COND_ACCESS);
	CTRL_ATTR_TAB(access, ACCESS_CONDSEQ);

	/* Process condition table */
	if(ctrl_check_condseq(cntxt, &cond, &match)) STACK_ERROR;

	/* Handle matched condition */
	lines = access.nbrows > cond.nbrows ? access.nbrows : cond.nbrows;
	if(!lines || match < lines)
		ctrl_parse_access(&ctrl->access, dyntab_val(&access, match, 0), "", "");
	else
		ctrl->access = AccessNone;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_access
** Description : get access status of a control for the identified user
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_access"
#define ERR_CLEANUP DYNTAB_FREE(access); \
					DYNTAB_FREE(acctype); \
					DYNTAB_FREE(accgroup); \
					DYNTAB_FREE(acccond); \
					cntxt->sql_trace = sql_trace
int ctrl_check_access(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl					/* in : control to check for access status
									  out : access status is set in ctrl->access */
){
	EVA_form *form = cntxt->form;
	char *acctyp = CTRL_ATTR_VAL(ACCES_TYPE);
	char *chkwhen = CTRL_ATTR_VAL(ACCES_CHECK_WHEN);
	DynTable access = { 0 };
	DynTable acctype = { 0 };
	DynTable accgroup = { 0 };
	DynTable acccond = { 0 };

	/* Handle debug */
	int sql_trace = cntxt->sql_trace;
	cntxt->sql_trace = atoi(CTRL_ATTR_VAL(DEBUG_ACCESS));

	/* Handle old access definition */
	if(!*acctyp || !strcmp(acctyp, "_EVA_BASIC_ACCESS"))
	{
		if(ctrl_check_access_v0(cntxt, ctrl)) STACK_ERROR;
	}
	/* Standard access definition */
	else
	{
		/* Check basic access at read time */
		if(form->step == CtrlRead)
		{
			/* Check object access for forms & top level control */
			if(!ctrl->CONTROL || !strcmp(ctrl->CONTROL, "_EVA_FORM") || ctrl == form->ctrl)
			{
				if(obj_check_user_access(cntxt, &form->objdata, 0, &ctrl->access)) STACK_ERROR;
			}
			else
				ctrl->access = AccessAll;

			/* Handle access definition methods */
			if(ctrl->access != AccessNone)
			{
				if(!strcmp(acctyp, "_EVA_INHERIT"))
				{
					ctrl->access &= form->ctrl[ctrl->i_parent].access;
				}
				else if(!strcmp(acctyp, "_EVA_READONLY"))
				{
					ctrl->access &= AccessView;
				}
				else if(!strcmp(acctyp, "_EVA_INHERIT_COND"))
				{
					ctrl->access &= form->ctrl[ctrl->i_parent].access;
					if(!*chkwhen && ctrl_check_access_condseq(cntxt, ctrl)) STACK_ERROR;
				}
				else ctrl->access = AccessNone;
			}
			ctrl->access0 = ctrl->access;
		}
		/* Check final access after read time */
		else if(!ctrl->accesschkd &&  ctrl->access != AccessNone)
		{
			ctrl->accesschkd = 1;
			if(!strcmp(acctyp, "_EVA_INHERIT"))
			{
				ctrl->access &= form->ctrl[ctrl->i_parent].access;
			}
			else if(!strcmp(acctyp, "_EVA_INHERIT_COND"))
			{
				ctrl->access &= form->ctrl[ctrl->i_parent].access;
				if(*chkwhen && ctrl_check_access_condseq(cntxt, ctrl)) STACK_ERROR;
			}
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

