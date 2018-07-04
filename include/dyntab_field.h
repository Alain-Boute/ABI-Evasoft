/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dyntab_field.h
** Description : dynamic size table fields handling functions externals
**      Author : Alain BOUTE
**     Created : Dec 24 2002
*********************************************************************/


/*********************************************************************
** Macro : DYNTAB_FIELD
** Description : transfer given field values from a table to another
*********************************************************************/
#define DYNTAB_FIELD(dest, src, field) { dyntab_free(dest); \
	if(dyntab_filter_field(dest, 0, src, "_EVA_"#field, ~0UL, NULL)) RETURN_ERR_MEMORY; }
#define DYNTAB_FIELD1(dest, src, field) { dyntab_free(dest); \
	if(dyntab_filter_field(dest, 1, src, "_EVA_"#field, ~0UL, NULL)) RETURN_ERR_MEMORY; }
#define DYNTAB_FIELDLINE(dest, src, field, l) { dyntab_free(dest); \
	if(dyntab_filter_field(dest, 0, src, "_EVA_"#field, l, NULL)) RETURN_ERR_MEMORY; }
#define DYNTAB_FIELDBEG(dest, src, field, beg) { dyntab_free(dest); \
	if(dyntab_filter_field(dest, 0, src, "_EVA_"#field, ~0UL, beg)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNTAB_FIELD_TAB
** Description : transfer given field values from a table to another with row=Line / col=Num cells distibution
*********************************************************************/
#define DYNTAB_FIELD_TABBEG(dest, src, beg, field) { unsigned long row = beg; dyntab_free(dest); \
	if(dyntab_filter_field(dest, 0, src, "_EVA_"#field, ~0UL, &row) || dyntab_order_lines(dest)) RETURN_ERR_MEMORY; }
#define DYNTAB_FIELD_TAB(dest, src, field) { dyntab_free(dest); \
	if(dyntab_filter_field(dest, 0, src, "_EVA_"#field, ~0UL, NULL) || dyntab_order_lines(dest)) RETURN_ERR_MEMORY; }
#define DYNTAB_FIELD_TAB1(dest, src, field) { dyntab_free(dest); \
	if(dyntab_filter_field(dest, 1, src, "_EVA_"#field, ~0UL, NULL) || dyntab_order_lines(dest)) RETURN_ERR_MEMORY; }
#define DYNTAB_FIELD_TABK(dest, src, field) { DYNTAB_FIELD_TAB(dest, src, field); if(k < (dest)->nbrows) k = (dest)->nbrows; }
#define DYNTAB_FIELDTAB1(dest, src, field) { dyntab_free(dest); \
	if(dyntab_filter_field(dest, 1, src, field, ~0UL, NULL) || dyntab_order_lines(dest)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNTAB_FIELD_TABDEF
** Description : transfer given field values - get form default if not found
*********************************************************************/
#define	DYNTAB_FIELD_TABDEF(tbldest, tblsrc, attrname) \
{ \
	DYNTAB_FIELD_TAB(tbldest, tblsrc, attrname); \
	if(!(tbldest)->nbrows && cntxt->form && cntxt->form->ctrl) DYNTAB_FIELD_TAB(tbldest, &cntxt->form->ctrl->attr, attrname); \
	if(!(tbldest)->nbrows) DYNTAB_FIELD_TAB(tbldest, &cntxt->cnf_data, attrname); \
}

/*********************************************************************
** Macro : DYNTAB_FIELD_VAL
** Description : return given field 1st value from a values list
*********************************************************************/
#define DYNTAB_FIELD_VAL(src, field) dyntab_field_val(src, "_EVA_"#field, ~0UL, 0)
#define DYNTAB_FIELDLINE_VAL(src, field, l) dyntab_field_val(src, "_EVA_"#field, l, 0)

/*********************************************************************
** Macro : DYNTAB_FIELD_CELL
** Description : return given field 1st value from a values list
*********************************************************************/
#define DYNTAB_FIELD_CELL(src, field) dyntab_field_cell(src, "_EVA_"#field, ~0UL, 0)
#define DYNTAB_FIELDLINE_CELL(src, field, l) dyntab_field_cell(src, "_EVA_"#field, l, 0)

/*********************************************************************
** Function : dyntab_next_field
** Description : return the index of first row matching a given field
*********************************************************************/
unsigned long dyntab_next_field		/* return : row index in tab 
												tab->nbrows if no match */
(
	DynTable *tab,					/* in : table to process */
	char *field,				 	/* in : field to match */
	unsigned long line,				/* in : line to match - ~0UL for none */
	unsigned long matchcol,			/* in : column index used for search */
	int matchmode,					/* in : how to select rows
										0 : !strcmp(cell[matchcol].txt, field)
										1 : !strcmp(cell[matchcol].field, field)
										2 : matchcol == cell[0].col+1 */
	unsigned long row				/* in : first row to search in tab */
);

/*********************************************************************
** Function : dyntab_filter_field
** Description : return a subset of rows matching a given field
*********************************************************************/
int dyntab_filter_field(			/* return : 0 on success, other on error */
	DynTable *res,					/* out : filtered table */
	int b_copyval,					/* in : duplicate values if not 0 */
	DynTable *tab,					/* in : table to process */
	char *field,				 	/* in : field to match */
	unsigned long line,				/* in : line to match - ~0UL for none */
	unsigned long *row				/* in : first row to process in tab
									  out : last processed row in tab */
);

/*********************************************************************
** Function : dyntab_field_val
** Description : return the value of first row matching a given field
*********************************************************************/
char *dyntab_field_val				/* return : 1st value of field in tab */
(
	DynTable *tab,					/* in : table to process */
	char *field,				 	/* in : field to match */
	unsigned long line,				/* in : column index used for search */
	unsigned long beg				/* in : first row to search in tab */
);

/*********************************************************************
** Function : dyntab_field_cell
** Description : return the cell of first row matching a given field
*********************************************************************/
DynTableCell *dyntab_field_cell		/* return : 1st cell or NULL if not found */
(
	DynTable *tab,					/* in : table to process */
	char *field,				 	/* in : field to match */
	unsigned long line,				/* in : column index used for search */
	unsigned long beg				/* in : first row to search in tab */
);
