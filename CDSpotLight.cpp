/************************************************************************************//**
 * @file CDSpotLight.cpp
 * @brief Dynamic Spotlight class implementation
 * @author Daniel Queteschiner
 *//*
 * Copyright (c) 2003 Daniel Queteschiner; All rights reserved.
 ****************************************************************************************/

#include "RabidFramework.h"

/* ------------------------------------------------------------------------------------ */
//	Constructor
/* ------------------------------------------------------------------------------------ */
CDSpotLight::CDSpotLight()
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	pSet = geWorld_GetEntitySet(CCD->World(), "DSpotLight");

	if(!pSet)
		return;						// None there.

	// Ok, we have dynamic lights somewhere.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	    pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		DSpotLight *pLight = (DSpotLight*)geEntity_GetUserData(pEntity);
		pLight->RadiusSpeed *= 1000.0f;			// From SECONDS to MILLISECONDS

		if(EffectC_IsStringNull(pLight->szEntityName))
		{
			char szName[128];
			geEntity_GetName(pEntity, szName, 128);
			pLight->szEntityName = szName;
		}

		// Ok, put this entity into the Global Entity Registry
		CCD->EntityRegistry()->AddEntity(pLight->szEntityName, "DSpotLight");

		// "negative light" shadow
		if(pLight->Shadow)
		{
			pLight->Color.r = -pLight->Color.r;
			pLight->Color.g = -pLight->Color.g;
			pLight->Color.b = -pLight->Color.b;
		}

		pLight->OriginOffset = pLight->origin;
		geVec3d_Scale(&(pLight->angles), GE_PIOVER180, &(pLight->RealAngle));

        if(pLight->Model)
		{
            geVec3d ModelOrigin;
	    	geWorld_GetModelRotationalCenter(CCD->World(), pLight->Model, &ModelOrigin);
            geVec3d_Subtract(&(pLight->origin), &ModelOrigin, &(pLight->OriginOffset));
  		}

		pLight->active = GE_FALSE;
		pLight->NumFunctionValues = strlen(pLight->RadiusFunction);

		if(pLight->NumFunctionValues == 0)
		{
			CCD->ReportError("[WARNING] DSpotLight has no RadiusFunction string\n", false);
			continue;
		}

		pLight->IntervalWidth = pLight->RadiusSpeed / (geFloat)pLight->NumFunctionValues;
	}
}

/* ------------------------------------------------------------------------------------ */
//	Destructor
/* ------------------------------------------------------------------------------------ */
CDSpotLight::~CDSpotLight()
{
}

/* ------------------------------------------------------------------------------------ */
//	Tick
/* ------------------------------------------------------------------------------------ */
geBoolean CDSpotLight::Tick(geFloat dwTicks)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	if(CCD->World() == NULL)
		return GE_TRUE;

	pSet = geWorld_GetEntitySet(CCD->World(), "DSpotLight");

	if(!pSet)
		return GE_TRUE;

	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		DSpotLight *Light;
		geFloat		Radius;
		geFloat		Percentage;
		int			Index;
		geVec3d		Pos;
		int32		Leaf;

		Light = (DSpotLight*)geEntity_GetUserData(pEntity);

		if(!EffectC_IsStringNull(Light->TriggerName))
		{
			if(GetTriggerState(Light->TriggerName))
			{
				if(Light->active == GE_FALSE)
				{
					Light->DynLight = geWorld_AddLight(CCD->World());
					Light->active = GE_TRUE;
				}
			}
			else
			{
				if(Light->active == GE_TRUE)
					geWorld_RemoveLight(CCD->World(), Light->DynLight);

				Light->active = GE_FALSE;
			}
		}
		else
		{
			if(Light->active == GE_FALSE)
			{
				Light->DynLight = geWorld_AddLight(CCD->World());
				Light->active = GE_TRUE;
			//	CCD->ReportError("DSpotLight light added", false);
			}
		}

		if(Light->active == GE_TRUE)
		{
			// pass the OriginOffset to SetOrigin
            // so that the light will stay in the same position relative to the model.
            Light->origin = Light->OriginOffset;
			SetOriginOffset(Light->EntityName, Light->BoneName, Light->Model, &(Light->origin));
			geWorld_GetLeaf(CCD->World(), &(Light->origin), &Leaf);
			Pos = Light->origin;

			if(Light->Rotate)
			{
				if(Light->Model)
				{
					geXForm3d Xf;
					geVec3d  Tmp;
					geWorld_GetModelXForm(CCD->World(), Light->Model, &Xf);
					geXForm3d_GetEulerAngles(&Xf, &Tmp);
					geVec3d_Add(&(Light->RealAngle), &Tmp, &(Light->angles));

					geVec3d_Scale(&(Light->angles), GE_180OVERPI, &(Light->angles));
				}
				else if(!EffectC_IsStringNull(Light->EntityName))
				{
					// changed QD 07/15/06
					/*
					geXForm3d BoneXForm;
					geActor	*theActor;

					theActor = GetEntityActor(Light->EntityName);

					if(!EffectC_IsStringNull(Light->BoneName))
					{
						if(geActor_GetBoneTransform(theActor, Light->BoneName, &BoneXForm) != GE_TRUE)
							continue;					// No such bone
					}
					else
					{
						if(geActor_GetBoneTransform(theActor, RootBoneName(theActor), &BoneXForm) != GE_TRUE)
							continue;					// No such bone
					}

					geXForm3d_RotateY(&BoneXForm, Light->RealAngle.Y);
					geXForm3d_GetEulerAngles(&BoneXForm, &(Light->angles));
					Light->angles.Z += Light->RealAngle.Z;

					*/
					SetAngle(Light->EntityName, Light->BoneName, &(Light->angles));

					if(Light->RealAngle.X || Light->RealAngle.Y || Light->RealAngle.Z)
					{
						geXForm3d XForm, BoneXForm;

						geXForm3d_SetZRotation(&XForm, Light->RealAngle.Z);
						geXForm3d_RotateX(&XForm, Light->RealAngle.X);
						geXForm3d_RotateY(&XForm, Light->RealAngle.Y);

						geXForm3d_SetZRotation(&BoneXForm, Light->angles.Z);
						geXForm3d_RotateX(&BoneXForm, Light->angles.X);
						geXForm3d_RotateY(&BoneXForm, Light->angles.Y);

						geXForm3d_Multiply(&BoneXForm, &XForm, &XForm);
						geXForm3d_GetEulerAngles(&XForm, &(Light->angles));
					}
					// end change QD 07/15/06

					// convert to degrees
					geVec3d_Scale(&(Light->angles), GE_180OVERPI, &(Light->angles));

				}
			}


			Percentage = Light->LastTime / Light->RadiusSpeed;

			Index = (int)(Percentage * Light->NumFunctionValues);

			if(Light->InterpolateValues && Index < Light->NumFunctionValues - 1)
			{
				geFloat	Remainder;
				geFloat	InterpolationPercentage;
				int		DeltaValue;
				geFloat	Value;

				Remainder = (geFloat)fmod(Light->LastTime, Light->IntervalWidth);
				InterpolationPercentage = Remainder / Light->IntervalWidth;
				DeltaValue = Light->RadiusFunction[Index + 1] - Light->RadiusFunction[Index];
				Value = Light->RadiusFunction[Index] + DeltaValue * InterpolationPercentage;
				Percentage = ((geFloat)(Value - 'a')) / ((geFloat)('z' - 'a'));
			}
			else
				Percentage = ((geFloat)(Light->RadiusFunction[Index] - 'a')) / ((geFloat)('z' - 'a'));

			Radius = Percentage * (Light->MaxRadius - Light->MinRadius) + Light->MinRadius;

			// angles in degrees
			geWorld_SetSpotLightAttributes(CCD->World(),
										   Light->DynLight,
										   &Pos,
										   &(Light->Color),
										   Radius,
										   Light->arc,
										   &(Light->angles),
										   Light->style,
										   Light->CastShadows);

			Light->LastTime = (geFloat)fmod(Light->LastTime + dwTicks, Light->RadiusSpeed);

			if(EffectC_IsPointVisible(CCD->World(),
					CCD->CameraManager()->Camera(),
					&Pos,
					Leaf,
					EFFECTC_CLIP_LEAF ) == GE_FALSE)
			{
				geWorld_RemoveLight(CCD->World(), Light->DynLight);
				Light->active = GE_FALSE;
			}
		}
	}

	return GE_TRUE;
}

//	******************** CRGF Overrides ********************

/* ------------------------------------------------------------------------------------ */
//	LocateEntity
//
//	Given a name, locate the desired item in the currently loaded level
//	..and return it's user data.
/* ------------------------------------------------------------------------------------ */
int CDSpotLight::LocateEntity(const char *szName, void **pEntityData)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;

	// Ok, check to see if there are dynamic lights in this world
	pSet = geWorld_GetEntitySet(CCD->World(), "DSpotLight");

	if(!pSet)
		return RGF_NOT_FOUND;									// No dynamic lights

	// Ok, we have dynamic lights.  Dig through 'em all.
	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	    pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		DSpotLight *pTheEntity = (DSpotLight*)geEntity_GetUserData(pEntity);

		if(!strcmp(pTheEntity->szEntityName, szName))
		{
			*pEntityData = (void*)pTheEntity;
			return RGF_SUCCESS;
		}
	}

	return RGF_NOT_FOUND;								// Sorry, no such entity here
}

/* ------------------------------------------------------------------------------------ */
//	ReSynchronize
//
//	Correct internal timing to match current time, to make up for time lost
//	..when outside the game loop (typically in "menu mode").
/* ------------------------------------------------------------------------------------ */
int CDSpotLight::ReSynchronize()
{
	return RGF_SUCCESS;
}


/* ----------------------------------- END OF FILE ------------------------------------ */
