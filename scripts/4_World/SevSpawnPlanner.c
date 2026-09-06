enum SevSpawnStatus
{
	INVALID,
	PENDING,
	READY,
	EXHAUSTED
}

class SevSpawnSearch
{
	int Status = SevSpawnStatus.INVALID;
	int Attempts;
	vector Center;
	vector Fallback;
	float Radius;
	bool IsReturn;
	vector Result;
	ref array<vector> Reserved = new array<vector>();

	bool GetResult(out vector position)
	{
		position = vector.Zero;
		if (Status != SevSpawnStatus.READY) return false;
		position = Result;
		return true;
	}
}

class SevSpawnSurface
{
	bool Validate(vector candidate, out vector position) { position = vector.Zero; return false; }
}

class SevSpawnPlanner
{
	protected ref SevSpawnSurface m_Surface;
	protected float m_Minimum;
	protected float m_ReturnRadius;

	void SevSpawnPlanner(SevSpawnSurface surface, float minimum = 10, float returnRadius = 25)
	{
		m_Surface = surface;
		m_Minimum = minimum;
		m_ReturnRadius = returnRadius;
	}

	static bool ValidPosition(vector position)
	{
		return SevSessionStore.ValidVector(position, 100000) && position[0] > 0 && position[2] > 0;
	}

	static vector Sample(vector center, float radius, float u, float theta)
	{
		if (!ValidPosition(center) || !SevHarnessConfig.Between(radius, 0.01, 200) || !SevHarnessConfig.Between(u, 0, 1) || !SevHarnessConfig.Between(theta, 0, Math.PI2)) return vector.Zero;
		float distance = Math.Sqrt(u) * radius;
		return Vector(center[0] + distance * Math.Cos(theta), center[1], center[2] + distance * Math.Sin(theta));
	}

	static bool Accept(bool dry, bool walkable, bool clear, vector position, array<vector> reserved, float minimum)
	{
		if (!dry || !walkable || !clear || !ValidPosition(position) || !reserved || reserved.Count() > 8 || !SevHarnessConfig.Between(minimum, 5, 50)) return false;
		foreach (vector occupied : reserved)
		{
			if (!ValidPosition(occupied)) return false;
			float dx = position[0] - occupied[0];
			float dz = position[2] - occupied[2];
			if (dx * dx + dz * dz < minimum * minimum) return false;
		}
		return true;
	}

	SevSpawnSearch BeginArena(vector center, float radius, array<vector> reserved)
	{
		SevSpawnSearch search = new SevSpawnSearch();
		if (!m_Surface || !ValidPosition(center) || !SevHarnessConfig.Between(radius, 0.01, 200) || !SevHarnessConfig.Between(m_Minimum, 5, 50)) return search;
		if (!reserved || reserved.Count() > 8) return search;
		foreach (vector occupied : reserved)
		{
			if (!ValidPosition(occupied)) return search;
			search.Reserved.Insert(occupied);
		}
		search.Center = center;
		search.Radius = radius;
		search.Status = SevSpawnStatus.PENDING;
		return search;
	}

	SevSpawnSearch BeginReturn(vector origin, vector fallback)
	{
		array<vector> empty = new array<vector>();
		SevSpawnSearch search = BeginArena(origin, m_ReturnRadius, empty);
		if (!ValidPosition(fallback) || !SevHarnessConfig.Between(m_ReturnRadius, 5, 100)) search.Status = SevSpawnStatus.INVALID;
		search.IsReturn = true;
		search.Fallback = fallback;
		return search;
	}

	int Advance(SevSpawnSearch search, int candidateBudget)
	{
		if (!search || search.Status != SevSpawnStatus.PENDING || candidateBudget < 1) return 0;
		int used;
		int budget = Math.Min(candidateBudget, 8);
		while (used < budget && search.Attempts < 32)
		{
			vector candidate;
			if (search.IsReturn && search.Attempts == 0) candidate = search.Center;
			else if (search.IsReturn && search.Attempts == 31) candidate = search.Fallback;
			else candidate = Sample(search.Center, search.Radius, Math.RandomFloat01(), Math.RandomFloat(0, Math.PI2));
			search.Attempts++;
			used++;
			vector position;
			bool valid = ValidPosition(candidate) && m_Surface.Validate(candidate, position);
			// Testing the saved origin must not silently snap a roof/interior origin
			// to distant terrain; nearby attempts may search safe open terrain.
			if (search.IsReturn && search.Attempts == 1 && Math.AbsFloat(position[1] - candidate[1]) > 0.5) valid = false;
			if (valid && Accept(true, true, true, position, search.Reserved, m_Minimum))
			{
				search.Result = position;
				search.Status = SevSpawnStatus.READY;
				return used;
			}
		}
		if (search.Attempts >= 32) search.Status = SevSpawnStatus.EXHAUSTED;
		return used;
	}

	int AdvanceTick(array<ref SevSpawnSearch> searches)
	{
		// Up to eight entrants, each with arena and return preflight requests.
		if (!searches || searches.Count() > 16) return 0;
		int used;
		foreach (SevSpawnSearch search : searches)
		{
			used += Advance(search, 8 - used);
			if (used == 8) break;
		}
		return used;
	}
}

// Conservative open-terrain predicate, not a navigation or occupancy guarantee.
class SevNativeSpawnSurface : SevSpawnSurface
{
	protected ref array<Object> m_Excluded = new array<Object>();

	protected bool DryFootprint(vector position)
	{
		// Center and four cardinal edges of a 1 m footprint; bounded five probes.
		for (int index = 0; index < 5; index++)
		{
			vector point = position;
			if (index == 1) point[0] = point[0] + 0.5;
			if (index == 2) point[0] = point[0] - 0.5;
			if (index == 3) point[2] = point[2] + 0.5;
			if (index == 4) point[2] = point[2] - 0.5;
			if (GetGame().SurfaceIsSea(point[0], point[2]) || GetGame().SurfaceIsPond(point[0], point[2])) return false;
			float height = GetGame().SurfaceY(point[0], point[2]);
			if (!(height > GetGame().SurfaceGetSeaLevelMax() + 0.25) || Math.AbsFloat(height - position[1]) > 0.3) return false;
		}
		return true;
	}

	override bool Validate(vector candidate, out vector position)
	{
		position = vector.Zero;
		if (!GetGame().IsServer() || !SevSpawnPlanner.ValidPosition(candidate)) return false;
		int size = GetGame().GetWorld().GetWorldSize();
		if (candidate[0] < 1 || candidate[2] < 1 || candidate[0] >= size - 1 || candidate[2] >= size - 1) return false;
		vector ground = candidate;
		ground[1] = GetGame().SurfaceY(ground[0], ground[2]);
		if (!SevSpawnPlanner.ValidPosition(ground) || !DryFootprint(ground)) return false;
		vector terrainNormal = GetGame().SurfaceGetNormal(ground[0], ground[2]);
		if (!(terrainNormal[1] >= 0.94)) return false;
		vector contact;
		vector normal;
		int component;
		if (!DayZPhysics.RaycastRV(ground + "0 0.5 0", ground - "0 0.5 0", contact, normal, component, null, null, null, true, true, ObjIntersectGeom)) return false;
		if (!SevSpawnPlanner.ValidPosition(contact) || !(normal[1] >= 0.94) || Math.AbsFloat(contact[1] - ground[1]) > 0.1) return false;
		if (GetGame().IsBoxColliding(contact + "0 1 0", "0 0 0", "1 1.8 1", m_Excluded)) return false;
		position = contact;
		return true;
	}
}
