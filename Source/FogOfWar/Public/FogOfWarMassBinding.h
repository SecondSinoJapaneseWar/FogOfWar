// Copyright Winyunq, 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#ifndef FOW_USE_MASSBATTLE_BINDING
#define FOW_USE_MASSBATTLE_BINDING 1
#endif

#if FOW_USE_MASSBATTLE_BINDING
#include "Fragments/Team.h"
#include "Fragments/Transform.h"

#define FOW_LOCATION_FRAGMENT FLocating
#define FOW_TEAM_FRAGMENT FTeam
#define FOW_GET_LOCATION(Fragment) ((Fragment).Location)
#define FOW_GET_PREVIOUS_LOCATION(Fragment) ((Fragment).PreLocation)
#define FOW_GET_TEAM_INDEX(Fragment) ((Fragment).index)

#else
#include "MassFogOfWarFragments.h"

#define FOW_LOCATION_FRAGMENT FFogOfWarLocationFragment
#define FOW_TEAM_FRAGMENT FFogOfWarTeamFragment
#define FOW_GET_LOCATION(Fragment) ((Fragment).Location)
#define FOW_GET_PREVIOUS_LOCATION(Fragment) ((Fragment).PreviousLocation)
#define FOW_GET_TEAM_INDEX(Fragment) ((Fragment).TeamIndex)

#endif
