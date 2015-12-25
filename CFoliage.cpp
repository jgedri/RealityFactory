/************************************************************************************//**
 * @file CFoliage.cpp
 * @brief Foliage class implementation
 * @author Dan Valeo
 * @date Jul 2004
 *//*
 * Copyright (c) 2004 Dan Valeo; All rights reserved.
 ****************************************************************************************/

#include "RabidFramework.h"
#include "CFoliage.h"
#include <Ram.h>

extern geBitmap *TPool_Bitmap(const char *DefaultBmp, const char *DefaultAlpha,
							  const char *BName, const char *AName);

/* ------------------------------------------------------------------------------------ */
// Constructor
/* ------------------------------------------------------------------------------------ */
CFoliage::CFoliage()
{
	geEntity_EntitySet *pSet = geWorld_GetEntitySet(CCD->World(), "Foliage");

	if(!pSet)
		return;

	geEntity *pEntity;

	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Foliage *S = static_cast<Foliage*>(geEntity_GetUserData(pEntity));

		if(EffectC_IsStringNull(S->szEntityName))
		{
			// Get the Entity Name
			char szName[128];
			geEntity_GetName(pEntity, szName, 128);
			S->szEntityName = szName;
		}

		// Add it to Entity Registry
		CCD->EntityRegistry()->AddEntity(S->szEntityName, "Foliage");

		if(!EffectC_IsStringNull(S->BitmapName))
		{
			char BmpName[256];

			// build bmp and alpha names
			sprintf(BmpName, "%s%s", S->BitmapName, ".bmp"); // Kept for future implementation of animated foliage

			if(!EffectC_IsStringNull(S->AlphamapName))
			{
				char AlphaName[256];
				sprintf(AlphaName, "%s%s", S->AlphamapName, ".bmp");// Kept for future implementation of animated foliage
				S->Bitmap = TPool_Bitmap(BmpName, AlphaName, NULL, NULL);
			}
			else
			{
				S->Bitmap = TPool_Bitmap(BmpName, BmpName, NULL, NULL);
			}
		}
	}
}


/* ------------------------------------------------------------------------------------ */
// Destructor
/* ------------------------------------------------------------------------------------ */
CFoliage::~CFoliage()
{
}


/* ------------------------------------------------------------------------------------ */
// Tick
/* ------------------------------------------------------------------------------------ */
void CFoliage::Tick(geFloat dwTicks)
{
	geEntity_EntitySet *pSet = geWorld_GetEntitySet(CCD->World(), "Foliage");

	if(!pSet)
		return;

	geEntity *pEntity;
	geFloat range_squared, test, half;
	geVec3d cPos;
	char Texture[32];

	for(pEntity=geEntity_EntitySetGetNextEntity(pSet, NULL); pEntity;
		pEntity=geEntity_EntitySetGetNextEntity(pSet, pEntity))
	{
		Foliage *S = static_cast<Foliage*>(geEntity_GetUserData(pEntity));

		// check distance from camera
		CCD->CameraManager()->GetPosition(&cPos);

		range_squared = geVec3d_DistanceBetweenSquared(&S->origin, &cPos);

		if(range_squared < (S->Range * S->Range))
		{
			GE_LVertex	Vertex;
			GE_Collision Collision;
			geVec3d fPos,tPos;
			int sc;
			float scale;

			// Randomize using specified Seed
			srand(S->Seed);

			// set poly color
			Vertex.r = S->Color.r;
			Vertex.g = S->Color.g;
			Vertex.b = S->Color.b;

			Vertex.u = 0.0f;
			Vertex.v = 0.0f;

			if(S->Hanging)
			{
				test = -10000.0f;
				half = -0.5f;
			}
			else
			{
				test = 10000.0f;
				half = 0.5f;
			}

			for(sc=0; sc<S->Density; ++sc)
			{
				// Create random position and scale
				float sl = S->sMax - S->sMin;

				if(sl == 0.f)
					sl = 1.0;
				else
					scale = static_cast<float>((rand() % 10) * (sl/10.0f)) + S->sMin;

				fPos.X = S->origin.X +((rand() % (S->Diameter * 2)) - S->Diameter);
				fPos.Z = S->origin.Z +((rand() % (S->Diameter * 2)) - S->Diameter);
				fPos.Y = S->origin.Y;

				//perform collision check
				tPos = fPos;
				tPos.Y -= test; // set test point to 10000 texels below/above origin

				// perform BSP collision test only
				geWorld_Collision(CCD->World(), NULL, NULL,
					&fPos, &tPos, GE_CONTENTS_SOLID_CLIP | GE_CONTENTS_WINDOW,
					GE_COLLIDE_MODELS, 0x0, NULL, NULL, &Collision);

				// set new position
				Vertex.X = Collision.Impact.X;
				Vertex.Y = Collision.Impact.Y + ((geBitmap_Height(S->Bitmap)*scale)*half); // add half height of image
				Vertex.Z = Collision.Impact.Z;

				// set vertex alpha
				if(S->UseAlpha)
				{
					float pD = geVec3d_DistanceBetweenSquared(&cPos, &Collision.Impact);

					Vertex.a = 255.f * (pD / ((S->Range - S->Diameter) * (S->Range - S->Diameter)));
					Vertex.a = 255.f - Vertex.a;

					if(Vertex.a > 255.f)
						Vertex.a = 255.f;

					if(Vertex.a < 0.f)
						Vertex.a = 0.f;
				}
				else
					Vertex.a = 255.f;

				// check that texture matches allowable texture
				if(!EffectC_IsStringNull(S->Texture))
				{
					getSingleTextureNameByTrace(CCD->World(), &fPos, &tPos, Texture);
					if(strcmp(Texture,S->Texture) != 0)
						Vertex.a = 0.f; // prevent it from being drawn
				}

				// only draw if visible
				if(Vertex.a > 0.f)
				{
					// draw poly facing camera
					geWorld_AddPolyOnce(CCD->World(), &Vertex, 1,
						S->Bitmap, GE_TEXTURED_POINT,
						GE_RENDER_DEPTH_SORT_BF | GE_RENDER_DO_NOT_OCCLUDE_OTHERS,
						scale);
				}
			}
		}
	}

	srand(static_cast<unsigned int>(dwTicks * 1000.f));
}


/* ----------------------------------- END OF FILE ------------------------------------ */
