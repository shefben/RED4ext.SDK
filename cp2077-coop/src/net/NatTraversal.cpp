#include "NatTraversal.hpp"
#include <chrono>
#include <iostream>
#include <thread>

namespace CoopNet {

NatTraversal::NatTraversal() = default;

NatTraversal::~NatTraversal() {
    if (m_agent) {
        juice_destroy(m_agent);
    }
}

void NatTraversal::SetCandidateCallback(CandidateCallback cb) {
    m_callback = std::move(cb);
}

void NatTraversal::SetTurnCreds(const TurnCreds& creds) {
    m_turnCreds = creds;
    m_haveTurn = true;
}

void NatTraversal::ClearTurnCreds() {
    m_turnCreds = {};
    m_haveTurn = false;
}

void NatTraversal::Start() {
    juice_config_t cfg = {};
    cfg.stun_server_host = "stun.l.google.com";
    cfg.stun_server_port = 19302;
    cfg.cb_candidate = [](juice_agent_t*, const char* sdp, void* u) {
        auto* self = static_cast<NatTraversal*>(u);
        self->m_localCandidate = sdp ? sdp : "";
        if (self->m_callback) {
            self->m_callback(sdp);
        }
    };
    cfg.cb_state_changed = nullptr;
    cfg.user_ptr = this;
    m_agent = juice_create(&cfg);
    if (m_agent) {
        juice_gather_candidates(m_agent);
    }
}

const std::string& NatTraversal::GetLocalCandidate() const {
    return m_localCandidate;
}

bool NatTraversal::GetTurnCreds(TurnCreds& creds) const {
    if (!m_haveTurn)
        return false;
    creds = m_turnCreds;
    return true;
}

bool NatTraversal::PerformHandshake(const std::string& remoteCand, uint64_t& relayBytes) {
    if (!m_agent || remoteCand.empty())
        return false;

    relayBytes = 0;

    // Set remote description to initiate connection
    int result = juice_set_remote_description(m_agent, remoteCand.c_str());
    if (result != 0) {
        return false;
    }

    // Wait for connection with timeout
    auto totalStart = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - totalStart < std::chrono::seconds(20)) {
        juice_state_t state = juice_get_state(m_agent);
        if (state == JUICE_STATE_CONNECTED) {
            // Connection successful
            return true;
        }
        if (state == JUICE_STATE_FAILED) {
            // Connection failed
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

} // namespace CoopNet
