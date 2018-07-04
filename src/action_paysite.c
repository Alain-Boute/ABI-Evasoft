/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_paysite.c
** Description : handler for buttons & actions of type PAY_SITE
**				 send payment data to secure payment bank site
**      Author : Alain BOUTE
**     Created : Aug 8 2006
*********************************************************************/

#include "eva.h"

#include <openssl/hmac.h>               /* pour la fonction HMAC() */

#define LogTPE(fmt,p1,p2,p3) {\
	FILE *f = fopen("TPE.txt","ac");\
	if(f) { fprintf(f, "\n%s [%s] - " fmt " - societe=%s - montant=%s", \
	cx->d, cx->ref ? cx->ref : "NoTPE!", p1, p2, p3, \
	cx->societe ? cx->societe : "", cx->amount ? cx->amount : ""); fclose(f); }}\

/*********************************************************************
** Typedef : PaySiteCtx
** Description : specific contaxt data for PAY_SITE
*********************************************************************/
typedef	struct _PaySiteCtx
{
	EVA_ctrl ctl;
	EVA_ctrl *ctrl;
	char *tpe;
	char *societe;
	DynTableCell *key;
	DynTableCell *retform;
	DynTableCell *stsfield;
	char ref[32];				/* order reference (IdObj) */
	char amount[32];			/* order amount (IdObj) */
	char d[32];					/* date formated */
	char k[32];					/* binary key */
	size_t kl;					/* binary key length */
	int b_test;
}
	PaySiteCtx;

/*********************************************************************
** Function : HMAC_SHA1
** Description : keyed MAC digest calculation
*********************************************************************/
#define ERR_FUNCTION "HMAC_SHA1"
#define ERR_CLEANUP
int HMAC_SHA1(				/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	char *res,				/* out : digest in hex format (40 bytes + 1) */
	char *k,				/* in : key */
	size_t kl,				/* in : key length */
	char *txt,				/* in : data to digest */
	size_t len				/* in : data length */
){
	char b[32];
	unsigned long i, j = 0;

	HMAC(EVP_sha1(), (unsigned char*)k, kl, (unsigned char*)txt, len, (unsigned char*)b, (unsigned int*)&j);
	for(i = 0; i < j; i++) sprintf(res + i * 2, "%02X", b[i] & 0xFF);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_url
** Description : output hidden input for return URL in payment transaction
*********************************************************************/
void output_url(
	EVA_context *cntxt,				/* in : execution context data */
	PaySiteCtx *cx,					/* in/out : execution context data */
	char *name,						/* in : input name */
	char *tag						/* in : URL tag */
){
	EVA_form *form = cntxt->form;
	printf(	"\n<input type=hidden name=%s value='http&#x3a;&#x2f;&#x2f;%s&#x2f;eva&#x2f;%s.cgi&#x3f;PaySite%s&#x3d;%s,%s,%s'>",
			name, cntxt->srvname, cntxt->dbname, tag,
			dyntab_val(&cntxt->session, 0, 0),
			dyntab_val(&cx->ctrl->id, 0, 0),
			dyntab_val(&form->id_obj, 0, 0));
}

/*********************************************************************
** Function : call_pay_site
** Description : 1st step : redirect to bank site with order data
*********************************************************************/
#define ERR_FUNCTION "call_pay_site"
#define ERR_CLEANUP	DYNTAB_FREE(res); \
					DYNTAB_FREE(expr); \
					M_FREE(hash)
int call_pay_site(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	PaySiteCtx *cx			/* in : execution context data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = cx->ctrl;
	DynTable res = {0};
	DynTable expr = {0};
	DynBuffer *hash = NULL;
	char mac[42];
	char longref[3200] = {0};
	char *version = "1.2open";
	unsigned long i, j;
	DynTableCell *c;

	/* Store reference to caller button in object */
	if(cx->ctrl && cx->ctrl->attr.cell && cx->ctrl->attr.cell->IdObj)
	{
		DynTableCell val = {0};
		c = cx->ctrl->attr.cell;
		val.IdObj = DYNTAB_TOUL(&cntxt->id_obj);
		val.len = snprintf(add_sz_str(cx->ref), "%lu", cx->ctrl->attr.cell->IdObj);
		val.txt = cx->ref;
		val.b_relation = 1;
		val.field = "_EVA_PAY_BTN";
		if(qry_add_val(cntxt, &val, 3, NULL)) STACK_ERROR;
	}

	/* Build order reference - add random number for test */
	snprintf(add_sz_str(cx->ref), "%s", dyntab_val(&form->id_obj, 0, 0), 0xFFF & rand());

	/* Compute & format amount */
	CTRL_ATTR(expr, PAY_AMOUNT);
	if(ctrl_eval_valtyp(cntxt, ctrl, &res, "_EVA_CURRENTOBJ", NULL, &expr)) STACK_ERROR;
	snprintf(add_sz_str(cx->amount), "%sEUR", dyntab_val(&res, 0, 0));

	/* Compute & format long reference */
	CTRL_ATTR(expr, PAY_LONGREF);
	if(ctrl_eval_valtyp(cntxt, ctrl, &res, "_EVA_CURRENTOBJ", NULL, &expr)) STACK_ERROR;
	c = dyntab_cell(&res, 0, 0);
	if(c)
	{
		setcase(4, c->txt, c->len);
		for(i = 0, j = 0; i < c->len; i++)
			if(isalnum(c->txt[i]))
				longref[j++] = c->txt[i];
			else if(j && longref[j - 1] != '.')
				longref[j++] = '.';
	}

	/* Build MAC */
	DYNBUF_ADD3(&hash, "", cx->tpe, 0, NO_CONV, "*");
	DYNBUF_ADD(&hash, cx->d, 0, NO_CONV);
	DYNBUF_ADD3(&hash, "*", cx->amount, 0, NO_CONV, "*");
	DYNBUF_ADD(&hash, cx->ref, 0, NO_CONV);
	DYNBUF_ADD3(&hash, "*", longref, 0, NO_CONV, "*");
	DYNBUF_ADD(&hash, version, 0, NO_CONV);
	DYNBUF_ADD3(&hash, "*FR*", cx->societe, 0, NO_CONV, "*");
	if(HMAC_SHA1(cntxt, mac, cx->k, cx->kl, hash->data, hash->cnt)) STACK_ERROR;

	/* Output HTML & form header */
	put_html_page_header(cntxt, NULL, "Paiement par carte bancaire", NULL, 2);
	printf("\n<form method=post name=tpe action='%s'>",
		cx->b_test ?	"https://ssl.paiement.cic-banques.fr/test/paiement.cgi" :
						"https://ssl.paiement.cic-banques.fr/paiement.cgi");

	/* Output form data */
	printf(	"\n<input type=hidden name=version value='%s'>", version);
	printf(	"\n<input type=hidden name=TPE value='%s'>", cx->tpe);
	printf(	"\n<input type=hidden name=date value='%s'>", cx->d);
	printf(	"\n<input type=hidden name=montant value='%s'>", cx->amount);
	printf(	"\n<input type=hidden name=reference value=%s>", cx->ref);
	printf(	"\n<input type=hidden name=MAC value='%s'>", mac);
	printf(	"\n<input type=hidden name='texte-libre' value='%s'>", longref);
	printf(	"\n<input type=hidden name=lgue value=FR>"
			"\n<input type=hidden name=societe value='%s'>", cx->societe);
	output_url(cntxt, cx, "url_retour", "Return");
	output_url(cntxt, cx, "url_retour_ok", "Ok");
	output_url(cntxt, cx, "url_retour_err", "Err");
	printf("<script>document.write('<input type=hidden name=bouton value=CB>')</script>"
			"<noscript><center>Cliquez pour accéder au site de paiement<br>"
			"<input type=submit name=bouton value='Paiement&#x20;CB'></noscript>"
			"</form>"
			"<script>document.tpe.submit()</script>");

	/* Terminate processing */
	LogTPE("Appel banque%s%s%s", "", "", "")
	cntxt->b_terminate = 0x2F;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : init_pay_site
** Description : initialize	
*********************************************************************/
#define ERR_FUNCTION "init_pay_site"
#define ERR_CLEANUP	DYNTAB_FREE(res); \
					DYNTAB_FREE(expr)
int init_pay_site(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	PaySiteCtx *cx			/* in/out : execution context data */
){
	DynTable res = {0};
	DynTable expr = {0};
	unsigned long i;
	EVA_ctrl *ctrl = cx->ctrl;

	/* Get call context from query string if applicable */
	if(!ctrl && cntxt->qrystr && *cntxt->qrystr)
	{
		char *qry = strchr(cntxt->qrystr, '=');
		if(qry) qry++;
		if(qry && *qry)
		{
			if(dyntab_from_list(&res, qry, strlen(qry), ",", 0, 4)) RETURN_ERR_MEMORY;
			DYNTAB_ADD_CELL(&cntxt->session, 0, 0, &res, 0, 0);
			if(dyntab_sz(&res, 1, 0))
			{
				ctrl = &cx->ctl;
				cx->ctrl = &cx->ctl;
				if(qry_cache_idobj(&ctrl->attr, DYNTAB_TOULRC(&res, 1, 0)) ||
					ctrl_read_baseobj(cntxt, &ctrl->attr)) STACK_ERROR;
			}
			DYNTAB_ADD_CELL(&cntxt->id_obj, 0, 0, &res, 2, 0);
		}
	}
	else if(!ctrl && cntxt->cgi)
		cntxt->qrystr = "TPE=";

	/* Read payment params from control */
	if(!ctrl) RETURN_OK;
	cx->tpe = CTRL_ATTR_VAL(CODE_TPE);
	cx->societe = CTRL_ATTR_VAL(FIRM_NAME);
	cx->key = CTRL_ATTR_CELL(HASH_KEY);
	cx->retform = CTRL_ATTR_CELL(RET_FORM);
	cx->stsfield = CTRL_ATTR_CELL(STS_FIELD);
	cx->b_test = *CTRL_ATTR_VAL(PAY_TEST) == '1';

	/* Build binary key */
	cx->kl = cx->key ? cx->key->len / 2 : 0;
	for(i = 0; i < cx->kl; i++)
		cx->k[i] = (char)(TO_HEX(cx->key->txt[i * 2 + 1]) | (TO_HEX(cx->key->txt[i * 2]) << 4));

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : return_pay_site
** Description : handle return from bank site
*********************************************************************/
#define ERR_FUNCTION "return_pay_site"
#define ERR_CLEANUP
int return_pay_site(		/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	PaySiteCtx *cx,			/* in/out : execution context data */
	char *sts, size_t sz	/* in : transaction status displayed */
){

	/* Set displayed form */
	if(!cx->retform) cx->retform = DYNTAB_FIELD_CELL(&cx->ctrl->attr, FORMSTAMP);
	if(cx->retform) DYNTAB_ADD_CELLP(&cntxt->id_form, 0, 0, cx->retform);

	/* Set return status */
	if(sts && 
		cgi_build_name(cntxt, &cx->ctrl->cginame, 'D', 0, DYNTAB_TOUL(&cntxt->id_form), DYNTAB_TOUL(&cntxt->id_obj),
							add_sz_str("_EVA_PAY_MSG"), NULL, 0, 1, 0, 0) ||
		cgi_add_input(cntxt, NULL, cx->ctrl->cginame->data, cx->ctrl->cginame->cnt, sts, sz)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : result_pay_site
** Description : handle transaction result from bank site
*********************************************************************/
#define ERR_FUNCTION "result_pay_site"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					M_FREE(hash)
int result_pay_site(		/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	PaySiteCtx *cx			/* in/out : execution context data */
){
	DynTable data = {0};
	DynBuffer *hash = NULL;
	char *res = "Err-GET call not handled";

	/* If CGI data present */
	if(cntxt->nb_cgi == 8)
	{
		/* Read CGI data */
		char *date = cntxt->cgi[1].value;
		char *macok = cntxt->cgi[4].value;
		char *longref = cntxt->cgi[5].value;
		char *sts = cntxt->cgi[6].value;
		char *plus = cntxt->cgi[7].value;
		char mac[64];
		unsigned long idobj;
		cx->tpe = cntxt->cgi[0].value;
		snprintf(add_sz_str(cx->ref), "%s", cntxt->cgi[3].value);
		snprintf(add_sz_str(cx->amount), "%s", cntxt->cgi[2].value);
		res = NULL;
		idobj = strtoul(cx->ref, NULL, 10);

		/* Read button info if available */
		if(idobj && !cx->ctrl)
		{
			cx->ctrl = &cx->ctl;
			if(qry_obj_field(cntxt, &data, idobj, "_EVA_PAY_BTN")) STACK_ERROR;
			if(qry_cache_idobj(&cx->ctrl->attr, DYNTAB_TOUL(&data)) ||
				ctrl_read_baseobj(cntxt, &cx->ctrl->attr)) STACK_ERROR;

			/* If button info available */
			if(cx->ctrl->attr.nbrows)
			{
				/* Build MAC */
				if(init_pay_site(cntxt, cx)) STACK_ERROR;
				DYNBUF_ADD(&hash, plus, 0, NO_CONV);
				//DYNBUF_ADD_STR(&hash, "+");
				DYNBUF_ADD(&hash, cx->tpe, 0, NO_CONV);
				DYNBUF_ADD3(&hash, "+", date, 0, NO_CONV, "+");
				DYNBUF_ADD(&hash, cx->amount, 0, NO_CONV);
				DYNBUF_ADD3(&hash, "+", cx->ref, 0, NO_CONV, "+");
				DYNBUF_ADD(&hash, longref, 0, NO_CONV);
				DYNBUF_ADD3(&hash, "+1.2open+", sts, 0, NO_CONV, "+");
				if(HMAC_SHA1(cntxt, mac, cx->k, cx->kl, hash->data, hash->cnt)) STACK_ERROR;
				if(strcmp(mac, macok))
				{
					res = "Document falsifie";
				}
				else
					res = "OK";	  
			}
		}
		if(!res || !cx->kl) res = "ErrNoKey";

		/* Store status & result */
		if(idobj)
		{
			DynTableCell val = {0};
			val.IdObj = idobj;
			val.txt = res;
			val.len = strlen(val.txt);
			val.field = "_EVA_PAY_RES";
			if(qry_add_val(cntxt, &val, 0, NULL)) STACK_ERROR;
			val.IdValue = 0;
			val.IdField = 0;
			val.txt = sts;
			val.len = strlen(val.txt);
			val.field = "_EVA_PAY_STS";
			if(qry_add_val(cntxt, &val, 0, NULL)) STACK_ERROR;
		}
 		LogTPE("Notification - status=%s - result=%s%s", sts, res, "")
	}

	/* Acknowledge */
    printf("Pragma: no-cache\r\n");
    printf("Cache-Control: no-cache\r\n");
    printf("Expires: -1\r\n");
    printf("Content-type: text/plain\r\n\r\n");
	printf("Date: %s\r\n"                       
			"Version: %s\r\n"                    
			"%s\r\n", cx->d, "1", res);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_pay_site
** Description : button click handler for PAY_SITE
*********************************************************************/
#define ERR_FUNCTION "action_pay_site"
#define ERR_CLEANUP	
int action_pay_site(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	PaySiteCtx _cx = {0}, *cx = &_cx;
	char *cb;
	char *t = cntxt->timestamp;

	/* Handle save before on button click */
	if(i_ctrl)
	{
		int b_done = BUTN_SAVE_NEXT;
		if(form->b_modified && 
			form_save_dialog(cntxt, i_ctrl, "_EVA_CONFIRMNONE", BUTN_SAVE_NEXT, &b_done)) STACK_ERROR;
		if(b_done != BUTN_SAVE_NEXT) RETURN_OK;
		form = cntxt->form;
	}

	/* Format date */
	snprintf(add_sz_str(cx->d), "%.2s/%.2s/%.4s:%.2s:%.2s:%.2s", t + 6, t + 4, t, t + 8, t + 10, t + 12);

	/* Init context */
	cx->ctrl = i_ctrl ? form->ctrl + i_ctrl : NULL;
 	if(init_pay_site(cntxt, cx)) STACK_ERROR;
	cb = cntxt->qrystr ? cntxt->qrystr : "";
	if(!*cb)
	{
		/* Pay button click : redirect to bank site */
		if(call_pay_site(cntxt, cx)) STACK_ERROR;
	}
	else if(!strncmp(cb, add_sz_str("TPE=")))
	{
		/* Transaction result : store */
		if(result_pay_site(cntxt, cx)) STACK_ERROR;
	}
	else if(!strncmp(cb, add_sz_str("PaySiteErr=")))
	{
		/* Bank site error : display error message */
		if(return_pay_site(cntxt, cx, add_sz_str("Err"))) STACK_ERROR;
	}
	else if(!strncmp(cb, add_sz_str("PaySiteOk=")))
	{
		/* Bank site return : display error message */
		if(return_pay_site(cntxt, cx, add_sz_str("Ok"))) STACK_ERROR;
	}
	else if(!strncmp(cb, add_sz_str("PaySiteReturn=")))
	{
		/* Bank site error : display error message */
		if(return_pay_site(cntxt, cx, NULL, 0)) STACK_ERROR;
	}
	else
		LogTPE("Error *** unhandled - ictrl=%lu QRY=%.20s nbcgi=%lu", i_ctrl, cntxt->qrystr ? cntxt->qrystr : "(none)", cntxt->nb_cgi)

	if(cx->b_test) LogTPE("***TEST*** - ictrl=%lu QRY=%.20s nbcgi=%lu", i_ctrl, cntxt->qrystr ? cntxt->qrystr : "(none)", cntxt->nb_cgi)

	RETURN_OK_CLEANUP;
}
