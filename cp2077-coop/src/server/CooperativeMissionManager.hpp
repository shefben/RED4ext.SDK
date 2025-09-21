#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <string>
#include <memory>

namespace RED4ext
{
    // Mission state enums
    enum class MissionState : uint8_t
    {
        Inactive = 0,
        Starting = 1,
        InProgress = 2,
        Paused = 3,
        Completed = 4,
        Failed = 5,
        Cancelled = 6
    };

    enum class ObjectiveState : uint8_t
    {
        Inactive = 0,
        Active = 1,
        Completed = 2,
        Failed = 3,
        Optional = 4
    };

    enum class DialogueChoiceResult : uint8_t
    {
        Pending = 0,
        Approved = 1,
        Rejected = 2,
        Timeout = 3
    };

    // Mission data structures
    struct QuestObjective
    {
        std::string objectiveId;
        std::string description;
        ObjectiveState state;
        bool isOptional;
        float progressPercentage;
        std::chrono::steady_clock::time_point lastUpdate;

        QuestObjective() : state(ObjectiveState::Inactive), isOptional(false),
                          progressPercentage(0.0f), lastUpdate(std::chrono::steady_clock::now()) {}
    };

    struct DialogueChoice
    {
        std::string choiceText;
        int32_t choiceIndex;
        uint32_t suggestedById;
        std::chrono::steady_clock::time_point submitTime;
        std::vector<uint32_t> approvals;
        std::vector<uint32_t> rejections;

        DialogueChoice() : choiceIndex(-1), suggestedById(0), submitTime(std::chrono::steady_clock::now()) {}
    };

    struct MissionCheckpoint
    {
        std::string checkpointId;
        std::string questId;
        std::string questPhase;
        std::vector<QuestObjective> objectives;
        std::chrono::steady_clock::time_point creationTime;
        uint32_t createdByPlayerId;

        MissionCheckpoint() : createdByPlayerId(0), creationTime(std::chrono::steady_clock::now()) {}
    };

    struct CooperativeMission
    {
        std::string missionId;
        std::string questId;
        std::string currentPhase;
        MissionState state;
        uint32_t hostPlayerId;

        std::vector<uint32_t> participants;
        std::unordered_map<std::string, QuestObjective> objectives;
        std::vector<MissionCheckpoint> checkpoints;

        // Dialogue system
        bool isInDialogue;
        std::string currentSpeaker;
        std::string currentDialogueId;
        std::vector<DialogueChoice> pendingChoices;
        std::chrono::steady_clock::time_point dialogueTimeout;

        // Mission settings
        bool syncChoices;
        bool syncObjectives;
        bool syncDialogue;
        bool allowIndependentExploration;
        float maxDistanceFromMission;

        // Statistics
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastUpdate;
        uint32_t syncVersion;

        CooperativeMission()
            : state(MissionState::Inactive), hostPlayerId(0), isInDialogue(false),
              syncChoices(true), syncObjectives(true), syncDialogue(true),
              allowIndependentExploration(true), maxDistanceFromMission(500.0f),
              startTime(std::chrono::steady_clock::now()),
              lastUpdate(std::chrono::steady_clock::now()), syncVersion(0) {}
    };

    struct MissionParticipant
    {
        uint32_t playerId;
        std::string playerName;
        bool isReady;
        bool isConnected;
        std::chrono::steady_clock::time_point lastActivity;

        // Position tracking
        float posX, posY, posZ;
        float distanceFromMission;

        // Progress tracking
        uint32_t completedObjectives;
        uint32_t totalObjectives;
        float progressPercentage;

        MissionParticipant() : playerId(0), isReady(false), isConnected(false),
                              lastActivity(std::chrono::steady_clock::now()),
                              posX(0.0f), posY(0.0f), posZ(0.0f), distanceFromMission(0.0f),
                              completedObjectives(0), totalObjectives(0), progressPercentage(0.0f) {}
    };

    // Main cooperative mission management class
    class CooperativeMissionManager
    {
    public:
        static CooperativeMissionManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Mission management
        std::string CreateMission(uint32_t hostPlayerId, const std::string& questId,
                                 const std::vector<uint32_t>& participants);
        bool StartMission(const std::string& missionId);
        bool EndMission(const std::string& missionId);
        bool CancelMission(const std::string& missionId);
        bool PauseMission(const std::string& missionId);
        bool ResumeMission(const std::string& missionId);

        // Participant management
        bool AddParticipant(const std::string& missionId, uint32_t playerId);
        bool RemoveParticipant(const std::string& missionId, uint32_t playerId);
        bool TransferHost(const std::string& missionId, uint32_t newHostId);
        void UpdateParticipantPosition(uint32_t playerId, float x, float y, float z);
        void UpdateParticipantActivity(uint32_t playerId);

        // Quest synchronization
        void SyncQuestStart(const std::string& missionId, const std::string& questId);
        void SyncQuestPhase(const std::string& missionId, const std::string& questPhase);
        void SyncQuestCompletion(const std::string& missionId, bool successful);
        void SyncObjectiveState(const std::string& missionId, const std::string& objectiveId,
                               ObjectiveState state, float progress = 0.0f);

        // Dialogue system
        bool StartDialogue(const std::string& missionId, const std::string& speakerId,
                          const std::string& dialogueId);
        bool SubmitDialogueChoice(const std::string& missionId, uint32_t playerId, int32_t choiceIndex);
        bool ApproveDialogueChoice(const std::string& missionId, uint32_t playerId, int32_t choiceIndex);
        bool RejectDialogueChoice(const std::string& missionId, uint32_t playerId, int32_t choiceIndex);
        void ProcessDialogueTimeout(const std::string& missionId);

        // Checkpoint system
        std::string CreateCheckpoint(const std::string& missionId, uint32_t creatorId,
                                   const std::string& checkpointName);
        bool RestoreCheckpoint(const std::string& missionId, const std::string& checkpointId);
        std::vector<MissionCheckpoint> GetCheckpoints(const std::string& missionId) const;

        // Mission queries
        CooperativeMission* GetMission(const std::string& missionId);
        const CooperativeMission* GetMission(const std::string& missionId) const;
        CooperativeMission* FindMissionByPlayer(uint32_t playerId);
        std::vector<std::string> GetActiveMissions() const;
        std::vector<uint32_t> GetMissionParticipants(const std::string& missionId) const;

        // Mission settings
        bool UpdateMissionSettings(const std::string& missionId, bool syncChoices,
                                  bool syncObjectives, bool syncDialogue,
                                  bool allowIndependentExploration, float maxDistance);

        // Statistics and monitoring
        uint32_t GetActiveMissionCount() const;
        uint32_t GetTotalParticipants() const;
        std::chrono::milliseconds GetAverageMissionDuration() const;

        // Event callbacks
        using MissionStartedCallback = std::function<void(const std::string& missionId, const std::vector<uint32_t>& participants)>;
        using MissionEndedCallback = std::function<void(const std::string& missionId, bool successful)>;
        using ParticipantJoinedCallback = std::function<void(const std::string& missionId, uint32_t playerId)>;
        using ParticipantLeftCallback = std::function<void(const std::string& missionId, uint32_t playerId)>;
        using ObjectiveUpdatedCallback = std::function<void(const std::string& missionId, const std::string& objectiveId, ObjectiveState state)>;
        using DialogueChoiceCallback = std::function<void(const std::string& missionId, uint32_t playerId, int32_t choiceIndex)>;

        void SetMissionStartedCallback(MissionStartedCallback callback);
        void SetMissionEndedCallback(MissionEndedCallback callback);
        void SetParticipantJoinedCallback(ParticipantJoinedCallback callback);
        void SetParticipantLeftCallback(ParticipantLeftCallback callback);
        void SetObjectiveUpdatedCallback(ObjectiveUpdatedCallback callback);
        void SetDialogueChoiceCallback(DialogueChoiceCallback callback);

        // Network synchronization
        void BroadcastMissionState(const std::string& missionId);
        void SyncMissionToPlayer(const std::string& missionId, uint32_t playerId);
        void NotifyObjectiveUpdate(const std::string& missionId, const std::string& objectiveId);
        void NotifyDialogueChoice(const std::string& missionId, int32_t choiceIndex);

    private:
        CooperativeMissionManager() = default;
        ~CooperativeMissionManager() = default;
        CooperativeMissionManager(const CooperativeMissionManager&) = delete;
        CooperativeMissionManager& operator=(const CooperativeMissionManager&) = delete;

        // Internal data
        std::unordered_map<std::string, std::unique_ptr<CooperativeMission>> m_missions;
        std::unordered_map<uint32_t, std::unique_ptr<MissionParticipant>> m_participants;
        std::unordered_map<uint32_t, std::string> m_playerToMission; // Player ID to mission ID mapping

        // Thread safety
        mutable std::shared_mutex m_missionsMutex;
        mutable std::shared_mutex m_participantsMutex;
        mutable std::mutex m_callbacksMutex;

        // Statistics
        uint32_t m_totalMissionsCreated;
        uint32_t m_totalMissionsCompleted;
        std::chrono::steady_clock::time_point m_lastCleanup;

        // Event callbacks
        MissionStartedCallback m_missionStartedCallback;
        MissionEndedCallback m_missionEndedCallback;
        ParticipantJoinedCallback m_participantJoinedCallback;
        ParticipantLeftCallback m_participantLeftCallback;
        ObjectiveUpdatedCallback m_objectiveUpdatedCallback;
        DialogueChoiceCallback m_dialogueChoiceCallback;

        // Internal methods
        std::string GenerateMissionId();
        std::string GenerateCheckpointId();

        void CleanupInactiveMissions();
        void CleanupDisconnectedParticipants();
        void ProcessDialogueTimeouts();
        void UpdateMissionProgress(CooperativeMission* mission);
        void ValidateParticipantProximity(CooperativeMission* mission);

        bool IsPlayerEligibleForMission(uint32_t playerId, const std::string& questId) const;
        bool IsValidQuestId(const std::string& questId) const;
        bool CanPlayerJoinMission(uint32_t playerId, const std::string& missionId) const;
        bool ShouldExecuteChoice(const std::string& missionId, int32_t choiceIndex) const;

        void NotifyMissionStarted(const std::string& missionId);
        void NotifyMissionEnded(const std::string& missionId, bool successful);
        void NotifyParticipantJoined(const std::string& missionId, uint32_t playerId);
        void NotifyParticipantLeft(const std::string& missionId, uint32_t playerId);
        void NotifyObjectiveUpdated(const std::string& missionId, const std::string& objectiveId, ObjectiveState state);
        void NotifyDialogueChoiceMade(const std::string& missionId, uint32_t playerId, int32_t choiceIndex);

        MissionParticipant* FindParticipant(uint32_t playerId);
        const MissionParticipant* FindParticipant(uint32_t playerId) const;

        void SendMissionStateToParticipants(const CooperativeMission* mission);
        void SendObjectiveUpdateToParticipants(const CooperativeMission* mission, const std::string& objectiveId);
        void SendDialogueUpdateToParticipants(const CooperativeMission* mission);
    };

    // Utility functions for mission management
    namespace MissionUtils
    {
        std::string MissionStateToString(MissionState state);
        MissionState StringToMissionState(const std::string& stateStr);

        std::string ObjectiveStateToString(ObjectiveState state);
        ObjectiveState StringToObjectiveState(const std::string& stateStr);

        bool ValidateQuestId(const std::string& questId);
        bool ValidateObjectiveId(const std::string& objectiveId);

        float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2);
        float CalculateMissionProgress(const std::unordered_map<std::string, QuestObjective>& objectives);

        uint32_t HashMissionState(const CooperativeMission& mission);
        std::vector<uint32_t> GetEligiblePlayers(const std::string& questId);
    }

    // Network message structures for client-server communication
    struct MissionStateUpdate
    {
        std::string missionId;
        std::string questId;
        std::string currentPhase;
        MissionState state;
        uint32_t syncVersion;
        std::vector<QuestObjective> objectives;
        bool isInDialogue;
        std::string currentSpeaker;
        std::vector<DialogueChoice> availableChoices;
    };

    struct ObjectiveUpdate
    {
        std::string missionId;
        std::string objectiveId;
        ObjectiveState state;
        float progressPercentage;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct DialogueUpdate
    {
        std::string missionId;
        std::string speakerId;
        std::string dialogueId;
        std::vector<DialogueChoice> choices;
        uint32_t timeoutSeconds;
    };

    struct ParticipantUpdate
    {
        std::string missionId;
        uint32_t playerId;
        std::string playerName;
        bool isReady;
        bool isConnected;
        float posX, posY, posZ;
        uint32_t completedObjectives;
        uint32_t totalObjectives;
    };
}