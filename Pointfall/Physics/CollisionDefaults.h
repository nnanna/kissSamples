
struct CollisionDefaults
{
	CollisionDefaults()
	{
		IMPULSE_FACTOR				= 0.4f;
		COLLISION_RADIUS_SQ			= 1.2f;
		MAX_TOTAL_COLLISIONS		= 8000;
		MAX_PER_ENTITY_COLLISIONS	= 2;
	}

	CollisionDefaults(float pRadiusSq, int pMaxEntityCollisions)
	{
		IMPULSE_FACTOR				= 0.4f;
		COLLISION_RADIUS_SQ			= pRadiusSq;
		MAX_TOTAL_COLLISIONS		= 8000;
		MAX_PER_ENTITY_COLLISIONS	= pMaxEntityCollisions;
	}

	float IMPULSE_FACTOR;
	float COLLISION_RADIUS_SQ;
	unsigned MAX_TOTAL_COLLISIONS;
	unsigned MAX_PER_ENTITY_COLLISIONS;
};