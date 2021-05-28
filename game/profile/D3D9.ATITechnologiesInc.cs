// ATI Vendor Profile Script
//
// This script is responsible for setting global
// capability strings based on the nVidia vendor.

if(GFXCardProfiler::getVersion() < 64.44)
{
	$GFX::OutdatedDrivers = true;
	$GFX::OutdatedDriversLink = "<a:www.ati.com>You can get newer drivers here.</a>.";
}