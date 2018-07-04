/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_output_catalog.c
** Description : handling fonctions for catalog list controls
**      Author : Alain BOUTE
**     Created : Sept 24 2005
*********************************************************************/

#include "eva.h"

typedef struct _Key
{
	unsigned long idfld;		/* field value Id */
	unsigned long idsel;		/* selected value for the key */
	int rel;					/* set if selected value is a relation */
	unsigned long isel;			/* index of selected value in val */
	DynTable val;				/* list of key values for this field */
#define col_IdVal	0
#define col_IdRel	1
#define col_NbObj	2
#define col_IdObj	3
#define col_Label	4
}
	Key;
typedef struct _Catalog 
{
	unsigned long i_ctrl;		/* index of catalog control in form->ctrl */
	DynTable key;				/* input fields used for catalog key index */
	DynTable img;				/* image group field for catalog lists */
	DynTable imgdir;			/* images base directory */
	char dw1[16];				/* desired with for thumbnails (0 for no resize) */
	char dh1[16];				/* desired height for thumbnails (0 for no resize) */
	char dw2[16];				/* desired with for small thumbnails (0 for no resize) */
	char dh2[16];				/* desired height for small thumbnails (0 for no resize) */
	DynTable categ;				/* clicked image values for catalog key index */
	unsigned long idimg;		/* image field value id for catalog lists  */
	unsigned long idimg_r;		/* 1 if image field is a relation, 0 else */
	DynBuffer *id;				/* list of selected objects Id */
	unsigned long nbid;			/* # of selected objects Id */
	Key *k;						/* detailed infos for each index key */
	unsigned long lvl;			/* level (key index) of 1st field with no selected value */
	unsigned long idsel;		/* selected group value  */
	int rel;					/* set if selected group value is a relation */
	char path[4096];			/* images base path */
	unsigned long idName;		/* IdValue for _EVA_FILE_SERVER_NAME */
}
	Catalog;


/*********************************************************************
** Function : output_catalog_sublist
** Description : output selected items of a catalog
*********************************************************************/
#define ERR_FUNCTION "output_catalog_sublist"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					DYNTAB_FREE(obj); \
					M_FREE(sql); \
					M_FREE(lbl); \
					M_FREE(name)
int output_catalog_sublist(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat,						/* in : catalog context data */
	char *idobj, size_t szobj,			/* in : objects to list */
	unsigned long strip,				/* in : output strip limit (pixels wide) */
	unsigned long lvl,					/* in : list level */
	unsigned long isel					/* in : selected value index in cat->k[lvl].val */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + cat->i_ctrl;
	DynTable data = { 0 };
	DynTable obj = { 0 };
	DynBuffer *sql = NULL;
	DynBuffer *lbl = NULL;
	DynBuffer *name = NULL;
	unsigned long i, j = 0, wsum = 0;
	char *imgdir = dyntab_val(&cat->imgdir, 0, 0);
	int b_strip = 0, b_dyn_size = !(*cat->dh1 || *cat->dw1) && *imgdir;
	CHECK_HTML_STATUS;

	/* Handle image field type */
	if(cat->idimg_r)
	{
		/* Read file Ids for selected objects */
		DYNBUF_ADD_STR(&sql, 
			"-- output_catalog_sublist : Read file Ids for selected objects\n"
			"SELECT DISTINCT IdRelObj AS Val,IdObj,IdRelObj AS IdValue FROM TLink\n");
	}
	else
	{
		/* Read value images for selected objects */
		DYNBUF_ADD_STR(&sql, 
			"-- output_catalog_sublist : Read value images for selected objects\n"
			"SELECT DISTINCT TxtValue AS Val,IdObj,TLink.IdValue AS IdValue FROM TLink\n"
			"INNER JOIN TVal ON TVal.IdValue=TLink.IdValue\n");
	}
	DYNBUF_ADD3(&sql, 
		"WHERE IdObj IN (", idobj, szobj, NO_CONV, ") AND DateDel IS NULL AND Num=1 AND IdField=");
	DYNBUF_ADD_INT(&sql, cat->idimg);
	DYNBUF_ADD_STR(&sql, " ORDER BY Val");
	if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &data, 6)) STACK_ERROR;

	/* Output images for objects groups */
	M_FREE(sql);
	for(i = 0; i < data.nbrows; i++)
	{
		/* Read object label */
		DynTableCell *c = dyntab_cell(&data, i, 0);
		if(qry_obj_idfield(cntxt, &obj, c->IdObj, 0) ||
			qry_obj_label(cntxt, NULL, NULL, &lbl, NULL, NULL, NULL, NULL, NULL, 0, &obj, 0)) STACK_ERROR;
		if(sql) DYNBUF_ADD_STR(&sql, "\n");
		DYNBUF_ADD_BUF(&sql, lbl, NO_CONV);
		j++;

		/* If next image is different */
		if(dyntab_cmp(&data, i, 0, &data, i + 1, 0))
		{
			unsigned long w = 0, h;

			/* Strip if index list */
			if(strip && wsum > strip) 
			{
				b_strip = 1;
				break;
			}

			/* Build button name for list display if more than one object for image */
			M_FREE(name);
			if(j > 1)
			{
				DYNBUF_ADD_BUF(&name, ctrl->cginame, NO_CONV);
				name->data[0] = 'B';
				DYNBUF_ADD3_INT(&name, ".IMG$", c->IdValue, "");
				if(isel != ~0UL)
				{
					unsigned long idval = DYNTAB_TOULRC(&cat->k[lvl].val, isel, col_IdVal);
					unsigned long idrel = DYNTAB_TOULRC(&cat->k[lvl].val, isel, col_IdRel);
					DYNBUF_ADD3_INT(&name, ".", idval ? idval : idrel, ".");
					DYNBUF_ADD_INT(&name, idrel ? 1 : 0);
				}
			}

			/* Output half size image */
			if(b_dyn_size)
			{
				/* Traitement cas sans dimensions (pour bricometal) */
				char path[4096], dh[16], dw[16];
				snprintf(add_sz_str(path), "../docs/%s/%s/%s", cntxt->dbname, imgdir, c->txt);
				get_image_size(path, &w, &h);
				w /= 2; h /= 2;
				snprintf(add_sz_str(dw), "%lu", w);
				snprintf(add_sz_str(dh), "%lu", h);
				if(put_html_image_dir(cntxt, name ? name->data : NULL, imgdir, c->txt, sql ? sql->data : NULL,
												dw, dh, 0, c->IdObj, NULL, NULL)) STACK_ERROR;
			}
			else
			{
				if(put_html_image_dir(cntxt, name ? name->data : NULL, imgdir, c->txt, sql ? sql->data : NULL,
												cat->dw2, cat->dh2, 0, c->IdObj, strip ? &w : NULL, NULL)) STACK_ERROR;
			}

			if(strip) wsum += w + 5;
			DYNBUF_ADD_STR(html, " ");

			/* Skip images in order to get a sample of available images (when numerous) */
			if(strip)
			{
				/* Calc position equivalent to occupied space */
				unsigned long pos = (unsigned long) (1.0 * data.nbrows / (strip + 10) * wsum);
				if(pos > i && dyntab_cmp(&data, i + 1, 0, &data, pos, 0))
				{
					if(pos >= data.nbrows) pos = data.nbrows - 1;
					while(!dyntab_cmp(&data, pos - 1, 0, &data, pos, 0)) pos--;
					i = pos - 1;
					b_strip = 1;
				}
			}
			M_FREE(sql); j = 0;
		}
	}
	if(b_strip) DYNBUF_ADD_STR(html, " <font size=4><b>...");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : read_lower_idobj
** Description : get objects for the given value
*********************************************************************/
#define ERR_FUNCTION "read_lower_idobj"
#define ERR_CLEANUP	M_FREE(sql)
int read_lower_idobj(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat,						/* in : catalog context data */
	DynBuffer **idobj,					/* out : matched objects Ids */
	unsigned long *nbobj,				/* out : # of matched objects */
	unsigned long lvl,					/* in : depth in tree - index in cat->k */
	unsigned long i						/* in : value index in cat->k[lvl].val */
){
	DynBuffer *sql = NULL;

	/* Get value / relation Id */
	unsigned long idval = DYNTAB_TOULRC(&cat->k[lvl].val, i, col_IdVal);
	unsigned long idrel = DYNTAB_TOULRC(&cat->k[lvl].val, i, col_IdRel);

	if(idval || idrel)
	{
		if(cat->id)
		{
			DYNBUF_ADD3_INT(&sql,
				"-- read_lower_idobj\n"
				"SELECT DISTINCT IdObj FROM TLink\n"
				"WHERE DateDel IS NULL AND IdField=", cat->k[lvl].idfld, " AND ");
			if(idrel) DYNBUF_ADD_STR(&sql, "IdRelObj")
			else DYNBUF_ADD_STR(&sql, "IdValue");
			DYNBUF_ADD3_INT(&sql, "=", idrel ? idrel : idval, "\n");
			DYNBUF_ADD3_BUF(&sql,
				"AND IdObj IN (", cat->id, NO_CONV, ")\n");
			if(sql_exec_query(cntxt, sql->data) || sql_get_buf(cntxt, idobj, 2)) STACK_ERROR;
			if(nbobj) *nbobj = cntxt->sql_nbrows;
		}
		else 
		{
			if(idobj) M_FREE(*idobj);
			if(nbobj) *nbobj = 0;
		}
	}
	else if(idobj) 
	{
		/* Handle empty value */
		M_FREE(*idobj);
		DYNBUF_ADD_CELL(idobj, &cat->k[lvl].val, i, col_IdObj, NO_CONV);
		if(nbobj) *nbobj = DYNTAB_TOULRC(&cat->k[lvl].val, i, col_NbObj);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_value_button
** Description : output selection button for a catalog group value
*********************************************************************/
#define ERR_FUNCTION "output_value_button"
#define ERR_CLEANUP	M_FREE(name); \
					M_FREE(lbl)
int output_value_button(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat,						/* in : catalog context data */
	unsigned long lvl,					/* in : depth in tree - index in cat->k */
	unsigned long i,					/* in : value index in cat->k[lvl].val */
	char *img,							/* in : button image */
	char *imgsel						/* in : button selected image */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + cat->i_ctrl;
	DynBuffer *name = NULL;
	DynBuffer *lbl = NULL;
	unsigned long idval = DYNTAB_TOULRC(&cat->k[lvl].val, i, col_IdVal);
	unsigned long idrel = DYNTAB_TOULRC(&cat->k[lvl].val, i, col_IdRel);

	/* Do not output if empty end of tree */
	if(i == ~0UL || cat->k[lvl].val.nbrows == 1 && !idval && !idrel) RETURN_OK;

	/* Build button name */
	DYNBUF_ADD_BUF(&name, ctrl->cginame, NO_CONV);
	name->data[0] = 'B';
	DYNBUF_ADD3_INT(&name, ".VAL$", lvl, ".");
	DYNBUF_ADD_INT(&name, idval ? idval : idrel);
	DYNBUF_ADD3_INT(&name, ".", (!idval && idrel) ? 1 : 0, "");

	/* Build button label */
	DYNBUF_ADD_CELL(&lbl, &cat->k[lvl].val, i, col_Label, NO_CONV);
	if(put_html_button(cntxt, name->data, lbl ? lbl->data : "???",
		img, imgsel, NULL, 0, 12 | (imgsel ? 0 : 1))) STACK_ERROR;				  

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_catalog_title
** Description : output selection tree for a catalog
*********************************************************************/
#define ERR_FUNCTION "output_catalog_title"
#define ERR_CLEANUP	M_FREE(name)
int output_catalog_title(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat						/* in : catalog context data */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cat->i_ctrl;
	DynBuffer *name = NULL;
	CHECK_HTML_STATUS;
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("CLR"));
	DYNBUF_ADD_STR(html, "<font size=-1>");
	if(put_html_button_sz(cntxt, name->data, ctrl->LABEL, "_eva_catalog_root.gif", "_eva_catalog_root_s.gif", NULL, NULL, 0, 0, 0, 12))
		STACK_ERROR;
	DYNBUF_ADD_STR(html, "</font>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_catalog_tree
** Description : output selection tree for a catalog
*********************************************************************/
#define ERR_FUNCTION "output_catalog_tree"
#define ERR_CLEANUP
int output_catalog_tree(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat,						/* in : catalog context data */
	unsigned long lvl					/* in : depth in tree - index in cat->k */
){
	unsigned long i;
	CHECK_HTML_STATUS;
	if(lvl >= cat->key.nbrows) RETURN_OK;

	/* Output title if level 0 */
	if(!lvl)
	{
		if(output_catalog_title(cntxt, cat)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "<br>");
	}

	/* Output field values */
	for(i = 0; i < cat->k[lvl].val.nbrows; i++)
	{
		int b_sel = lvl < cat->lvl && cat->k[lvl].isel == i;
		if(lvl) DYNBUF_ADD3_INT(html, "<img src='/img/_eva_nop.gif' width=", lvl * 20, " height=1 align=absmiddle>");
		if(b_sel) DYNBUF_ADD_STR(html, "<b>");
		if(output_value_button(cntxt, cat, lvl,	i,
			b_sel ? "_eva_item_small_s.gif" : "_eva_item_small.gif",
			b_sel ? "_eva_item_small.gif" : "_eva_item_small_s.gif")) STACK_ERROR;				  
		if(b_sel) DYNBUF_ADD_STR(html, "</b>");
		DYNBUF_ADD_STR(html, "<br>");

		/* If selected value : output lower level */
		if(b_sel && output_catalog_tree(cntxt, cat, lvl + 1)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_catalog_index
** Description : output selected items of a catalog
*********************************************************************/
#define ERR_FUNCTION "output_catalog_index"
#define ERR_CLEANUP	M_FREE(idobj);\
					M_FREE(fname);\
					M_FREE(sql);\
					if(f) fclose(f);
int output_catalog_index(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat						/* in : catalog context data */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cat->i_ctrl;
	DynBuffer *idobj = NULL;
	DynBuffer *fname = NULL;
	DynBuffer *sql = NULL;
	unsigned long i, j, strip = 600;
	unsigned long lvl = cat->lvl;
	int b_cache;
	struct stat fs;
	FILE *f = NULL;
	CHECK_HTML_STATUS;

	/* Build cache file name */
	if(!cat->id) RETURN_OK;
	DYNBUF_ADD_CELL(&fname, &cntxt->form->id_form, 0, 0, NO_CONV);
	DYNBUF_ADD3_CELL(&fname, ".", &cntxt->form->id_obj, 0, 0, NO_CONV, ".");
	DYNBUF_ADD_CELL(&fname, &cntxt->form->ctrl[cat->i_ctrl].id, 0, 0, NO_CONV);
	for(i = 0; i < lvl; i++)
	{
		DYNBUF_ADD3_INT(&fname, "-", cat->k[i].idfld, ".");
		DYNBUF_ADD_INT(&fname, cat->k[i].idsel);
	}
	DYNBUF_ADD_STR(&fname, ".htm");

	/* Check if cache file exists */
	if(chdir_db_doc(cntxt)) STACK_ERROR;
	mkdir("cache"); chdir("cache");
	b_cache = !stat(fname->data, &fs);

	/* Check if page data was modified after cache file creation */
	if(b_cache)
	{
		char datecr[32];
		DYNBUF_ADD3_BUF(&sql, "SELECT MAX(DateCr) FROM TLink WHERE IdObj IN (", cat->id, NO_CONV, ")");
		if(sql_exec_query(cntxt, sql->data) || sql_get_buf(cntxt, &sql, 2)) STACK_ERROR;
		b_cache = !time_to_datetxt(fs.st_mtime, datecr) && strcmp(sql->data, datecr) < 0;
	}

	/* If cache valid : read from file */
	if(b_cache)
	{
		char *cdata = malloc(fs.st_size + 4);
		if(!cdata) RETURN_ERR_MEMORY;
		f = fopen(fname->data, "rb");
		if(!f || fread(cdata, fs.st_size, 1, f) != 1) RETURN_ERROR("Cache read error", NULL);
		DYNBUF_ADD(html, cdata, fs.st_size, NO_CONV);
	}
	else
	/* Cache not valid : output page & save to file */
	{
		size_t pos = (html && *html) ? (*html)->cnt : 0;

		/* Output catalog list for each lower level value */
		chdir(cntxt->path);
		for(i = 0; i < cat->k[lvl].val.nbrows; i++)
		{
			/* Output value button */
			DYNBUF_ADD_STR(html, "<table width=100% style=\"border-bottom:3px double gray\"><tr><td><font size=-1>");
			if(lvl)
			{
				if(CTRL_ATTR_CELL(NO_TREE) && output_catalog_title(cntxt, cat)) STACK_ERROR;
				for(j = 0; j < lvl; j++)
					if(output_value_button(cntxt, cat, j, cat->k[j].isel, "catalog_dot.gif", "catalog_dot_s.gif")) STACK_ERROR;				  
				DYNBUF_ADD_STR(html, " ");
			}
			DYNBUF_ADD_STR(html, "</font> <b>");
			if(output_value_button(cntxt, cat, lvl, i, "catalog_dot.gif", "catalog_dot_s.gif")) STACK_ERROR;				  
			DYNBUF_ADD_STR(html, "</b></td></tr></table>");

			/* Read objects for the value */
			if(read_lower_idobj(cntxt, cat, &idobj, NULL, lvl, i)) STACK_ERROR;

			/* Output list of images */
			if(idobj && output_catalog_sublist(cntxt, cat, idobj->data, idobj->cnt, strip, lvl, i)) STACK_ERROR;
		}

		/* Save page to cache file */
		chdir_db_doc(cntxt); chdir("cache");
		f = fopen(fname->data, "wbc");
		if(!f || fwrite((*html)->data + pos, (*html)->cnt - pos, 1, f) != 1) RETURN_ERROR("Cache write error", NULL);
	}
	chdir(cntxt->path);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_catalog_list
** Description : output selected items of a catalog
*********************************************************************/
#define ERR_FUNCTION "output_catalog_list"
#define ERR_CLEANUP
int output_catalog_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat						/* in : catalog context data */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cat->i_ctrl;
	unsigned long j, lvl = cat->key.nbrows - 1;
	CHECK_HTML_STATUS;

	/* Output each selected value */
	DYNBUF_ADD_STR(html, "<table width=100% style=\"border-bottom:3px double gray\"><tr><td><font size=-1>");
	if(CTRL_ATTR_CELL(NO_TREE) && output_catalog_title(cntxt, cat)) STACK_ERROR;
	for(j = 0; j < lvl; j++)
		if(output_value_button(cntxt, cat, j, cat->k[j].isel, "catalog_dot.gif", "catalog_dot_s.gif")) STACK_ERROR;				  
	DYNBUF_ADD_STR(html, "</font> <b>");
	if(output_value_button(cntxt, cat, lvl, cat->k[j].isel, "catalog_dot.gif", NULL)) STACK_ERROR;				  
	DYNBUF_ADD_STR(html, "</td></tr></table>");

	/* Output list of images */
	if(output_catalog_sublist(cntxt, cat, cat->id->data, cat->id->cnt, 0, 0, ~0UL)) STACK_ERROR;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_catalog_group
** Description : output items for the selected image
*********************************************************************/
#define ERR_FUNCTION "output_catalog_group"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					DYNTAB_FREE(obj); \
					M_FREE(sql); \
					M_FREE(lbl)
int output_catalog_group(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat						/* in : catalog context data */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cat->i_ctrl;
	DynTable data = { 0 };
	DynTable obj = { 0 };
	DynBuffer *sql = NULL;
	DynBuffer *lbl = NULL;
	unsigned long i = 0;
	char opname[64];
	CHECK_HTML_STATUS;

	/* Output each selected value */
	DYNBUF_ADD_STR(html, "<table width=100% style=\"border-bottom:3px double gray\"><tr><td><font size=-1>");
	if(CTRL_ATTR_CELL(NO_TREE) && output_catalog_title(cntxt, cat)) STACK_ERROR;
	for(i = 0; i < cat->key.nbrows - 1 && cat->k[i + 1].val.nbrows && cat->k[i + 1].isel != ~0UL; i++)
		if(output_value_button(cntxt, cat, i, cat->k[i].isel, "catalog_dot.gif", "catalog_dot_s.gif")) STACK_ERROR;				  
	DYNBUF_ADD_STR(html, "</font> <b>");
	if(cat->k[i].val.nbrows &&
		output_value_button(cntxt, cat, i, cat->k[i].isel, "catalog_dot.gif", "catalog_dot_s.gif")) STACK_ERROR;				  
	DYNBUF_ADD_STR(html, "</td></tr></table>");

	/* Read image file name */
	if(cat->idimg_r)
	{
		DYNTAB_ADD_INT(&data, 0, 0, cat->idsel);
	}
	else
	{
		DYNBUF_ADD3_INT(&sql, "SELECT TxtValue FROM TVal WHERE IdValue=", cat->idsel, "");
		if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &data, 0)) STACK_ERROR;
	}

	/* Output image */
	DYNBUF_ADD_STR(html, "<table width=100%><tr><td>");
	if(put_html_image_dir(cntxt, NULL, dyntab_val(&cat->imgdir, 0, 0), dyntab_val(&data, 0, 0), NULL,
								cat->dw1, cat->dh1, 0, 0, NULL, NULL)) STACK_ERROR;				  

	/* Read objects with selected image */
	M_FREE(sql);
	if(ctrl->i_child) DYNBUF_ADD_STR(&sql, "CREATE TEMPORARY TABLE IdList\n");
	DYNBUF_ADD3_BUF(&sql, 
		"SELECT DISTINCT IdObj FROM TLink\n"
		"WHERE IdObj IN (", cat->id, NO_CONV, ")\n");
	DYNBUF_ADD3_INT(&sql,
		"AND DateDel IS NULL AND IdField=", cat->idimg, " AND ");
	if(cat->idimg_r) DYNBUF_ADD_STR(&sql, "IdRelObj=") else DYNBUF_ADD_STR(&sql, "IdValue=");
	DYNBUF_ADD_INT(&sql, cat->idsel);
	if(sql_exec_query(cntxt, sql->data) || !ctrl->i_child && sql_get_table(cntxt, &data, 0)) STACK_ERROR;

	/* Read & output links */
	M_FREE(sql);
	DYNBUF_ADD_STR(&sql, 
		"SELECT DISTINCT IdRelObj FROM TLink");
	if(ctrl->i_child)
		DYNBUF_ADD_STR(&sql, ",IdList\nWHERE TLink.IdObj=IdList.IdObj\n")
	else
		DYNBUF_ADD3_BUF(&sql, "\nWHERE IdObj IN (", cat->id, NO_CONV, ")\n");
	DYNBUF_ADD_STR(&sql,
		"AND DateDel IS NULL AND IdField IN(");
	for(i = 0; i < cat->key.nbrows; i++)
	{
		if(i) DYNBUF_ADD_STR(&sql, ",");
		DYNBUF_ADD_INT(&sql, cat->k[i].idfld);
	}
	DYNBUF_ADD_STR(&sql, ")");
	if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &obj, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td><td align=right valign=top><div align=left><font size=-1>");
	for(i = 0; i < obj.nbrows; i++)
	{
		unsigned long idobj = DYNTAB_TOULRC(&obj, i, 0);
		build_open_btn_name(cntxt, opname, 0, idobj, 0, 0);
		DYNBUF_ADD_STR(html, "<br>");
		if(qry_obj_label(cntxt, NULL, NULL, &lbl, NULL, NULL, NULL, NULL, NULL, 0, NULL, idobj) ||
			lbl && put_html_button(cntxt, opname, lbl->data, "item_small.gif", "item_small_s.gif", NULL, 0, 12))
			STACK_ERROR;				  
	}
	DYNBUF_ADD_STR(html, "</div></td></tr></table>");

	/* If display list given */
	if(ctrl->i_child)
	{
		/* Output list */
		EVA_ctrl *list = cntxt->form->ctrl + ctrl->i_child;
		list->POSITION = "_EVA_SameCell";
		list->LABELPOS = "_EVA_NONE";
		list->objtbl->from_idlist = 1;
		CTRL_PRI_HDLR(ctrl->i_child);
	}
	else
		/* Output object labels */
		for(i = 0; i < data.nbrows; i++)
		{
			/* Output object label as open button */
			unsigned long idobj = DYNTAB_TOULRC(&data, i, 0);
			build_open_btn_name(cntxt, opname, 0, idobj, 0, 0);
			if(qry_obj_label(cntxt, NULL, NULL, &lbl, NULL, NULL, NULL, NULL, NULL, 0, NULL, idobj) ||
				lbl && put_html_button(cntxt, opname, lbl->data, "_eva_item.gif", "_eva_item1.gif", NULL, 0, 12))
				STACK_ERROR;				  
			DYNBUF_ADD_STR(html, "<br>");
		}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : listobj_not_in
** Description : return items of first list not in second list
*********************************************************************/
int listobj_not_in(			/* return : 0 on success, other on error */
	DynBuffer **res,		/* out : list of items in lst1 not in lst2 */
	unsigned long *nbres,	/* out : # of items in res */
	DynBuffer *lst1,		/* in : list of items */
	DynBuffer *lst2			/* in : list of items */
){
	DynTable t1 = {0};
	DynTable t2 = {0};
	unsigned long i1 = 0, i2 = 0;
	M_FREE(*res);
	if(lst1 && dyntab_from_list(&t1, lst1->data, lst1->cnt, ",", 0, 0)) return 1;
	if(lst2 && dyntab_from_list(&t2, lst2->data, lst2->cnt, ",", 0, 0)) return 1;
	dyntab_sort(&t1, qsort_col0);
	dyntab_sort(&t2, qsort_col0);
	while(i1 < t1.nbrows)
	{
		int cmp = dyntab_cmp(&t1, i1, 0, &t2, i2, 0);
		if(cmp)
		{
			DynTableCell *c = dyntab_cell(&t1, i1, 0);
			if(*res && dynbuf_add(res, add_sz_str(","), NO_CONV)) return 1;
			if(dynbuf_add(res, c->txt, c->len, NO_CONV)) return 1;
			if(nbres) (*nbres)++;
		}
		i1++;
		if(cmp >= 0 && i2 < t2.nbrows) i2++;
	}
	return 0;
}

/*********************************************************************
** Fonction : qsort_col4
** Description : qsort callback for DynTable - column 0
*********************************************************************/
int qsort_col4(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1 + 4)->txt;
	char *t2 = ((DynTableCell *)el2 + 4)->txt;
	return STRCMPNUL(t1, t2);
}

/*********************************************************************
** Function : prepare_catalog_values
** Description : set appropriate values for Catalog structure
*********************************************************************/
#define ERR_FUNCTION "prepare_catalog_values"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					M_FREE(sql); \
					M_FREE(lbl)
int prepare_catalog_values(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	Catalog *cat						/* in : catalog context data */
){
	DynTable data = { 0 };
	DynBuffer *sql = NULL;
	DynBuffer *lbl = NULL;
	unsigned long i, j;
	unsigned long nbobj = 0;

	/* Get key fields */
	for(i = 0; i <= cat->lvl && cat->id && cat->id->cnt; i++)
	{
		/* Read field name */
		if(qry_obj_idfield(cntxt, &data, DYNTAB_TOULRC(&cat->key, i, 0), cntxt->val_FIELD)) STACK_ERROR;
		if(!data.nbrows) break;
		cat->k[i].idfld = dyntab_cell(&data, 0, 0)->IdValue;

		/* Read field values */
		M_FREE(sql);
		DYNBUF_ADD3_BUF(&sql,
			"SELECT IdValue,IdRelObj, COUNT(IdObj) FROM TLink\n"
			"WHERE IdObj IN (", cat->id, NO_CONV, ")\n");
		DYNBUF_ADD3_INT(&sql,
			"AND DateDel IS NULL AND IdField=", cat->k[i].idfld, "\n"
			"GROUP BY IdValue,IdRelObj\n");
		if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &cat->k[i].val, 0)) STACK_ERROR;

		/* Build values labels */
		for(j = 0; j < cat->k[i].val.nbrows; j++)
		{
			unsigned long idval = DYNTAB_TOULRC(&cat->k[i].val, j, col_IdVal);
			unsigned long idrel = DYNTAB_TOULRC(&cat->k[i].val, j, col_IdRel);

			/* Get displayed value */
			if(idval)
			{
				/* Simple value : retrieve value text */
				char qry[256];
				snprintf(add_sz_str(qry), "SELECT TxtValue FROM TVal WHERE IdValue=%lu", idval);
				if(sql_exec_query(cntxt, qry) || sql_get_buf(cntxt, &lbl, 2)) STACK_ERROR;
			}
			else
			{
				/* Relation to other object : get object label */
				if(qry_obj_idfield(cntxt, &data, idrel, 0)) STACK_ERROR;
				if(qry_obj_label(cntxt, NULL, NULL, &lbl, NULL, NULL, NULL, NULL, NULL, 0, &data, 0)) STACK_ERROR;
			}
			DYNTAB_ADD_BUF(&cat->k[i].val, j, col_Label, lbl);
		}

		/* Check for objects without field values */
		M_FREE(sql);
		DYNBUF_ADD3_BUF(&sql,
			"SELECT DISTINCT IdObj FROM TLink\n"
			"WHERE IdObj IN (", cat->id, NO_CONV, ")\n");
		DYNBUF_ADD3_INT(&sql,
			"AND DateDel IS NULL AND IdField=", cat->k[i].idfld, "");
		if(sql_exec_query(cntxt, sql->data) || sql_get_buf(cntxt, &lbl, 2)) STACK_ERROR;

		/* Add empty value if applicable */
		nbobj = 0;
		if(!lbl || cntxt->sql_nbrows != cat->nbid)
		{
			/* Set matching objects list */
			unsigned long irow = cat->k[i].val.nbrows;
			if(qry_obj_field(cntxt, &data, DYNTAB_TOULRC(&cat->key, i, 0), "_EVA_LABEL")) STACK_ERROR;
			if(listobj_not_in(&sql, &nbobj, cat->id, lbl)) RETURN_ERR_MEMORY;
			DYNTAB_ADD_BUF(&cat->k[i].val, irow, col_IdObj, sql);
			DYNTAB_ADD_INT(&cat->k[i].val, irow, col_NbObj, nbobj);

			/* Set value label */
			DYNTAB_ADD_STR(&cat->k[i].val, irow, col_Label, "_");
		}

		/* Sort values */
		dyntab_sort(&cat->k[i].val, qsort_col4);

		/* One or no value for the field : select */
		if(cat->k[i].val.nbrows < 2 && cat->key.nbrows)
		{	 
			unsigned long idval = DYNTAB_TOULRC(&cat->k[i].val, 0, col_IdVal);
			unsigned long idrel = DYNTAB_TOULRC(&cat->k[i].val, 0, col_IdRel);
			cat->k[i].idsel = idval ? idval : idrel;
			cat->k[i].rel = idrel ? 1 : 0;
			if(i == cat->lvl) cat->lvl++;
		}

		/* Search selected value index in sorted values */
		for(j = 0; j < cat->k[i].val.nbrows;  j++)
		{
			unsigned long idval = DYNTAB_TOULRC(&cat->k[i].val, j, col_IdVal);
			unsigned long idrel = DYNTAB_TOULRC(&cat->k[i].val, j, col_IdRel);
			if(cat->k[i].idsel ? cat->k[i].idsel == (cat->k[i].rel ? idrel : idval) : !idval && !idrel)
			{
				cat->k[i].isel = j;
				break;
			}
		}

		/* Handle unmatched selected value */
		if(j >= cat->k[i].val.nbrows) 
		{
			cat->lvl = i;
			cat->k[i].isel = ~0UL;
			break;
		}

		/* Exit loop if selected level is reached */
		if(i >= cat->lvl) break;

		/* Filter objects on the selected value */
		if(read_lower_idobj(cntxt, cat, &cat->id, &cat->nbid, i, cat->k[i].isel)) STACK_ERROR;
	}

	/* Read distinct images for selected objects */
	if(!cat->id) RETURN_OK;
	M_FREE(sql);
	DYNBUF_ADD_STR(&sql, 
		"-- output_catalog_list : Read distinct images for selected objects\n"
		"SELECT DISTINCT ");
	if(cat->idimg_r) DYNBUF_ADD_STR(&sql, "IdRelObj") else DYNBUF_ADD_STR(&sql, "IdValue");
	DYNBUF_ADD3_BUF(&sql, 
		" FROM TLink\n"
		"WHERE IdObj IN (", cat->id, NO_CONV, ")\n");
	DYNBUF_ADD3_INT(&sql,
		"AND DateDel IS NULL AND Num=1 AND IdField=", cat->idimg, "");
	if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;

	/* Auto select image if only one */
	if(data.nbrows == 1) cat->idsel = DYNTAB_TOUL(&data);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_catalog
** Description : output controls of type CATALOG
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_catalog"
#define ERR_CLEANUP	qry_build_free(&flt); \
					for(i = 0; i < cat->key.nbrows; i++) DYNTAB_FREE(cat->k[i].val); \
					DYNTAB_FREE(cat->key); \
					DYNTAB_FREE(cat->img); \
					DYNTAB_FREE(cat->categ); \
					M_FREE(cat->id); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(cgival); \
					M_FREE(name); \
					M_FREE(sql)
int ctrl_output_catalog(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	QryBuild flt = {0};
	DynTable data = { 0 };
	DynTable imgattr = { 0 };
	DynTable cgival = { 0 };
	DynBuffer *name = NULL;
	DynBuffer *sql = NULL;
	Catalog _cat = {0}, *cat = &_cat;
	unsigned long i, dw = 0, dh = 0;
	char *btn = CGI_CLICK_BTN_SUBFIELD;
	CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
	CHECK_HTML_STATUS;

	/* Initialize catalog structure */
	cat->i_ctrl = i_ctrl;
	CTRL_ATTR(cat->key, SEARCH_CTRL);
	CTRL_ATTR(cat->img, IMG_CTRL);
	C_ALLOC(cat->k, sizeof(cat->k[0]), cat->key.nbrows);

	/* Get image path & field from image control */
	if(qry_listobj_field(cntxt, &imgattr, &cat->img, NULL)) STACK_ERROR;
	DYNTAB_FIELD1(&cat->imgdir, &imgattr, IMAGE_DIR);
	DYNTAB_FIELD(&data, &imgattr, TYPE);
	cat->idimg_r = !dyntab_txt_cmp(add_sz_str("_EVA_RELATION"), &data, 0, 0) ||
					!dyntab_txt_cmp(add_sz_str("_EVA_FILEUPLOAD"), &data, 0, 0);
	DYNTAB_FIELD(&data, &imgattr, FIELD);
	if(data.nbrows) cat->idimg = dyntab_cell(&data, 0, 0)->IdValue;
	cat->idName = IDVAL("_EVA_FILE_SERVER_NAME");

	/* Get thumbnails size from image control */
	if(cat->idimg_r)
	{
		/* relation field : use open button image dimensions */
		unsigned long idimgopn = strtoul(DYNTAB_FIELD_VAL(&imgattr, IMGBTN_CTRL), NULL, 10);
		if(qry_obj_field(cntxt, &imgattr, idimgopn, NULL)) STACK_ERROR;
	}
	dw = strtoul(DYNTAB_FIELD_VAL(&imgattr, WIDTH), NULL, 10);
	dh = strtoul(DYNTAB_FIELD_VAL(&imgattr, HEIGHT), NULL, 10);
	if(dw)
	{
		snprintf(add_sz_str(cat->dw1), "%lu", dw);
		snprintf(add_sz_str(cat->dw2), "%lu", dw / 2);
	}
	if(dh)
	{
		snprintf(add_sz_str(cat->dh1), "%lu", dh);
		snprintf(add_sz_str(cat->dh2), "%lu", dh / 2);
	}

	/* Read CGI selected values */
	CTRL_GET_CGI_SUBFIELD("SEL");
	if(dyntab_from_list(&data, DYNTAB_VAL_SZ(&cgival, 0, 0), ",", 0, 2)) RETURN_ERR_MEMORY;
	for(i = 0; i < data.nbrows; i++)
	{
		char *end = NULL;
		unsigned long *idsel = (i ? &cat->k[i - 1].idsel : &cat->idsel);
		int *rel = (i ? &cat->k[i - 1].rel : &cat->rel);
		*idsel = strtoul(dyntab_val(&data, i, 0), &end, 10);
		*rel = end && *end;
	}
	if(data.nbrows) cat->lvl = data.nbrows - 1;

	/* Handle control clicks */
	if(!strcmp(btn, "VAL"))
	{
		/* Item value click : change selection */
		cat->idsel = 0;
		cat->k[cgi->Num].idsel = cgi->Line;
		cat->k[cgi->Num].rel = cgi->Lang;
		cat->lvl = cgi->Num + 1;
	}
	else if(!strcmp(btn, "CLR"))
	{
		cat->idsel = 0;
		cat->lvl = 0;
		memset(cat->k, 0, sizeof(cat->k[0]) * cat->key.nbrows);
	}
	else if(!strcmp(btn, "IMG"))
	{
		/* List value click : set group selection */
		CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
		unsigned long lvl = (cat->lvl >= cat->key.nbrows) ? cat->key.nbrows - 1 : cat->lvl;
		cat->idsel = cgi->Num;
		cat->k[lvl].idsel = cgi->Line;
		cat->k[lvl].rel = cgi->Lang;
		cat->lvl = lvl + 1;
	}

	/* Get objects matching filter */
	CTRL_ATTR(data, BASE_FORMS);
	if(data.nbrows && qry_add_filter_forms(cntxt, &flt, &data, NULL)) STACK_ERROR;
	if(CTRL_ATTR_CELL(FILTER_TYPE) && qry_parse_filter_nodes(cntxt, &flt, &ctrl->attr)) STACK_ERROR;
	if(qry_filter_listobj(cntxt, &cat->id, &flt)) STACK_ERROR;
	cat->nbid = cntxt->sql_nbrows;

	/* Prepare values */
	if(prepare_catalog_values(cntxt, cat)) STACK_ERROR;

	/* Output control header */
	if(ctrl_format_pos(cntxt, ctrl, 1) || ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;

	/* Output selection tree */
	DYNBUF_ADD_STR(html, "<tr><td valign=top><font size=-2><nobr>");
	if(!CTRL_ATTR_CELL(NO_TREE) && output_catalog_tree(cntxt, cat, 0)) CLEAR_ERROR;

	/* Output images for selected items */
	DYNBUF_ADD_STR(html, "</td><td valign=top");
	if(ctrl->TABLEBGCOLOR && ctrl->TABLEBGCOLOR[0]) DYNBUF_ADD3(html, " bgcolor=#", ctrl->TABLEBGCOLOR, 0, TO_HTML, "");
	DYNBUF_ADD_STR(html, ">");
	if((cat->idsel ?
			output_catalog_group : 
		((!cat->key.nbrows  || cat->lvl == cat->key.nbrows)) ?
			output_catalog_list :
			output_catalog_index)(cntxt, cat)) CLEAR_ERROR;
	DYNBUF_ADD_STR(html, "</td></tr></table>");

	/* Output control footer */
	if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;

	/* Output hidden input to hold selected values */
	CTRL_CGINAMESUBFIELD(&name, NULL, "SEL");
	DYNBUF_ADD3_BUF(html, "<input type=hidden name='", name, NO_CONV, "' value='");
	DYNBUF_ADD_INT(html, cat->idsel);
	if(cat->rel) DYNBUF_ADD_STR(html, "R");
	for(i = 0; i < cat->lvl; i++)
	{
		DYNBUF_ADD_STR(html, ",");
		DYNBUF_ADD_INT(html, cat->k[i].idsel);
		if(cat->k[i].rel) DYNBUF_ADD_STR(html, "R");
	}
	DYNBUF_ADD_STR(html, "'>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_output_catalog
** Description : handles OUTPUT controls of type CATALOG
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_output_catalog"
#define ERR_CLEANUP	DYNTAB_FREE(displist)
int ctrl_add_output_catalog(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable displist = {0};

	switch(form->step)
	{
	case CtrlRead:
		CTRL_ATTR(displist, DISPLAY_LIST);
		if(displist.nbrows && ctrl_add_child(cntxt, i_ctrl, &displist)) STACK_ERROR;
		break;

	case HtmlView:
	case HtmlPrint:
	case HtmlEdit:
		if(ctrl_output_catalog(cntxt, i_ctrl)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
