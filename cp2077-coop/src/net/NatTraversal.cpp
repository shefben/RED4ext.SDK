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

void NatTraversal::Start() {
    juice_agent_config_t cfg = JUICE_AGENT_CONFIG_DEFAULT;
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
    if (juice_create(&cfg, &m_agent) == 0) {
        juice_set_user_pointer(m_agent, this);
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

    bool connected = false;
    relayBytes = 0;

    juice_set_state_changed_cb(
        m_agent,
        [](juice_agent_t*, juice_state_t st, void* u) {
            if (st == JUICE_STATE_CONNECTED)
                *static_cast<bool*>(u) = true;
        },
        &connected);

    juice_set_remote_description(m_agent, remoteCand.c_str());
    juice_connect(m_agent);

    auto start = std::chrono::steady_clock::now();
    while (!connected) {
        juice_poll(m_agent);
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
            TurnCreds creds;
            if (GetTurnCreds(creds)) {
                std::cout << "ICE failed, trying TURN" << std::endl;
                juice_destroy(m_agent);
                juice_agent_config_t cfg = JUICE_AGENT_CONFIG_DEFAULT;
                cfg.stun_server_host = "stun.l.google.com";
                cfg.stun_server_port = 19302;
                cfg.turn_server_host = creds.host.c_str();
                cfg.turn_server_port = creds.port;
                cfg.turn_username = creds.user.c_str();
                cfg.turn_password = creds.pass.c_str();
                cfg.cb_candidate = [](juice_agent_t*, const char* sdp, void* u) {
                    auto* self = static_cast<NatTraversal*>(u);
                    self->m_localCandidate = sdp ? sdp : "";
                    if (self->m_callback) {
                        self->m_callback(sdp);
                    }
                };
                cfg.cb_state_changed = [](juice_agent_t*, juice_state_t st, void* u) {
                    if (st == JUICE_STATE_CONNECTED)
                        *static_cast<bool*>(u) = true;
                };
                if (juice_create(&cfg, &m_agent) == 0) {
                    juice_set_user_pointer(m_agent, this);
                    juice_set_remote_description(m_agent, remoteCand.c_str());
                    juice_connect(m_agent);
                    start = std::chrono::steady_clock::now();
                    continue;
                }
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (connected) {
        extern uint64_t juice_get_bytes_relayed(juice_agent_t*);
        relayBytes = juice_get_bytes_relayed(m_agent);
        return true;
    }
    return false;
}

} // namespace CoopNet

