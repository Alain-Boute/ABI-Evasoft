/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : date_utils.c
** Description : date conversion functions
**      Author : Alain BOUTE
**     Created : Feb 15 2002
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Macro : ENSURE_VALID_DATE
** Description : prevent invalid dates like Feb 30
*********************************************************************/
#define ENSURE_VALID_DATE	if(M0 == 2 && D0 > 28) { M0++; D0 -= 28; } \
							else if((M0 == 4 || M0 == 6 || M0 == 9 || M0 == 11) && D0 > 30) { M0++; D0 = 1; }

/*********************************************************************
** Function : time_to_datetxt
** Description : return a YYYYMMDDHHMMSS format date from unix time
*********************************************************************/
int time_to_datetxt(		/* return : 0 on success, other on error */
	time_t t,				/* in : unix time to convert */
	char *valdate			/* out : converted time - must be 16 bytes long minimum */
)
{
	struct tm *dt;

	/* Make date value */
	if(!valdate) return 1;
	*valdate =  0;
	dt = localtime(&t);
	if(!dt) return 1;
	sprintf(valdate, "%04d%02d%02d%02d%02d%02d", 
			1900 + dt->tm_year, dt->tm_mon+1, dt->tm_mday, 
			dt->tm_hour, dt->tm_min, dt->tm_sec);

	return 0;
}

/*********************************************************************
** Function : full_datetxt
** Description : set missing digits for a YYYYMMDDHHMMSS date
*********************************************************************/
void full_datetxt(
	char *date			/* in/out : date - must be 16 bytes long minimum */
){
	size_t sz = date ? strlen(date) : 0;
	if(sz != 4 && sz != 6 && sz != 8 && sz != 10 && sz != 12) return;
	sprintf(date + sz, "%s", "0101000000" + sz - 4);
}

/*********************************************************************
** Function : datetxt_now
** Description : return current time in YYYYMMDDHHMMSS format (static string)
*********************************************************************/
char *datetxt_now()
{
	static char date[16];
	time_t t = time(NULL);

	time_to_datetxt(t, date);

	return date;
}

/*********************************************************************
** Function : date_difference
** Description : compute delay between two dates in various units
*********************************************************************/
int date_difference(									/* return : date difference in given unit */
	int Y0, int M0, int D0, int h0, int m0, int s0,		/* in : 1st date */
	int Y1, int M1, int D1, int h1, int m1, int s1,		/* in : 2nd date */
	char *fmt											/* in/out : same as datetxt_to_age() */
){
	/* Try unix time translation */
	int b_unix;
	struct tm dt;
	time_t t0, t1;
	memset(&dt, 0, sizeof(dt));
	dt.tm_isdst = -1;
	dt.tm_year = Y0 - 1900; dt.tm_mon = M0 - 1; dt.tm_mday = D0;
	dt.tm_hour = h0; dt.tm_min = m0; dt.tm_sec = s0;
	t0 = mktime(&dt);
	dt.tm_year = Y1 - 1900; dt.tm_mon = M1 - 1; dt.tm_mday = D1;
	dt.tm_hour = h1; dt.tm_min = m1; dt.tm_sec = s1;
	t1 = mktime(&dt);
	b_unix = t0 != (time_t)-1 && t1 != (time_t)-1;

	switch(*fmt)
	{
	case 's':
		/* Unix time implementation */
		if(b_unix) return t1 - t0;

		/* Simple implementation for days in same month */
		if(Y0 == Y1 && M0 == M1)
			return (D1 - D0 + (h1 >= h0 ? 0 : 1))*86400 + (h1 - h0 + (m1 >= m0 ? 0 : 1))*3600 + (m1 - m0 + (s1 >= s0 ? 0 : 1))*60 + s1 - s0;

		/* TODO : exact implementation for old dates */
		*fmt = 'm';
	case 'm':
		/* Unix time implementation */
		if(b_unix) return t1/60 - t0/60;

		/* Simple implementation for days in same month */
		if(Y0 == Y1 && M0 == M1)
			return (D1 - D0 + (h1 >= h0 ? 0 : 1))*1440 + (h1 - h0 + (m1 >= m0 ? 0 : 1))*60 + m1 - m0;

		/* TODO : exact implementation for old dates */
		*fmt = 'h';
	case 'h':
		/* Unix time implementation */
		if(b_unix) return (t1 - t0)/3600;

		/* Simple implementation for days in same month */
		if(Y0 == Y1 && M0 == M1)
			return (D1 - D0 + (h1 >= h0 ? 0 : 1))*24 + h1 - h0;

		/* TODO : exact implementation for old dates */
		*fmt = 'D';
	case 'D':
		/* Unix time implementation */
		if(b_unix) return (t1 - t0)/86400;	 

		/* Simple implementation for days in same month */
		if(Y0 == Y1 && M0 == M1) return (D1 - D0);

		*fmt = 'M';
	case 'M':
		return M1 - M0 + (Y1 - Y0) * 12 - (D1 >= D0 ? 0 : 1);
	}
	*fmt = 'Y';
	return Y1 - Y0 - ((M1 > M0 || M1 == M0 && D1 >= D0) ? 0 : 1);
}

/*********************************************************************
** Function : datetxt_to_age
** Description : build age from date
*********************************************************************/
int datetxt_to_age(		/* return : age (signed) in the requested unit */
	char *date,			/* in : date to translate in YYYYMMDDHHMMS format */
	char *now,			/* in : date to compare with in YYYYMMDDHHMMS format */
	char *format		/* in/out : desired age unit - do not use char constants !!! Use a pointer to alloc-ed or stack memory
							in :
							 0  = best fit
							's' = seconds
							'm' = minutes
							'h' = hours
							'D' = days
							'M' = months
							other = years
						   out : used format as above (may be different from input to avoid overflow)
							'E' on error */
){
	int Y0 = 0, M0 = 0, D0 = 0, h0 = 0, m0 = 0, s0 = 0;
	int Y1 = 0, M1 = 0, D1 = 0, h1 = 0, m1 = 0, s1 = 0;
	char fmt = (char)(format ? *format : 0);
	char *base = now ? now : datetxt_now();
	int age = 0;
	int b_done = 0;

	/* Check params - return 0 on error */
	if(format) *format = 'E';
	if(!date || strlen(date) < 4) return 0;
	sscanf(date, "%4d%2d%2d%2d%2d%2d", &Y0, &M0, &D0, &h0, &m0, &s0);
	sscanf(base, "%4d%2d%2d%2d%2d%2d", &Y1, &M1, &D1, &h1, &m1, &s1);
	if(Y0 < 1000 || Y1 < 1000) return 0;
	if(!M0) M0 = 1;
	if(!D0) D0 = 1;
	if(!M1) M1 = 1;
	if(!D1) D1 = 1;

	/* Select best fit if required */
	if(!fmt)
	{
		while(!b_done)  
		{
			char outfmt;
			switch(fmt)
			{
				case 0: fmt = 'Y'; break;
				case 'Y': fmt = 'M'; break;
				case 'M': fmt = 'D'; break;
				case 'D': fmt = 'h'; break;
				case 'h': fmt = 'm'; break;
				default: fmt = 's';	b_done = 1;
			}
			outfmt = fmt;
			age = date_difference(Y0, M0, D0, h0, m0, s0, Y1, M1, D1, h1, m1, s1, &outfmt);
			b_done |= abs(age) > 1 || outfmt != fmt;
			fmt = outfmt;
		}
	}

	/* Compute date difference & assign used format */
	age = date_difference(Y0, M0, D0, h0, m0, s0, Y1, M1, D1, h1, m1, s1, &fmt);
	if(format) *format = fmt;
	return age;
}

/*********************************************************************
** Function : datetxt_invalid
** Description : check if a date is valid
*********************************************************************/
int datetxt_invalid(		/* return : 0 if date valid, other if invalid */
	char *valdate			/* in : date to check (YYYYMMDDHHMMSS format) */
){
	int year = 0, month = 1, day = 1, hour = 0, min = 0, sec = 0, nb;
	char *c = valdate;
	size_t sz;

	/* Empty date is invalid */
	if(!valdate || !*valdate) return 1;

	/* Only digits allowed */
	while(*c) if(*c < '0' || *c > '9') return 1; else c++;

	/* Check length */
	sz = c - valdate;
	if(sz != 4 && sz != 6 && sz != 8 && sz != 10 && sz != 12 && sz != 14) return 1;

	/* Check year / month / day */
	nb = sscanf(valdate, "%4u%2u%2u%2u%2u%2u", &year, &month, &day, &hour, &min, &sec);
	return !nb || year < 100 || !month || month > 12 || !day ||
			day > ((month == 1 || month == 3 || month == 5 || month == 7 ||
					month == 8 || month == 10 || month == 12) ? 31 : month == 2 ? 29 : 30);
}

/*********************************************************************
** Function : input_to_datetxt
** Description : return a YYYYMMDDHHMMSS format date from user input
*********************************************************************/
int input_to_datetxt(		/* return : 0 on success, other on error */
	char *valdate,			/* in : date to convert (user input format) */
	char *date				/* out : converted date - must be 16 bytes long minimum */
)
{
	int num[6], nb, i, i_hour = -1;
	char *txt = valdate;

	if(!date) return 1;
	*date = 0;
	if(!valdate || !*valdate) return 0;
	memset(num, 0, sizeof(num));


	/* Search for up to 6 numbers */
	for(i = 0; i < 6 && *txt; i++)
	{
		int sz;
		while(*txt && *txt < '0' || *txt > '9') txt++;
		nb = sscanf(txt, "%d%n", &(num[i]), &sz);
		if(*txt) txt += nb ? sz : 1;
		if(i_hour < 0 && *txt ==':') i_hour= i;
	}
	nb = i;

	/* Handle early hour separator */
	if(i_hour == 0 || i_hour == 1)
	{
		nb = 1;
		num[3] = num[i_hour];
		num[4] = num[i_hour+1];
		num[5] = num[i_hour+1];
		if(i_hour == 0) sscanf(datetxt_now()+6, "%02d", &num[0]);
	}

	/* Switch on # of numbers input */
	if(nb == 1 && num[0] >= 1 && num[0] <= 31)
	{
		/* One number in range 1-31 : treat as day of month */
		strncpy(date, datetxt_now(), 6);
		sprintf(date+6, "%02d", num[0]);
	}
	else if(nb == 1 && num[0] >= 100 && num[0] <= 9999)
	{
		/* One number in range 100-9999 : treat as year */
		sprintf(date, "%04d", num[0]);
	}
	else if(nb == 1 && num[0] >= 1000)
	{
		/* One number over 999 : treat as literal datetxt */
		for(i = 0; valdate[i] && valdate[i] >= '0' && valdate[i] <= '9' && i < 14; i++) date[i] = valdate[i];
		date[i] = 0;
	}
	else if(nb == 2 && num[0] >= 1 && num[0] <= 31 && num[1] >= 1 && num[1] <= 12) 
	{
		/* Two numbers in range 1-31 / 1-12 : treat as day / month */
		strncpy(date, datetxt_now(), 4);
		sprintf(date+4, "%02d%02d", num[1], num[0]);
	}
	else if(nb == 2 && num[0] >= 1 && num[0] <= 12 && num[1] >= 1000 && num[1] <= 9999) 
	{
		/* Two numbers in range 1-12 / 1000-9999 : treat as month / year */
		sprintf(date, "%04d%02d", num[1], num[0]);
	}
	else if(nb >= 3 && num[0] >= 1 && num[0] <= 31 && num[1] >= 1 && num[1] <= 12) 
	{
		/* Three or more numbers with first two in range 1-31 / 1-12 : treat as day / month / year */
		div_t c;
		sscanf(datetxt_now(), "%04d", &i);
		c = div(i, 100);
		
		/* Translate year under 100 to closest century */
		if(num[2] < 100) num[2] += 100 * (c.quot + (c.rem <= 50 ?
													num[2] > c.rem + 50 ? -1 : 0 :
													num[2] < c.rem - 50 ? 1 : 0));
		sprintf(date, "%04d%02d%02d", num[2], num[1], num[0]);
	}
	else if(nb >= 3 && num[0] >= 100 && num[0] <= 9999 && num[1] >= 1 && num[1] <= 12 && num[2] >= 1 && num[2] <= 31) 
	{
		/* Three or more numbers in range 100-9999 / 1-12 / 1-31 : treat as year / month / day */
		sprintf(date, "%04d%02d%02d", num[0], num[1], num[2]);
	}

	/* Add time if given */
	if((nb > 3 || i_hour >= 0) && *date && num[3] < 24 && num[4] < 60 && num[5] < 60) 
	{
		sprintf(date + 8, "%02d%02d", num[3], num[4]);
		if(num[5]) sprintf(date + 12, "%02d", num[5]);
	}

	/* Set return value */
	if(!*date) strncpy(date, valdate, 16);

	return datetxt_invalid(date);
}

/*********************************************************************
** Function : datetxt_to_time
** Description : return a unix time from format YYYYMMDDHHMMSS
*********************************************************************/
int datetxt_to_time(		/* return : 0 on success, other on error */
	char *valdate,			/* in : date to convert (YYYYMMDDHHMMSS format) */
	time_t *t,				/* out : unix time */
	struct tm *tmout		/* out : unix time structure */
){
	static struct tm _dt;
	struct tm *dt = tmout ? tmout : &_dt;
	size_t len;

	/* Check parameters */
	if(!t) return 1;
	*t = (time_t)-1;
	memset(dt, 0, sizeof(*dt));
	if(!valdate || !*valdate) return 1;
	len = strlen(valdate);
	if(len < 4) return 1;

	/* Convert date to Unix time */
	dt->tm_isdst = -1;
	dt->tm_mday = 1;
	sscanf(valdate, "%4u%2u%2u%2u%2u%2u", 
					&dt->tm_year, &dt->tm_mon, &dt->tm_mday,
					&dt->tm_hour, &dt->tm_min, &dt->tm_sec);
	dt->tm_year -= 1900;
	if(dt->tm_mon) dt->tm_mon--;
	*t = mktime(dt);

	return (*t == (time_t)-1);
}

/*********************************************************************
** Function : delay_to_datetxt
** Description : add a delay to a reference date
*********************************************************************/
int delay_to_datetxt(	/* return : 0 on success, other on error */
	char *date,			/* out: converted date - 15 bytes long minimum */
	char *date_ref,		/* in : date reference from wich to compute date; if null use current time */
	int delay,			/* in : delay to add */
	char fmt			/* in : delay unit : h/m/s/D/M/Y */
){

	int Y0=0, M0=0, D0=0, h0=0, m0=0, s0=0, Y1=0, M1=0, D1=0, h1=0, m1=0, s1=0;
	time_t t = 0;
	struct tm dt;
	int b_errtime;

	/* Check & initialize params */
	if(!date) return 1;
	if(!date_ref || !*date_ref) date_ref = datetxt_now();

	/* Parse reference date */
	sscanf(date_ref, "%4d%2d%2d%2d%2d%2d", &Y1, &M1, &D1, &h1, &m1, &s1);
	b_errtime = datetxt_to_time(date_ref, &t, &dt);
	*date = 0;
	if(Y1 < 100) return 1;
	if(!M1) M1 = 1;
	if(!D1) D1 = 1;

	/* Start from date_ref */
	D0 = D1;
	M0 = M1;
	Y0 = Y1;
	h0 = h1;
	m0 = m1;
	s0 = s1;

	/* Compute date depending on delay unit */
	switch(fmt)
	{
	case 's':
		sprintf(date, "%04d%02d%02d%02d%02d%02d", Y0, M0, D0, h0, m0, s0);
		break;

	
	case 'm':
		sprintf(date, "%04d%02d%02d%02d%02d", Y0, M0, D0, h0, m0);
		break;
	
	case 'h':
		{
			/* Try computation with unix time */
			time_t t1 = (time_t)-1;
			dt.tm_hour += delay;

			if(!b_errtime) t1 = mktime(&dt);
			if(b_errtime || t1 == (time_t)-1 || time_to_datetxt(t1, date))
			{
				/* Use approximate computation */

				/* Ensure valid date */
				ENSURE_VALID_DATE;
				sprintf(date, "%04d%02d%02d", Y0, M0, D0);
				if(h0 || m0 || s0) sprintf(date+8, "%02d%02d%02d", h0, m0, s0); 
			}
		}
		break;
	
	case 'D':
		{
			/* Try computation with unix time */
			if(!b_errtime) 
			{
				time_t t1;
				int b_notime = !dt.tm_hour && !dt.tm_min && !dt.tm_sec;
				dt.tm_mday += delay;
				if(b_notime) dt.tm_hour = 1;
				t1 = mktime(&dt);
				b_errtime = t1 == (time_t)-1 || time_to_datetxt(t1, date);
				if(b_notime) date[8] = 0;
			}
			if(b_errtime)
			{
				/* Use approximate computation (365 days / year) */
				div_t dy = div(delay, 365);
				div_t dm = div(dy.rem, 12);
				D0 += dm.rem;
				if(D0 < 1) { D0 += 30; M0--; }
				else if(D0 > 30) { D0 -= 30; M0++; }
				M0 += dm.quot;
				if(M0 < 1) { M0 += 12; Y0--; }
				else if(M0 > 12) { M0 -= 12; Y0++; }
				Y0 += dy.quot;

				/* Ensure valid date */
				ENSURE_VALID_DATE;
				sprintf(date, "%04d%02d%02d", Y0, M0, D0);
				if(h0 || m0 || s0) sprintf(date+8, "%02d%02d%02d", h0, m0, s0); 
			}
		}
		break;
	
	case 'M':
		/* Compute month delay */
		{
			div_t dy = div(delay, 12);
			M0 += dy.rem;
			if(M0 < 1) { M0 += 12; Y0--; }
			else if(M0 > 12) { M0 -= 12; Y0++; }
			Y0 += dy.quot;
			sprintf(date, "%04d%02d%02d", Y0, M0, D0);
			if(h0 || m0 || s0) sprintf(date+8, "%02d%02d%02d", h0, m0, s0); 
			break;
		}
	
	default:
		Y0 += delay;
		sprintf(date, "%04d%02d%02d", Y0, M0, D0);
		if(h0 || m0 || s0) sprintf(date+8, "%02d%02d%02d", h0, m0, s0); 
	}

	return 0;
}

/*********************************************************************
** Function : datetxt_time
** Description : output time for date
*********************************************************************/
size_t datetxt_time(		/* return : # of output chars */
	char *date_out,			/* out : converted date - must be 32 bytes long minimum */
	char *date,				/* in : date/time in YYYYMMDDHHMMSS format to convert */
	size_t len,				/* in : strlen(date) */
	size_t outlen			/* in : max # of chars to use in date */
){
	size_t sz = 0;
	if(!outlen) outlen = len;
	if(outlen > 8) sz += sprintf(date_out, " %.2s:%.2s", len > 8 ? date+8 : "00", len > 10 ? date+10 : "00");
	if(outlen > 12) sz += sprintf(date_out + sz, ":%.2s", len > 12 ? date+12 : "00"); 
	return sz;
}

/*********************************************************************
** Function : datetxt_to_numbers
** Description : return a number formated date string 
*********************************************************************/
void datetxt_to_numbers(
	char *date_out,			/* out : converted date - must be 32 bytes long minimum */
	char *date,				/* in : date/time in YYYYMMDDHHMMSS format to convert */
	size_t len,				/* in : strlen(date) */
	int b_year2,			/* in : year on 2 digits if set */
	size_t outlen			/* in : max # of chars to use in date */
){
	if(!outlen) outlen = len;

	/* Handle short date formats */
	if(outlen <= 4)
	{
		sprintf(date_out, "%.4s", date); 
	}
	else if(outlen <= 6)
	{
		sprintf(date_out, "%.2s/%.4s", len > 4 ? date+4 : "01", date); 
	}
	else
	{
		/* Convert date part */
		size_t sz = sprintf(date_out, b_year2 ? "%.2s/%.2s/%.2s" : "%.2s/%.2s/%.4s",
									len > 6 ? date+6 : "01", len > 4 ? date+4 : "01", 
									(b_year2 && len > 2) ? date+2 : date); 

		/* Convert time part */
		datetxt_time(date_out + sz, date, len, outlen); 
	}
}

/*********************************************************************
** Function : datetxt_day
** Description : output day for date
*********************************************************************/
size_t datetxt_day(					/* return : # of output chars */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *date_out,					/* out : converted date - must be 32 bytes long minimum */
	char *date,						/* in : date/time in YYYYMMDDHHMMSS format to convert */
	size_t len,						/* in : strlen(date) */
	int b_day,						/* in : output week day if set - only weekday & day if 2 */
	int b_long						/* in : output long names if set */
){
	time_t t;
	struct tm dt;
	size_t sz = 0;
	unsigned monthday = 1;
	if(len > 7) sscanf(date + 6, "%2d", &monthday);

	/* Add weekday if applicable */
	if(b_day && len > 7)
	{
		int b_wd =  !datetxt_to_time(date, &t, &dt);
		if(!b_wd)
		{
			int b_feb29 = !(dt.tm_year % 4) && strcmp(date + 4, "0229") < 0;
			dt.tm_wday = (4 - 365 * (1970 - dt.tm_year) - (1967 - dt.tm_year)/4 - (1999 - dt.tm_year)/100 - (b_feb29 ? 0 : 1)) % 7;
		}
		sz += sprintf(date_out, "%s ", dyntab_val((b_long ? &cntxt->daylong : &cntxt->dayshort), dt.tm_wday, 0));
	}
	if(b_day == 2) return sz;

	/* Add day */
	sz += sprintf(date_out + sz, "%u", monthday); 
	if((b_day || b_long) && monthday == 1) sz += sprintf(date_out + sz, "er"); 
	return sz;
}

/*********************************************************************
** Function : datetxt_to_text
** Description : return a text formated date string 
*********************************************************************/
void datetxt_to_text(
	EVA_context *cntxt,				/* in/out : execution context data */
	char *date_out,					/* out : converted date - must be 32 bytes long minimum */
	char *date,						/* in : date/time in YYYYMMDDHHMMSS format to convert */
	size_t len,						/* in : strlen(date) */
	int b_mask,						/* in : output control bitmask
										bit 0 : output week day if set
										bit 1 : do not output year if set*/
	int b_long,						/* in : output long names if set */
	size_t outlen					/* in : max # of chars to use in date */
){
	/* Get month index */
	int b_day = b_mask & 1;
	int b_year = !(b_mask & 2);
	unsigned month = 1;
	if(len > 5) sscanf(date + 4, "%2d", &month);
	if(--month > 11) month = 11;
	if(!outlen) outlen = len;

	/* Handle short date formats */
	if(outlen <= 4)
	{
		sprintf(date_out, "Année %.4s", date); 
	}
	else if(outlen <= 6)
	{
		sprintf(date_out, "%s %.4s", dyntab_val((b_long ? &cntxt->monthlong : &cntxt->monthshort), month, 0), date); 
	}
	else
	{
		/* Convert date part */
		size_t sz = datetxt_day(cntxt, date_out, date, len, b_day, b_long);
		sz += sprintf(date_out + sz, " %s", dyntab_val((b_long ? &cntxt->monthlong : &cntxt->monthshort), month, 0)); 
		if(b_year) sz += sprintf(date_out + sz, " %.4s", date); 

		/* Convert time part */
		datetxt_time(date_out + sz, date, len, outlen); 
	}
}

/*********************************************************************
** Function : datetxt_to_format
** Description : return a formatted date string 
*********************************************************************/
int datetxt_to_format(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *date_out,					/* out : converted date - must be 32 bytes long minimum */
	char *date,						/* in : date/time in YYYYMMDDHHMMSS format to convert */
	char *format					/* in : output format */
){
	size_t len;

	/* Check parameters */
	if(!date_out) return -1;
	*date_out = 0;
	if(!date || !*date) return 0;
	len = strlen(date);
	if(datetxt_invalid(date))
	{
		sprintf(date_out, "%.30s", date);
		return 1;
	}

	/* Parse format specification */
	if(!format) format = "";
	if(!strcmp(format, "_EVA_DATE"))  datetxt_to_numbers(date_out, date, len, 0, len < 8 ? 0 : 8);
	else if(!strcmp(format, "_EVA_DATETIME")) datetxt_to_numbers(date_out, date, len, 0, len < 14 ? 0 : 14); 
	else if(!strcmp(format, "_EVA_Short")) datetxt_to_numbers(date_out, date, len, 1, 8); 
	else if(!strcmp(format, "_EVA_ShortTime")) datetxt_to_numbers(date_out, date, len, 1, 12); 
	else if(!strcmp(format, "_EVA_Std")) datetxt_to_numbers(date_out, date, len, 0, 8); 
	else if(!strcmp(format, "_EVA_StdTime")) datetxt_to_numbers(date_out, date, len, 0, 12); 
	else if(!strcmp(format, "_EVA_StdTimeSec")) datetxt_to_numbers(date_out, date, len, 0, 14); 
	else if(!strcmp(format, "_EVA_StdMonth")) datetxt_to_numbers(date_out, date, len, 0, 6); 
	else if(!strcmp(format, "_EVA_Year")) datetxt_to_numbers(date_out, date, len, 0, 4); 
	else if(!strcmp(format, "_EVA_Long")) datetxt_to_text(cntxt, date_out, date, len, 0, 0, 8);
	else if(!strcmp(format, "_EVA_LongVar")) datetxt_to_text(cntxt, date_out, date, len, 1, 0, 0);
	else if(!strcmp(format, "_EVA_LongTime")) datetxt_to_text(cntxt, date_out, date, len, 0, 0, 12);
	else if(!strcmp(format, "_EVA_LongDay")) datetxt_to_text(cntxt, date_out, date, len, 1, 0, 8);
	else if(!strcmp(format, "_EVA_LongVarDay")) datetxt_to_text(cntxt, date_out, date, len, 1, 0, 0);
	else if(!strcmp(format, "_EVA_LongTimeDay")) datetxt_to_text(cntxt, date_out, date, len, 1, 0, 12);
	else if(!strcmp(format, "_EVA_Full")) datetxt_to_text(cntxt, date_out, date, len, 0, 1, 8);
	else if(!strcmp(format, "_EVA_FullVar")) datetxt_to_text(cntxt, date_out, date, len, 0, 1, 0);
	else if(!strcmp(format, "_EVA_FullVarDay")) datetxt_to_text(cntxt, date_out, date, len, 1, 1, 0);
	else if(!strcmp(format, "_EVA_FullTime")) datetxt_to_text(cntxt, date_out, date, len, 0, 1, 12);
	else if(!strcmp(format, "_EVA_FullDay")) datetxt_to_text(cntxt, date_out, date, len, 1, 1, 8);
	else if(!strcmp(format, "_EVA_FullTimeDay")) datetxt_to_text(cntxt, date_out, date, len, 1, 1, 12);
	else if(!strcmp(format, "_EVA_MonthYear")) datetxt_to_text(cntxt, date_out, date, len, 0, 1, 6);
	else if(!strcmp(format, "_EVA_MonthYearShort")) datetxt_to_text(cntxt, date_out, date, len, 0, 0, 6); 
	else if(!strcmp(format, "_EVA_DayMonth")) datetxt_to_text(cntxt, date_out, date, len, 3, 0, 8); 
	else if(!strcmp(format, "_EVA_DayMonthFull")) datetxt_to_text(cntxt, date_out, date, len, 3, 1, 8); 
	else if(!strcmp(format, "_EVA_Day")) datetxt_day(cntxt, date_out, date, len, 0, 1); 
	else if(!strcmp(format, "_EVA_DayShort")) datetxt_day(cntxt, date_out, date, len, 0, 0); 
	else if(!strcmp(format, "_EVA_DayLong")) datetxt_day(cntxt, date_out, date, len, 1, 0); 
	else if(!strcmp(format, "_EVA_DayFull")) datetxt_day(cntxt, date_out, date, len, 1, 1); 
	else if(!strcmp(format, "_EVA_Time")) datetxt_time(date_out, date, len, 12); 
	else if(!strcmp(format, "_EVA_DayTime") ||
			!strcmp(format, "_EVA_DayFullTime"))
	{
		size_t sz = datetxt_day(cntxt, date_out, date, len, 2, !strcmp(format, "_EVA_DayTime") ? 0 : 1);
		sz += sprintf(date_out + sz, " ");
		datetxt_time(date_out + sz, date, len, 12);
	}
	else if(!strcmp(format, "_EVA_Adaptative"))
	{
		char f = 'D';
		int age = datetxt_to_age(date, NULL, &f); 
		if(f == 'D' && abs(age) < 7)
		{
			size_t sz = abs(age) < 2 ?
				sprintf(date_out, 
						age == 0 ? "aujourd'hui" :
						age == 1 ? "hier" :
						age == 2 ? "avant hier" :
						age == -1 ? "demain" :
						age == -2 ? "après demain" : "") :
				datetxt_day(cntxt, date_out, date, len, 1, 1);
			if(len > 8) datetxt_time(date_out + sz, date, len, 12); 
		}
		else
			datetxt_to_numbers(date_out, date, len, 0, 0);
	}
	else if(!strcmp(format, "_EVA_DATE_SORT"))
	{
		sprintf(date_out, "%.4s-%.2s-%.2s", 
			date, len > 4 ? date+4 : "01", len > 6 ? date+6 : "01"); 
	}
	else
		datetxt_to_numbers(date_out, date, len, 0, 0);

	return 0;
}

/*********************************************************************
** Function : number_space_thousands
** Description : format a number with thousands spaces & decimals
*********************************************************************/
char *number_space_thousands(	/* return : formated number (static copy) */
	char *stdnum,				/* in : original unformated number */
	int decimals				/* in : right pad with 0 up to # of decimals */
){
	static char num[64];
	size_t len = strlen(stdnum), spc;
	char *end = strchr(stdnum, '.');
	char *in = stdnum, *out = num, *chk;
	if(!stdnum) return stdnum;
	if(!end) end = stdnum + len;
	memset(num, 0, sizeof(num));

	/* Check number */
	if(!len || len > 32) return stdnum;
	if(*in == '-') { *out = *in; in++; out++; }
	chk = in;
	while(*chk)
		if(strchr("0123456789.", *chk)) chk++;
		else return stdnum;

	/* Copy blocks of 3 chars */
	if(decimals > 10) decimals = 10;
	else if(decimals < -10) decimals = -10;
	spc = end - in;
	spc %= 3;
	while(*in && *in != '.' && in < end)
	{
		if(*num) *out++ = ' ';
		memcpy(out, in, spc);
		out += spc;
		in += spc;
		spc = 3;
	}

	/* Handle decimals */
	if(decimals > 0)
	{
		/* Add decimals if positive */
		int i;
		if(*in == '.') in++;
		*out++ = '.';
		for(i = 0; i < decimals; i++)	*out++ = (char)(*in ? *in++ : '0');
	}
	else if(decimals < 0)
	{
		/* Round integer part */
		int i;
		for(i = 0; i < -decimals && out > num; i++)
		{
			if(*out != ' ') { out--; decimals--; } else *out-- = '0';
		}
	}

	return num;
}

/*********************************************************************
** Function : input_to_number
** Description : return a clean number from user input
*********************************************************************/
char *input_to_number(char *in)
{
	static char num[32];
	char *input = in;
	char *n = num;
	int b_sep = 0;
	if(*input == '-') *n++ = *input++;
	while(*input && n - num < sizeof(num) - 1)
	{
		/* Keep digits without leading zero */
		if(*input > '0' && *input <= '9' || *input == '0' && n > num)
			*n++ = *input;

		/* Keep first decimal separator */
		else if(strchr(".,", *input) && !b_sep)
		{
			b_sep = 1;

			/* Insert leading zero before decimal separator */
			if(n == num) *n++ = '0';
			*n++ = '.';
		}
		input++;
	}

	/* Remove trailing zeros after decimal separator */
	if(b_sep)
	{
		while(n > num && *(n - 1) == '0') n--;
		/* Remove trailing decimal separator */
		if(*(n - 1) == '.') n--;
	}
	*n = 0;
	if(in && *in && (n == num || !strcmp(num, "-"))) sprintf(num, "0");
	return num;
}

/*********************************************************************
** Function : human_filesize
** Description : return a file size in bytes / kb / Mb / Tb / Pb form
*********************************************************************/
char *human_filesize(size_t size)
{
	static char num[32];
	double numf = (double) size;
	char *unit = "octets";
	*num = 0;
	if(numf >= 1000.)
	{
		numf /= 1000.;
		unit = "ko";
		if(numf >= 1000.)
		{
			numf /= 1000.;
			unit = "Mo";
			if(numf >= 1000.)
			{
				numf /= 1000.;
				unit = "Go";
				if(numf >= 1000.)
				{
					numf /= 1000.;
					unit = "To";
					if(numf >= 1000.)
					{
						numf /= 1000.;
						unit = "Po";
					}
				}
			}
		}
		if(numf < 10) sprintf(num, "%1.2lf %s", numf, unit);
		else if(numf < 100) sprintf(num, "%1.1lf %s", numf, unit);
		else sprintf(num, "%d %s", (int)numf, unit);
	}
	else
		sprintf(num, "%lu", size);

	return num;
}

/*********************************************************************
** Function : number_to_format
** Description : return a formatted number
*********************************************************************/
char *number_to_format(char *in, char *format)
{
	static char num[32];
	long numl = in ? strtol(in, NULL, 10) : 0;
	double numf = in ? atof(in) : 0.0;
	*num = 0;
	if(!in || !*in) return "";
	if(!strcmp(format, "_EVA_Std")) sprintf(num, "%ld", numl);
	else if(!strcmp(format, "_EVA_StdDec")) sprintf(num, "%lf", numf);
	else if(!strcmp(format, "_EVA_FILESIZE")) return human_filesize(numl);
	else if(!stricmp(format, "_EVA_Money"))
	{
		sprintf(num, "%1.2lf", numf);
		return number_space_thousands(num, 2);
	}
	else
		return input_to_number(in);

	return num;
}

/*********************************************************************
** Function : unique_bit_index
** Description : return the unique bit position (starting at 1) in a number	
**				 return 0 if multiple or no bits set
*********************************************************************/
unsigned long unique_bit_index(unsigned long word)
{
	int i, j;
	for(i = 1, j = 0; word; i++, word >>= 1)
		if(word & 1)
		{
			if(j) return 0; 
			j = i;
		}

	return j;
}

/*********************************************************************
* Fonction : dbl_str
** Description : convert double to simplest string
*********************************************************************/
size_t dbl_str(			/* return : # of chars output to txt */
	char *txt,			/* out : d converted to string */
	double d,			/* in : number to convert */
	const char *fmt		/* in : optional print format */
){
	size_t sz = sprintf(txt, fmt ? fmt : "%lf", d);
	char *dot = strchr(txt, '.');
	if(dot)
	{
		while(sz && txt[sz - 1] == '0') txt[--sz] = 0;
		if(txt + sz == dot + 1)
		{
			*dot = 0;
			sz--;
		}
	}
	return sz;
}

/*********************************************************************
* Fonction : mystrdup
** Description : strdup clone
*********************************************************************/
char *mystrdup(char *src, size_t sz)
{
	char *r = malloc(sz + 2); 
	if(r) 
	{
		memset(r, 0, sz + 1);
		if(src) memcpy(r, src, sz);
	}
	return r;
}
