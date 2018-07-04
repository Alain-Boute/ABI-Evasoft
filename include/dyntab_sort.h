/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dyntab_sortcb.h
** Description : qsort callbacks for ordering dynamic tables
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/


/*********************************************************************
** Function : dyntab_order_lines
** Description : place values of a DynTable structure at the corresponding val->line
*********************************************************************/
int dyntab_order_lines(		/* return : 0 on success, other on error */
	DynTable *tab			/* in/out : table to process */
);

/*********************************************************************
** Function : dyntab_group
** Description : group values of a DynTable structure
*********************************************************************/
int dyntab_group(			/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	char *oper				/* in : operation mode */
);

/*********************************************************************
** Function : dyntab_sort
** Description : sort a table
*********************************************************************/
void dyntab_sort(
	DynTable *tab,			/* in/out : table to sort */
	int (*qsort_cb)			/* in : qsort callback */
		(const void *el1, const void *el2)
);

/*********************************************************************
** Fonction : qsort_col1
** Description : qsort callback for DynTable - column 1 
*********************************************************************/
int qsort_col1(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col1desc
** Description : qsort callback for DynTable - column 1 descinding order
*********************************************************************/
int qsort_col1desc(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col2
** Description : qsort callback for DynTable - column 2 as text
*********************************************************************/
int qsort_col2(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col2i
** Description : qsort callback for DynTable - column 2 as integer
*********************************************************************/
int qsort_col2i(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col0i
** Description : qsort callback for DynTable - column 0 as integer
*********************************************************************/
int qsort_col0i(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col0idesc
** Description : qsort callback for DynTable - column 0 as integer descending
*********************************************************************/
int qsort_col0idesc(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col0desc
** Description : qsort callback for DynTable - column 0 descending
*********************************************************************/
int qsort_col0desc(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col0
** Description : qsort callback for DynTable - column 0
*********************************************************************/
int qsort_col0(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col1fdesc
** Description : qsort callback for DynTable - column 1 as float descending
*********************************************************************/
int qsort_col1fdesc(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col1f
** Description : qsort callback for DynTable - column 1 as float
*********************************************************************/
int qsort_col1f(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_col013i
** Description : qsort callback for DynTable - columns 0 (integer DESC), 1 (aplha) , 3 (integer)
*********************************************************************/
int qsort_col013i(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_val_idobj
** Description : qsort callback for table col 0 on Num
*********************************************************************/
int qsort_val_idobj(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_mode_values
** Description : qsort callback for table in values form mode
*********************************************************************/
int qsort_mode_values(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_val_num
** Description : qsort callback for table col 0 on Num
*********************************************************************/
int qsort_val_num(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_val_line
** Description : qsort callback for table col 0 on Num
*********************************************************************/
int qsort_val_line(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_val_col
** Description : qsort callback for table col 0 on col
*********************************************************************/
int qsort_val_col(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_val_fld
** Description : qsort callback for table col 0 on field
*********************************************************************/
int qsort_val_fld(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_val_fldnum
** Description : qsort callback for table col 0 on field / Num
*********************************************************************/
int qsort_val_fldnum(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_ctrlval
** Description : qsort callback for control values table
*********************************************************************/
int qsort_ctrlval(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_colval
** Description : qsort callback for table cells on col 0 IdObj / col / field / Num
*********************************************************************/
int qsort_colval(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_colobjdata
** Description : qsort callback for table cells on col 0 IdObj / col / field / Num
*********************************************************************/
int qsort_colobjdata(const void *el1, const void *el2);

/*********************************************************************
** Fonction : qsort_compare_values
** Description : qsort callback for values tables comparison
*********************************************************************/
int qsort_compare_values(const void *el1, const void *el2);
