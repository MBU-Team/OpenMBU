// nVidia Vendor Profile Script
//
// This script is responsible for setting global
// capability strings based on the nVidia vendor.

if(GFXCardProfiler::getVersion() < 53.82)
{
   $GFX::OutdatedDrivers = true;
   $GFX::OutdatedDriversLink = "<a:www.nvidia.com>You can get newer drivers here.</a>.";
}
else
{
    $GFX::OutdatedDrivers = false;
}

// Silly card has trouble with this!
GFXCardProfiler::setCapability("autoMipmapLevel", false);
