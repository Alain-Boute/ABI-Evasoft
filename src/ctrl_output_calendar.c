/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_output_calendar.c
** Description : handling functions for output controls of type CALENDAR
**      Author : Alain BOUTE
**     Created : April 26 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Typedef : EVA_calendar
** Description : data for calendar output
*********************************************************************/
typedef struct _EVA_calendar
{
	DynTable filter;				/* conditional expression used to select displayed objects */
	DynTable datefield;				/* date field for placing object matching filter */
	char *datemodifier;				/* date modifier (.dateCr or .DateDel) for date field */
	DynTable titlefield;			/* expressions for object title */
	DynTable detailfield;			/* expressions for object detail */
	DynTable eventbgcolor;			/* background color for events title */
	DynTable openform;				/* form to use to open events */
	DynTable hidden;				/* events are initially hidden if set */
	DynTable objlist;				/* objects displayed on calendar */
	char datecal[16];				/* user input start date */
	char date0[16];					/* actual start date */
	char datebeg[16];				/* 1st displayed date */
	char dateend[16];				/* end date */
	unsigned long cols;				/* # of columns in calendar */
	unsigned long periods;			/* # of periods to output */
	unsigned long maxlines;			/* max # of periods to output */
	unsigned long i_ctrl;			/* control index in cntxt->form->ctrl */
	unsigned long ctldate;			/* bitmask options for date controls */
#define CtlDateInput	1L		/* Date input */
#define CtlUnitInput	2L		/* Format select list input */
#define CtlUpDownArrows	4L		/* Date up & down arrows */
#define CtlEvtSelect	8L		/* Events type selector */
#define CtlMaxLabels	128L	/* Max # of labels input */
	char *datefmt;					/* date format for cell date title */
	size_t datefmt_minsz;			/* minimum size of event date (not output if under) */
	char *unit;						/* display mode : _EVA_YEAR ... */
	char *dateinit;					/* initial start date */
	char *resfield;					/* field to return selected date */
	char *fontsize;					/* events output font size */
	char *wemode;					/* display mode for week ends */
	char *webgcolor;				/* background color for week ends */
	char *datelblbgcolor;			/* background color for date labels */
	char *headerbgcolor;			/* background color for header */
	char *todaybgcolor;				/* background color for today's cell */
	char *todaylblbgcolor;			/* background color for today's cell label */
	unsigned long nbevntstyp;		/* # of selected events types */
}
	EVA_calendar;


/*********************************************************************
** Function : ctrl_add_calendar_input
** Description : handles OUTPUT controls of type CALENDAR
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_calendar_input"
#define ERR_CLEANUP	DYNTAB_FREE(options); \
					DYNTAB_FREE(values); \
					DYNTAB_FREE(cgival); \
					M_FREE(name)
int ctrl_add_calendar_input(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *datebeg,					/* in : start date - no input if "0" */
	char *unit,						/* in : period unit - no input if "0" */
	unsigned long periods			/* in : # of periods - no input if 0 */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable options = {0};
	DynTable values = {0};
	DynTable cgival = {0};
	DynBuffer *name = NULL;
	char datefmt[64] = {0};
	CHECK_HTML_STATUS;

	/* Output # of periods input */
	if(periods)
	{
		CTRL_CGINAMESUBFIELD(&name, NULL, "MAXLABELS");
		cginame_setkeep(cntxt, name->data, 1);
		DYNBUF_ADD3_BUF(html, "<input type=text name='", name, NO_CONV, "' size=2 value=");
		if(periods) DYNBUF_ADD_INT(html, periods);
		DYNBUF_ADD_STR(html, ">");
	}

	/* Output period unit list input */
	if(strcmp(unit, "0"))
	{
		if(!unit || !*unit) unit = "_EVA_YEAR";
		CTRL_CGINAMESUBFIELD(&name, NULL, "DATEUNIT");
		cginame_setkeep(cntxt, name->data, 1);
		DYNTAB_ADD_STR(&options, 0, 1, "Année");
		DYNTAB_ADD_STR(&options, 0, 0, "_EVA_YEAR");
		DYNTAB_ADD_STR(&options, 1, 1, "Mois");
		DYNTAB_ADD_STR(&options, 1, 0, "_EVA_MONTH");
		DYNTAB_ADD_STR(&options, 2, 1, "Sem.");
		DYNTAB_ADD_STR(&options, 2, 0, "_EVA_WEEK");
		DYNTAB_ADD_STR(&options, 3, 1, "Jour");
		DYNTAB_ADD_STR(&options, 3, 0, "_EVA_DAY");
		DYNTAB_ADD_STR(&options, 4, 1, "Heure");
		DYNTAB_ADD_STR(&options, 4, 0, "_EVA_HOUR");
		DYNTAB_ADD(&values, 0, 0, unit, 0, NO_CONV);
		if(put_html_list(cntxt, name, &options, &values, 1, 0, 0, 1, 1, NULL)) STACK_ERROR;
	}

	/* If start date given */
	if(strcmp(datebeg, "0"))
	{
		/* Output period left shift buttons */
		if(periods)
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("P"));
			DYNBUF_ADD_STR(html, " ");
			if(put_html_button(cntxt, name->data, NULL, "_eva_btn_arrowl1_small.gif", "_eva_btn_arrowl1_small_s.gif",
												"Page précédente", 0, 0)) STACK_ERROR;
			DYNBUF_ADD_STR(html, " ");
		}
		CTRL_CGINAMEBTN(&name, NULL, add_sz_str("P1"));
		DYNBUF_ADD_STR(html, " ");
		if(put_html_button(cntxt, name->data, NULL, "_eva_btn_arrowl_small.gif", "_eva_btn_arrowl_small_s.gif",
											"Période précédente", 0, 0)) STACK_ERROR;

		/* Output start date input */
		CTRL_CGINAMESUBFIELD(&name, NULL, "STARTDATE");
		cginame_setkeep(cntxt, name->data, 1);
		datetxt_to_format(cntxt, datefmt, datebeg, NULL);
		DYNBUF_ADD3_BUF(html, " <input type=text name='", name, NO_CONV, "'");
		DYNBUF_ADD3(html, " value='", datefmt, 0, NO_CONV, "'");
		DYNBUF_ADD3_INT(html, " size=", strlen(datefmt) + 1, "");
		if(cntxt->jsenabled && unit[5]!='Y' && unit[5]!='M')
		{
			cntxt->jsCalendarInput = 1;
			DYNBUF_ADD_STR(html, " onClick=scwShowSubmit(this,this)");
		}
		DYNBUF_ADD_STR(html, "> ");

		/* Output period right shift buttons */
		CTRL_CGINAMEBTN(&name, NULL, add_sz_str("F1"));
		if(put_html_button(cntxt, name->data, NULL, "_eva_btn_arrowr_small.gif", "_eva_btn_arrowr_small_s.gif",
											"Période suivante", 0, 0)) STACK_ERROR;
		if(periods)
		{
			DYNBUF_ADD_STR(html, " ");
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("F"));
			if(put_html_button(cntxt, name->data, NULL, "_eva_btn_arrowr1_small.gif", "_eva_btn_arrowr1_small_s.gif",
												"Page suivante", 0, 0)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "</nobr></font>");
	}

	/* Cleanup CGI subfields */
	CTRL_GET_CGI_SUBFIELD("DATEUNIT");
	CTRL_GET_CGI_SUBFIELD("MAXLABELS");
	CTRL_GET_CGI_SUBFIELD("STARTDATE");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_get_calendar_input
** Description : read input controls of a calendar input group
*********************************************************************/
#define ERR_FUNCTION "ctrl_get_calendar_input"
#define ERR_CLEANUP	DYNTAB_FREE(cgival)
int ctrl_get_calendar_input(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	unsigned long ctldate,			/* in : bitmask options for date controls */
	char *datebeg,					/* out : start date */
	char **unit,					/* out : period unit */
	unsigned long *periods			/* out : # of periods */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = {0};
	char *btn = i_ctrl == form->i_ctrl_clic ? CGI_CLICK_BTN_SUBFIELD : "";
	unsigned long period = 1;

	/* Read # of periods */
	if(periods && ctldate & CtlMaxLabels)
	{
		CTRL_GET_CGI_SUBFIELD1("MAXLABELS");
		if(dyntab_sz(&cgival, 0, 0)) *periods = strtoul(dyntab_val(&cgival, 0, 0), NULL, 10);
	}
	else periods = &period;

	/* Read period unit */
	CTRL_GET_CGI_SUBFIELD1("DATEUNIT");
	if(dyntab_sz(&cgival, 0, 0)) *unit = dyntab_val(&cgival, 0, 0);

	/* Read start date */
	if(ctldate & CtlDateInput)
	{
		CTRL_GET_CGI_SUBFIELD1("STARTDATE");
		if(dyntab_sz(&cgival, 0, 0))
			input_to_datetxt(dyntab_val(&cgival, 0, 0), datebeg);
	}

	/* Handle buttons clicks : shift datebeg */
	if((ctldate & CtlUpDownArrows) && *btn && strlen(btn) < 3 && strchr("FP", *btn) && unit && *unit && strlen(*unit) > 7) 
	{
		char date[16];
		char fmt = (*unit)[5];
		size_t tr = fmt == 'Y' ? 4 : fmt == 'M' ? 6 : 8;
		int offset = (*btn == 'P' ? -1 : 1) * (btn[1] ? 1 : *periods) * (fmt == 'W' ? 7 : 1);
		if(fmt == 'W') fmt = 'D';
		delay_to_datetxt(date, datebeg, offset, fmt);

		/* Trick to handle DST changes */
		delay_to_datetxt(datebeg, date, 2, 'h');
		datebeg[tr] = 0;
	}
	
	/* Handle date links : goto clicked date */
	if(!strncmp(btn, add_sz_str("GOYEAR")))
	{
		*unit = "_EVA_YEAR";
		strcpy(datebeg, btn + 6);
	}
	else if(!strncmp(btn, add_sz_str("GOMONTH")))
	{
		*unit = "_EVA_MONTH";
		strcpy(datebeg, btn + 7);
	}
	else if(!strncmp(btn, add_sz_str("GOWEEK")))
	{
		*unit = "_EVA_WEEK";
		strcpy(datebeg, btn + 6);
	}
	else if(!strncmp(btn, add_sz_str("GODAY")))
	{
		*unit = "_EVA_DAY";
		strcpy(datebeg, btn + 5);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_datesel
** Description : output select date button
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_datesel"
#define ERR_CLEANUP M_FREE(name)
int ctrl_output_calendar_datesel(	/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl,				/* in : calendar data */
	char *dstdate					/* in : date link in YYYYMMDD format */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cl->i_ctrl;
	DynBuffer *name = NULL;
	char btname[128];
	CHECK_HTML_STATUS;
	if(!*cl->resfield) RETURN_OK;

	CTRL_CGINAMEBTN1(&name, NULL, btname, snprintf(add_sz_str(btname), "CLOSESETVAL=%s=%s", cl->resfield, dstdate));
	if(put_html_button(cntxt, name->data, NULL, "_eva_ctrl_rel.gif", "_eva_ctrl_rel_s.gif",
									"Sélectionner ce jour", 0, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_datelink
** Description : output link to another date of the calendar
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_datelink"
#define ERR_CLEANUP M_FREE(name)
int ctrl_output_calendar_datelink(	/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl,				/* in : calendar data */
	char *link,						/* in : displayed date link */
	char *dstdate,					/* in : date link in YYYYMMDD format */
	char *unit						/* in : new display mode */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cl->i_ctrl;
	DynBuffer *name = NULL;
	char destdate[16] = {0}, date[64], btname[32];
	size_t tr = *unit == 'Y' ? 4 : *unit == 'M' ? 6 : 8;
	CHECK_HTML_STATUS;

	strncpy(destdate, dstdate, tr);
	CTRL_CGINAMEBTN1(&name, NULL, btname, sprintf(btname, "GO%s%s", unit, destdate));
	DYNBUF_ADD3_BUF(html, "<a href='javascript:cb(\"", name, NO_CONV, "\");'");
	datetxt_to_format(cntxt, date, destdate, "_EVA_FullVarDay");
	DYNBUF_ADD3(html, " title='Cliquez pour afficher ", date, 0, HTML_TOOLTIP, "'>");
	DYNBUF_ADD3(html, "<b>", link, 0, NO_CONV, "</b></a>");
	if(ctrl_output_calendar_datesel(cntxt, cl, dstdate)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_title
** Description : outout main title of a calendar with links for format change & data selectors
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_title"
#define ERR_CLEANUP M_FREE(name)
int ctrl_output_calendar_title(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl				/* in : calendar data */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cl->i_ctrl;
	char printbuf[1024], date[64];
	time_t t = 0;
	struct tm dt;
	DynBuffer *name = NULL;
	CHECK_HTML_STATUS;

	/* Output date parts as button links */
	dynbuf_print2(html, "<td bgcolor=#%s colspan=%lu><table noborder width=100%%><tr><td><font size=+1><b><nobr>",
					cl->headerbgcolor[0] ? cl->headerbgcolor : "DDDDDD", cl->cols);
	datetxt_to_time(cl->date0, &t, &dt);
	if(strncmp(cl->unit, add_sz_str("_EVA_YEAR")))
	{
		if(strcmp(cl->unit, "_EVA_MONTH"))
		{
			datetxt_to_format(cntxt, date, cl->date0, "_EVA_DayFull");
			DYNBUF_ADD(html, date, 0, TO_HTML);
			DYNBUF_ADD_STR(html, " ");
			if(ctrl_output_calendar_datelink(cntxt, cl, dyntab_val(&cntxt->monthlong, dt.tm_mon, 0), cl->date0, "MONTH")) STACK_ERROR;
		}
		else
			DYNBUF_ADD_CELL(html, &cntxt->monthlong, dt.tm_mon, 0, TO_HTML);
		DYNBUF_ADD_STR(html, " ");
		datetxt_to_format(cntxt, date, cl->date0, "_EVA_Year");
		if(ctrl_output_calendar_datelink(cntxt, cl, date, cl->date0, "YEAR")) STACK_ERROR;
		if(!strcmp(cl->unit, "_EVA_WEEK"))
		{
		}
	}
	else 
	{
		DYNBUF_ADD3(html, "Année ", cl->date0, 4, TO_HTML, "");
		if(cl->date0[5])
			DYNBUF_ADD3_INT(html, " - ", dt.tm_year + 1901, "");
	}

	/* Output period selector */
	DYNBUF_ADD_STR(html, "</b></font><img src='../img/_eva_nop.gif' width=20></nobr> <nobr>");
	if(cl->ctldate & CtlUnitInput) DYNBUF_ADD_STR(html, "Format ");
	if(ctrl_add_calendar_input(cntxt, cl->i_ctrl,
								(cl->ctldate & CtlDateInput) ? cl->datecal : "0",
								(cl->ctldate & CtlUnitInput) ? cl->unit : "0", cl->periods)) STACK_ERROR;

	/* Output events selector */
	if(cl->filter.nbrows > 1 && cl->ctldate & CtlEvtSelect)
	{
		unsigned long i;
		char status[32] = {0};
		for(i = 0; i < cl->filter.nbrows && i < sizeof(status)-2; i++)
		{
			/* Output table header */
			DynTableCell *c = dyntab_cell(&cl->filter, i, 0);
			char *im, *im1;
			if(i % 2 == 0) DYNBUF_ADD_STR(html, "</td><td>")
			DYNBUF_ADD_STR(html, "<table noborder");
			if(dyntab_sz(&cl->eventbgcolor, i, 0))
				DYNBUF_ADD3_CELL(html, " bgcolor=#", &cl->eventbgcolor, i, 0, NO_CONV, "");
			DYNBUF_ADD_STR(html, " cellspacing=0 cellpadding=0 width=100%><tr><td><font size=-2>");

			/* Output checkbox & label for event type */
			im = "_eva_smallcheckbox_no.gif";
			im1 = "_eva_smallcheckbox_yes.gif";
			if(c->col) { char *t = im; im = im1; im1 = t; }
			CTRL_CGINAMEBTN1(&name, NULL, printbuf, sprintf(printbuf, "XOREVENT%lu", i));
			snprintf(add_sz_str(printbuf), "Les fiches [%s] sont %s\n\nCliquez pour les %s", dyntab_val(&cl->filter, i, 1),
												c->col ? "visibles" : "masquées", c->col ? "masquer" : "afficher");
			if(put_html_button(cntxt, name->data, dyntab_val(&cl->filter, i, 1), im, im1, printbuf, 0, 12)) STACK_ERROR;

			/* Output single event type button */
			im = "_eva_btn_1.gif";
			im1 = "_eva_btn_1_s.gif";
			if(cl->nbevntstyp == 1 && c->col) { char *t = im; im = im1; im1 = t; }
			CTRL_CGINAMEBTN1(&name, NULL, printbuf, sprintf(printbuf, "SELEVENT%lu", i));
			snprintf(add_sz_str(printbuf), cl->nbevntstyp == 1 && c->col ? "Seules les fiches [%s] sont visibles\n\nCliquez pour voir toutes les fiches" :
										"Voir uniquement les fiches [%s]", dyntab_val(&cl->filter, i, 1));
			if(put_html_button(cntxt, name->data, NULL, im, im1, printbuf, 0, 4)) STACK_ERROR;

			/* Output table footer & handle status */
			DYNBUF_ADD_STR(html, "</td></tr></table>")
			status[i] = (char)(c->col ? '1' : '0');
		}

		/* Output hidden input for status */
		CTRL_CGINAMESUBFIELD(&name, NULL, "EVENTS");
		DYNBUF_ADD3_BUF(html, "<input type=hidden name='", name, NO_CONV, "'");
		DYNBUF_ADD3(html, " value=", status, 0, NO_CONV, ">");
	}
	DYNBUF_ADD_STR(html, "</td></tr></table></td></tr><tr>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_cell
** Description : output list of objects for the given period
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_cell"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					DYNTAB_FREE(title); \
					DYNTAB_FREE(detail)
int ctrl_output_calendar_cell(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl,				/* in : calendar data */
	unsigned long *i0,				/* in/out : current index in objlist */
	char *dateend					/* in : end date for the cell */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + cl->i_ctrl;
	DynTable data = {0};
	DynTable title = {0};
	DynTable detail = {0};
	unsigned long i, j, curobj = DYNTAB_TOUL(&form->id_obj);
	char printbuf[1024], date[64];
	CHECK_HTML_STATUS;

	for(i = *i0, j = 0; i < cl->objlist.nbrows; i++, j++)
	{
		/* Output row header */
		DynTableCell *c = dyntab_cell(&cl->objlist, i, 1);
		unsigned long line = DYNTAB_TOULRC(&cl->objlist, i, 2);
		DynTableCell *titlefield = dyntab_cell(&cl->titlefield, line, 0);
		DynTableCell *detailfield = dyntab_cell(&cl->detailfield, line, 0);
		DynTableCell *eventbgcolor = dyntab_cell(&cl->eventbgcolor, line, 0);
		unsigned long openform = DYNTAB_TOULRC(&cl->openform, line, 0);
		int b_button = !strcmp(cl->unit, "_EVA_DAY") || eventbgcolor == NULL;
		char *bgcolor = table_row_bgcolor(cntxt, ctrl->objtbl, i, NULL);
		char *objtype = dyntab_val(&cl->filter, line, 1);
		unsigned long idobj = DYNTAB_TOULRC(&cl->objlist, i, 0);
		if(!c || !c->txt || strcmp(c->txt, dateend) >= 0) break;
		if(!bgcolor || !*bgcolor) bgcolor = "FFFFFF";

		/* Strip to max # of objects per cell */
		if(!cl->maxlines || j < cl->maxlines)
		{
			/* Output open button */
			unsigned long line = DYNTAB_TOULRC(&cl->objlist, i, 3);
			unsigned long k;
			DYNBUF_ADD_STR(html, "<tr>");
			if(idobj != curobj && qry_obj_field(cntxt, &data, idobj, NULL)) STACK_ERROR;
			if(!b_button || !strcmp(cl->unit, "_EVA_DAY"))
			{
				DYNBUF_ADD_STR(html, "<td");
				if(eventbgcolor) DYNBUF_ADD3_CELLP(html, " width=3 bgcolor=#", eventbgcolor, NO_CONV, "");
				DYNBUF_ADD_STR(html, ">");
				if(eventbgcolor) DYNBUF_ADD_STR(html, "&nbsp;");
				DYNBUF_ADD_STR(html, "</td>");
			}
			if(b_button && 
				ctrl_add_symbol_btn(cntxt, ctrl, NULL, idobj == curobj ? & form->objdata : &data, 0, bgcolor, "SYMBOL+OBJNOTES"))
					STACK_ERROR;

			/* Output object title */
			if(titlefield && titlefield->txt && 
				form_eval_fieldexpr(cntxt, &title, 0, idobj, titlefield->txt, titlefield->len, idobj == curobj ? NULL : &data, 0))
					CLEAR_ERROR;
			if(detailfield && detailfield->txt && 
				form_eval_fieldexpr(cntxt, &detail, 0, idobj, detailfield->txt, detailfield->len, idobj == curobj ? NULL : &data, 0))
					CLEAR_ERROR;
			if(cl->datefmt && c && c->len >= cl->datefmt_minsz)
				datetxt_to_format(cntxt, date, c->txt, cl->datefmt);
			else if(!eventbgcolor && !b_button) snprintf(add_sz_str(date), "%s",	objtype);
			else *date = 0;
			dynbuf_print2(html, "<td bgcolor=#%s><font size=%s>", bgcolor, cl->fontsize);
			if(*date) dynbuf_print(html, "<b>%s</b> ", date);
			dynbuf_print4(html, "<a href='javascript:ol(%lu,%lu,%lu,%lu);'", idobj, openform,
							DYNTAB_TOUL(&cntxt->form->id_form),
							DYNTAB_TOUL(&cntxt->form->id_obj));
			k = detail.nbrows ? detail.nbrows - 1 : 0; while(k && dyntab_cell(&detail, k, 0)->Line != line) k--;
			DYNBUF_ADD3(html, " title='", objtype, 0, HTML_TOOLTIP, " ");
			datetxt_to_format(cntxt, date, c->txt, "_EVA_FullVarDay");
			DYNBUF_ADD(html, date, 0, HTML_NO_QUOTE);
			DYNBUF_ADD3_CELL(html, "\n\n", &detail, k, 0, HTML_TOOLTIP, "'");
			if(put_showhelp(cntxt, html)) STACK_ERROR;
			DYNBUF_ADD_STR(html, ">");
			k = title.nbrows ? title.nbrows - 1 : 0; while(k && dyntab_cell(&title, k, 0)->Line != line) k--;
			DYNBUF_ADD3_CELL(html, "", &title, k, 0, TO_HTML, "</a></font></td></tr>");
		}
	}
	if(cl->maxlines && j > cl->maxlines)
		DYNBUF_ADD3_INT(html, "<tr><td colspan=2><font size=-2>(<b>...</b>+", j - cl->maxlines, ")</font></td></tr>");
	*i0 = i;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : date_minimize
** Description : reduce date length to its minimal length
*********************************************************************/
void date_minimize(
	 char *d						/* in/out : date to minimize */
){
	size_t sz = d ? strlen(d) : 0;
	if(sz < 5) return;
	if(sz > 12 && !strcmp(d + 12, "00")) d[12] = 0;
	if(sz > 10 && !strcmp(d + 10, "00")) d[10] = 0;
	if(sz > 8 && !strcmp(d + 8, "00")) d[8] = 0;
	if(sz > 6 && !strcmp(d + 6, "01")) d[6] = 0;
	if(!strcmp(d + 4, "01")) d[4] = 0;
}

/*********************************************************************
** Function : ctrl_output_calendar_dayhours
** Description : output dayly calendar for one day
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_dayhours"
#define ERR_CLEANUP if(szini == (*html)->cnt) dynbuf_add(html, add_sz_str("<td>&nbsp;</td>"), NO_CONV)
int ctrl_output_calendar_dayhours(	/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl,				/* in : calendar data */
	unsigned long *i0,				/* in/out : current index in objlist */
	char *h0,						/* in : first hour to output */
	char *h1,						/* in : last hour to output */
	char *dateday					/* in : day for the cell */
){
	unsigned long cnt = *i0;
	char date0[16], date1[16], d[16];
	size_t sz = strlen(dateday);
	size_t szini;
	CHECK_HTML_STATUS;
	szini = (*html)->cnt;
 
	/* Normalize dateday */
	strcpy(d, dateday);
	if(sz < 6) strcpy(d + 4, "0101");
	else if(sz < 8)	strcpy(d + 6, "01");

	/* Set date range */
	snprintf(add_sz_str(date0), "%s%s", d, h0);
	date_minimize(date0);
	snprintf(add_sz_str(date1), "%s%s", d, h1);
	date_minimize(date1);

	/* Reach first event in range */
	while(*i0 < cl->objlist.nbrows)
	{
		char *datenext = dyntab_val(&cl->objlist, *i0, 1);
		if(strcmp(datenext, date1) >= 0) RETURN_OK;
		if(strcmp(datenext, date0) >= 0) break;
		(*i0)++; 
	}

	/* Output calendar cell */
	if(cnt < *i0) DYNBUF_ADD_STR(html, "</tr><tr>");
	if(ctrl_output_calendar_cell(cntxt, cl, i0, date1)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_year
** Description : output yearly calendar
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_year"
#define ERR_CLEANUP 
int ctrl_output_calendar_year(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl				/* in : calendar data */
){
	unsigned long cnt = 0;
	char date0[16], date1[16], date[128];
	unsigned long i, j;
	time_t t = 0;
	struct tm dt;
	int year, month;
	CHECK_HTML_STATUS;

	/* Set default parameters */
	cl->cols = 12;
	if(!*cl->fontsize) cl->fontsize = "-2";

	/* Output title */
	if(ctrl_output_calendar_title(cntxt, cl)) STACK_ERROR;

	/* Output each month in year */
	if(sscanf(cl->datebeg, "%4u%2u", &year, &month) < 2) month = 1;
	DYNBUF_ADD_STR(html, "<tr>");
	for(i = 0; i < 12; i++)
	{
		/* Output calendar cell */
		if(i && i % 6 == 0) DYNBUF_ADD_STR(html, "</tr><tr>");
		if(month != 1) sprintf(date0, "%04i%02lu", year, month);
		else sprintf(date0, "%04i", year);
		DYNBUF_ADD_STR(html, "<td valign=top width=16% height=100>"
							 "<table border=1 rules=rows width=100% cellspacing=0 cellpadding=0><tr>");
		DYNBUF_ADD3(html, "<td colspan=2 bgcolor=#", cl->datelblbgcolor[0] ? cl->datelblbgcolor : "DDDDDD", 0, TO_HTML, ">");
		if(ctrl_output_calendar_datelink(cntxt, cl, dyntab_val(&cntxt->monthlong, month - 1, 0), date0, "MONTH")) STACK_ERROR;
		if(month == 1 && cl->date0[5]) DYNBUF_ADD3_INT(html, "&nbsp;<b>", year, "</b>");
		DYNBUF_ADD_STR(html, "</td>");

		/* Output each day in month */
		for(j = 0; j < 31; j++)
		{
			if(j) sprintf(date0, "%04i%02lu%02lu", year, month, j + 1);
			datetxt_to_time(date0, &t, &dt);
			if(dt.tm_mon != month - 1) break;
			sprintf(date1, "%04i%02lu%02lu", year, month, j + 2);
			if(cl->objlist.nbrows <= cnt || strcmp(date1, dyntab_val(&cl->objlist, cnt, 1)) <= 0) continue;
			DYNBUF_ADD3(html, "</tr><tr><td align=center bgcolor=#", cl->datelblbgcolor[0] ? cl->datelblbgcolor : "DDDDDD", 0, TO_HTML, ">");
			snprintf(add_sz_str(date), "<font size=-2>%s</font><br><font size=-1>%d",
										dyntab_val(&cntxt->dayshort, dt.tm_wday, 0),
										dt.tm_mday);
			if(ctrl_output_calendar_datelink(cntxt, cl, date, date0, "DAY")) STACK_ERROR;
			DYNBUF_ADD_STR(html, "</td><td><table noborder width=100% cellspacing=0><tr>");
				if(ctrl_output_calendar_dayhours(cntxt, cl, &cnt, "00", "9", date0)) STACK_ERROR;
			DYNBUF_ADD_STR(html, "</tr></table></td>");
		}
		DYNBUF_ADD_STR(html, "</tr></table></td>");

		/* Handle year change */
		month += 1;
		if(month > 12)
		{
			month = 1;
			year += 1;
		}
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_month
** Description : output monthly calendar
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_month"
#define ERR_CLEANUP 
int ctrl_output_calendar_month(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl				/* in : calendar data */
){
	unsigned long cnt = 0;
	char date0[16], date1[16], date[64];
	unsigned long i, j;
	time_t t = 0;
	struct tm dt;
	int b_wegroup = !*cl->wemode;
	int month;
	CHECK_HTML_STATUS;

	/* Output title && day labels */
	cl->cols = (b_wegroup ? 6 : 7);
	if(!*cl->fontsize) cl->fontsize = "-2";
	cl->datefmt_minsz = 10;
	if(ctrl_output_calendar_title(cntxt, cl)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "<tr>");
	for(i = 0; i < (unsigned long)(b_wegroup ? 5 : 7); i++)
	{
		DYNBUF_ADD3(html, "<td align=center width=14% bgcolor=#", cl->datelblbgcolor, 0, TO_HTML, ">");
		DYNBUF_ADD3_CELL(html, "<b>&nbsp;", &cntxt->daylong, i < 6 ? i + 1 : 0, 0, TO_HTML, "&nbsp;</b></td>");
	}
	if(b_wegroup) DYNBUF_ADD3(html, "<td align=center width=14% bgcolor=#", cl->datelblbgcolor, 0, TO_HTML, ">"
									"<font size=-1><b>samedi - dimanche</b></font></td>");
	DYNBUF_ADD_STR(html, "</tr><tr>");

	/* Output each day in month */
	strcpy(date0, cl->datebeg);
	datetxt_to_time(cl->date0, &t, &dt);
	month = dt.tm_mon;
	for(i = 0; strcmp(date0, cl->dateend) <= 0; i++)
	{
		time_t t1 = 0;
		struct tm dt1;
		char *bgcolor = NULL;
		int b_today;

		/* Output row change if applicable : 1 line per week */
		datetxt_to_time(date0, &t, &dt);
		if(strcmp(date0, cl->datebeg) && dt.tm_wday == 1)
			DYNBUF_ADD_STR(html, "</tr><tr>");

		/* Build end date for cell */
		delay_to_datetxt(date1, cl->datebeg, i + 1, 'D');
		datetxt_to_time(date1, &t1, &dt1);
		date1[8] = 0;
		b_today = !strncmp(date0, cntxt->timestamp, 8) ||
					!date0[4] && !strncmp(date0, cntxt->timestamp, 4) && !strncmp("0101", cntxt->timestamp + 4, 4) ||
					!date0[6] && !strncmp(date0, cntxt->timestamp, 6) && !strncmp("01", cntxt->timestamp + 6, 2) ||
					b_wegroup && dt.tm_wday == 6 && !strncmp(date1, cntxt->timestamp, 8);

		/* Output calendar cell header */
		DYNBUF_ADD_STR(html, "<td valign=top height=60");
		if(cl->todaybgcolor[0] && b_today)
			/* Handle cell background : today */
			bgcolor = cl->todaybgcolor;
		else if(cl->webgcolor[0] && strcmp(cl->wemode, "_EVA_NONE") && (
					dt.tm_wday == 6 && strcmp(cl->wemode, "_EVA_SUNDAY") ||
					dt.tm_wday == 0)) 
			/* Handle cell background : week end */
			bgcolor = cl->webgcolor;
		if(bgcolor) DYNBUF_ADD3(html, " bgcolor=#", bgcolor, 0, NO_CONV, "");
		DYNBUF_ADD_STR(html, ">");

		/* Output header for table of events & cell label : date for the day & next day if we group */
		DYNBUF_ADD_STR(html, "<table noborder width=100% cellspacing=0><tr>");
		DYNBUF_ADD3(html, "<td colspan=2 bgcolor=#", b_today ? cl->todaylblbgcolor : cl->datelblbgcolor, 0, TO_HTML, ">");
		for(j = 0; j < 2; j++)
		{
			char *d = j ? date1 : date0;
			struct tm *cdt = j ? &dt1 : &dt;
			b_today = !strncmp(d, cntxt->timestamp, 8) ||
							b_wegroup && dt.tm_wday == 6 && !strncmp(date1, cntxt->timestamp, 8);
			datetxt_to_format(cntxt, date, d, b_today ? "_EVA_DayFull" : "_EVA_Day");
			DYNBUF_ADD3(html, "<font size=", !strncmp(d, cntxt->timestamp, 8) ? "+1" : (b_wegroup && dt.tm_wday == 6) ? "-1" : "+0", 0, NO_CONV, ">");
			if(ctrl_output_calendar_datelink(cntxt, cl, date, d, "DAY")) STACK_ERROR;
			DYNBUF_ADD_STR(html, "</font>");
			if(month != cdt->tm_mon)
				DYNBUF_ADD3_CELL(html, "<font size=-2> ", &cntxt->monthshort, cdt->tm_mon, 0, NO_CONV, "</font>");
			if(j) break;
			if(!b_wegroup || dt.tm_wday != 6 || strcmp(date1, cl->dateend) > 0) break;
			i++;
			DYNBUF_ADD_STR(html, " - ");
		}
		DYNBUF_ADD_STR(html, "</td></tr><tr>");

		/* Output calendar cell contents for the day & next day if we group */
		cl->datefmt = (b_wegroup && dt.tm_wday == 6) ? "_EVA_DayTime" : "_EVA_Time";
		if(ctrl_output_calendar_dayhours(cntxt, cl, &cnt, "00", "9", date0)) STACK_ERROR;
		if(b_wegroup && dt.tm_wday == 6)
		{
			if(ctrl_output_calendar_dayhours(cntxt, cl, &cnt, "00", "9", date1)) STACK_ERROR;
			delay_to_datetxt(date1, cl->datebeg, i + 1, 'D');
		}
		DYNBUF_ADD_STR(html, "</tr></table></td>");

		strcpy(date0, date1);
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_read_calendar_times
** Description : read calendar break times for morning, midday, afternnon & evening
*********************************************************************/
void ctrl_read_calendar_times(
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl,				/* in : calendar data */
	char **h						/* out : time table filled with calendar parameter */
){
	EVA_ctrl *ctrl = cntxt->form->ctrl + cl->i_ctrl;
	h[0] = "00";
	h[1] = CTRL_ATTR_VAL(TIME_MIDDAY);
	if(!h[1][0]) h[1] = "12";
	h[2] = CTRL_ATTR_VAL(TIME_AFTERNOON);
	if(!h[2][0]) h[2] = "14";
	h[3] = CTRL_ATTR_VAL(TIME_EVENING);
	if(!h[3][0]) h[3] = "19";
	h[4] = "9";
}

/*********************************************************************
** Function : ctrl_output_calendar_week
** Description : output weekly calendar
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_week"
#define ERR_CLEANUP 
int ctrl_output_calendar_week(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl				/* in : calendar data */
){
	char date0[16], date1[16], date[64], printbuf[1024];
	unsigned long i, j;
	char *labels[] = { "M<br>a<br>t<br>i<br>n", "M<br>i<br>d", "A<br>.<br>M<br>.", "S<br>o<br>i<br>r" };
	char *h[5];
	unsigned long cnt[] = { 0, 0, 0, 0 };
	int b_wegroup = !*cl->wemode;
	int b_today;
	CHECK_HTML_STATUS;

	/* Read calendar times */
	ctrl_read_calendar_times(cntxt, cl, h);

	/* Output title & day labels */
	cl->cols = 8;
	if(!*cl->fontsize) cl->fontsize = "-2";
	cl->datefmt = "_EVA_Time";
	cl->datefmt_minsz = 10;
	if(ctrl_output_calendar_title(cntxt, cl)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "<td></td>");
	for(j = 0; j < 7; j++)
	{
		delay_to_datetxt(date0, cl->datebeg, j, 'D');
		delay_to_datetxt(date1, cl->datebeg, j + 1, 'D');
		b_today = !strncmp(date0, cntxt->timestamp, 8) ||
						b_wegroup && j == 6 && !strncmp(date1, cntxt->timestamp, 8);
		if(j < 6 || ! b_wegroup)
			dynbuf_print3(html, "<td width=%d%% bgcolor=#%s>%s<nobr>",
								(100 / (b_wegroup ? 6 : 7)) + 1,
								b_today ? cl->todaylblbgcolor : cl->datelblbgcolor,
								(j == 5 && b_wegroup) ? "<font size=-1>" : "");
		datetxt_to_format(cntxt, date, date0, (j >= 5 && b_wegroup && strncmp(date0, cntxt->timestamp, 8)) ? "_EVA_DayLong" : "_EVA_DayFull");
		if(ctrl_output_calendar_datelink(cntxt, cl, date, date0, "DAY")) STACK_ERROR;
		if(j != 5 || ! b_wegroup)
			DYNBUF_ADD_STR(html, "</td>")
		else
			DYNBUF_ADD_STR(html, "</nobr> <nobr>");
	}

	/* Output days parts by rows */
	for(i = 0; i < sizeof(labels)/sizeof(labels[0]); i++)
	{
		/* Output part header */
		dynbuf_print3(html, "</tr><tr><td bgcolor=#%s height=%d align=center><font size=-1><b>%s</b></td>",
							cl->datelblbgcolor, 
							i & 1 ? 100 : 50,
							labels[i]);

		/* Output each day in week */
		for(j = 0; j < 7; j++)
		{
			char *bgcolor = NULL;

			/* Define period */
			delay_to_datetxt(date0, cl->datebeg, j, 'D');
			date0[8] = 0;
 			delay_to_datetxt(date1, cl->datebeg, j + 1, 'D');
			b_today = !strncmp(date0, cntxt->timestamp, 8) ||
							b_wegroup && j == 6 && !strncmp(date1, cntxt->timestamp, 8);

			/* Output calendar cell */
			if(j < 6 || ! b_wegroup)
			{
				DYNBUF_ADD_STR(html, "<td valign=top");
				if(b_today && cl->todaybgcolor[0])
					/* Handle cell background : today */
					bgcolor = cl->todaybgcolor;
				else if(cl->webgcolor[0] && strcmp(cl->wemode, "_EVA_NONE") && (
							j == 5 && strcmp(cl->wemode, "_EVA_SUNDAY") ||
							j == 6)) 
					/* Handle cell background : week end */
					bgcolor = cl->webgcolor;
				if(bgcolor) DYNBUF_ADD3(html, " bgcolor=#", bgcolor, 0, NO_CONV, "");
				DYNBUF_ADD_STR(html, ">");
			}
			DYNBUF_ADD_STR(html, "<table border=0 width=100% cellspacing=0><tr>");
			if(ctrl_output_calendar_dayhours(cntxt, cl, &cnt[i], h[i], h[i + 1], date0)) STACK_ERROR;
			DYNBUF_ADD_STR(html, "</tr></table>");
			if(j != 5 || !b_wegroup)
				DYNBUF_ADD_STR(html, "</td>");
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_day
** Description : output dayly calendar
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_day"
#define ERR_CLEANUP 
int ctrl_output_calendar_day(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl				/* in : calendar data */
){
	unsigned long cnt = 0, i;
	char *labels[] = { "Matin", "Midi", "Après midi", "Soir" };
	char *h[5];
	CHECK_HTML_STATUS;

	/* Read calendar times */
	ctrl_read_calendar_times(cntxt, cl, h);

	/* Output title && day labels */
	cl->cols = sizeof(labels)/sizeof(labels[0]);
	cl->maxlines = 0;
	if(!*cl->fontsize) cl->fontsize = "-1";
	cl->datefmt = "_EVA_Time";
	cl->datefmt_minsz = 10;
	if(ctrl_output_calendar_title(cntxt, cl)) STACK_ERROR;
	for(i = 0; i < cl->cols; i++)
	{
		DYNBUF_ADD3_INT(html, "<td valign=top height=80 width=", i & 1 ? 20 : 16, "%>");
		DYNBUF_ADD3(html, "<table border=0 width=100% cellspacing=0><tr>"
						"<td colspan=3 bgcolor=#", cl->datelblbgcolor, 0, TO_HTML, ">");
		DYNBUF_ADD3(html, "<b>", labels[i], 0, TO_HTML, "</b></td></tr><tr>");
		if(ctrl_output_calendar_dayhours(cntxt, cl, &cnt, h[i], h[i + 1], cl->datebeg)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</tr></table></td>");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar_read
** Description : read data for calendar output
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar_read"
#define ERR_CLEANUP DYNTAB_FREE(empty); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(cgival); \
					M_FREE(sql); \
					qry_build_free(&flt)
int ctrl_output_calendar_read(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl				/* in : calendar data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + cl->i_ctrl;
	DynTable empty = {0};
	DynTable data = {0};
	DynTable cgival = {0};
	DynBuffer *sql = NULL;
	QryBuild flt = {0};
	time_t t = 0;
	struct tm dt;
	unsigned long i;
	char *btn = cl->i_ctrl == form->i_ctrl_clic ? CGI_CLICK_BTN_SUBFIELD : "";

	/* Read calendar parameters */
	CTRL_ATTR_TAB(cl->filter, FILTER);
	CTRL_ATTR_TAB(cl->datefield, DATE_FIELD);
	for(i = 0; i < cl->datefield.nbrows; i++)
	{
		DynTableCell *v = dyntab_cell(&cl->datefield, i, 0);
		char *modif = strrchr(v->txt, '.');
		if(!modif) continue;
		*modif = 0;
		v->len = modif - v->txt;
		v->col = qry_parse_field_modifier(modif + 1);
	}
	CTRL_ATTR_TAB(cl->titlefield, TITLE_FIELD);
	CTRL_ATTR_TAB(cl->detailfield, DETAIL_FIELD);
	if(qry_eval_var_expr_table(cntxt, &cl->titlefield, &ctrl->attr) ||
		qry_eval_var_expr_table(cntxt, &cl->detailfield, &ctrl->attr)) STACK_ERROR;
	CTRL_ATTR_TAB(cl->eventbgcolor, EVENT_BGCOLOR);
	CTRL_ATTR_TAB(cl->openform, OPEN_FORM);
	CTRL_ATTR_TAB(cl->hidden, HIDDEN);

	/* Read selected filters */
	CTRL_GET_CGI_SUBFIELD("EVENTS");
	{
		DynTableCell *status = dyntab_cell(&cgival, 0, 0);
		int b_sel = !strncmp(btn, add_sz_str("SELEVENT")) ? 1 : !strncmp(btn, add_sz_str("XOREVENT")) ? 2 : 0;
		unsigned long i_sel = b_sel ? strtoul(btn + 8, NULL, 10) : 0;
		char *cnt = status && status->txt ? status->txt : "";
		unsigned long nb = 0;
		while(*cnt) { if(*cnt == '1') nb++; cnt++; }
		if(b_sel && nb == 1 && status->txt[i_sel] == '1') b_sel = 3;
		for(i = 0; i < cl->filter.nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(&cl->filter, i, 0);
			c->col = !status ? !dyntab_sz(&cl->hidden, i, 0) : (status->txt && status->len > i && status->txt[i] == '0') ? 0 : 1;
			if(b_sel) switch(b_sel)
			{
			case 1:	c->col = i == i_sel; break;
			case 2:	if(i == i_sel) c->col = !c->col; break;
			default: c->col = 1;
			}
			if(c->col) cl->nbevntstyp++;
		}
	}

	/* Set beginning & end dates depending on format */
	if(strlen(cl->datebeg) < 6) strcpy(cl->datebeg + 4, "01");
	if(strlen(cl->datebeg) < 8) strcpy(cl->datebeg + 6, "01");
	if(!strcmp(cl->unit, "_EVA_DAY"))
	{	/* One day display */
		cl->datebeg[8] = 0;
		delay_to_datetxt(cl->dateend, cl->datebeg, 1, 'D');
		cl->dateend[8] = 0;
		if(!strcmp(cl->datebeg + 6, "01")) cl->datebeg[6] = 0;
		if(!strcmp(cl->datebeg + 4, "01")) cl->datebeg[4] = 0;
	}
	else if(!strcmp(cl->unit, "_EVA_WEEK"))
	{	/* Seven days display */
		cl->datebeg[8] = 0;
		datetxt_to_time(cl->datebeg, &t, &dt);
		if(dt.tm_wday != 1)
		{
			delay_to_datetxt(cl->dateend, cl->datebeg, dt.tm_wday ? 1 - dt.tm_wday : -6, 'D');
			strcpy(cl->datebeg, cl->dateend);
			cl->datebeg[8] = 0;
			strcpy(cl->date0, cl->datebeg);
		}
		delay_to_datetxt(cl->dateend, cl->datebeg, 7, 'D');
		cl->dateend[8] = 0;
		if(!strcmp(cl->datebeg + 6, "01")) cl->datebeg[6] = 0;
		if(!strcmp(cl->datebeg + 4, "01")) cl->datebeg[4] = 0;
	}
	else if(!strcmp(cl->unit, "_EVA_MONTH"))
	{	/* One Month display */
		char tmp[16];
		cl->datebeg[8] = 0;
		sprintf(cl->datebeg + 6, "01");
		datetxt_to_time(cl->datebeg, &t, &dt);
		delay_to_datetxt(cl->dateend, cl->datebeg, 1, 'M');

		/* Start on monday */
		if(dt.tm_wday != 1)
		{
			strcpy(cl->date0, cl->datebeg);
			delay_to_datetxt(tmp, cl->datebeg, dt.tm_wday ? 1 - dt.tm_wday : -6, 'D');
			strcpy(cl->datebeg, tmp);
			cl->datebeg[8] = 0;
		}
		else
			cl->datebeg[6] = 0;

		/* End on sunday */
		datetxt_to_time(cl->dateend, &t, &dt);
		if(dt.tm_wday != 1)
		{
			delay_to_datetxt(tmp, cl->dateend, dt.tm_wday ? 7 - dt.tm_wday : 1, 'D');
			strcpy(cl->dateend, tmp);
			cl->dateend[8] = 0;
		}
		else
			cl->dateend[6] = 0;
	}
	else 
	{	/* Year display */
		int year, month, m_start = atoi(CTRL_ATTR_VAL(START_MONTH));
		if(sscanf(cl->datebeg, "%4u%2u", &year, &month) < 2) month = 1;
		sprintf(cl->date0, "%04i%02i", year - (m_start > month ? 1 : 0), m_start ? m_start : 1);
		strcpy(cl->datebeg, cl->date0);
		delay_to_datetxt(cl->dateend, cl->datebeg, 1, 'Y');
		cl->date0[m_start > 1 ? 6 : 4] = 0;
		cl->dateend[m_start > 1 ? 6 : 4] = 0;
	}

	/* Create list of objects matching filters & date */
	if(sql_exec_query(cntxt, "DROP TABLE IF EXISTS CalendarObjList")) STACK_ERROR;
	if(sql_exec_query(cntxt, "CREATE TEMPORARY TABLE CalendarObjList (IdObj INT, DateVal CHAR(14), Num INT, Line INT) TYPE=HEAP")) STACK_ERROR;
	for(i = 0; i < cl->filter.nbrows; i++) 
	{
		/* Read filter definition - continue if not active */
		QryBuilNode *node;
		DynTableCell *df = dyntab_cell(&cl->datefield, i, 0);
		if(qry_obj_field(cntxt, &data, DYNTAB_TOULRC(&cl->filter, i, 0), NULL)) STACK_ERROR;
		DYNTAB_ADD_CELLP(&cl->filter, i, 1, DYNTAB_FIELD_CELL(&data, LABEL));
		if(!dyntab_cell(&cl->filter, i, 0)->col) continue;

		/* Parse base filter & date condition */
		qry_build_free(&flt);
		if(qry_parse_filter_nodes(cntxt, &flt, &data)) STACK_ERROR;

		/* Add date condition */
		node = flt.node + flt.nbnode++;
		if(dyntab_from_list(&node->field, df->txt, df->len, ",", 0, 1)) RETURN_ERR_MEMORY;
		DYNTAB_ADD(&node->value, 0, 0, cl->datebeg, 0, NO_CONV);
		DYNTAB_ADD(&node->value, 1, 0, cl->dateend, 0, NO_CONV);
		node->modif = df->col;
		node->match = InBound;
		node->srcjoin = 0;

		/* Add matching objects to objects list */
		M_FREE(sql);
		DYNBUF_ADD_STR(&sql, "INSERT INTO CalendarObjList (IdObj,DateVal,Num,Line) \n");
		if(qry_build_clauses(cntxt, &flt, 1)) STACK_ERROR;
		if(node->modif)
		{
			DYNBUF_ADD3_INT(&flt.optselect, "TLink", node->curjoin, ".");
			DYNBUF_ADD(&flt.optselect, node->modif == F_DateDel ? "DateDel" : "DateCr", 0, NO_CONV);
			DYNBUF_ADD_STR(&flt.optselect, " AS DateVal");
		}
		else
			DYNBUF_ADD3_INT(&flt.optselect, "TVal", node->curjoin, ".TxtValue AS DateVal");
		DYNBUF_ADD3_INT(&flt.optselect, ",", i, " AS Num");
		DYNBUF_ADD3_INT(&flt.optselect, ",TLink", node->curjoin, ".Line AS Line");
		if(qry_build_flt_select(cntxt, &sql, &empty, &flt, 0, 0)) STACK_ERROR;
		if(qry_exec_filter(cntxt, &flt, sql->data)) STACK_ERROR;	
	}

	/* Read object / date table */
	if(sql_exec_query(cntxt, "SELECT DISTINCT IdObj,DateVal,Num,Line FROM CalendarObjList ORDER BY DateVal,Num,IdObj") || 
		sql_get_table(cntxt, &cl->objlist, 2)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_calendar
** Description : handles OUTPUT controls of type CALENDAR
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_calendar"
#define ERR_CLEANUP
int ctrl_output_calendar(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_calendar *cl				/* in : calendar data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + cl->i_ctrl;
	DynTableCell *ctldate = CTRL_ATTR_CELL(DATECTL_CALENDAR);
	DynTableCell *dateinit = CTRL_ATTR_CELL(DATE_INIT);
	CHECK_HTML_STATUS;

	/* Set calendar initial date */
	cl->ctldate = ctldate ? strtoul(ctldate->txt, NULL, 10) : CtlDateAll;
	if(ctrl->FIELD && ctrl->FIELD[0] && !(cl->ctldate & CtlDateInput))
	{
		if(form_eval_fieldexpr(cntxt, &ctrl->val, 0, 0, ctrl->FIELD, strlen(ctrl->FIELD), NULL, 0)) CLEAR_ERROR;
		cl->dateinit = dyntab_val(&ctrl->val, 0, 0);
	}
	else if(dateinit)
	{
		if(form_eval_fieldexpr(cntxt, &ctrl->val, 0, 0, dateinit->txt, dateinit->len, NULL, 0)) CLEAR_ERROR;
		cl->dateinit = dyntab_val(&ctrl->val, 0, 0);
	}
	if(!cl->dateinit || strlen(cl->dateinit) < 4) cl->dateinit = cntxt->timestamp;
	strncpy(cl->datebeg, cl->dateinit, 14);

	/* Set other calendar parameters & read data */
	cl->unit = CTRL_ATTR_VAL(DATE_UNIT);
	if(strlen(cl->unit) < 8) cl->unit = "_EVA_MONTH";
	if(ctrl_get_calendar_input(cntxt, cl->i_ctrl, cl->ctldate, cl->datebeg, &cl->unit, &cl->periods)) STACK_ERROR;
	sprintf(cl->datecal, "%.8s", cl->datebeg);
	strcpy(cl->date0, cl->datecal);
	if(!strncmp(cl->unit, add_sz_str("_EVA_YEAR"))) ctrl->BORDER = 0;
	if(ctrl_output_calendar_read(cntxt, cl)) STACK_ERROR;
	cl->wemode = CTRL_ATTR_VAL(WEEKEND);
	cl->webgcolor = CTRL_ATTR_VAL(WEEKEND_BGCOLOR);
	cl->datelblbgcolor = CTRL_ATTR_VAL(DATELABEL_BGCOLOR);
	if(!cl->datelblbgcolor[0]) cl->datelblbgcolor = "CCCCCC";
	cl->headerbgcolor = CTRL_ATTR_VAL(HEADER_BGCOLOR);
	cl->todaybgcolor = CTRL_ATTR_VAL(TODAY_BGCOLOR);
	if(!cl->todaybgcolor[0]) cl->todaybgcolor = cl->datelblbgcolor;
	cl->todaylblbgcolor = CTRL_ATTR_VAL(TODAYLABEL_BGCOLOR);
	if(!cl->todaylblbgcolor[0]) cl->todaylblbgcolor = cl->todaybgcolor;
	cl->maxlines = strtoul(CTRL_ATTR_VAL(HOURCELL_MAXLINES), NULL, 10);
	cl->resfield = CTRL_ATTR_VAL(RETURN_DATE);
	cl->fontsize = CTRL_ATTR_VAL(FONTSIZE);

	/* Use appropriate display mode */
	if(ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;
	if((!strcmp(cl->unit, "_EVA_DAY") ? ctrl_output_calendar_day :
		!strcmp(cl->unit, "_EVA_WEEK") ? ctrl_output_calendar_week :
		!strcmp(cl->unit, "_EVA_MONTH") ? ctrl_output_calendar_month :
									ctrl_output_calendar_year)(cntxt, cl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_output_calendar
** Description : handles OUTPUT controls of type CALENDAR
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_output_calendar"
#define ERR_CLEANUP 
int ctrl_add_output_calendar(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	EVA_calendar cl = {0};

	switch(form->step)
	{
	case CtrlRead:
		if(ctrl->FIELD && ctrl->FIELD[0])
		{
			/* Read control values */
			ctrl->cginame->data[0] = 'D';
			if(ctrl_read_values(cntxt, i_ctrl)) STACK_ERROR;
		}
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Set default parameters */
		if(!CTRL_ATTR_CELL(BORDER)) ctrl->BORDER = 1;
		cl.i_ctrl = i_ctrl;

		/* Output headers & calendar */
		if(table_read_controls(cntxt, i_ctrl, NULL)) STACK_ERROR;
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		if(ctrl_output_calendar(cntxt, &cl)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, "</tr></table>");
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		if(ctrl->FIELD && ctrl->FIELD[0])
		{
			CGI_VALUES_DONTKEEP(&ctrl->cgival);
			if(cl.ctldate & CtlDateInput)
			{
				DYNTAB_ADD(&ctrl->val, 0, 0, cl.datecal, 0, NO_CONV);
				if(ctrl_put_hidden(cntxt, ctrl, 0)) STACK_ERROR;
			}
		}
		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
