#ifndef __BSE_INTERFACE_H__
#define __BSE_INTERFACE_H__

/*
===============================================================================

	Minimal Quake 4 rvBSEManager interface for the DOOM 3 GPL engine.

	BSE = Beam System Effects, Raven's particle/effects system (replaces id's
	idDeclParticle/.prt path). The retail Quake 4 gamex86.dll receives a
	rvBSEManager* through gameImport_t.bse and calls back into it via the
	vtable, so this declaration MUST match quake4-sdk/source/bse/BSEInterface.h
	slot-for-slot (virtual dtor + 15 methods, in this exact order).

	For the boot-to-menu milestone the implementation (BSE_Manager.cpp) is a
	pure no-op: nothing in the game's Init / decl-registration / main-menu path
	issues a bse->* call. Real effects come later, post-menu.

===============================================================================
*/

enum {
	VIEWEFFECT_DOUBLEVISION = 0,
	VIEWEFFECT_SHAKE,
	VIEWEFFECT_TUNNEL
};

typedef enum {
	EC_IGNORE = 0,
	EC_IMPACT,
	EC_IMPACT_PARTICLES,

	EC_MAX,
} effectCategory_t;

class rvRenderEffectLocal;		// fwd only - the stub never dereferences it
// renderEffect_t (typedef of struct renderEffect_s) is provided by renderer/RenderWorld.h,
// which is included via the precompiled header before this file. Do not re-declare it here
// (a 'struct renderEffect_t' tag would clash with that typedef -> C2371).

// Interface to the effects system
class rvBSEManager {
public:
	virtual						~rvBSEManager( void ) {}

	virtual	bool				Init( void ) = 0;
	virtual	bool				Shutdown( void ) = 0;

	virtual	bool				PlayEffect( rvRenderEffectLocal *def, float time ) = 0;
	virtual	bool				ServiceEffect( rvRenderEffectLocal *def, float time ) = 0;
	virtual	void				StopEffect( rvRenderEffectLocal *def ) = 0;
	virtual	void				FreeEffect( rvRenderEffectLocal *def ) = 0;
	virtual	float				EffectDuration( const rvRenderEffectLocal *def ) = 0;

	virtual	bool				CheckDefForSound( const renderEffect_t *def ) = 0;

	virtual	void				BeginLevelLoad( void ) = 0;
	virtual	void				EndLevelLoad( void ) = 0;

	virtual	void				StartFrame( void ) = 0;
	virtual	void				EndFrame( void ) = 0;
	virtual bool				Filtered( const char *name, effectCategory_t category ) = 0;

	virtual void				UpdateRateTimes( void ) = 0;
	virtual bool				CanPlayRateLimited( effectCategory_t category ) = 0;
};

extern	rvBSEManager			*bse;

#endif /* !__BSE_INTERFACE_H__ */
