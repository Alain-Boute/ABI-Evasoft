/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_update.c
** Description : handling fonctions for update buttons
**      Author : Alain BOUTE
**     Created : July 1 2003
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : action_update
** Description : button click handler for UPDATE
*********************************************************************/
#define ERR_FUNCTION "action_update"
#define ERR_CLEANUP	DYNTAB_FREE(destfield); \
					DYNTAB_FREE(srctype); \
					DYNTAB_FREE(srcvalobj); \
					DYNTAB_FREE(srcval); \
					DYNTAB_FREE(setmode); \
					DYNTAB_FREE(userel); \
					DYNTAB_FREE(values); \
					DYNTAB_FREE(debug); \
					DYNTAB_FREE(updobj); \
					DYNTAB_FREE(updfield); \
					DYNTAB_FREE(listobj); \
					DYNTAB_FREE(tmpval); \
					DYNTAB_FREE(curval); \
					M_FREE(expr); \
					cntxt->sql_trace = sql_trace
int action_update(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable destfield = { 0 };
	DynTable srctype = { 0 };
	DynTable srcvalobj = { 0 };
	DynTable srcval = { 0 };
	DynTable setmode = { 0 };
	DynTable userel = { 0 };
	DynTable values = { 0 };
	DynTable debug = { 0 };
	DynTable updobj = { 0 };
	DynTable updfield = { 0 };
	DynTable listobj = { 0 };
	DynTable tmpval = { 0 };
	DynTable curval = { 0 };
	DynBuffer *expr = NULL;
	int sql_trace = cntxt->sql_trace;
	unsigned long i, j;
	char *confirm = CTRL_ATTR_VAL(CONFIRM);
	char *after = CTRL_ATTR_VAL(EXEC_AFTER);
	int b_savetodb = *confirm && strcmp(confirm, "_EVA_NOSAVE");
	unsigned long curobj;
	unsigned long curform = DYNTAB_TOUL(&cntxt->form->id_form);
	char *updobjtyp = CTRL_ATTR_VAL(UPDATE);
	if(!*updobjtyp) updobjtyp = "_EVA_CURRENTOBJ";

	/* Handle save before */
	if(b_savetodb)
	{
		int b_done = BUTN_SAVE_NEXT;
		if(form_save_dialog(cntxt, i_ctrl, confirm, BUTN_SAVE_NEXT, &b_done)) STACK_ERROR;
		if(b_done != BUTN_SAVE_NEXT) RETURN_OK;
	}
	curobj = DYNTAB_TOUL(&cntxt->form->id_obj);

	/* Read list of objects to uppdate */
	CTRL_ATTR(updfield, UPDATE_OBJFIELD);
	if(strcmp(updobjtyp, "_EVA_CURRENTOBJ") || curobj || updfield.nbrows)
	{
		if(!strcmp(updobjtyp, "_EVA_FILTER"))
		{
			CTRL_ATTR(listobj, FILTERS);
			if(!listobj.nbrows) RETURN_WARNING("No filter given : UPDATE operation ignored for button");
		}
		else CTRL_ATTR(listobj, LISTOBJ);
		if(ctrl_eval_valtyp(cntxt, ctrl, &updobj, updobjtyp, &listobj, &updfield)) STACK_ERROR;
		if(!updobj.nbrows > 1000) RETURN_WARNING("Too many objects selected (over 1000) : UPDATE operation ignored for button");
	}
  	else DYNTAB_ADD_STR(&updobj, 0, 0, "0");

	/* Read update parameters */
	CTRL_ATTR_TAB(destfield, DESTFIELD);
	CTRL_ATTR_TAB(srctype, SRCTYPE);
	CTRL_ATTR_TAB(srcvalobj, SRCLISTOBJ);
	CTRL_ATTR_TAB(srcval, SRCFIELD);
	CTRL_ATTR_TAB(setmode, SET_MODE);
	CTRL_ATTR_TAB(userel, DEST_RELMODE);
	CTRL_ATTR_TAB(debug, DEBUG_SQL_EXPR);

	/* For each destination field */
	for(i = 0; i < destfield.nbrows; i++)
	{
		/* Read source value / expression  for this destination field */
		unsigned long idfield = dyntab_cell(&destfield, i, 0)->IdValue;
		char *mode = dyntab_val(&setmode, i, 0);
		char *typ = dyntab_val(&srctype, i, 0);
		int b_same = !strcmp(typ, "_EVA_DESTOBJ");

		/* Handle debug */
		cntxt->sql_trace = dyntab_sz(&debug, i, 0) ? 2 : 0;

		/* Evaluate source expression on source object if not destination object */
		if(!b_same)
		{
			DYNTAB_FREE(values);
			DYNTAB_FREE(listobj);
			for(j = 0; j < srcvalobj.nbcols; j++)
				if(dyntab_sz(&srcvalobj, i, j))
					DYNTAB_SET_CELL(&listobj, listobj.nbrows, 0, &srcvalobj, i, j);
			for(j = 0; j < srcval.nbcols; j++)
				if(dyntab_sz(&srcval, i, j))
					DYNTAB_ADD_CELL(&values, values.nbrows, 0, &srcval, i, j);
			if(ctrl_eval_valtyp(cntxt, ctrl, &tmpval, typ, &listobj, &values)) CLEAR_ERROR_CONT;
		}
		else
		{
			/* Optimize variable evaluation */
			M_FREE(expr);
			if(qry_eval_sqlexpr_var(cntxt, &expr, dyntab_val(&srcval, i, 0), &ctrl->attr, NULL)) STACK_ERROR;
		}

		/* For each object to update */
		if(tmpval.nbrows || !strcmp("_EVA_REPLACE", mode) || b_same)
			for(j = 0; j < updobj.nbrows; j++)
			{
				unsigned long idobj = DYNTAB_TOULRC(&updobj, j, 0);
				unsigned long k, num = 0, line= 0;

				/* Evaluate source expression on destination object */
				if(b_same && form_eval_fieldexpr(cntxt, &tmpval, 0, idobj, DYNBUF_VAL_SZ(expr), NULL, 0)) CLEAR_ERROR_CONT;

				/* Handle destination values indexes if add line mode */
				if(!strcmp(mode, "_EVA_ADD_LINE"))
				{
					/* Get values for destination field in destination object */
					if(form_get_field_values(cntxt, &curval, dyntab_val(&destfield, i, 0),
											idobj == curobj ? curform : 0, idobj)) STACK_ERROR;
					for(k = 0; k < curval.nbrows; k++)
					{
						DynTableCell *c = dyntab_cell(&curval, k, 0);
						if(c->Line > line) line = c->Line;
						if(c->Num > num) num = c->Num;
					}

					/* Set values indexes */
					for(k = 0; k < tmpval.nbrows; k++)
					{
						DynTableCell *c = dyntab_cell(&tmpval, k, 0);
						c->Num = 1;
						c->Line = line + k + 1;
					}
				}

				/* Set values format as applicable */
				if(*dyntab_val(&userel, i, 0))
				{
					char *v = dyntab_val(&userel, i, 0), *tmp;
					for(k = 0; k < tmpval.nbrows; k++)
					{
						DynTableCell *c =  dyntab_cell(&tmpval, k, 0);
						switch(*v)
						{
						case '1':
							c->b_relation = 1;
							break;
						case '2':
							tmp = input_to_number(c->txt);
							if(strcmp(tmp, c->txt)) DYNTAB_ADD(&tmpval, k, 0, tmp, strlen(tmp), NO_CONV);
							break;
						}
					}
				}

				/* If object was saved or is not current object */
				if((b_savetodb || idobj != curobj) && strcmp(dyntab_val(&destfield, i, 0), "_EVA_SELTAB"))
				{
					/* Update data base */
					if(qry_update_idobj_idfield(cntxt, idobj, idfield, &tmpval,
							!strcmp("_EVA_REPLACE", mode) ? 3 : !strcmp("_EVA_ADD", mode) ? 0 : !strcmp("_EVA_REMOVE", mode) ? 5 : 1))
						STACK_ERROR;
				}
				else 
					/* Update CGI data */
					if(cgi_set_field_values(cntxt,
							DYNTAB_TOUL(&cntxt->form->id_form), idobj,
							DYNTAB_VAL_SZ(&destfield, i, 0), 
							&tmpval, mode, 0))
						CLEAR_ERROR;
			}
	}

	if(!*after || !strcmp(after, "_EVA_NONE"))
	{
		if(!b_savetodb) form->nextstep = HtmlEdit;
	}
	else if(!strcmp(after, "_EVA_CLOSE"))
	{
		if(form_close_nosave(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), NULL)) STACK_ERROR;
	}
	else if(!strcmp(after, "_EVA_ACTION"))
	{
		if(action_sequence(cntxt, i_ctrl)) STACK_ERROR;
	}
	form->b_reload = 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
