#pragma once

struct AnimationComponent
{
	int baseAnimation = 0;
	int secondaryAnimation = 1;

	float blendFactor = 0.0f; //0.0f = base, 1.0f = secondary
	bool useLayering = false;

	std::string upperBodyRootNode = "mixamorig:Spine";

	float time = 0.0f; //playback time

	bool useSpeedControl = false;
	float speed = 0.0f;
	float maxSpeedForFullBlend = 10.0f;

	bool playOnce = false;
	bool freezeAtEnd = false;
	bool animationFinished = false;
	float animationDuration = 1.0f;

	bool drawSkeleton = false;
};
