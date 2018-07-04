/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : date_utils.h
** Description : date conversion functions
**      Author : Alain BOUTE
**     Created : Feb 15 2002
*********************************************************************/

#define SUNDAY		"Dim"
#define MONDAY		"Lun"
#define TUESDAY		"Mar"
#define WEDNESDAY	"Mer"
#define THURSDAY	"Jeu"
#define FRIDAY		"Ven"
#define SATURDAY	"Sam"
#define SECONDE		"sec"
#define MINUTE		"min"
#define HOUR		"hr"
#define DAY			"jour"
#define MONTH		"mois"
#define YEAR		"an"

/*********************************************************************
** Function : datetxt_to_age
** Description : build age from date
*********************************************************************/
int datetxt_to_age(		/* return : age (signed) in the requested unit */
	char *date,			/* in : date to translate in YYYYMMDDHHMMS format */
	char *now,			/* in : date to compare with in YYYYMMDDHHMMS format */
	char *format		/* in/out : age unit - do not use char constants !!! Use a pointer to alloc-ed or stack memory
							in :
							 0  = best fit
							's' = seconds
							'm' = minutes
							'h' = hours
							'D' = days
							'M' = months
							other = years
						   out :
							used format as above (may be different from input to avoid overflow)
							'E' on error
							 0 else */
);

/*********************************************************************
** Function : time_to_datetxt
** Description : return a YYYYMMDDHHMMSS format date from unix time
*********************************************************************/
int time_to_datetxt(		/* return : 0 on success, other on error */
	time_t t,				/* in : unix time to convert */
	char *valdate			/* out : converted time - must be 16 bytes long minimum */
);

/*********************************************************************
** Function : full_datetxt
** Description : set missing digits for a YYYYMMDDHHMMSS date
*********************************************************************/
void full_datetxt(
	char *date			/* in/out : date - must be 16 bytes long minimum */
);

/*********************************************************************
** Function : datetxt_now
** Description : return current time in YYYYMMDDHHMMSS format (static string)
*********************************************************************/
char *datetxt_now();

/*********************************************************************
** Function : input_to_datetxt
** Description : return a YYYYMMDDHHMMSS format date from user input
*********************************************************************/
int input_to_datetxt(		/* return : 0 on success, other on error */
	char *valdate,			/* in : date to convert (user input format) */
	char *date				/* out : converted time - must be 16 bytes long minimum */
);

/*********************************************************************
** Function : datetxt_invalid
** Description : check if a date is valid
*********************************************************************/
int datetxt_invalid(		/* return : 0 if date valid, other if invalid */
	char *valdate			/* in : date to check (YYYYMMDDHHMMSS format) */
);

/*********************************************************************
** Function : datetxt_to_time
** Description : return a unix time from format YYYYMMDDHHMMSS
*********************************************************************/
int datetxt_to_time(		/* return : 0 on success, other on error */
	char *valdate,			/* in : date to convert (YYYYMMDDHHMMSS format) */
	time_t *t,				/* out : unix time */
	struct tm *tmout		/* out : unix time structure */
);

/*********************************************************************
** Function : delay_to_datetxt
** Description : build date from delay & date reference
*********************************************************************/
int delay_to_datetxt(	/* return : 0 on success, other on error */
	char *date,			/* out: converted date - 32 bytes long minimum */
	char *date_ref,		/* in : date reference from wich to compute date; if null use current time */
	int delay,			/* in : delay to translate */
	char fmt			/* in : delay unit : h/m/s/D/M/Y */
);


/*********************************************************************
** Function : datetxt_to_format
** Description : return a formatted date string 
*********************************************************************/
int datetxt_to_format(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *date_out,			/* out : converted date - must be 32 bytes long minimum */
	char *valdate,			/* in : date/time in YYYYMMDDHHMMSS format to convert */
	char *format			/* in : output format */
);

/*********************************************************************
** Function : number_space_thousands
** Description : insert thousands spaces in the integer part of a number
*********************************************************************/
char *number_space_thousands(	/* return : formated number (static copy) */
	char *stdnum,				/* in : original unformated number */
	int decimals				/* in : right pad with 0 up to # of decimals */
);

/*********************************************************************
** Function : input_to_number
** Description : return a number with spaces between thousands
*********************************************************************/
char *input_to_number(char *input);

/*********************************************************************
** Function : number_to_format
** Description : return a formatted number
*********************************************************************/
char *number_to_format(char *in, char *format);

/*********************************************************************
** Function : human_filesize
** Description : return a file size in bytes / kb / Mb / Gb / Tb / Pb form
*********************************************************************/
char *human_filesize(size_t size);

/*********************************************************************
** Function : unique_bit_index
** Description : return the unique bit position (starting at 1) in a number	
**				 return 0 if multiple or no bits set
*********************************************************************/
unsigned long unique_bit_index(unsigned long word);

/*********************************************************************
* Fonction : dbl_str
** Description : convert double to string
*********************************************************************/
size_t dbl_str(			/* return : # of chars output to txt */
	char *txt,			/* out : d converted to string */
	double d,			/* in : number to convert */
	const char *fmt		/* in : optional print format */
);

/*********************************************************************
* Fonction : mystrdup
** Description : strdup clone
*********************************************************************/
char *mystrdup(char *src, size_t sz);
