#pragma once
#include <cstdint>

namespace CoopNet
{
class Connection;

bool Ledger_Transfer(Connection* conn, int64_t delta, uint64_t nonce, uint64_t& outBalance);
} // namespace CoopNet
