#pragma once
#include "Format.h"
#include "Root.h"
#include "PS1SeqScanner.h"
#include "Matcher.h"
#include "VGMColl.h"

// *********
// PS1Format
// *********

BEGIN_FORMAT(PS1)
	USING_SCANNER(PS1SeqScanner)
	//USING_MATCHER_WITH_ARG(SimpleMatcher, true)
	USING_MATCHER(FilegroupMatcher)
END_FORMAT()



