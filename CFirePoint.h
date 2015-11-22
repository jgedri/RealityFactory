/*
	CFirePoint.h:		FirePoint handler

	(c) 1999 Ralph Deane

	This file contains the class declaration for the CFirePoint
class that encapsulates weapon firing for
RGF-based games.
*/

#ifndef __RGF_CFIREPOINT_H_
#define __RGF_CFIREPOINT_H_

#pragma warning( disable : 4068 )

#include "genesis.h"

// class declaration for CRain
class CFirePoint : public CRGFComponent
{
public:
  CFirePoint();
  ~CFirePoint();	
  void Tick(float dwTicks);
  int LocateEntity(char *szName, void **pEntityData);
  int ReSynchronize();
};


#endif

