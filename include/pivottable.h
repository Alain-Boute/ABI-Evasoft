/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : pivottable.h
** Description : declarations for pivot tables & statistics calculations
**      Author : Alain BOUTE
**     Created : April 2 2004
*********************************************************************/

#define ROWCNT (pv->row ? pv->row->labels.nbrows : 0)
#define COLCNT (pv->col ? pv->col->labels.nbrows : 0)

/*********************************************************************
** Function : pivottable_free
** Description : free data alloc-ed for a pivot table
*********************************************************************/
void pivottable_free(
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
);

/*********************************************************************
** Function : pivottable_add_field
** Description : build values table for a pivot table field
*********************************************************************/
int pivottable_add_field(					/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv,							/* in/out : pivot table data */
	PivotTableField *pvf					/* in : pivot field parameters */
);

/*********************************************************************
** Function : pivottable_select
** Description : build selection table with objects matching form & SQL condition
*********************************************************************/
int pivottable_select(						/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
);

/*********************************************************************
** Function : pivottable_calc_single
** Description : compute a pivot table with single indicator at each position
*********************************************************************/
int pivottable_calc_single(					/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
);

/*********************************************************************
** Function : pivottable_calc
** Description : compute a pivot table
*********************************************************************/
int pivottable_calc(						/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
);
