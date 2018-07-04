/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dyntab.h
** Description : dynamic tables structure definitions & macros
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "dynbuf.h"

/*********************************************************************
** Typedef : DynTableCell
** Description : cell description for dynamic size tables
*********************************************************************/
typedef struct _DynTableCell
{
	char *txt;					/* value text */
	size_t len;					/* value text length */
	char *field;				/* field name from TVal */
	char b_relation;			/* 1 if value is a relation (cell holds an object Id) */
	char b_modified;			/* !0 if cell holds a value changed by user (for ctrl->val) */
	char b_dontfreetxt;			/* do not free txt if not 0 */
	char b_dontfreefield;		/* do not free field if not 0 */

	/* Data for DB & CGI interface */
	unsigned long Pkey;			/* record identifier (primary key in TLink) */
	unsigned long IdObj;		/* object identifier in TLink */
	unsigned long IdValObj;		/* value object identifier in TLink */
	unsigned long IdField;		/* field identifier in TVal */
	unsigned long IdValue;		/* value identifier in TVal (cell holds a value stored in TVal) */
	unsigned long Num;			/* value index to handle multiple values */
	unsigned long Line;			/* value index to handle table rows & list options level */
	unsigned long Lang;			/* language code to handle multiple language values */
	unsigned long i_cgi;		/* index in cntxt->cgi (cell holds CGI data) */

	/* Data for tables output */
	enum _EVA_celltype
	{
		Ct_cellval,
		Ct_rowtot,
		Ct_coltot,
		Ct_grandtot,
		Ct_rowlbl,
		Ct_collbl,
		Ct_rowtotlbl,
		Ct_coltotlbl,
		Ct_rowname,
		Ct_colname
	} 
		ctype;					/* cell value type for tables formating */
	unsigned long col;			/* index of table column (cell holds a table value) */
	unsigned long row;			/* index of table row (cell holds a table value) */
}
	DynTableCell;

/*********************************************************************
** Typedef : DynTable
** Description : dynamic size table of values
*********************************************************************/
typedef struct _DynTable
{
	unsigned long nbrows;		/* # of used rows in the table */
	unsigned long szrows;		/* # of alloc-ed rows in the table */		
	unsigned long nbcols;		/* # of used columns in the table */
	unsigned long szcols;		/* # of alloc-ed columns in the table */		
	DynTableCell *cell;			/* cells values */
}
	DynTable;

/*********************************************************************
** Function : dyntab_free_cell
** Description : free a DynTable structure
*********************************************************************/
void dyntab_free_cell(
	DynTable *tab,		/* in : DynTable to free */
	unsigned long i		/* index of cell in tab->cell */
);

/*********************************************************************
** Function : dyntab_free
** Description : free a DynTable structure
*********************************************************************/
void dyntab_free(
	DynTable *tab		/* in : DynTable to free */
);

/*********************************************************************
** Function : dyntab_resize
** Description : resize a DynTable structure to hold a given cell
*********************************************************************/
int dyntab_resize(			/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
);

/*********************************************************************
** Function : dyntab_sz
** Description : return the value size of a cell
*********************************************************************/
size_t dyntab_sz		/* out : string value (never NULL) */
(
	DynTable *tab,			/* in : table to read */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
);

/*********************************************************************
** Function : dyntab_val
** Description : return the value of a cell
*********************************************************************/
char *dyntab_val		/* out : string value (never NULL) */
(
	DynTable *tab,			/* in : table to read */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
);

/*********************************************************************
** Function : dyntab_cell
** Description : return the data size of a cell
*********************************************************************/
DynTableCell *dyntab_cell	/* out : cell data */
(
	DynTable *tab,			/* in : table to read */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
);

/*********************************************************************
** Function : dyntab_set
** Description : set a cell's contents in a DynTable structure
*********************************************************************/
int dyntab_set					/* return : 0 on success, other on error */
(
	DynTable *tab,				/* in : table to process */
	unsigned long row,			/* in : row index */
	unsigned long col,			/* in : column index */
	DynTableCell *cell			/* in : cell value to copy */
);

/*********************************************************************
** Function : dyntab_moveto
** Description : move contents of a DynTable structure to another
*********************************************************************/
void dyntab_moveto(
	DynTable *dst,				/* out : destination table */
	DynTable *src				/* in : source table */
);

/*********************************************************************
** Function : dyntab_copy
** Description : copy a cell's contents to a DynTable structure
*********************************************************************/
int dyntab_copy			/* return : 0 on success, other on error */
(
	DynTable *tab,				/* in : destination table */
	unsigned long row,			/* in : row index */
	unsigned long col,			/* in : column index */
	DynTableCell *src,			/* in : source cell data */
	int b_strdup				/* in : duplicate strings if non 0 */
);

/*********************************************************************
** Function : dyntab_add
** Description : add text to a cell in a DynTable structure
*********************************************************************/
int dyntab_add				/* return : 0 on success, other on error */
(
	DynTable *tab,			/* in : table to process */
	unsigned long row,		/* in : row index */
	unsigned long col,		/* in : column index */
	char *txt, size_t len,	/* in : text to add to table */
	ReplaceTable *sr,		/* in : search & replace table */
	int srmode				/* in : search & replace direction (1=direct, 0=none, -1=reverse) */
);

/*********************************************************************
** Function : dyntab_from_tab
** Description : copy part of a DynTable structure to another
*********************************************************************/
int dyntab_from_tab(		/* return : 0 on success, other on error */
	DynTable *dest,			/* out : table to fill */
	unsigned long row0,		/* in : destination row index */
	unsigned long col0,		/* in : destination column index */
	DynTable *src,			/* in : table to read from - copied range as follows */
	unsigned long r0,		/* in : start at this row index */
	unsigned long c0,		/* in : start at this column index */
	unsigned long rn,		/* in : stop at this row index */
	unsigned long cn,		/* in : stop at this column index */
	int fillmode			/* in : how to fill table :
									bit 0 : use multiple rows if set
									bit 1 : use multiple colums if set
									bit 2 : include empty values if set
									bit 3 : loop on source columns first if set
									bit 4 : clear tab if set
									bit 5 : copy values if set */
);

/*********************************************************************
** Function : dyntab_from_list
** Description : fill a DynTable structure with a list contents
*********************************************************************/
int dyntab_from_list(			/* return : 0 on success, other on error */
	DynTable *tab,				/* in/out : table to fill */
	char *txt, size_t len,		/* in : list to read */
	char *sep,					/* in : items separator */
	unsigned long rowcol,		/* in : row / column index */
	int fillmode				/* in : how to fill table :
									bit 0 : fill rowcol row if set, fill rowcol row else
									bit 1 : clear tab if set */
);

/*********************************************************************
** Function : dyntab_set_txt
** Description : set text to a cell in a DynTable structure
*********************************************************************/
int dyntab_set_txt(			/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	unsigned long row,		/* in : row index */
	unsigned long col,		/* in : column index */
	char *txt, size_t len	/* in : text to add to table */
);

/*********************************************************************
** Function : dyntab_to_dynbuf
** Description : dump a DynTable in a DynBuffer
*********************************************************************/
int dyntab_to_dynbuf		/* return : 0 on success, other on error */
(
	DynTable *tab,			/* in : table to process */
	DynBuffer **buf,		/* out : bufferu to output to */
	char *cs, size_t sz_cs,	/* in : column separator */
	char *rs, size_t sz_rs,	/* in : row separator */
	ReplaceTable *sr,		/* in : search & replace table */
	int srmode				/* in : search & replace direction (1=direct, 0=none, -1=reverse) */
);

/*********************************************************************
** Function : dyntab_txt_cmp
** Description : returns the comparison result between text and a cell
*********************************************************************/
int dyntab_txt_cmp			/* return : comparison result : -1 / 0 / 1 */
(
	char *txt,				/* in : text to compare */
	size_t sz,				/* in : # of chars in txt (0 = use strlen) */
	DynTable *tab,			/* in : DynTable pointer */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
);

/*********************************************************************
** Function : dyntab_cmp
** Description : returns the comparison result between two cells
*********************************************************************/
int dyntab_cmp			/* return : comparison result : -1 / 0 / 1 */
(
	DynTable *tab1,			/* in : 1st DynTable pointer */
	unsigned long row1,		/* in : row index */
	unsigned long col1,		/* in : column index */
	DynTable *tab2,			/* in : 2nd DynTable pointer */
	unsigned long row2,		/* in : row index */
	unsigned long col2		/* in : column index */
);

/*********************************************************************
** Function : dyntab_del_numline
** Description : delete rows with given Num/Line indexes in a table
*********************************************************************/
void dyntab_del_numline(
	DynTable *tab,			/* in/out : table to delete in */
	unsigned long num,		/* in : num index */
	unsigned long line		/* in : line index */
);

/*********************************************************************
** Function : dyntab_del_rows
** Description : delete rows in a table
*********************************************************************/
void dyntab_del_rows(
	DynTable *tab,					/* in : DynTable pointer */
	unsigned long row,				/* in : 1st row to delete */
	unsigned long nbrows			/* in : # of rows to delete */
);

/*********************************************************************
** Function : dyntab_ins_rows
** Description : insert rows in a table
*********************************************************************/
int dyntab_ins_rows(				/* return : 0 on success, other on error */
	DynTable *tab,					/* in : DynTable pointer */
	unsigned long row,				/* in : insert before this row */
	unsigned long nbrows			/* in : # of rows to insert */
);

/*********************************************************************
** Macro : DYNTAB_FREE
** Description : free a DynTable structure
*********************************************************************/
#define DYNTAB_FREE(tab) dyntab_free(&(tab))

/*********************************************************************
** Macro : DYNTAB_INDEX
** Description : calc cell position in a DynTable structure
*********************************************************************/
#define DYNTAB_INDEX(tab, row, col) ((col) + ((row) * tab->szcols))

/*********************************************************************
** Macro : DYNTAB_ADD
** Description : add a cell value in a DynTable structure
*********************************************************************/
#define DYNTAB_ADD(tab, row, col, txt, len, conv) \
	{ if(dyntab_add(tab, row, col, txt, len, conv)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNTAB_ADD_NL
** Description : add a cell value in a DynTable structure & set Num & Line attribute
*********************************************************************/
#define DYNTAB_ADD_NL(tab, _row, _col, txt, len, __n, _l, _b_modif) \
	{	char b_modif = (char)(_b_modif); unsigned long _n = __n, l = _l, row = _row, col = _col; \
		DynTableCell *_c; if(dyntab_add(tab, row, col, txt, len, NO_CONV)) RETURN_ERR_MEMORY; \
		_c = dyntab_cell(tab, row, col); _c->Num = _n; _c->Line = l; _c->b_modified = b_modif; }

/*********************************************************************
** Macro : DYNTAB_ADD_FIELD
** Description : add a cell value in a DynTable structure & set Field attribute
*********************************************************************/
#define DYNTAB_ADD_FIELD(_tab, _row, _col, _txt, _len, _field, _idfield, _idobj) DYNTAB_ADD_FIELD_NL(_tab, _row, _col, _txt, _len, _field, _idfield, _idobj, 1, 0)
#define DYNTAB_ADD_FIELD_NL(_tab, _row, _col, _txt, _len, _field, _idfield, _idobj, _num, _line) \
	{	DynTable *__tab = _tab; unsigned long __row = _row, __col = _col; DynTableCell *_c; \
		if(dyntab_add(__tab, __row, __col, _txt, _len, NO_CONV)) RETURN_ERR_MEMORY; \
		_c = dyntab_cell(__tab, __row, __col); \
		_c->IdObj = _idobj; \
		_c->field = _field; \
		_c->IdField = _idfield; \
		_c->Num = _num; \
		_c->Line = _line; \
		_c->b_dontfreefield = 1; }

/*********************************************************************
** Macro : DYNTAB_ADD_TXTID
** Description : add a cell value in a DynTable structure & set IdValue attribute
*********************************************************************/
#define DYNTAB_ADD_TXTID(tab, _row, _col, txt, _id) \
	{	unsigned long id = _id, row = _row, col = _col; \
		DynTableCell *_c; if(dyntab_add(tab, row, col, add_sz_str(txt), NO_CONV)) RETURN_ERR_MEMORY; \
		_c = dyntab_cell(tab, row, col); _c->IdValue = id; }

/*********************************************************************
** Macro : DYNTAB_ADD_CELL
** Description : add a cell to a cell value in a DynTable structure
*********************************************************************/
#define DYNTAB_ADD_CELL(tab1, row1, col1, tab2, row2, col2) DYNTAB_ADD_CELLP(tab1, row1, col1, dyntab_cell(tab2, row2, col2))

/*********************************************************************
** Macro : DYNTAB_ADD_CELLP
** Description : copy a cell value in a DynTable structure
*********************************************************************/
#define DYNTAB_ADD_CELLP(tab, row, col, _c_) { if(dyntab_copy(tab, row, col, _c_, 1)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNTAB_ADD_STR
** Description : set a cell value to a static string in a DynTable structure
*********************************************************************/
#define DYNTAB_ADD_STR(tab, row, col, tag) { if(dyntab_set_txt(tab, row, col, tag, sizeof(tag) - 1)) RETURN_ERR_MEMORY; }
#define DYNTAB_SET(tab, row, col, tag) DYNTAB_ADD_STR(tab, row, col, tag)

/*********************************************************************
** Macro : DYNTAB_SET_CELL
** Description : set a cell value in a DynTable structure with value of another cell
*********************************************************************/
#define DYNTAB_SET_CELL(tab1, row1, col1, tab2, row2, col2) DYNTAB_SET_CELLP(tab1, row1, col1, dyntab_cell(tab2, row2, col2))

/*********************************************************************
** Macro : DYNTAB_SET_CELLP
** Description : set a cell value in a DynTable structure with value of another cell
*********************************************************************/
#define DYNTAB_SET_CELLP(tab, row, col, cell) { if(dyntab_copy(tab, row, col, cell, 0)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNTAB_VAL_SZ
** Description : returns cell value & size
*********************************************************************/
#define DYNTAB_VAL_SZ(tab, row, col) dyntab_val(tab, row, col), dyntab_sz(tab, row, col)

/*********************************************************************
** Macro : DYNTAB_ADD_BUF
** Description : add a DynBuffer content in a DynTable structure
*********************************************************************/
#define DYNTAB_ADD_BUF(tab, row, col, buf) \
	{ if((buf) && (buf)->cnt) DYNTAB_ADD(tab, row, col, (buf)->data, (buf)->cnt, NO_CONV) \
	  else DYNTAB_ADD(tab, row, col, NULL, 0, NO_CONV) }

/*********************************************************************
** Macro : DYNTAB_ADD_INT
** Description : add an integer value using DYNBUF_ADD
*********************************************************************/
#define DYNTAB_ADD_INT(tab, row, col, nb) \
{ \
	char _tmp[32] = {0}; \
	itoa(nb, _tmp, 10); \
	DYNTAB_ADD(tab, row, col, _tmp, 0, NO_CONV); \
}

/*********************************************************************
** Macro : DYNTAB_TOUL
** Description : convert tab(0,0) tu unsigned long
*********************************************************************/
#define DYNTAB_TOULRC(tab, row, col) strtoul(dyntab_val(tab, row, col), NULL, 10)
#define DYNTAB_TOUL(tab) DYNTAB_TOULRC(tab, 0, 0)

/*********************************************************************
** Macro : DYNTAB_SAMEVAL
** Description : return 1 if two cells hold identical values
*********************************************************************/
#define DYNTAB_SAMEVAL(val, dbval) (val->len == dbval->len && \
					val->Num == dbval->Num && \
					val->Line == dbval->Line && \
					val->len == dbval->len && \
					!STRCMPNUL(val->txt, dbval->txt))

/*********************************************************************
** Macro : DYNTAB_SAMEVAL_MODE
** Description : return 1 if two cells hold identical values with comparison mode
*********************************************************************/
#define DYNTAB_SAMEVAL_MODE(val, dbval, mode) ( \
					val->len == dbval->len && \
					(mode == 1 || mode == 4 || mode == 5 || val->Num == dbval->Num) && \
					(mode == 1 || mode == 5 || val->Line == dbval->Line) && \
					!STRCMPNUL(val->txt, dbval->txt))
