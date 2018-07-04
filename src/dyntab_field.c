/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dyntab_field.c
** Description : dynamic size table fields handling functions
**      Author : Alain BOUTE
**     Created : Dec 24 2002
*********************************************************************/

#include <stdlib.h>
#include "dyntab.h"

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
){
	unsigned long i;

	if(!tab) return 0;
	if(!tab->nbrows || (matchmode != 2 && matchcol >= tab->nbcols)) return tab->nbrows;

	for(i = row; i < tab->nbrows; i++)
	{
		DynTableCell *cell = dyntab_cell(tab, i, matchmode == 2 ? 0 : matchcol);
		if(!cell) continue;
		if(cell->IdObj != tab->cell[row].IdObj) return tab->nbrows;
		if((	matchmode == 2 ? 
					(cell->col == matchcol) :
				matchmode == 1 ? 
					(!field || !strcmp(cell->field ? cell->field : "", field)) : 
				/* default matchmode */
				(!field || !strcmp(cell->txt ? cell->txt : "", field)))	&&
				/* line match */
				(line == ~0UL || line == cell->Line))
			break;
	}
	return i;
}

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
	unsigned long *beg				/* in : first row to process in tab
									  out : last processed row in tab */
){
	unsigned long i, j;

	if(!res || !tab) return 0;

	i = dyntab_next_field(tab, field, line, 0, 1, beg ? *beg : 0);
	while(i < tab->nbrows)
	{
		DynTableCell *cell = dyntab_cell(tab, i, 0);
		if(cell->IdObj != tab->cell[beg ? *beg : 0].IdObj)
		{
			if(beg) *beg = i - 1;
			return 0;
		}
		for(j = 0; j < tab->nbcols; j++)
			if(dyntab_copy(res, res->nbrows, j, dyntab_cell(tab, i, j), b_copyval)) return 1;
		i = dyntab_next_field(tab, field, line, 0, 1, i + 1);
	}
	if(beg) *beg = i - 1;
	return 0;
}

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
){
	return dyntab_val(tab, dyntab_next_field(tab, field, line, 0, 1, beg), 0);
}

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
){
	return dyntab_cell(tab, dyntab_next_field(tab, field, line, 0, 1, beg), 0);
}
