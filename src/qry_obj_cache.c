/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : qry_obj_cache.c
** Description : cache handling functions for objects read from DB
**      Author : Alain BOUTE
**     Created : Oct 7 2007
**********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : qry_cache_idobj_field
** Description : read a single object data
*********************************************************************/
#define ERR_FUNCTION "qry_cache_idobj_field"
#define ERR_CLEANUP	
int qry_cache_idobj_field(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result : object data */
	unsigned long idobj,			/* in : object Id */
	char *field,					/* in : field values to read - read all fields if NULL */
	int mode						/* in : bit mask
										bit 0 : copy data if not 0
										bit 1 : free res if not 0 */
){
	unsigned long i;

	/* Free result if applicable */
	if(mode & 2) dyntab_free(res);

	/* Search object in cache */
	for(i = 0; i < cntxt->oc_nb && cntxt->oc[i].id != idobj; i++);
	if(i >= cntxt->oc_nb)
	{
		/* Object is in not cache : check if space left in buffer */
		if(i >= nbelem(cntxt->oc))
		{
			/* No space left : read object from DB & return */
			if(qry_obj_field(cntxt, res, idobj, field)) STACK_ERROR;
			RETURN_OK;
		}

		/* Read data in cache */
		cntxt->oc_nb++;
		cntxt->oc[i].id = idobj;
		if(qry_obj_field(cntxt, &cntxt->oc[i].data, idobj, NULL)) STACK_ERROR;
	}

	/* Copy data to result */
	if(field)
	{
		if(dyntab_filter_field(res, 1, &cntxt->oc[i].data, field, 0, 0))	RETURN_ERR_MEMORY;
	}
	else
		if(dyntab_from_tab(res, res->nbrows, 0, &cntxt->oc[i].data, 0, 0, cntxt->oc[i].data.nbrows, 1, 1 | (mode & 1 ? 32 : 0))) RETURN_ERR_MEMORY;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_uncache_idobj
** Description : remove object data from cache
*********************************************************************/
void qry_uncache_idobj(
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idobj				/* in : object Id to remove from cache */
){
	unsigned long i;

	/* Search object in cache - return if not found */
	for(i = 0; i < cntxt->oc_nb && cntxt->oc[i].id != idobj; i++);
	if(i >= cntxt->oc_nb) return;

	/* Free cache position */
	dyntab_free(&cntxt->oc[i].data);
	cntxt->oc[i].id = 0;
}

/*********************************************************************
** Function : qry_cache_objlist_field
** Description : read one field from multiple objects data
*********************************************************************/
#define ERR_FUNCTION "qry_cache_objlist_field"
#define ERR_CLEANUP	
int qry_cache_objlist_field(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result : object data */
	DynTable *idobj,				/* in : objects Id list */
	char *field,					/* in : field values to read - read all fields if NULL */
	int mode						/* in : bit mask
										bit 0 : copy data if not 0
										bit 1 : free res if not 0 */
){
	unsigned long i;

	/* Read given objects from cache */
	if(mode & 2) dyntab_free(res);
	for(i = 0; i < idobj->nbrows; i++)
		if(qry_cache_idobj_field(cntxt, res, DYNTAB_TOULRC(idobj, i, 0), field, mode & ~2L)) RETURN_ERR_MEMORY;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
