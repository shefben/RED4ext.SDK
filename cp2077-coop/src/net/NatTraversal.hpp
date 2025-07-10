#pragma once

#include <functional>
#include <juice/juice.h>
#include <string>

namespace CoopNet {

using CandidateCallback = std::function<void(const char*)>;

struct TurnCreds {
    std::string host;
    int port = 0;
    std::string user;
    std::string pass;
};

class NatTraversal {
public:
    NatTraversal();
    ~NatTraversal();

    void SetCandidateCallback(CandidateCallback cb);
    void SetTurnCreds(const TurnCreds& creds);

    void Start();
    bool PerformHandshake(const std::string& remoteCand, uint64_t& relayBytes);

    const std::string& GetLocalCandidate() const;
    bool GetTurnCreds(TurnCreds& creds) const;

private:
    juice_agent_t* m_agent = nullptr;
    CandidateCallback m_callback;
    std::string m_localCandidate;

    TurnCreds m_turnCreds;
    bool m_haveTurn = false;
};

} // namespace CoopNet
