#include "ModelPolygon.h"

#include "Donya/Useful.h"	// Use EPSILON constant.

namespace Donya
{
	namespace Model
	{
		void PolygonGroup::SetCullMode( CullMode ignoreDir ) { cullMode = ignoreDir; }

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

			float							nearestDistance = FLT_MAX;
			Donya::Vector3					faceNormal;	// Does not normalized.
			std::array<Donya::Vector3, 3>	edges;		// CullMode::Back:[0:AB][1:BC][2:CA]. CullMode::Front:[0:AC][1:CB][2:BA].

			for ( const auto &it : polygons )
			{
				ExtractPolygonEdges( edges, it.points );
				faceNormal	= Donya::Vector3::Cross( edges[0], edges[1] );

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

			auto Multiply = []( const Donya::Vector3 &v, float fourthParam, const Donya::Vector4x4 &m )
			{
				Donya::Vector4 transformed = m.Mul( v, fourthParam );
				transformed /= transformed.w;
				return transformed.XYZ();
			};

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

			const Donya::Vector3  edgeAB = result.nearestPolygon.points[1] - result.nearestPolygon.points[0];
			const Donya::Vector3  edgeAC = result.nearestPolygon.points[2] - result.nearestPolygon.points[0];
			result.nearestPolygon.normal = Donya::Cross( edgeAB, edgeAC ).Unit();

			result.distance = Donya::Vector3{ result.intersection - rayStart }.Length();

			return result;
		}

		void PolygonGroup::ExtractPolygonEdges( std::array<Donya::Vector3, 3> &dest, const std::array<Donya::Vector3, 3> &source ) const
		{
			auto A = [&]() { return source[0]; };
			auto B = [&]() { return source[1]; };
			auto C = [&]() { return source[2]; };
			auto Vector = [&]( auto &from, auto &to )
			{
				return to() - from();
			};

			switch ( cullMode )
			{
			case CullMode::Back: // [0:AB][1:BC][2:CA].
				dest[0] = Vector( A, B );
				dest[1] = Vector( B, C );
				dest[2] = Vector( C, A );
				return;
			case CullMode::Front: // [0:AC][1:CB][2:BA].
				dest[0] = Vector( A, C );
				dest[1] = Vector( C, B );
				dest[2] = Vector( B, A );
				return;
			default: _ASSERT_EXPR( 0, L"Error : Unexpected cull-mode!" ); return;
			}
		}
		Donya::Vector3 PolygonGroup::ArrayAccess( const std::array<Donya::Vector3, 3> &source, size_t index ) const
		{
			constexpr size_t pointCount = 3; // == source.size(). 1 based.
			assert(  index < pointCount );

			switch ( cullMode )
			{
			case CullMode::Back:	return source[index];
			case CullMode::Front:	return source[pointCount - 1 - index];
			default: _ASSERT_EXPR( 0, L"Error : Unexpected cull-mode!" ); break;
			}
			return {};
		}
	}
}
