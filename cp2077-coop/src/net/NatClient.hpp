#pragma once
#include <functional>
#include <juice/juice.h>

namespace CoopNet
{
using CandidateCallback = std::function<void(const char*)>;

void Nat_SetCandidateCallback(CandidateCallback cb);
void Nat_Start();
class Connection;

void Nat_PerformHandshake(Connection* conn);

uint64_t Nat_GetRelayBytes();
void Nat_AddRemoteCandidate(const char* cand);

void Nat_SetTurnCreds(const std::string& host, int port,
                      const std::string& user, const std::string& pass);
bool Nat_GetTurnCreds(std::string& host, int& port,
                      std::string& user, std::string& pass);
}
