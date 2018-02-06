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

#ifndef _egShader_H_
#define _egShader_H_

#include "egPrerequisites.h"
#include "egResource.h"
#include "egTexture.h"
#include <set>
#include <vector>
#include <string>

namespace Horde3D {

// forward declarations
class Tokenizer;

// =================================================================================================
// Code Resource
// =================================================================================================

class CodeResource;
typedef SmartResPtr< CodeResource > PCodeResource;

class CodeResource : public Resource
{
public:
	static Resource *factoryFunc( const std::string &name, int flags )
		{ return new CodeResource( name, flags ); }
	
	CodeResource( const std::string &name, int flags );
	~CodeResource();
	Resource *clone();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );

	bool hasDependency( CodeResource *codeRes ) const;
	bool tryLinking( uint32 *flagMask );
	std::string assembleCode() const;

	bool isLoaded() const { return _loaded; }
	const std::string &getCode() const { return _code; }

private:
	bool raiseError( const std::string &msg );
	void updateShaders();

private:
	uint32                                             _flagMask;
	std::string                                        _code;
	std::vector< std::pair< PCodeResource, size_t > >  _includes;	// Pair: Included res and location in _code

	friend class Renderer;
};


// =================================================================================================
// Shader Resource
// =================================================================================================

struct ShaderResData
{
	enum List
	{
		ContextElem = 600,
		SamplerElem,
		UniformElem,
		ContNameStr,
		SampNameStr,
		SampDefTexResI,
		UnifNameStr,
		UnifSizeI,
		UnifDefValueF4
	};
};

// =================================================================================================

struct BlendModes
{
	enum List
	{
		Zero = 0,
		One,
		SrcAlpha,
		OneMinusSrcAlpha,
		DestAlpha,
		OneMinusDestAlpha,
		DestColor,
		SrcColor,
		OneMinusDestColor,
		OneMinusSrcColor
	};
};

struct TestModes
{
	enum List
	{
		LessEqual,
		Less,
		Equal,
		Greater,
		GreaterEqual,
		Always
	};
};

struct CullModes
{
	enum List
	{
		Back,
		Front,
		None
	};
};


struct ShaderCombination
{
	uint32              combMask;
	
	uint32              shaderObj;
	uint32              lastUpdateStamp;

	// Engine uniforms
	int                 uni_frameBufSize;
	int                 uni_viewMat, uni_viewMatInv, uni_projMat, uni_viewProjMat, uni_viewProjMatInv, uni_viewerPos;
	int                 uni_worldMat, uni_worldNormalMat, uni_nodeId, uni_customInstData;
	int                 uni_skinMatRows;
	int                 uni_lightPos, uni_lightDir, uni_lightColor;
	int                 uni_shadowSplitDists, uni_shadowMats, uni_shadowMapSize, uni_shadowBias;
	int                 uni_parPosArray, uni_parSizeAndRotArray, uni_parColorArray;
	int                 uni_olayColor;

	std::vector< int >  customSamplers;
	std::vector< int >  customUniforms;
	std::vector< int >  customBuffers;


	ShaderCombination() :
		combMask( 0 ), shaderObj( 0 ), lastUpdateStamp( 0 )
	{
	}
};


struct ShaderContext
{
	std::string                       id;
	uint32                            flagMask;
	
	// RenderConfig
	BlendModes::List                  blendStateSrc;
	BlendModes::List				  blendStateDst;
	TestModes::List                   depthFunc;
	CullModes::List                   cullMode;
	uint16							  tessVerticesInPatchCount;
	bool                              depthTest;
	bool                              writeDepth;
	bool                              alphaToCoverage;
	bool							  blendingEnabled;
	
	// Shaders
	std::vector< ShaderCombination >  shaderCombs;
	int                               vertCodeIdx, fragCodeIdx, geomCodeIdx, tessCtlCodeIdx, tessEvalCodeIdx, computeCodeIdx;
	bool                              compiled;


	ShaderContext() :
		blendStateSrc( BlendModes::Zero ), blendStateDst( BlendModes::Zero ), depthFunc( TestModes::LessEqual ),
		cullMode( CullModes::Back ), depthTest( true ), writeDepth( true ), alphaToCoverage( false ), tessVerticesInPatchCount( 1 ),
		vertCodeIdx( -1 ), fragCodeIdx( -1 ), geomCodeIdx( -1 ), tessCtlCodeIdx( -1 ), tessEvalCodeIdx( -1 ), computeCodeIdx( -1 ), compiled( false ),
		blendingEnabled( false )
	{
	}
};

// =================================================================================================
struct ShaderBuffer
{
	std::string            id;
};

struct ShaderSampler
{
	std::string            id;
	TextureTypes::List     type;
	PTextureResource       defTex;
	int                    texUnit;
	uint32                 sampState;
	uint32				   usage;

	ShaderSampler() :
		texUnit( -1 ), sampState( 0 ), type( TextureTypes::Tex2D ), usage( 0 )
	{
	}
};

struct ShaderUniform
{
	std::string    id;
	float          defValues[4];
	unsigned char  size;
};

class ShaderResource : public Resource
{
public:
	static Resource *factoryFunc( const std::string &name, int flags )
		{ return new ShaderResource( name, flags ); }

	static void setPreambles( const std::string &vertPreamble, const std::string &fragPreamble, const std::string &geomPreamble,
							  const std::string &tessCtlPreamble, const std::string &tessEvalPreamble, const std::string &computePreamble )
	{
		_vertPreamble = vertPreamble; _fragPreamble = fragPreamble; 
		_geomPreamble = geomPreamble; 
		_tessCtlPreamble = tessCtlPreamble; _tessEvalPreamble = tessEvalPreamble; 
		_computePreamble = computePreamble;
	}

	static uint32 calcCombMask( const std::vector< std::string > &flags );
	
	ShaderResource( const std::string &name, int flags );
	~ShaderResource();
	
	static void initializationFunc();
	void initDefault();
	void release();
	bool load( const char *data, int size );

	void preLoadCombination( uint32 combMask );
	void compileContexts();
	ShaderCombination *getCombination( ShaderContext &context, uint32 combMask );

	int getElemCount( int elem ) const;
	int getElemParamI( int elem, int elemIdx, int param ) const;
	float getElemParamF( int elem, int elemIdx, int param, int compIdx ) const;
	void setElemParamF( int elem, int elemIdx, int param, int compIdx, float value );
	const char *getElemParamStr( int elem, int elemIdx, int param ) const;

	ShaderContext *findContext( const std::string &name )
	{
		for( uint32 i = 0; i < _contexts.size(); ++i )
			if( _contexts[i].id == name ) return &_contexts[i];
		
		return 0x0;
	}

	std::vector< ShaderContext > &getContexts() { return _contexts; }
	CodeResource *getCode( uint32 index ) { return &_codeSections[index]; }

private:
	bool raiseError( const std::string &msg, int line = -1 );
	bool parseFXSection( char *data );

	bool parseFXSectionContext( Tokenizer &tok, const char * identifier, int targetRenderBackend );

	bool compileCombination( ShaderContext &context, ShaderCombination &sc );
	
private:
	static std::string            _vertPreamble, _fragPreamble, _geomPreamble, _tessCtlPreamble, _tessEvalPreamble, _computePreamble;
	static std::string            _tmpCodeVS, _tmpCodeFS, _tmpCodeGS, _tmpCodeCS, _tmpCodeTSCtl, _tmpCodeTSEval;
	static bool					  _defaultPreambleSet;

	std::vector< ShaderContext >  _contexts;
	std::vector< ShaderSampler >  _samplers;
	std::vector< ShaderUniform >  _uniforms;
	std::vector< ShaderBuffer >   _buffers;
	std::vector< CodeResource >   _codeSections;
	std::set< uint32 >            _preLoadList;

	friend class Renderer;
};

typedef SmartResPtr< ShaderResource > PShaderResource;

}
#endif //_egShader_H_
