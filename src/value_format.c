/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : value_format.c
** Description : handling functions to output values in various formats
**      Author : Alain BOUTE
**     Created : Jan 10 2006
**********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : put_value_fmt
** Description : output a formated value in text mode
*********************************************************************/
#define ERR_FUNCTION "put_value_fmt"
#define ERR_CLEANUP  
int put_value_fmt(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer **buf,					/* out : buffer to add text to */
	char *txt, size_t len,				/* in : value to format */
	char *fmt							/* in : format specification */
){
	/* Check params */
	if(!txt || !*txt || !len || !buf) RETURN_OK;
	if(!fmt) fmt = "";

	/* Handle value format */
	if(!strcmp(fmt, "_EVA_AGE"))
	{
		/* Age formats */
		char ageunit = 0;
		char *agelbl = "";
		int age = datetxt_to_age(txt, NULL, &ageunit);
		DYNBUF_ADD_INT(buf, age);
		switch(ageunit)
		{
		case 'Y': agelbl = " an"; break;
		case 'M': agelbl = " mois"; age = 0; break;
		case 'D': agelbl = " jour"; break;
		case 'h': agelbl = " heure"; break;
		case 'm': agelbl = " min"; break;
		case 's': agelbl = " sec"; break;
		default: agelbl = "?"; age = 0;
		}
		DYNBUF_ADD(buf, agelbl, 0, NO_CONV);
		if(age > 1) DYNBUF_ADD_STR(buf, "s");
	}
	else if(!strcmp(fmt, "_EVA_HOUR") && (len == 4 || len == 6 || len == 12 || len == 14))
	{
		/* Hour */
		unsigned long z = len >= 12 ? 8 : 0;
		DYNBUF_ADD(buf, txt + z, 2, NO_CONV);
		DYNBUF_ADD_STR(buf, ":");
		DYNBUF_ADD(buf, txt + z + 2, 2, NO_CONV);
		if(len == 6 || len == 14)
		{
			DYNBUF_ADD_STR(buf, ":");
			DYNBUF_ADD(buf, txt + z + 4, 2, NO_CONV);
		}
	}
	else if(!strcmp(fmt, "_EVA_FILESIZE"))
	{
		/* Human file size */
		DYNBUF_ADD(buf, number_to_format(txt, "_EVA_FILESIZE"), 0, NO_CONV);
	}
	else if(!stricmp(fmt, "_EVA_MONEY"))
	{
		/* Money */
		DYNBUF_ADD(buf, number_to_format(txt, "_EVA_Money"), 0, NO_CONV);
	}
	else if(!stricmp(fmt, "_EVA_NUMBER"))
	{
		DYNBUF_ADD(buf, txt, len, NO_CONV);
	}
	else if(!strcmp(fmt, "_EVA_TELEPHONE"))
	{
		/* Telephone number */
		char tel[32];
		if(!strchr(txt, ' '))
		{
			if(*txt == '0' && len == 10)
			{
				/* French numbers format */
				len = sprintf(tel, "%.2s %.2s %.2s %.2s %.2s", txt, txt + 2, txt + 4, txt + 6, txt + 8);
				txt = tel;
			}
			else if(*txt == '+' && len < 15 && len > 5)
			{
				/* International numbers format */
				size_t l = sprintf(tel, "%.3s", txt);
				size_t cnt = 3;
				while(cnt < len)
				{
					l += sprintf(tel + l, " %.3s", txt + cnt);
					cnt += 3;
				}
				len = l;
				txt = tel;
			}
		}
		DYNBUF_ADD(buf, txt, len, NO_CONV);
	}
	else if(!strncmp(fmt, add_sz_str("_EVA_")) && !datetxt_invalid(txt))
	{
		/* Date formats */
		char date[32];
		datetxt_to_format(cntxt, date, txt, fmt);
		DYNBUF_ADD(buf, date, 0, NO_CONV);
	}
	else 
	{
		DYNBUF_ADD(buf, txt, len, NO_CONV);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
