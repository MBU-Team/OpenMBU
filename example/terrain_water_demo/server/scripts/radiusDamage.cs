//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Support function which applies damage to objects within the radius of
// some effect, usually an explosion.  This function will also optionally 
// apply an impulse to each object.

function radiusDamage(%sourceObject, %position, %radius, %damage, %damageType, %impulse)
{
   // Use the container system to iterate through all the objects
   // within our explosion radius.  We'll apply damage to all ShapeBase
   // objects.
   InitContainerRadiusSearch(%position, %radius, $TypeMasks::ShapeBaseObjectType);

   %halfRadius = %radius / 2;
   while ((%targetObject = containerSearchNext()) != 0) {

      // Calculate how much exposure the current object has to
      // the explosive force.  The object types listed are objects
      // that will block an explosion.  If the object is totally blocked,
      // then no damage is applied.
      %coverage = calcExplosionCoverage(%position, %targetObject,
         $TypeMasks::InteriorObjectType |  $TypeMasks::TerrainObjectType |
         $TypeMasks::ForceFieldObjectType | $TypeMasks::VehicleObjectType);
      if (%coverage == 0)
         continue;

      // Radius distance subtracts out the length of smallest bounding
      // box axis to return an appriximate distance to the edge of the
      // object's bounds, as opposed to the distance to it's center.
      %dist = containerSearchCurrRadiusDist();

      // Calculate a distance scale for the damage and the impulse.
      // Full damage is applied to anything less than half the radius away,
      // linear scale from there.
      %distScale = (%dist < %halfRadius)? 1.0:
         1.0 - ((%dist - %halfRadius) / %halfRadius);

      // Apply the damage
      %targetObject.damage(%sourceObject, %position,
         %damage * %coverage * %distScale, %damageType);

      // Apply the impulse
      if (%impulse) {
         %impulseVec = VectorSub(%targetObject.getWorldBoxCenter(), %position);
         %impulseVec = VectorNormalize(%impulseVec);
         %impulseVec = VectorScale(%impulseVec, %impulse * %distScale);
         %targetObject.applyImpulse(%position, %impulseVec);
      }
   }
}
