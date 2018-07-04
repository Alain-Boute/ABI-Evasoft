/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_output_planning.c
** Description : handling functions for output controls of type PLANNING
**      Author : Alain BOUTE
**     Created : Aug 30 2007
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Typedef : EVA_planning
** Description : data for planning output
*********************************************************************/
typedef struct _EVA_planning
{
	unsigned long i_ctrl;			/* control index in cntxt->form->ctrl */
	DynTable forms;					/* forms used to select displayed events */
	DynTable filters;				/* conditional expression used to select displayed events */
	DynTable titlefield;			/* expression for events title */
	unsigned long titleform;		/* form used to display events title */
	DynTable colorfield;			/* expression for events color */
	DynTable detailfield;			/* expression for events tooltip */
	ReplaceTable *titleconv;		/* conversion for events title */
	DynTable rowlabel;				/* rows label */
	ReplaceTable *rowtitleconv;		/* conversion for rows title */
	DynTable periodctl;				/* controls defining period */
	DynTable objdates;				/* table of events to display */
	DynTable rows;					/* rows objects & label */
	unsigned long currow;			/* current row in process */
	char datebeg[16];				/* start date */
	unsigned long nb;				/* # of units to display per frame */
	char unit;						/* M=month, days else */
	unsigned long frames;			/* # of period frames to output */
	DynTableCell *we_bgcolor;		/* background color for week ends */
	DynTableCell *today_bgcolor;	/* background color for today */

	/* Frame output temp data */
	int wday;						/* week day of 1st day in frame 0=sunday */
	int curday;						/* column # of today in frame */
	char framebeg[16];				/* frame start date */
	char frameend[16];				/* frame end date */
	int curcol;						/* current column in line processing */
	unsigned long *nextline;		/* list of events (index in objdates) to output on next line (overlapping) */
	unsigned long nbnext;			/* # of events to output on next line (overlapping) */
	int b_nextline;					/* output style for overlapping line if set */
}
	EVA_planning;
	
/*********************************************************************
** Function : output_planning_cell_color
** Description : return planning bg color for a cell
*********************************************************************/
char *output_planning_cell_color(	/* return : background color */
	EVA_planning *pl,				/* in : planning data */
	int col							/* in : column # to output */
){
	/* Handle today */
	if(col == pl->curday) return pl->today_bgcolor ? pl->today_bgcolor->txt : "FF8888";

	/* Handle week end */
	if((pl->wday + col)%7 == 0 || (pl->wday + col)%7 == 6) return pl->we_bgcolor ? pl->we_bgcolor->txt : "DDDDDD";

	return NULL;
}

/*********************************************************************
** Function : output_planning_emptycells
** Description : output empty planning cells
*********************************************************************/
#define ERR_FUNCTION "output_planning_emptycells"
#define ERR_CLEANUP 
int output_planning_emptycells(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_planning *pl,				/* in : planning data */
	int cols						/* in : maximum column to output */
){
	EVA_form *form = cntxt->form;

	while(pl->curcol < cols)
	{
		char *bgcol = output_planning_cell_color(pl, pl->curcol);
		DYNBUF_ADD_STR(form->html, "<td class=PlanningCell");
		if(bgcol) DYNBUF_ADD3(form->html, " bgcolor=#", bgcol, 0, NO_CONV, "");
		DYNBUF_ADD_STR(form->html, "></td>");
		pl->curcol++;
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_planning_event
** Description : output one planning event
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_planning_event"
#define ERR_CLEANUP 
int ctrl_output_planning_event(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_planning *pl,				/* in : planning data */
	unsigned long j					/* in : index of event in pl->objdates */
){
	EVA_form *form = cntxt->form;
	char *dbeg, *dend, fmt = 'D';
	DynTableCell *color, *text, *tooltip, defcolor = { add_sz_str("888888") };
	char opnbtn[128];
	int res, res1;

	/* Check if event is in frame */
	dend = dyntab_val(&pl->objdates, j, 3);
	if(strncmp(pl->framebeg, dend, 8) > 0) RETURN_OK;
	dbeg = dyntab_val(&pl->objdates, j, 2);
	if(strncmp(pl->frameend, dbeg, 8) <= 0) RETURN_OK;

	/* Handle overlapping events */
	res = datetxt_to_age(pl->framebeg, dbeg, "D");
	if(pl->curcol && res < pl->curcol)
	{
		M_REALLOC(unsigned long, pl->nextline, pl->nbnext);
		pl->nextline[pl->nbnext - 1] = j;
		RETURN_OK;
	}

	/* Output empty cells */
	if(output_planning_emptycells(cntxt, pl, res)) STACK_ERROR;

	/* Read event cell text & color */
	fmt = 'D';
	res = datetxt_to_age(strcmp(dbeg, pl->framebeg) > 0 ? dbeg : pl->framebeg, dend, &fmt);
	if(res < 0) res = 0;
	pl->curcol += res + 1;
	color = dyntab_cell(&pl->objdates, j, 5);
	text = dyntab_cell(&pl->objdates, j, 4);
	tooltip = dyntab_cell(&pl->objdates, j, 6);

	/* Output cell header */
	DYNBUF_ADD3_INT(form->html, "<td class=PlanningEvent colspan=", res + 1, " onClick=cb('B");
	DYNBUF_ADD(form->html, opnbtn, build_open_btn_name(cntxt, opnbtn, 0, DYNTAB_TOULRC(&pl->objdates, j, 0), 0, 0), NO_CONV);
	DYNBUF_ADD_STR(form->html, "')");
	if(put_showhelp(cntxt, form->html)) STACK_ERROR;

	/* Output cell tooltip : check if given */
	if(pl->detailfield.nbrows)
	{
		if(tooltip->txt && tooltip->len)
			DYNBUF_ADD3_CELLP(form->html, " helpmsg='", tooltip, HTML_TOOLTIP, "'");
	}
	else
	{
		/* Default tooltip : row title + date */
		DYNBUF_ADD3_CELL(form->html, " helpmsg='", &pl->rows, pl->currow, 1, HTML_TOOLTIP, " : ");
		datetxt_to_format(cntxt, opnbtn, dbeg,"_EVA_FullDay");
		DYNBUF_ADD(form->html, opnbtn, strlen(opnbtn), NO_CONV);
		if(strcmp(dbeg, dend))
		{
			datetxt_to_format(cntxt, opnbtn, dend,"_EVA_FullDay");
			DYNBUF_ADD_STR(form->html, " - ");
			DYNBUF_ADD(form->html, opnbtn, strlen(opnbtn), NO_CONV);
		}
		DYNBUF_ADD_STR(form->html, "'");
	}

	/* Handle cell color */
	if(!(color && color->len) && !(text && text->len)) color = &defcolor;
	if(color && color->len)
	{
		DYNBUF_ADD_STR(form->html, " bgcolor=#");
		DYNBUF_ADD_CELLP(form->html, color, NO_CONV);
	}

	/* Output cell content */
	DYNBUF_ADD_STR(form->html, ">");
	fmt = 'D';
	res1 = datetxt_to_age(dbeg, dend, &fmt);
	if((res >= res1 / 2 || res >= 4))
	{
		if(dynbuf_add(form->html, DYNTAB_VAL_SZ(&pl->objdates, j, 4), pl->titleconv, 1)) RETURN_ERR_MEMORY;
	}
	else
		DYNBUF_ADD_STR(form->html, "&nbsp;");
	DYNBUF_ADD_STR(form->html, "</td>");
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_planning_frame
** Description : output one planning frame
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_planning_frame"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					M_FREE(nextline)
int ctrl_output_planning_frame(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_planning *pl				/* in : planning data */
){
	EVA_form *form = cntxt->form;
	DynTable data = {0};
	unsigned long *nextline = NULL, nbnext = 0;
	time_t t = 0;
	struct tm dt, *dt1;
	unsigned long j;
	int col, cols, mdays;
	char fmt = 'D';
	char month[64];
	char opnbtn[128];

	/* Calc end date for frame & # of days */
	delay_to_datetxt(pl->frameend, pl->framebeg, pl->nb, pl->unit);
	cols = datetxt_to_age(pl->framebeg, pl->frameend, &fmt);
	datetxt_to_time(pl->framebeg, &t, &dt);
	pl->wday = dt.tm_wday;
	pl->curday = datetxt_to_age(pl->framebeg, cntxt->timestamp, &fmt);

	/* Output month title line */
	DYNBUF_ADD_STR(form->html, "<tr>");
	for(col = 0; col < cols; col += mdays)
	{
		time_t t0 = t;
		dt.tm_mon++;
		dt.tm_mday = 1;
		dt.tm_hour = 12;
		t = mktime(&dt);
		dt1 = localtime(&t);
		if(!dt1) break;
		mdays = (t - t0 + 4000) / 86400;
		DYNBUF_ADD3_INT(form->html, "<td class=PlanningMonthCell colspan=", mdays + (col ? 0 : 1), ">");
		DYNBUF_ADD(form->html, month, snprintf(add_sz_str(month), "%s %d",
			dyntab_val(&cntxt->monthlong, (dt1->tm_mon + 11) % 12, 0), dt1->tm_year + 1900 - (dt1->tm_mon ? 0 : 1)), TO_HTML);
		DYNBUF_ADD_STR(form->html, "</td>");
	}
	DYNBUF_ADD_STR(form->html, "</tr>");

	/* Output days title line */
	DYNBUF_ADD3_CELL(form->html, "<tr><td class=PlanningDayCell>", &pl->rowlabel, 0, 0, TO_HTML, "</td>");
	datetxt_to_time(pl->framebeg, &t, &dt);
	for(dt1 = &dt, col = 0; col < cols; col++)
	{
		char txt[2] = {0};
		char *bgcol = output_planning_cell_color(pl, col);
		*txt = (char)toupper(dyntab_val(&cntxt->dayshort, dt1->tm_wday, 0)[0]);
		DYNBUF_ADD_STR(form->html, "<td class=PlanningDayCell width=20");
		if(bgcol) DYNBUF_ADD3(form->html, " bgcolor=#", bgcol, 0, NO_CONV, "");
		DYNBUF_ADD3(form->html, ">", txt, 1, NO_CONV, "<br>");
		DYNBUF_ADD_INT(form->html, dt1->tm_mday);
		DYNBUF_ADD_STR(form->html, "</td>");
		dt.tm_mday++;
		dt.tm_hour = 12;
		t = mktime(&dt);
		dt1 = localtime(&t);
	}
	DYNBUF_ADD_STR(form->html, "<td colspan=32></td></tr>");

	/* Output rows */
	for(pl->currow = 0; pl->currow < pl->rows.nbrows; pl->currow++)
	{
		unsigned long idobj = DYNTAB_TOULRC(&pl->rows, pl->currow, 0);

		/* Locate first event in frame - skip row if none */
		pl->curcol = 0;
		for(j = 0; j < pl->objdates.nbrows; j++)
		{
			if(dyntab_cmp(&pl->objdates, j, 1, &pl->rows, pl->currow, 0)) continue;
			if(strncmp(pl->frameend, dyntab_val(&pl->objdates, j, 2), 8) > 0 &&
				strncmp(pl->framebeg, dyntab_val(&pl->objdates, j, 3), 8) <= 0) break;
		}
		if(j == pl->objdates.nbrows) continue;

		/* Output row title */
		DYNBUF_ADD3_INT(form->html, "<tr><td colspan=", cols + 1, " style='border-bottom:solid black 1px'></td></tr>");
		DYNBUF_ADD3(form->html, "<tr><td class=PlanningRowTitleCell onClick=cb('", opnbtn, build_open_btn_name(cntxt, opnbtn, pl->titleform, idobj, 0, 0), NO_CONV, "')>");
		if(dynbuf_add(form->html, DYNTAB_VAL_SZ(&pl->rows, pl->currow, 1), pl->rowtitleconv, 1)) RETURN_ERR_MEMORY;
		DYNBUF_ADD_STR(form->html, "</td>");

		/* Output events in objdates */
		for(;j < pl->objdates.nbrows && !dyntab_cmp(&pl->objdates, j, 1, &pl->rows, pl->currow, 0); j++)
			if(ctrl_output_planning_event(cntxt, pl, j)) STACK_ERROR;

		/* Output empty cells */
		if(output_planning_emptycells(cntxt, pl, cols)) STACK_ERROR;

		/* Output extra lines for overlapping events */
		while(pl->nbnext)
		{
			/* Store overlapping events list */
			M_FREE(nextline);
			nextline = pl->nextline;
			nbnext = pl->nbnext;
			pl->nextline = NULL;
			pl->nbnext = 0;

			/* Output empty cell under row title */
			DYNBUF_ADD_STR(form->html, "</tr><tr><td class=PlanningRowTitleCell></td>");

			/* Output overlapping events */
			pl->curcol = 0;
			pl->b_nextline = 1;
			for(j = 0; j < nbnext; j++)
				if(ctrl_output_planning_event(cntxt, pl, nextline[j])) STACK_ERROR;

			/* Output empty cells */
			if(output_planning_emptycells(cntxt, pl, cols)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(form->html, "</tr>");
		pl->b_nextline = 0;
	}
	DYNBUF_ADD3_INT(form->html, "<tr><td colspan=", cols + 1, " style='border-bottom:solid black 1px'></td></tr>");
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : qsort_objdates
** Description : qsort callback for pl->objdates - columns 1 (integer), 2 (aplha)
*********************************************************************/
int qsort_objdates(const void *el1, const void *el2)
{
	char *t10 = ((DynTableCell *)el1+1)->txt;
	char *t20 = ((DynTableCell *)el2+1)->txt;
	char *t11 = ((DynTableCell *)el1+2)->txt;
	char *t21 = ((DynTableCell *)el2+2)->txt;
	int i1, i2;

	i1 = t10 ? atoi(t10) : 0;
	i2 = t20 ? atoi(t20) : 0;
	if(i1 != i2) return i1 > i2 ? 1 : -1;

	return strcmp(t11 ? t11 : "", t21 ? t21 : "");
}

/*********************************************************************
** Function : ctrl_read_planning_data
** Description : read data for planning output
*********************************************************************/
#define ERR_FUNCTION "ctrl_read_planning_data"
#define ERR_CLEANUP DYNTAB_FREE(empty); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(cgival); \
					M_FREE(sql); \
					qry_build_free(&flt)
int ctrl_read_planning_data(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_planning *pl				/* in : planning data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + pl->i_ctrl;
	DynTable empty = {0};
	DynTable data = {0};
	DynTable cgival = {0};
	DynBuffer *sql = NULL;
	QryBuild flt = {0};
	QryBuilNode *ndbeg, *ndend, *nrow;
	unsigned long i, idctrl;
	char dateend[16];
	char *dateend_mode = CTRL_ATTR_VAL(DATE_END_MODE);

	/* Parse base filter & forms */
	CTRL_ATTR_TAB(pl->forms, BASE_FORMS);
	CTRL_ATTR_TAB(pl->filters, BASE_FILTERS);
	if(qry_add_filter_forms(cntxt, &flt, &pl->forms, &pl->filters)) STACK_ERROR;

	/* Read start date - 1st in period controls */
	CTRL_ATTR_TAB(pl->periodctl, CTRLTREE);
	if(qry_obj_field(cntxt, &data, DYNTAB_TOULRC(&pl->periodctl, 0, 0), "_EVA_FIELD")) STACK_ERROR;
	if(data.cell && data.cell->txt && form_get_field_values(cntxt, &cgival, data.cell->txt, 0, 0)) STACK_ERROR;
	strcpy(pl->datebeg, (cgival.cell && cgival.cell->txt) ? cgival.cell->txt : datetxt_now());

	/* Read # of units - 2nd in period controls */
	DYNTAB_FREE(cgival);
	if(qry_obj_field(cntxt, &data, DYNTAB_TOULRC(&pl->periodctl, 1, 0), "_EVA_FIELD")) STACK_ERROR;
	if(data.cell && data.cell->txt && form_get_field_values(cntxt, &cgival, data.cell->txt, 0, 0)) STACK_ERROR;
	pl->nb = DYNTAB_TOUL(&cgival);
	if(pl->nb <= 0) pl->nb = 1;

	/* Read period unit - 3rd in period controls */
	DYNTAB_FREE(cgival);
	if(qry_obj_field(cntxt, &data, DYNTAB_TOULRC(&pl->periodctl, 2, 0), "_EVA_FIELD")) STACK_ERROR;
	if(data.cell && data.cell->txt && form_get_field_values(cntxt, &cgival, data.cell->txt, 0, 0)) STACK_ERROR;
	pl->unit = *dyntab_val(&cgival, 0, 0);
	if(!pl->unit) pl->unit = 'D';

	/* Read # of frames - 4th in period controls */
	DYNTAB_FREE(cgival);
	if(qry_obj_field(cntxt, &data, DYNTAB_TOULRC(&pl->periodctl, 3, 0), "_EVA_FIELD")) STACK_ERROR;
	if(data.cell && data.cell->txt && form_get_field_values(cntxt, &cgival, data.cell->txt, 0, 0)) STACK_ERROR;
	pl->frames = DYNTAB_TOUL(&cgival);
	if(pl->frames <= 0) pl->frames = 1;

	/* Add date condition */
	delay_to_datetxt(dateend, pl->datebeg, pl->nb * pl->frames, pl->unit);
	ndbeg = flt.node + flt.nbnode++;
	if(qry_obj_field(cntxt, &ndbeg->field, strtoul(CTRL_ATTR_VAL(CTRL_DATE_BEG), NULL, 10), "_EVA_FIELD")) STACK_ERROR;

	/* Handle end date mode */
	if(!strcmp(dateend_mode, "_EVA_DATE"))
	{
		/* Date field for end date select events on both dates */
		DYNTAB_ADD(&ndbeg->value, 0, 0, dateend, 8, NO_CONV);
		ndbeg->match = SmallerEqual;

		/* Use 2nd node for end date */
		ndend = flt.node + flt.nbnode++;
		if(qry_obj_field(cntxt, &ndend->field, strtoul(CTRL_ATTR_VAL(CTRL_DATE_END), NULL, 10), "_EVA_FIELD")) STACK_ERROR;
		DYNTAB_ADD(&ndend->value, 0, 0, pl->datebeg, 0, NO_CONV);
		ndend->match = LargerEqual;
	}
	else
	{
		/* Events on one day (no duration) : use InBound operator on date node */
		DYNTAB_ADD(&ndbeg->value, 0, 0, pl->datebeg, 8, NO_CONV);
		DYNTAB_ADD(&ndbeg->value, 1, 0, dateend, 8, NO_CONV);
		ndbeg->match = InBound;
		ndend = ndbeg;
	}

	/* Add row node */
	nrow = flt.node + flt.nbnode++;
	idctrl = strtoul(CTRL_ATTR_VAL(CTRL_ROWS), NULL, 10);
	if(qry_obj_field(cntxt, &nrow->field, idctrl, "_EVA_FIELD")) STACK_ERROR;
	if(qry_obj_field(cntxt, &data, idctrl, "_EVA_BASE_FORMS")) STACK_ERROR;
	pl->titleform = DYNTAB_TOUL(&data);

	/* Build query from filter - add date & .Line fields */
	if(qry_build_clauses(cntxt, &flt, 1)) STACK_ERROR;
	DYNBUF_ADD3_INT(&flt.optselect, "TLink", nrow->curjoin, ".IdRelObj AS RowObj,");
	DYNBUF_ADD3_INT(&flt.optselect, "TVal", ndbeg->curjoin, ".TxtValue AS DateBeg,");
	DYNBUF_ADD3_INT(&flt.optselect, "TVal", ndend->curjoin, ".TxtValue AS DateEnd");
	if(qry_build_flt_select(cntxt, &sql, &empty, &flt, 0, 0)) STACK_ERROR;

	/* Execute query - place result in pl->objlist & sort by row & date */
	if(qry_exec_filter(cntxt, &flt, sql->data) || sql_get_table(cntxt, &pl->objdates, 0)) STACK_ERROR;	
	if(!pl->objdates.nbrows) RETURN_OK;
	dyntab_sort(&pl->objdates, qsort_objdates);

	/* Build events title & color */
	if(qry_obj_field(cntxt, &pl->titlefield, strtoul(CTRL_ATTR_VAL(CTRL_DISPLAY), NULL, 10), "_EVA_FIELD,_EVA_SRCFIELD")) STACK_ERROR;
	if(qry_add_table_col(cntxt, &pl->objdates, DYNTAB_VAL_SZ(&pl->titlefield, 0, 0))) STACK_ERROR;
	if(qry_obj_field(cntxt, &pl->colorfield, strtoul(CTRL_ATTR_VAL(CTRL_COLOR), NULL, 10), "_EVA_FIELD,_EVA_SRCFIELD")) STACK_ERROR;
	if(qry_add_table_col(cntxt, &pl->objdates, DYNTAB_VAL_SZ(&pl->colorfield, 0, 0))) STACK_ERROR;

	/* Handle HTML format for events title */
	if(qry_obj_field(cntxt, &data, strtoul(CTRL_ATTR_VAL(CTRL_DISPLAY), NULL, 10), "_EVA_DISPLAYFORMAT")) STACK_ERROR;
	if(strcmp(dyntab_val(&data, 0, 0), "_EVA_HTML")) pl->titleconv = plain_to_html;

	/* Build events tooltip */
	if(qry_obj_field(cntxt, &pl->detailfield, strtoul(CTRL_ATTR_VAL(CTRL_TOOLTIP), NULL, 10), "_EVA_FIELD,_EVA_SRCFIELD")) STACK_ERROR;
	if(qry_add_table_col(cntxt, &pl->objdates, DYNTAB_VAL_SZ(&pl->detailfield, 0, 0))) STACK_ERROR;

	/* Gather rows in a separate table with sorted distinct values */
	if(dyntab_from_tab(&pl->rows, 0, 0, &pl->objdates, 0, 1, pl->objdates.nbrows, 2, 1)) RETURN_ERR_MEMORY;
	dyntab_sort(&pl->rows, qsort_col0i);
	dyntab_group(&pl->rows, "DISTINCT");

	/* Handle rows title & format */
	idctrl = strtoul(CTRL_ATTR_VAL(CTRL_ROW_TITLE), NULL, 10);
	if(qry_obj_field(cntxt, &pl->rowlabel, idctrl, "_EVA_LABEL")) STACK_ERROR;
	if(qry_obj_idfield(cntxt, &data, idctrl, 0)) STACK_ERROR;
	DYNTAB_FIELD(&cgival, &data, FIELD)
	if(!cgival.nbrows) DYNTAB_FIELD(&cgival, &data, SRCFIELD)
	if(cgival.nbrows)
	{
		M_FREE(sql);
		if(qry_eval_sqlexpr_var(cntxt, &sql, cgival.cell->txt, &data, NULL)) STACK_ERROR;
		if(qry_obj_field(cntxt, &data, idctrl, "_EVA_DISPLAYFORMAT")) STACK_ERROR;
		if(strcmp(dyntab_val(&data, 0, 0), "_EVA_HTML")) pl->rowtitleconv = plain_to_html;

		/* Build rows title & sort on labels */
		for(i = 0; i < pl->rows.nbrows; i++)
		{
			if(form_eval_fieldexpr(cntxt, &data, 0, DYNTAB_TOULRC(&pl->rows, i, 0), sql->data, sql->sz, NULL, 0)) CLEAR_ERROR;
			DYNTAB_ADD_CELL(&pl->rows, i, 1, &data, 0, 0);
		}
		dyntab_sort(&pl->rows, qsort_col1);
	}

	/* Read other display parameters */
	pl->we_bgcolor = CTRL_ATTR_CELL(WEEKEND_BGCOLOR);
	pl->today_bgcolor = CTRL_ATTR_CELL(TODAY_BGCOLOR);
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_planning
** Description : output all planning frames
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_planning"
#define ERR_CLEANUP cntxt->sql_trace = sql_trace
int ctrl_output_planning(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	EVA_planning _pl = {0}, *pl = &_pl;
	unsigned long i;
	int sql_trace = cntxt->sql_trace;

	/* Read planning data */
	pl->i_ctrl = i_ctrl;
	if(CTRL_ATTR_CELL(DEBUG_SQL_EXPR)) cntxt->sql_trace = 2;
	if(ctrl_read_planning_data(cntxt, pl)) STACK_ERROR;

	/* Output table header */
	DYNBUF_ADD_STR(form->html, "<table cellpadding=0 cellspacing=0 style='border-collapse:collapse'>");

	/* Output each frame */
	for(i = 0; i < pl->frames; i++)
	{
		if(i) DYNBUF_ADD3_INT(form->html, "<tr><td height=10 colspan=", pl->nb * (pl->unit == 'M' ? 31 : 1) + 10,"></td></tr>");
		delay_to_datetxt(pl->framebeg, pl->datebeg, i * pl->nb, pl->unit);
		if(ctrl_output_planning_frame(cntxt, pl)) STACK_ERROR;	
	}
	DYNBUF_ADD_STR(form->html, "</table>");
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_output_planning
** Description : handles OUTPUT controls of type CALENDAR
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_output_planning"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree)
int ctrl_add_output_planning(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable ctrltree = { 0 };

	/* Read period controls */
	CTRL_ATTR(ctrltree, CTRLTREE);

	switch(form->step)
	{
	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		if(!form->html) RETURN_OK;

		/* Add period controls */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		if(ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
		if(ctrl_put_table_footer(cntxt, ctrl)) STACK_ERROR;

		/* Output planning */
		if(ctrl_output_planning(cntxt, i_ctrl)) CLEAR_ERROR;
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	default:
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
