#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

float const MATH_PI			= 3.14159265359f;
unsigned const SAMPLE_NUM	= 10000;

struct Vec3
{
	float x;
	float y;
	float z;
};

float dot( Vec3 a, Vec3 b )
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

void SaveTGA( char const* path, unsigned width, unsigned height, void const* data )
{
	#pragma pack( push, 1 )
	struct STGAHeader 
	{
	   char  idlength;
	   char  colourmaptype;
	   char  datatypecode;
	   short int colourmaporigin;
	   short int colourmaplength;
	   char  colourmapdepth;
	   short int x_origin;
	   short int y_origin;
	   short width;
	   short height;
	   char  bitsperpixel;
	   char  imagedescriptor;
	};
	#pragma pack( pop )


	FILE* f = nullptr;
    fopen_s( &f, path, "wb" );
    if ( !f )
    {
        return;
    }

	STGAHeader header;
	memset( &header, 0, sizeof( header ) );
	header.width			= width;
	header.height			= height;
	header.datatypecode		= 2;
	header.bitsperpixel		= 24;
	header.imagedescriptor	= 1 << 5;
	fwrite( &header, sizeof( header ), 1, f );

	fwrite( data, width * height * 3, 1, f );

	fclose( f );
}

Vec3 RandomConeRays[ SAMPLE_NUM ];

void GenerateRandomConeRays( float coneAngle )
{
	float const cosConeAngle = cosf( 0.5f * ( coneAngle * MATH_PI / 180.0f ) );
	for ( unsigned iSample = 0; iSample < SAMPLE_NUM; ++iSample )
	{
		float cosTheta	= ( (float) rand() / RAND_MAX ) * ( 1.0f - cosConeAngle ) + cosConeAngle;
		float sinTheta	= sqrtf( 1.0f - cosTheta * cosTheta );
		float phi		= MATH_PI * ( (float) rand() / RAND_MAX ) * 2.0f - 1.0f;
		float cosPhi	= cosf( phi );
		float sinPhi	= sinf( phi );

		RandomConeRays[ iSample ] = { sinTheta * cosPhi, sinTheta * sinPhi, cosTheta };
	}
}

float Occlusion( float occluderAngleSin, float occluderToBeamAngleCos )
{	
	float const occluderToBeamAngleSin	= sqrtf( 1.0f - occluderToBeamAngleCos * occluderToBeamAngleCos );
	float const occluderAngleCos		= sqrtf( 1.0f - occluderAngleSin * occluderAngleSin );
	Vec3 const occluderDir				= { occluderToBeamAngleSin, 0.0f, occluderToBeamAngleCos };

	unsigned hitNum = 0;
	for ( unsigned iSample = 0; iSample < SAMPLE_NUM; ++iSample )
	{
		if ( dot( occluderDir, RandomConeRays[ iSample ] ) > occluderAngleCos )
		{
			++hitNum;
		}
	}

	return 1.0f - hitNum / (float) SAMPLE_NUM;
}

int main( int argc, char** argv )
{
    if ( argc < 4 )
    {
        printf( "Incorrect params\n" );
        printf( "Usage: ConeSphereOcclusionLUT.exe lut_width lut_height cone_angle_in_degress\n" );
        printf( "ConeSphereOcclusionLUT.exe 128 64 30\n" );
        return 1;
    }

	float coneAngle		= 30.0f;
	unsigned lutWidth	= 128;
	unsigned lutHeight	= 64;
	sscanf_s( argv[ 1 ], "%u", &lutWidth );
	sscanf_s( argv[ 2 ], "%u", &lutHeight );
	sscanf_s( argv[ 3 ], "%f", &coneAngle );

	uint8_t* lutData = new uint8_t[ lutWidth * lutHeight * 3 ];
	GenerateRandomConeRays( coneAngle );

	for ( unsigned iSinTheta = 0; iSinTheta < lutHeight; ++iSinTheta ) // angle subtended by the occluder [0;1]
	{
		float const sinTheta = iSinTheta / ( lutHeight - 1.0f );

		for ( unsigned iCosPhi = 0; iCosPhi < lutWidth; ++iCosPhi ) // angle between the cone axis and the vector pointing towards the occluder (sphere) center [-1;1]
		{
			float const cosPhi = 2.0f * ( iCosPhi / ( lutWidth - 1.0f ) ) - 1.0f;
			float const sampleValue = Occlusion( sinTheta, cosPhi );

			uint8_t sampleU8 = uint8_t( sampleValue * 255.0f + 0.5f );
			lutData[ iCosPhi * 3 + iSinTheta * lutWidth * 3 + 0 ] = sampleU8;
			lutData[ iCosPhi * 3 + iSinTheta * lutWidth * 3 + 1 ] = sampleU8;
			lutData[ iCosPhi * 3 + iSinTheta * lutWidth * 3 + 2 ] = sampleU8;
		}
	}

	SaveTGA( "cone_sphere_occlusion_lut.tga", lutWidth, lutHeight, lutData );
	delete[] lutData;

	return 0;
}