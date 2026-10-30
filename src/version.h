// Gobinator's version, in one place: the log (main.cpp), the EXE's Properties > Details (gobinator.rc) and the zip's
// name (package\make-zip.ps1 reads GOBINATOR_VERSION_TAG from this file). A beta or release candidate counts up
// GOBINATOR_VERSION_BUILD and says so in the text; a release has build 0 and plain text ("1.1").
#pragma once

#define GOBINATOR_VERSION_MAJOR 1
#define GOBINATOR_VERSION_MINOR 1
#define GOBINATOR_VERSION_PATCH 0
#define GOBINATOR_VERSION_BUILD 0
#define GOBINATOR_VERSION_TEXT "1.1"
#define GOBINATOR_VERSION_TAG "1.1"

#ifndef RC_INVOKED
#define GOBINATOR_WIDE_(s) L##s
#define GOBINATOR_WIDE(s) GOBINATOR_WIDE_(s)
#endif
