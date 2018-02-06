// *************************************************************************************************
//
// Horde3D
//   Next-Generation Graphics Engine
// --------------------------------------
// Copyright (C) 2006-2016 Nicolas Schulz and Horde3D team
//
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/legal/epl-v10.html
//
// *************************************************************************************************

#define _CRT_SECURE_NO_DEPRECATE
#include "Horde3D.h"
#include "utPlatform.h"
#include "utEndian.h"
#include "utMath.h"
#include <math.h>
#ifdef PLATFORM_WIN
#	define WIN32_LEAN_AND_MEAN 1
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#endif
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>

using namespace Horde3D;
using namespace std;

#ifdef __MINGW32__
#undef PLATFORM_WIN
#endif


namespace Horde3DUtils {

struct InfoBox
{
	H3DRes  fontMatRes;
	float   x, y_row0;
	float   width;
	int     row;
} infoBox;

ofstream            outf;
map< int, string >  resourcePaths;


string cleanPath( string path )
{
	// Remove spaces at the beginning
	int cnt = 0;
	for( int i = 0; i < (int)path.length(); ++i )
	{
		if( path[i] != ' ' ) break;
		else ++cnt;
	}
	if( cnt > 0 ) path.erase( 0, cnt );

	// Remove slashes, backslashes and spaces at the end
	cnt = 0;
	for( int i = (int)path.length() - 1; i >= 0; --i )
	{
		if( path[i] != '/' && path[i] != '\\' && path[i] != ' ' ) break;
		else ++cnt;
	}

	if( cnt > 0 ) path.erase( path.length() - cnt, cnt );

	return path;
}

}  // namespace


// =================================================================================================
// Exported API functions
// =================================================================================================

using namespace Horde3DUtils;


DLLEXP const char *h3dutGetResourcePath( int type )
{
	return resourcePaths[type].c_str();
}


DLLEXP void h3dutSetResourcePath( int type, const char *path )
{
	string s = path != 0x0 ? path : "";

	resourcePaths[type] = cleanPath( s );
}


DLLEXP bool h3dutLoadResourcesFromDisk( const char *contentDir )
{
	bool result = true;
	string dir;
	vector< string > dirs;

	// Split path string
	char *c = (char *)contentDir;
	do
	{
		if( *c != '|' && *c != '\0' )
			dir += *c;
		else
		{
			dir = cleanPath( dir );
			if( dir != "" ) dir += '/';
			dirs.push_back( dir );
			dir = "";
		}
	} while( *c++ != '\0' );
	
	// Get the first resource that needs to be loaded
	int res = h3dQueryUnloadedResource( 0 );
	
	char *dataBuf = 0;
	size_t bufSize = 0;

	while( res != 0 )
	{
		ifstream inf;
		
		// Loop over search paths and try to open files
		for( unsigned int i = 0; i < dirs.size(); ++i )
		{
			string fileName = dirs[i] + resourcePaths[h3dGetResType( res )] + "/" + h3dGetResName( res );
			inf.clear();
			inf.open( fileName.c_str(), ios::binary );
			if( inf.good() ) break;
		}

		// Open resource file
		if( inf.good() ) // Resource file found
		{
			// Find size of resource file
			inf.seekg( 0, ios::end );
			size_t fileSize = (size_t) inf.tellg();
			if( bufSize < fileSize  )
			{
				delete[] dataBuf;				
				dataBuf = new char[fileSize];
				if( !dataBuf )
				{
					bufSize = 0;
					continue;
				}
				bufSize = fileSize;
			}
			if( fileSize == 0 )	continue;
			// Copy resource file to memory
			inf.seekg( 0 );
			inf.read( dataBuf, fileSize );
			inf.close();
			// Send resource data to engine
			result &= h3dLoadResource( res, dataBuf, ( int ) fileSize );
		}
		else // Resource file not found
		{
			// Tell engine to use the dafault resource by using NULL as data pointer
			h3dLoadResource( res, 0x0, 0 );
			result = false;
		}
		// Get next unloaded resource
		res = h3dQueryUnloadedResource( 0 );
	}
	delete[] dataBuf;

	return result;
}


DLLEXP bool h3dutDumpMessages()
{
	if( !outf.is_open() )
	{
		// Reset log file
		outf.setf( ios::fixed );
		outf.precision( 3 );
		outf.open( "Horde3D_Log.html", ios::out );
		if( !outf ) return false;

		outf << "<html>\n";
		outf << "<head>\n";
		outf << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n";
		outf << "<title>Horde3D Log</title>\n";
		outf << "<style type=\"text/css\">\n";
		
		outf << "body, html {\n";
		outf << "background: #000000;\n";
		outf << "width: 1000px;\n";
		outf << "font-family: Arial;\n";
		outf << "font-size: 16px;\n";
		outf << "color: #C0C0C0;\n";
		outf << "}\n";

		outf << "h1 {\n";
		outf << "color : #FFFFFF;\n";
		outf << "border-bottom : 1px dotted #888888;\n";
		outf << "}\n";

		outf << "pre {\n";
		outf << "font-family : arial;\n";
		outf << "margin : 0;\n";
		outf << "}\n";

		outf << ".box {\n";
		outf << "border : 1px dotted #818286;\n";
		outf << "padding : 5px;\n";
		outf << "margin: 5px;\n";
		outf << "width: 950px;\n";
		outf << "background-color : #292929;\n";
		outf << "}\n";

		outf << ".err {\n";
		outf << "color: #EE1100;\n";
		outf << "font-weight: bold\n";
		outf << "}\n";

		outf << ".warn {\n";
		outf << "color: #FFCC00;\n";
		outf << "font-weight: bold\n";
		outf << "}\n";

		outf << ".info {\n";
		outf << "color: #C0C0C0;\n";
		outf << "}\n";

		outf << ".debug {\n";
		outf << "color: #CCA0A0;\n";
		outf << "}\n";

		outf << "</style>\n";
		outf << "</head>\n\n";

		outf << "<body>\n";
		outf << "<h1>Horde3D Log</h1>\n";
		outf << "<h3>" << h3dGetVersionString() << "</h3>\n";
		outf << "<div class=\"box\">\n";
		outf << "<table>\n";

		outf.flush();
	}

	int level;
	float time;
	string text = h3dGetMessage( &level, &time );
	
	while( text != "" )
	{
		outf << "<tr>\n";
		outf << "<td width=\"100\">";
		outf << time;
		outf << "</td>\n";
		outf << "<td class=\"";
		
		switch( level )
		{
		case 1:
			outf << "err";
			break;
		case 2:
			outf << "warn";
			break;
		case 3:
			outf << "info";
			break;
		default:
			outf << "debug";
		}
		
		outf << "\"><pre>\n";
		outf << text.c_str();
		outf << "\n</pre></td>\n";
		outf << "</tr>\n";

		outf.flush();
		
		text = h3dGetMessage( &level, &time );
	}
	
	return true;
}


DLLEXP void h3dutShowText( const char *text, float x, float y, float size, float colR,
                           float colG, float colB, H3DRes fontMaterialRes )
{
	if( text == 0x0 || *text == '\0' ) return;
	
	float ovFontVerts[64 * 16];
	float *p = ovFontVerts;
	float pos = 0;
	
	do
	{
		unsigned char ch = (unsigned char)*text++;

		float u0 = 0.0625f * (ch % 16);
		float v0 = 1.0f - 0.0625f * (ch / 16);

		*p++ = x + size * 0.5f * pos;         *p++ = y;         *p++ = u0;            *p++ = v0;
		*p++ = x + size * 0.5f * pos,         *p++ = y + size;  *p++ = u0;            *p++ = v0 - 0.0625f;
		*p++ = x + size * 0.5f * pos + size;  *p++ = y + size;  *p++ = u0 + 0.0625f;  *p++ = v0 - 0.0625f;
		*p++ = x + size * 0.5f * pos + size;  *p++ = y;         *p++ = u0 + 0.0625f;  *p++ = v0;
		
		pos += 1.f;
	} while( *text && pos < 64 );

	h3dShowOverlays( ovFontVerts, (int)pos * 4, colR, colG, colB, 1.f, fontMaterialRes, 0 );
}


void beginInfoBox( float x, float y, float width, int numRows, const char *title,
                   H3DRes fontMaterialRes, H3DRes boxMaterialRes )
{
	float fontSize = 0.03f;
	float barHeight = fontSize + 0.01f;
	float bodyHeight = numRows * 0.035f + 0.005f;
	
	infoBox.fontMatRes = fontMaterialRes;
	infoBox.x = x;
	infoBox.y_row0 = y + barHeight + 0.005f;
	infoBox.width = width;
	infoBox.row = 0;
	
	// Title bar
	float ovTitleVerts[] = { x, y, 0, 1, x, y + barHeight, 0, 0,
	                         x + width, y + barHeight, 1, 0, x + width, y, 1, 1 };
    h3dShowOverlays( ovTitleVerts, 4,  0.15f, 0.23f, 0.31f, 0.8f, boxMaterialRes, 0 );

	// Title text
	h3dutShowText( title, x + 0.005f, y + 0.005f, fontSize, 0.7f, 0.85f, 0.95f, fontMaterialRes );

	// Body
	float yy = y + barHeight;
	float ovBodyVerts[] = { x, yy, 0, 1, x, yy + bodyHeight, 0, 0,
	                        x + width, yy + bodyHeight, 1, 0, x + width, yy, 1, 1 };
	h3dShowOverlays( ovBodyVerts, 4, 0.12f, 0.12f, 0.12f, 0.5f, boxMaterialRes, 0 );
}


void addInfoBoxRow( const char *column1, const char *column2 )
{
	float fontSize = 0.028f;
	float fontWidth = fontSize * 0.5f;
	float x = infoBox.x;
	float y = infoBox.y_row0 + infoBox.row++ * 0.035f;

	// First column
    if( column1 )
    {
        h3dutShowText( column1, x + 0.005f, y, fontSize, 1, 1, 1, infoBox.fontMatRes );
    }

	// Second column
    if( column2 )
    {
        x = infoBox.x + infoBox.width - ((strlen( column2 ) - 1) * fontWidth + fontSize);
        h3dutShowText( column2, x - 0.005f, y, fontSize, 1, 1, 1, infoBox.fontMatRes );
    }
}


DLLEXP void h3dutShowInfoBox( float x, float y, float width, const char *title,
                              int numRows, const char **column1, const char **column2,
                              H3DRes fontMaterialRes, H3DRes panelMaterialRes )
{
    beginInfoBox( x, y, width, numRows, title, fontMaterialRes, panelMaterialRes );
    for( int i=0; i<numRows; ++i )
        addInfoBoxRow( column1 ? column1[i] : 0, column2 ? column2[i] : 0 );
}


DLLEXP void h3dutShowFrameStats( H3DRes fontMaterialRes, H3DRes panelMaterialRes, int mode )
{
	static stringstream text;
	static float curFPS = 30;
	static float timer = 100;
	static float fps = 30;
	static float frameTime = 0;
	static float animTime = 0;
	static float geoUpdateTime = 0;
	static float particleSimTime = 0;
	static float fwdLightsGPUTime = 0;
	static float defLightsGPUTime = 0;
	static float shadowsGPUTime = 0;
	static float particleGPUTime = 0;
	static float computeGPUTime = 0;

	// Calculate FPS
	float curFrameTime = h3dGetStat( H3DStats::FrameTime, true );
	curFPS = 1000.0f / curFrameTime;
	
	timer += curFrameTime / 1000.0f;
	if( timer > 0.7f )
	{	
		fps = curFPS;
		frameTime = curFrameTime;
		animTime = h3dGetStat( H3DStats::AnimationTime, true );
		geoUpdateTime = h3dGetStat( H3DStats::GeoUpdateTime, true );
		particleSimTime = h3dGetStat( H3DStats::ParticleSimTime, true );
		fwdLightsGPUTime = h3dGetStat( H3DStats::FwdLightsGPUTime, true );
		defLightsGPUTime = h3dGetStat( H3DStats::DefLightsGPUTime, true );
		shadowsGPUTime = h3dGetStat( H3DStats::ShadowsGPUTime, true );
		particleGPUTime = h3dGetStat( H3DStats::ParticleGPUTime, true );
		computeGPUTime = h3dGetStat( H3DStats::ComputeGPUTime, true );
		timer = 0;
	}
	else
	{
		// Reset accumulative counters
		h3dGetStat( H3DStats::AnimationTime, true );
		h3dGetStat( H3DStats::GeoUpdateTime, true );
		h3dGetStat( H3DStats::ParticleSimTime, true );
	}
	
	if( mode > 0 )
	{
		// InfoBox
		beginInfoBox( 0.03f, 0.03f, 0.32f, 4, "Frame Stats", fontMaterialRes, panelMaterialRes );
		
		// FPS
		text.str( "" );
		text << fixed << setprecision( 2 ) << fps;
		addInfoBoxRow( "FPS", text.str().c_str() );
		
		// Triangle count
		text.str( "" );
		text << (int)h3dGetStat( H3DStats::TriCount, true );
		addInfoBoxRow( "Tris", text.str().c_str() );
		
		// Number of batches
		text.str( "" );
		text << (int)h3dGetStat( H3DStats::BatchCount, true );
		addInfoBoxRow( "Batches", text.str().c_str() );
		
		// Number of lighting passes
		text.str( "" );
		text << (int)h3dGetStat( H3DStats::LightPassCount, true );
		addInfoBoxRow( "Lights", text.str().c_str() );
	}

	if( mode > 1 )
	{
		// Video memory
		beginInfoBox( 0.03f, 0.30f, 0.32f, 2, "VMem", fontMaterialRes, panelMaterialRes );
		
		// Textures
		text.str( "" );
		text << h3dGetStat( H3DStats::TextureVMem, false ) << "mb";
		addInfoBoxRow( "Textures", text.str().c_str() );
		
		// Geometry
		text.str( "" );
		text << h3dGetStat( H3DStats::GeometryVMem, false ) << "mb";
		addInfoBoxRow( "Geometry", text.str().c_str() );
		
		// CPU time
		beginInfoBox( 0.03f, 0.44f, 0.32f, 4, "CPU Time", fontMaterialRes, panelMaterialRes );
		
		// Frame time
		text.str( "" );
		text << frameTime << "ms";
		addInfoBoxRow( "Frame Total", text.str().c_str() );
		
		// Animation
		text.str( "" );
		text << animTime << "ms";
		addInfoBoxRow( "Animation", text.str().c_str() );

		// Geometry updates
		text.str( "" );
		text << geoUpdateTime << "ms";
		addInfoBoxRow( "Geo Updates", text.str().c_str() );

		// Particle simulation
		text.str( "" );
		text << particleSimTime << "ms";
		addInfoBoxRow( "Particles", text.str().c_str() );

		// GPU time
        beginInfoBox( 0.03f, 0.65f, 0.32f, 4, "GPU Time", fontMaterialRes, panelMaterialRes );

		// Forward and deferred lights
		text.str( "" );
		text << (fwdLightsGPUTime + defLightsGPUTime) << "ms";
		addInfoBoxRow( "Lights", text.str().c_str() );
		
		// Shadows
		text.str( "" );
		text << shadowsGPUTime << "ms";
		addInfoBoxRow( "Shadows", text.str().c_str() );

		// Particles
		text.str( "" );
		text << particleGPUTime << "ms";
		addInfoBoxRow( "Particles", text.str().c_str() );

		// Compute
		text.str( "" );
		text << computeGPUTime << "ms";
		addInfoBoxRow( "Compute", text.str().c_str() );
	}
}


DLLEXP void h3dutFreeMem( char **ptr )
{
	if( ptr == 0x0 ) return;
	
	delete[] *ptr; *ptr = 0x0;
}

DLLEXP H3DRes h3dutCreateGeometryRes( 
	const char *name, 
	int numVertices, int numTriangleIndices,
	float *posData, 
	uint32 *indexData, 
	short *normalData,
	short *tangentData,
	short *bitangentData,
	float *texData1, float *texData2 )
{
	if( numVertices == 0 || numTriangleIndices == 0 ) return 0;

	H3DRes res = h3dAddResource( H3DResTypes::Geometry, name, 0 );
				
	uint32 size = 
		// General data
		4 +					// Horde Flag 
		sizeof( uint32 ) +	// Version 
		sizeof( uint32 ) +	// Joint Count ( will be always set to set to zero because this method does not create joints )
		sizeof( uint32 ) +	// number of streams 
		sizeof( uint32 ) +	// streamsize
		// vertex stream data
		sizeof( uint32 ) +	// stream id 
		sizeof( uint32 ) +	// stream element size 
		numVertices * sizeof( float ) * 3 + // vertices data
	
		// morph targets 
		sizeof( uint32 );	// number of morph targets ( will be always set to zero because this method does not create morph targets )

	if( normalData )
	{
		size += 
			// normal stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
			numVertices * sizeof( uint16 ) * 3; // normal data
	}

	if( tangentData && bitangentData )
	{
		size += 
			// normal stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
			numVertices * sizeof( uint16 ) * 3 + // tangent data
			// normal stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
			numVertices * sizeof( uint16 ) * 3; // bitangent data
	}

	int numTexSets = 0;
	if( texData1 ) ++numTexSets;
	if( texData2 ) ++numTexSets;

	for( int i = 0; i < numTexSets; ++i )
	{
		size += 
			// texture stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
		numVertices * sizeof( float ) * 2; // texture data
	}

	size += 
		// index stream data
		sizeof( uint32 ) +	// index count 
		numTriangleIndices * sizeof( uint32 ); // index data


	// Create resource data block
	char *data = new char[size];

	char *pData = data;
	// Write Horde flag
    pData = elemcpyd_le((char*)(pData), "H3DG", 4);
	// Set version to 5 
	pData = elemset_le((uint32*)(pData), 5u);
	// Set joint count (zero for this method)
	pData = elemset_le((uint32*)(pData), 0u);
	// Set number of streams
	pData = elemset_le((uint32*)(pData), 1u + numTexSets + ( normalData ? 1 : 0 ) + ((tangentData && bitangentData) ? 2 : 0));
	// Write number of elements in each stream
	pData = elemset_le((int32*)(pData), numVertices);

	// Beginning of stream data

	// Vertex Stream ID
	pData = elemset_le((uint32*)(pData), 0u);
	// set vertex stream element size
	pData = elemset_le<uint32>((uint32*)(pData), sizeof( float ) * 3);
	// vertex data
    pData = elemcpyd_le((float*)(pData), posData, numVertices * 3);

	if( normalData )
	{
		// Normals Stream ID
		pData = elemset_le((uint32*)(pData), 1u);
		// set normal stream element size
		pData = elemset_le<uint32>((uint32*)(pData), sizeof( short ) * 3);
		// normal data
        pData = elemcpyd_le((short*)(pData), normalData, numVertices * 3);
	}

	if( tangentData && bitangentData )
	{
		// Tangent Stream ID
		pData = elemset_le((uint32*)(pData), 2u);
		// set tangent stream element size
		pData = elemset_le<uint32>((uint32*)(pData), sizeof( short ) * 3);
		// tangent data
		pData = elemcpyd_le((short*)(pData), tangentData, numVertices * 3);
	
		// Bitangent Stream ID
		pData = elemset_le((uint32*)(pData), 3u);
		// set bitangent stream element size
		pData = elemset_le<uint32>((uint32*)(pData), sizeof( short ) * 3);
		// bitangent data
		pData = elemcpyd_le((short*)(pData), bitangentData, numVertices * 3);
	}

	// texture coordinates stream
	if( texData1 )
	{
		pData = elemset_le((uint32*)(pData), 6u); // Tex Set 1
		pData = elemset_le<uint32>((uint32*)(pData), sizeof( float ) * 2); // stream element size
		pData = elemcpyd_le((float *)(pData), texData1, 2 * numVertices ); // stream data
	}
	if( texData2 )
	{
		pData = elemset_le((uint32*)(pData), 7u); // Tex Set 2
		pData = elemset_le<uint32>((uint32*)(pData), sizeof( float ) * 2); // stream element size
		pData = elemcpyd_le((float *)(pData), texData2, 2 * numVertices ); // stream data
	}

	// Set number of indices
	pData = elemset_le((int32*)(pData), numTriangleIndices);	
	
	// index data
	pData = elemcpyd_le((uint32*)(pData), indexData, numTriangleIndices);			

	// Set number of morph targets to zero
	pData = elemset_le((uint32*)(pData), 0u);

	if ( res ) h3dLoadResource( res, data, size );
	delete[] data;

	return res;
}

DLLEXP bool h3dutCreateTGAImage( const unsigned char *pixels, int width, int height, int bpp,
                                 char **outData, int *outSize )
{
	if( outData == 0x0 || outSize == 0x0 ) return false;
	
	*outData = 0x0; *outSize = 0;
	
	if( bpp != 24 && bpp != 32 ) return false;

	*outSize = width * height * (bpp / 8) + 18;
	char *data = new char[*outSize];
	*outData = data;

	// Build TGA header
	char c;
	short s;
	c = 0;      data = elemcpyd_le( data, &c, 1 );              // idLength
	c = 0;      data = elemcpyd_le( data, &c, 1 );              // colmapType
	c = 2;      data = elemcpyd_le( data, &c, 1 );              // imageType
	s = 0;      data = elemcpyd_le( (short*) data, &s, 1 );     // colmapStart
	s = 0;      data = elemcpyd_le( (short*) data, &s, 1 );     // colmapLength
	c = 0;      data = elemcpyd_le( data, &c, 1 );              // colmapBits
	s = 0;      data = elemcpyd_le( (short*) data, &s, 1 );     // x
	s = 0;      data = elemcpyd_le( (short*) data, &s, 1 );     // y
	s = width;  data = elemcpyd_le( (short*) data, &s, 1 );     // width
	s = height; data = elemcpyd_le( (short*) data, &s, 1 );     // height
	c = bpp;    data = elemcpyd_le( data, &c, 1 );              // bpp
	c = 0;      data = elemcpyd_le( data, &c, 1 );              // imageDesc

	// Copy data
    if( pixels ) memcpy( data, pixels, width * height * (bpp / 8) );

	return true;
}


DLLEXP void h3dutPickRay( H3DNode cameraNode, float nwx, float nwy, float *ox, float *oy, float *oz,
                          float *dx, float *dy, float *dz )
{				
	// Transform from normalized window [0, 1] to normalized device coordinates [-1, 1]
	float cx( 2.0f * nwx - 1.0f );
	float cy( 2.0f * nwy - 1.0f );   
	
	// Get projection matrix
	Matrix4f projMat;
	h3dGetCameraProjMat( cameraNode, projMat.x );
	
	// Get camera view matrix
	const float *camTrans;
	h3dGetNodeTransMats( cameraNode, 0x0, &camTrans );		
	Matrix4f viewMat( camTrans );
	viewMat = viewMat.inverted();
	
	// Create inverse view-projection matrix for unprojection
	Matrix4f invViewProjMat = (projMat * viewMat).inverted();

	// Unproject
	Vec4f p0 = invViewProjMat * Vec4f( cx, cy, -1, 1 );
	Vec4f p1 = invViewProjMat * Vec4f( cx, cy, 1, 1 );
	p0.x /= p0.w; p0.y /= p0.w; p0.z /= p0.w;
	p1.x /= p1.w; p1.y /= p1.w; p1.z /= p1.w;
	
	if( h3dGetNodeParamI( cameraNode, H3DCamera::OrthoI ) == 1 )
	{
		float frustumWidth = h3dGetNodeParamF( cameraNode, H3DCamera::RightPlaneF, 0 ) -
		                     h3dGetNodeParamF( cameraNode, H3DCamera::LeftPlaneF, 0 );
		float frustumHeight = h3dGetNodeParamF( cameraNode, H3DCamera::TopPlaneF, 0 ) -
		                      h3dGetNodeParamF( cameraNode, H3DCamera::BottomPlaneF, 0 );
		
		Vec4f p2( cx, cy, 0, 1 );

		p2.x = cx * frustumWidth * 0.5f;
		p2.y = cy * frustumHeight * 0.5f;
		viewMat.x[12] = 0; viewMat.x[13] = 0; viewMat.x[14] = 0;
		p2 = viewMat.inverted() * p2;			

		*ox = camTrans[12] + p2.x;
		*oy = camTrans[13] + p2.y;
		*oz = camTrans[14] + p2.z;
	}
	else
	{
		*ox = camTrans[12];
		*oy = camTrans[13];
		*oz = camTrans[14];
	}
	*dx = p1.x - p0.x;
	*dy = p1.y - p0.y;
	*dz = p1.z - p0.z;
}

DLLEXP H3DNode h3dutPickNode( H3DNode cameraNode, float nwx, float nwy )
{	
	float ox, oy, oz, dx, dy, dz;
	h3dutPickRay( cameraNode, nwx, nwy, &ox, &oy, &oz, &dx, &dy, &dz );
	
	if( h3dCastRay( H3DRootNode, ox, oy, oz, dx, dy, dz, 1 ) == 0 )
	{
		return 0;
	}
	else
	{
		H3DNode intersectionNode = 0;
		if( h3dGetCastRayResult( 0, &intersectionNode, 0, 0 ) )
			return intersectionNode;
		else
			return 0;
	}

}


DLLEXP void h3dutGetScreenshotParam( int *width,  int *height ) {
  h3dGetRenderTargetData( 0, "", 0, width, height, 0x0, 0x0, 0 );
}

DLLEXP bool h3dutScreenshotRaw( unsigned char *rgb, int len_rgb )
  {
    // Ask Horde for the width and height of the screenshot.
    int width, height;
    h3dGetRenderTargetData( 0, "", 0, &width, &height, 0x0, 0x0, 0 );

    // Sanity check: User must have provided a buffer that can hold the entire
    // RGB image (each of the RGB components is an unsigned char).
    if (len_rgb < width * height * 3) {
      return false;
    }

    // Ensure we have enough space in the auxiliary buffer. This buffer must be
    // large enough to store each RGBA component as a *float* - *not* an
    // unsinged char.
    static vector<float> f32buf;
    f32buf.reserve(width * height * 4);

    // Determine the size of the pixel buffer in *bytes*. This is somewhat
    // unintuitive because Horde returns each of the RGBA components as a
    // float32, not an unsigned integer.
    const int num_bytes = width * height * 4 * sizeof(float);

  // Copy the pixels (RGBA, float32) into the auxiliary buffer.
  h3dGetRenderTargetData( 0, "", 0, 0x0, 0x0, 0x0, &f32buf[0], num_bytes);

  // Unpack the image from the auxiliary RGBA (float32) buffer into the user
  // provide RGB (uint8) array.
	for( int y = 0; y < height; ++y ) {
      for( int x = 0; x < width; ++x ) {
        int idx = y * width + x;
        rgb[3 * idx + 0] = ftoi_r( clamp( f32buf[4 * idx + 0], 0.f, 1.f ) * 255.f );
        rgb[3 * idx + 1] = ftoi_r( clamp( f32buf[4 * idx + 1], 0.f, 1.f ) * 255.f );
        rgb[3 * idx + 2] = ftoi_r( clamp( f32buf[4 * idx + 2], 0.f, 1.f ) * 255.f );
      }
  }

  return true;
}


DLLEXP bool h3dutScreenshot( const char *filename )
{
	if( filename == 0x0 ) return false;
	
	int width, height;
	h3dGetRenderTargetData( 0, "", 0, &width, &height, 0x0, 0x0, 0 );

	float *pixelsF = new float[width * height * 4];
	h3dGetRenderTargetData( 0, "", 0, 0x0, 0x0, 0x0, pixelsF, width * height * 16 );
	
	// Convert to BGR8
	unsigned char *pixels = new unsigned char[width * height * 3];
	for( int y = 0; y < height; ++y )
	{
		for( int x = 0; x < width; ++x )
		{
			pixels[(y * width + x) * 3 + 0] = ftoi_r( clamp( pixelsF[(y * width + x) * 4 + 2], 0.f, 1.f ) * 255.f );
			pixels[(y * width + x) * 3 + 1] = ftoi_r( clamp( pixelsF[(y * width + x) * 4 + 1], 0.f, 1.f ) * 255.f );
			pixels[(y * width + x) * 3 + 2] = ftoi_r( clamp( pixelsF[(y * width + x) * 4 + 0], 0.f, 1.f ) * 255.f );
		}
	}
	delete[] pixelsF;
	
	char *image;
	int imageSize;
	h3dutCreateTGAImage( pixels, width, height, 24, &image, &imageSize );

	size_t bytesWritten = 0;
	FILE *f = fopen( filename, "wb" );
	if( f )
	{
		bytesWritten = fwrite( image, 1, imageSize, f );
		fclose( f );
	}

	delete[] pixels;
	h3dutFreeMem( &image );

	return bytesWritten == (size_t) width * height * 3 + 18;
}


// =================================================================================================
// DLL entry point
// =================================================================================================

#ifdef PLATFORM_WIN
BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
   switch( ul_reason_for_call )
	{
	case DLL_PROCESS_DETACH:
		// Close log file
		if( outf.is_open() )
		{
			outf << "</table>\n";
			outf << "</div>\n";
			outf << "</body>\n";
			outf << "</html>";
			outf.close();
		}
	break;
	}
	
	return TRUE;
}
#endif
