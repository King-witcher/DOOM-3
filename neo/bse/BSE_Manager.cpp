/*
===============================================================================

	No-op implementation of Raven's BSE (Beam System Effects) manager, just
	enough for the engine to hand a valid rvBSEManager* to the Quake 4
	gamex86.dll (gameImport_t.bse). Every method is a stub: the game makes no
	bse->* calls on the boot-to-menu path. Vtable order matches BSEInterface.h.

===============================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "BSEInterface.h"

class rvBSEManagerLocal : public rvBSEManager {
public:
	virtual bool	Init( void ) { return true; }
	virtual bool	Shutdown( void ) { return true; }

	virtual bool	PlayEffect( rvRenderEffectLocal *def, float time ) { return false; }
	virtual bool	ServiceEffect( rvRenderEffectLocal *def, float time ) { return true; }	// report "expired" so the game frees it
	virtual void	StopEffect( rvRenderEffectLocal *def ) { }
	virtual void	FreeEffect( rvRenderEffectLocal *def ) { }
	virtual float	EffectDuration( const rvRenderEffectLocal *def ) { return 0.0f; }

	virtual bool	CheckDefForSound( const renderEffect_t *def ) { return false; }

	virtual void	BeginLevelLoad( void ) { }
	virtual void	EndLevelLoad( void ) { }

	virtual void	StartFrame( void ) { }
	virtual void	EndFrame( void ) { }
	virtual bool	Filtered( const char *name, effectCategory_t category ) { return true; }	// filter everything out

	virtual void	UpdateRateTimes( void ) { }
	virtual bool	CanPlayRateLimited( effectCategory_t category ) { return false; }
};

static rvBSEManagerLocal	bseManagerLocal;
rvBSEManager *				bse = &bseManagerLocal;
