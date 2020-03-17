#include "ModelPolygon.h"

#include "Donya/Useful.h"	// Use EPSILON constant.

namespace Donya
{
	namespace Model
	{
		void PolygonGroup::Assign( std::vector<Polygon> &rvSource )
		{
			polygons = std::move( rvSource );
		}
		void PolygonGroup::Assign( const std::vector<Polygon> &source )
		{
			polygons = source;
		}

		RaycastResult PolygonGroup::Raycast( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, bool onlyWantIsIntersect )
		{
			RaycastResult result;

			const Donya::Vector3 rayVec  = rayEnd - rayStart;
			const Donya::Vector3 nRayVec = rayVec.Unit();

			float							nearestDistance = FLT_MAX;
			Donya::Vector3					faceNormal;	// Does not normalized.
			std::array<Donya::Vector3, 3>	edges;		// [0:AB][1:BC][2:CA]. Edges of face.

			for ( const auto &it : polygons )
			{
				edges[0]	= it.points[1] - it.points[0];
				edges[1]	= it.points[2] - it.points[1];
				edges[2]	= it.points[0] - it.points[2];
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
					Donya::Vector3 vIV		= it.points[i] - intersection;
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
	}
}
