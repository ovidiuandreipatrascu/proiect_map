#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <chrono>

#include "app.hpp"

using namespace map_project;

TEST_CASE("health body reports ok and uptime") {
    const auto started_at = std::chrono::steady_clock::now();
    const auto body = HealthBody(started_at);

    CHECK(body["status"] == "ok");
    CHECK(body.contains("uptime_seconds"));
    CHECK(body["uptime_seconds"].get<long>() >= 0);
}

TEST_CASE("version body contains all required fields") {
    const auto body = VersionBody();

    CHECK(body["app"] == kAppName);
    CHECK(body["version"] == kAppVersion);
    CHECK(body.contains("commit"));
    CHECK(body.contains("built_at"));
}

TEST_CASE("env falls back when the variable is missing") {
    CHECK(Env("MAP_VARIABLE_THAT_DOES_NOT_EXIST", "fallback") == "fallback");
}

TEST_CASE("error body has the standard shape") {
    const auto body = ErrorBody("not_found", "route does not exist");

    CHECK(body["error"] == "not_found");
    CHECK(body["message"] == "route does not exist");
}

TEST_CASE("new store starts at one") {
    Store store;
    CHECK(store.NextId() == 1);
}

TEST_CASE("take next id increments") {
    Store store;

    CHECK(store.TakeNextId() == 1);
    CHECK(store.TakeNextId() == 2);
    CHECK(store.NextId() == 3);
}

TEST_CASE("reset returns to one") {
    Store store;
    store.TakeNextId();
    store.TakeNextId();

    store.Reset();

    CHECK(store.NextId() == 1);
}

TEST_CASE("home page mentions the application name") {
    CHECK(HomePage().find(kAppName) != std::string::npos);
}

TEST_CASE("validate name length between 1 and 50 characters") {
    CHECK_FALSE(IsValidName(""));
    CHECK(IsValidName("A"));
    CHECK(IsValidName(std::string(50, 'x')));
    CHECK_FALSE(IsValidName(std::string(51, 'x')));
}

TEST_CASE("validate email format with one @ and dot after") {
    CHECK(IsValidEmail("ana@example.com"));
    CHECK(IsValidEmail("a@b.c"));
    CHECK_FALSE(IsValidEmail("fara-at.com"));
    CHECK_FALSE(IsValidEmail("@start.com"));
    CHECK_FALSE(IsValidEmail("ana@farapunct"));
    CHECK_FALSE(IsValidEmail("ana@@example.com"));
}

TEST_CASE("validate phone requires 10 digits optional leading plus") {
    CHECK(IsValidPhone("0722334455"));
    CHECK(IsValidPhone("+0722334455"));
    CHECK_FALSE(IsValidPhone("072233445"));     // 9 digits
    CHECK_FALSE(IsValidPhone("07223344556"));    // 11 digits without +
    CHECK_FALSE(IsValidPhone("+072233445"));    // + and 9 digits
    CHECK_FALSE(IsValidPhone("072233445a"));    // non-digit
}

TEST_CASE("validate category against allowed set") {
    CHECK(IsValidCategory("family"));
    CHECK(IsValidCategory("friends"));
    CHECK(IsValidCategory("work"));
    CHECK(IsValidCategory("other"));
    CHECK_FALSE(IsValidCategory("colleagues"));
    CHECK_FALSE(IsValidCategory(""));
}

TEST_CASE("store adds contact and detects case-insensitive email conflicts") {
    Store store;
    const auto contact = store.AddContact("Ana Pop", "ana@example.com", "0722334455", "work");

    CHECK(contact.id == 1);
    CHECK(contact.name == "Ana Pop");
    CHECK(contact.email == "ana@example.com");

    // Email conflict with different casing must be detected
    CHECK(store.HasEmail("Ana@Example.COM"));
    CHECK(store.HasEmail("ana@example.com"));
    CHECK_FALSE(store.HasEmail("dan@example.com"));

    // Reset clears contacts and email registry
    store.Reset();
    CHECK_FALSE(store.HasEmail("ana@example.com"));
    CHECK(store.GetAllContacts().empty());
}

TEST_CASE("store retrieves contact by id or reports not found") {
    Store store;
    store.AddContact("Dan Ilie", "dan@example.com", "0711223344", "family");

    const auto found = store.GetContact(1);
    CHECK(found.has_value());
    CHECK(found->name == "Dan Ilie");

    const auto missing = store.GetContact(999);
    CHECK_FALSE(missing.has_value());
}

TEST_CASE("store lists contacts ordered by name ascending and tie breaker by id") {
    Store store;
    store.AddContact("Zoe Radu", "zoe@example.com", "0733112233", "friends");
    store.AddContact("Ana Pop", "ana1@example.com", "0722334455", "work");
    store.AddContact("Ana Pop", "ana2@example.com", "0722334456", "family");

    const auto list = store.GetContacts();
    REQUIRE(list.size() == 3);
    CHECK(list[0].name == "Ana Pop");
    CHECK(list[0].id == 2);
    CHECK(list[1].name == "Ana Pop");
    CHECK(list[1].id == 3);
    CHECK(list[2].name == "Zoe Radu");
}

TEST_CASE("store filters contacts by category and query substring case-insensitively") {
    Store store;
    store.AddContact("Ana Pop", "ana@example.com", "0722334455", "work");
    store.AddContact("Bogdan Pop", "bogdan@example.com", "0744112233", "work");
    store.AddContact("Ana Ionescu", "ana.i@example.com", "0755112233", "friends");

    // Filter by category only
    const auto work_contacts = store.GetContacts("work", std::nullopt);
    CHECK(work_contacts.size() == 2);

    // Search query only (case-insensitive substring "ana")
    const auto ana_contacts = store.GetContacts(std::nullopt, "ana");
    CHECK(ana_contacts.size() == 2); // "Ana Pop" and "Ana Ionescu"

    // Combined category AND query
    const auto combined = store.GetContacts("work", "ana");
    REQUIRE(combined.size() == 1);
    CHECK(combined[0].name == "Ana Pop");
    CHECK(combined[0].category == "work");
}
