#include "ModelPolygon.h"

#include "Donya/Useful.h"	// Use EPSILON constant.

namespace Donya
{
	namespace Model
	{
		namespace
		{
			Donya::Vector3 Multiply( const Donya::Vector3 &v, float fourthParam, const Donya::Vector4x4 &m )
			{
				Donya::Vector4 transformed = m.Mul( v, fourthParam );
				transformed /= transformed.w;
				return transformed.XYZ();
			};
		}

		void PolygonGroup::ApplyCullMode( CullMode ignoreDir )
		{
			cullMode = ignoreDir;
			CalcAllPolygonNormal();
		}

		void PolygonGroup::ApplyCoordinateConversion( const Donya::Vector4x4 &newConversionMatrix )
		{
			// Return to default the coordinate system.
			// The default coordinate matrix is identity, so it is safe even if when first time.
			ApplyMatrixToAllPolygon( coordinateConversion.Inverse() );

			coordinateConversion = newConversionMatrix;

			// Convert to new coordinate system.
			ApplyMatrixToAllPolygon( newConversionMatrix );
		}

		void PolygonGroup::Assign( std::vector<Polygon> &rvSource )
		{
			polygons = std::move( rvSource );
		}
		void PolygonGroup::Assign( const std::vector<Polygon> &source )
		{
			polygons = source;
		}

		RaycastResult PolygonGroup::Raycast( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, bool onlyWantIsIntersect ) const
		{
			RaycastResult result;

			const Donya::Vector3 rayVec  = rayEnd - rayStart;
			const Donya::Vector3 nRayVec = rayVec.Unit();

			float							nearestDistance = rayVec.Length();
			Donya::Vector3					faceNormal;	// Does not normalized.
			std::array<Donya::Vector3, 3>	edges;		// CullMode::Back:[0:AB][1:BC][2:CA]. CullMode::Front:[0:AC][1:CB][2:BA].

			for ( const auto &it : polygons )
			{
				edges		= ExtractPolygonEdges( it.points );
				faceNormal	= Donya::Cross( edges[0], -edges[2] ); // AB x AC

				// The ray does not intersection to back-face.
				// (If use '<': allow the horizontal intersection, '<=': disallow the horizontal intersection)
				if ( 0.0f   < Donya::Vector3::Dot( rayVec, faceNormal ) ) { continue; }
				// else

				// Distance between intersection point and rayStart.
				float currentDistance{};
				{
					const Donya::Vector3 vPV = it.points[0] - rayStart;

					float dotPN = Donya::Vector3::Dot( vPV,		faceNormal );
					float dotRN = Donya::Vector3::Dot( nRayVec,	faceNormal );

					currentDistance = dotPN / ( dotRN + EPSILON /* Prevent zero-divide */ );
				}

				// The intersection point is there inverse side by rayEnd.
				if ( currentDistance < 0.0f ) { continue; }
				// else

				// I need the nearest polygon only.
				if ( nearestDistance <= currentDistance ) { continue; }
				// else

				Donya::Vector3 intersection = rayStart + ( nRayVec * currentDistance );

				// Judge the intersection-point is there inside of triangle.
				bool onOutside = false;
				for ( size_t i = 0; i < it.points.size()/* 3 */; ++i )
				{
					// Requirement: All vector(I->P) must facing right side of the edge vector.
					Donya::Vector3 vIV		= ArrayAccess( it.points, i ) - intersection;
					Donya::Vector3 cross	= Donya::Vector3::Cross( vIV, edges[i] );

					float dotCN = Donya::Vector3::Dot( cross, faceNormal );
					if (  dotCN < 0.0f )
					{
						onOutside = true;
						break;
					}
				}
				if ( onOutside ) { continue; }
				// else

				nearestDistance = currentDistance;

				result.wasHit			= true;
				result.distance			= currentDistance;
				result.nearestPolygon	= it;
				result.intersection		= intersection;

				if ( onlyWantIsIntersect ) { return result; }
				// else
			}

			return result;
		}
		RaycastResult PolygonGroup::RaycastWorldSpace( const Donya::Vector4x4 &transform, const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, bool onlyWantIsIntersect ) const
		{
			const Donya::Vector4x4 invTransform = transform.Inverse();

			// Transformed space.
			const Donya::Vector3 tsRayStart	= Multiply( rayStart,	1.0f, invTransform );
			const Donya::Vector3 tsRayEnd	= Multiply( rayEnd,		1.0f, invTransform );

			auto  result = Raycast( tsRayStart, tsRayEnd, onlyWantIsIntersect );
			if ( !result.wasHit ) { return result; }
			// else

			struct VecFloat
			{
				Donya::Vector3 *v = nullptr;
				float f = 0.0f;
			};

			VecFloat applyList[]
			{
				{ &result.intersection,				1.0f },
				{ &result.nearestPolygon.points[0],	1.0f },
				{ &result.nearestPolygon.points[1],	1.0f },
				{ &result.nearestPolygon.points[2],	1.0f }
			};

			auto ToWorldSpace = [&]( VecFloat *pTarget )
			{
				*pTarget->v = Multiply( *pTarget->v, pTarget->f, transform );
			};
			for ( auto &it : applyList )
			{
				ToWorldSpace( &it );
			}

			const std::array<Donya::Vector3, 3> actualPoints
			{
				ArrayAccess( result.nearestPolygon.points, 0U ),
				ArrayAccess( result.nearestPolygon.points, 1U ),
				ArrayAccess( result.nearestPolygon.points, 2U )
			};
			result.nearestPolygon.points = actualPoints;

			const Donya::Vector3  edgeAB = result.nearestPolygon.points[1] - result.nearestPolygon.points[0];
			const Donya::Vector3  edgeAC = result.nearestPolygon.points[2] - result.nearestPolygon.points[0];
			result.nearestPolygon.normal = Donya::Cross( edgeAB, edgeAC ).Unit();

			result.distance = Donya::Vector3{ result.intersection - rayStart }.Length();

			return result;
		}

		void PolygonGroup::ApplyMatrixToAllPolygon( const Donya::Vector4x4 &transform )
		{
			auto Transform = []( Polygon *pTarget, const Donya::Vector4x4 &m )
			{
				pTarget->points[0] = Multiply( pTarget->points[0], 1.0f, m );
				pTarget->points[1] = Multiply( pTarget->points[1], 1.0f, m );
				pTarget->points[2] = Multiply( pTarget->points[2], 1.0f, m );
			};

			for ( auto &it : polygons )
			{
				Transform( &it, transform );
			}

			CalcAllPolygonNormal();
		}

		void PolygonGroup::CalcAllPolygonNormal()
		{
			for ( auto &it : polygons )
			{
				it.normal = CalcNormalByOrder( it.points );
			}
		}
		Donya::Vector3 PolygonGroup::CalcNormalByOrder( const std::array<Donya::Vector3, 3> &source ) const
		{
			auto A = [&]() { return ArrayAccess( source, 0U ); };
			auto B = [&]() { return ArrayAccess( source, 1U ); };
			auto C = [&]() { return ArrayAccess( source, 2U ); };
			auto Vector = [&]( auto &from, auto &to )
			{
				return to() - from();
			};

			const Donya::Vector3 ab = Vector( A, B );
			const Donya::Vector3 ac = Vector( A, C );
			return Donya::Cross( ab, ac ).Unit();
		}

		std::array<Donya::Vector3, 3> PolygonGroup::ExtractPolygonEdges( const std::array<Donya::Vector3, 3> &source ) const
		{
			auto A = [&]() { return ArrayAccess( source, 0U ); };
			auto B = [&]() { return ArrayAccess( source, 1U ); };
			auto C = [&]() { return ArrayAccess( source, 2U ); };
			auto Vector = [&]( auto &from, auto &to )
			{
				return to() - from();
			};

			return std::array<Donya::Vector3, 3>
			{
				Vector( A, B ),
				Vector( B, C ),
				Vector( C, A )
			};
		}

		Donya::Vector3 PolygonGroup::ArrayAccess( const std::array<Donya::Vector3, 3> &source, size_t index ) const
		{
			constexpr size_t pointCount = 3;	// == source.size(). 1 based.
			assert ( index < pointCount );		// index is 0, 1 or 2.

			switch ( cullMode )
			{
			case CullMode::Back:	// Behave like this: [0:A][1:B][2:C]
				return source[index];
			case CullMode::Front:	// Behave like this: [0:A][1:C][2:B]
				// "( count - index )" returns "3, 2, 1".
				// So "( count - index ) % count" returns "0, 2, 1".
				return source[( pointCount - index ) % pointCount];
			default: _ASSERT_EXPR( 0, L"Error : Unexpected cull-mode!" ); break;
			}

			constexpr Donya::Vector3 ERROR_VALUE{ FLT_MAX, FLT_MAX, FLT_MAX };
			return ERROR_VALUE;
		}
	}
}
