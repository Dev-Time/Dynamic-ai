//! because too many things are private..
//! i wanted to change the exp log to [Dynamic AI] for the spawn() function


class eAISpatialPatrol : eAIPatrol
{
	private static int m_NumberOfDynamicPatrols;

	vector m_Position;
	autoptr array<vector> m_Waypoints;
	eAIWaypointBehavior m_WaypointBehaviour;
	float m_MinimumRadius;
	float m_MaximumRadius;
	float m_DespawnRadius;
	float m_MovementSpeedLimit;
	float m_MovementThreatSpeedLimit;
	int m_NumberOfAI;
	int m_RespawnTime;
	int m_DespawnTime;
	string m_Loadout;
	ref eAIFaction m_Faction;
	ref eAIFormation m_Formation;
	bool m_CanBeLooted;
	bool m_UnlimitedReload;
	float m_AccuracyMin;
	float m_AccuracyMax;
	float m_ThreatDistanceLimit;
	float m_DamageMultiplier;

	eAIGroup m_Group;
	float m_TimeSinceLastSpawn;
	bool m_CanSpawn;
	private bool m_WasGroupDestroyed;

	//! @note hard function param limit seems to be 17, adding anymore will cause CTD
	static eAISpatialPatrol CreateEx(vector pos, array<vector> waypoints, eAIWaypointBehavior behaviour, string loadout = "", int count = 1, int respawnTime = 600, int despawnTime = 600, eAIFaction faction = null, eAIFormation formation = null, bool autoStart = true, float minR = 300, float maxR = 800, float despawnR = 880, float speedLimit = 3.0, float threatspeedLimit = 3.0, bool canBeLooted = true, bool unlimitedReload = false/*, float accuracyMin = -1, float accuracyMax = -1*/)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0("eAISpatialPatrol", "Create");
		#endif

		eAISpatialPatrol patrol;
		Class.CastTo(patrol, ((typename)eAISpatialPatrol).Spawn());
		patrol.m_Position = pos;
		patrol.m_Waypoints = waypoints;
		patrol.m_WaypointBehaviour = behaviour;
		patrol.m_NumberOfAI = count;
		patrol.m_Loadout = loadout;
		patrol.m_RespawnTime = respawnTime;
		patrol.m_DespawnTime = despawnTime;
		patrol.m_MinimumRadius = minR;
		patrol.m_MaximumRadius = maxR;
		patrol.m_DespawnRadius = despawnR;
		patrol.m_MovementSpeedLimit = speedLimit;
		patrol.m_MovementThreatSpeedLimit = threatspeedLimit;
		patrol.m_Faction = faction;
		patrol.m_Formation = formation;
		patrol.m_CanBeLooted = canBeLooted;
		patrol.m_UnlimitedReload = unlimitedReload;
		patrol.m_CanSpawn = true;
		if (patrol.m_Faction == null) patrol.m_Faction = new eAIFactionCivilian();
		if (patrol.m_Formation == null) patrol.m_Formation = new eAIFormationVee();
		if (autoStart) patrol.Start();
		return patrol;
	}

	static eAISpatialPatrol Create(vector pos, array<vector> waypoints, eAIWaypointBehavior behaviour, string loadout = "", int count = 1, int respawnTime = 600, eAIFaction faction = null, bool autoStart = true, float minR = 300, float maxR = 800, float speedLimit = 3.0, float threatspeedLimit = 3.0, bool canBeLooted = true, bool unlimitedReload = false)
	{
		return CreateEx(pos, waypoints, behaviour, loadout, count, respawnTime, 600, faction, null, autoStart, minR, maxR, maxR * 1.1, speedLimit, threatspeedLimit, canBeLooted, unlimitedReload);
	}

	void SetAccuracy(float accuracyMin, float accuracyMax)
	{
		m_AccuracyMin = accuracyMin;
		m_AccuracyMax = accuracyMax;
	}

	void SetThreatDistanceLimit(float distance)
	{
		m_ThreatDistanceLimit = distance;
	}

	void SetDamageMultiplier(float multiplier)
	{
		m_DamageMultiplier = multiplier;
	}

	private eAIBase SpawnAI(vector pos)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "SpawnAI");
		#endif

		pos = ExpansionAIPatrol.GetPlacementPosition(pos);

		eAIBase ai;
		if (!Class.CastTo(ai, GetGame().CreateObject(GetRandomAI(), pos))) return null;

		ai.SetPosition(pos);

		if ( m_Loadout == "" )
			m_Loadout = m_Faction.GetDefaultLoadout();

		ExpansionHumanLoadout.Apply(ai, m_Loadout, false);
				
		ai.SetMovementSpeedLimits(m_MovementSpeedLimit, m_MovementThreatSpeedLimit);
		ai.Expansion_SetCanBeLooted(m_CanBeLooted);
		ai.eAI_SetUnlimitedReload(m_UnlimitedReload);
		ai.eAI_SetAccuracy(m_AccuracyMin, m_AccuracyMax);
		ai.eAI_SetThreatDistanceLimit(m_ThreatDistanceLimit);
		ai.eAI_SetDamageMultiplier(m_DamageMultiplier);

		return ai;
	}

	bool WasGroupDestroyed()
	{
		if (!m_Group)
			return false;

		if (m_WasGroupDestroyed)
			return true;

		for (int i = 0; i < m_Group.Count(); i++)
		{
			DayZPlayerImplement member = m_Group.GetMember(i);
			if (member && member.IsInherited(PlayerBase) && member.IsAlive())
			{
				return false;
			}
		}

		m_WasGroupDestroyed = true;

		if (m_NumberOfDynamicPatrols)
			m_NumberOfDynamicPatrols--;

		return true;
	}

	void Spawn()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "Spawn");
		#endif

		if (m_Group) return;

		if (GetExpansionSettings().GetLog().AIPatrol)
            GetExpansionSettings().GetLog().PrintLog("[Dynamic AI] Spawning " + m_NumberOfAI + " " + m_Faction.ClassName().Substring(10, m_Faction.ClassName().Length() - 10) + " bots at " + m_Position);

		m_TimeSinceLastSpawn = 0;
		m_CanSpawn = false;
		m_WasGroupDestroyed = false;

		eAIBase ai = SpawnAI(m_Position);
		m_Group = ai.GetGroup();
		m_Group.SetFaction(m_Faction);
		m_Group.SetFormation(m_Formation);
		m_Group.SetWaypointBehaviour(m_WaypointBehaviour);
		for (int idx = 0; idx < m_Waypoints.Count(); idx++)
		{
			m_Group.AddWaypoint(m_Waypoints[idx]);
			if (m_Waypoints[idx] == m_Position)
			{
				m_Group.m_CurrentWaypointIndex = idx;
				if (Math.RandomIntInclusive(0, 1))
					m_Group.m_BackTracking = true;
			}
		}

		for (int i = 1; i < m_NumberOfAI; i++)
		{
			ai = SpawnAI(m_Formation.ToWorld(m_Formation.GetPosition(i)));
			ai.SetGroup(m_Group);
		}

		m_NumberOfDynamicPatrols++;
	}

	void Despawn()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "Despawn");
		#endif

		if (m_Group)
		{
			m_Group.ClearAI();
			m_Group = null;
		}

		if (!m_WasGroupDestroyed && m_NumberOfDynamicPatrols)
			m_NumberOfDynamicPatrols--;
	}

	override void OnUpdate()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "OnUpdate");
		#endif

		if ( WasGroupDestroyed() && m_RespawnTime < 0 )
		{
			return;
		}

		if (!m_CanSpawn && (!m_Group || m_WasGroupDestroyed))
		{
			m_TimeSinceLastSpawn += eAIPatrol.UPDATE_RATE_IN_SECONDS;
			m_CanSpawn = m_RespawnTime > -1 && m_TimeSinceLastSpawn >= m_RespawnTime;
		}

		if (!m_Group)
		{
			if (!m_CanSpawn)
			{
				return;
			}

			int maxPatrols = GetExpansionSettings().GetAI().MaximumDynamicPatrols;
			if (maxPatrols > -1 && m_NumberOfDynamicPatrols >= maxPatrols)
			{
				return;
			}
		}

		//! CE API is only avaialble after game is loaded
		if (!GetCEApi())
			return;

		vector patrolPos = m_Position;
		DayZPlayerImplement leader = null;
		if (m_Group && m_Group.GetLeader())
		{
			leader = m_Group.GetLeader();
			patrolPos = leader.GetPosition();
		}

		if (m_Group)
		{
			if (GetCEApi().AvoidPlayer(patrolPos, m_DespawnRadius))
			{
				m_TimeSinceLastSpawn += eAIPatrol.UPDATE_RATE_IN_SECONDS;
				if (m_TimeSinceLastSpawn >= m_DespawnTime)
					Despawn();
			}
		}
		else
		{
			if (!GetCEApi().AvoidPlayer(patrolPos, m_MaximumRadius) && GetCEApi().AvoidPlayer(patrolPos, m_MinimumRadius))
			{
				Spawn();
			}
		}
	}
	
	override void Debug()
	{
		super.Debug();
		
		Print(m_Group);
		Print(m_TimeSinceLastSpawn);
		Print(m_CanSpawn);
		Print(m_NumberOfAI);
		Print(WasGroupDestroyed());
	}
};