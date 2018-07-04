/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dyntab_sortcb.c
** Description : qsort callbacks for ordering dynamic tables
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : dyntab_order_lines
** Description : place values of a DynTable structure at the corresponding val->line
*********************************************************************/
int dyntab_order_lines(		/* return : 0 on success, other on error */
	DynTable *tab			/* in/out : table to process */
){
	DynTableCell *val;
	DynTable res = {0};
	unsigned long i, j, line, lines = 0, nbrows = tab ? tab->nbrows : 0;
	int b_reorder = 0;

	/* Get maximum line index in table */
	if(!nbrows) return 0;
	dyntab_sort(tab, qsort_ctrlval);
	for(i = 0; i < nbrows; i++)
	{
		val = dyntab_cell(tab, i, 0);
		if(!val->len || !val->txt || !*val->txt) continue;
		if(val->Line > lines) lines = val->Line;
		if(val->Line && val->Line != i + 1) b_reorder = 1;
	}
	if(!b_reorder) return 0;

	/* Place values in lines */
	for(i = 0; i < nbrows; i++)
	{
		val = dyntab_cell(tab, i, 0);
		if(!val->len || !val->txt || !*val->txt) continue;
		line = val->Line ? val->Line - 1 : lines ? lines : 0;
		for(j = 0; j < res.nbcols && dyntab_sz(&res, line, j); j++);
		if(dyntab_copy(&res, line, j, val, 
				!val->b_dontfreefield  && val->field ||
				!val->b_dontfreetxt && val->txt))
			return 1;
	}
	dyntab_moveto(tab, &res);
	return 0;
}

/*********************************************************************
** Function : dyntab_sort
** Description : sort a table
*********************************************************************/
void dyntab_sort(
	DynTable *tab,			/* in/out : table to sort */
	int (*qsort_cb)			/* in : qsort callback */
		(const void *el1, const void *el2)
){
	/* Check params */
	if(!tab) return;

	/* Call qsort */
	qsort(tab->cell, tab->nbrows, sizeof(tab->cell[0])*tab->szcols, qsort_cb);

}

/*********************************************************************
** Fonction : qsort_col1
** Description : qsort callback for DynTable - column 1 
*********************************************************************/
int qsort_col1(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1+1)->txt;
	char *t2 = ((DynTableCell *)el2+1)->txt;
	int res;
	int i1, i2;

	if(!t1 || !*t1) t1 = "~";
	if(!t2 || !*t2) t2 = "~";

	res = stricmp(t1, t2);
	if(res) return res;

	/* Identical strings : sort on column 0 integer */
	t1 = ((DynTableCell *)el1)->txt;
	t2 = ((DynTableCell *)el2)->txt;
	i1 = t1 ? atoi(t1) : 0;
	i2 = t2 ? atoi(t2) : 0;

	return i1 > i2 ? 1 : i1 < i2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_col1desc
** Description : qsort callback for DynTable - column 1 descending order
*********************************************************************/
int qsort_col1desc(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1+1)->txt;
	char *t2 = ((DynTableCell *)el2+1)->txt;
	int res;
	int i1, i2;

	if(!t1) t1 = "";
	if(!t2) t2 = "";

	res = -stricmp(t1, t2);
	if(res) return res;

	t1 = ((DynTableCell *)el1)->txt;
	t2 = ((DynTableCell *)el2)->txt;
	i1 = t1 ? atoi(t1) : 0;
	i2 = t2 ? atoi(t2) : 0;

	return i1 > i2 ? 1 : i1 < i2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_col2
** Description : qsort callback for DynTable - column 1 
*********************************************************************/
int qsort_col2(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1+2)->txt;
	char *t2 = ((DynTableCell *)el2+2)->txt;
	if(!t1 || !*t1) t1 = "²";
	if(!t2 || !*t2) t2 = "²";
	return _stricmp( t1, t2 );
}

/*********************************************************************
** Fonction : qsort_col2i
** Description : qsort callback for DynTable - column 2 as number
*********************************************************************/
int qsort_col2i(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1+2)->txt;
	char *t2 = ((DynTableCell *)el2+2)->txt;
	int i1, i2;

	i1 = t1 ? atoi(t1) : 0;
	i2 = t2 ? atoi(t2) : 0;

	return i1 > i2 ? 1 : i1 < i2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_col0idesc
** Description : qsort callback for DynTable - column 0 as number descending
*********************************************************************/
int qsort_col0idesc(const void *el1, const void *el2)
{
	return -qsort_col0i(el1, el2);
}

/*********************************************************************
** Fonction : qsort_col0i
** Description : qsort callback for DynTable - column 0 as number
*********************************************************************/
int qsort_col0i(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1)->txt;
	char *t2 = ((DynTableCell *)el2)->txt;
	double i1, i2;

	i1 = t1 ? atof(t1) : 0;
	i2 = t2 ? atof(t2) : 0;

	return i1 > i2 ? 1 : i1 < i2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_col0desc
** Description : qsort callback for DynTable - column 0 descending
*********************************************************************/
int qsort_col0desc(const void *el1, const void *el2)
{
	return -qsort_col0(el1, el2);
}

/*********************************************************************
** Fonction : qsort_col0
** Description : qsort callback for DynTable - column 0
*********************************************************************/
int qsort_col0(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1)->txt;
	char *t2 = ((DynTableCell *)el2)->txt;
	return STRCMPNUL(t1, t2);
}

/*********************************************************************
** Fonction : qsort_col1fdesc
** Description : qsort callback for DynTable - column 1 as float descending
*********************************************************************/
int qsort_col1fdesc(const void *el1, const void *el2)
{
	return -qsort_col1f(el1, el2);
}

/*********************************************************************
** Fonction : qsort_col1f
** Description : qsort callback for DynTable - column 1 as float
*********************************************************************/
int qsort_col1f(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1 + 1)->txt;
	char *t2 = ((DynTableCell *)el2 + 1)->txt;
	double f1, f2;

	f1 = t1 ? atof(t1) : 0;
	f2 = t2 ? atof(t2) : 0;

	return f1 > f2 ? 1 : f1 < f2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_col013i
** Description : qsort callback for DynTable - columns 0 (integer DESC), 1 (aplha) , 3 (integer)
*********************************************************************/
int qsort_col013i(const void *el1, const void *el2)
{
	char *t10 = ((DynTableCell *)el1)->txt;
	char *t20 = ((DynTableCell *)el2)->txt;
	char *t11 = ((DynTableCell *)el1+1)->txt;
	char *t21 = ((DynTableCell *)el2+1)->txt;
	char *t13 = ((DynTableCell *)el1+3)->txt;
	char *t23 = ((DynTableCell *)el2+3)->txt;
	int i1, i2, res;

	i1 = t10 ? atoi(t10) : 0;
	i2 = t20 ? atoi(t20) : 0;
	if(i1 != i2) return i1 > i2 ? 1 : -1;

	res = stricmp(t11 ? t11 : "", t21 ? t21 : "");
	if(res) return res;

	i1 = t13 ? atoi(t13) : 0;
	i2 = t23 ? atoi(t23) : 0;
	if(i1 != i2) return i1 > i2 ? 1 : -1;
	return 0;
}

/*********************************************************************
** Fonction : qsort_val_idobj
** Description : qsort callback for table cell->IdObj
*********************************************************************/
int qsort_val_idobj(const void *el1, const void *el2)
{
	unsigned long n1 = ((DynTableCell *)el1)->IdObj;
	unsigned long n2 = ((DynTableCell *)el2)->IdObj;
	return n1 > n2 ? 1 : n1 < n2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_val_num
** Description : qsort callback for table col 0 on Num
*********************************************************************/
int qsort_val_num(const void *el1, const void *el2)
{
	unsigned long n1 = ((DynTableCell *)el1)->Num;
	unsigned long n2 = ((DynTableCell *)el2)->Num;
	return n1 > n2 ? 1 : n1 < n2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_val_line
** Description : qsort callback for table col 0 on Num
*********************************************************************/
int qsort_val_line(const void *el1, const void *el2)
{
	unsigned long n1 = ((DynTableCell *)el1)->Line;
	unsigned long n2 = ((DynTableCell *)el2)->Line;
	return n1 > n2 ? 1 : n1 < n2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_val_col
** Description : qsort callback for table col 0 on col
*********************************************************************/
int qsort_val_col(const void *el1, const void *el2)
{
	unsigned long n1 = ((DynTableCell *)el1)->col;
	unsigned long n2 = ((DynTableCell *)el2)->col;
	return n1 > n2 ? 1 : n1 < n2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_val_row
** Description : qsort callback for table col 0 on row
*********************************************************************/
int qsort_val_row(const void *el1, const void *el2)
{
	unsigned long n1 = ((DynTableCell *)el1)->row;
	unsigned long n2 = ((DynTableCell *)el2)->row;
	return n1 > n2 ? 1 : n1 < n2 ? -1 : 0;
}

/*********************************************************************
** Fonction : qsort_val_fld
** Description : qsort callback for table col 0 on field
*********************************************************************/
int qsort_val_fld(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1)->field;
	char *t2 = ((DynTableCell *)el2)->field;
	return STRICMPNUL(t1, t2);
}

/*********************************************************************
** Fonction : qsort_val_fldnum
** Description : qsort callback for table col 0 on field / Num
*********************************************************************/
int qsort_val_fldnum(const void *el1, const void *el2)
{
	int res = qsort_val_fld(el1, el2);
	if(!res) res = qsort_ctrlval(el1, el2);
	return res;
}

/*********************************************************************
** Fonction : qsort_ctrlval
** Description : qsort callback for control values table
*********************************************************************/
int qsort_ctrlval(const void *el1, const void *el2)
{
	int res = qsort_val_num(el1, el2);
	if(!res) 
	{
		res = qsort_val_line(el1, el2);
		if(!res) res = qsort_col0(el1, el2);
	}
	return res;
}

/*********************************************************************
** Fonction : qsort_colval
** Description : qsort callback for table cells on col 0 on IdObj / col / field / Num
*********************************************************************/
int qsort_colval(const void *el1, const void *el2)
{
	int res = qsort_val_idobj(el1, el2);
	if(!res)
	{
		res = qsort_val_col(el1, el2);
		if(!res)
		{
			res = qsort_val_row(el1, el2);
			if(!res) res = qsort_val_fldnum(el1, el2);
		}
	}
	return res;
}

/*********************************************************************
** Fonction : qsort_mode_values
** Description : qsort callback for table in values form mode
*********************************************************************/
int qsort_mode_values(const void *el1, const void *el2)
{
	int res = qsort_val_idobj(el1, el2);
	if(!res) res = qsort_val_col(el1, el2);
	if(!res) res = qsort_val_fld(el1, el2);
	if(!res) res = qsort_val_line(el1, el2);
	if(!res) res = qsort_val_num(el1, el2);
	if(!res)
	{
		DynTableCell *e1 = (DynTableCell *)el1;
		DynTableCell *e2 = (DynTableCell *)el2;
		res = e1->Pkey < e2->Pkey ? 1 : -1; 
	}
	return res;
}

/*********************************************************************
** Fonction : qsort_compare_values
** Description : qsort callback for values tables comparison
*********************************************************************/
int qsort_compare_values(const void *el1, const void *el2)
{
	int res = qsort_val_idobj(el1, el2);
	if(!res) res = qsort_val_col(el1, el2);
	if(!res) res = qsort_val_fld(el1, el2);
	if(!res) res = qsort_val_line(el1, el2);
	if(!res) res = qsort_val_num(el1, el2);
	if(!res)
	{
		DynTableCell *e1 = (DynTableCell *)el1;
		DynTableCell *e2 = (DynTableCell *)el2;
		res = STRCMPNUL(e1->txt, e2->txt); 
	}
	return res;
}

/*********************************************************************
** Fonction : qsort_colobjdata
** Description : qsort callback for table cells on col 0 IdObj / col / field / Num
*********************************************************************/
int qsort_colobjdata(const void *el1, const void *el2)
{
	int res = qsort_val_col(el1, el2);
	if(!res) res = qsort_val_fldnum(el1, el2);
	return res;
}

/*********************************************************************
** Function : dyntab_group
** Description : group values of a DynTable structure
*********************************************************************/
int dyntab_group(			/* return : 0 on success, other on error */
	DynTable *tab,			/* in : table to process */
	char *oper				/* in : operation mode */
){
	if(!tab) return 0;

	/* Handle sum operation mode */
	if(!strcmp(oper, "SUM"))
	{
		double sum = 0;
		unsigned long i;
		char s[16];
		for(i = 0; i < tab->nbrows; i++) sum += atof(dyntab_val(tab, i, 0));
		dyntab_free(tab);
		return dyntab_add(tab, 0, 0, s, dbl_str(s, sum, NULL), NO_CONV);
	}

	/* Handle average operation mode */
	if(!strcmp(oper, "AVG"))
	{
		double sum = 0;
		unsigned long i, nb;
		char s[16];
		for(i = 0, nb = 0; i < tab->nbrows; i++)
		{
			char *v = dyntab_val(tab, i, 0);
			double n;
			nb += sscanf(v, "%lf", &n);
			sum += n;
		}
		dyntab_free(tab);
		return dyntab_add(tab, 0, 0, s, dbl_str(s, nb ? sum / nb : 0, NULL), NO_CONV);
	}

	/* Handle minimum / maximum operation mode */
	if(!strcmp(oper, "MIN") || !strcmp(oper, "MAX") || !strcmp(oper, "NMIN") || !strcmp(oper, "NMAX"))
	{
		if(tab->nbrows > 1) 
			dyntab_sort(tab, !strcmp(oper, "MIN") ? qsort_col0 : !strcmp(oper, "MAX") ? qsort_col0desc :
							 !strcmp(oper, "NMIN") ? qsort_col0i : qsort_col0idesc);
		if(tab->cell)
		{
			tab->cell->Num = 0;
			tab->cell->Line = 0;
			tab->nbrows = 1;
		}
		return 0;
	}

	/* Handle distinct values operation mode */
	if(!strcmp(oper, "DISTINCT"))
	{
		unsigned long i, j;
		for(i = 0; i < tab->nbrows; i++)
			for(j = i + 1; j < tab->nbrows; j ++)
			{
				DynTableCell *v1 = dyntab_cell(tab, i, 0);
				DynTableCell *v2 = dyntab_cell(tab, j, 0);
				if(!STRCMPNUL(v1->txt, v2->txt)) dyntab_del_rows(tab, j--, 1);
			}
		return 0;
	}

	/* Handle first value operation mode */
	if(!strcmp(oper, "FIRST"))
	{
		if(!tab->nbcols) return 0;
		tab->nbrows = 1;
		tab->nbcols = 1;
		return 0;
	}

	/* Handle values list operation mode */
	if(!strcmp(oper, "LIST") || !strcmp(oper, "LINES") || !strcmp(oper, "SPACE"))
	{
		unsigned long i;
		char *s = !strcmp(oper, "LIST") ? ", " : !strcmp(oper, "SPACE") ? " ": "\n";
		DynBuffer *buf = NULL;
		for(i = 0; i < tab->nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(tab, i, 0);
			if(i && dynbuf_add(&buf, s, 0, NO_CONV)) return 1;
			if(dynbuf_add(&buf, c->txt, c->len, NO_CONV)) return 1;
		}
		dyntab_free(tab);
		return !buf ? 0 : dyntab_add(tab, 0, 0, buf->data, buf->cnt, NO_CONV);
	}

	/* Handle count operation mode */
	if(!strcmp(oper, "COUNT"))
	{
		unsigned long i = tab->nbrows;
		char s[16];
		dyntab_free(tab);
		return dyntab_add(tab, 0, 0, s, snprintf(add_sz_str(s), "%lu", i), NO_CONV);
	}

	/* Handle count distinct values operation mode */
	if(!strcmp(oper, "COUNT_DISTINCT"))
	{
		unsigned long i;
		char s[16];
		if(dyntab_group(tab, "DISTINCT")) return 1;
		i = tab->nbrows;
		dyntab_free(tab);
		return dyntab_add(tab, 0, 0, s, snprintf(add_sz_str(s), "%lu", i), NO_CONV);
	}

	/* Handle telephone format */
	if(!strcmp(oper, "TEL"))
	{
		char tel[16];
		unsigned long i;
		for(i = 0; i < tab->nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(tab, i, 0);

			/* French numbers format */
			if(!c->txt || *c->txt != '0' || c->len != 10) continue;
			snprintf(add_sz_str(tel), "%.2s %.2s %.2s %.2s %.2s", c->txt, c->txt + 2, c->txt + 4, c->txt + 6, c->txt + 8);
			dyntab_add(tab, i, 0, tel, 14, NO_CONV);
		}
		return 0;
	}
	return 1;
}
