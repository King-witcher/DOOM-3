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

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

/*
===============================================================================

	File System

	No stdio calls should be used by any part of the game, because of all sorts
	of directory and separator char issues. Throughout the game a forward slash
	should be used as a separator. The file system takes care of the conversion
	to an OS specific separator. The file system treats all file and directory
	names as case insensitive.

	The following cvars store paths used by the file system:

	"fs_basepath"		path to local install, read-only
	"fs_savepath"		path to config, save game, etc. files, read & write
	"fs_cdpath"			path to cd, read-only
	"fs_devpath"		path to files created during development, read & write

	The base path for file saving can be set to "fs_savepath" or "fs_devpath".

===============================================================================
*/

static const ID_TIME_T		FILE_NOT_FOUND_TIMESTAMP	= 0xFFFFFFFF;
static const int		MAX_PURE_PAKS				= 128;
// Quake 4 1.4.2 SDK: master server can keep server updated with a list of allowed paks per OS
static const int		MAX_GAMEPAK_PER_OS			= 10;
static const int		MAX_OSPATH					= 256;

// modes for OpenFileByMode. used as bit mask internally
typedef enum {
	FS_READ		= 0,
	FS_WRITE	= 1,
	FS_APPEND	= 2
} fsMode_t;

typedef enum {
	PURE_OK,		// we are good to connect as-is
	PURE_RESTART,	// restart required
	PURE_MISSING,	// pak files missing on the client
	PURE_NODLL		// no DLL could be extracted
} fsPureReply_t;

typedef enum {
	DLTYPE_URL,
	DLTYPE_FILE
} dlType_t;

typedef enum {
	DL_WAIT,		// waiting in the list for beginning of the download
	DL_INPROGRESS,	// in progress
	DL_DONE,		// download completed, success
	DL_ABORTING,	// this one can be set during a download, it will force the next progress callback to abort - then will go to DL_FAILED
	DL_FAILED
} dlStatus_t;

typedef enum {
	FILE_EXEC,
	FILE_OPEN
} dlMime_t;

typedef enum {
	FIND_NO,
	FIND_YES,
	FIND_ADDON
} findFile_t;

typedef struct urlDownload_s {
	idStr				url;
	char				dlerror[ MAX_STRING_CHARS ];
	int					dltotal;
	int					dlnow;
	int					dlstatus;
	dlStatus_t			status;
} urlDownload_t;

typedef struct fileDownload_s {
	int					position;
	int					length;
	void *				buffer;
} fileDownload_t;

typedef struct backgroundDownload_s {
	struct backgroundDownload_s	*next;	// set by the fileSystem
	dlType_t			opcode;
	idFile *			f;
	fileDownload_t		file;
	urlDownload_t		url;
	volatile bool		completed;
} backgroundDownload_t;

// forward declaration (provided via idlib/Lib.h, but declared here in case
// FileSystem.h is included standalone - needed by ReadCodePakLists)
class idBitMsg;

// file list for directory listings
class idFileList {
	friend class idFileSystemLocal;
public:
	const char *			GetBasePath( void ) const { return basePath; }
	int						GetNumFiles( void ) const { return list.Num(); }
	const char *			GetFile( int index ) const { return list[index]; }
	const idStrList &		GetList( void ) const { return list; }

private:
	idStr					basePath;
	idStrList				list;
};

// mod list
class idModList {
	friend class idFileSystemLocal;
public:
	int						GetNumMods( void ) const { return mods.Num(); }
	const char *			GetMod( int index ) const { return mods[index]; }
	const char *			GetDescription( int index ) const { return descriptions[index]; }

private:
	idStrList				mods;
	idStrList				descriptions;
};

// ============================================================================
//
// idFileSystem - PORTED to the Quake 4 1.4.2 retail SDK (v37) vtable layout.
//
// The virtual method order, count, and signatures below mirror the Q4 1.4.2 SDK
// (quake4-sdk/source/framework/FileSystem.h, class idFileSystem) EXACTLY, so the
// retail gamex86.dll resolves every fileSystem-> call to the correct slot.
//
// Retail build defines honored when deciding which methods occupy a slot:
//   RV_UNIFIED_ALLOCATOR=ON, _RV_MEM_SYS_SUPPORT=OFF, RV_SINGLE_DECL_FILE=ON,
//   RV_BINARYDECLS=OFF, Q4SDK=ON, Q4SDK_MD5R=ON, _USE_OPENAL=ON, _XENON=OFF.
// The only conditional inside the SDK class is the _XENON block
// (AddDownloadedPak/RemoveDownloadedPak/AddExplicitPak/RemoveExplicitPak/
// IsPakLoaded); _XENON=OFF so those 5 methods are NOT present.
//
// Total: 71 entries (slot 0 = destructor + 70 named virtuals).
//
// NOTE on signature fidelity vs. surviving DOOM 3 engine callers:
//  - ReadFile keeps ID_TIME_T* timestamp. With _USE_32BIT_TIME_T (set in
//    _Common.props) ID_TIME_T == 32-bit time_t == same size/ABI as the SDK's
//    'unsigned *', so the slot is binary-compatible.
//  - SetPureServerChecksums / GetPureServerChecksums keep the DOOM 3 scalar
//    gamePakChecksum signature (the SDK uses int[MAX_GAMEPAK_PER_OS] + an extra
//    out param). These are server pure-negotiation methods, never invoked by the
//    Q4 DLL on the boot/menu path, and 6 internal async network callers rely on
//    the scalar form. The vtable SLOT POSITION is unchanged, which is what the
//    ABI requires here.
// ============================================================================
class idFileSystem {
public:
	virtual					~idFileSystem() {}
							// Initializes the file system.
	virtual void			Init( void ) = 0;
							// Restarts the file system.
	virtual void			Restart( void ) = 0;
							// Shutdown the file system.
	virtual void			Shutdown( bool reloading ) = 0;
							// Returns true if the file system is initialized.
	virtual bool			IsInitialized( void ) const = 0;
							// Returns true if we are doing an fs_copyfiles.
	virtual bool			PerformingCopyFiles( void ) const = 0;
							// Returns a list of mods found along with descriptions
	virtual idModList *		ListMods( void ) = 0;
							// Frees the given mod list
	virtual void			FreeModList( idModList *modList ) = 0;
							// Lists files with the given extension in the given directory.
	virtual idFileList *	ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL ) = 0;
							// Lists files in the given directory and all subdirectories with the given extension.
	virtual idFileList *	ListFilesTree( const char *relativePath, const char *extension, bool sort = false, const char* gamedir = NULL ) = 0;
							// Frees the given file list.
	virtual void			FreeFileList( idFileList *fileList ) = 0;
							// Converts a full OS path to a relative path.
	virtual const char *	OSPathToRelativePath( const char *OSPath ) = 0;
							// Converts a relative path to a full OS path.
	virtual const char *	RelativePathToOSPath( const char *relativePath, const char *basePath = "fs_devpath" ) = 0;
							// Builds a full OS path from the given components.
	virtual const char *	BuildOSPath( const char *base, const char *game, const char *relativePath ) = 0;
							// Creates the given OS path for as far as it doesn't exist already.
	virtual void			CreateOSPath( const char *OSPath ) = 0;
							// Returns true if a file is in a pak file.
	virtual bool			FileIsInPAK( const char *relativePath ) = 0;
							// Returns a space separated string containing the checksums of all referenced pak files.
	virtual void			UpdatePureServerChecksums( void ) = 0;
							// setup the mapping of OS -> game pak checksum
	virtual bool			UpdateGamePakChecksums( void ) = 0;
							// configure pure mode (server pure-negotiation; DOOM 3 scalar gamePakChecksum signature kept)
	virtual fsPureReply_t	SetPureServerChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum, int missingChecksums[ MAX_PURE_PAKS ], int *missingGamePakChecksum ) = 0;
							// fills a 0-terminated list of pak checksums for a client
	virtual void			GetPureServerChecksums( int checksums[ MAX_PURE_PAKS ], int OS, int *gamePakChecksum ) = 0;
							// before doing a restart, force the pure list and the search order
	virtual void			SetRestartChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum ) = 0;
							// equivalent to calling SetPureServerChecksums with an empty list
	virtual	void			ClearPureChecksums( void ) = 0;
							// get a mask of supported OSes. if not pure, returns -1
	virtual unsigned int	GetOSMask( void ) = 0;
							// Reads a complete file. Returns the length, or -1 on failure.
	virtual int				ReadFile( const char *relativePath, void **buffer, ID_TIME_T *timestamp = NULL ) = 0;
							// Frees the memory allocated by ReadFile.
	virtual void			FreeFile( void *buffer ) = 0;
							// Writes a complete file, will create any needed subdirectories.
	virtual int				WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_savepath" ) = 0;
							// Removes the given file.
	virtual void			RemoveFile( const char *relativePath, const char *basePath = "fs_savepath" ) = 0;
							// Removes the given file and returns the filesystem status of the removal
	virtual int				RemoveExplicitFile( const char *OSPath ) = 0;
							// is file loading allowed?
	virtual void			SetIsFileLoadingAllowed( bool mode ) = 0;
							// returns file loading status
	virtual bool			GetIsFileLoadingAllowed( void ) const = 0;
							// set the current asset log name.
	virtual void			SetAssetLogName( const char *logName ) = 0;
							// write out a list of all files loaded.
	virtual void			WriteAssetLog( void ) = 0;
							// clear list of all files loaded.
	virtual void			ClearAssetLog( void ) = 0;
							// Accessor for asset log name (with filter)
	virtual const char*		GetAssetLogName( void ) = 0;
							// new file allocators for tools
	virtual idFile *		GetNewFileMemory( void ) = 0;
	virtual idFile *		GetNewFilePermanent( void ) = 0;
							// Opens a file for reading.
	virtual idFile *		OpenFileRead( const char *relativePath, bool allowCopyFiles = true, const char* gamedir = NULL ) = 0;
							// Opens a file for writing, will create any needed subdirectories.
	virtual idFile *		OpenFileWrite( const char *relativePath, const char *basePath = "fs_savepath", bool ASCII = false ) = 0;
							// Opens a file for writing at the end.
	virtual idFile *		OpenFileAppend( const char *filename, bool sync = false, const char *basePath = "fs_basepath" ) = 0;
							// Opens a file for reading, writing, or appending depending on the value of mode.
	virtual idFile *		OpenFileByMode( const char *relativePath, fsMode_t mode ) = 0;
							// Opens a file for reading from a full OS path.
	virtual idFile *		OpenExplicitFileRead( const char *OSPath ) = 0;
							// Opens a file for writing to a full OS path.
	virtual idFile *		OpenExplicitFileWrite( const char *OSPath ) = 0;
							// Closes a file.
	virtual void			CloseFile( idFile *f ) = 0;
							// Returns immediately, performing the read from a background thread.
	virtual void			BackgroundDownload( backgroundDownload_t *bgl ) = 0;
							// resets the bytes read counter
	virtual void			ResetReadCount( void ) = 0;
							// retrieves the current read count
	virtual int				GetReadCount( void ) = 0;
							// adds to the read count
	virtual void			AddToReadCount( int c ) = 0;
							// look for a dynamic module
	virtual void			FindDLL( const char *basename, char dllPath[ MAX_OSPATH ], bool updateChecksum ) = 0;
							// case sensitive filesystems use an internal directory cache
	virtual void			ClearDirCache( void ) = 0;
							// lookup a relative path, return the size or 0 if not found
	virtual int				RelativeDownloadPathForChecksum( int checksum, char path[ MAX_STRING_CHARS ] ) = 0;
							// verify the file can be downloaded, lookup a relative path, return the size or 0 if not found
	virtual int				ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isGamePak ) = 0;
							// verify the file can be downloaded, lookup an absolute (OS) path, return the size or 0 if not found
	virtual int				ValidateDownloadPakForRelativePath( const char *relativePath, char path[ MAX_STRING_CHARS ], bool &isGamePakReturn ) = 0;

	virtual idFile *		MakeTemporaryFile( void ) = 0;

							// make downloaded pak files known so pure negociation works next time
	virtual int				AddZipFile( const char *path ) = 0;

							// look for a file in the loaded paks or the addon paks
	virtual findFile_t		FindFile( const char *path ) = 0;

							// get map/addon decls and take into account addon paks that are not on the search list
	virtual int				GetNumMaps() = 0;
	virtual int				GetMapDeclIndex( const char *mapName ) = 0;
	virtual const idDict *	GetMapDecl( int i ) = 0;
	virtual const idDict *	GetMapDecl( const char *mapName ) = 0;
	virtual void			FindMapScreenshot( const char *path, char *buf, int len ) = 0;

							// Converts a full OS path to an import path.
	virtual bool			OSpathToImportPath( const char *osPath, idStr &iPath, bool stripTemp = false ) = 0;
							// Opens a file for reading from the fs_importpath directory
	virtual idFile *		OpenImportFileRead( const char *filename ) = 0;
							// Copy a file
	virtual void			CopyOSFile( const char *fromOSPath, const char *toOSPath ) = 0;
	virtual void			CopyOSFile( idFile *src, const char *toOSPath ) = 0;

							// demo functions - only for use by the core
	virtual void			WriteDemoHeader( idFile *file ) = 0;
	virtual int				ReadDemoHeader( idFile *file ) = 0;

							// indicates if the filesystem is currently running with pak files restrictions or addons
	virtual bool			IsRunningWithRestrictions( void ) = 0;

							// new in 1.4
	virtual void			ReadCodePakLists( const idBitMsg &msg ) = 0;
	virtual bool			HaveCodePakLists( void ) const = 0;

							// pick best language - used by the core to pick a good default language based on present zpaks
	virtual void			SelectDefaultLanguage( void ) = 0;

	virtual void			ClearAddonList( void ) = 0;

	// ------------------------------------------------------------------------
	// DOOM 3 legacy helpers that are NOT part of the Quake 4 vtable.
	// Declared NON-virtual on the base so they add no vtable slot, yet remain
	// callable through the engine's idFileSystem* (used by async networking and
	// the session menu). Quake 4 has no Doom 3 expansion content, so both
	// return false. (No corresponding override exists in idFileSystemLocal.)
	// ------------------------------------------------------------------------
	bool					HasD3XP( void ) { return false; }
	bool					RunningD3XP( void ) { return false; }
};

extern idFileSystem *		fileSystem;

#endif /* !__FILESYSTEM_H__ */
