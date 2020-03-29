#pragma once

#include <array>

#undef max
#undef min
#include <cereal/types/array.hpp>

#include "Donya/Vector.h"
#include "Donya/Serializer.h"

namespace Donya
{
	namespace Model
	{
		struct Polygon
		{
			int								materialIndex = -1;	// -1 is invalid.
			Donya::Vector3					normal;				// The normalized normal of polygon.
			std::array<Donya::Vector3, 3>	points;				// The model space vertices of a triangle. CW.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( materialIndex ),
					CEREAL_NVP( normal ),
					CEREAL_NVP( points )
				);
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		
		/// <summary>
		/// The raycast result of PolygonGroup. Each member is invalid if the "wasHit" is false.
		/// </summary>
		struct RaycastResult
		{
			bool			wasHit = false;		// This will be true if the ray was hit to some polygon.
			float			distance = 0.0f;	// The distance between the start position of ray and the point of intersection.
			Polygon			nearestPolygon;		// The nearest polygon in intersected polygons.
			Donya::Vector3	intersection;		// The point of intersection to the nearest polygon.
		};

		/// <summary>
		/// PolygonGroup has polygons of a model and provides the Raycast method.
		/// </summary>
		class PolygonGroup
		{
		public:
			/// <summary>
			/// Represents the definition order of triangle that will be excluded when Raycast().
			/// </summary>
			enum class CullMode
			{
				Back,	// I regard as the definition order is CW. The CCW polygon will be ignored.
				Front	// I regard as the definition order is CCW. The CW polygon will be ignored.
			};
		private:
			CullMode				cullMode = CullMode::Back;
			Donya::Vector4x4		coordinateConversion;
			std::vector<Polygon>	polygons;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( cullMode ),
					CEREAL_NVP( coordinateConversion ),
					CEREAL_NVP( polygons )
				);
				if ( 1 <= version )
				{
					// archive();
				}
			}
		public:
			/// <summary>
			/// Set the direction of ignoring normal. Then reassign the normal of all polygon, so heavy.
			/// </summary>
			void ApplyCullMode( CullMode ignoreDirection );
			CullMode GetCullMode() const { return cullMode; }
		public:
			/// <summary>
			/// Apply a coordinate conversion matrix to all polygons. So it is heavy.
			/// </summary>
			void ApplyCoordinateConversion( const Donya::Vector4x4 &coordinateConversion );
		public:
			void Assign( std::vector<Polygon> &rvPolygons );
			void Assign( const std::vector<Polygon> &polygons );
		public:
			/// <summary>
			/// If you set true to "onlyWantIsIntersect", This method will stop as soon if the ray intersects anything. This is a convenience if you just want to know the ray will intersection.
			/// </summary>
			RaycastResult Raycast( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, bool onlyWantIsIntersect = false ) const;
			/// <summary>
			/// Doing the Raycast in the space that represented by "worldTransform". The belong space of the members of the return value is "worldTransform" also.<para></para>
			/// If you set true to "onlyWantIsIntersect", This method will stop as soon if the ray intersects anything. This is a convenience if you just want to know the ray will intersection.
			/// </summary>
			RaycastResult RaycastWorldSpace( const Donya::Vector4x4 &worldTransformOfPolygon, const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, bool onlyWantIsIntersect = false ) const;
		private:
			/// <summary>
			/// The points and normal will be reassigned by current cullMode.
			/// </summary>
			void ApplyMatrixToAllPolygon( const Donya::Vector4x4 &transform );
			/// <summary>
			/// Set the normal of all polygon by cullMode.
			/// </summary>
			void CalcAllPolygonNormal();
		private:
			/// <summary>
			/// Calculate the normal of the points by cullMode.
			/// </summary>
			Donya::Vector3 CalcNormalByOrder( const std::array<Donya::Vector3, 3> &source ) const;
			/// <summary>
			/// Extract by cullMode. Returns [0:AB][1:BC][2:CA].
			/// </summary>
			std::array<Donya::Vector3, 3> ExtractPolygonEdges( const std::array<Donya::Vector3, 3> &source ) const;
			/// <summary>
			/// Access by cullMode.
			/// </summary>
			Donya::Vector3 ArrayAccess( const std::array<Donya::Vector3, 3> &source, size_t index ) const;
		};
	}
}
CEREAL_CLASS_VERSION( Donya::Model::Polygon,		0 )
CEREAL_CLASS_VERSION( Donya::Model::PolygonGroup,	0 )
