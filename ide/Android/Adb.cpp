#include "Android.h"

NAMESPACE_UPP

Adb::Adb(const String& path) : path(path)
{
	
}

Adb::~Adb()
{
	
}

String Adb::MakeInstallCmd(const String& serial, const String& apkPath) const
{
	return path + " -s " + serial + " install -r " + apkPath;
}

String Adb::MakeInstallOnDeviceCmd(const String& apkPath) const
{
	return path + " -d install -r " + apkPath;
}

String Adb::MakeInstallOnEmulatorCmd(const String& apkPath) const
{
	return path + " -e install -r " + apkPath;
}

END_UPP_NAMESPACE
