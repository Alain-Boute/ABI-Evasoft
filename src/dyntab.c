/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dyntab.c
** Description : dynamic size tables handling functions
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/


#include "dyntab.h"

/*********************************************************************
** Function : dyntab_free_cell
** Description : free a DynTable structure
*********************************************************************/
void dyntab_free_cell(
	DynTable *tab,		/* in : DynTable to free */
	unsigned long i		/* index of cell in tab->cell */
){
	if(tab->cell[i].txt && !tab->cell[i].b_dontfreetxt) M_FREE(tab->cell[i].txt);
	if(tab->cell[i].field && !tab->cell[i].b_dontfreefield) M_FREE(tab->cell[i].field);
}

/*********************************************************************
** Function : dyntab_free
** Description : free a DynTable structure
*********************************************************************/
void dyntab_free(
	DynTable *tab		/* in : DynTable to free */
){
	unsigned long i, j;

	if(!tab) return;
	j = tab->nbrows * tab->nbcols;
	for(i = 0; i < j; i++) dyntab_free_cell(tab, i);
	M_FREE(tab->cell);
	memset(tab, 0, sizeof(tab[0]));
}

/*********************************************************************
** Function : dyntab_resize
** Description : resize a DynTable structure to hold a given cell
*********************************************************************/
int dyntab_resize(			/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
){
	DynTableCell *cell;		/* new cells */
	unsigned long oldcols;	/* old line # of cells (szcols) */
	unsigned long oldrows;	/* old # of lines (nbrows) */
	unsigned long i;
	size_t sz;

	/* Realloc data cells if needed */
	if(!tab) return 0;
	if(row == ~0UL) row = 0;
	if(col == ~0UL) col = 0;
	if(row >= tab->szrows || col >= tab->szcols)
	{
		/* Calc new sizes for cells array */
		oldrows = tab->nbrows;
		oldcols = tab->szcols;
		if(row >= tab->szrows) tab->szrows = (row + 1) * 3 / 2;
		if(col >= tab->szcols) tab->szcols = (col + 1);

		/* Alloc memory for new cells array */
		sz = tab->szrows * tab->szcols * sizeof(cell[0]);
		cell = mem_alloc(sz);
		if(!cell) return 1;
		memset(cell, 0, sz);

		/* Reorganize cells with new column width if changed */
		if(tab->cell)
		{
			if(tab->szcols > oldcols) 
				for(i = 0; i < oldrows; i++)
					memcpy(cell + i * tab->szcols, tab->cell + i * oldcols, sizeof(cell[0]) * oldcols);
			else
					memcpy(cell, tab->cell, sizeof(cell[0]) * oldcols * tab->nbrows);

			M_FREE(tab->cell);
		}
		tab->cell = cell;
	}

	/* Set table size */
	if(col >= tab->nbcols) tab->nbcols = col + 1;
	if(row >= tab->nbrows) tab->nbrows = row + 1;

	return 0;
}

/*********************************************************************
** Function : dyntab_sz
** Description : return the value size of a cell
*********************************************************************/
size_t dyntab_sz(			/* out : string size */
	DynTable *tab,			/* in : table to read */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
){
	unsigned long i;

	/* Check params */
	if(!tab || row >= tab->nbrows || col >= tab->nbcols) return 0;

	/* Return cell size */
	i = DYNTAB_INDEX(tab, row, col);
	return	tab->cell[i].len ? tab->cell[i].len : tab->cell[i].txt ? strlen(tab->cell[i].txt) : 0;
}

/*********************************************************************
** Function : dyntab_val
** Description : return the value of a cell
*********************************************************************/
char *dyntab_val(			/* out : string value (never NULL) */
	DynTable *tab,			/* in : table to read */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
){
	unsigned long i;

	/* Check params */
	if(!tab || row >= tab->nbrows || col >= tab->nbcols) return "";

	/* Return cell value */
	i = DYNTAB_INDEX(tab, row, col);
	return tab->cell[i].txt ? tab->cell[i].txt : "";
}

/*********************************************************************
** Function : dyntab_cell
** Description : return the data size of a cell
*********************************************************************/
DynTableCell *dyntab_cell(	/* out : cell data */
	DynTable *tab,			/* in : table to read */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
){
	/* Check params */
	unsigned long i;
	if(!tab || row >= tab->nbrows || col >= tab->nbcols) return NULL;


	/* Return cell pointer */
	i = DYNTAB_INDEX(tab, row, col);
	return	tab->cell + i;
}

/*********************************************************************
** Function : dyntab_set
** Description : set a cell's contents in a DynTable structure
*********************************************************************/
int dyntab_set(					/* return : 0 on success, other on error */
	DynTable *tab,				/* in : table to process */
	unsigned long row,			/* in : row index */
	unsigned long col,			/* in : column index */
	DynTableCell *c				/* in : cell value to copy */
){
	unsigned long i;

	/* Memorize cell to avoid pointer sweeping on dyntab_resize (happens if c is in tab) */
	DynTableCell cell = {0};
	if(c) memcpy(&cell, c, sizeof(cell));

	/* Resize table if needed - return on error */
	if(!tab) return 0;
	if(dyntab_resize(tab, row, col)) return 1;

	/* Free previous value */
	i = DYNTAB_INDEX(tab, row, col);
	dyntab_free_cell(tab, i);

	/* Copy new value if applicable */
	memcpy(tab->cell + i, &cell, sizeof(cell));

	return 0;
}

/*********************************************************************
** Function : dyntab_moveto
** Description : move contents of a DynTable structure to another
*********************************************************************/
void dyntab_moveto(
	DynTable *dst,				/* out : destination table */
	DynTable *src				/* in : source table */
){
	dyntab_free(dst);
	memcpy(dst, src, sizeof(*dst));
	memset(src, 0, sizeof(*src));
}

/*********************************************************************
** Function : dyntab_copy
** Description : copy a cell's contents to a DynTable structure
*********************************************************************/
int dyntab_copy(				/* return : 0 on success, other on error */
	DynTable *tab,				/* in : destination table */
	unsigned long row,			/* in : row index */
	unsigned long col,			/* in : column index */
	DynTableCell *src,			/* in : source cell data */
	int b_strdup				/* in : duplicate strings if not 0 */
){
	DynTableCell *cell;

	if(!tab) return 0;

	/* Copy cell */
	if(dyntab_set(tab, row, col, src)) return 1;
	cell = dyntab_cell(tab, row, col);
	cell->b_dontfreetxt = 1;
	cell->b_dontfreefield = 1;

	/* Duplicate strings if applicable */
#ifndef _EVA_DLL
	/* TODO : bug - DLL crashes sometime : freed memory used */
	if(b_strdup)
#endif
	{
		/* Value string */
		if(cell->txt)
		{
			cell->txt = mem_strdup(cell->txt);
			if(!cell->txt) return 1;
			cell->b_dontfreetxt = 0;
		}
		
		/* Field string */
		if(cell->field)
		{
			cell->field = mem_strdup(cell->field);
			if(!cell->field) return 1;
			cell->b_dontfreefield = 0;
		}
	}

	return 0;
}

/*********************************************************************
** Function : dyntab_add
** Description : add a copy of text to a cell in a DynTable structure
*********************************************************************/
int dyntab_add(				/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	unsigned long row,		/* in : row index */
	unsigned long col,		/* in : column index */
	char *txt, size_t len,	/* in : text to add to table */
	ReplaceTable *sr,		/* in : search & replace table */
	int srmode				/* in : # of elements in sr */
){
	DynTableCell *cell;
	DynBuffer *buf = NULL;

	/* Resize table if needed - return on error */
	if(!tab) return 0;
	if(dyntab_resize(tab, row, col)) return 1;

	if(!len && txt) len = strlen(txt);

	/* Use a buffer to handle search & replace */
	if(sr && srmode && txt && len)
	{
		if(dynbuf_add(&buf, txt, len, sr, srmode)) return 1;
		txt = buf->data;
		len = buf->cnt;
	}

	/* Free previous text */
	cell = dyntab_cell(tab, row, col);
	if(!cell->b_dontfreetxt) mem_free(cell->txt);

	/* Reset cell text */
	cell->len = 0;
	cell->txt = 0;

	/* Add text if applicable */
	if(txt && *txt && len)
	{
		cell->txt = mem_alloc(len + 1);
		if(!cell->txt) { mem_free(buf); return 1; }
		memcpy(cell->txt, txt, len);
		cell->len = len;
		cell->b_dontfreetxt = 0;
	}
	mem_free(buf);
	return 0;
}

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
									bit 3 : reserved
									bit 4 : clear tab if set
									bit 5 : copy values if set */
){
	unsigned long i, j, row = row0, col = col0;
	int b_rows = fillmode & 1;
	int b_cols = fillmode & 2;
	int b_useempty = fillmode & 4;
	int b_strdup = fillmode & 32;

	/* Clear dest & check params - return as applicable */
	if(!dest) return 0;
	if(fillmode & 16) dyntab_free(dest);
	if(!src || !src->nbrows || !src->nbcols) return 0;
	if(rn > src->nbrows) rn = src->nbrows;
	if(cn > src->nbcols) cn = src->nbcols;
	if(r0 >= rn || c0 >= cn) return 0;

	/* Copy cells - loop on rows */
	for(i = r0; i < rn; i++)
	{
		for(j = c0; j < cn; j++)
		{
			DynTableCell *c = dyntab_cell(src, i, j);
			if(!b_useempty && (!c || !c->txt || !c->len)) continue;
			if(dyntab_copy(dest, row, col, c, b_strdup)) return 1;
			if(b_cols) col++; 
			else if(b_rows) row++;
			else return 0;
		}
		if(b_rows && b_cols) { row++;	col = col0; }
	}
	return 0;
}

/*********************************************************************
** Function : dyntab_from_list
** Description : fill a DynTable structure with a list contents
*********************************************************************/
int dyntab_from_list(			/* return : 0 on success, other on error */
	DynTable *tab,				/* in/out : table to fill */
	char *txt, size_t len,		/* in : list to read */
	char *sep,					/* in : items separator */
	unsigned long index,		/* in : row / column index */
	int fillmode				/* in : how to fill table :
									bit 0 : fill columns of index row if set, fill rows of index column else
									bit 1 : clear tab if set
									bit 2 : include cells for empty values if set */
){
	unsigned long i = 0;
	char *next = txt, *listend = txt + len;
	size_t sep_sz = strlen(sep);

	/* Clear table & return as applicable */
	if(!tab) return 0;
	if(fillmode & 2) dyntab_free(tab);
	if(!txt || !*txt || !len) return 0;

	/* No separator : put value in table */
	if(!sep || !*sep) return dyntab_add(tab, 0, 0, txt, len, NULL, 0);


	/* Separate items & place theim in table */
	i = fillmode & 1 ? tab->nbcols : tab->nbrows;
	do
	{
		char *end = strstr(next, sep);
		if(!end || end > listend) end = listend;
		if(end > next)
		{
			if(dyntab_add(tab, fillmode & 1 ? index : i, fillmode & 1 ? i : index, next, end - next, NULL, 0)) return 1;
			i++;
		}
		else if(fillmode & 4)
		{
			if(dyntab_add(tab, fillmode & 1 ? index : i, fillmode & 1 ? i : index, "", 0, NULL, 0)) return 1;
			i++;
		}
		next = end + sep_sz;
	}
	while(next < listend);

	return 0;
}

/*********************************************************************
** Function : dyntab_set_txt
** Description : set text to a cell in a DynTable structure
*********************************************************************/
int dyntab_set_txt(			/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	unsigned long row,		/* in : row index */
	unsigned long col,		/* in : column index */
	char *txt, size_t len	/* in : text to add to table */
){
	DynTableCell *cell;

	/* Resize table if needed - return on error */
	if(!tab) return 0;
	if(dyntab_resize(tab, row, col)) return 1;

	if(!len && txt) len = strlen(txt);

	/* Free previous text */
	cell = dyntab_cell(tab, row, col);
	if(!cell->b_dontfreetxt) mem_free(cell->txt);

	/* Set cell text */
	cell->b_dontfreetxt = 1;
	cell->txt = txt;
	cell->len = len;
	return 0;
}

/*********************************************************************
** Function : dyntab_to_dynbuf
** Description : dump a DynTable in a DynBuffer
*********************************************************************/
int dyntab_to_dynbuf(		/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	DynBuffer **buf,		/* out : buffer to output to */
	char *cs, size_t sz_cs,	/* in : column separator */
	char *rs, size_t sz_rs,	/* in : row separator */
	ReplaceTable *sr,		/* in : search & replace table */
	int srmode				/* in : # of elements in sr */
){
	unsigned long i, j;

	if(!tab) return 0;
	for(i = 0; i < tab->nbrows && (!i || rs); i++)
	{
		if(i && dynbuf_add(buf, rs, sz_rs, NO_CONV)) return 1;
		for(j = 0; j < tab->nbcols && (!j || cs); j++)
		{
			if(j && dynbuf_add(buf, cs, sz_cs, sr, srmode)) return 1;
			if(dynbuf_add(buf, DYNTAB_VAL_SZ(tab, i, j), sr, srmode)) return 1;
		}
	}
	return 0;
}

/*********************************************************************
** Function : dyntab_txt_cmp
** Description : returns the comparison result between text and a cell
*********************************************************************/
int dyntab_txt_cmp(			/* return : comparison result : -1 / 0 / 1 */
	char *txt,				/* in : text to compare */
	size_t sz,				/* in : # of chars in txt (0 = use strlen) */
	DynTable *tab,			/* in : DynTable pointer */
	unsigned long row,		/* in : row index */
	unsigned long col		/* in : column index */
){
	char *txt1;
	size_t sz1;

	/* Check params */
	if(!tab) return txt && *txt;
	if(!txt) txt = "";
	if(!sz) sz = strlen(txt);

	/* Return string comparison on max length */
	txt1 = dyntab_val(tab, row, col);
	sz1 = dyntab_sz(tab, row, col);
	return strncmp(txt, txt1, sz1 < sz ? sz : sz1);
}

/*********************************************************************
** Function : dyntab_cmp
** Description : returns the comparison result between two cells
*********************************************************************/
int dyntab_cmp(				/* return : comparison result : -1 / 0 / 1 */
	DynTable *tab1,			/* in : 1st DynTable pointer */
	unsigned long row1,		/* in : row index */
	unsigned long col1,		/* in : column index */
	DynTable *tab2,			/* in : 2nd DynTable pointer */
	unsigned long row2,		/* in : row index */
	unsigned long col2		/* in : column index */
){
	char *txt1;
	size_t sz1;

	/* Check params */
	txt1 = dyntab_val(tab1, row1, col1);
	sz1 = dyntab_sz(tab1, row1, col1);

	/* Return comparison result */
	return dyntab_txt_cmp(txt1, sz1, tab2, row2, col2);
}

/*********************************************************************
** Function : dyntab_del_numline
** Description : delete rows with given Num/Line indexes in a table
*********************************************************************/
void dyntab_del_numline(
	DynTable *tab,			/* in/out : table to delete in */
	unsigned long num,		/* in : num index */
	unsigned long line		/* in : line index */
){
	unsigned long i;
	if(!tab) return;
	for(i = tab->nbrows; i; i--)
	{
		DynTableCell *c = dyntab_cell(tab, i - 1, 0);
		if(!num || c->Num && !line || c->Line == line)
			dyntab_del_rows(tab, i - 1, 1);
	}
}

/*********************************************************************
** Function : dyntab_del_rows
** Description : delete rows in a table
*********************************************************************/
void dyntab_del_rows(
	DynTable *tab,					/* in : DynTable pointer */
	unsigned long row,				/* in : 1st row to delete */
	unsigned long nbrows			/* in : # of rows to delete */
){
	unsigned long i, j, k;

	/* Check params */
	if(!tab || row >= tab->nbrows) return;
	if(row + nbrows > tab->nbrows) nbrows = tab->nbrows - row;

	/* Free deleted rows cells */
	for(i = row; i < row + nbrows; i++)
		for(j = 0; j < tab->nbcols; j++)
		{
			k = DYNTAB_INDEX(tab, i, j);
			dyntab_free_cell(tab, k);
		}

	/* Move following rows & clear remaining rows */
	i = DYNTAB_INDEX(tab, row, 0);
	j = DYNTAB_INDEX(tab, row + nbrows, 0);
	k = tab->szcols	* (tab->nbrows - row - nbrows);
	memmove(tab->cell + i, tab->cell + j, sizeof(tab->cell[0]) * k);
	i = DYNTAB_INDEX(tab, tab->nbrows - nbrows, 0);
	k = tab->szcols	* nbrows;
	memset(tab->cell + i, 0, sizeof(tab->cell[0]) * k);
	tab->nbrows -= nbrows;
}

/*********************************************************************
** Function : dyntab_ins_rows
** Description : insert rows in a table
*********************************************************************/
int dyntab_ins_rows(				/* return : 0 on success, other on error */
	DynTable *tab,					/* in : DynTable pointer */
	unsigned long row,				/* in : insert before this row */
	unsigned long nbrows			/* in : # of rows to insert */
){
	unsigned long i, j, k, nbr, nbr0;

	/* Check params */
	if(!tab || !nbrows) return 0;

	/* Resize table */
	nbr0 = tab->nbrows;
	nbr = ((row < nbr0) ? nbr0 : row) + nbrows;
	if(dyntab_resize(tab, nbr, tab->nbcols)) return 1;

	/* Move following rows & clear new rows */
	if(nbr0 >= row)
	{
		i = DYNTAB_INDEX(tab, row, 0);
		j = DYNTAB_INDEX(tab, row + nbrows, 0);
		k = tab->szcols	* (nbr0 - row);
		memmove(tab->cell + j, tab->cell + i, sizeof(tab->cell[0]) * k);
		k = tab->szcols	* nbrows;
		memset(tab->cell + i, 0, sizeof(tab->cell[0]) * k);
		tab->nbrows = nbr;
	}
	return 0;
}

/*********************************************************************
** Function : dyntab_transform
** Description : return a transformed (cross tab) DynTable structure
**		Notes on the returned DynTable :
**			* ibuf member contains count
**			* txt member contains first value (points to tab->valbuf)
*********************************************************************/
DynTable *dyntab_transform(		/* return : pointer on transformed table if Ok, NULL on error */
	DynTable *tab,				/* in : table to transform */
	unsigned long row,			/* in : column # in tab used for lines */
	unsigned long col,			/* in : column # in tab used for columns */
	unsigned long val,			/* in : column # in tab used for values */
	DynTable *cross				/* in : initial columns or NULL
								   out : transformed table */
){
	unsigned long i, j, k;

	if(!tab || !cross) return NULL;

	cross->cell[0].txt = tab->cell[row].txt;

	/* for each row in source table */
	for(i = 1; i < tab->nbrows; i++)
	{
		/* Search for col value in row 1 */
		for(j = 1; j < cross->nbcols &&	
			STRCMPNUL(cross->cell[j].txt, tab->cell[col + i * tab->szcols].txt); j++);

		/* Add new column if not found */
		if(j == cross->nbcols)
		{
			if(dyntab_resize(cross, 0, j)) { dyntab_free(cross); return NULL; }
			cross->cell[j].txt = tab->cell[col + i * tab->szcols].txt;
		}

		/* Search for row value in column row */
		for(k = 1; k < cross->nbrows && 
			STRCMPNUL(cross->cell[k * cross->szcols].txt, tab->cell[row + i * tab->szcols].txt); k++);

		/* Add new line if not found */
		if(k == cross->nbrows)
		{
			if(dyntab_resize(cross, k, 0)) { dyntab_free(cross); return NULL; }
			cross->cell[k * cross->szcols].txt = tab->cell[row + i * tab->szcols].txt;
		}

		/* Set text & count */
		cross->cell[j + k * cross->szcols].txt = tab->cell[val + i * tab->szcols].txt;
		cross->cell[j + k * cross->szcols].len = tab->cell[val + i * tab->szcols].len;
	}

	return cross;
}
