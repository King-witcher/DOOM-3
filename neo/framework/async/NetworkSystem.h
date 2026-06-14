/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __NETWORKSYSTEM_H__
#define __NETWORKSYSTEM_H__


/*
===============================================================================

  Network System.

===============================================================================
*/

typedef struct {
	idStr		nickname;
	idStr		clan;
	short		ping;
	int			rate;
} scannedClient_t;

typedef struct {
	netadr_t	adr;
	idDict		serverInfo;
	int			ping;

	int			clients;

	int			OSMask;

//RAVEN BEGIN
// shouchard:  added favorite flag
	bool		favorite;	// true if this has been marked by a user as a favorite
	bool		dedicated;
// shouchard:  added performance filtered flag
	bool		performanceFiltered;	// true if the client machine is too wimpy to have good performance
//RAVEN END
} scannedServer_t;

typedef enum {
	SC_NONE = -1,
	SC_FAVORITE,
	SC_LOCKED,
	SC_DEDICATED,
	SC_PB,
	SC_NAME,
	SC_PING,
	SC_REPEATER,
	SC_PLAYERS,
	SC_GAMETYPE,
	SC_MAP,
	SC_ALL,
	NUM_SC
} sortColumn_t;

typedef struct {
	sortColumn_t			column;
	idList<int>::cmp_t*		compareFn;
	idList<int>::filter_t*	filterFn;
	const char*				description;
} sortInfo_t;

class idNetworkSystem {
public:
	virtual					~idNetworkSystem( void ) {}

	virtual void			Shutdown( void );

	virtual void			ServerSendReliableMessage( int clientNum, const idBitMsg &msg, bool inhibitRepeater = false );
	virtual void			RepeaterSendReliableMessage( int clientNum, const idBitMsg &msg, bool inhibitHeader = false, int including = -1 ) { }
	virtual void			RepeaterSendReliableMessageExcluding( int excluding, const idBitMsg &msg, bool inhibitHeader = false, int clientNum = -1 ) { } // NOTE: Message is sent to all viewers if clientNum is -1; excluding is used for playback.
	virtual void			ServerSendReliableMessageExcluding( int clientNum, const idBitMsg &msg, bool inhibitRepeater = false );
	virtual int				ServerGetClientPing( int clientNum );
	virtual int				ServerGetClientTimeSinceLastPacket( int clientNum );
	virtual int				ServerGetClientTimeSinceLastInput( int clientNum );
	virtual int				ServerGetClientOutgoingRate( int clientNum );
	virtual int				ServerGetClientIncomingRate( int clientNum );
	virtual float			ServerGetClientIncomingPacketLoss( int clientNum );
	virtual int				ServerGetClientNum( int clientId ) { return 0; }
	virtual	int				ServerGetServerTime( void ) { return 0; }

	// returns the new clientNum or -1 if there weren't any free.
	virtual int				ServerConnectBot( void ) { return 0; }

	virtual int				RepeaterGetClientNum( int clientId ) { return 0; }

	virtual void			ClientSendReliableMessage( const idBitMsg &msg );
	virtual int				ClientGetPrediction( void );
	virtual int				ClientGetTimeSinceLastPacket( void );
	virtual int				ClientGetOutgoingRate( void );
	virtual int				ClientGetIncomingRate( void );
	virtual float			ClientGetIncomingPacketLoss( void );
// RAVEN BEGIN
// ddynerman: added some utility functions
	// uses a static buffer, copy it before calling in game again
	virtual const char*		GetServerAddress( void ) { return ""; }
	virtual const char*		GetClientAddress( int clientNum ) { return ""; }
	virtual	void			AddFriend( int clientNum ) { }
	virtual void			RemoveFriend( int clientNum ) { }
	// for MP games
	virtual void			SetLoadingText( const char* loadingText ) { }
	virtual void			AddLoadingIcon( const char* icon ) { }
	virtual const char*		GetClientGUID( int clientNum ) { return ""; }
// RAVEN END

	virtual void			GetTrafficStats(  int &bytesSent, int &packetsSent, int &bytesReceived, int &packetsReceived ) const { }

	// server browser
	virtual int				GetNumScannedServers( void ) { return 0; }
	virtual const scannedServer_t*	GetScannedServerInfo( int serverNum ) { return NULL; }
	virtual const scannedClient_t*	GetScannedServerClientInfo( int serverNum, int clientNum ) { return NULL; }
	virtual void			AddSortFunction( const sortInfo_t &sortInfo ) { }
	virtual bool			RemoveSortFunction( const sortInfo_t &sortInfo ) { return false; }
	virtual void			UseSortFunction( const sortInfo_t &sortInfo, bool use = true ) { }
	virtual bool			SortFunctionIsActive( const sortInfo_t &sortInfo ) { return false; }

	// returns true if enabled
	virtual bool			HTTPEnable( bool enable ) { return false; }

	virtual void			ClientSetServerInfo( const idDict &serverSI ) { }
	virtual void			RepeaterSetInfo( const idDict &info ) { }

	virtual const char*		GetViewerGUID( int clientNum ) { return ""; }

private:
	scannedServer_t			scannedServer;
	scannedClient_t			scannedClient;
};

extern idNetworkSystem *	networkSystem;

#endif /* !__NETWORKSYSTEM_H__ */
