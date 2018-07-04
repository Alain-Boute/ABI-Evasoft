/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dll_adsea01.c
** Description : fonctions specifiques pour l'ADSEA de l'Ain : facturation
**      Author : Alain BOUTE
**     Created : July 31 2003
*********************************************************************/

#include "eva.h"

/* Macros pour la mise a jour des valeurs */
#define QRY_SET_RECORD(idobj, f, idval, t, l, b_rel, valmode) { val.IdObj = idobj; val.IdField = cf->val_##f; val.IdValue = idval; val.txt = t; val.len = l; val.b_relation = b_rel;\
																if(qry_add_val(cntxt, &val, valmode, NULL)) STACK_ERROR; }
#define QRY_CREATE_REL(idobj, f, idrelobj) QRY_SET_RECORD(idobj, f, idrelobj, NULL, 0, 1, 0)
#define QRY_UPDATE_REL(idobj, f, idrelobj) QRY_SET_RECORD(idobj, f, idrelobj, NULL, 0, 1, 3)
#define QRY_UPDATE_VAL(idobj, f, t, l) QRY_SET_RECORD(idobj, f, 0, t, l, 0, 3)
#define QRY_UPDATE_IDVAL(idobj, f, idval) QRY_SET_RECORD(idobj, f, idval, 0, 0, 0, 3)

/*********************************************************************
** Typedef : cntxt_fact
** Description : variables et constantes utilisées pour la facturation et l'échéancier
*********************************************************************/
typedef struct _cntxt_fact
{
	char *label;		/* Libelle de l'operation en cours */

	/* Constantes valeurs et objets */
	unsigned long idfacture;
	unsigned long form_Mesure;
	unsigned long form_LigneFacture;
	unsigned long val_FORMSTAMP;
	unsigned long val_ETAT_FACTURATION;
	unsigned long val_ANNEE_FIN;
	unsigned long val_FACTURE;
	unsigned long val_MES_LIGNE_FACT;
	unsigned long val_DEST_FACTURE;
	unsigned long val_QTE_FACTURE;
	unsigned long val_QTE_CORRECTION;
	unsigned long val_MOTIF_CORRECTION;
	unsigned long val_MESURE_JUGT;
	unsigned long val_EFFET_JUGT;
	unsigned long val_ECHEANCE_JUGT;
	unsigned long val_DATE_JUGT;
	unsigned long val_DATE_ECHEANCE;
	unsigned long val_DATE_FACT_DEBUT;
	unsigned long val_DATE_FACT_FIN;
	unsigned long val_DATE_FACTURE;
	unsigned long val_NOM_SOCIETE;
	unsigned long val_En_cours;
	unsigned long val_Maintien;
	unsigned long val_Archivee;
	unsigned long val_En_attente;
	unsigned long val_ETAT_ECHEANCIER;
	unsigned long val_ECHEANCIER;
	unsigned long val_Echeance0M;
	unsigned long val_Echeance1M;
	unsigned long val_Echeance2M;
	unsigned long val_Majorite0M;
	unsigned long val_Majorite1M;
	unsigned long val_Majorite2M;
	unsigned long val_AttenteAffectationTS;

	/* Compteurs echeancier et facturation */
	unsigned long nb_echeance;
	unsigned long nb_echeance1M;
	unsigned long nb_echeance2M;
	unsigned long nb_attenteTS;
	unsigned long nb_lignfact;
	unsigned long nb_modif;
	int b_detail_journal;

	/* Dates mensuelles */
	char datedebmois[10];
	char datefinmois[10];
	unsigned long nbjoursmois;
	char datemois1[10];
	char datemois2[10];
	char datemois3[10];

	/* Donnees sur les mesures */
	DynTable tmes;
	unsigned long nb_mes;
	struct _mesdata
	{
		unsigned long id;
		char deb[10];
		char fin[10];
		char majorite[10];
		char effetderjug[10];	/* date de d'effet du dernier jugement */
		int b_mainlevee;
		int b_jeunemajeur;
		DynTable notes;
		char statfact[20];
		unsigned long idvalstat;
		int b_forcstat;
		char modefact[20];
		char debfact[10];
		char finfact[10];
		char prisecharge[10];
		char affect[10];
		unsigned long dest;		/* Id caisse facturation */
		unsigned long ts;		/* Id travailleur social affecté */
	}
		*mes;
}
	cntxt_fact;

/*********************************************************************
** Function : Init_liste_mes
** Description : initialisation de la liste des mesures à facturer
*********************************************************************/
#define ERR_FUNCTION "Init_liste_mes"
#define ERR_CLEANUP	DYNTAB_FREE(data)
int Init_liste_mes(
	EVA_context *cntxt,				/* in/out : execution context data */
	cntxt_fact *cf					/* in/out : structure à initialiser */
){
	DynTable data = { 0 };
	char qry[2048] = {0};
	char txt[32];

	/* Initialisation dates mois */
	if(cf->idfacture)
	{
		if(qry_obj_field(cntxt, &data, cf->idfacture, "DATE_FACTURE")) STACK_ERROR;
		strncpy(cf->datedebmois, dyntab_val(&data, 0, 0), 6);
	}
	else
		strncpy(cf->datedebmois, cntxt->timestamp, 6);
	sprintf(cf->datedebmois + 6, "01");
	delay_to_datetxt(cf->datemois1, cf->datedebmois, 1, 'M');
	delay_to_datetxt(cf->datefinmois, cf->datemois1, -1, 'D');
	cf->datefinmois[8] = 0;
	delay_to_datetxt(cf->datemois1, cf->datedebmois, 1, 'M');
	delay_to_datetxt(cf->datemois2, cf->datedebmois, 2, 'M');
	delay_to_datetxt(cf->datemois3, cf->datedebmois, 3, 'M');
	cf->nbjoursmois = atoi(cf->datefinmois + 6);

	/* Debut du journal d'execution */
	put_html_page_header(cntxt, NULL, cf->label, NULL, 3);
	printf("<hr><font face=Arial><b>%s au %.2s/%.4s</b><hr>", cf->label, cf->datedebmois + 4, cf->datedebmois);
	fflush(stdout);

	/* Définition des constantes communes */
	cf->val_FORMSTAMP = get_id_value(cntxt, add_sz_str("_EVA_FORMSTAMP"));
	cf->val_MOTIF_CORRECTION = get_id_value(cntxt, add_sz_str("MOTIF_CORRECTION"));
	cf->val_ETAT_FACTURATION = get_id_value(cntxt, add_sz_str("ETAT_FACTURATION"));
	cf->val_FACTURE = get_id_value(cntxt, add_sz_str("FACTURE"));
	cf->val_DEST_FACTURE = get_id_value(cntxt, add_sz_str("DEST_FACTURE"));
	cf->val_DATE_ECHEANCE = get_id_value(cntxt, add_sz_str("DATE_ECHEANCE"));
	cf->val_DATE_FACT_DEBUT = get_id_value(cntxt, add_sz_str("DATE_FACT_DEBUT"));
	cf->val_DATE_FACT_FIN = get_id_value(cntxt, add_sz_str("DATE_FACT_FIN"));
	cf->val_DATE_FACTURE = get_id_value(cntxt, add_sz_str("DATE_FACTURE"));
	cf->val_NOM_SOCIETE = get_id_value(cntxt, add_sz_str("NOM_SOCIETE"));
	cf->val_En_cours = get_id_value(cntxt, add_sz_str("En cours"));
	cf->val_Maintien = get_id_value(cntxt, add_sz_str("Maintien"));
	cf->val_Archivee = get_id_value(cntxt, add_sz_str("Archivée"));
	cf->val_En_attente = get_id_value(cntxt, add_sz_str("En attente"));
	cf->val_ETAT_ECHEANCIER = get_id_value(cntxt, add_sz_str("ETAT_ECHEANCIER"));
	cf->val_ECHEANCIER = get_id_value(cntxt, add_sz_str("ECHEANCIER"));
	cf->val_Echeance0M = get_id_value(cntxt, add_sz_str("Echeance ce mois"));
	cf->val_Echeance1M = get_id_value(cntxt, add_sz_str("Echeance le mois prochain"));
	cf->val_Echeance2M = get_id_value(cntxt, add_sz_str("Echeance dans deux mois"));
	cf->val_Majorite0M = get_id_value(cntxt, add_sz_str("Majorité ce mois"));
	cf->val_Majorite1M = get_id_value(cntxt, add_sz_str("Majorité le mois prochain"));
	cf->val_Majorite2M = get_id_value(cntxt, add_sz_str("Majorité dans deux mois"));
	cf->val_AttenteAffectationTS = get_id_value(cntxt, add_sz_str("Attente affectation TS"));

	/* Creation de la liste des mesures a traiter */
	if(sql_exec_query(cntxt, "CREATE TEMPORARY TABLE IdMes (IdObj INT) TYPE=HEAP")) STACK_ERROR;

	/* Lecture des mesures de la facture */
	if(cf->idfacture)
	{
		printf(" Lecture des mesures de la facture ...");
		fflush(stdout);
		snprintf(qry, sizeof(qry), 
				"INSERT INTO IdMes (IdObj) \n"
				"SELECT DISTINCT TLink2.IdRelObj FROM TLink AS TLink0 \n"
				"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj \n"
				"INNER JOIN TLink AS TLink2 ON TLink0.IdObj=TLink2.IdObj \n"
				"WHERE TLink0.DateDel IS NULL AND TLink0.IdRelObj=%lu AND TLink0.IdField=%lu \n"
				"AND TLink1.DateDel IS NULL AND TLink1.IdRelObj=%lu AND TLink1.IdField=%lu \n"
				"AND TLink2.DateDel IS NULL AND TLink2.IdField=%lu",
			cf->form_LigneFacture, cf->val_FORMSTAMP, cf->idfacture, cf->val_FACTURE, cf->val_MES_LIGNE_FACT);
		if(sql_exec_query(cntxt, qry)) STACK_ERROR;
		printf(" %lu trouvées<br>", cntxt->sql_nbrows);
		fflush(stdout);
	}

	/* Lecture des mesures avec des jugements récents */
	printf("Lecture des mesures avec des jugements sur les 24 derniers mois ...");
	fflush(stdout);
	delay_to_datetxt(txt, cf->datedebmois, -24, 'M');
	snprintf(qry, sizeof(qry), 
			"INSERT INTO IdMes (IdObj) \n"
			"SELECT DISTINCT TLink0.IdRelObj FROM TLink AS TLink0 \n"
			"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj \n"
			"INNER JOIN TVal ON TLink1.IdValue=TVal.IdValue \n"
			"AND TLink0.DateDel IS NULL AND TLink0.IdField=%lu \n"
			"AND TLink1.DateDel IS NULL AND TLink1.IdField IN (%lu,%lu,%lu) \n"
			"AND TVal.TxtValue>='%s'",
		cf->val_MESURE_JUGT, cf->val_EFFET_JUGT, cf->val_ECHEANCE_JUGT, cf->val_DATE_JUGT, txt);
	if(sql_exec_query(cntxt, qry)) STACK_ERROR;
	printf(" %lu trouvées<br>", cntxt->sql_nbrows);
	fflush(stdout);

	/* Lecture des mesures en cours */
	printf("Lecture des mesures en cours et en attente ...");
	fflush(stdout);
	snprintf(qry, sizeof(qry), 
			"INSERT INTO IdMes (IdObj) \n"
			"SELECT DISTINCT TLink0.IdObj FROM TLink AS TLink0 \n"
			"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj \n"
			"WHERE TLink0.DateDel IS NULL AND TLink0.IdRelObj=%lu AND TLink0.IdField=%lu \n"
			"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdValue<>%lu",
		cf->form_Mesure, cf->val_FORMSTAMP, cf->val_ETAT_FACTURATION, cf->val_Archivee);
	if(sql_exec_query(cntxt, qry)) STACK_ERROR;
	printf(" %lu trouvées<br>", cntxt->sql_nbrows);
	fflush(stdout);

	/* Compilation des mesures a examiner */
	printf("Nombre total de mesures à examiner :");
	fflush(stdout);
	if(sql_exec_query(cntxt, "SELECT DISTINCT IdObj FROM IdMes ORDER BY IdObj") || 
		sql_get_table(cntxt, &cf->tmes, 2) ||
		sql_exec_query(cntxt, "DROP TABLE IdMes")) STACK_ERROR;

	/* Allocation memoire pour les données des mesures */
	cf->nb_mes = cf->tmes.nbrows;
	C_ALLOC(cf->mes, sizeof(cf->mes[0]), cf->nb_mes);
	printf(" %lu<br>", cf->nb_mes);
	fflush(stdout);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : Calc_echeancier
** Description : preparation de l'échancier mensuel
*********************************************************************/
#define ERR_FUNCTION "Calc_echeancier"
#define ERR_CLEANUP	
int Calc_echeancier(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	cntxt_fact *cf					/* in : contexte de facturation */
){
	unsigned long i, cnt = 0;

	/* Maj echeancier pour chaque mesure à examiner */
	if(cf) for(i = 0; i < cf->nb_mes; i++)
	{
		unsigned long echeance = 0;
		char *datefin = cf->mes[i].fin;
		char *majorite = cf->mes[i].majorite;
		DynTable *ano = &cf->mes[i].notes;
		DynTableCell val = {0};

		/* Barre d'avancement */
		if(i * 60 / cf->nb_mes >= cnt)
		{
			putc('*', stdout);
			fflush(stdout);
			cnt++;
		}

		/* Calcul des échéances sur 3 mois */
		if(strcmp(majorite, datefin) <= 0 && 
			strcmp(majorite, cf->datedebmois) >= 0 && strcmp(majorite, cf->datefinmois) <= 0) 
		{
			echeance = cf->val_Majorite0M;
			cf->nb_echeance++;
		}
		else if(strcmp(datefin, cf->datedebmois) >= 0 && strcmp(datefin, cf->datefinmois) <= 0)
		{
			echeance = cf->val_Echeance0M;
			cf->nb_echeance++;
		}
		else if(strcmp(majorite, datefin) <= 0 && 
				strcmp(majorite, cf->datefinmois) > 0 && strcmp(majorite, cf->datemois2) < 0) 
		{
			echeance = cf->val_Majorite1M;
			cf->nb_echeance1M++;
		}
		else if(strcmp(datefin, cf->datefinmois) > 0 && strcmp(datefin, cf->datemois2) < 0) 
		{
			echeance = cf->val_Echeance1M;
			cf->nb_echeance1M++;
		}
		else if(strcmp(majorite, datefin) <= 0 && 
				strcmp(majorite, cf->datemois2) >= 0 && strcmp(majorite, cf->datemois3) < 0) 
		{
			echeance = cf->val_Majorite2M;
			cf->nb_echeance2M++;
		}
		else if(strcmp(datefin, cf->datemois2) >= 0 && strcmp(datefin, cf->datemois3) < 0) 
		{
			echeance = cf->val_Echeance2M;
			cf->nb_echeance2M++;
		}
		else if(!cf->mes[i].ts)
		{
			echeance = cf->val_AttenteAffectationTS;
			cf->nb_attenteTS++;
		}

		QRY_UPDATE_IDVAL(cf->mes[i].id, ECHEANCIER, echeance);
		if(qry_update_idobj_idfield(cntxt, cf->mes[i].id, cf->val_ETAT_ECHEANCIER, ano, 3)) STACK_ERROR;
	}

	/* Affiche le bouton de retour à la page precedente */
	printf("*</pre></font></b><hr>");
	printf("%lu échéances ce mois<br>"
			"%lu échéances le mois prochain<br>"
			"%lu échéances à deux mois<br>",
			"%lu mesures en attente d'affectation TS<br><br>",
			cf->nb_echeance, cf->nb_echeance1M, cf->nb_echeance2M, cf->nb_attenteTS);
	printf("<hr><br><center><input type=image name='B$#NOP' src='../img/_eva_btn_gobackobj_fr.gif'></center><br>\n");
	cntxt->b_terminate = 15;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : qsort_nom
** Description : tri sur colonnes 15/16 
*********************************************************************/
int qsort_nom(const void *el1, const void *el2)
{
	char *t1 = ((DynTableCell *)el1+15)->txt;
	char *t2 = ((DynTableCell *)el2+15)->txt;
	int res;

	if(!t1 || !*t1) t1 = "";
	if(!t2 || !*t2) t2 = "";

	res = stricmp(t1, t2);
	if(res) return res;

	t1 = ((DynTableCell *)el1+16)->txt;
	t2 = ((DynTableCell *)el2+16)->txt;
	if(!t1 || !*t1) t1 = "";
	if(!t2 || !*t2) t2 = "";

	return stricmp(t1, t2);
}

/*********************************************************************
** Function : Init_AEMO
** Description : initialisation des constantes de facturation
*********************************************************************/
#define ERR_FUNCTION "Init_AEMO"
#define ERR_CLEANUP	DYNTAB_FREE(data)
int Init_AEMO(
	EVA_context *cntxt,				/* in/out : execution context data */
	cntxt_fact *cf					/* in/out : structure à initialiser */
){
	DynTable data = { 0 };
	unsigned long i, cnt = 0;
	char qry[2048] = {0};
	char txt[32];

	/* Définition des constantes spécifiques */
	cf->label = cf->idfacture ? "Facturation AEMO" : "Echéancier AEMO";
	cf->form_Mesure = 26925;
	cf->form_LigneFacture = 59650;
	cf->val_ANNEE_FIN = get_id_value(cntxt, add_sz_str("FIN_AEMO"));
	cf->val_MES_LIGNE_FACT = get_id_value(cntxt, add_sz_str("AEMO_LIGNE_FACT"));
	cf->val_QTE_FACTURE = get_id_value(cntxt, add_sz_str("JOURS_FACT"));
	cf->val_QTE_CORRECTION = get_id_value(cntxt, add_sz_str("JOURS_CORRECTION"));
	cf->val_MOTIF_CORRECTION = get_id_value(cntxt, add_sz_str("MOTIF_CORRECTION"));
	cf->val_MESURE_JUGT = get_id_value(cntxt, add_sz_str("REF_AEMO"));
	cf->val_EFFET_JUGT = get_id_value(cntxt, add_sz_str("ORD_JUGTS_EFFET_DU"));
	cf->val_ECHEANCE_JUGT = get_id_value(cntxt, add_sz_str("ECHEANCE_JUGT"));
	cf->val_DATE_JUGT = get_id_value(cntxt, add_sz_str("ORD_JUGTS_DATE"));

	/* Creation de la liste des mesures a traiter */
	if(Init_liste_mes(cntxt, cf)) STACK_ERROR;

	/* Lecture des donnees complementaires */
	printf("Lecture des donnees complementaires ...");
	fflush(stdout);
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("ENFANT->DATE_NAISS"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("MIN(<-REF_AEMO->ORD_JUGTS_EFFET_DU)"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("MAX(<-REF_AEMO->ORD_JUGTS_DATE)"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("AEMO_JEUNE_MAJEUR"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("ETAT_FACTURATION"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("FACT_MODE"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_PRISE_EN_CHARGE"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_FACT_DEBUT"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_FACT_FIN"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("CAISSE_FACTURE"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("FORCER_ETAT"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_ECHEANCE"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_AFFECT_TS"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("EDUCATEUR"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("ENFANT->NOM"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("ENFANT->PRENOM"))) STACK_ERROR;

	/* Affichage de la barre d'avancement */
	printf("<hr><font face=Courier><b><pre>Examen de %lu mesures\n%s", cf->nb_mes, WAIT_TXTSCALE);
	fflush(stdout);

	/* Tri des mesures sur nom/prénom */
	dyntab_sort(&cf->tmes, qsort_nom);

	/* Pour chaque mesure a examiner */
	for(i = 0; i < cf->nb_mes; i++)
	{
		char *datedeb = cf->mes[i].deb;
		char *datefin = cf->mes[i].fin;
		char *datenaiss = dyntab_val(&cf->tmes, i, 1);
		char *d_echeance = dyntab_val(&cf->tmes, i, 12);
		char *majorite = cf->mes[i].majorite;
		DynTable *ano = &cf->mes[i].notes;
		unsigned long idval = get_id_value(cntxt, DYNTAB_VAL_SZ(&cf->tmes, i, 3));

		/* Barre d'avancement */
		if(i * 60 / cf->nb_mes >= cnt)
		{
			putc('*', stdout);
			fflush(stdout);
			cnt++;
		}

		/* Lecture des donnees de la mesure */
		cf->mes[i].id = DYNTAB_TOULRC(&cf->tmes, i, 0);
		if(datetxt_invalid(datenaiss))
		{
			DYNTAB_SET(ano, ano->nbrows, 0, "Date de naissance incorrecte");
		}
		else if(!delay_to_datetxt(txt, datenaiss, 18, 'Y')) strncpy(majorite, txt, 8);
		strncpy(datedeb, dyntab_val(&cf->tmes, i, 2), 8);
		cf->mes[i].b_jeunemajeur = dyntab_val(&cf->tmes, i, 4)[0] != 0;
		strncpy(cf->mes[i].statfact, dyntab_val(&cf->tmes, i, 5), sizeof(cf->mes->statfact));
		cf->mes[i].idvalstat = get_id_value(cntxt, DYNTAB_VAL_SZ(&cf->tmes, i, 5));
		strncpy(cf->mes[i].modefact, dyntab_val(&cf->tmes, i, 6), sizeof(cf->mes->modefact));
		strncpy(cf->mes[i].prisecharge, dyntab_val(&cf->tmes, i, 7), 8);
		strncpy(cf->mes[i].debfact, dyntab_val(&cf->tmes, i, 8), 8);
		strncpy(cf->mes[i].finfact, dyntab_val(&cf->tmes, i, 9), 8);
		cf->mes[i].dest = DYNTAB_TOULRC(&cf->tmes, i, 10);
		cf->mes[i].b_forcstat = dyntab_val(&cf->tmes, i, 11)[0] != 0;
		strncpy(cf->mes[i].affect, dyntab_val(&cf->tmes, i, 13), 8);
		cf->mes[i].ts = DYNTAB_TOULRC(&cf->tmes, i, 14);

		/* Lecture du dernier jugement */
		snprintf(add_sz_str(qry),
			"SELECT DISTINCT TLink0.IdObj FROM TLink AS TLink0\n"
			"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj\n"
			"WHERE TLink0.DateDel IS NULL AND TLink0.IdField=%lu AND TLink0.IdRelObj=%lu\n"
			"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdValue=%lu\n"
			"ORDER BY TLink0.IdObj DESC",
			cf->val_MESURE_JUGT, cf->mes[i].id, cf->val_DATE_JUGT, idval);
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;

		/* Lecture date d'échéance */
		if(qry_obj_field(cntxt, &data, DYNTAB_TOUL(&data), "ECHEANCE_JUGT,ORD_JUGTS_EFFET_DU,ORD_JUGTS_TYPE")) STACK_ERROR;
		strncpy(datefin, dyntab_field_val(&data, "ECHEANCE_JUGT", ~0UL, 0), 8);

		/* Détermine si mesure terminée (main levée) */
		{
			char *effetdu = dyntab_field_val(&data, "ORD_JUGTS_EFFET_DU", ~0UL, 0);
			char *datejug = dyntab_val(&cf->tmes, i, 3);
			strncpy(cf->mes[i].effetderjug, effetdu, 8);
			cf->mes[i].b_mainlevee = 
				!strcmp(dyntab_field_val(&data, "ORD_JUGTS_TYPE", ~0UL, 0), "Main levée") ||
				!strncmp(effetdu, datefin, 8) || !strncmp(datejug, datefin, 8);
		}

		/* Prise en compte majorite en fin de mesure */
		if(*majorite && strcmp(majorite, datefin) < 0)
		{
			if(cf->mes[i].b_jeunemajeur)
			{
				/* Mesure jeune majeur : fin de mesure a 21 ans maxi */
				DYNTAB_SET(ano, ano->nbrows, 0, "Jeune majeur");
				delay_to_datetxt(txt, datenaiss, 21, 'Y');
				if(*txt && strcmp(txt, datefin) < 0)
				{
					if(!strncmp(majorite, cf->datedebmois, 6)) DYNTAB_SET(ano, ano->nbrows, 0, "Arrêt à 21 ans");
					strncpy(datefin, txt, 8);
				}
			}
			else 
			{
				if(!strncmp(majorite, cf->datedebmois, 6)) DYNTAB_SET(ano, ano->nbrows, 0, "Arrêt à 18 ans");
				strncpy(datefin, majorite, 8);
			}
		}

		/* Prise en compte fin de facturation manuelle */
		if(!strcmp(cf->mes[i].modefact, "DateManuelle") && cf->mes[i].finfact[0])
			strncpy(datefin, cf->mes[i].finfact, 8);

		/* Vérification dates debut / fin */
		if(*datedeb && datetxt_invalid(datedeb)) *datedeb = 0;
		if(*datefin && datetxt_invalid(datefin)) *datefin = 0;
		if(!*datedeb) DYNTAB_SET(ano, ano->nbrows, 0, "Debut de mesure inconnu");
		if(!*datefin) DYNTAB_SET(ano, ano->nbrows, 0, "Fin de mesure inconnue");

		/* Maj date echeance mesure si necessaire */
		if(strlen(datefin) > 7 && strcmp(d_echeance, datefin) &&
			qry_update_idobj_idfield_1(cntxt, cf->mes[i].id, cf->val_DATE_ECHEANCE, datefin, 8)) STACK_ERROR;
	}

	printf("*\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : Entete_journal
** Description : produit l'entete du journal d'execution
*********************************************************************/
#define ERR_FUNCTION "Entete_journal"
#define ERR_CLEANUP	
int Entete_journal(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	cntxt_fact *cf					/* in/out : structure à initialiser */
){
	cf->b_detail_journal = 0;
	printf("</pre></font></b><hr>"
			"<b><u>Préparation de la facture - mise à jour des mesures facturées</u></b><hr>"
			"<table border=0 cellpadding=1 cellspacing=0 width=850 bgcolor=#DDDDDD><tr>"
			"<td width=150>Enfant</td>"
			"<td width=80 align=right>Début</td>"
			"<td width=10 align=center>-</td>"
			"<td width=80>Fin</td>"
			"<td width=120>Destinataire</td>"
			"<td width=30 align=right>NB</td>"
			"<td width=10>+</td>"
			"<td width=50>C</td>"
			"<td width=60>Ancien</td>"
			"<td width=80>Statut</td>"
			"<td>Motif</td>"
			"</tr></table>\n");
	fflush(stdout);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : calc_jcor_AEMO
** Description : calule les jours de régularisation d'une mesure AEMO
*********************************************************************/
#define ERR_FUNCTION "calc_jcor_AEMO"
#define ERR_CLEANUP	DYNTAB_FREE(lignfact); \
					DYNTAB_FREE(data)
int calc_jcor_AEMO(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	cntxt_fact *cf,					/* in/out : structure à initialiser */
	unsigned long i,				/* in : indice mesure dans cf->mes */
	char *datedeb,					/* in : date de début de facturation */
	char *datefin,					/* in : date de fin de facturation */
	unsigned long idlignefact,		/* in : n° ligne facture si existante */
	unsigned long nbjours,			/* in : nb de jours en charge dans le mois */
	int *nbjcorr					/* out : nb de jours de correction */
){
	char datetmp[16];
	unsigned long idmesure = cf->mes[i].id;
	unsigned long tot_fact = 0;
	char *dfact0 = NULL;
	DynTable lignfact = { 0 };
	DynTable data = { 0 };
	char qry[2048] = {0};
	unsigned long j;
	char fmt = 'D';
	char *dfin = strcmp(cf->datemois1, datefin) < 0 ? cf->datemois1 : datefin;

	/* Calcul depuis la dernière correction si mesure commencée avant 06-2004 */
	if(strcmp(cf->mes[i].deb, "2004") < 0)
	{
		/* Lecture des lignes de facture après 2004 pour la mesure */
		snprintf(qry, sizeof(qry), 
				"SELECT DISTINCT TLink0.IdObj,TLink2.IdRelObj,DFact.TxtValue FROM TLink AS TLink0 \n"
				"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj \n"
				"INNER JOIN TLink AS TLink2 ON TLink0.IdObj=TLink2.IdObj \n"
				"INNER JOIN TLink AS TLink3 ON TLink2.IdRelObj=TLink3.IdObj \n"
				"INNER JOIN TVal AS DFact ON TLink3.IdValue=DFact.IdValue \n"
				"WHERE TLink0.DateDel IS NULL AND TLink0.IdField=%lu AND TLink0.IdRelObj=%lu AND TLink0.IdObj<>%lu\n"
				"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdRelObj=%lu \n"
				"AND TLink2.DateDel IS NULL AND TLink2.IdField=%lu \n"
				"AND TLink3.DateDel IS NULL AND TLink3.IdField=%lu AND DFact.TxtValue>'2004' \n"
				"ORDER BY DFact.TxtValue DESC",
			cf->val_FORMSTAMP, cf->form_LigneFacture, idlignefact, 
			cf->val_MES_LIGNE_FACT, idmesure,
			cf->val_FACTURE,
			cf->val_DATE_FACTURE);
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &lignfact, 2)) STACK_ERROR;
		if(!lignfact.nbrows) RETURN_OK;

		/* Examen des lignes de factures */
		for(j = 0; j < lignfact.nbrows; j++)
		{
			/* Lecture des jours facturés */
			snprintf(qry, sizeof(qry), 
					"SELECT DISTINCT TLink.IdField,TVal.TxtValue FROM TLink \n"
					"INNER JOIN TVal ON TLink.IdValue=TVal.IdValue \n"
					"WHERE TLink.IdObj=%s AND TLink.DateDel IS NULL AND TLink.IdField IN (%lu,%lu) \n",
				dyntab_val(&lignfact, j, 0), 
				cf->val_QTE_FACTURE, cf->val_QTE_CORRECTION);
			if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;

			/* Arret si correction - decompte à partir du mois suivant */
			if(data.nbrows > 1 || DYNTAB_TOUL(&data) == cf->val_QTE_CORRECTION)
			{
				delay_to_datetxt(datetmp, dyntab_val(&lignfact, j, 2), 1, 'M');
				dfact0 = datetmp;
				break; 
			}

			/* Total de jours facturés */
			tot_fact += DYNTAB_TOULRC(&data, 0, 1);
		}

		/* Calcul du début de la période à régulariser */
		if(!dfact0) dfact0 = dyntab_val(&lignfact, lignfact.nbrows - 1, 2);
		if(strcmp(dfact0, datedeb) < 0) dfact0 = datedeb;

		/* Régularisation uniquement à partir de 2004 */
		if(strcmp(dfact0, "2004") > 0)
		{
			/* Calcul nb de jours à facturer sur la période à régulariser */
			int a_fact = strcmp(dfact0, dfin) < 0 ? datetxt_to_age(dfact0, dfin, &fmt) : 0;
			*nbjcorr = (a_fact > 0 ? a_fact : 0) - tot_fact - nbjours;
		}
	}
	/* Calcul sur l'ensemble des factures */
	else
	{
		/* Lecture des totaux de facturation pour la mesure */
		int a_fact = datetxt_to_age(datedeb, dfin, &fmt);
		snprintf(qry, sizeof(qry), 
				"SELECT SUM(TVal.TxtValue) FROM TLink AS TLink0 \n"
				"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj \n"
				"INNER JOIN TLink AS TLink2 ON TLink0.IdObj=TLink2.IdObj \n"
				"INNER JOIN TVal ON TLink2.IdValue=TVal.IdValue \n"
				"WHERE TLink0.DateDel IS NULL AND TLink0.IdField=%lu AND TLink0.IdRelObj=%lu AND TLink0.IdObj<>%lu\n"
				"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdRelObj=%lu \n"
				"AND TLink2.DateDel IS NULL AND TLink2.IdField IN(%lu,%lu)",
			cf->val_FORMSTAMP, cf->form_LigneFacture, idlignefact, 
			cf->val_MES_LIGNE_FACT, idmesure,
			cf->val_QTE_FACTURE, cf->val_QTE_CORRECTION);
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;
		tot_fact = DYNTAB_TOUL(&data);
		*nbjcorr = (a_fact > 0 ? a_fact : 0) - tot_fact - nbjours;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : FactureAEMO
** Description : preparation d'une facture d'AEMO
*********************************************************************/
#define ERR_FUNCTION "FactureAEMO"
#define ERR_CLEANUP	DYNTAB_FREE(lignfact); \
					DYNTAB_FREE(data); \
					M_FREE(msg)
int FactureAEMO(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	DynTable lignfact = { 0 };
	DynBuffer *msg = NULL;
	DynTable data = { 0 };
	char qry[2048] = {0};
	cntxt_fact _cf = {0}, *cf = &_cf;
	unsigned long i, j;
	char txt[32];

	/* Inhibit warning for unused param */
	i_ctrl = 0;

	/* Retour si facture non enregistree */
	if(!dyntab_sz(&form->id_obj, 0, 0))
	{
		DYNBUF_ADD_STR(&cntxt->debug_msg, "\n**** Traitement non effectué\n  -> Enregistrez d'abord la facture\n");
		RETURN_OK;
	}

	/* Preparation des donnees de facturation */
	cf->idfacture = DYNTAB_TOUL(&form->id_obj);
	if(Init_AEMO(cntxt, cf) || Entete_journal(cntxt, cf)) STACK_ERROR;

	/* Retour si pas dernière facture */
	if(form_eval_fieldexpr(cntxt, &data, 0, 27461, add_sz_str("MAX(<-_EVA_FORMSTAMP->DATE_FACTURE)"), NULL, 0)) STACK_ERROR;
	if(strcmp(dyntab_val(&data, 0, 0), cf->datedebmois) > 0)
	{
		DYNBUF_ADD_STR(&cntxt->debug_msg, "\n**** Traitement refusé : vous n'êtes pas sur la dernière facture\n");
		RETURN_OK;
	}

	/* Pour chaque mesure en cours */
	for(i = 0; i < cf->nb_mes; i++)
	{
		unsigned long idmesure = cf->mes[i].id, idlignefact = 0, iddest = cf->mes[i].dest;
		char *datedeb = cf->mes[i].deb;
		char *datefin = cf->mes[i].fin;
		char *majorite = cf->mes[i].majorite;
		char datedebcor[16] = {0}, datefincor[16] = {0};
		char *modefact = cf->mes[i].modefact, *mindeb, *maxfin;
		int nbjours = cf->nbjoursmois, nbjcorr = 0, b_calcmanuel;	  
		unsigned long statut;
		char *valstatut = cf->mes[i].statfact, *nouvstatut, *bgcolor = "FFFFFF";
		DynTableCell val = {0};
		DynTable *ano = &cf->mes[i].notes;

		/* RAZ message pour la mesure */
		M_FREE(msg);

		/* Recherche si une ligne existe deja pour cette mesure dans la facture */
		snprintf(qry, sizeof(qry), 
				"SELECT TLink0.IdObj AS IdObj,TLink0.Num AS Num FROM TLink AS TLink0 \n"
				"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj \n"
				"INNER JOIN TLink AS TLink2 ON TLink0.IdObj=TLink2.IdObj \n"
				"WHERE TLink0.DateDel IS NULL AND TLink0.IdRelObj=%lu AND TLink0.IdField=%lu \n"
				"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdRelObj=%lu \n"
				"AND TLink2.DateDel IS NULL AND TLink2.IdField=%lu AND TLink2.IdRelObj=%lu \n"
				"GROUP BY IdObj ORDER BY IdObj DESC",
			cf->form_LigneFacture, cf->val_FORMSTAMP, cf->val_MES_LIGNE_FACT, idmesure, cf->val_FACTURE, cf->idfacture);
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;
		idlignefact = DYNTAB_TOUL(&data);

		/* Laisse du temps aux autres utilisateurs */
		sql_control(cntxt, -5);

		/* Si plus d'une ligne : ne touche à rien */
		if(data.nbrows > 1) continue;

		/* Lecture des donnees de la ligne */
		DYNTAB_FREE(lignfact);
		if(idlignefact && qry_obj_idfield(cntxt, &lignfact, idlignefact, 0)) STACK_ERROR;

		/* Gestion du mode de facturation */
		b_calcmanuel = idlignefact && dyntab_field_cell(&lignfact, "CALC_MANUEL", ~0UL, 0);
		if(b_calcmanuel)
		{
			/* Calcul manuel : lecture des données de la ligne */
			nbjours = strtoul(dyntab_field_val(&lignfact, "JOURS_FACT", ~0UL, 0), NULL, 10);
			nbjcorr = strtoul(dyntab_field_val(&lignfact, "JOURS_CORRECTION", ~0UL, 0), NULL, 10);
			if(dyntab_filter_field(ano, 0, &lignfact, "MOTIF_CORRECTION", ~0UL, NULL)) RETURN_ERR_MEMORY; 
			iddest = strtoul(dyntab_field_val(&lignfact, "DEST_FACTURE", ~0UL, 0), NULL, 10);
		}
		else if(!strcmp(modefact, "DateManuelle"))
		{
			if(qry_obj_field(cntxt, &data, idmesure, "MOTIF_FACT_MANUELLE")) STACK_ERROR;
			if(data.nbrows) DYNTAB_ADD_CELL(ano, ano->nbrows, 0, &data, 0, 0);
			strncpy(datedebcor, cf->mes[i].debfact, 8);
			strncpy(datefincor, cf->mes[i].finfact, 8);
		}
		else if(!strcmp(modefact, "DateRecept") && strcmp(cf->mes[i].prisecharge, cf->mes[i].deb) > 0)
		{
			strncpy(datedebcor, cf->mes[i].prisecharge, 8);
		}
		else if(strcmp(modefact, "DateEffet"))
		{
			/* Facturation auto */
			if(cf->mes[i].affect[0])
				strncpy(datedebcor, cf->mes[i].affect, 8);
			else if(strcmp(cf->mes[i].prisecharge, cf->mes[i].deb) > 0)
				strncpy(datedebcor, cf->mes[i].prisecharge, 8);
		}

		/* Changement de destinataire a 18 ans */
		if(strcmp(majorite, cf->datefinmois) < 0)
		{
			/* Si mesure jeune majeur et 18 ans avant ce mois */
 			if(cf->mes[i].b_jeunemajeur && strcmp(majorite, cf->datedebmois) < 0)
			{
				/* Destinataire = PJJ quand plus de 18 ans */
				iddest = 27616;

				/* Correction si premier mois facture après 18 ans */
				delay_to_datetxt(txt, majorite, 1, 'M');
				if(!strncmp(txt, cf->datedebmois, 6))
				{
					delay_to_datetxt(txt, cf->datedebmois, -1, 'D');
					txt[8] = 0;
					nbjcorr = atoi(txt + 6) - atoi(majorite + 6) + 1;
					DYNTAB_SET(ano, ano->nbrows, 0, "Régularisation 1er mois");
				}
			}
			else if(strcmp(majorite, datefin) < 0)
			{
				/* Fin de facturation le mois des 18 ans */
				strncpy(datefincor, majorite, 8);
			}
		}

		/* Destinataire par defaut = DIPAS DEPARTEMENT */
		if(!iddest) iddest = 27615;

		/* Normalisation dates debut / fin */
		if(*datedebcor && datetxt_invalid(datedebcor)) *datedebcor = 0;
		if(*datefincor && datetxt_invalid(datefincor)) *datefincor = 0;
		if(!*datedebcor && *datedeb) strcpy(datedebcor, datedeb);
		if(!*datefincor && *datefin) strcpy(datefincor, datefin);
 
//		if(cf->mes[i].id == 725991) sql_control(cntxt, 4);

		/* Etat de facturation de la mesure */
		mindeb = (!*datedeb || strcmp(datedeb, datedebcor) > 0) ? datedebcor : datedeb;
		maxfin = !*datefincor ? "A" : strcmp(datefin, datefincor) < 0 ? datefincor : datefin;
		if(cf->mes[i].b_forcstat && cf->mes[i].idvalstat)
		{
			/* Etat force : nouvel etat = ancien etat */
			statut = cf->mes[i].idvalstat;
			nouvstatut = cf->mes[i].statfact;
		}
		else if(strcmp(maxfin, cf->datedebmois) <= 0)
		{
			/* Date de fin de facturation atteinte : maintien de la mesure si:
					* 12 mois max apres echeance
					* facturation non manuelle
					* facturation DIPAS 01
					* majorité non atteinte
					* pas de main levee 
					* echeance après 1 janvier 2003 */
			delay_to_datetxt(txt, datefin, 12, 'M');
			if(	strcmp(txt, cf->datedebmois) > 0 &&
				strcmp(modefact, "DateManuelle") &&
				iddest == 27615 &&
				strcmp(majorite, cf->datefinmois) > 0 &&
				!cf->mes[i].b_mainlevee &&
				strcmp(datefin, "2003") > 0)
			{
				statut = cf->val_Maintien;
				DYNTAB_SET(ano, ano->nbrows, 0, "Maintien en attente de jugement");
				nouvstatut = "Maintien";
				bgcolor = "#FF8888";
			}
			else
			{
				statut = cf->val_Archivee;
				nouvstatut = "Archivée";
				bgcolor = "#CCCCFF";
				nbjours = 0;
			}
		}
		else if(strcmp(mindeb, cf->datefinmois) > 0)
		{
			statut = cf->val_En_attente;
			nouvstatut = "En attente";
			bgcolor = "#FFCCCC";
		}
		else
		{
			statut = cf->val_En_cours;
			nouvstatut = "En cours";
			bgcolor = "#CCFFCC";
			if(!cf->mes[i].ts) DYNTAB_SET(ano, ano->nbrows, 0, "Attente affectation TS");
		}

		/* Calcul du nombre de jours factures pour le mois */
		if(!b_calcmanuel && statut != cf->val_Maintien && statut != cf->val_En_attente)
		{
			if(!cf->mes[i].ts || strcmp(datefincor, cf->datedebmois) <= 0)
			{
				/* Mois après la date d'écheance ou TS non affecté : non facturé */
				nbjours = 0;
			}
			else if(strcmp(datefincor, cf->datefinmois) <= 0)
			{
				/* Fin le mois en cours : facturer jour de fin - 1 */
				nbjours = atoi(datefincor + 6) - 1;
			}
			else if(strcmp(datedebcor, cf->datefinmois) > 0)
			{
				/* Mois avant la date de début :non facturé */
				nbjours = 0;
			}															 
			else if(strcmp(datedebcor, cf->datedebmois) >= 0)
			{
				/* Début le mois en cours : facturer jour de début à fin de mois */
				nbjours = cf->nbjoursmois - atoi(datedebcor + 6) + 1;
			}

			/* Si TS affecté et pas de correction donnée */
			if(cf->mes[i].ts && !nbjcorr)
			{
				/* Calcul des corrections */
				if(calc_jcor_AEMO(cntxt, cf, i, datedebcor, datefincor, idlignefact, nbjours, &nbjcorr)) STACK_ERROR;
				if(nbjcorr)
				{
					char buf[256] = {0}, d[32];
					int b_new = !strcmp(cf->mes[i].effetderjug, datedeb);
					datetxt_to_format(cntxt, d, b_new ? datedebcor : cf->mes[i].effetderjug, NULL);
					DYNBUF_ADD3_INT(&msg, "Correction ", nbjcorr, "j");
					DYNTAB_ADD(ano, ano->nbrows, 0, buf, 
						snprintf(add_sz_str(buf), "%s à/c du %s", 
								b_new ? "Prise en charge" : cf->mes[i].b_mainlevee ? "Main levée" : "Maintien", 
								d), NO_CONV);
				}					   
			}


			/* Annotation si modification des dates de facturation */
			mindeb = strcmp(datedeb, datedebcor) > 0 ? datedebcor : datedeb;
			maxfin = strcmp(datedeb, datedebcor) < 0 ? datedebcor : datedeb;
			if(strcmp(mindeb, cf->datefinmois) <= 0 && strcmp(maxfin, cf->datedebmois) >= 0 && !nbjcorr)
			{
 				datetxt_to_format(cntxt, txt, datedebcor, NULL);
				DYNTAB_ADD(ano, ano->nbrows, 0, qry, snprintf(add_sz_str(qry), "Prise en charge le %s", txt), NO_CONV);
			}
			mindeb = strcmp(datefin, datefincor) > 0 ? datefincor : datefin;
			maxfin = strcmp(datefin, datefincor) < 0 ? datefincor : datefin;
			if(strcmp(mindeb, cf->datefinmois) <= 0 && strcmp(maxfin, cf->datedebmois) >= 0 && !nbjcorr)
			{
 				datetxt_to_format(cntxt, txt, datefincor, NULL);
				DYNTAB_ADD(ano, ano->nbrows, 0, qry, snprintf(add_sz_str(qry), "Fin de prise en charge le %s", txt), NO_CONV);
			}
		}

		/* Journal d'execution pour la mesure si applicable */
		if(cf->b_detail_journal || strcmp(valstatut, nouvstatut) || msg)
		{
			if(strcmp(valstatut, nouvstatut)) cf->nb_modif++;
			printf("<table border=0 cellpadding=1 cellspacing=0 width=800><tr>");
			printf("<td width=150><font size=-1>%s %s</td>", dyntab_val(&cf->tmes, i, 15), dyntab_val(&cf->tmes, i, 16));
			printf("<td width=80 align=right><font size=-1>");
			if(*datedeb) printf("%.2s/%.2s/%.4s", datedeb+6, datedeb+4, datedeb);
			printf("</td><td width=10 align=center>-</td><td width=80><font size=-1>");
			if(*datefin) printf("%.2s/%.2s/%.4s", datefin+6, datefin+4, datefin);
			printf("</td>");
			if(qry_obj_idfield(cntxt, &data, iddest, cf->val_NOM_SOCIETE)) STACK_ERROR;
			printf("<td width=120><font size=-2>%s</td>", dyntab_val(&data, 0, 0));
			printf("<td width=30 align=right>%d</td><td width=10>j</td><td width=50>%+d</td>", nbjours, nbjcorr);
			printf("<td width=60><font size=-2>%s</td>", strcmp(valstatut, nouvstatut) ? valstatut : "");
			printf("<td width=80 bgcolor=%s><font size=-1>%s</td>", bgcolor, nouvstatut);
			printf("<td%s><font size=-2><b>", ano->nbrows ? " bgcolor=#FFDDBB" : "");
			for(j = 0; j < ano->nbrows; j++) printf("%s%s", j ? ", " : "", dyntab_val(ano, j, 0));
			if(msg) printf("<br>%s", msg->data);
			printf("</b></font></td>");
			printf("</tr></table>");
			fflush(stdout);
		}

		/* Maj etat mesure et dates facturation */
		if(strcmp(valstatut, nouvstatut) && statut) QRY_UPDATE_IDVAL(idmesure, ETAT_FACTURATION, statut);
		if(strcmp(cf->mes[i].debfact, datedebcor)) QRY_UPDATE_VAL(cf->mes[i].id, DATE_FACT_DEBUT, datedebcor, 0);
		if(strcmp(cf->mes[i].finfact, datefincor)) QRY_UPDATE_VAL(cf->mes[i].id, DATE_FACT_FIN, datefincor, 0);

		/* Ligne non necessaire : efface la ligne si elle existe et continue */
		if(!cf->mes[i].ts || (statut != cf->val_En_cours && statut != cf->val_Maintien && !b_calcmanuel && !nbjcorr))
		{
			if(idlignefact)
			{
				snprintf(add_sz_str(qry), "UPDATE TLink SET DateDel='%s',IdWhoDel=%lu WHERE DateDel IS NULL AND IdObj=%lu",
					cntxt->timestamp, DYNTAB_TOUL(&cntxt->id_user), idlignefact);
				if(sql_exec_query(cntxt, qry)) STACK_ERROR;
			}

			/* Maj année de fin */
			QRY_UPDATE_VAL(idmesure, ANNEE_FIN, datefincor, 4);
			continue;
		}

		/* Mesure facturée : creation si la ligne n'existe pas */
		cf->nb_lignfact++;
		if(!idlignefact)
		{
			if(qry_add_new_obj(cntxt, &data, cf->form_LigneFacture)) STACK_ERROR;
			idlignefact = DYNTAB_TOUL(&data);
			QRY_CREATE_REL(idlignefact, FACTURE, cf->idfacture);
			QRY_CREATE_REL(idlignefact, MES_LIGNE_FACT, idmesure);
		}

		/* Mise a jour des valeurs calculees */
		QRY_UPDATE_REL(idlignefact, DEST_FACTURE, iddest);
		if(nbjours) 
		{
			QRY_UPDATE_VAL(idmesure, ANNEE_FIN, NULL, 0);
			QRY_UPDATE_VAL(idlignefact, QTE_FACTURE, txt, snprintf(add_sz_str(txt), "%d", nbjours));
		}
		else
		{
			QRY_UPDATE_VAL(idlignefact, QTE_FACTURE, NULL, 0);
		}
		if(nbjcorr) 
		{
			QRY_UPDATE_VAL(idlignefact, QTE_CORRECTION, txt, snprintf(add_sz_str(txt), "%d", nbjcorr));
		}
		else
		{
			QRY_UPDATE_VAL(idlignefact, QTE_CORRECTION, NULL, 0);
		}
		if(qry_update_idobj_idfield(cntxt, idlignefact, cf->val_MOTIF_CORRECTION, ano, 3)) STACK_ERROR;
	}

	printf("</table><br><br>"
			"<table border=1 rules=none width=100%%><tr><td align=center>%lu mesures examinées<br>%lu mesures modifiées<br>%lu mesures facturées</td></tr>"
			"<tr><td align=center><font size=-1><i>Utilisez la commande Fichier/Enregistrer "
				"de votre navigateur si vous voulez conserver ce journal</td></tr>"
			"<tr><td align=center><br><input type=submit value='Revenir à la facture' name='B$#NOP'></td></tr>"
			"</table>\n<script>document.mainform['B$#NOP'].focus();</script>",
			cf->nb_mes, cf->nb_modif, cf->nb_lignfact);

	cntxt->b_terminate = 31;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : EcheancierAEMO
** Description : preparation de l'échancier mensuel
*********************************************************************/
#define ERR_FUNCTION "EcheancierAEMO"
#define ERR_CLEANUP	i_ctrl = 0
int EcheancierAEMO(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	cntxt_fact cf = {0};

	/* Lecture des mesures à examiner */
	if(Init_AEMO(cntxt, &cf) || Calc_echeancier(cntxt, &cf)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP


/*********************************************************************
** Function : Init_TPS
** Description : initialisation des constantes de facturation
*********************************************************************/
#define ERR_FUNCTION "Init_TPS"
#define ERR_CLEANUP	DYNTAB_FREE(data)
int Init_TPS(
	EVA_context *cntxt,				/* in/out : execution context data */
	cntxt_fact *cf					/* in/out : structure à initialiser */
){
	DynTable data = { 0 };
	unsigned long i, cnt = 0;
	char qry[2048] = {0};

	/* Définition des constantes spécifiques */
	cf->label = cf->idfacture ? "Facturation TPS" : "Echéancier TPS";
	cf->form_Mesure = 27108;
	cf->form_LigneFacture = 188313;
	cf->val_ANNEE_FIN = get_id_value(cntxt, add_sz_str("FIN_TPS"));
	cf->val_MES_LIGNE_FACT = get_id_value(cntxt, add_sz_str("TPS_LIGNE_FACT"));
	cf->val_QTE_FACTURE = get_id_value(cntxt, add_sz_str("MOIS_FACTURE"));
	cf->val_QTE_CORRECTION = get_id_value(cntxt, add_sz_str("MOIS_CORRECTION"));
	cf->val_MESURE_JUGT = get_id_value(cntxt, add_sz_str("REF_TPS"));
	cf->val_EFFET_JUGT = get_id_value(cntxt, add_sz_str("ORD_JUGTS_EFFET_DU"));
	cf->val_ECHEANCE_JUGT = get_id_value(cntxt, add_sz_str("ECHEANCE"));
	cf->val_DATE_JUGT = get_id_value(cntxt, add_sz_str("ORD_JUGTS_DATE"));

	/* Creation de la liste des mesures a traiter */
	if(Init_liste_mes(cntxt, cf)) STACK_ERROR;

	/* Lecture des donnees complementaires */
	printf("Lecture des donnees complementaires ...");
	fflush(stdout);
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("MIN(<-REF_TPS->ORD_JUGTS_EFFET_DU)"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("MAX(<-REF_TPS->ORD_JUGTS_DATE)"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("ETAT_FACTURATION"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("FACT_MODE"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_FACT_DEBUT"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_FACT_FIN"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("CAISSE_PF"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("FORCER_ETAT"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DATE_ECHEANCE"))) STACK_ERROR;
	if(qry_add_table_col(cntxt, &cf->tmes, add_sz_str("DELEGUE"))) STACK_ERROR;

	/* Affichage de la barre d'avancement */
	printf("<hr><font face=Courier><b><pre>Examen de %lu mesures\n%s", cf->nb_mes, WAIT_TXTSCALE);
	fflush(stdout);

	/* Pour chaque mesure a examiner */
	for(i = 0; i < cf->nb_mes; i++)
	{
		char *datedeb = cf->mes[i].deb;
		char *datefin = cf->mes[i].fin;
		char *d_echeance = dyntab_val(&cf->tmes, i, 9);
		DynTable *ano = &cf->mes[i].notes;
		unsigned long idval = get_id_value(cntxt, DYNTAB_VAL_SZ(&cf->tmes, i, 2));

		/* Barre d'avancement */
		if(i * 60 / cf->nb_mes >= cnt)
		{
			putc('*', stdout);
			fflush(stdout);
			cnt++;
		}

		/* Lecture des donnees de la mesure */
		cf->mes[i].id = DYNTAB_TOULRC(&cf->tmes, i, 0);
		strncpy(datedeb, dyntab_val(&cf->tmes, i, 2), 8);
		strncpy(cf->mes[i].statfact, dyntab_val(&cf->tmes, i, 3), sizeof(cf->mes->statfact));
		cf->mes[i].idvalstat = dyntab_cell(&cf->tmes, i, 3)->IdValue;
		strncpy(cf->mes[i].modefact, dyntab_val(&cf->tmes, i, 4), sizeof(cf->mes->modefact));
		strncpy(cf->mes[i].debfact, dyntab_val(&cf->tmes, i, 5), 8);
		strncpy(cf->mes[i].debfact, dyntab_val(&cf->tmes, i, 6), 8);
		cf->mes[i].dest = DYNTAB_TOULRC(&cf->tmes, i, 7);
		cf->mes[i].b_forcstat = dyntab_val(&cf->tmes, i, 8)[0] != 0;
		cf->mes[i].ts = DYNTAB_TOULRC(&cf->tmes, i, 10);

		/* Lecture du dernier jugement */
		snprintf(add_sz_str(qry),
			"SELECT DISTINCT TLink0.IdObj FROM TLink AS TLink0\n"
			"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj\n"
			"WHERE TLink0.DateDel IS NULL AND TLink0.IdField=%lu AND TLink0.IdRelObj=%lu\n"
			"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdValue=%lu\n",
			cf->val_MESURE_JUGT, cf->mes[i].id, cf->val_DATE_JUGT, idval);
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &data, 2) ||
			qry_obj_data(cntxt, &data, &data, NULL)) STACK_ERROR;
		strncpy(datefin, dyntab_field_val(&data, "ECHEANCE", ~0UL, 0), 8);

		/* Détermine si mesure terminée (main levée) */
		{
			char *effetdu = dyntab_field_val(&data, "ORD_JUGTS_EFFET_DU", ~0UL, 0);
			char *datejug = dyntab_val(&cf->tmes, i, 3);
			cf->mes[i].b_mainlevee = 
				!strcmp(dyntab_field_val(&data, "ORD_JUGTS_TYPE", ~0UL, 0), "Main levée") ||
				!strncmp(effetdu, datefin, 8) || !strncmp(datejug, datefin, 8);
		}

		/* Maj date echeance mesure si necessaire */
		if(strlen(datefin) > 7 && strcmp(d_echeance, datefin) &&
			qry_update_idobj_idfield_1(cntxt, cf->mes[i].id, cf->val_DATE_ECHEANCE, datefin, 8)) STACK_ERROR;

		/* Vérification dates debut / fin */
		if(*datedeb && datetxt_invalid(datedeb)) *datedeb = 0;
		if(*datefin && datetxt_invalid(datefin)) *datefin = 0;
		if(!*datedeb) DYNTAB_SET(ano, ano->nbrows, 0, "Debut de mesure inconnu");
		if(!*datefin) DYNTAB_SET(ano, ano->nbrows, 0, "Fin de mesure inconnue");
	}

	printf("*\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : FactureTPS
** Description : preparation d'une facture d'TPS
*********************************************************************/
#define ERR_FUNCTION "FactureTPS"
#define ERR_CLEANUP	DYNTAB_FREE(lignfact); \
					DYNTAB_FREE(data)
int FactureTPS(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	DynTable lignfact = { 0 };
	DynTable data = { 0 };
	char qry[2048] = {0};
	cntxt_fact _cf = {0}, *cf = &_cf;
	unsigned long i, j;
	char txt[32];

	/* Inhibit warning for unused param */
	i_ctrl = 0;

	/* Retour si facture non enregistree */
	if(!dyntab_sz(&form->id_obj, 0, 0))
	{
		DYNBUF_ADD_STR(&cntxt->debug_msg, "\n**** Traitement non effectué\n  -> Enregistrez d'abord la facture\n");
		RETURN_OK;
	}

	/* Preparation des donnees de facturation */
	cf->idfacture = DYNTAB_TOUL(&form->id_obj);
	if(Init_TPS(cntxt, cf) || Entete_journal(cntxt, cf)) STACK_ERROR;

	/* Pour chaque mesure en cours */
	for(i = 0; i < cf->nb_mes; i++)
	{
		unsigned long idmesure = cf->mes[i].id, idlignefact = 0, iddest = cf->mes[i].dest;
		char *datedeb = cf->mes[i].deb;
		char *datefin = cf->mes[i].fin;
		char datedebcor[16] = {0}, datefincor[16] = {0};
		char *modefact = cf->mes[i].modefact, *mindeb, *maxfin;
		int nbfact = 0, nbcorr = 0, b_calcmanuel;	  
		unsigned long statut;
		char *valstatut = cf->mes[i].statfact, *nouvstatut, *bgcolor = "FFFFFF";
		DynTableCell val = {0};
		DynTable *ano = &cf->mes[i].notes;

		/* Recherche si une ligne existe deja pour cette mesure dans la facture */
		snprintf(qry, sizeof(qry), 
				"SELECT TLink0.IdObj AS IdObj,TLink0.Num AS Num FROM TLink AS TLink0 \n"
				"INNER JOIN TLink AS TLink1 ON TLink0.IdObj=TLink1.IdObj \n"
				"INNER JOIN TLink AS TLink2 ON TLink0.IdObj=TLink2.IdObj \n"
				"WHERE TLink0.DateDel IS NULL AND TLink0.IdRelObj=%lu AND TLink0.IdField=%lu \n"
				"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdRelObj=%lu \n"
				"AND TLink2.DateDel IS NULL AND TLink2.IdField=%lu AND TLink2.IdRelObj=%lu \n"
				"GROUP BY IdObj ORDER BY IdObj DESC",
			cf->form_LigneFacture, cf->val_FORMSTAMP, cf->val_MES_LIGNE_FACT, idmesure, cf->val_FACTURE, cf->idfacture);
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;
		idlignefact = DYNTAB_TOUL(&data);

		/* Si plus d'un ligne : efface les lignes en trop */
		for(j = 1; j < data.nbrows; j++)
		{
			snprintf(add_sz_str(qry), "UPDATE TLink SET DateDel='%s',IdWhoDel=%lu WHERE DateDel IS NULL AND IdObj=%lu",
				cntxt->timestamp, DYNTAB_TOUL(&cntxt->id_user), DYNTAB_TOULRC(&data, j, 0));
			if(sql_exec_query(cntxt, qry)) STACK_ERROR;
		}

		/* Lecture des donnees de la ligne */
		DYNTAB_FREE(lignfact);
		if(idlignefact && qry_obj_idfield(cntxt, &lignfact, idlignefact, 0)) STACK_ERROR;

		/* Gestion du mode de facturation */
		b_calcmanuel = idlignefact && dyntab_field_cell(&lignfact, "CALC_MANUEL", ~0UL, 0);
		if(b_calcmanuel)
		{
			/* Calcul manuel : lecture des données de la ligne */
			nbfact = strtoul(dyntab_field_val(&lignfact, "MOIS_FACT", ~0UL, 0), NULL, 10);
			nbcorr = strtoul(dyntab_field_val(&lignfact, "MOIS_CORRECTION", ~0UL, 0), NULL, 10);
			if(dyntab_filter_field(ano, 0, &lignfact, "MOTIF_CORRECTION", ~0UL, NULL)) RETURN_ERR_MEMORY; 
			iddest = strtoul(dyntab_field_val(&lignfact, "DEST_FACTURE", ~0UL, 0), NULL, 10);
		}
		else if(!strcmp(modefact, "DateManuelle"))
		{
			if(qry_obj_field(cntxt, &data, idmesure, "MOTIF_FACT_MANUELLE")) STACK_ERROR;
			if(data.nbrows) DYNTAB_ADD_CELL(ano, ano->nbrows, 0, &data, 0, 0);
			strncpy(datedebcor, cf->mes[i].debfact, 8);
			strncpy(datefincor, cf->mes[i].finfact, 8);
		}

		/* Destinataire par defaut = CAF AIN */
		if(!iddest) iddest = 163248;

		/* Normalisation dates debut / fin */
		if(*datedebcor && datetxt_invalid(datedebcor)) *datedebcor = 0;
		if(*datefincor && datetxt_invalid(datefincor)) *datefincor = 0;
		if(!*datedebcor && *datedeb) strcpy(datedebcor, datedeb);
		if(!*datefincor && *datefin) strcpy(datefincor, datefin);

		/* Etat de facturation de la mesure */
		mindeb = (!*datedeb || strcmp(datedeb, datedebcor) > 0) ? datedebcor : datedeb;
		maxfin = !*datefincor ? "A" : strcmp(datefin, datefincor) < 0 ? datefincor : datefin;
		if(cf->mes[i].b_forcstat)
		{
			statut = cf->mes[i].idvalstat;
			nouvstatut = cf->mes[i].statfact;
		}
		else if(strcmp(maxfin, cf->datedebmois) <= 0)
		{
			/* Date de fin de facturation atteinte : mesure archivee */
			statut = cf->val_Archivee;
			nouvstatut = "Archivée";
			bgcolor = "#CCCCFF";
		}
		else if(strcmp(mindeb, cf->datefinmois) > 0)
		{
			statut = cf->val_En_attente;
			nouvstatut = "En attente";
			bgcolor = "#FFCCCC";
		}
		else
		{
			statut = cf->val_En_cours;
			nouvstatut = "En cours";
			bgcolor = "#CCFFCC";
			if(!cf->mes[i].ts) DYNTAB_SET(ano, ano->nbrows, 0, "Attente affectation TS");
		}

		/* Journal d'execution pour la mesure si applicable */
		if(cf->b_detail_journal || strcmp(valstatut, nouvstatut))
		{
			if(strcmp(valstatut, nouvstatut)) cf->nb_modif++;
			printf("<table border=0 cellpadding=1 cellspacing=0 width=800><tr>");
			if(form_eval_fieldexpr(cntxt, &data, 0,  idmesure, add_sz_str("ALLOCATAIRE->NOM,PRENOM"), NULL, 0)) STACK_ERROR; 
			dyntab_sort(&data, qsort_colval);
			printf("<td width=150><font size=-1>%s %s</td>", dyntab_val(&data, 1, 0), dyntab_val(&data, 0, 0));
			printf("<td width=80 align=right><font size=-1>");
			if(*datedeb) printf("%.2s/%.2s/%.4s", datedeb+6, datedeb+4, datedeb);
			printf("</td><td width=10 align=center>-</td><td width=80><font size=-1>");
			if(*datefin) printf("%.2s/%.2s/%.4s", datefin+6, datefin+4, datefin);
			printf("</td>");
			if(qry_obj_idfield(cntxt, &data, iddest, cf->val_NOM_SOCIETE)) STACK_ERROR;
			printf("<td width=120><font size=-2>%s</td>", dyntab_val(&data, 0, 0));
			printf("<td width=30 align=right>%d</td><td width=10>j</td><td width=20>%+d</td>", nbfact, nbcorr);
			printf("<td width=60><font size=-2>%s</td>", strcmp(valstatut, nouvstatut) ? valstatut : "");
			printf("<td width=80 bgcolor=%s><font size=-1>%s</td>", bgcolor, nouvstatut);
			printf("<td%s><font size=-2><b>", ano->nbrows ? " bgcolor=#FFDDBB" : "");
			for(j = 0; j < ano->nbrows; j++) printf("%s%s", j ? ", " : "", dyntab_val(ano, j, 0));
			printf("</b></font></td>");
			printf("</tr></table>");
			fflush(stdout);
		}

		/* Maj etat mesure et dates facturation */
		if(strcmp(valstatut, nouvstatut)) QRY_UPDATE_IDVAL(idmesure, ETAT_FACTURATION, statut);
		if(strcmp(cf->mes[i].debfact, datedebcor)) QRY_UPDATE_VAL(cf->mes[i].id, DATE_FACT_DEBUT, datedebcor, 0);
		if(strcmp(cf->mes[i].finfact, datefincor)) QRY_UPDATE_VAL(cf->mes[i].id, DATE_FACT_FIN, datefincor, 0);

		/* Ligne non necessaire : efface la ligne si elle existe et continue */
		if(statut != cf->val_En_cours && !b_calcmanuel)
		{
			if(idlignefact)
			{
				snprintf(add_sz_str(qry), "UPDATE TLink SET DateDel='%s',IdWhoDel=%lu WHERE DateDel IS NULL AND IdObj=%lu",
					cntxt->timestamp, DYNTAB_TOUL(&cntxt->id_user), idlignefact);
				if(sql_exec_query(cntxt, qry)) STACK_ERROR;
			}

			/* Maj mesure : encours et année de fin si terminée */
			QRY_UPDATE_IDVAL(idmesure, ETAT_FACTURATION, statut);
			QRY_UPDATE_VAL(idmesure, ANNEE_FIN, datefincor, 4);
			continue;
		}

		/* Mesure en cours : effacement année de fin */
		cf->nb_lignfact++;

		/* Si la ligne n'existe pas : creation */
		if(!idlignefact)
		{
			if(qry_add_new_obj(cntxt, &data, cf->form_LigneFacture)) STACK_ERROR;
			idlignefact = DYNTAB_TOUL(&data);
			QRY_CREATE_REL(idlignefact, FACTURE, cf->idfacture);
			QRY_CREATE_REL(idlignefact, MES_LIGNE_FACT, idmesure);
		}

		/* Mise a jour des valeurs calculees */
		QRY_UPDATE_REL(idlignefact, DEST_FACTURE, iddest);
		if(statut == cf->val_En_cours)
		{
			QRY_UPDATE_VAL(idmesure, ANNEE_FIN, NULL, 0);
			QRY_UPDATE_VAL(idlignefact, QTE_FACTURE, "1", 1);
		}
		else
		{
			QRY_UPDATE_VAL(idlignefact, QTE_FACTURE, NULL, 0);
		}
		if(nbcorr) 
		{
			QRY_UPDATE_VAL(idlignefact, QTE_CORRECTION, txt, snprintf(add_sz_str(txt), "%d", nbcorr));
		}
		else
		{
			QRY_UPDATE_VAL(idlignefact, QTE_CORRECTION, NULL, 0);
		}
		if(qry_update_idobj_idfield(cntxt, idlignefact, cf->val_MOTIF_CORRECTION, ano, 3)) STACK_ERROR;
	}

	printf("</table><br><br>"
			"<table border=1 rules=none width=100%%><tr><td align=center>%lu mesures examinées<br>%lu mesures modifiées<br>%lu mesures facturées</td></tr>"
			"<tr><td align=center><font size=-1><i>Utilisez la commande Fichier/Enregistrer "
				"de votre navigateur si vous voulez conserver ce journal</td></tr>"
			"<tr><td align=center><br><input type=submit value='Revenir à la facture' name='B$#NOP'></td></tr>"
			"</table>\n<script>document.mainform['B$#NOP'].focus();</script>",
			cf->nb_mes, cf->nb_modif, cf->nb_lignfact);

	cntxt->b_terminate = 31;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : EcheancierTPS
** Description : preparation de l'échancier mensuel
*********************************************************************/
#define ERR_FUNCTION "EcheancierTPS"
#define ERR_CLEANUP	i_ctrl = 0
int EcheancierTPS(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	cntxt_fact cf = {0};

	/* Lecture des mesures à examiner */
	if(Init_TPS(cntxt, &cf) || Calc_echeancier(cntxt, &cf)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

