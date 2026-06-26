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

#ifndef __DRAWVERT_H__
#define __DRAWVERT_H__

/*
===============================================================================

	Draw Vertex.

===============================================================================
*/

// RAVEN/Q4: the retail game DLL passes idDrawVert across the engine<->game boundary
// (patch control points, model geometry). Quake 4 1.4.2 REORDERED the fields and added
// a second color, growing the struct from D3's 60 bytes to 64. Match the v37 layout
// EXACTLY -- otherwise idList<idDrawVert> strides wrong and st/normal/color read at the
// wrong offsets (this was the slow/garbage collision build for patch primitives).
//   layout: xyz(0) color(12) normal(16) color2(28) tangents[2](32) st(56)  size 64
class idDrawVert {
public:
	idVec3			xyz;
	byte			color[4];
	idVec3			normal;
	byte			color2[4];
	idVec3			tangents[2];
	idVec2			st;

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

	const idVec3 &	GetNormal( void ) const;
	void			SetNormal( float x, float y, float z );
	void			SetNormal( const idVec3 &n );

	const idVec3 &	GetTangent( void ) const;
	void			SetTangent( float x, float y, float z );
	void			SetTangent( const idVec3 &t );

	const idVec3 & 	GetBiTangent( void ) const;
	void			SetBiTangent( float x, float y, float z );
	void			SetBiTangent( const idVec3 &t );
	void			SetBiTangentSign( float sign );

	void			Lerp( const idDrawVert &a, const idDrawVert &b, const float f );
	void			LerpAll( const idDrawVert &a, const idDrawVert &b, const float f );

	void			Normalize( void );

	void			SetColor( dword color );
	dword			GetColor( void ) const;
};

ID_INLINE float idDrawVert::operator[]( const int index ) const {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}
ID_INLINE float	&idDrawVert::operator[]( const int index ) {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}

ID_INLINE void idDrawVert::Clear( void ) {
	xyz.Zero();
	st.Zero();
	normal.Zero();
	tangents[0].Zero();
	tangents[1].Zero();
	color[0] = color[1] = color[2] = color[3] = 0;
}

ID_INLINE const idVec3 &idDrawVert::GetNormal( void ) const { return normal; }
ID_INLINE void idDrawVert::SetNormal( const idVec3 &n ) { normal = n; }
ID_INLINE void idDrawVert::SetNormal( float x, float y, float z ) { normal.Set( x, y, z ); }
ID_INLINE const idVec3 &idDrawVert::GetTangent( void ) const { return tangents[0]; }
ID_INLINE void idDrawVert::SetTangent( float x, float y, float z ) { tangents[0].Set( x, y, z ); }
ID_INLINE void idDrawVert::SetTangent( const idVec3 &t ) { tangents[0] = t; }
ID_INLINE const idVec3 &idDrawVert::GetBiTangent( void ) const { return tangents[1]; }
ID_INLINE void idDrawVert::SetBiTangent( float x, float y, float z ) { tangents[1].Set( x, y, z ); }
ID_INLINE void idDrawVert::SetBiTangent( const idVec3 &t ) { tangents[1] = t; }
ID_INLINE void idDrawVert::SetBiTangentSign( float sign ) { }

ID_INLINE void idDrawVert::Lerp( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );
}

ID_INLINE void idDrawVert::LerpAll( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );
	normal = a.normal + f * ( b.normal - a.normal );
	tangents[0] = a.tangents[0] + f * ( b.tangents[0] - a.tangents[0] );
	tangents[1] = a.tangents[1] + f * ( b.tangents[1] - a.tangents[1] );
	color[0] = (byte)( a.color[0] + f * ( b.color[0] - a.color[0] ) );
	color[1] = (byte)( a.color[1] + f * ( b.color[1] - a.color[1] ) );
	color[2] = (byte)( a.color[2] + f * ( b.color[2] - a.color[2] ) );
	color[3] = (byte)( a.color[3] + f * ( b.color[3] - a.color[3] ) );
}

ID_INLINE void idDrawVert::SetColor( dword color ) {
	*reinterpret_cast<dword *>(this->color) = color;
}

ID_INLINE dword idDrawVert::GetColor( void ) const {
	return *reinterpret_cast<const dword *>(this->color);
}

#endif /* !__DRAWVERT_H__ */
