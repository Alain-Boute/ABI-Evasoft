/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dll_fap.c
** Description : calculs de tarifs pour FAP/TREBI
**      Author : Alain BOUTE
**     Created : Aug 12 2006
*********************************************************************/

#include "eva.h"

/* Tableau des fiches remises */
typedef struct _remise
{
	unsigned long id;			/* IdObj de la fiche remise */
	enum { RQte, RQteCum, RMont, RFix, RMarge }
			mode;				/* Type de remise */
	struct _remtab				/* Tableau de remise */
	{
		unsigned long qte;		/* quantite mini pour la remise */
		double mont;			/* montant mini pour la remise */
		double pc;				/* pourcentage de remise */
	} *r;
	unsigned long nbr;			/* nb de lignes de r */
}
	Remise;

/* Tableau des fiches catalogue */
typedef struct _categ
{
	unsigned long id;			/* IdObj de la fiche */
	unsigned long idrem;		/* IdObj de la fiche remise */
	double totHT;				/* total montants public */
	unsigned long qte;			/* total quantites */
}
	Categ;

/* Tableau des propositions de prix */
typedef struct _Prix
{
	unsigned long idrem;	/* IdObj de la fiche remise */
	int qte;				/* quantite mini pour la remise */
	double mont;			/* montant mini pour la remise */
	double pcrem;			/* pourcentage remise */
	char *meth;				/* methode de remise */
	int b_sel;				/* prix sélectionnable si non zero */
} Prix;

/* Tableau des articles du panier */
typedef struct _panier
{
	unsigned long id;		/* IdObj de la fiche panier */
	unsigned long qte;		/* quantite commandee */
	unsigned long idart;	/* IdObj de la fiche article */
	double pxpub;			/* prix public HT unitaire */
	double pa;				/* prix d'achat HT unitaire */
	double pv;				/* prix de vente HT * qte */
	double pcrem;			/* pourcentage remise */
	double poids;			/* poids article * qte */
	double lmax;			/* longueur max */
	double pv_min;			/* PV HT à marge mini */
	double pc_max;			/* % remise à marge mini */
	int no_pv_min;			/* prix plancher (marge mini) désactivé */
	char *meth;				/* methode utilisee pour le calcul de remise */
	unsigned long remart;	/* IdObj de la fiche remise article */
	DynTable idcat;			/* IdObj des fiches catégorie */
	DynTable idprod;		/* IdObj des fiches produits */
	unsigned long idrem;	/* IdObj fiche remise selectionnee */
	unsigned long idcatrem;	/* IdObj fiche catalogue pour la remise selectionnee */
	Prix *tabprix;			/* tableau de prix degressif */
	unsigned long nbprix;	/* # de lignes dans tabprix */
	int b_promo;			/* code promo : non nul si applique */
	int b_rem;				/* maj par CalcRemise : non nul si appliquee */
	int b_panier;			/* non nul si l'article est au panier */
}
	Panier;

/* Contexte du panier */
typedef struct _cxpanier
{
	EVA_context *cntxt;		/* contexte d'execution */
	unsigned long idrem;	/* remise globale */
	unsigned long idsess;	/* session du panier */
	unsigned long iduser;	/* utilisateur du panier */
	unsigned long u_pcrem;	/* remise utilisateur : % de base */
	int b_usrpro;			/* non nul si remise pro applicable */
	int b_usrttc;			/* non nul si affichage prix TTC */
	DynTable exotva;		/* catégorie de TVA applicable */
	DynTable usrcat;		/* remise utilisateur : catalogue / famille */
	DynTable usrart;		/* remise utilisateur : article */
	DynTable remcat;		/* remise utilisateur : remise */
	DynTable typremcat;		/* remise utilisateur : type de remise (article / famille / catalogue) */
	unsigned long totQTE;	/* nb total d'articles du panier */
	double totHT;			/* montant total HT après remise du panier */
	double totHrem;			/* montant total HT sans remise du panier */
	double totPV;			/* montant total HT apres remise - uniquement articles avec prix d'achat */
	double totPA;			/* prix d'achat total du panier */
	double totPoids;		/* poids total du panier */
	double Lmax;			/* longueur max */
	double txTVA;			/* taux de TVA */
	Categ *cat;				/* tableau des categories du panier */
	unsigned long nbcat;	/* nb de categories dans panier */
	Panier *panier;			/* tableau des articles du panier */
	unsigned long nbpanier;	/* nb d'articles dans panier */
	DynBuffer *idart;		/* liste des IdObj des articles du panier */
	double marge_min;		/* marge sur prix de vente minimum */
	int no_margemin;		/* ignore la marge mini si non zero */
	int no_rsecata;			/* ignore les remises catalogue si non zero */
	double pc_promo;		/* pourcentage de remise du code promo */
	DynTable id_promo;		/* IdObj des fiches article / catégorie en promo */
}
	CxPanier;

/*********************************************************************
** Fonction : qsort_remise
** Description : qsort callback pour les tables de remises
*********************************************************************/
int qsort_remise(const void *el1, const void *el2)
{
	struct _remtab *r1 = (struct _remtab *)el1;
	struct _remtab *r2 = (struct _remtab *)el2;
	double d1 = r1->qte, d2 = r2->qte;
	if(!d1 && !d2) { d1 = r1->mont; d2 = r2->mont; }
	return d1 == d2 ? 0 : d1 > d2 ? 1 : -1;
}

/*********************************************************************
** Fonction : qsort_tabprix
** Description : qsort callback pour les tableaux de prix
*********************************************************************/
int qsort_tabprix(const void *el1, const void *el2)
{
	Prix *r1 = (Prix *)el1;
	Prix *r2 = (Prix *)el2;
	return r1->pcrem == r2->pcrem ?
				r1->qte == r2->qte ? 0 :
					r1->qte > r2->qte ? 1 : -1 :
				r1->pcrem > r2->pcrem ? 1 : -1;
}

/*********************************************************************
** Function : FicheRemise
** Description : retourne la fiche remise demandee
*********************************************************************/
#define ERR_FUNCTION "FicheRemise"
#define ERR_CLEANUP DYNTAB_FREE(data);\
					DYNTAB_FREE(tab)
int FicheRemise(							/* return : 0 on success, other on error */
	CxPanier *cx,							/* in/out : execution context data */
	Remise **res,							/* out : fiche remise */
	unsigned long id						/* in : fiche demandee */
){
	EVA_context *cntxt = cx->cntxt;
	DynTable data = {0};
	DynTable tab = {0};
	static Remise *remise;
	static unsigned long nbremise;
	unsigned long i = 0;
	char *mode;
	Remise *r;
	if(!id || !res) RETURN_OK;

	/* Recherche de la fiche dans le buffer */
	while(i < nbremise && remise[i].id != id) i++;
	if(i < nbremise)
		/* Retourne la fiche trouvee dans le buffer */
		r = remise + i;
	else
	{
		/* Fiche non trouvee : ajoute au buffer */
		M_REALLOC(Remise, remise, nbremise);
		r = remise + nbremise - 1;
		memset(r, 0, sizeof(r[0]));
		r->id = id;

		/* Lecture fiche remise */
		if(qry_obj_field(cntxt, &data, id, "MODE_REMISE,QTE,MONTANT,REMISE,MARGE,ARRONDI_REMISE")) STACK_ERROR;
		mode = dyntab_field_val(&data, "MODE_REMISE", ~0UL, 0);
		if(dyntab_filter_field(&tab, 0, &data, "REMISE", ~0UL, NULL)) RETURN_ERR_MEMORY;

		/* Traitement selon type de remise */
		if(!*mode || !strcmp(mode, "Fixe"))
		{
			r->mode = RFix;
			r->nbr = 1;
			M_ALLOC(r->r, sizeof(r->r[0]));
			r->r->pc = atof(dyntab_val(&tab, 0, 0));
		}
		else if(!strcmp(mode, "Marge"))
		{
			r->mode = RMarge;
			r->nbr = 1;
			M_ALLOC(r->r, sizeof(r->r[0]));
			r->r->pc = atof(dyntab_field_val(&data, "MARGE", ~0UL, 0));
			r->r->mont = atof(dyntab_field_val(&data, "ARRONDI_REMISE", ~0UL, 0));
		}
		else
		{
			/* Remise par montant ou quantité */
			char *fld = "QTE";
			r->mode = RQte;
			M_ALLOC(r->r, sizeof(r->r[0]) * tab.nbrows);
			r->nbr = tab.nbrows;
			for(i = 0; i < tab.nbrows; i++)
					r->r[i].pc = atof(dyntab_val(&tab, i, 0));

			/* Lecture du mode de remise */
			if(!strcmp(mode, "Quantité cumulée"))
				r->mode = RQteCum;
			else if(!strcmp(mode, "Montant"))
			{
				r->mode = RMont;
				fld = "MONTANT";
			}

			/* Lecture du tableau des quantites / montants */
			tab.nbrows = 0;
			if(dyntab_filter_field(&tab, 0, &data, fld, ~0UL, 0)) RETURN_ERR_MEMORY;
			for(i = 0; i < tab.nbrows; i++)
			{
				struct _remtab *t = r->r + i;
				char *v = dyntab_val(&tab, i, 0);
				if(r->mode == RMont)
					t->mont = atof(v);
				else
					t->qte = strtoul(v, NULL, 10);
			}

			/* Tri du tableau par quantites / montants */
			qsort(r->r, r->nbr, sizeof(r->r[0]), qsort_remise);
		}
	}
	*res = r;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : AjoutPrix
** Description : ajoute une proposition de prix a la fiche panier
*********************************************************************/
#define ERR_FUNCTION "AjoutPrix"
#define ERR_CLEANUP 
int AjoutPrix(								/* return : 0 on success, other on error */
	CxPanier *cx,							/* in/out : execution context data */
	Panier *p,								/* in/out : fiche panier */
	unsigned long id,						/* in : IdObj fiche remise */
	int qte,								/* in : quantite commandee */
	double mont,							/* in : montant total commande */
	double pcrem,							/* in : pourcentage de remise */
	char *meth,								/* in : methode de remise */
	int opt									/* in : options de prix
												bit 0 : utilise la marge mini si non zero
												bit 1 : prix sélectionnable si non zero */
){
	EVA_context *cntxt = cx->cntxt;
	Prix *t;
	if(!p || !pcrem) RETURN_OK;

	/* Selection plancher de marge si necessaire */
	if(p->pc_max && pcrem > p->pc_max && (opt & 1))
	{
		pcrem = p->pc_max;
		meth = "Plancher";
	}

	/* Ajoute le prix au buffer */
	M_REALLOC(Prix, p->tabprix, p->nbprix);
	t = p->tabprix + p->nbprix - 1;
	t->idrem = id;
	t->qte = qte > 0 ? qte : 1;
	t->mont = mont;
	t->pcrem = pcrem;
	t->meth = meth;
	t->b_sel = (opt & 2) ? 1 : 0;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : TabPrix
** Description : produit le tableau de prix d'un article en code HTML
*********************************************************************/
#define ERR_FUNCTION "TabPrix"
#define ERR_CLEANUP M_FREE(buf)
int TabPrix(								/* return : 0 on success, other on error */
	CxPanier *cx,							/* in/out : execution context data */
	DynTable *res,							/* out : table to output result to */
	Panier *p								/* in : fiche panier */
){
	EVA_context *cntxt = cx->cntxt;
	EVA_form *form = cntxt->form;
	DynBuffer *buf = NULL;
	char txtres[64] = {0};
	unsigned long i = 0;
	double tva = cx->b_usrttc ? cx->txTVA : 0;
	double pc = 0;
	if(!p->pxpub) RETURN_OK;

	/* Entete du tableau */
	DYNBUF_ADD3(&buf, "<table class=TabPrix cellspacing=0><tr bgcolor=#FFAAAA>"
					"<td align=center>Qté</td>"
					"<td align=right><nobr>Prix <b>&euro;", tva ? "TTC" : "HT", 0, NO_CONV, "</td>"
					"<td align=center>%</td>"
					"<td>Remise</td></tr><tr");

	/* Ligne prix public */
	if(!p->pcrem)
		DYNBUF_ADD_STR(&buf, " bgcolor=#FFFFAA")
	DYNBUF_ADD3(&buf, "><td align=center>-</td>"
					"<td align=right><b>", txtres, snprintf(add_sz_str(txtres), "%.2lf", p->pxpub * (1. + tva / 100.)), NO_CONV, "</td>"
					"<td align=center>-</td>");
	DYNBUF_ADD3(&buf, "<td><font size=-2><nobr>", !p->nbprix ? "<b>Prix Net</b>" : "Prix public", 0, NO_CONV, "</td></tr>");

	/* Lignes remises par quantité */
	pc = p->pcrem - 0.001;
	for(i = 0; i < p->nbprix; i++)
	{
		if(p->tabprix[i].pcrem <= pc) continue;
		pc = p->tabprix[i].pcrem;
		if(p->pcrem == pc)
			DYNBUF_ADD_STR(&buf, "<tr bgcolor=#FFFFAA>")
		else
			DYNBUF_ADD_STR(&buf, "<tr>");
		DYNBUF_ADD_STR(&buf, "<td align=center>")
		if(p->tabprix[i].mont)
		{
			DYNBUF_ADD3_INT(&buf, "<font size=-1><i>", (int)(p->tabprix[i].mont + 0.5), "&euro;</i></font>");
		}
		else
		{
			DYNBUF_ADD_INT(&buf, p->tabprix[i].qte);
		}
		DYNBUF_ADD_STR(&buf, "</td><td align=right><b>")
		DYNBUF_ADD(&buf, txtres, snprintf(add_sz_str(txtres), "%.2lf",
						(100. - p->tabprix[i].pcrem) * p->pxpub / 100. *
						(1. + tva / 100.)), NO_CONV);
		DYNBUF_ADD3(&buf, "</td><td align=center><nobr>-", txtres, snprintf(add_sz_str(txtres), "%.0f", p->tabprix[i].pcrem), NO_CONV,
			"%</td><td><font size=-2>");
		if(!strcmp(p->tabprix[i].meth, "Plancher"))
			DYNBUF_ADD_STR(&buf, "<b>Plancher")
		else if(!strcmp(p->tabprix[i].meth, "Client") || !strcmp(p->tabprix[i].meth, "CatClient"))
		{
			DYNBUF_ADD3_INT(&buf, "<a href=javascript:ol(", cx->iduser, ",0,");
			DYNBUF_ADD_CELL(&buf, &form->id_form, 0, 0, NO_CONV);
			DYNBUF_ADD3_CELL(&buf, ",", &form->id_obj, 0, 0, NO_CONV, ") title=\"Affiche votre compte client\">");
			DYNBUF_ADD3(&buf, "", p->tabprix[i].meth, 0, NO_CONV, "</a>");
		}
		else
		{
			DYNBUF_ADD3_INT(&buf, "<a href=javascript:ol(", p->tabprix[i].idrem, ",0");
			DYNBUF_ADD3_CELL(&buf, ",", &form->id_form, 0, 0, NO_CONV, ",");
			DYNBUF_ADD_CELL(&buf, &form->id_obj, 0, 0, NO_CONV);
			DYNBUF_ADD3(&buf, ") title=\"+ d'infos sur la remise\">", p->tabprix[i].meth, 0, NO_CONV, "</a>");
		}
		DYNBUF_ADD_STR(&buf, "</td></tr>")
	}

	/* Ligne de total */
	DYNBUF_ADD3(&buf,
		"<tr><td colspan=4 bgcolor=#DDDDDD align=right>Total <b>&euro; ", tva ? "TTC" : "HT", 0, NO_CONV, "</td></tr>");
	DYNBUF_ADD3_INT(&buf,
		"<tr><td align=center>", p->qte, "</td><td align=right><b>");
	DYNBUF_ADD(&buf, txtres, snprintf(add_sz_str(txtres), "%.2lf", p->qte ? p->pv / p->qte * (1. + tva / 100.) : 0), NO_CONV);
	DYNBUF_ADD3(&buf, "</td><td colspan=2 align=right><b><font size=+0><nobr>", txtres, snprintf(add_sz_str(txtres), "%.2f", p->pv * (1. + tva / 100.)), NO_CONV, "</font>");
	DYNBUF_ADD_STR(&buf, "</td></tr></table>");
	DYNTAB_ADD_BUF(res, 0, 0, buf);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : CalcRemise
** Description : retourne le taux de remise correspondant à un montant ou une quantite
*********************************************************************/
#define ERR_FUNCTION "CalcRemise"
#define ERR_CLEANUP 
int CalcRemise(								/* return : 0 on success, other on error */
	CxPanier *cx,							/* in/out : execution context data */
	Panier *p,								/* in/out : fiche panier */
	unsigned long id,						/* in : IdObj fiche remise */
	unsigned long qte,						/* in : quantite demandee */
	double mont,							/* in : montant achete */
	char *meth								/* in : methode de remise */
){
	EVA_context *cntxt = cx->cntxt;
	unsigned long i = 0, q = qte;
	double res = 0, m = mont;
	Remise *r = NULL;
	if(!id) return 0;

	/* Lecture de la fiche remise */
	p->b_rem = 0;
	if(FicheRemise(cx, &r, id)) STACK_ERROR;

	if(r)
	{
		/* Traitement selon type de remise */
		switch(r->mode)
		{
		case RFix:
			res = r->r->pc;
			if(res) AjoutPrix(cx, p, id, 1, 0, res, meth, 3);
			break;
		case RQte:
		case RQteCum:
			if(r->mode == RQte)
			{
				q = p->qte;
				qte = 0;
			}
			else
			{
				if(p->b_panier)
					qte -= p->qte;
				else
					q += p->qte;
			}
			for(i = 0; i < r->nbr; i++) AjoutPrix(cx, p, id, r->r[i].qte - qte, 0, r->r[i].pc, meth, 1 | ((q >= r->r[i].qte ) ? 2 : 0));
			break;
		case RMont:
			if(p->b_panier)
				mont -= p->pxpub * p->qte;
			else
				m += p->pxpub * p->qte;
			for(i = 0; i < r->nbr; i++)
			{
				qte = (int)(ceil((r->r[i].mont - mont) / p->pxpub));
				AjoutPrix(cx, p, id, qte, r->r[i].mont, r->r[i].pc, meth, 1 | ((m >= r->r[i].mont) ? 2 : 0));
			}
			break;
		case RMarge:
			if(!p->pa) break;
			m = p->pa / (1. - r->r->pc / 100.);
			if(r->r->mont) m = r->r->mont * (int)(1 + m / r->r->mont);
			res = (1. - m / p->pxpub) * 100.;
			if(res) AjoutPrix(cx, p, id, 1, 0, res, meth, 3);
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : FicheCategorie
** Description : retourne la fiche catalogue demandee
*********************************************************************/
#define ERR_FUNCTION "FicheCategorie"
#define ERR_CLEANUP DYNTAB_FREE(data)
int FicheCategorie(							/* return : 0 on success, other on error */
	CxPanier *cx,							/* in/out : execution context data */
	Categ **res,							/* out : fiche catalogue */
	unsigned long id						/* in : fiche demandee */
){
	EVA_context *cntxt = cx->cntxt;
	DynTable data = {0};
	unsigned long i = 0;
	Categ *c;
	if(!res) RETURN_OK;
	*res = NULL;
	if(!id) RETURN_OK;

	/* Recherche de la fiche dans le buffer */
	while(i < cx->nbcat && cx->cat[i].id != id) i++;
	if(i < cx->nbcat)
		/* Retourne la fiche trouvee dans le buffer */
		c = cx->cat + i;
	else
	{
		/* Fiche non trouvee : ajoute au buffer */
		M_REALLOC(Categ, cx->cat, cx->nbcat);
		c = cx->cat + cx->nbcat - 1;
		memset(c, 0, sizeof(c[0]));
		c->id = id;
		if(qry_obj_field(cntxt, &data, id, "CATEG_REMISE,CATEG_REMPRO,PRODUIT_REMISE,PRODUIT_REMPRO")) STACK_ERROR;
		if(cx->b_usrpro)
		{
			c->idrem = strtoul(dyntab_field_val(&data, "CATEG_REMPRO", ~0UL, 0), NULL, 0);
			if(!c->idrem) 
				c->idrem = strtoul(dyntab_field_val(&data, "PRODUIT_REMPRO", ~0UL, 0), NULL, 0);
		}
		if(!c->idrem) 
			c->idrem = strtoul(dyntab_field_val(&data, "CATEG_REMISE", ~0UL, 0), NULL, 0);
		if(!c->idrem) 
			c->idrem = strtoul(dyntab_field_val(&data, "PRODUIT_REMISE", ~0UL, 0), NULL, 0);
	}
	*res = c;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : CheckPromo
** Description : Vérifie si la fiche panier fait l'objet d'un coupon promotion
*********************************************************************/
void CheckPromo(
	CxPanier *cx,							/* in : execution context data */
	Panier *p,								/* out : p->b_promo */
	unsigned long id						/* in : IdObj fiche article / catégorie à tester */
){
	unsigned long i;
	for(i = 0; i < cx->id_promo.nbrows; i++)
			if(cx->id_promo.cell[i].IdValObj == id)
			{
				p->b_promo = 1;
				return;
			}
}
/*********************************************************************
** Function : FichePanier
** Description : Lit la fiche panier demandee
*********************************************************************/
#define ERR_FUNCTION "FichePanier"
#define ERR_CLEANUP DYNTAB_FREE(data);\
					DYNTAB_FREE(d)
int FichePanier(							/* return : 0 on success, other on error */
	CxPanier *cx,							/* in/out : execution context data */
	Panier *p,								/* in/out : fiche panier */
	unsigned long id						/* in : IdObj fiche panier demandee */
){
	EVA_context *cntxt = cx->cntxt;
	DynTable data = {0};
	DynTable d = {0};
	DynTable *t;
	Categ *cat = NULL;
	unsigned long i;

	/* Lecture fiche panier */
	p->id = id;
	if(qry_obj_field(cntxt, &data, p->id, "QUANTITE,ARTICLE_PANIER,PRIX_FACT_HT,PRIX_PUB_HT,PRIX_ACHAT_HT,METH_REMISE")) STACK_ERROR;
	p->qte = strtoul(dyntab_field_val(&data, "QUANTITE", ~0UL, 0), NULL, 0);
	p->pxpub = atof(dyntab_field_val(&data, "PRIX_PUB_HT", ~0UL, 0)) / p->qte;
	p->pa = atof(dyntab_field_val(&data, "PRIX_ACHAT_HT", ~0UL, 0)) / p->qte;
	p->pv = atof(dyntab_field_val(&data, "PRIX_FACT_HT", ~0UL, 0));
	if(id) p->idart = strtoul(dyntab_field_val(&data, "ARTICLE_PANIER", ~0UL, 0), NULL, 0);
	p->meth = dyntab_field_val(&data, "METH_REMISE", ~0UL, 0);
	if(*p->meth) p->meth = mem_strdup(p->meth);
	else p->meth = NULL;

	/* Lecture fiche article */
	if(qry_obj_field(cntxt, &data, p->idart, "PRIX_VENTE_HT,PRIX_ACHAT_HT,POIDS,ARTICLE_REMISE,ARTICLE_REMPRO,LONGUEUR,LARGEUR,HAUTEUR,SANS_PLANCHER")) STACK_ERROR;
	if(qry_obj_field(cntxt, &p->idcat, p->idart, "CATALOGUE,FAMILLE,CATEGORIE,SOUS_CATEGORIE,MATIERE")) STACK_ERROR;
	if(cx->b_usrpro)
		p->remart = strtoul(dyntab_field_val(&data, "ARTICLE_REMPRO", ~0UL, 0), NULL, 0);
	if(!p->remart)
		p->remart = strtoul(dyntab_field_val(&data, "ARTICLE_REMISE", ~0UL, 0), NULL, 0);
	p->poids = atof(dyntab_field_val(&data, "POIDS", ~0UL, 0));
	p->no_pv_min = dyntab_field_cell(&data, "SANS_PLANCHER", ~0UL, 0) ? 1 : 0;
	if(!p->pxpub) p->pxpub = atof(dyntab_field_val(&data, "PRIX_VENTE_HT", ~0UL, 0));
	if(!p->pa) p->pa = atof(dyntab_field_val(&data, "PRIX_ACHAT_HT", ~0UL, 0));

	/* Lecture quantité CGI si fiche courante */
	if(p->idart == DYNTAB_TOUL(&cntxt->id_obj))
	{
		unsigned long qte;
		if(form_get_field_values(cntxt, &d, "QUANTITE", 0, p->idart)) STACK_ERROR;
		qte = strtoul(dyntab_val(&d, 0, 0), NULL, 0);
		if(qte <= 0)
		{
			if(!p->qte) p->qte = 1;
		}
		else if(qte < 10000000)
			p->qte = qte;
	}

	/* Ajout catégories fiches produit */
	d.nbrows = 0;
	if(form_eval_fieldexpr(cntxt, &p->idprod, 0, p->idart, add_sz_str("<-PRODUIT_ARTICLE"), NULL, 0) ||
		p->idprod.nbrows &&
		qry_listobj_field(cntxt, &d, &p->idprod, "CATALOGUE,FAMILLE,CATEGORIE,SOUS_CATEGORIE,MATIERE")) STACK_ERROR;
	for(i = 0; i < d.nbrows; i++) DYNTAB_ADD_CELL(&p->idcat, p->idcat.nbrows, 0, &d, i, 0);
	if(dyntab_group(&p->idcat, "DISTINCT")) STACK_ERROR;

	/* Contrôle si code promo applicable */
	if(!cx->id_promo.nbrows)
		p->b_promo = 1;
	else
	{
		CheckPromo(cx, p, p->idart);
		for(i = 0; !p->b_promo && i < p->idcat.nbrows; i++) CheckPromo(cx, p, strtoul(p->idcat.cell[i].txt, NULL, 10));
		for(i = 0; !p->b_promo && i < p->idprod.nbrows; i++) CheckPromo(cx, p, strtoul(p->idprod.cell[i].txt, NULL, 10));
	}

	/* Calcul prix plancher de marge */
	if(!cx->no_margemin && !p->no_pv_min)
	{
		p->pv_min = p->pa * p->qte / (p->no_pv_min ? 1. : (1. - cx->marge_min / 100.));
		if(p->pxpub) p->pc_max = 100. * (1. - p->pv_min / p->qte / p->pxpub);
	}

	/* Valeurs cumulées si dans panier */
	if(!p->qte || !id) RETURN_OK;
	d.nbrows = 0;
	if(dyntab_filter_field(&d, 0, &data, "LONGUEUR", ~0UL, 0) ||
		dyntab_filter_field(&d, 0, &data, "LARGEUR", ~0UL, 0) ||
		dyntab_filter_field(&d, 0, &data, "HAUTEUR", ~0UL, 0) ||
		d.nbrows && dyntab_group(&d, "NMAX")) STACK_ERROR;
	if(d.nbrows) p->lmax = atof(dyntab_val(&d, 0, 0));
	if(cx->Lmax < p->lmax) cx->Lmax = p->lmax;

	/* Gestion totaux par catégorie et produit */
	for(t = &p->idcat; t; t = t == &p->idcat ? &p->idprod : NULL)
		for(i = 0; i < t->nbrows; i++)
		{
			if(FicheCategorie(cx, &cat, DYNTAB_TOULRC(t, i, 0))) STACK_ERROR;
			if(cat)
			{
				cat->qte += p->qte;
				cat->totHT += p->pxpub * p->qte;
			}
		}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : CalcRemArt
** Description : calcule les remises pour un article
*********************************************************************/
#define ERR_FUNCTION "CalcRemArt"
#define ERR_CLEANUP 
int CalcRemArt(								/* return : 0 on success, other on error */
	CxPanier *cx,							/* in : contexte panier courant */
	Panier *p								/* in : article panier a calculer */
){
	EVA_context *cntxt = cx->cntxt;
	Categ *cat = NULL;
	unsigned long i, j = 0;

	if(p->remart)
	{
		/* Remise par article */
		p->pcrem = 0;
		if(CalcRemise(cx, p, p->remart, p->qte, p->pxpub * p->qte, "Article")) STACK_ERROR;
	}
	else
	{
		/* Remise autre que par article : recherche du meilleur prix */

		/* Remise globale */
		if(!cx->no_rsecata && cx->idrem &&
			CalcRemise(cx, p, cx->idrem, cx->totQTE, cx->totHrem, "Magasin")) STACK_ERROR;

		/* Remise par produit */
		if(!cx->no_rsecata) for(i= 0; i < p->idprod.nbrows; i++)
		{
			if(FicheCategorie(cx, &cat, DYNTAB_TOULRC(&p->idprod, i, 0))) STACK_ERROR;
			if(cat && cat->idrem)
			{
				if(CalcRemise(cx, p, cat->idrem, cat->qte, cat->totHT, "Produit")) STACK_ERROR;
				if(p->b_rem) p->idcatrem = cat->id;
			}
		}

		/* Remise par categorie */
		if(!cx->no_rsecata) for(i= 0; i < p->idcat.nbrows; i++)
		{
			if(FicheCategorie(cx, &cat, DYNTAB_TOULRC(&p->idcat, i, 0))) STACK_ERROR;
			if(cat && cat->idrem)
			{
				char *fld = dyntab_cell(&p->idcat, i, 0)->field;
				if(!fld) fld = "???";
				if(CalcRemise(cx, p, cat->idrem, cat->qte, cat->totHT,
								!strcmp(fld, "CATALOGUE") ? "Catalogue" :
								!strcmp(fld, "FAMILLE") ? "Famille" :
								!strcmp(fld, "CATEGORIE") ? "Catégorie" :
								!strcmp(fld, "SOUS_CATEGORIE") ? "SousCatég" :
								!strcmp(fld, "MATIERE") ? "Matière" :
								fld)) STACK_ERROR;
				if(p->b_rem) p->idcatrem = cat->id;
			}
		}
		
		/* Remise par catalogue - compte client */
		for(i= 0; i < cx->typremcat.nbrows; i++)
		{
			char *typ = dyntab_val(&cx->typremcat, i, 0);
			unsigned long idrem = DYNTAB_TOULRC(&cx->remcat, i, 0);
			unsigned long idcat = DYNTAB_TOULRC(&cx->usrcat, i, 0);
			unsigned long idart = DYNTAB_TOULRC(&cx->usrart, i, 0);
			if(*typ == '1')
			{
				/* Remise spécifique sur un article */
				if(idart == p->idart && CalcRemise(cx, p, idrem, 0, 0, "CatClient")) STACK_ERROR;
			}
			else
			{
				/* Remise spécifique famille / catalogue */
				if(FicheCategorie(cx, &cat, idcat)) STACK_ERROR;
				if(cat) for(j = 0; j < p->idcat.nbrows; j++)
				{
					DynTableCell *c = dyntab_cell(&p->idcat, j, 0);
					if(strtoul(c->txt, NULL, 10) == cat->id)
					{
						if(CalcRemise(cx, p, idrem, cat->qte, cat->totHT, "CatClient")) STACK_ERROR;
						if(p->b_rem) p->idcatrem = cat->id;
					}
				}
			}
		}
		
		/* Remise client */
		if(cx->u_pcrem) AjoutPrix(cx, p, 0, 1, 0, cx->u_pcrem, "Client", 3);
		
		/* Remise code promo */
		if(cx->pc_promo && p->b_promo) AjoutPrix(cx, p, 0, 1, 0, cx->pc_promo, "Promo", 2);
	}

	/* Tri tableau de prix */
	if(p->nbprix)
	{
		qsort(p->tabprix, p->nbprix, sizeof(Prix), qsort_tabprix);
		for(i = p->nbprix - 1; i && !p->tabprix[i].b_sel; i--);
		if(p->tabprix[i].b_sel)
		{
			p->idrem = p->tabprix[i].idrem;
			p->pcrem = p->tabprix[i].pcrem;
			p->meth = p->tabprix[i].meth;
			p->b_rem = 1;
		}
	}

	/* Calcul prix de vente avant plancher */
	p->pv = p->pxpub * (1. - p->pcrem / 100.) * p->qte;

	/* Prix de vente arrondi à 2 décimales */
	p->pv = 0.01 * (int)(p->pv * 100. + 0.499);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : CalcPvArt
** Description : calcule le prix de vente d'un article
*********************************************************************/
#define ERR_FUNCTION "CalcPvArt"
#define ERR_CLEANUP 
int CalcPvArt(								/* return : 0 on success, other on error */
	CxPanier *cx,							/* in : contexte panier courant */
	Panier *p								/* in : article panier a calculer */
){
	EVA_context *cntxt = cx->cntxt;

	/* Calcule le prix et les remises si nécessaire */
	if(p->pv)
	{
		p->pcrem = (1 - p->pv / p->pxpub / p->qte) * 100;
	}
	else if(CalcRemArt(cx, p)) STACK_ERROR;

	/* Totaux */
	if(p->id && p->qte)
	{
		cx->totHT += p->pv;
		cx->totPoids += p->poids * p->qte;
		if(cx->idart) DYNBUF_ADD_STR(&cx->idart, ",");
		DYNBUF_ADD_INT(&cx->idart, p->idart);
		if(p->pa)
		{
			cx->totPV += p->pv;
			cx->totPA += p->pa * p->qte;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : LirePanier
** Description : lit l'ensemble du panier d'une session
*********************************************************************/
#define ERR_FUNCTION "LirePanier"
#define ERR_CLEANUP DYNTAB_FREE(data)
int LirePanier(								/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	CxPanier **ocx,							/* out : contexte panier pour le formulaire courant */
	Panier **art,							/* out : article panier si idobj est un article ou une fiche panier */
	unsigned long idobj						/* in : fiche sur laquelle s'applique la fonction */
){
	EVA_form *form = cntxt->form;
	DynTable data = {0};
	static CxPanier *tcx;
	static unsigned long nbcx;
	static Panier tmp;
	CxPanier *cx =NULL;
	Panier *p = NULL;
	unsigned long i;
	unsigned long idsess = DYNTAB_TOUL(&cntxt->session);
	unsigned long curobj = form ? DYNTAB_TOUL(&form->id_obj) : 0;
	char *formlabel = form ? DYNTAB_FIELD_VAL(&form->ctrl->attr, LABEL) : "";
	int mode_tarif = 0;
	unsigned long idpromo;

	/* Choix de la session selon le contexte */
	if(!strcmp(formlabel, "Session utilisateur") || !strcmp(formlabel, "Contenu d'un panier"))
	{
		/* Fiche courante = session : calcule le panier de la session affichee */
		idsess = curobj;
	}
	else if(dyntab_field_cell(&form->objdata, "PANIER_SESSION", ~0UL, 0))
	{
		/* Fiche courante = selection d'un article : calcule le panier correspondant */
		idsess = strtoul(dyntab_field_val(&form->objdata, "PANIER_SESSION", ~0UL, 0), NULL, 0);
	}
	else
	{
		/* Autres fiches : utilise la session courante et le mode de tarification de la fiche courante */
		if(form_get_field_values(cntxt, &data, "MODE_TARIF", 0, 0)) STACK_ERROR;
		mode_tarif = DYNTAB_TOUL(&data);
	}

	/* Recherche le panier pour la session */
	for(i = 0; i < nbcx; i++) if(tcx[i].idsess == idsess)
	{
		cx = tcx + i;
		break;
	}

	/* Lecture panier si necessaire */
	if(!cx)
	{
		/* Alloue la memoire pour un nouveau panier */
		M_REALLOC(CxPanier, tcx, nbcx);
		cx = tcx + nbcx - 1;
		memset(cx, 0, sizeof(cx[0]));

		/* Initialise le panier */
		cx->cntxt = cntxt;
		cx->idsess = idsess;

		/* Lecture données utilisateur */
		if(mode_tarif != 2)
		{
			if(qry_obj_field(cntxt, &data, cx->idsess, "CLIENT_DEVIS") ||
				!data.nbrows && qry_obj_field(cntxt, &data, cx->idsess, "_EVA_USER")) STACK_ERROR;
			cx->iduser = strtoul(dyntab_val(&data, 0, 0), NULL, 0);
			if(qry_obj_field(cntxt, &data, cx->iduser, "TYPE_REMISE_USER,ARTICLE_USER,CATALOG_USER,REMISE_USER,REMISE,REMISE_PRO,SIRET,MARGE_MIN,EXO_TVA")) STACK_ERROR;
			if(dyntab_filter_field(&cx->exotva, 1, &data, "EXO_TVA", ~0UL, 0) ||
				dyntab_filter_field(&cx->usrcat, 1, &data, "CATALOG_USER", ~0UL, 0) ||
				dyntab_filter_field(&cx->usrart, 1, &data, "ARTICLE_USER", ~0UL, 0) ||
				dyntab_filter_field(&cx->typremcat, 1, &data, "TYPE_REMISE_USER", ~0UL, 0) ||
				dyntab_filter_field(&cx->remcat, 1, &data, "REMISE_USER", ~0UL, 0) ||
				dyntab_order_lines(&cx->usrcat) ||
				dyntab_order_lines(&cx->usrart) ||
				dyntab_order_lines(&cx->remcat) ||
				dyntab_order_lines(&cx->typremcat)) RETURN_ERR_MEMORY;
			cx->u_pcrem = strtoul(dyntab_field_val(&data, "REMISE", ~0UL, 0), NULL, 0);
			cx->b_usrpro = dyntab_field_cell(&data, "REMISE_PRO", ~0UL, 0) != NULL;
			cx->b_usrttc = !dyntab_field_cell(&data, "SIRET", ~0UL, 0);
		}

		/* Taux de TVA applicable */
		{
			char *tx = "0";
			char *exo = dyntab_val(&cx->exotva, 0, 0);
			if(!strcmp(exo, "Corse")) tx = dyntab_field_val(&cntxt->cnf_data, "TVA_VT_RED", 0, 0);
			else if(!*exo) tx = dyntab_field_val(&cntxt->cnf_data, "TVA_VT_NORM", 0, 0);
			cx->txTVA = atof(tx);
		}
		
		/* Remise globale */
		if(cx->b_usrpro)
			cx->idrem = strtoul(dyntab_field_val(&cntxt->cnf_data, "REMPRO_GLOBALE", ~0UL, 0), NULL, 0);
		if(!cx->idrem)
			cx->idrem = strtoul(dyntab_field_val(&cntxt->cnf_data, "REMISE_GLOBALE", ~0UL, 0), NULL, 0);
		cx->no_rsecata = mode_tarif == 1;

		/* Marge minimum */
		cx->no_margemin = mode_tarif == 1;
		{
			char *mgmin = dyntab_field_val(&data, "MARGE_MIN", ~0UL, 0);
			if(!*mgmin) mgmin = dyntab_field_val(&cntxt->cnf_data, cx->b_usrpro ? "MARGE_MINI_PRO" : "MARGE_MIN", ~0UL, 0);
			cx->marge_min = atof(mgmin);
			if(cx->marge_min < 10 || cx->marge_min > 95) cx->marge_min = 0;
		}

		/* Lecture code promo si présent */
		if(form_get_field_values(cntxt, &data, "PROMO_SESSION", 0, idsess)) STACK_ERROR;
		idpromo = DYNTAB_TOUL(&data);
		if(idpromo)
		{
			if(qry_obj_field(cntxt, &data, idpromo, "REMISE_PROMO") ||
				qry_obj_field(cntxt, &cx->id_promo, idpromo, "ARTICLES_PROMO,PRODUITS_PROMO,FAMILLES_PROMO")) STACK_ERROR;
			cx->pc_promo = atof(dyntab_val(&data, 0, 0));
			for(i = 0; i < cx->id_promo.nbrows; i++) cx->id_promo.cell[i].IdValObj = strtoul(cx->id_promo.cell[i].txt, NULL, 10);
		}

		/* Lecture du panier */
		if(form_eval_fieldexpr(cntxt, &data, 0, cx->idsess, add_sz_str("<-PANIER_SESSION"), NULL, 0)) STACK_ERROR;
		M_ALLOC(cx->panier, (data.nbrows + 1) * sizeof(cx->panier[0]));
		cx->nbpanier = data.nbrows;
		
		/* Lecture des articles du panier */
		for(i = 0; i < cx->nbpanier; i++)
		{
			p = cx->panier + i;
			p->b_panier = 1;
			if(FichePanier(cx, p, DYNTAB_TOULRC(&data, i, 0))) STACK_ERROR;
			cx->totHrem += 0.01 * (int)(100. * p->pxpub * p->qte + 0.499);
			cx->totQTE += p->qte;
		}
		
		/* Calcul du prix de vente pour les articles du panier */
		for(i = 0; i < cx->nbpanier; i++)
			if(CalcPvArt(cx, cx->panier + i)) STACK_ERROR;
	}
	*ocx = cx;
	
	/* Recherche article courant */
	for(i = 0; i < cx->nbpanier; i++)
	{
		p = cx->panier + i;
		if(p->id == idobj || p->idart == idobj) break;
		p = NULL;
	}

	/* Fiche article pas dans le panier : lecture dans panier tampon */
	if(!p)
	{
		p = &tmp;
		if(p->idart != idobj)
		{
			memset(p, 0, sizeof(tmp));
			p->idart = idobj;
			if(FichePanier(cx, p, 0)) STACK_ERROR;
			if(!p->qte) p->qte = 1;
			if(CalcPvArt(cx, p)) STACK_ERROR;
		}
	}
	*art = p;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : TarifFAP
** Description : calcul de tarif pour paniers et BC FAP
*********************************************************************/
#define ERR_FUNCTION "TarifFAP"
#define ERR_CLEANUP DYNTAB_FREE(data)
int TarifFAP(								/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	DynTable *res,							/* out : table to output result to */
	unsigned long idobj,					/* in : fiche concernee */
	char *func								/* in : valeur a calculer */
){
	DynTable data = {0};
	CxPanier *cx = NULL;
	Panier *art = NULL;
	char txtres[64] = {0};

	/* Lecture du panier et de la fiche panier */
	if(LirePanier(cntxt, &cx, &art, idobj)) STACK_ERROR;

	/* Traitement selon fonction */
	if(!strcmp(func, "TotTTC"))
	{
		/* Total du panier avec remise */
		dbl_str(txtres, (int)(cx->totHT * (100. + cx->txTVA) + .499) / 100., NULL);
	}
	if(!strcmp(func, "TotHT"))
	{
		/* Total du panier avec remise */
		dbl_str(txtres, cx->totHT, NULL);
	}
	else if(!strcmp(func, "TotHrem"))
	{
		/* Total du panier sans remise */
		dbl_str(txtres, cx->totHrem, NULL);
	}
	else if(!strcmp(func, "MoyHrem"))
	{
		/* Remise globale du panier */
		dbl_str(txtres, cx->totHrem ? 100. * (1. - cx->totHT / cx->totHrem) : 0, NULL);
	}
	else if(!strcmp(func, "TotMarge"))
	{
		/* Montant marge du panier */
		dbl_str(txtres, cx->totPV - cx->totPA, NULL);
	}
	else if(!strcmp(func, "PcMarge"))
	{
		/* % marge du panier */
		dbl_str(txtres, cx->totPA ? 1 - cx->totPA / cx->totPV : 0, NULL);
	}
	if(!strcmp(func, "TotPoids"))
	{
		/* Total du poids du panier */
		dbl_str(txtres, cx->totPoids, NULL);
	}
	if(!strcmp(func, "Lmax"))
	{
		/* Longueur max du panier */
		dbl_str(txtres, cx->Lmax, NULL);
	}
	if(!strcmp(func, "IdArt"))
	{
		/* Liste des articles du panier */
		DYNTAB_ADD_BUF(res, 0, 0, cx->idart);
		RETURN_OK;
	}
	else if(art)
	{
		if(!strcmp(func, "RemArt"))
		{
			/* Methode remise sur un article du panier */
			if(art->meth) snprintf(add_sz_str(txtres), "%s", art->meth);
		}
		else if(!strcmp(func, "IdRemArt"))
		{
			/* IdObj remise sur un article du panier */
			if(art->idrem) 
			{
				DYNTAB_ADD_INT(res, 0, 0, art->idrem);
				RETURN_OK;
			}
		}
		else if(!strcmp(func, "IdCatRemArt"))
		{
			/* IdObj remise sur un article du panier */
			if(art->idcatrem) 
			{
				DYNTAB_ADD_INT(res, 0, 0, art->idcatrem);
				RETURN_OK;
			}
		}
		else if(!strcmp(func, "PcArt"))
		{
			/* Remise % sur un article du panier */
			dbl_str(txtres, art->pcrem, NULL);
		}
		else if(!strcmp(func, "ImgRem"))
		{
			/* Image remise d'un article du panier */
			if(art->idrem) 
			{
				if(qry_obj_field(cntxt, res, art->idrem, "IMAGE")) STACK_ERROR;
				RETURN_OK;
			}
		}
		else if(!strcmp(func, "PvArt"))
		{
			/* Prix vente unitaire HT d'un article du panier */
			dbl_str(txtres, art->pv / art->qte, NULL);
		}
		else if(!strcmp(func, "MargeArt"))
		{
			/* Marge sur un article du panier */
			dbl_str(txtres, (1. - art->pa / art->pv) * 100., NULL);
		}
		else if(!strcmp(func, "TotArt"))
		{
			/* Total d'un article du panier */
			dbl_str(txtres, art->pv, NULL);
		}
		else if(!strncmp(func, add_sz_str("TabPrixArt")))
		{
			/* Tableau de prix d'un article du panier */
			if(TabPrix(cx, res, art)) STACK_ERROR;
			RETURN_OK;
		}
	}
	dyntab_free(res);
	if(*txtres) DYNTAB_ADD(res, 0, 0, txtres, 0, NO_CONV);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
