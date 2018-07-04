/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_create.c
** Description : handler for buttons & actions of type CREATE
**				 create series of objects
**      Author : Alain BOUTE
**     Created : Oct 16 2004
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : action_create_objects
** Description : button click handler for CREATE
*********************************************************************/
#define ERR_FUNCTION "action_create_objects"
#define ERR_CLEANUP	DYNTAB_FREE(srcobj); \
					DYNTAB_FREE(dstobj); \
					DYNTAB_FREE(dstfield); \
					DYNTAB_FREE(dstrelmod); \
					DYNTAB_FREE(dstkey); \
					DYNTAB_FREE(srcfield); \
					DYNTAB_FREE(srcvaltyp); \
					DYNTAB_FREE(srcvalobj); \
					DYNTAB_FREE(listobj); \
					DYNTAB_FREE(field); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(objdata); \
					qry_build_free(&flt); \
					M_FREE(label)
int action_create_objects(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable srcobj = {0};
	DynTable dstobj = {0};
	DynTable dstfield = {0};
	DynTable dstrelmod = {0};
	DynTable dstkey = {0};
	DynTable srcfield = {0};
	DynTable srcvaltyp = {0};
	DynTable srcvalobj = {0};
	DynTable listobj = {0};
	DynTable field = {0};
	DynTable data = {0};
	DynTable objdata = {0};
	DynTable lineval = {0};
	QryBuild flt = {0};
	DynBuffer *label = NULL;
	unsigned long dstform = strtoul(CTRL_ATTR_VAL(CREATE_FORM), NULL, 10);
	unsigned long idform = DYNTAB_TOUL(&form->id_form);
	unsigned long idobj = DYNTAB_TOUL(&form->id_obj);
	unsigned long i, j, k;	  
	char *crmode = CTRL_ATTR_VAL(CREATE_MODE);
	char *dialog = CTRL_ATTR_VAL(DIALOG);
	char *after = CTRL_ATTR_VAL(EXEC_AFTER);
	unsigned long limit = strtoul(CTRL_ATTR_VAL(LIMIT), NULL, 10);
	int b_msg;
	DynTableCell *newobjfield = CTRL_ATTR_CELL(NEWOBJ_FIELD);
	DynTableCell *srcupdate = CTRL_ATTR_CELL(SRC_UPDATE);
	int b_lineval = !strcmp(crmode, "_EVA_VALUES_TABLE");
	char *confirm = CTRL_ATTR_VAL(CONFIRM);
	int b_savetodb = *confirm && strcmp(confirm, "_EVA_NOSAVE");

	/* Handle save before */
	if(b_savetodb)
	{
		int b_done = BUTN_SAVE_NEXT;
		if(form_save_dialog(cntxt, i_ctrl, confirm, BUTN_SAVE_NEXT, &b_done)) STACK_ERROR;
		if(b_done != BUTN_SAVE_NEXT) RETURN_OK;
	}

	/* Read created values definition */
	CTRL_ATTR_TAB(dstfield, DESTFIELD);
	CTRL_ATTR_TAB(dstrelmod, DEST_RELMODE);
	CTRL_ATTR_TAB(dstkey, DEST_KEY);
	CTRL_ATTR_TAB(srcvaltyp, SRCTYPE);
	CTRL_ATTR_TAB(srcvalobj, SRCLISTOBJ);
	CTRL_ATTR_TAB(srcfield, SRCFIELD);


	/* Handle create method */
	if(!strcmp(crmode, "_EVA_RELATION"))
	{
		/* Create one object per object in given relation controls */
		CTRL_ATTR(srcobj, SRCOBJ_INPUT);

		/* Read control values on current object */ 
		if(qry_listobj_field(cntxt, &data, &srcobj, "_EVA_FIELD")) STACK_ERROR;
		if(dyntab_to_dynbuf(&data, &label, ",", 1, ",", 1, NO_CONV)) STACK_ERROR;
		if(label && form_get_field_values(cntxt, &srcobj, label->data, idform, idobj)) STACK_ERROR;
		M_FREE(label);
	}
	else if(!strcmp(crmode, "_EVA_CONDEXPR"))
	{
		/* Create one object per object matching given filter */
		/* Evaluate filter */ 
		CTRL_ATTR(data, SELOBJ_FILTER);
		if(qry_add_filter_forms(cntxt, &flt, NULL, &data)) STACK_ERROR;
		if(qry_filter_objects(cntxt, &srcobj, &flt)) STACK_ERROR;
	}
	else if(b_lineval)
	{
		/* Create one object per line of destination values */
		if(dyntab_resize(&lineval, 1, dstfield.nbrows)) RETURN_ERR_MEMORY;

		/* Compute values for each field */
		for(i = 0; i < dstfield.nbrows; i++)
		{
			/* Transfer value definition */
			char *srctyp = dyntab_val(&srcvaltyp, i, 0);
			if(!*srctyp) srctyp = "_EVA_VALUE";
			DYNTAB_SET_CELL(&field, 0, 0, &srcfield, i, 0);
			field.cell->Line = 0;
			dyntab_free(&listobj);
			for(k = 0; k < srcvalobj.nbcols && dyntab_sz(&srcvalobj, i, k); k++)
				DYNTAB_SET_CELL(&listobj, k, 0, &srcvalobj, i, k);

			/* Evaluate value - abort objects creation on error */
			if(ctrl_eval_valtyp(cntxt, ctrl, &data, srctyp, &listobj, &field)) 
			{
				CLEAR_ERROR;
				RETURN_OK;
			}

			/* Store result in one column of temp table & count lines */
			for(j = 0; j < data.nbrows; j++) 
			{
				DynTableCell *c = dyntab_cell(&data, j, 0), *c0;
				DYNTAB_ADD_CELLP(&lineval, j, i, c);
				c0 = dyntab_cell(&lineval, 0, i);
				if(!j) c0->row = data.nbrows;
				c0->col |= c->Line ? 2 : 1;
				if(!c->Line) continue;
				for(k = 0; k < srcobj.nbrows && DYNTAB_TOULRC(&srcobj, k, 0) != c->Line; k++);
				if(k == srcobj.nbrows) DYNTAB_ADD_INT(&srcobj, srcobj.nbrows, 0, c->Line);
			}
		}
	}
	else
	{
		/* Create a single object */
		DYNTAB_ADD_INT(&srcobj, 0, 0, idobj);
	}

	/* Output page header if applicable */
	b_msg = !*dialog && srcobj.nbrows > 1 ||
			!strcmp(dialog, "_EVA_ALWAYS") ||
			!strcmp(dialog, "_EVA_COUNT") && limit <= srcobj.nbrows;
	if(b_msg)
	{
		put_html_page_header(cntxt, NULL, NULL, NULL, 3);
		printf("<font face=Arial><hr><b>%s</b><hr>", ctrl->LABEL);
		cntxt->b_terminate = 15;
	}

	/* For each source object */
	for(i = 0; i < srcobj.nbrows; i++)
	{
		unsigned long idsrc = DYNTAB_TOULRC(&srcobj, i, 0), iddest = 0;
		int b_create = 1;
		dyntab_free(&objdata);

		/* Display progress message if applicable */
		if(b_msg)
		{
			if(qry_obj_field(cntxt, &data, idsrc, NULL)) STACK_ERROR;
			if(qry_obj_label(cntxt, &label, NULL, &label, NULL, NULL, NULL, NULL, NULL, 0, &data, 0)) CLEAR_ERROR;
			printf("==&gt; <font color=#0000FF>%s</font><br>", label ? label->data : "???");
		}

		/* Calc values for each destination field */
		for(j = 0; j < dstfield.nbrows; j++)
		{
			/* Get relation & key options */
			DynTableCell *fld = dyntab_cell(&dstfield, j, 0);
			int valmode = atoi(dyntab_val(&dstrelmod, j, 0));
			int b_key = !dstkey.nbrows || dyntab_sz(&dstkey, j, 0) > 0;

			/* Get values from temp table if already computed */
			if(b_lineval)
			{
				DynTableCell *c0 = dyntab_cell(&lineval, 0, j);
				dyntab_free(&data);
				for(k = 0; k < c0->row; k++) 
				{
					DynTableCell *c = dyntab_cell(&lineval, k, j);
					unsigned long row = data.nbrows;
					if(c->Line != idsrc && c0->col & 2) continue;
					DYNTAB_SET_CELLP(&data, row, 0, c);
					dyntab_cell(&data, row, 0)->Line = 0;
				}
			}
			else
			/* Compute values on current object */
			{
				/* Handle value source type & field */
				char *srctyp = dyntab_val(&srcvaltyp, j, 0);
				if(!*srctyp) srctyp = "_EVA_VALUE";
				dyntab_free(&listobj);
				if(!strcmp(srctyp, "_EVA_PROC_SELOBJ"))
				{
					/* Handle specific value source type : selected object */
					DYNTAB_ADD_INT(&listobj, 0, 0, idsrc);
					srctyp = "_EVA_LISTOBJ";
				}
				else
					for(k = 0; k < srcvalobj.nbcols && dyntab_sz(&srcvalobj, j, k); k++)
						DYNTAB_SET_CELL(&listobj, k, 0, &srcvalobj, j, k);
				dyntab_free(&field);
				DYNTAB_SET_CELL(&field, 0, 0, &srcfield, j, 0);
				field.cell->Line = 0;

				/* Evaluate value - don't create this object if error */
				if(ctrl_eval_valtyp(cntxt, ctrl, &data, srctyp, &listobj, &field))
				{
					b_create = 0;
					if(b_msg) printf("<font color=#FF0000>%s</font> : %s<br>", fld->txt, cntxt->err.text);
					CLEAR_ERROR;
					break;
				}
			}

			/* Add values to object data */
			for(k = 0; k < data.nbrows; k++)
			{
				unsigned long row = objdata.nbrows;
				DynTableCell *c;

				/* Handle decimal values */
				if(valmode == 2)
				{
					char *tmp;
					c = dyntab_cell(&data, k, 0);
					tmp = input_to_number(c->txt);
					if(strcmp(tmp, c->txt)) DYNTAB_ADD(&data, k, 0, tmp, strlen(tmp), NO_CONV);
				}

				/* Set value attributes */
				DYNTAB_ADD_CELL(&objdata, row, 0, &data, k, 0);
				c = dyntab_cell(&objdata, row, 0);
				if(c->field && !c->b_dontfreefield) M_FREE(c->field);
				c->field = fld->txt;
				c->IdField = fld->IdValue;
				c->b_dontfreefield = 1;
				c->b_relation |= (valmode == 1);
				c->i_cgi = b_key ? j : ~0UL;
			}
		}

		/* Continue if no data */
		if(!objdata.nbrows || !b_create)
		{
			/* Display message if applicable */
			if(b_msg) printf("<font color=#FFAA00>Pas de données - fiche non créée</font><br>");
			continue;
		}

		/* Check for duplicates - add form filter */
		qry_build_free(&flt);
		if(qry_add_filter_simple_int(cntxt, &flt, RelNone, "_EVA_FORMSTAMP", RelList, dstform)) STACK_ERROR;

		/* Add filter for each key value */
		for(j = 0; j < objdata.nbrows; j++)
		{
			DynTableCell *c = dyntab_cell(&objdata, j, 0);
			if(c->i_cgi == ~0UL || !c->txt || !c->txt[0] || !c->len) continue;
			dyntab_free(&data);
			DYNTAB_SET_CELLP(&data, 0, 0, c);
			dyntab_free(&field);
			DYNTAB_SET_CELLP(&field, 0, 0, dyntab_cell(&dstfield, c->i_cgi,0));
			if(qry_add_filter(cntxt, &flt, RelNone, &field, c->b_relation ? RelList :  Like, &data)) STACK_ERROR;
		}

		/* Exec query for duplicates if keys where present */
		dyntab_free(&data);
		if(flt.nbnode > 1 && qry_filter_objects(cntxt, &data, &flt)) STACK_ERROR;

		/* Continue if duplicate object exists */
		if(data.nbrows)
		{
			/* Display message with existing object if applicable */
			if(b_msg)
			{
				M_FREE(label);
				if(qry_obj_field(cntxt, &field, DYNTAB_TOUL(&data), NULL)) STACK_ERROR;
				if(qry_obj_label(cntxt, &label, NULL, &label, NULL, NULL, NULL, NULL, NULL, 0, &field, 0)) CLEAR_ERROR;
				printf("<font color=#FFAA00>La fiche existe déjà</font> : %s<br>", label ? label->data : "???");
			}
			continue;
		}

		/* Create new object & set data */
		if(qry_add_new_idobj(cntxt, &iddest, dstform)) STACK_ERROR;
		for(j = 0; j < objdata.nbrows; j++)
		{
			DynTableCell *c = dyntab_cell(&objdata, j, 0);
			if(!c->txt || !c->txt[0] || !c->len) continue;
			c->IdObj = iddest;
			if(qry_add_val(cntxt, c, 2, NULL)) STACK_ERROR;
		}
		DYNTAB_ADD_INT(&dstobj, dstobj.nbrows, 0, iddest);
		dyntab_cell(&dstobj, dstobj.nbrows - 1, 0)->b_relation = 1;

		/* Display message with created object if applicable */
		if(b_msg)
		{
			M_FREE(label);
			if(qry_obj_field(cntxt, &field, iddest, NULL)) STACK_ERROR;
			if(qry_obj_label(cntxt, &label, NULL, &label, NULL, NULL, NULL, NULL, NULL, 0, &field, 0)) CLEAR_ERROR;
			printf("<font color=#00FF00>++++ Fiche créée</font> : %s<br>", label ? label->data : "???");
		}

		/* Update source object if applicable */
		if(srcupdate)
		{
			char idrelobj[32];
			DynTableCell val = {0};
			val.field = srcupdate->txt;
			val.IdField = srcupdate->IdValue;
			val.IdObj = b_lineval ? idobj : idsrc;
			val.txt = idrelobj;
			val.len = snprintf(add_sz_str(idrelobj), "%lu", iddest);
			val.b_relation = 1;
			if(qry_add_val(cntxt, &val, 2, NULL)) STACK_ERROR;
		}
	}

	/* Store list of created objects in given field */
	if(newobjfield)
	{
		/* If object was saved or is not current object */
		if(b_savetodb)
		{
			/* Update data base */
			if(qry_update_idobj_idfield(cntxt, idobj, newobjfield->IdValue, &dstobj, 1))
				CLEAR_ERROR;
		}
		else 
			/* Update CGI data */
			if(cgi_set_field_values(cntxt, idform, idobj, newobjfield->txt, newobjfield->len, &dstobj, "_EVA_ADD", 0))
				CLEAR_ERROR;
	}
 
	/* Output page footer if applicable */
	if(b_msg)
	{
		printf("<hr><br><center><input type=image name='B$#NOP' src='../img/_eva_btn_gobackobj_fr.gif'></center><br>\n");
	}
	if(*after && strcmp(after, "_EVA_NONE"))
	{
		if(form_close_nosave(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), NULL)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
