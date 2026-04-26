# All variables and this file are optional, if they are not present the PG and the
# makefiles will try to parse the correct values from the file system.
#
# Variables that specify exclusions can use % as a wildcard to specify that anything in
# that position will match. A partial path can also be specified to, for example, exclude
# a whole folder from the parsed paths from the file system
#
# Variables can be specified using = or +=
# = will clear the contents of that variable both specified from the file or the ones parsed
# from the file system
# += will add the values to the previous ones in the file or the ones parsed from the file
# system
#
# The PG can be used to detect errors in this file, just create a new project with this addon
# and the PG will write to the console the kind of error and in which line it is

meta:
	ADDON_NAME = ofxNDI
	ADDON_DESCRIPTION = Minimal OpenFrameworks addon for NDI send/receive.
	ADDON_AUTHOR = ofxNDI contributors
	ADDON_TAGS =
	ADDON_URL = https://github.com/leadedge/ofxNDI

common:
	ADDON_INCLUDES += libs/NDI/include
	ADDON_INCLUDES += src

# osx:
#	ADDON_LDFLAGS = -L/usr/local/lib -lndi
#	ADDON_LDFLAGS += -Wl,-rpath,/usr/local/lib

macos:
	ADDON_LDFLAGS = -L/usr/local/lib -lndi
	ADDON_LDFLAGS += -Wl,-rpath,/usr/local/lib

linux64:
	ADDON_LDFLAGS = -lndi

linux:
	ADDON_LDFLAGS = -lndi

linuxarmv6l:
	ADDON_LDFLAGS = -lndi

linuxarmv7l:
	ADDON_LDFLAGS = -lndi
