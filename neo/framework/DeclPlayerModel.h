//----------------------------------------------------------------
// DeclPlayerModel.h
//
// Reconstructed for the Quake 4 port: the retail Quake 4 engine registers this
// decl type (DECL_PLAYER_MODEL, keyword "playerModel"); the MP game dereferences
// the DATA MEMBERS of the returned decl directly (->team, ->sounds, ->headOffset,
// ...), so the field order and types below must match the Q4 1.4.2 SDK's
// framework/DeclPlayerModel.h exactly.
//----------------------------------------------------------------

#ifndef __DECLPLAYERMODEL_H__
#define __DECLPLAYERMODEL_H__

/*
===============================================================================

rvDeclPlayerModel

===============================================================================
*/

class rvDeclPlayerModel : public idDecl {
public:
	rvDeclPlayerModel();

	idStr					model;
	idStr					head;
	idVec3					headOffset;
	idStr					uiHead;
	idStr					team;
	idStr					skin;
	idStr					description;
	idDict					sounds;

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool noCaching );
	virtual void			FreeData( void );
	virtual void			Print( void );

	virtual	bool			RebuildTextSource( void ) { return( false ); }
	virtual bool			Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
};

#endif // __DECLPLAYERMODEL_H__
