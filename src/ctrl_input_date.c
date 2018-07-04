/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_date.c
** Description : handling functions for DATE input controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_read_date
** Description : handles INPUT controls of type DATE
*********************************************************************/
#define ERR_FUNCTION "ctrl_read_date"
#define ERR_CLEANUP	DYNTAB_FREE(cgival)
int ctrl_read_date(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	unsigned long i_ctrl	/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	int age = 0;
	unsigned long i, j;
	DynTable cgival = {0};
	size_t sz, sz1;
	unsigned long idobj = DYNTAB_TOUL(&form->id_obj);
	char *date_checktype = CTRL_ATTR_VAL(DATE_CHECKTYPE);
	int b_duration = *CTRL_ATTR_VAL(DURATION);
	char age_input = *CTRL_ATTR_VAL(INPUT_AGE);
	char age_unit = *CTRL_ATTR_VAL(AGE_UNIT);
	char *btn = CGI_CLICK_BTN_SUBFIELD;
	if(!age_unit) age_unit = 'Y';
	sz1 = !age_input ? 14 : age_unit == 'Y'? 4 : 8;

	/* Handle reference date in age input */
	if(!strcmp(date_checktype, "_EVA_FIELD"))
	{
		DynTableCell *c = CTRL_ATTR_CELL(DATEREF_FIELD);
		if(c && ctrl_eval_fieldexpr(cntxt, ctrl, &cgival, c->txt)) STACK_ERROR;
	}
	else if(!strcmp(date_checktype, "_EVA_DateCr"))
	{
		if(idobj && ctrl_eval_fieldexpr(cntxt, ctrl, &cgival, "MIN(_EVA_FORMSTAMP.DateCr)")) STACK_ERROR;
	}
	if(cgival.nbrows) strncpy(ctrl->dateref, dyntab_val(&cgival, 0, 0), 14);
	if(!ctrl->dateref[0]) strncpy(ctrl->dateref, cntxt->timestamp, 14);
	strncpy(ctrl->ageref, (strcmp(date_checktype, "_EVA_FIELD") &&
							strcmp(date_checktype, "_EVA_DateCr") ||
							CTRL_ATTR_CELL(AGEREF)) ? cntxt->timestamp : ctrl->dateref, 12);

	/* Handle age input status */
	CTRL_GET_CGI_SUBFIELD1("AGE");
	sz = dyntab_sz(&ctrl->val, 0, 0);
	ctrl->status = age_input && (
			(cgival.nbrows ? strcmp(btn, "DATEMODE") : !strcmp(btn, "AGEMODE")) ||
			!ctrl->cgival.nbrows && (age_input == '1' || sz && sz < 5));

	/* Format each value from user input */
	for(i = 0; i < ctrl->val.nbrows; i++)
	{
		DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
		if(c->i_cgi || datetxt_invalid(c->txt))
		{
			/* Store value if modified */
			char date[32] = {0};
			input_to_datetxt(c->txt, date);
			if(c->txt && c->txt[0] && *date && strcmp(date, c->txt))
				DYNTAB_ADD_NL(&ctrl->val, i, 0, date, 0, c->Num, c->Line, c->b_modified || !c->i_cgi);
			ctrl->b_modified |= c->b_modified;
		}
	}

	/* Transform age inputs  */
	for(i = 0; i < cgival.nbrows; i++)
	{
		char date[32] = {0};
		DynTableCell *c = dyntab_cell(&cgival, i, 0);

		/* Retrieve corresponding value in ctrl->val */
		for(j = 0; j < ctrl->val.nbrows; j++)
		{
			DynTableCell *c1 = dyntab_cell(&ctrl->val, j, 0);
			if(c->Num == c1->Num && c->Line == c1->Line) break;
		}

		/* Transform age to date if applicable */
		if(c->txt)
		{
			int b_err, agecur;
			char *val = dyntab_val(&ctrl->val, j, 0);
			char fmt = age_unit;
			sz = dyntab_sz(&ctrl->val, 0, 0);
			agecur = (b_duration ? -1 : 1) * datetxt_to_age(val, ctrl->ageref, &fmt);
			age = atoi(dyntab_val(&cgival, i, 0));
			b_err = delay_to_datetxt(date, ctrl->ageref, (b_duration ? age : -age), age_unit);

			/* Update date value if age changed */
			if(!b_err && (!*val || fmt != age_unit || age != agecur))
			{
				sz = strlen(date);
				DYNTAB_ADD_NL(&ctrl->val, j, 0, date, sz < sz1 ? sz : sz1, c->Num, c->Line, 1);
				ctrl->b_modified = 1;
			}
		}
		else if(dyntab_sz(&ctrl->val, j, 0))
		{
			/* Clear value if applicable */
			DYNTAB_ADD_NL(&ctrl->val, j, 0, NULL, 0, c->Num, c->Line, 1);
			ctrl->b_modified = 1;
		}
	}

	/* Handle control label */
	if(ctrl->status)
	{
		ctrl->LABEL = CTRL_ATTR_VAL(AGE_LABEL);
		if(!ctrl->LABEL[0]) ctrl->LABEL = (*CTRL_ATTR_VAL(DURATION) ? "Durée" : "Age");
	}

	/* Sort values on Line / Num */
	dyntab_sort(&ctrl->val, qsort_ctrlval);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_edtview_date
** Description : output HTML code for controls of type DATE
*********************************************************************/
#define ERR_FUNCTION "ctrl_editview_date"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					M_FREE(name)
int ctrl_editview_date(			/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = {0};
	DynBuffer *name = NULL;
	unsigned long i, k;
	char *input_age = CTRL_ATTR_VAL(INPUT_AGE);
	char *fmt = CTRL_ATTR_VAL(FORMAT);
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);
	unsigned long columns = ctrl->COLUMNS;
	char age_unit = *CTRL_ATTR_VAL(AGE_UNIT);
	char *age_label = CTRL_ATTR_VAL(AGE_LABEL);
	int b_duration = *CTRL_ATTR_VAL(DURATION);
	if(!age_unit) age_unit = 'Y';
	if(!*age_label) age_label = b_duration ? "Durée" : "Age";

	/* Clear existing age inputs */
	CTRL_GET_CGI_SUBFIELD("AGE");

	/* Add HTML code for control label & position */
	if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
	if(*input_age && form->step == HtmlEdit) DYNBUF_ADD_STR(form->html, "<table noborder><tr><td>");

	/* Handle empty value in view mode */
	if(form->step != HtmlEdit && !ctrl->val.nbrows && *ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

	/* Add HTML code for each value */
	k = ctrl->val.nbrows + ((form->step == HtmlEdit && b_multiple && dyntab_sz(&ctrl->val, ctrl->val.nbrows - 1, 0)) ? 1 : 0);
	for(i = 0; !i || i < k; i++)
	{
		char date[64] = {0};
		DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
		int age = 0;
		char unit[8] = {0};
		char selunit = age_unit;
		int b_err = 0;

		/* Separate multiple values with a line break */
		if(i)
		{
			if(*input_age && form->step == HtmlEdit)
				DYNBUF_ADD_STR(form->html, "</td></tr><tr><td>")
			else
				DYNBUF_ADD_STR(form->html, "<br>");
		}

		/* Calc age if applicable */
		if(*input_age)
		{
			/* Compute age - force input in given unit if edit mode */
			if(c) age = datetxt_to_age(c->txt, ctrl->ageref, &selunit);
			b_err = form->step == HtmlEdit && ctrl->status && selunit != age_unit;
			if(b_err) selunit = age_unit;
			strcpy(unit, 
					selunit == 's' ? SECONDE :
					selunit == 'm' ? MINUTE :
					selunit == 'h' ? HOUR :
					selunit == 'D' ? DAY :
					selunit == 'M' ? MONTH : YEAR);
			if(unit[strlen(unit) - 1] != 's' && (b_err && form->step == HtmlEdit || age > 1 || age < -1))
				unit[strlen(unit)] = 's';
		}

		/* Handle control input formats */
		if(ctrl->status)
		{
			/* Age input : add text control with age value */
			ctrl->COLUMNS = 3;
			CTRL_CGINAMESUBFIELD(&name, c, "AGE");
			if(!b_err && c && c->txt) sprintf(date, "%d", (b_duration ? -1 : 1) * age);
			if(ctrl_add_text_value(cntxt, ctrl, i, date, strlen(date), name)) STACK_ERROR;

			/* Add age unit & date button or label */
			DYNBUF_ADD3(form->html, " ", unit, 0, NO_CONV, " ");
			*date = 0;
			if(c) datetxt_to_format(cntxt, date, c->txt, "_EVA_DATETIME");
			if(form->step == HtmlEdit)
			{
				DYNBUF_ADD_STR(form->html, "</td><td>");
				CTRL_CGINAMEBTN(&name, c, add_sz_str("DATEMODE"));
				if(put_html_button(cntxt, name->data,
					*date ? date : CTRL_ATTR_VAL(LABEL), NULL, NULL,
					"Date correspondante\n\nCliquez pour saisir une date", 0, 0)) STACK_ERROR;

				/* Add date value as hidden input */
				if(ctrl_put_hidden(cntxt, ctrl, i)) STACK_ERROR;
			}
			else if(*date)
				DYNBUF_ADD3(form->html, "(", date, 0, NO_CONV, ")");
		}
		else
		{
			/* Standard date input : set appropriate numbers & width if text format in edit mode */
			if(form->step == HtmlEdit)
			{
				fmt = strstr(fmt, "Month") ? "_EVA_StdMonth" :
						strstr(fmt, "Year") ? "_EVA_Year" :
						"_EVA_DATETIME";
			}
			if(c && !datetxt_to_format(cntxt, date, c->txt, fmt))
				ctrl->COLUMNS = strlen(date);
			else if(!*fmt)
				ctrl->COLUMNS = 10;
			else
			{
				char d[64];
				datetxt_to_format(cntxt, d, cntxt->timestamp, fmt);
				ctrl->COLUMNS = strlen(d);
			}

			/* Add date input or value */
			if(cntxt->form->step == HtmlEdit)
			{
				CTRL_CGINAMEVAL(&name, i);
				if(put_html_text_input(cntxt, 
						DYNBUF_VAL_SZ(name),
						date, strlen(date),
						0, *CTRL_ATTR_VAL(JSCALENDAR) ? 4 : 0, 1,
						ctrl->COLUMNS,
						atoi(CTRL_ATTR_VAL(MAXLENGTH))) ||
					ctrl_put_hidden_old(cntxt, ctrl, i, name, date, strlen(date)))
				STACK_ERROR;
			}
			else
				DYNBUF_ADD(cntxt->form->html, date, strlen(date), TO_HTML)

			/* Add age button / text as applicable */
			if(*input_age && (form->step == HtmlEdit || !*fmt))
			{
				sprintf(date, "%d %s", (b_duration ? -age : age), unit);
				if(form->step == HtmlEdit)
				{
					DYNBUF_ADD_STR(form->html, "</td><td>");
					CTRL_CGINAMEBTN(&name, c, add_sz_str("AGEMODE"));
					if(put_html_button(cntxt, name->data,
						(c && c->txt) ? date : age_label, NULL, NULL,
						b_duration ? "Durée correspondant à la date\n\nCliquez pour saisir une durée" :
								"Age correspondant à la date\n\nCliquez pour saisir un âge", 0, 0)) STACK_ERROR;
				}
				else if(c && c->txt)
					DYNBUF_ADD3(form->html, " (", date, 0, NO_CONV, ")");
			}
		}
	}

	/* Add HTML code for the control footer */
	if(*input_age && form->step == HtmlEdit) DYNBUF_ADD_STR(form->html, "</td></tr></table>");
	if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;

	ctrl->COLUMNS = columns;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_date
** Description : read user input values for controls of type DATE
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_date"
#define ERR_CLEANUP
int ctrl_add_date(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i;

	switch(form->step)
	{
	case CtrlRead:
		if(ctrl_read_date(cntxt, i_ctrl)) STACK_ERROR;
		break;

	case InputCheck:
		/* Check each value */
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			/* Get min & max alowed date values */
			char val[16] = {0};
			char datemin[16] = {0}, datemax[32] = {0}, *daymin, *daymax;
			char datebuf[32];
			char *date_checktype = CTRL_ATTR_VAL(DATE_CHECKTYPE);
			strncpy(val, dyntab_val(&ctrl->val, i, 0), sizeof(val)-1);
			if(!*val) continue;
			full_datetxt(val);

			/* Check date is valid */
			if(*val && datetxt_invalid(val))
			{
				ctrl->error = 2;
				if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, "\n");
				DYNBUF_ADD3_CELL(&ctrl->errmsg, "La date [", &ctrl->val, i, 0, NO_CONV, "] n'est pas correcte");
				continue;
			}

			if(!strcmp(date_checktype, "_EVA_FIXED_DATES"))
			{
				/* Fixed dates : simply read values */
				strncpy(datemin, CTRL_ATTR_VAL(MIN_DATE), sizeof(datemin) - 1);
				strncpy(datemax, CTRL_ATTR_VAL(MAX_DATE), sizeof(datemax) - 1);
			}
			else
			{
				/* Dates relative to variable day : get day limits  */
				char fmt = 'D';
				daymax = CTRL_ATTR_VAL(MAX_DATEDIFF);
				daymin = CTRL_ATTR_VAL(MIN_DATEDIFF);

				/* Add given days to reference date */
				if(*daymin) delay_to_datetxt(datemin, ctrl->dateref, atoi(daymin), fmt);
				if(*daymax) delay_to_datetxt(datemax, ctrl->dateref, atoi(daymax), fmt);
				datemin[8] = 0;
				datemax[8] = 0;
			}

			/* Set min / max default values */
			if(!*datemin) strcpy(datemin, "0100");
			if(!*datemax) strcpy(datemax, "99991231");

			/* Check input value validity & min / max */
			if(*datemin && strncmp(datemin, val, strlen(datemin)) > 0 || *datemax && strncmp(datemax, val, strlen(datemax)) < 0)
			{
				ctrl->error = 2;
				if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, "\n");
				if(*datemin && *datemax)
				{
					datetxt_to_format(cntxt, datebuf, datemin, CTRL_ATTR_VAL(FORMAT));
					DYNBUF_ADD3(&ctrl->errmsg, "La date doit être après le ", datebuf, 0, NO_CONV, " et avant le ");
					datetxt_to_format(cntxt, datebuf, datemax, CTRL_ATTR_VAL(FORMAT));
					DYNBUF_ADD(&ctrl->errmsg, datebuf, 0, NO_CONV);
				}
				else if(*datemin)
				{
					datetxt_to_format(cntxt, datebuf, datemin, CTRL_ATTR_VAL(FORMAT));
					DYNBUF_ADD3(&ctrl->errmsg, "La date doit être après le ", datebuf, 0, NO_CONV, "");
				}
				else
				{
					datetxt_to_format(cntxt, datebuf, datemax, CTRL_ATTR_VAL(FORMAT));
					DYNBUF_ADD3(&ctrl->errmsg, "La date doit être avant le ", datebuf, 0, NO_CONV, "");
				}
			}
		}
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		if(!form->html) break;
		/* Add HTML code for control */
		if(ctrl_editview_date(cntxt, i_ctrl)) STACK_ERROR;
		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
