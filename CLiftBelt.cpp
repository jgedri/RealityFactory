/************************************************************************************//**
 * @file CLiftBelt.cpp
 * @author Ralph Deane
 * @brief CLifBelt class
 *//*
 * Copyright (c) Ralph Deane; All rights reserved
 ****************************************************************************************/

#include "RabidFramework.h"

/* ------------------------------------------------------------------------------------ */
//	Constructor
/* ------------------------------------------------------------------------------------ */
CLiftBelt::CLiftBelt()
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	Change = false;

	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");

	if(pSet)
	{
		for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
			pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
		{
// changed QD 12/15/05
// causes and endless loop if there are more than 1 LiftBelt entities...
// allow only 1 entity per level?
			// pEntity = geEntity_EntitySetGetNextEntity(pSet, NULL);
// end change
			LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);

			if(EffectC_IsStringNull(pSource->szEntityName))
			{
				char szName[128];
				geEntity_GetName(pEntity, szName, 128);
				pSource->szEntityName = szName;
			}

			// Ok, put this entity into the Global Entity Registry
			CCD->EntityRegistry()->AddEntity(pSource->szEntityName, "LiftBelt");

			pSource->Powerlevel = 0.0f;
			pSource->active = GE_FALSE;
			pSource->Fuel = 0.0f;
			pSource->bState = GE_FALSE;
		}
	}
}

/* ------------------------------------------------------------------------------------ */
//	Destructor
/* ------------------------------------------------------------------------------------ */
CLiftBelt::~CLiftBelt()
{
}

/* ------------------------------------------------------------------------------------ */
//	Tick
/* ------------------------------------------------------------------------------------ */
void CLiftBelt::Tick(geFloat dwTicks)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	geVec3d fVector = {0.0f, 1.0f, 0.0f};

	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");

	if(pSet)
	{
		for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
			pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
		{
// changed QD 12/15/05
			// pEntity = geEntity_EntitySetGetNextEntity(pSet, NULL);
// end change
			LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);

			CPersistentAttributes *theInv = CCD->ActorManager()->Inventory(CCD->Player()->GetActor());

			pSource->bState = GE_FALSE;

			if(!EffectC_IsStringNull(pSource->EnableAttr) || pSource->EnableAlways)
			{
				if(CCD->Player()->GetUseAttribute(pSource->EnableAttr) || pSource->EnableAlways)
				{
					if(!pSource->active)
					{
						if(!EffectC_IsStringNull(pSource->PowerAttr))
						{
							pSource->Fuel = (float)theInv->Value(pSource->PowerAttr);
							CCD->HUD()->ActivateElement(pSource->PowerAttr, true);

							if(pSource->Fuel > 0.0f)
							{
								pSource->active = GE_TRUE;
								pSource->Powerlevel = 0.0f;
							}
						}
						else
						{
							pSource->active = GE_TRUE;
							pSource->Powerlevel = 0.0f;
						}
					}

					if(pSource->active)
					{
						if(!EffectC_IsStringNull(pSource->PowerAttr))
						{
							if(pSource->Fuel == 0.0f)
							{
								pSource->active = GE_FALSE;
								CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
								Change = false;
								return;
							}

							if(pSource->Powerlevel > 0.0f)
							{
								pSource->Fuel -= (pSource->PowerUse*dwTicks*0.001f);

								if(pSource->Fuel < 0.0f)
									pSource->Fuel = 0.0f;

								theInv->Set(pSource->PowerAttr, (int)pSource->Fuel);
							}
						}

						pSource->bState = GE_TRUE;

						// changed QD 12/15/05
						CCD->ActorManager()->GetForce(CCD->Player()->GetActor(), 3, NULL, &(pSource->Powerlevel), NULL);
						// end change

						if(Change)
						{
							if(Increase)
							{
								if(pSource->Powerlevel < pSource->LiftForce)
								{
									pSource->Powerlevel += (pSource->AccelRate*dwTicks*0.001f);

									if(pSource->Powerlevel > pSource->LiftForce)
										pSource->Powerlevel = pSource->LiftForce;

									CCD->ActorManager()->SetForce(CCD->Player()->GetActor(), 3, fVector, pSource->Powerlevel, pSource->Powerlevel*0.5f);
								}
							}
							else
							{
								pSource->Powerlevel = 0.0f;

								if(pSource->DropFast)
									CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
							}
						}

						if(pSource->Powerlevel > 0.0f)
							CCD->ActorManager()->SetGravityTime(CCD->Player()->GetActor(), 0.0f);

						// geEngine_Printf(CCD->Engine()->Engine(), 0, 30, "Force = %f", pSource->Powerlevel);
					}

					Change = false;
					return;
				}
				else
				{
					if(pSource->active)
					{
						pSource->active = GE_FALSE;
						CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
					}
				}
			}
		}
	}

	Change = false;
}

/* ------------------------------------------------------------------------------------ */
//	ChangeLift
/* ------------------------------------------------------------------------------------ */
void CLiftBelt::ChangeLift(bool increase)
{
	Change = true;
	Increase = increase;
}

/* ------------------------------------------------------------------------------------ */
//	DisableHud
/* ------------------------------------------------------------------------------------ */
void CLiftBelt::DisableHud(const char *Attr)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	bool IsLift = false;

	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");

	if(pSet)
	{
		for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
			pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
		{
// changed QD 12/15/05
			// pEntity = geEntity_EntitySetGetNextEntity(pSet, NULL);
// end change
			LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);

			if(!EffectC_IsStringNull(pSource->EnableAttr))
			{
				if(!strcmp(Attr, pSource->EnableAttr))
					IsLift = true;
			}
		}

		if(IsLift)
		{
			pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");

			for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
				pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
			{
// changed QD 12/15/05
				// pEntity = geEntity_EntitySetGetNextEntity(pSet, NULL);
// end change
				LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);

				if(!EffectC_IsStringNull(pSource->EnableAttr))
				{
					if(CCD->Player()->GetUseAttribute(pSource->EnableAttr))
					{
						CCD->Player()->DelUseAttribute(pSource->EnableAttr);
						CCD->HUD()->ActivateElement(pSource->EnableAttr, false);

						if(pSource->active)
						{
							pSource->active = GE_FALSE;
							CCD->ActorManager()->RemoveForce(CCD->Player()->GetActor(), 3);
						}
					}
				}
			}
		}
	}
}

/* ------------------------------------------------------------------------------------ */
//	LocateEntity
//
//	Given a name, locate the desired item in the currently loaded level
//	..and return it's user data.
/* ------------------------------------------------------------------------------------ */
int CLiftBelt::LocateEntity(const char *szName, void **pEntityData)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	// Ok, check to see if there are liftbelts in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "LiftBelt");

	if(!pSet)
		return RGF_NOT_FOUND;							// No LiftBelt entities

	// Ok, we have liftbelts somewhere.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	    pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		LiftBelt *pSource = (LiftBelt*)geEntity_GetUserData(pEntity);

		if(!strcmp(pSource->szEntityName, szName))
		{
			*pEntityData = (void*)pSource;
			return RGF_SUCCESS;
		}
	}

	return RGF_NOT_FOUND;								// Sorry, no such entity here
}

/* ----------------------------------- END OF FILE ------------------------------------ */
