/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : check_login.c
** Description : user identification handling functions
**      Author : Alain BOUTE
**     Created : Sept 23 2001
*********************************************************************/

#include "eva.h"

#define FRAUD_WARNING "Utilisation simultanée du compte sur plusieurs postes - Session terminée"
#define FRAUD_BLOKMSG "Utilisation simultanée du compte sur plusieurs postes - Compte bloqué"

/*********************************************************************
** Function : set_session_statistics
** Description : update statistics data in current session
*********************************************************************/
void set_session_statistics(
	EVA_context *cntxt,			/* in : execution context */
	DynTable *data,				/* in : session data */
	unsigned long idobj,		/* in : displayed object or form */
	unsigned long idfield		/* in : updated field (IdValue) */
){
	char qry[1024] = {0};
	unsigned long i; 
	DynTableCell *c = NULL;

	/* Return if no object */
	if(!idobj) return;
	if(idobj == ~0UL) idobj = 0;

	/* Search for given field & object in session data */
	for(i = 0; i < data->nbrows; i++)
	{
		c = dyntab_cell(data, i, 0);
		if(c->IdField == idfield && (!idobj || c->txt && strtoul(c->txt, NULL, 10) == idobj)) break;
	}

	/* If field / obj pair found : increment Num counter */
	if(i < data->nbrows)
		snprintf(add_sz_str(qry), "UPDATE TLink SET Num=%lu,DateCr=%s,IdWhoCr=%s WHERE Pkey=%lu",
					c->Num + 1, cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), c->Pkey);
	/* Else : create new record */
	else if(idobj)
		snprintf(add_sz_str(qry), "INSERT INTO TLink (IdObj,IdField,IdRelObj,Num,DateCr,IdWhoCr) VALUES (%s,%lu,%lu,1,%s,%s)", 
										cntxt->session.cell->txt, idfield, idobj, cntxt->timestamp,
										cntxt->id_user.nbrows ? cntxt->id_user.cell->txt : "NULL");
	if(*qry) sql_exec_query(cntxt, qry);
}

/*********************************************************************
** Function : qry_get_rec_info
** Description : return information on a record
*********************************************************************/
#define ERR_FUNCTION "qry_get_rec_info"
#define ERR_CLEANUP 
int qry_get_rec_info(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : read information */
	unsigned long pkey,							/* in : record to read */
	char *fields								/* in : fields to read */
){
	char qry[256];
	snprintf(add_sz_str(qry), "SELECT %s FROM TLink WHERE Pkey=%lu", fields, pkey);
	if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, res, 2)) STACK_ERROR;
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_message
** Description : display a message page
*********************************************************************/
void output_message(
	EVA_context *cntxt,				/* in/out : execution context data */
	char *msg						/* in : message text */
){
	put_html_page_header(cntxt, NULL, NULL, NULL, 3);
	cntxt->txsize += printf(
		"<font face=Arial><center>"
		"<br><br><u>Agent de sécurité</u><br><br>"
		"<hr><b><font size=+1 color=#886600>%s</font></b><hr><br><br>",
		msg);
	cntxt->b_terminate = 15;
}

/*********************************************************************
** Function : output_ident_inputs
** Description : display HTML inputs for user identification
*********************************************************************/
void output_ident_inputs(
	EVA_context *cntxt,				/* in/out : execution context data */
	char *title						/* in : identification title */
){
	int b_showid = strtoul(DYNTAB_FIELD_VAL(&cntxt->cnf_data, ALLOW_CLEAR_LOGIN), NULL, 10);
	cntxt->txsize += printf(
		"%s<br><br>"
		"<table noborder width=100%% bgcolor=#FFFFFF>"
		"<tr><td align=right width=50%%><font size=-1 color=#0000AA>"
			"Identifiant</font></td><td><input type=%s name='D$#_EVA_LOGINSUBMIT'></td></tr>"
		"<tr><td align=right><font size=-1 color=#0000AA>"
			"Mot de passe</font></td><td><input type=password name='D$#_EVA_LOGINSUBMITP'></td></tr></table>"
		"<br><input type=submit name=B$#CHECK_IDENT value=\"Valider l'identification\"><br><br>",
		title,
		b_showid ? "text" : "password");
	if(cntxt->jsenabled) cntxt->txsize += printf(
		"<script>document.mainform['D$#_EVA_LOGINSUBMIT'].focus();</script>");
}

/*********************************************************************
** Function : handle_workstation_id
** Description : handle workstation cookie
*********************************************************************/
#define ERR_FUNCTION "handle_workstation_id"
#define ERR_CLEANUP DYNTAB_FREE(sqlres)
int handle_workstation_id(						/* return : 1 on success, other on error */
	EVA_context *cntxt							/* in/out : execution context data */
){
	DynTable sqlres = { 0 };
	unsigned long idformwks = strtoul(DYNTAB_FIELD_VAL(&cntxt->cnf_data, FORM_USERWKS), NULL, 10);
	unsigned long fldidwks = idformwks ? get_id_value(cntxt, add_sz_str("_EVA_ID_WKS")) : 0;
	char qry[2148];

	/* Do not handle if no worksation form defined in server config */
	if(!idformwks || !fldidwks) RETURN_OK;

	/* Check if cookie present */
	if(cntxt->id_wks)
	{ 
		/* Check if object exist for workstation */
		unsigned long valcook = get_id_value(cntxt, DYNBUF_VAL_SZ(cntxt->id_wks));
		if(valcook)
		{
			snprintf(add_sz_str(qry),
				"-- Search for workstation Id\n"
				"SELECT DISTINCT IdObj FROM TLink WHERE IdField=%lu AND IdValue=%lu AND DateDel IS NULL",
				fldidwks, valcook);
			if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
			cntxt->objwks = DYNTAB_TOUL(&sqlres);
		}

		/* Create workstation object if applicable */
		if(!cntxt->objwks)
		{
			if(qry_add_new_obj(cntxt, &sqlres, idformwks) ||
				qry_add_obj_field_val(cntxt, &sqlres, "_EVA_ID_WKS", DYNBUF_VAL_SZ(cntxt->id_wks), 0, 1))
				STACK_ERROR;
			cntxt->objwks = DYNTAB_TOUL(&sqlres);
		}
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : check_user_fraud
** Description : handle same user from multiple workstations
*********************************************************************/
#define ERR_FUNCTION "check_user_fraud"
#define ERR_CLEANUP DYNTAB_FREE(sqlres)
int check_user_fraud(							/* return : 1 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	char *fraudchk,								/* in : fraud check mode */
	char **errmsg								/* out : error message if any */
){
	DynTable sqlres = { 0 };
	char qry[1024];

	/* Read last click time */
	DynTableCell *c = DYNTAB_FIELD_CELL(&cntxt->sess_data, USER_IP);
	if(c && qry_get_rec_info(cntxt, &sqlres, c->Pkey, "DateCr")) STACK_ERROR;

	/* Search for clicks of same user after last click */
	snprintf(add_sz_str(qry),
		"SELECT IdObj FROM TLink WHERE IdWhoCr=%lu AND IdField=%lu AND DateCr>'%s'",
		DYNTAB_TOUL(&cntxt->id_user), IDVAL("_EVA_USER_IP"), dyntab_val(&sqlres, 0, 0));
	if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;

	/* Handle fraud detection */
	if(sqlres.nbrows)
	{
		unsigned long i;
		size_t len = strlen(fraudchk);
		if(!strcmp(fraudchk, "_EVA_BLOCK"))
		{
			/* Block user account */
			*errmsg = FRAUD_BLOKMSG;
			if(qry_add_idobj_field_val(cntxt, DYNTAB_TOUL(&cntxt->id_user), "_EVA_DEACTIVATE", fraudchk, len, 1, 0, 1))
				STACK_ERROR;
		}
		else
		{
			*errmsg = FRAUD_WARNING;
		}

		/* Terminate all sessions - memorize fraud & resulting action */
		if(qry_add_idobj_field_val(cntxt, DYNTAB_TOUL(&cntxt->session), "_EVA_TERMINATE", fraudchk, len, 1, 0, 1))
			STACK_ERROR;
		for(i = 0; i < sqlres.nbrows; i++)
			if(qry_add_idobj_field_val(cntxt, DYNTAB_TOULRC(&sqlres, i, 0), "_EVA_TERMINATE", fraudchk, len, 1, 0, 1))
				STACK_ERROR;
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ckeck_user_validity
** Description : check if current user account is valid
*********************************************************************/
#define ERR_FUNCTION "ckeck_user_validity"
#define ERR_CLEANUP
int ckeck_user_validity(						/* return : 1 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	char **errmsg								/* out : error message if any */
){
	unsigned long iduser = DYNTAB_TOUL(&cntxt->id_user);
	char *fraudchk;

	*errmsg = NULL;
	if(!iduser || iduser == cntxt->id_public) RETURN_OK;
	if(qry_obj_data(cntxt, &cntxt->user_data, &cntxt->id_user, NULL)) STACK_ERROR;
	fraudchk = DYNTAB_FIELD_VAL(&cntxt->cnf_data, CHECK_FRAUD);
	if(DYNTAB_FIELD_CELL(&cntxt->user_data, NO_IP_CHECK)) cntxt->b_noipcheck = 1;

	/* Check account validity */
	if(DYNTAB_FIELD_CELL(&cntxt->user_data, DEACTIVATE) || !(
		DYNTAB_FIELD_CELL(&cntxt->user_data, LOGIN) &&
		DYNTAB_FIELD_CELL(&cntxt->user_data, PASSWORD)))
	{
		/* Message if account deactivated */
		*errmsg = "Votre compte est désactivé";
	}

	/* Disable connection if fraud detection & no cookie */
	else if(iduser != cntxt->id_public && *fraudchk)
	{
		if(!cntxt->id_wks->cnt)
		{
			*errmsg = "La détection de fraude est activée : votre navigateur doit autoriser les cookies";
		}
		else
		{
			if(check_user_fraud(cntxt, fraudchk, errmsg)) STACK_ERROR;
		}
	}

	/* Ignore user if not valid */
	if(*errmsg)
	{
		DYNTAB_FREE(cntxt->id_user);
		DYNTAB_FREE(cntxt->user_data);
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ckeck_session_validity
** Description : check if current session is valid
*********************************************************************/
#define ERR_FUNCTION "ckeck_session_validity"
#define ERR_CLEANUP DYNTAB_FREE(sqlres); \
					if(!cntxt->session.nbrows) DYNTAB_FREE(cntxt->sess_data)
int ckeck_session_validity(						/* return : 1 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	char **errmsg								/* out : error message if any */
){
	DynTable sqlres = { 0 };
	char *txt;
	time_t t = 0, minutes;

	/* Return if no active session */
	*errmsg = NULL;
	if(!cntxt->session.nbrows) RETURN_OK;

	/* Read current session & user data */
	if(qry_obj_data(cntxt, &cntxt->sess_data, &cntxt->session, NULL)) STACK_ERROR;
	DYNTAB_FIELD(&cntxt->id_user, &cntxt->sess_data, USER);

	/* Check user account validity */
	if(ckeck_user_validity(cntxt, errmsg)) STACK_ERROR;
	if(*errmsg) RETURN_OK;

	/* Check session termination */
	txt = DYNTAB_FIELD_VAL(&cntxt->sess_data, TERMINATE);
	if(*txt)
	{
		*errmsg = !strcmp(txt, "10") ? FRAUD_WARNING :
				  !strcmp(txt, "11") ? FRAUD_BLOKMSG :
									  "Cette session est terminée";
		DYNTAB_FREE(cntxt->session);
		RETURN_OK;
	}

	/* Check user IP changes */
	if(cntxt->user_ip && !cntxt->b_noipcheck && strcmp(cntxt->user_ip, DYNTAB_FIELD_VAL(&cntxt->sess_data, USER_IP)))
	{
		*errmsg = "Votre session n'est plus valide (l'adresse IP a changé)";
		DYNTAB_FREE(cntxt->session);
		RETURN_OK;
	}

	/* Check maximum duration time */
	txt = DYNTAB_FIELD_VAL(&cntxt->user_data, MAX_SESSION_DURATION);
	if(!*txt) txt = DYNTAB_FIELD_VAL(&cntxt->cnf_data, MAX_SESSION_DURATION);
	if(!*txt) txt = "14400";
	minutes = strtoul(txt, NULL, 10);
	if(minutes)
	{
		DynTableCell *c = DYNTAB_FIELD_CELL(&cntxt->sess_data, FORMSTAMP);
		if(c && qry_get_rec_info(cntxt, &sqlres, c->Pkey, "DateCr")) STACK_ERROR; 
		datetxt_to_time(dyntab_val(&sqlres, 0, 0), &t, NULL);
		if(cntxt->tcur - t > minutes * 60)
		{
			*errmsg = "Votre session n'est plus valide (durée maximum dépassée)";
			DYNTAB_FREE(cntxt->session);
			RETURN_OK;
		}
	}

	/* Check inactive time */
	txt = DYNTAB_FIELD_VAL(&cntxt->user_data, MAX_INACTIVE_DURATION);
	if(!*txt) txt = DYNTAB_FIELD_VAL(&cntxt->cnf_data, MAX_INACTIVE_DURATION);
	minutes = strtoul(txt, NULL, 10);
	if(minutes && !cntxt->b_login_pwd)
	{
		DynTableCell *c = DYNTAB_FIELD_CELL(&cntxt->sess_data, USER_IP);
		if(c && qry_get_rec_info(cntxt, &sqlres, c->Pkey, "DateCr")) STACK_ERROR;
		datetxt_to_time(dyntab_val(&sqlres, 0, 0), &t, NULL);
		if(cntxt->tcur - t > minutes * 60)
		{
			*errmsg = "Durée maximum d'inactivité dépassée - identification requise";
			RETURN_OK;
		}
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : check_login_pwd
** Description : search account matching user credentials
*********************************************************************/
#define ERR_FUNCTION "check_login_pwd"
#define ERR_CLEANUP DYNTAB_FREE(login); \
					DYNTAB_FREE(passwd); \
					DYNTAB_FREE(sqlres); \
					M_FREE(sql)
int check_login_pwd(							/* return : 1 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	char **errmsg								/* out : error message if any */
){
	DynTable login = { 0 };
	DynTable passwd = { 0 };
	DynTable sqlres = { 0 };
	DynBuffer *sql = NULL;
	char qry[1024];

	/* Read login & password from CGI data */
	if(cgi_filter_values(cntxt, &login, 'D', ~0UL, ~0UL, ~0UL, "_EVA_LOGINSUBMIT", NULL, 0, 0) ||
		cgi_filter_values(cntxt, &passwd, 'D', ~0UL, ~0UL, ~0UL, "_EVA_LOGINSUBMITP", NULL, 0, 0))
		STACK_ERROR;

	/* Clear all identification CGI data */
	if(cgi_filter_values(cntxt, &sqlres, 0, ~0UL, ~0UL, ~0UL, "_EVA_LOGINSUBMIT", NULL, 0, 0)) STACK_ERROR;
	CGI_VALUES_DELETE(&sqlres);
	if(cgi_filter_values(cntxt, &sqlres, 0, ~0UL, ~0UL, ~0UL, "_EVA_LOGINSUBMITP", NULL, 0, 0)) STACK_ERROR;
	CGI_VALUES_DELETE(&sqlres);

	/* Return if no login given */
	*errmsg = NULL;
	cntxt->b_login_pwd = 0;
	if(!dyntab_sz(&login, 0, 0) || !dyntab_sz(&passwd, 0, 0)) RETURN_OK;

	/* Process login / pwd pair */
	{
		unsigned long id_login = IDVAL("_EVA_LOGIN");
		unsigned long id_pwd = IDVAL("_EVA_PASSWORD");
		login.nbrows = 1;
		passwd.nbrows = 1;

		/* Build & exec query for matching account */
		DYNBUF_ADD3_INT(&sql, 
					"-- check_login_pwd : search for account\n"
					"SELECT DISTINCT T1.IdObj FROM TLink AS T1\n"
					"INNER JOIN TLink AS T2 ON T1.IdObj=T2.IdObj \n"
					"WHERE T1.DateDel IS NULL AND T1.IdField=" , id_login, " AND T1.IdValue IN (");
		if(qry_values_list(cntxt, &login, 1, &sql)) STACK_ERROR;
		DYNBUF_ADD3_INT(&sql, 
					")\n"
					"AND T2.DateDel IS NULL AND T2.IdField=", id_pwd, " AND T2.IdValue IN (");
		if(qry_values_list(cntxt, &passwd, 1, &sql)) STACK_ERROR;
		DYNBUF_ADD_STR(&sql, ")");
		if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
		M_FREE(sql);

		/* If user identification ok */
		if(sqlres.nbrows == 1)
		{
			/* Set id_user & load user data */
			DYNTAB_ADD_CELL(&cntxt->id_user, 0, 0, &sqlres, 0, 0);
			if(qry_obj_data(cntxt, &cntxt->user_data, &cntxt->id_user, NULL)) STACK_ERROR;

			/* Check account validity */
			if(ckeck_user_validity(cntxt, errmsg)) STACK_ERROR;
			if(*errmsg) RETURN_OK;
			cntxt->b_login_pwd = 1;

			/* End current session if user was identified & different */
			DYNTAB_FREE(login);
			if(cntxt->session.nbrows)
			{
				DYNTAB_FIELD(&login, &cntxt->sess_data, USER);
				if(login.nbrows &&
					DYNTAB_TOUL(&login) != cntxt->id_public &&
					dyntab_cmp(&login, 0, 0, &cntxt->id_user, 0, 0))
				{
					DYNTAB_FREE(cntxt->session);
				}
				else
				{
					/* User has identified : update in session */
					if(login.nbrows)
					{
						snprintf(add_sz_str(qry), "UPDATE TLink SET IdRelObj=%lu,DateCr='%s',IdWhoCr=%lu WHERE Pkey=%lu", 
														DYNTAB_TOUL(&cntxt->id_user), cntxt->timestamp,
														DYNTAB_TOUL(&cntxt->id_user), login.cell->Pkey);
						if(sql_exec_query(cntxt, qry)) STACK_ERROR;
					}
					else
						if(qry_add_obj_field_val(cntxt, &cntxt->session, "_EVA_USER", DYNTAB_VAL_SZ(&cntxt->id_user, 0, 0), 1, 1)) STACK_ERROR;
				}
			}
		}
		else if(sqlres.nbrows)
		{
			/* Duplicate credentials : return message */
			*errmsg = "Conflit d'identification - contactez l'administrateur";
		}
		else
		{
			/* Unknown credentials : return message */
			*errmsg = "Identification incorrecte";
		}
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : check_user_identification
** Description : global identification process
*********************************************************************/
#define ERR_FUNCTION "check_user_identification"
#define ERR_CLEANUP DYNTAB_FREE(sqlres)
int check_user_identification(					/* return : 1 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	char **errmsg								/* out : error message if any */
){
	DynTable sqlres = { 0 };
	char qry[1024];

	/* Handle workstation Id */
	if(handle_workstation_id(cntxt)) STACK_ERROR;

	/* Handle No identification button */
	*errmsg = NULL;
	if(cntxt->cgi && cntxt->cgi[cntxt->cgibtn].name && !strcmp(cntxt->cgi[cntxt->cgibtn].name, "B$#NO_IDENT"))
	{
		check_login_pwd(cntxt, errmsg);
		*errmsg = NULL;
		DYNTAB_FREE(cntxt->id_user);
		DYNTAB_FREE(cntxt->session);
		RETURN_OK;
	}

	/* Check session validity */
	if(ckeck_session_validity(cntxt, errmsg)) STACK_ERROR;
	if(*errmsg) RETURN_OK;

	/* Handle user login */
	if(check_login_pwd(cntxt, errmsg)) STACK_ERROR;
	if(*errmsg) RETURN_OK;

	/* Handle automatic identification if no session or user given */
	if(!cntxt->session.nbrows && !cntxt->id_user.nbrows)
	{
		/* Retrieve last session for workstation if present */
		if(cntxt->objwks)
		{
			snprintf(add_sz_str(qry),
				"-- check_login : Retrieve last session for workstation\n"
				"SELECT MAX(IdObj) FROM TLink WHERE	IdRelObj=%lu AND IdField=%lu",
				cntxt->objwks, IDVAL("_EVA_WKS"));
			if(sql_exec_query(cntxt, qry) ||
				sql_get_table(cntxt, &cntxt->session, 2)) STACK_ERROR;
		}
		else if(cntxt->user_ip)
		/* Retrieve last session for IP adress if present */
		{
			unsigned long id_ip = get_id_value(cntxt, cntxt->user_ip, strlen(cntxt->user_ip));
			if(id_ip)
			{
				snprintf(add_sz_str(qry),
					"-- check_login : Retrieve last session for workstation\n"
					"SELECT MAX(IdObj) FROM TLink WHERE	IdValue=%lu AND IdField=%lu",
					id_ip, IDVAL("_EVA_USER_IP"));
				if(sql_exec_query(cntxt, qry) ||
					sql_get_table(cntxt, &cntxt->session, 2)) STACK_ERROR;

				/* Ignore same IP if workstation present */
				if(qry_obj_field(cntxt, &cntxt->sess_data, DYNTAB_TOUL(&cntxt->session), "_EVA_WKS")) STACK_ERROR;
				if(cntxt->sess_data.nbrows)
				{
					DYNTAB_FREE(cntxt->sess_data);
					DYNTAB_FREE(cntxt->session);
				}
			}
		}

		/* Check session validity */
		if(ckeck_session_validity(cntxt, errmsg)) STACK_ERROR;
		if(*errmsg) DYNTAB_FREE(cntxt->session);
		*errmsg = NULL;

		/* Check automatic session reconnection validity */
		if(cntxt->session.nbrows)
		{
			/* Ckeck automatic identification is allowed */
			int auto_cnf = atoi(DYNTAB_FIELD_VAL(&cntxt->cnf_data, AUTO_IDENT));
			int auto_user = atoi(DYNTAB_FIELD_VAL(&cntxt->user_data, AUTO_IDENT));
			int b_ok = auto_cnf == 2 ? (auto_user != -1) : auto_user > 0;
			if(b_ok)
			{
				/* Check auto ident allowed for this workstation in user account */
				DYNTAB_FIELD_TAB(&sqlres, &cntxt->user_data, USER_IDENT_WKS);
				if(sqlres.nbrows) 
				{
					unsigned long i;
					for(b_ok = 0, i = 0; i < sqlres.nbrows && !b_ok; i++)
						b_ok = DYNTAB_TOULRC(&sqlres, i, 0) == cntxt->objwks;
				}
			}

			/* Cancel session reconnection if not valid */
			if(!b_ok)
			{
				DYNTAB_FREE(cntxt->session);
			}

			/* Else : ask for confirmation if user identified */
			else if(cntxt->id_user.nbrows && DYNTAB_TOUL(&cntxt->id_user) != cntxt->id_public && auto_user != 2)
			{
				/* Output confirm button for auto identification */
				output_message(cntxt, "Confirmer l'identification mémorisée");
				cntxt->txsize += printf("<b>Si vous êtes <u>%s</u></b><br><br>"
					"<input type=submit name=B$#AUTO_IDENT value='Cliquez ici'><br><br><hr>",
					DYNTAB_FIELD_VAL(&cntxt->user_data, USERNAME));
				
				/* Output input fields for other user identification  */
				output_ident_inputs(cntxt, "Vous pouvez vous identifier sous un autre nom");
				cntxt->txsize += printf("<hr><br><b>Vous pouvez aussi continuer en</b><br>"
					"<input type=submit name=B$#NO_IDENT value='Mode non identifié'><br><br><hr>");
				DYNTAB_FREE(cntxt->sess_data);
				*errmsg = "";
			}
		}

		/* Reset automatic session reconnection if not valid */
		if(!cntxt->session.nbrows)
		{
			DYNTAB_FREE(cntxt->id_user);
			DYNTAB_FREE(cntxt->user_data);
			DYNTAB_FREE(cntxt->sess_data);
		}
	}

	/* Read administrator accounts */
	{
		unsigned long i;
		DYNTAB_FIELD(&sqlres, &cntxt->cnf_data, ADMINISTRATOR)
		for(i = 0; i < sqlres.nbrows; i++)
			if(!dyntab_cmp(&cntxt->id_user, 0, 0, &sqlres, i, 0))
			{
				cntxt->b_admin = 1;
				break;
			}
		if(sqlres.nbrows && qry_obj_field(cntxt, &cntxt->mail_admin, DYNTAB_TOUL(&sqlres), "EMAIL")) STACK_ERROR;
		if(!dyntab_cell(&cntxt->mail_admin, 0, 0)) DYNTAB_ADD_STR(&cntxt->mail_admin, 0, 0, "eva@abing.fr");
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : check_login
** Description : check login & passwd and session
*********************************************************************/
#define ERR_FUNCTION "check_login"
#define ERR_CLEANUP DYNTAB_FREE(sqlres); \
					DYNTAB_FREE(debug); \
					DYNTAB_FREE(id_form); \
					M_FREE(sql)
int check_login(				/* return : 0 on success, other on error */
	EVA_context *cntxt			/* in : execution context - cgi data, argc, argv, ...
								  out : user identification - id_user, b_user_ok */
){
	DynTable sqlres = { 0 };
	DynTable debug = { 0 };
	DynTable id_form = { 0 };
	DynBuffer *sql = NULL;
	char printbuf[1024];
	DynTableCell *homepage;
	char *referer = getenv("HTTP_REFERER");
	unsigned long sourceref = 0;
	char *errmsg = NULL;

	/* Handle user identification */
	if(check_user_identification(cntxt, &errmsg)) STACK_ERROR;

	/* Set user identification status - use public user account if not identified */
	if(!dyntab_sz(&cntxt->id_user, 0, 0) && cntxt->id_public)
		DYNTAB_ADD_INT(&cntxt->id_user, 0, 0, cntxt->id_public);
	if(qry_obj_data(cntxt, &cntxt->user_data, &cntxt->id_user, NULL)) STACK_ERROR;
	cntxt->b_identified = cntxt->id_user.nbrows && DYNTAB_TOUL(&cntxt->id_user) != cntxt->id_public;

	/* Output security agent dialog & return if applicable */
	if(errmsg)
	{
		if(*errmsg)
		{
			output_message(cntxt, errmsg);
			output_ident_inputs(cntxt, "Merci de vous identifier pour continuer votre travail");
		}
		RETURN_OK;
	}

	/* If user login & has homepage : reset current page if home page */
	homepage = DYNTAB_FIELD_CELL(&cntxt->user_data, HOMEPAGE);
	if(cntxt->b_login_pwd && homepage)
	{
		if(!dyntab_cmp(&cntxt->homepage, 0, 0, &cntxt->id_form, 0, 0))
		{
			DYNTAB_FREE(cntxt->id_form);
			DYNTAB_FREE(cntxt->id_obj);
		}
		DYNTAB_FREE(cntxt->alt_form);
		DYNTAB_FREE(cntxt->alt_obj);
	}

	/* If user account is available */
	if(cntxt->id_user.nbrows)
	{
		/* Select home page, menubar & images path */
		unsigned long i;
		DynTableCell *menubar = DYNTAB_FIELD_CELL(&cntxt->user_data, MENUBAR);
		char *maxjoinusr = DYNTAB_FIELD_VAL(&cntxt->user_data, MAXJOIN);
		unsigned long maxjoin = strtoul(*maxjoinusr ? maxjoinusr : DYNTAB_FIELD_VAL(&cntxt->cnf_data, MAXJOIN), NULL, 10);
		if(homepage) DYNTAB_SET_CELLP(&cntxt->homepage, 0, 0, homepage);
		if(menubar) DYNTAB_SET_CELLP(&cntxt->menubar, 0, 0, menubar);
		DYNTAB_FIELD(&cntxt->imgpath, &cntxt->user_data, IMGPATH);
		DYNTAB_FIELD(&cntxt->user_groups, &cntxt->user_data, USERGROUP);

		/* Save uploaded files */
		if(cgi_save_files(cntxt)) STACK_ERROR;

		/* Set max query time if applicable */
		if(maxjoin)
		{
			snprintf(add_sz_str(printbuf), "SET SESSION max_join_size=%lu", 1000 * maxjoin);
			if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
		}

		/* Set debug options */
		DYNTAB_FIELD(&debug, &cntxt->user_data, DEBUG);
		if(debug.nbrows) for(i = 0; i < debug.nbrows; i++)
		{
			char * dbg = dyntab_val(&debug, i, 0);
			if(!strcmp(dbg, "_EVA_TRACE_CGI")) cntxt->debug |= DEBUG_CGI_RAW;
			else if(!strcmp(dbg, "_EVA_DEBUG_CGI")) cntxt->debug |= DEBUG_CGI;
			else if(!strcmp(dbg, "_EVA_DEBUG_SQL")) cntxt->debug |= DEBUG_SQL;
			else if(!strcmp(dbg, "_EVA_DEBUG_SQL_RES")) cntxt->debug |= DEBUG_SQL_RES;
			else if(!strcmp(dbg, "_EVA_DEBUG_SQL_SLOW")) cntxt->debug |= DEBUG_SQL_SLOW;
			else if(!strcmp(dbg, "_EVA_DEBUG_ENV")) cntxt->debug |= DEBUG_ENV;
			else if(!strcmp(dbg, "_EVA_DEBUG_HTML")) cntxt->debug |= DEBUG_HTML;
		}

		/* Handle trace output mode - shared navigation */
		{
			DynTableCell *tr = DYNTAB_FIELD_CELL(&cntxt->user_data, TRACEOUTPUT);
			if(tr)
			{
				/* Limit trace time to 180 min */
				char fmt = 'm';
				qry_get_rec_info(cntxt, &debug, tr->Pkey, "DateCr");
				if(datetxt_to_age(dyntab_val(&debug, 0, 0), datetxt_now(), &fmt) > 180 || fmt != 'm')
				{
					sprintf(printbuf, "UPDATE TLink SET DateDel=%s,IdWhoDel=%s WHERE Pkey=%lu",
									cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), tr->Pkey);
					if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
				}
				else
					cntxt->debug |= DEBUG_HTML_RAW;
			}
		}

		/* Delete trace file if trace not activated */
		if(!(cntxt->debug & DEBUG_HTML_RAW))
		{
			char fname[128];
			sprintf(fname, "%s-%s.html", cntxt->dbname, dyntab_val(&cntxt->id_user, 0, 0));
			if(!chdir(cntxt->rootdir) && !chdir("trace")) remove(fname);
		}

		/* Set option button mode */
		{
			char *optbtnmode = DYNTAB_FIELD_VAL(&cntxt->user_data, OPTIONBUTTON);
			cntxt->opt_btn_mode =	!strcmp(optbtnmode, "_EVA_NONE") ? OptBtn_None :
									(!*optbtnmode || !strcmp(optbtnmode, "_EVA_HELP")) ? OptBtn_Help :
									!strcmp(optbtnmode, "_EVA_OPENSAME") ? OptBtn_OpenSame : OptBtn_OpenBottom;
			DYNTAB_FIELD(&cntxt->viewforms, &cntxt->user_data, USER_VIEW_FORMS);
		}

		/* Output debug info */
		debug_put_info(cntxt);
	}
	else
	{
		/* Set next form to default login */
		if(dyntab_cmp(&cntxt->homepage, 0, 0, &cntxt->id_form, 0, 0) &&
			form_set_homepage(cntxt)) STACK_ERROR;
	}

	/* Add site image path */
	if(dyntab_filter_field(&cntxt->imgpath, 0, &cntxt->cnf_data, "_EVA_IMGPATH", ~0UL, NULL)) RETURN_ERR_MEMORY;

	/* Handle IP filter */	
	if(cntxt->user_ip && strcmp(cntxt->user_ip, "127.0.0.1"))
	{
		/* Get user or server filter */
		char *filtmode = DYNTAB_FIELD_VAL(&cntxt->user_data, IP_FILTER);
		DYNTAB_FIELD(&debug, &cntxt->user_data, ACCESS_IP);
		if(!*filtmode)
		{
			filtmode = DYNTAB_FIELD_VAL(&cntxt->cnf_data, IP_FILTER);
			DYNTAB_FIELD(&debug, &cntxt->cnf_data, ACCESS_IP);
		}

		/* Check IP adress if filtering active */
		if(cntxt->user_ip && *filtmode && debug.nbrows)
		{
			/* Check if user IP is in given address list */
			unsigned long i;
			for(i = 0; i < debug.nbrows; i++)
			{
				DynTableCell *c = dyntab_cell(&debug, i, 0);
				if(!strncmp(cntxt->user_ip, c->txt, c->len)) break;
			}

			/* Handle allow / deny mode to grant access */
			if(strcmp(filtmode, "_EVA_ALLOW") ? i < debug.nbrows : i == debug.nbrows)
			{
				/* Access denied : output message & terminate */
				output_message(cntxt,"Vous n'êtes pas autorisé à accéder à ce serveur");
				cntxt->txsize += printf(
					"<br>Motif : adresse IP non conforme<br><br>"
					"<br>Contactez l'administrateur pour obtenir l'accès<br><br>");
			}
		}
	}

	/* Handle referer : clear if same application */
	if(referer)
	{
		snprintf(add_sz_str(printbuf), "/eva/%s.cgi", cntxt->dbname);
		if(strstr(referer, printbuf)) referer = NULL;
	}

	/* Handle source reference : read object number */
	{
		char *src = strstr(cntxt->qrystr, "Source=");
		if(src) sourceref = strtoul(src + 7, NULL, 10);
	}

	/* Create session if not first call or known workstation or referer */
	if(!cntxt->session.nbrows && (cntxt->nb_cgi || cntxt->objwks || referer || sourceref))
	{
		unsigned long idsession = strtoul(DYNTAB_FIELD_VAL(&cntxt->cnf_data, FORM_SESSION), NULL, 10);
		if(idsession && (
			qry_add_new_obj(cntxt, &cntxt->session, idsession) ||
			qry_add_obj_field_val(cntxt, &cntxt->session, "_EVA_USER_IP", cntxt->user_ip, 0, 0, 1) ||
			qry_add_obj_field_val(cntxt, &cntxt->session, "_EVA_USER_AGENT", cntxt->user_agent, 0, 0, 1) ||
			qry_add_obj_field_val(cntxt, &cntxt->session, "_EVA_USER", DYNTAB_VAL_SZ(&cntxt->id_user, 0, 0), 1, 1)))
			STACK_ERROR;
	}

	/* Add workstation to session if applicable */
	if(cntxt->session.nbrows && cntxt->objwks && !DYNTAB_FIELD_CELL(&cntxt->sess_data, WKS) &&
			qry_add_idobj_field_rel(cntxt, DYNTAB_TOUL(&cntxt->session), "_EVA_WKS", cntxt->objwks, 1, 0, 1))
		STACK_ERROR;

	/* Store source reference in session if applicable */
	if(sourceref && qry_add_idobj_field_rel(cntxt, DYNTAB_TOUL(&cntxt->session), "_EVA_SOURCE_REF", sourceref, 0, 0, 1)) STACK_ERROR;

	/* Store referer in session if applicable */
	if(referer && qry_add_obj_field_val(cntxt, &cntxt->session, "_EVA_REFERER", referer, 0, 0, 1)) STACK_ERROR;

	/* Set base directory */
	chdir(cntxt->rootdir); chdir("cgi");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
