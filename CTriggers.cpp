/*
CTriggers.cpp:		Trigger handler

  (c) 1999 Edward A. Averill, III
  
	This file contains the class implementation for the CTriggers
	class that encapsulates trigger handling in the RGF.
*/

//	You only need the one, master include file.

#include "RabidFramework.h"

extern geBoolean EffectC_IsStringNull(char *String );
extern bool GetTriggerState(char *TriggerName);
extern geSound_Def *SPool_Sound(char *SName);

//	CTriggers
//
//	Default constructor.  Set all triggers to default values and load
//	..any audio we need.

CTriggers::CTriggers()
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	
	m_TriggerCount = 0;					// No triggers
	
	//	Ok, check to see if there are triggers in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	
	if(!pSet) 
		return;									// No triggers
	
	//	Ok, we have triggers somewhere.  Dig through 'em all.
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);
		if(pTrigger->Model==NULL)
			continue;
		if(EffectC_IsStringNull(pTrigger->szEntityName))
		{
			char szName[128];
			geEntity_GetName(pEntity, szName, 128);
			pTrigger->szEntityName = szName;
		}
		m_TriggerCount++;							// Kick count
		// Ok, put this entity into the Global Entity Registry
		CCD->EntityRegistry()->AddEntity(pTrigger->szEntityName, "Trigger");
		// Now add the trigger into the Model Manager, which will do all the
		// ..hard work for us!
		if(!pTrigger->Model)
		{
			char szError[256];
			sprintf(szError,"Triggers : Missing Model '%s'", pTrigger->szEntityName);
			CCD->ReportError(szError, false);
			CCD->ShutdownLevel();
			delete CCD;
			MessageBox(NULL, szError,"Missing Trigger Model", MB_OK);
			exit(-333);
		}
		CCD->ModelManager()->AddModel(pTrigger->Model, ENTITY_TRIGGER);
		CCD->ModelManager()->SetLooping(pTrigger->Model, false);
		CCD->ModelManager()->SetAnimationSpeed(pTrigger->Model, pTrigger->AnimationSpeed);
		
		// Reset all the animation data for each and every trigger
		pTrigger->bInAnimation= GE_FALSE;	// Not animating
		pTrigger->AnimationTime = 0;		// No time in animation
		pTrigger->bTrigger = GE_FALSE;		// Not triggered
		pTrigger->time = 0.0f;
		pTrigger->isHit = GE_FALSE;
		pTrigger->CallBack = GE_FALSE;
		pTrigger->bInCollision = GE_FALSE;	// No collisions
		pTrigger->bActive = GE_TRUE;		// Trigger is good to go
		pTrigger->bState = GE_FALSE;		// Trigger state is off
		pTrigger->LastIncrement = 0;		// No last time count
		pTrigger->theSound = NULL;		// No sound, right now...
		pTrigger->SoundHandle = -1;		// No sound playing
		if(!EffectC_IsStringNull(pTrigger->szSoundFile))
		{
			pTrigger->theSound=SPool_Sound(pTrigger->szSoundFile);
			if(!pTrigger->theSound)
			{
				char szError[256];
				sprintf(szError,"Can't open audio file '%s'\n", pTrigger->szSoundFile);
				CCD->ReportError(szError, false);
			}
		}
	}
	
	//	Ok, we've counted the  triggers and reset 'em all to their default
	//	..values.  Leave!
	
	return;
}

//	~CTriggers
//
//	Default destructor

CTriggers::~CTriggers()
{
	return;
}

//	HandleCollision
//
//	Handle a collision with a  trigger

int CTriggers::HandleCollision(geWorld_Model *pModel, bool HitType)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	bool state;
	
	SetState();
	
	if(m_TriggerCount == 0)
		return RGF_FAILURE;									// None here, ignore call.
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	if(!pSet)
	{
		CCD->ReportError("CTriggers: handlecollision: no triggers", false);
		return RGF_FAILURE;
	}
	
	//	Once more we scan the list.  Does this get old, or what?
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		// Get the  data so we can compare models
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);
		if(pTrigger->Model == pModel)
		{
			if((!pTrigger->bShoot) && (HitType == true))
			{
				FreeState();
				if(CCD->ModelManager()->EmptyContent(pTrigger->Model))
					return RGF_EMPTY;
				return RGF_FAILURE;
			}
			state = true;
			if(!EffectC_IsStringNull(pTrigger->TriggerName))
				state = GetTTriggerState(pTrigger->TriggerName);
			if(!state)
			{
				pTrigger->CallBack = GE_TRUE;
				pTrigger->CallBackCount = 2;
			}
			// If trigger not already running, and active, trigger it!
			if((!pTrigger->bInAnimation) && (pTrigger->bActive == GE_TRUE) && pTrigger->isHit==false && state==true)
			{
				pTrigger->bState=true;
				pTrigger->bTrigger= true;			// It's this one, trigger the animation
				CCD->ModelManager()->Start(pTrigger->Model);
				pTrigger->SoundHandle = PlaySound(pTrigger->theSound, pTrigger->origin, pTrigger->bAudioLoops);
				pTrigger->isHit= true;
				pTrigger->time= 0.0f;
				pTrigger->bInAnimation = GE_TRUE;
			}
			FreeState();
			if(CCD->ModelManager()->EmptyContent(pTrigger->Model))
				return RGF_EMPTY;
			return RGF_SUCCESS;  // Hmmph, we hit a trigger
		}
		
	}
	FreeState();
	return RGF_FAILURE;							// We hit no known triggers
}


// MOD010122 - Added this function.
//	HandleTriggerEvent
//
//	Handle an animated model's trigger event

bool CTriggers::HandleTriggerEvent(char *TName)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	bool state;
	
	SetState();
	
	if(m_TriggerCount == 0)
		return false;									// None here, ignore call.
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	if(!pSet)
	{
		CCD->ReportError("CTriggers: handletriggerevent: no triggers", false);
		return false;
	}
	
	//	Once more we scan the list.  Does this get old, or what?
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		// Get the  data so we can compare models
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);
		if(strcmp(pTrigger->szEntityName, TName) == 0)
		{
			state = true;
			if(!EffectC_IsStringNull(pTrigger->TriggerName))
				state = GetTTriggerState(pTrigger->TriggerName);
			if(!state)
			{
				pTrigger->CallBack = GE_TRUE;
				pTrigger->CallBackCount = 2;
			}
			// If trigger not already running, and active, trigger it!
			if((!pTrigger->bInAnimation) && (pTrigger->bActive == GE_TRUE) && pTrigger->isHit==false && state==true)
			{
				pTrigger->bState=true;
				pTrigger->bTrigger= true;			// It's this one, trigger the animation
                //Let the model's tick routine start the animation
				//				CCD->ModelManager()->Start(pTrigger->Model);
				pTrigger->SoundHandle = PlaySound(pTrigger->theSound, pTrigger->origin, pTrigger->bAudioLoops);
				pTrigger->isHit= true;
				pTrigger->time= 0.0f;
				pTrigger->bInAnimation = GE_TRUE;
			}
			FreeState();
			return true;  // Hmmph, we hit a trigger
		}
		
	}
    FreeState();
    return false;							// We hit no known triggers
}

int CTriggers::PlaySound(geSound_Def *theSound, geVec3d Origin, bool SoundLoop)
{
	if(!theSound)
		return -1;
	
	Snd Sound;
	
	memset( &Sound, 0, sizeof( Sound ) );
    geVec3d_Copy( &(Origin), &( Sound.Pos ) );
    Sound.Min=kAudibleRadius;
	Sound.Loop=SoundLoop;
	Sound.SoundDef = theSound;
	int index = CCD->EffectManager()->Item_Add(EFF_SND, (void *)&Sound);
	if(SoundLoop)
		return index;
	return -1;
}

//	IsATrigger
//
//	Return TRUE if the passed-in model is a  trigger, FALSE otherwise

bool CTriggers::IsATrigger(geWorld_Model *theModel)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	
	if(m_TriggerCount == 0)
		return false;										// Don't waste time here.
	
	//	Ok, check to see if there are  triggers in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	
	if(!pSet) 
		return false;									// No  triggers
	
	//	Ok, we have  triggers somewhere.  Dig through 'em all.
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);
		if(pTrigger->Model == theModel)
			return true;							// Model IS a  trigger
	}
	
	return false;							// Nope, it's not a  trigger
}

//	Tick
//
//	Increment animation times for all _animating_  triggers that aren't
//	..in a collision state.

void CTriggers::Tick(float dwTicks)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	
	if(m_TriggerCount == 0)
		return;									// Don't waste time here
	
	//	Ok, check to see if there are  triggers in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	
	if(!pSet) 
		return;									// No  triggers
	
	//	Ok, we have triggers somewhere.  Dig through 'em all.
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);
		if(!pTrigger->Model)
			continue;
		pTrigger->LastIncrement = dwTicks;					// The amount that moved
		if(pTrigger->isHit==GE_TRUE)
		{
			pTrigger->time += dwTicks;
			if(pTrigger->bState==GE_TRUE)
			{
				if(pTrigger->time>=(pTrigger->TimeOn*1000.0f))
				{
					pTrigger->bState=GE_FALSE;
					pTrigger->time = 0.0f;
					// MOD010122 - Next line commented out to fix bug.  isHit should not be turned off until the full trigger 
					//             cycle, on-off is complete.
					//					pTrigger->isHit=false;
				}
			}
			else
			{
				if(pTrigger->time>=(pTrigger->TimeOff*1000.0f))
				{
					pTrigger->bState=GE_FALSE;
					pTrigger->isHit=false;
					pTrigger->time = 0.0f;
				}
			}
		}
		if(pTrigger->CallBack==GE_TRUE)
		{
			pTrigger->CallBackCount-=1;
			if(pTrigger->CallBackCount==0)
				pTrigger->CallBack=GE_FALSE;
		}
		if((pTrigger->bInAnimation == GE_TRUE) &&
			(CCD->ModelManager()->IsRunning(pTrigger->Model) == false))
		{
			// Animation has stopped/not running, handle it.
			// MOD010122 - The "&& (!pTrigger->isHit)" was added to the next if statement.  We don't want to reset until
			//             the full trigger cycle, on-off, is complete.
			if((pTrigger->bOneShot != GE_TRUE) && (pTrigger->bActive == GE_TRUE) && (!pTrigger->isHit))
			{
				// Ok, not one-shot, reset the door
				pTrigger->bInAnimation = GE_FALSE;
				pTrigger->bTrigger = false;
			}
			if(pTrigger->SoundHandle != -1)
			{
				CCD->EffectManager()->Item_Delete(EFF_SND, pTrigger->SoundHandle);
				pTrigger->SoundHandle = -1;
			}
		}
	}
	
	return;
}

//	SaveTo
//
//	Save the current state of every  trigger in the current world
//	..off to an open file.

int CTriggers::SaveTo(FILE *SaveFD)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	
	//	Ok, check to see if there are  triggers in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	
	if(!pSet) 
		return RGF_SUCCESS;									// No triggers, whatever...
	
	//	Ok, we have triggers somewhere.  Dig through 'em all.
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);
		fwrite(&pTrigger->bInAnimation, sizeof(geBoolean), 1, SaveFD);
		fwrite(&pTrigger->bTrigger, sizeof(geBoolean), 1, SaveFD);
		fwrite(&pTrigger->AnimationTime, sizeof(int), 1, SaveFD);
		fwrite(&pTrigger->bInCollision, sizeof(geBoolean), 1, SaveFD);
		fwrite(&pTrigger->bActive, sizeof(geBoolean), 1, SaveFD);
		fwrite(&pTrigger->LastIncrement, sizeof(int), 1, SaveFD);
		fwrite(&pTrigger->tDoor, sizeof(float), 1, SaveFD);
		fwrite(&pTrigger->origin, sizeof(geVec3d), 1, SaveFD);
		fwrite(&pTrigger->bState, sizeof(geBoolean), 1, SaveFD);
		fwrite(&pTrigger->time, sizeof(float), 1, SaveFD);
		fwrite(&pTrigger->isHit, sizeof(geBoolean), 1, SaveFD);
		fwrite(&pTrigger->CallBack, sizeof(geBoolean), 1, SaveFD);
		fwrite(&pTrigger->CallBackCount, sizeof(int), 1, SaveFD);
	}
	
	return RGF_SUCCESS;
}

//	RestoreFrom
//
//	Restore the state of every  trigger in the current world from an
//	..open file.

int CTriggers::RestoreFrom(FILE *RestoreFD)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	
	//	Ok, check to see if there are  triggers in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	
	if(!pSet) 
		return RGF_SUCCESS;									// No triggers, whatever...
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
    pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		Trigger *pTrigger = (Trigger*)geEntity_GetUserData(pEntity);
		fread(&pTrigger->bInAnimation, sizeof(geBoolean), 1, RestoreFD);
		fread(&pTrigger->bTrigger, sizeof(geBoolean), 1, RestoreFD);
		fread(&pTrigger->AnimationTime, sizeof(int), 1, RestoreFD);
		fread(&pTrigger->bInCollision, sizeof(geBoolean), 1, RestoreFD);
		fread(&pTrigger->bActive, sizeof(geBoolean), 1, RestoreFD);
		fread(&pTrigger->LastIncrement, sizeof(int), 1, RestoreFD);
		fread(&pTrigger->tDoor, sizeof(float), 1, RestoreFD);
		fread(&pTrigger->origin, sizeof(geVec3d), 1, RestoreFD);
		fread(&pTrigger->bState, sizeof(geBoolean), 1, RestoreFD);
		fread(&pTrigger->time, sizeof(float), 1, RestoreFD);
		fread(&pTrigger->isHit, sizeof(geBoolean), 1, RestoreFD);
		fread(&pTrigger->CallBack, sizeof(geBoolean), 1, RestoreFD);
		fread(&pTrigger->CallBackCount, sizeof(int), 1, RestoreFD);
		if(pTrigger->bInAnimation)
			geWorld_OpenModel(CCD->World(), pTrigger->Model, GE_TRUE);
    }
	
	return RGF_SUCCESS;
}

//	******************** CRGF Overrides ********************

//	LocateEntity
//
//	Given a name, locate the desired item in the currently loaded level
//	..and return it's user data.

int CTriggers::LocateEntity(char *szName, void **pEntityData)
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	
	//	Ok, check to see if there are  triggers in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	
	if(!pSet) 
		return RGF_NOT_FOUND;									// No triggers
	
	//	Ok, we have  triggers.  Dig through 'em all.
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity)) 
	{
		Trigger *pTheEntity = (Trigger*)geEntity_GetUserData(pEntity);
		if(strcmp(pTheEntity->szEntityName, szName) == 0)
		{
			*pEntityData = (void *)pTheEntity;
			return RGF_SUCCESS;
		}
	}
	
	return RGF_NOT_FOUND;								// Sorry, no such entity here
}

//	ReSynchronize
//
//	Correct internal timing to match current time, to make up for time lost
//	..when outside the game loop (typically in "menu mode").

int CTriggers::ReSynchronize()
{
	
	return RGF_SUCCESS;
}


void CTriggers::SetState()
{
	geEntity_EntitySet *pSet;
	geEntity *pEntity;
	TState *pool;
	
	Bottom = (TState *)NULL;
	
	if(m_TriggerCount == 0)
		return;						// Don't waste CPU cycles
	
	//	Ok, check to see if there are triggers in this world
	
	pSet = geWorld_GetEntitySet(CCD->World(), "Trigger");
	
	if(!pSet)
		return;		
	
	//	Ok, we have triggers somewhere.  Dig through 'em all.
	
	for(pEntity= geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
	pEntity= geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Trigger *pSource = (Trigger*)geEntity_GetUserData(pEntity);
		if(EffectC_IsStringNull(pSource->szEntityName))
		{
			char szName[128];
			geEntity_GetName(pEntity, szName, 128);
			pSource->szEntityName = szName;
		}
		pool = GE_RAM_ALLOCATE_STRUCT(TState);
		memset(pool, 0xbb, sizeof(TState));
		pool->next = Bottom;
		Bottom = pool;
		if(pool->next)
			pool->next->prev = pool;
		pool->Name=strdup(pSource->szEntityName);
		pool->state=pSource->bState;
	}
	
	return;
}

bool CTriggers::GetTTriggerState(char *Name)
{
	TState *pool;
	
	char *EntityType = CCD->EntityRegistry()->GetEntityType(Name);
	if(EntityType)
	{
		if(!stricmp(EntityType, "Trigger"))
		{
			pool=Bottom;
			while ( pool != NULL )
			{
				if(!stricmp(Name, pool->Name))
					return pool->state;
				pool = pool->next;
			}
			char szError[256];
			sprintf(szError,"Invalid Trigger Name '%s'\n", Name);
			CCD->ReportError(szError, false);
			return false;
		}
	}
	return GetTriggerState(Name);
}

void CTriggers::FreeState()
{
	TState *pool, *temp;
	pool = Bottom;
	while	(pool!= NULL)
	{
		temp = pool->next;
		free(pool->Name);
		geRam_Free(pool);
		pool = temp;
	}
}
