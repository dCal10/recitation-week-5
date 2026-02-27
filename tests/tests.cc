#ifndef CATCH_CONFIG_MAIN
#  define CATCH_CONFIG_MAIN
#endif

#include <fstream>

#include "atm.hpp"
#include "catch.hpp"


/////////////////////////////////////////////////////////////////////////////////////////////
// Helper
/////////////////////////////////////////////////////////////////////////////////////////////

bool CompareFiles(const std::string& p1, const std::string& p2) {
  std::ifstream f1(p1);
  std::ifstream f2(p2);

  if (f1.fail() || f2.fail()) return false;

  std::string a, b;
  while (f1 >> a && f2 >> b) {
    if (a != b) return false;
  }

  return f1.eof() && f2.eof();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// RegisterAccount Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("RegisterAccount: Duplicate account throws", "[register]") {
  Atm atm;
  atm.RegisterAccount(1111, 2222, "Alice", 100.0);

  REQUIRE_THROWS_AS(atm.RegisterAccount(1111, 2222, "AliceAgain", 999.0),
                    std::invalid_argument);
}

TEST_CASE("RegisterAccount: Negative initial balance should not be allowed",
          "[register-negative-balance]") {
  Atm atm;

  // If implementation allows this, that is a vulnerability.
  REQUIRE_THROWS_AS(atm.RegisterAccount(3333, 4444, "Bob", -500.0),
                    std::invalid_argument);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WithdrawCash Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("WithdrawCash: Negative amount throws", "[withdraw-negative]") {
  Atm atm;
  atm.RegisterAccount(1234, 1111, "Carol", 100.0);

  REQUIRE_THROWS_AS(atm.WithdrawCash(1234, 1111, -10.0), std::invalid_argument);
}

TEST_CASE("WithdrawCash: Overdraft throws runtime_error",
          "[withdraw-overdraft]") {
  Atm atm;
  atm.RegisterAccount(1234, 1111, "Carol", 100.0);

  REQUIRE_THROWS_AS(atm.WithdrawCash(1234, 1111, 150.0), std::runtime_error);
}

TEST_CASE("WithdrawCash: Transaction is logged correctly", "[withdraw-log]") {
  Atm atm;
  atm.RegisterAccount(5555, 6666, "Dan", 200.0);

  atm.WithdrawCash(5555, 6666, 50.0);

  auto& transactions = atm.GetTransactions();
  REQUIRE(transactions[{5555, 6666}].size() == 1);

  REQUIRE(transactions[{5555, 6666}][0].find("Withdrawal") !=
          std::string::npos);
  REQUIRE(transactions[{5555, 6666}][0].find("50") != std::string::npos);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// DepositCash Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("DepositCash: Negative amount throws", "[deposit-negative]") {
  Atm atm;
  atm.RegisterAccount(7777, 8888, "Eve", 300.0);

  REQUIRE_THROWS_AS(atm.DepositCash(7777, 8888, -25.0), std::invalid_argument);
}

TEST_CASE("DepositCash: Deposit updates balance correctly",
          "[deposit-balance]") {
  Atm atm;
  atm.RegisterAccount(7777, 8888, "Eve", 300.0);

  atm.DepositCash(7777, 8888, 200.0);

  REQUIRE(atm.CheckBalance(7777, 8888) == Approx(500.0));
}

TEST_CASE("DepositCash: Transaction is logged correctly", "[deposit-log]") {
  Atm atm;
  atm.RegisterAccount(9999, 1010, "Frank", 1000.0);

  atm.DepositCash(9999, 1010, 250.0);

  auto& transactions = atm.GetTransactions();
  REQUIRE(transactions[{9999, 1010}].size() == 1);

  REQUIRE(transactions[{9999, 1010}][0].find("Deposit") != std::string::npos);
  REQUIRE(transactions[{9999, 1010}][0].find("250") != std::string::npos);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Invalid Account Access Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("WithdrawCash: Invalid account throws", "[invalid-withdraw]") {
  Atm atm;

  REQUIRE_THROWS_AS(atm.WithdrawCash(1, 1, 10.0), std::invalid_argument);
}

TEST_CASE("DepositCash: Invalid account throws", "[invalid-deposit]") {
  Atm atm;

  REQUIRE_THROWS_AS(atm.DepositCash(1, 1, 10.0), std::invalid_argument);
}

TEST_CASE("CheckBalance: Invalid account throws", "[invalid-balance]") {
  Atm atm;

  REQUIRE_THROWS_AS(atm.CheckBalance(1, 1), std::invalid_argument);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// PrintLedger Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("PrintLedger: Invalid account throws", "[ledger-invalid]") {
  Atm atm;

  REQUIRE_THROWS_AS(atm.PrintLedger("ledger.txt", 1, 1), std::invalid_argument);
}

TEST_CASE("PrintLedger: Ledger matches expected format", "[ledger-format]") {
  Atm atm;
  atm.RegisterAccount(2468, 1357, "Grace", 500.0);

  atm.DepositCash(2468, 1357, 100.0);
  atm.WithdrawCash(2468, 1357, 50.0);

  atm.PrintLedger("generated.txt", 2468, 1357);

  std::ofstream expected("expected.txt");
  expected << "Name: Grace\n";
  expected << "Card Number: 2468\n";
  expected << "PIN: 1357\n";
  expected << "----------------------------\n";
  expected << "Deposit - Amount: $100";
  expected.close();

  REQUIRE(CompareFiles("generated.txt", "expected.txt"));
}