#pragma once

struct AnimationComponent
{
	int baseAnimation = 0;
	int secondaryAnimation = 1;
	int layeredAnimation = 2;

	float blendFactor = 0.0f; //0.0f = base, 1.0f = secondary
	bool useLayering = false;

	float time = 0.0f; //playback time

	bool drawSkeleton = false;
};
