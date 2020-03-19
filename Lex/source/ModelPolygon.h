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
					CEREAL_NVP( points )
				);
				if ( 1 <= version )
				{
					// archive();
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
			std::vector<Polygon>	polygons;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( cullMode ),
					CEREAL_NVP( polygons )
				);
				if ( 1 <= version )
				{
					// archive();
				}
			}
		public:
			/// <summary>
			/// Set the direction of ignoring normal.
			/// </summary>
			void SetCullMode( CullMode ignoreDirection );
			CullMode GetCullMode() const { return cullMode; }
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
			RaycastResult RaycastWorldSpace( const Donya::Vector4x4 &worldTransform, const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, bool onlyWantIsIntersect = false ) const;
		private:
			/// <summary>
			/// Extract by cullMode.
			/// </summary>
			void ExtractPolygonEdges( std::array<Donya::Vector3, 3> &dest, const std::array<Donya::Vector3, 3> &source ) const;
			/// <summary>
			/// Access by cullMode.
			/// </summary>
			Donya::Vector3 ArrayAccess( const std::array<Donya::Vector3, 3> &source, size_t index ) const;
		};
	}
}
CEREAL_CLASS_VERSION( Donya::Model::Polygon,		0 )
CEREAL_CLASS_VERSION( Donya::Model::PolygonGroup,	0 )
