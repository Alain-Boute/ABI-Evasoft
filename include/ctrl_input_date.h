/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_date.h
** Description : header for control date interface
**      Author : Claude Broudin
**     Created : June 22 2003
*********************************************************************/

int ctrl_add_date(EVA_context *cntxt, unsigned long i_ctrl);

int datetxt_to_age(char *date, char *now, char *age, int format);
int age_to_datetxt(		/* return : 0 on success, other on error */
	int age,			/* in : age to translate */
	char *now,			/* in : date from which compute date */
	char *date,			/* out: converted date - 32 bytes long minimum */
	int format			/* in : conversion format */
);
