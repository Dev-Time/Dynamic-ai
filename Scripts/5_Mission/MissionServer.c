/*
name:TrueDolphin
date:21/7/2023
spatial ai spawns

renaming functions, checking stuff.

*/
modded class MissionServer {
  const int SZ_IN_SAFEZONE = 0x0001;
  int m_cur = 0;
  int m_Spatial_cur = 0;
  ref array < Man > Spatial_PlayerList = new array < Man > ;
  ref Spatial_Groups m_Spatial_Groups;
  ref Spatial_Players m_Spatial_Players;

  #ifdef EXPANSIONMODSPAWNSELECTION
    private ExpansionRespawnHandlerModule m_RespawnModule;
    
    bool InRespawnMenu(PlayerIdentity identity) {
      CF_Modules < ExpansionRespawnHandlerModule > .Get(m_RespawnModule);
      if (m_RespawnModule && m_RespawnModule.IsPlayerInSpawnSelect(identity)) {
        return true;
      }
      return false;
    } //LM's code altered to a bool in class
  #endif

  void MissionServer() {
    SpatialLoggerPrint("Spatial AI Date: 21/7/2023");
    if (GetSpatialSettings().Init() == true) {
      GetSpatialSettings().PullRef(m_Spatial_Groups);
      SpatialPlayerSettings().PullRef(m_Spatial_Players); 
      InitSpatialTriggers();
      SpatialLoggerPrint("Spatial AI Enabled");
      if (m_Spatial_Groups.Spatial_MinTimer == 60000) SpatialLoggerPrint("Spatial Debug Mode on");
      SpatialTimer();
    }
  } //constructor

  void SpatialTimer() {
    EXPrint("Spatial::Stack - Start");
    Spatial_Check();
    EXPrint("Spatial::Stack - End");
    int m_cor = Math.RandomIntInclusive(m_Spatial_Groups.Spatial_MinTimer, m_Spatial_Groups.Spatial_MaxTimer);
    SpatialLoggerPrint("Next valid check in: " + m_cor + "ms");
    GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SpatialTimer, m_cor, false);
  } //timer call for varied check loops #refactored by LieutenantMaster

  void Spatial_Check() {
    EXPrint("Spatial::Check - Start");
      // If there are no players in the list, return early.
      if (m_Players.Count() == 0) return;

      int playerChecks = m_Spatial_Groups.PlayerChecks;
      bool checkOnlyLeader = playerChecks >= 0;
      bool hasLimitedChecks = playerChecks != 0;

      // Make sure playerChecks is positive, even if it was initially negative.
      playerChecks = Math.AbsInt(playerChecks);

      int minAge = m_Spatial_Groups.MinimumAge;
      bool checkMinAge = minAge > 0;
      Spatial_PlayerList = m_Players;
      for (int i = 0; i < Spatial_PlayerList.Count() + 1; i++)
      {
        PlayerBase player = PlayerBase.Cast(Spatial_PlayerList.GetRandomElement());
        Spatial_PlayerList.RemoveItem(player);
        if (!player || !player.IsAlive() || !player.GetIdentity())
          continue;
        if (checkOnlyLeader)
        {
          eAIGroup PlayerGroup = eAIGroup.Cast(player.GetGroup());
          if (!PlayerGroup || player != player.GetGroup().GetLeader())
            continue;
        }

      #ifdef EXPANSIONMODSPAWNSELECTION
          // If the player is in the respawn menu, skip to the next iteration.
          if (InRespawnMenu(player.GetIdentity()))
              continue;
      #endif

          if (checkMinAge && !player.Spatial_CheckAge(minAge))
            continue;

          if (Spatial_CanSpawn(player))
            Spatial_LocalSpawn(player);

          if (!hasLimitedChecks)
            continue;
          if (i == playerChecks) return;
      }
    EXPrint("Spatial::Check - End");
  } // Standard loop through the player list, selecting random players and removing them from the list. #refactored by LieutenantMaster

  bool Spatial_CanSpawn(PlayerBase player) {
    EXPrint("Spatial::CanSpawn - Start");
    if (player.IsBleeding() && !GetSpatialSettings().Spatial_IsBleeding())
      return false;
    if (player.IsInVehicle() && !GetSpatialSettings().Spatial_InVehicle())
      return false;
    if (player.IsUnconscious() && !GetSpatialSettings().Spatial_IsUnconscious())
      return false;
    if (player.IsRestrained() && !GetSpatialSettings().Spatial_IsRestrained())
      return false;
    #ifdef EXPANSIONMODBASEBUILDING
      // Check if player is inside their own territory and its override
      if (player.IsInsideOwnTerritory() && !GetSpatialSettings().Spatial_InOwnTerritory())
        return false;
    #endif
    if (player.Expansion_IsInSafeZone() && !GetSpatialSettings().Spatial_IsInSafeZone())
      return false;

    #ifdef SZDEBUG
      // Check for specific safe zone debug status and its override
      if (player.GetSafeZoneStatus() == SZ_IN_SAFEZONE && !GetSpatialSettings().Spatial_TPSafeZone())
        return false;
    #endif
    if (m_Spatial_Groups.Points_Enabled == 2 && !player.Spatial_CheckZone())
      return false;
    if (m_Spatial_Groups.Points_Enabled != 0 && player.Spatial_CheckSafe())
      return false;
    return true;
    EXPrint("Spatial::CanSpawn - End");
  } //checks and overrides

  void Spatial_LocalSpawn(PlayerBase player) {
    EXPrint("Spatial::LocalSpawn - Start");
    if (!player) return;

    int m_Groupid = Math.RandomIntInclusive(0, 5000);
    SpatialDebugPrint("GroupID: " + m_Groupid);

    m_cur = Math.RandomIntInclusive(0, m_Spatial_Groups.Group.Count() - 1);
    int SpawnCount, lootable, ammo;
    Spatial_Group group;
    string faction, loadout, name;
    float chance;
    if (player.Spatial_CheckZone() == true) {
      SpawnCount = Math.RandomIntInclusive(player.Spatial_MinCount, player.Spatial_MaxCount);
      faction = player.Spatial_Faction();
      loadout = player.Spatial_Loadout();
      name = player.Spatial_Name();
      lootable = player.Spatial_Lootable();
      chance = player.Spatial_Chance();
      ammo = player.Spatial_UnlimitedReload();
    } else {
      group = Spatial_GetWeightedGroup(m_Spatial_Groups.Group);
      SpawnCount = Math.RandomIntInclusive(group.Spatial_MinCount, group.Spatial_MaxCount);
      faction = group.Spatial_Faction;
      loadout = group.Spatial_Loadout;
      name = group.Spatial_Name;
      lootable = group.Spatial_Lootable;
      chance = group.Spatial_Chance;
      ammo = group.Spatial_UnlimitedReload;
    }

    float random = Math.RandomFloat(0.0, 1.0);
    SpatialDebugPrint("Chance: " + chance + " | random: " + random);
    if (chance < random) return;

    if (SpawnCount > 0) {
      if (m_Spatial_Groups.GroupDifficulty == 1) {
        eAIGroup PlayerGroup;
        PlayerGroup = eAIGroup.Cast(player.GetGroup());
        if (!PlayerGroup) PlayerGroup = eAIGroup.GetGroupByLeader(player);
        if (PlayerGroup.Count() > 1) SpawnCount += (PlayerGroup.Count() - 1);
      }
    Spatial_Spawn(player, SpawnCount, faction, loadout, name, lootable, ammo);
    } else {
      SpatialDebugPrint("group/point ai count too low this check");
    }

    SpatialDebugPrint("End GroupID: " + m_Groupid);
    EXPrint("Spatial::LocalSpawn - End");
  } //create and stuff.

  Spatial_Group Spatial_GetWeightedGroup(array < ref Spatial_Group > groups, array < float > weights = NULL) {
    array < float > weights_T = weights;
    if (weights_T == NULL) {
      weights_T = new array < float > ;
      for (int i = 0; i < groups.Count(); i++) {
        weights_T.Insert(groups[i].Spatial_Weight);
      }
    }
    int index = ExpansionStatic.GetWeightedRandom(weights_T);
    if (index > -1)
      return groups[index];
    //! Should not happen
    Print("[Spatial_Group] GetWeightedGroup: All Groups have a 'Weight' of zero. Selecting pure random group instead.");
    return groups.GetRandomElement();
  } //expansion lightweight weighted group calcs

  bool Spatial_ValidPos(PlayerBase player, out vector pos) {
    pos = (ExpansionStatic.GetSurfacePosition(ExpansionMath.GetRandomPointInRing(player.GetPosition(), m_Spatial_Groups.MinDistance, m_Spatial_Groups.MaxDistance)));
    for (int i = 0; i < 50; i++) {
      if (GetGame().SurfaceIsSea(pos[0], pos[2]) || GetGame().SurfaceIsPond(pos[0], pos[2])) {
        pos = (ExpansionStatic.GetSurfacePosition(ExpansionMath.GetRandomPointInRing(player.GetPosition(), m_Spatial_Groups.MinDistance, m_Spatial_Groups.MaxDistance)));
      } else i = 50;
    }
    if (GetGame().SurfaceIsSea(pos[0], pos[2]) || GetGame().SurfaceIsPond(pos[0], pos[2])) return false;
    return true;
  } //fixed

  void Spatial_Spawn(PlayerBase player, int bod, string fac, string loa, string GroupName, int Lootable, bool UnlimitedReload) {
    EXPrint("Spatial::Spawn - Start");
    vector startpos, waypointpos;
    if (!Spatial_ValidPos(player, startpos)){
      SpatialLoggerPrint("Valid spawnpos not found. not spawning.");
      return;
    }
    if (!Spatial_ValidPos(player, waypointpos)){
      SpatialLoggerPrint("Valid waypointpos not found. using startpos.");
      waypointpos = startpos;
    }
    TVectorArray waypoints = { waypointpos };
    string Formation = "RANDOM";
    eAIWaypointBehavior behaviour = typename.StringToEnum(eAIWaypointBehavior, "ALTERNATE");
    int mindistradius, maxdistradius, despawnradius;
    mindistradius = 0;
    maxdistradius = 1200;
    despawnradius = 1200;
    auto dynPatrol = eAISpatialPatrol.CreateEx(startpos, waypoints, behaviour, loa, bod, m_Spatial_Groups.CleanupTimer + 500, m_Spatial_Groups.CleanupTimer - 500, eAIFaction.Create(fac), eAIFormation.Create(Formation), player, mindistradius, maxdistradius, despawnradius, 2.0, 3.0, Lootable, UnlimitedReload);
    if (dynPatrol) {
      dynPatrol.SetGroupName(GroupName);
      dynPatrol.SetSniperProneDistanceThreshold(0.0);
      dynPatrol.SetHunted(player);
    }
    Spatial_message(player, m_Spatial_Groups.MessageType, bod, fac, loa);
    EXPrint("Spatial::Spawn - End");
  } //Spatial_Spawn(player, SpawnCount, faction, loadout, unlimitedreload)

  void InitSpatialTriggers() {
        EXPrint("Spatial::Triggers - Start");
    if (m_Spatial_Groups.Points_Enabled == 1 || m_Spatial_Groups.Points_Enabled == 2) {
      SpatialLoggerPrint("points Enabled");
      foreach(Spatial_Point points: m_Spatial_Groups.Point) {
        Spatial_Trigger spatial_trigger = Spatial_Trigger.Cast(GetGame().CreateObjectEx("Spatial_Trigger", points.Spatial_Position, ECE_NONE));
        spatial_trigger.SetCollisionCylinder(points.Spatial_Radius, points.Spatial_Radius / 2);
        spatial_trigger.Spatial_SetData(points.Spatial_Safe, points.Spatial_Faction, points.Spatial_ZoneLoadout, points.Spatial_MinCount, points.Spatial_MaxCount, points.Spatial_HuntMode, points.Spatial_Name, points.Spatial_Lootable, points.Spatial_Chance, points.Spatial_UnlimitedReload);
        SpatialLoggerPrint("Trigger at location: " + points.Spatial_Position + " - Radius: " + points.Spatial_Radius);
        SpatialLoggerPrint("Safe: " + points.Spatial_Safe + " - Faction: " + points.Spatial_Faction + " - Loadout: " + points.Spatial_ZoneLoadout + " - counts: " + points.Spatial_MinCount + ":" + points.Spatial_MaxCount);
      }
    } else {
      SpatialLoggerPrint("points Disabled");
    }

    if (m_Spatial_Groups.Locations_Enabled != 0) {
      SpatialLoggerPrint("Locations Enabled");
      foreach(Spatial_Location location: m_Spatial_Groups.Location) {
        Location_Trigger location_trigger = Location_Trigger.Cast(GetGame().CreateObjectEx("Location_Trigger", location.Spatial_TriggerPosition, ECE_NONE));
        location_trigger.SetCollisionCylinder(location.Spatial_TriggerRadius, location.Spatial_TriggerRadius / 2);
        location_trigger.Spatial_SetData(location.Spatial_Name, location.Spatial_TriggerRadius, location.Spatial_ZoneLoadout, location.Spatial_MinCount, location.Spatial_MaxCount, location.Spatial_HuntMode, location.Spatial_Faction, location.Spatial_TriggerPosition, location.Spatial_SpawnPosition, location.Spatial_Lootable, location.Spatial_Timer, location.Spatial_Chance, location.Spatial_UnlimitedReload);
        SpatialLoggerPrint("Trigger at location: " + location.Spatial_TriggerPosition + " - Radius: " + location.Spatial_TriggerRadius + " - Spawn location: " + location.Spatial_SpawnPosition);
        SpatialLoggerPrint("Faction: " + location.Spatial_Faction + " - Loadout: " + location.Spatial_ZoneLoadout + " - counts: " + location.Spatial_MinCount + ":" + location.Spatial_MaxCount);
      }
    } else {
      SpatialLoggerPrint("Locations Disabled");
    }
        EXPrint("Spatial::Triggers - End");
  } //trigger zone initialisation

  void Spatial_message(PlayerBase player, int msg_no, int SpawnCount, string faction, string loadout) {
    if (!player) return;
    string message = string.Format("Player: %1 Number: %2, Faction name: %3, Loadout: %4", player.GetIdentity().GetName(), SpawnCount, faction, loadout);
    if (msg_no == 0) {
      SpatialLoggerPrint(message);
    } else if (msg_no == 1) {
      Spatial_WarningMessage(player, string.Format("%1 %2", SpawnCount, m_Spatial_Groups.MessageText));
      SpatialLoggerPrint(message);
    } else if (msg_no == 2) {
      Spatial_WarningMessage(player, m_Spatial_Groups.MessageText);
      SpatialLoggerPrint(message);
    } else if (msg_no == 3) {
      NotificationSystem.SendNotificationToPlayerExtended(player, 5, m_Spatial_Groups.MessageTitle, string.Format("%1 %2", SpawnCount, m_Spatial_Groups.MessageText), "set:dayz_gui image:tutorials");
      SpatialLoggerPrint(message);
    } else if (msg_no == 4) {
      NotificationSystem.SendNotificationToPlayerExtended(player, 5, m_Spatial_Groups.MessageTitle, m_Spatial_Groups.MessageText, "set:dayz_gui image:tutorials");
      SpatialLoggerPrint(message);
    } else if (msg_no == 5 && player.Spatial_HasGPSReceiver()) {
      NotificationSystem.SendNotificationToPlayerExtended(player, 5, m_Spatial_Groups.MessageTitle, m_Spatial_Groups.MessageText, "set:dayz_gui image:tutorials");
      SpatialLoggerPrint(message);
    }
  } //chat message or vanilla notification

  void Spatial_WarningMessage(PlayerBase player, string message) {
    if ((player) && (message != "")) {
      Param1 < string > Msgparam;
      Msgparam = new Param1 < string > (message);
      GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, Msgparam, true, player.GetIdentity());
    }
  } // Ingame chat message

  void SpatialLoggerPrint(string msg) {
      GetExpansionSettings().GetLog().PrintLog("[Spatial AI] " + msg);
  } //expansion logging

  void SpatialDebugPrint(string msg) {
    if (m_Spatial_Groups.Spatial_MinTimer == 60000)
      GetExpansionSettings().GetLog().PrintLog("[Spatial Debug] " + msg);
  } //expansion debug print
};