//----------------------------------------------------------------
// DeclPlayerModel.cpp
//
// Reconstructed for the Quake 4 port. The decl source is a braced block of
// quoted key/value pairs (see def/player.def):
//
//   playerModel model_player_marine {
//       "model"        "model_player_marine"
//       "def_head"     "..."
//       "def_head_ui"  "..."
//       "skin"         "..."
//       "team"         "marine"
//       "description"  "#str_..."
//       "snd_*"        "..."        // collected into the sounds dict
//   }
//----------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "DeclPlayerModel.h"

/*
=================
rvDeclPlayerModel::rvDeclPlayerModel
=================
*/
rvDeclPlayerModel::rvDeclPlayerModel() {
	headOffset.Zero();
}

/*
=================
rvDeclPlayerModel::Size
=================
*/
size_t rvDeclPlayerModel::Size( void ) const {
	return sizeof( rvDeclPlayerModel );
}

/*
=================
rvDeclPlayerModel::DefaultDefinition
=================
*/
const char *rvDeclPlayerModel::DefaultDefinition() const {
	return "{ \"model\" \"\" \"description\" \"\" }";
}

/*
=================
rvDeclPlayerModel::Parse
=================
*/
bool rvDeclPlayerModel::Parse( const char *text, const int textLength, bool noCaching ) {
	idLexer src;
	idToken	key, value;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	while ( 1 ) {
		if ( !src.ReadToken( &key ) ) {
			break;
		}
		if ( key == "}" ) {
			break;
		}
		if ( !src.ReadToken( &value ) ) {
			src.Warning( "unexpected end of playerModel decl" );
			MakeDefault();
			return false;
		}

		if ( !key.Icmp( "model" ) ) {
			model = value;
		} else if ( !key.Icmp( "def_head" ) ) {
			head = value;
		} else if ( !key.Icmp( "def_head_ui" ) || !key.Icmp( "ui_head" ) ) {
			uiHead = value;
		} else if ( !key.Icmp( "headOffset" ) ) {
			sscanf( value.c_str(), "%f %f %f", &headOffset.x, &headOffset.y, &headOffset.z );
		} else if ( !key.Icmp( "team" ) ) {
			team = value;
		} else if ( !key.Icmp( "skin" ) ) {
			skin = value;
		} else if ( !key.Icmp( "description" ) ) {
			description = value;
		} else if ( !key.Icmpn( "snd_", 4 ) ) {
			sounds.Set( key, value );
		}
		// unknown keys are tolerated (forward compatibility with mod content)
	}

	return true;
}

/*
=================
rvDeclPlayerModel::FreeData
=================
*/
void rvDeclPlayerModel::FreeData( void ) {
	model.Clear();
	head.Clear();
	headOffset.Zero();
	uiHead.Clear();
	team.Clear();
	skin.Clear();
	description.Clear();
	sounds.Clear();
}

/*
=================
rvDeclPlayerModel::Print
=================
*/
void rvDeclPlayerModel::Print( void ) {
	common->Printf( "playerModel %s: model '%s' head '%s' team '%s' skin '%s'\n",
		GetName(), model.c_str(), head.c_str(), team.c_str(), skin.c_str() );
}

/*
=================
rvDeclPlayerModel::Validate
=================
*/
bool rvDeclPlayerModel::Validate( const char *psText, int iTextLength, idStr &strReportTo ) const {
	return true;
}
