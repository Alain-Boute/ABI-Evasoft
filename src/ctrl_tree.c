/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_tree.c
** Description : handling functions for objects tree controls
**      Author : Alain BOUTE
**     Created : Apr 01 2002
*********************************************************************/

#include "eva.h"

typedef struct _TreeDef 
{
	unsigned long i_ctrl;			/* control index in cntxt->form->ctrl */
	unsigned long nbrel;			/* # of relations to process */
	unsigned long maxobj;			/* max # of objects to read */
	DynTable relfields;				/* relation fields */
	DynTable rellabel;				/* relation labels */
	DynTable relcolor;				/* relation colors */
	DynTable exclude;				/* exclude relation fields if set */
	DynTable relmask;				/* relation is not displayed at once if set */
	int b_reverse;					/* display reverse relation if set */
	char *notesexpr;				/* Expression evaluated to get extra text displayed under node */
	size_t notesexpr_sz;			/* length of notesexpr */
	int b_level1;					/* do not display level 0 object if set */
	int b_maxobj;					/* max # of objects is reached if set */
	DynTable treenodes;				/* table of displayed nodes path */
}
	TreeDef;

/*********************************************************************
** Function : get_node_status
** Description : read a tree node status in CGI data
*********************************************************************/
int get_node_status(				/* return : index of node in treenodes if found, treenodes->nbrows else */
	DynTable *treenodes,			/* in : control CGI data */
	char *path, size_t sz_path,		/* in : node path to read */
	int *status						/* out : node status / 0 if node not found */
){
	unsigned long i;

	if(status) *status = 0;
	if(!treenodes) return 0;

	/* Search path in CGI data */
	for(i = 0; i < treenodes->nbrows && dyntab_txt_cmp(path, sz_path, treenodes, i, 0); i++);
	
	/* If node not found : return */
	if(i >= treenodes->nbrows) return i;

	/* Read status */
	if(status) *status = dyntab_cell(treenodes, i, 0)->ctype;

	return i;
}

/*********************************************************************
** Function : set_node_status
** Description : set status of a tree node
*********************************************************************/
#define ERR_FUNCTION "set_node_status"
#define ERR_CLEANUP 
int set_node_status(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *treenodes,			/* in : control CGI data */
	char *path, size_t sz_path,		/* in : node path */
	unsigned long i,				/* in : node index if known or ~0UL for search path */
	int status						/* in : new status */
){
	DynTableCell *c;

	if(i == ~0UL) i = get_node_status(treenodes, path, sz_path, NULL);

	if(i >= treenodes->nbrows) 
	{
		if(!path || !*path || !sz_path) RETURN_ERROR("Logic error", NULL);
		i = treenodes->nbrows;
		DYNTAB_ADD(treenodes, treenodes->nbrows, 0, path, sz_path, NO_CONV);
	}
	c = dyntab_cell(treenodes, i, 0);
	c->ctype = status;
	if(!c->IdObj) c->IdObj = strtoul(c->txt, NULL, 10);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_tree_qry_children
** Description : return objects Ids related to given object
*********************************************************************/
#define ERR_FUNCTION "ctrl_tree_qry_children"
#define ERR_CLEANUP	M_FREE(sql)
int ctrl_tree_qry_children(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data  */
	DynTable *res,					/* out : list of Ids */
	char *idfld, size_t idfld_sz,	/* in : relation fields to use to build the tree (comma separated Ids) */
	int options,					/* in : options bitmask
										bit 0 : exclude fields if set, include if 0
										bit 1 : use reverse relation if set, direct if 0 */
	DynTable *id_obj				/* in : object id to search for in IdRelObj */
){
	DynBuffer *sql = NULL;
	char *obj = "IdObj", *rel = "IdRelObj", *tmp;

	/* Return if no objects given */
	if(!id_obj || !id_obj->nbrows) RETURN_OK;

	/* Swap obj / relobj if reverse relation tree */
	if(options & 2) { tmp = obj; obj = rel; rel = tmp; }

	/* Build SQL request */
	DYNBUF_ADD3(&sql, 
		"-- ctrl_tree_qry_children : read IdObj \n"
		"SELECT DISTINCTROW ", rel, 0, NO_CONV, ",IdField FROM TLink \n");
	DYNBUF_ADD3(&sql, 
		"WHERE TLink.DateDel IS NULL \n"
			"AND TLink.IdRelObj IS NOT NULL \n"
			"AND TLink.", obj, 0, NO_CONV, " IN (");
	if(qry_values_list(cntxt, id_obj, 0, &sql)) STACK_ERROR;
	DYNBUF_ADD_STR(&sql, ") \n");
	if(res->nbrows)
	{
		DYNBUF_ADD3(&sql, 
			"AND TLink.", rel, 0, NO_CONV, " NOT IN ("); 
		if(qry_values_list(cntxt, res, 0, &sql)) STACK_ERROR;
		DYNBUF_ADD_STR(&sql, ") \n"); 
	}
	if(idfld && *idfld && idfld_sz) 
	{
		DYNBUF_ADD_STR(&sql, "AND TLink.IdField");
		if(options & 1) DYNBUF_ADD_STR(&sql, " NOT");
		DYNBUF_ADD3(&sql,  " IN (", idfld, idfld_sz, NO_CONV, ") \n");
	}
	DYNBUF_ADD_STR(&sql, 
		"ORDER BY TLink.IdField, TLink.Line, TLink.Num, TLink.DateCr, TLink.Pkey");
	if(sql_exec_query(cntxt , sql->data) || sql_get_table(cntxt, res, 0)) STACK_ERROR;
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_tree_status
** Description : change status of a tree branch
*********************************************************************/
#define ERR_FUNCTION "ctrl_tree_status"
#define ERR_CLEANUP DYNTAB_FREE(list); \
					DYNTAB_FREE(id_obj); \
					M_FREE(child_path)
int ctrl_tree_status(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	TreeDef *tr,					/* in : tree control definition */
	unsigned long inode,			/* in : start node index in tr->treenodes */
	int status,						/* in : new status */
	int maxlvl						/* in : maximum tree levels to process */
){
	DynTable list = { 0 };
	DynTable id_obj = { 0 };
	DynBuffer *child_path = NULL;
	unsigned long i;

	/* Return if maximum level reached */
	if(maxlvl <= 0 || inode >= tr->treenodes.nbrows) RETURN_OK;
	if(tr->treenodes.nbrows > tr->maxobj)
	{
		tr->b_maxobj = 1;
		RETURN_OK;
	}

	/* Change node status */
	if(set_node_status(cntxt, &tr->treenodes, NULL, 0, inode, status)) STACK_ERROR;

	/* Read related objects Ids */
	DYNTAB_ADD_INT(&id_obj, 0, 0, dyntab_cell(&tr->treenodes, inode, 0)->IdObj);
	for(i = 0; i < tr->nbrel; i++)
	{
		char *msk = dyntab_val(&tr->relmask, i, 0);
		if(!*msk &&
			ctrl_tree_qry_children(cntxt, &list,
										DYNTAB_VAL_SZ(&tr->relfields, i, 1),
										(dyntab_sz(&tr->exclude, i, 0) ? 1 : 0) |
										(tr->b_reverse ? 2 : 0), &id_obj)) STACK_ERROR;
	}

	/* Process children nodes */
	for(i = 0; i < list.nbrows; i++)
	{
		unsigned long childnode;
		M_FREE(child_path);
		DYNBUF_ADD3_CELL(&child_path, "", &list, i, 0, NO_CONV, "/");
		DYNBUF_ADD_CELL(&child_path, &tr->treenodes, inode, 0, NO_CONV);
		childnode = get_node_status(&tr->treenodes, DYNBUF_VAL_SZ(child_path), NULL);
		if(childnode >= tr->treenodes.nbrows) DYNTAB_ADD_BUF(&tr->treenodes, childnode, 0, child_path);
		if(ctrl_tree_status(cntxt, tr, childnode, status, maxlvl-1)) STACK_ERROR; 
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_tree_minmax
** Description : minimize a tree diplay over a given node & develop under
*********************************************************************/
#define ERR_FUNCTION "ctrl_tree_minmax"
#define ERR_CLEANUP 
int ctrl_tree_minmax(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *treenodes,			/* in : control CGI data */
	char *path, size_t sz_path		/* in : focused node path */
){
	unsigned long i;

	/* Process given cgi values */
	for(i = treenodes->nbrows; i; i--)
	{
		DynTableCell *c = dyntab_cell(treenodes, i - 1, 0);
		char *p = c->txt;
		size_t l = c->len;
		if(!l) continue;

		/* Hide details if subnode */
		if(l > sz_path && !strncmp(p + l - sz_path, path, sz_path))
			c->ctype = 1;
		
		/* Hide children if parent */
		else if(l < sz_path && !strncmp(p, path + sz_path - l, l))
			c->ctype = 5;

		/* Hide others */
		else if(l != sz_path || strncmp(p, path, l))
		{
			dyntab_del_rows(treenodes, i - 1, 1);
			if(i < treenodes->nbrows) i++;
		}
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_html_tree
** Description : put HTML code of a tree showing relations
*********************************************************************/
#define ERR_FUNCTION "put_html_tree"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					DYNTAB_FREE(objdata); \
					DYNTAB_FREE(children); \
					DYNTAB_FREE(id_child); \
					M_FREE(subfield); \
					M_FREE(child_path); \
					M_FREE(name)
int put_html_tree(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	TreeDef *tr,					/* in : tree control definition */
	DynBuffer *path,				/* in : objects Ids path from tree root to id_obj */
	DynTable *id_obj,				/* in : current obj */
	int b_last,						/* in : 1 if last child of the level */
	int parent_status				/* in : status of the parent node */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + tr->i_ctrl;
	unsigned long i, inode;
	unsigned long idobj = DYNTAB_TOUL(id_obj);
	int status;
	DynTable data = { 0 };
	DynTable objdata = { 0 };
	DynTable children = { 0 };
	DynTable id_child = { 0 };
	DynBuffer *subfield = NULL;
	DynBuffer *name = NULL;
	DynBuffer *child_path = NULL;
	int b_hide = tr->b_level1 && !dyntab_cmp(id_obj, 0, 0, &form->id_obj, 0, 0);
	CHECK_HTML_STATUS;

	/* Build node path */
	DYNBUF_ADD_CELL(&child_path, id_obj, 0, 0, NO_CONV);	
	if(path) DYNBUF_ADD3_BUF(&child_path, "/", path, NO_CONV, "");
	
	/* Read node status */
	inode = get_node_status(&tr->treenodes, DYNBUF_VAL_SZ(child_path), &status);

	/* Return if parent status is single node and node not displayed */
	if(parent_status & 4 && inode >= tr->treenodes.nbrows) RETURN_OK;
	
	/* Output node status as CGI hidden input */
	if(status || !inode)
	{
		DYNBUF_ADD3_BUF(html, "<input type=hidden name='", ctrl->cginame, NO_CONV, "'");
		DYNBUF_ADD3_BUF(html, " value='", child_path, NO_CONV, "§");
		DYNBUF_ADD3_INT(html, "", status, "'>");
		if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(cntxt->form->html, "\n");
	}

	/* Read object data */
	if(qry_obj_data(cntxt, &objdata, id_obj, NULL)) STACK_ERROR;

	/* Read children objects Ids */
	for(i = 0; i < tr->nbrel; i++)
	{
		unsigned long row = children.nbrows, j;
		if(ctrl_tree_qry_children(cntxt, &children, 
					DYNTAB_VAL_SZ(&tr->relfields, i, 1), 
					(dyntab_sz(&tr->exclude, i, 0) ? 1 : 0) |
					(tr->b_reverse ? 2 : 0), id_obj)) STACK_ERROR;
		for(j = row; j < children.nbrows; j++) dyntab_cell(&children, j, 0)->col = i + 1;
	}

	/* Output table header */
	DYNBUF_ADD_STR(html, "<table cellspacing=0 cellpadding=0 border=0>");

	/* If not hidden first node */
	if(!b_hide)
	{
		/* Output cell header */
		DYNBUF_ADD_STR(html, "<tr><td valign=top width=20");
		if(!b_last || parent_status & 4) DYNBUF_ADD_STR(html, " background='../img/_eva_tree_bar.gif'");
		i = dyntab_cell(id_obj, 0, 0)->col;
		if(i && dyntab_sz(&tr->relcolor, i - 1, 0))
			DYNBUF_ADD3_CELL(html, " bgcolor=#", &tr->relcolor, i - 1, 0, NO_CONV, "");
		DYNBUF_ADD_STR(html, ">");

		/* Output tree structure (plus / minus buttons & continuation bars) */
		M_FREE(subfield);
		DYNBUF_ADD3_BUF(&subfield, "TREEOPEN=", child_path, NO_CONV, "§");
		CTRL_CGINAMEBTN(&name, NULL, DYNBUF_VAL_SZ(subfield));
		if(!children.nbrows)
			DYNBUF_ADD_STR(html, "<img src='../img/_eva_tree_end.gif'>")
		else if(status & 4)
		{
			if(put_html_button(cntxt, name->data, NULL,
						"_eva_tree_single.gif",
						"_eva_tree_single_s.gif",
						"Ouvrir la branche",
						0, 0))
					STACK_ERROR;
		}
		else if(status & 1)
		{
			if(put_html_button(cntxt, name->data, NULL,
						"_eva_tree_minus.gif",
						"_eva_tree_minus_s.gif",
						"Refermer la branche",
						0, 0))
					STACK_ERROR;
		}
		else
		{
			if(put_html_button(cntxt, name->data, NULL,
						"_eva_tree_plus.gif",
						"_eva_tree_plus_s.gif",
						"Ouvrir la branche",
						0, 0))
					STACK_ERROR;
		}
		M_FREE(name);
		DYNBUF_ADD_STR(html, "</td><td>\n");
		DYNBUF_ADD_STR(html, "<table cellspacing=0 cellpadding=0 border=0><tr>");

		/* Output column with vertical treebar background if open with children */
		DYNBUF_ADD_STR(html, "<td valign=top width=1%");
		if(children.nbrows && status & 1) DYNBUF_ADD_STR(html, " background='../img/_eva_tree_bar.gif'");
		DYNBUF_ADD_STR(html, ">");

		/* Output symbol for object */
		if(ctrl_add_symbol_btn(cntxt, ctrl, NULL, &objdata, 0, "*><font size=-1", "SYMBOL+LABEL+NOTES"))
				STACK_ERROR;
		M_FREE(name);
		DYNBUF_ADD_STR(html, "</font><font size=-2>");
		if(children.nbrows) DYNBUF_ADD3_INT(html, " (", children.nbrows, ")");

		/* Output 'Open all' button */
		if(children.nbrows)
		{
			M_FREE(subfield);
			DYNBUF_ADD3_BUF(&subfield, "TREEOPENALL=", child_path, NO_CONV, "§");
			CTRL_CGINAMEBTN(&name, NULL, DYNBUF_VAL_SZ(subfield));
			if(put_html_button(cntxt, name->data, NULL,
					"_eva_tree_plus2.gif", 
					"_eva_tree_plus2_s.gif",
					"Développer toutes les branches", 
					0, 4))
				STACK_ERROR;
			M_FREE(name);
		}

		/* Output extra display field if applicable */
		if(status & 1)
		{
			if(tr->notesexpr && tr->notesexpr[0]) 
			{
				data.nbrows=0;
				if(form_eval_fieldexpr(cntxt, &data, 0, idobj, tr->notesexpr, tr->notesexpr_sz, &objdata, 0)) CLEAR_ERROR;
				for(i = 0; i < data.nbrows; i++) DYNBUF_ADD3_CELL(html, "<br>", &data, i, 0, TO_HTML, "");
			}
		}

		DYNBUF_ADD_STR(html, "</font></td></tr>");
	}

	/* Recursively output related objects */
	if(status & 1) for(i = 0; i < children.nbrows; i++)
	{
		DYNTAB_SET_CELL(&id_child, 0, 0, &children, i, 0);
		DYNBUF_ADD_STR(html, "<tr><td colspan=3>");
		if(put_html_tree(cntxt, tr, child_path, &id_child, i == children.nbrows - 1, status)) 
			STACK_ERROR;
		DYNBUF_ADD_STR(html, "</td>\n</tr>");
	}

	/* Put table footer */
	if(!b_hide) DYNBUF_ADD_STR(html, "</table></td>\n</tr>");
	DYNBUF_ADD_STR(html, "</table>");
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_reltree
** Description : handle controls OUTPUT/RELTREE
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_reltree"
#define ERR_CLEANUP DYNTAB_FREE(tr->relfields); \
					DYNTAB_FREE(tr->rellabel); \
					DYNTAB_FREE(tr->exclude); \
					DYNTAB_FREE(tr->relcolor); \
					DYNTAB_FREE(tr->relmask); \
					DYNTAB_FREE(tr->treenodes); \
					DYNTAB_FREE(idobj); \
					DYNTAB_FREE(id_child); \
					DYNTAB_FREE(srcrec)
int ctrl_add_reltree(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	TreeDef _tr = {0}, *tr = &_tr;
	DynTable cgival = { 0 };
	DynTable srcrec = { 0 };
	DynTable idobj = { 0 };
	DynTable id_child = { 0 };
  	DynBuffer *buf = NULL;
	DynTableCell *c;
	char *txt;
	unsigned long i, node;

	switch(form->step)
	{
	case CtrlRead:
		if(cgi_build_basename(cntxt, &ctrl->cginame, i_ctrl, 'D')) STACK_ERROR;
		break;

	case HtmlView:
	case HtmlPrint:
	case HtmlEdit:
		if(!form->html) break;

		/* Read tree base objects */
		txt = CTRL_ATTR_VAL(STARTOBJ);
		if(!strcmp(txt, "_EVA_LEVEL1"))
		{
			tr->b_level1 = 1;
			DYNTAB_SET_CELL(&idobj, 0, 0, &form->id_obj, 0, 0);
		}
		else if(*txt)
			CTRL_ATTR(idobj, LISTOBJ)
		else
			DYNTAB_SET_CELL(&idobj, 0, 0, &form->id_obj, 0, 0);
		if(!idobj.nbrows) break;

		/* Read control attributes */
		tr->i_ctrl = i_ctrl;
		CTRL_ATTR_TAB(tr->rellabel, REL_LABEL);
		CTRL_ATTR_TAB(tr->relfields, RELFIELDS);
		CTRL_ATTR_TAB(tr->exclude, EXCLUDE);
		CTRL_ATTR_TAB(tr->relcolor, REL_COLOR);
		CTRL_ATTR_TAB(tr->relmask, MASK_INIT);
		c = CTRL_ATTR_CELL(MAX_OBJ);
		tr->maxobj = c ? strtoul(c->txt, NULL, 10) : 1000;
		c = CTRL_ATTR_CELL(NOTES_EXPR);
		if(c) { tr->notesexpr = c->txt; tr->notesexpr_sz = c->len; }
		tr->nbrel = tr->relfields.nbrows;
		if(tr->nbrel < tr->exclude.nbrows) tr->nbrel = tr->exclude.nbrows;
		if(tr->nbrel < tr->rellabel.nbrows) tr->nbrel = tr->rellabel.nbrows;
		tr->b_reverse = CTRL_ATTR_CELL(REL_REVERSE) != 0;

		/* Build list of relations Ids */
		for(i = 0; i < tr->nbrel; i++) if(dyntab_sz(&tr->relfields, i, 0))
		{
			M_FREE(buf);
			DYNTAB_FREE(cgival);
			if(dyntab_from_list(&cgival, DYNTAB_VAL_SZ(&tr->relfields, i, 0), ",",  0, 2)) RETURN_ERR_MEMORY;
			if(qry_values_list(cntxt, &cgival, 3, &buf)) STACK_ERROR;
			DYNTAB_ADD_BUF(&tr->relfields, i, 1, buf);
		}

		/* Read nodes status in CGI data */
		if(cgi_get_values(cntxt, &tr->treenodes, ctrl->cginame->data, 4)) STACK_ERROR;
		tr->b_maxobj = tr->treenodes.nbrows > tr->maxobj;
		CGI_VALUES_DONTKEEP(&tr->treenodes);
		for(i = 0; i < tr->treenodes.nbrows; i++)
		{
			DynTableCell *p = dyntab_cell(&tr->treenodes, i, 0);
			char *s = strchr(p->txt, '§');
			if(!s) continue;
			*s = 0;
			p->len = s - p->txt;
			p->ctype = strtoul(s + 1, NULL, 10);
		}

		/* Handle tree buttons click */
		if(form->i_ctrl_clic == i_ctrl)
		{
			char *btn = CGI_CLICK_BTN_SUBFIELD;
			char *path = btn;
			size_t sz_path = 0;
			int status;

			/* Delimit path data in button name */
			while(*path && *path != '=') path++; 
			if(*path) path++;
			while(*path && path[sz_path] != '§') sz_path++;

			/* Handle status changes depending on clicked button */
			node = get_node_status(&tr->treenodes, path, sz_path, &status);
			if(!strncmp(btn, add_sz_str("TREEOPEN="))) 
			{
				/* Tree open / close */
				if(set_node_status(cntxt, &tr->treenodes, path, sz_path, node, ((status & 4) || !status) ? 1 : 0)) STACK_ERROR;
			}
			else if(!strncmp(btn, add_sz_str("TREEOPENALL=")))
			{
				/* Open all nodes under clicked node */
				if(ctrl_tree_minmax(cntxt, &tr->treenodes, path, sz_path)) STACK_ERROR;
				if(set_node_status(cntxt, &tr->treenodes, path, sz_path, ~0UL, 1)) STACK_ERROR;
				node = get_node_status(&tr->treenodes, path, sz_path, NULL);
				if(ctrl_tree_status(cntxt, tr, node, 1, 4)) STACK_ERROR;
			}
			else if(!strncmp(btn, add_sz_str("MOVETO=")) || !strncmp(btn, add_sz_str("ADDTO=")))
			{
				/* Read destination object */
				int iddest = atoi(path);
				if(!iddest) break;

				/* Build confirmation page */
				/* Display confirmation dialog */
				/* Move selected nodes */
				for(i = 0; i < cgival.nbrows; i++)
				{
					char sql[256];

					/* Read moved object id */
					int idrelobj = atoi(dyntab_val(&cgival, i, 0));
					int idobj = atoi(dyntab_val(&cgival, i, 0)+ 2 + (int)log10((double)idrelobj));
					if(!idrelobj) continue;

					/* Build SQL request for source relation record */
					sprintf(sql, "SELECT Pkey, IdField, Num FROM TLink WHERE DateDel IS NULL AND IdObj=%d AND IdRelObj=%d", idobj, idrelobj);
					if(sql_exec_query(cntxt, sql) || sql_get_table(cntxt, &srcrec, 2)) STACK_ERROR;

					/* Process move if found */
					if(srcrec.nbrows)
					{
						/* Create new record for relation */
						sprintf(sql, "INSERT TLink SET IdObj=%d, IdRelObj=%d, IdField=%s, Num=%s DateCr=%s, IdWhoCr=%s", 
									iddest, idrelobj, dyntab_val(&srcrec, 0, 1), dyntab_val(&srcrec, 0, 2),
									cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0));
						if(sql_exec_query(cntxt, sql)) STACK_ERROR;

						/* If move mode */
						if(!strncmp(btn, add_sz_str("MOVETO=")))
						{
							/* Remove node from move data (clear in CGI data) */
							DynTableCell *val = dyntab_cell(&tr->treenodes, i, 0);
							if(val && val->i_cgi) 
							{
								cntxt->cgi[val->i_cgi].name[0] = 0; 
								cntxt->cgi[val->i_cgi].name_sz = 0; 
							}

							/* Remove old record */
							sprintf(sql, "UPDATE TLink SET DateDel='%s',IdWhoDel=%s WHERE Pkey=%s", 
										cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), dyntab_val(&srcrec, 0, 0));
							if(sql_exec_query(cntxt, sql)) STACK_ERROR;
						}
					}

				}

			}
		}
		/* Handle first display */
		else if(!tr->treenodes.nbrows)
		{
			/* Open nodes under selected node */
			int nb = atoi(CTRL_ATTR_VAL(SHOW_LEVELS));
			if(dyntab_from_tab(&tr->treenodes, 0, 0, &idobj, 0, 0, idobj.nbrows, 1, 1)) RETURN_ERR_MEMORY;
			for(i = 0; i < idobj.nbrows; i++)
				if(ctrl_tree_status(cntxt, tr, i, 1, nb > 0 ? nb : 4)) STACK_ERROR;
		}

		/* Output control header */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		if(tr->b_maxobj)
		{
			DYNBUF_ADD3_INT(form->html, "<p><b>*** Affichage incomplet (max=", tr->maxobj, ")</b></p>");
		}

		/* Output tree control */
		for(i = 0; i < idobj.nbrows; i++)
		{
			DYNTAB_SET_CELL(&id_child, 0, 0, &idobj, i, 0);
 			if(put_html_tree(cntxt, tr, NULL, &id_child, i == idobj.nbrows - 1, 1)) STACK_ERROR;
		}

		/* Output control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
