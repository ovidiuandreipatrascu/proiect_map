// Starter skeleton for the MAP project.
// Pure logic lives here so it can be unit tested without starting a server.
#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace map_project {

inline constexpr const char* kAppName = "map-project";
inline constexpr const char* kAppVersion = "0.1.0";
inline constexpr int kMaxNameLength = 50;

/// Reads an environment variable, falling back when it is missing or empty.
inline std::string Env(const char* key, const std::string& fallback) {
    const char* value = std::getenv(key);
    return (value == nullptr || *value == '\0') ? fallback : std::string(value);
}

/// Helper function to convert a string to lowercase for case-insensitive comparison.
inline std::string ToLower(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

/// Structure representing a contact in the agenda.
struct Contact {
    int id = 0;
    std::string name;
    std::string email;
    std::string phone;
    std::string category;
};

inline nlohmann::json ContactToJson(const Contact& c) {
    return {
        {"id", c.id},
        {"name", c.name},
        {"email", c.email},
        {"phone", c.phone},
        {"category", c.category}
    };
}

inline nlohmann::json ContactsToJson(const std::vector<Contact>& contacts) {
    nlohmann::json array = nlohmann::json::array();
    for (const auto& c : contacts) {
        array.push_back(ContactToJson(c));
    }
    return array;
}

/// Validates that name has between 1 and L (50) characters.
inline bool IsValidName(const std::string& name) {
    return !name.empty() && name.size() <= kMaxNameLength;
}

/// Validates that email has exactly one '@', with text before it and at least one '.' after it.
inline bool IsValidEmail(const std::string& email) {
    const size_t at_pos = email.find('@');
    if (at_pos == std::string::npos || at_pos == 0) {
        return false;
    }
    // Exactly one '@'
    if (email.find('@', at_pos + 1) != std::string::npos) {
        return false;
    }
    // At least one '.' after '@'
    const size_t dot_pos = email.find('.', at_pos + 1);
    if (dot_pos == std::string::npos) {
        return false;
    }
    return true;
}

/// Validates that phone has exactly 10 digits, optionally preceded by '+'.
inline bool IsValidPhone(const std::string& phone) {
    size_t start = 0;
    if (!phone.empty() && phone[0] == '+') {
        start = 1;
    }
    if (phone.size() - start != 10) {
        return false;
    }
    for (size_t i = start; i < phone.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(phone[i]))) {
            return false;
        }
    }
    return true;
}

/// Validates that category is one of: family, friends, work, other.
inline bool IsValidCategory(const std::string& category) {
    return category == "family" || category == "friends" ||
           category == "work" || category == "other";
}

/// Holds the application data in memory.
/// Everything is lost when the container restarts.
class Store {
public:
    void Reset() {
        std::lock_guard<std::mutex> guard(mutex_);
        next_id_ = 1;
        contacts_.clear();
    }

    int NextId() {
        std::lock_guard<std::mutex> guard(mutex_);
        return next_id_;
    }

    int TakeNextId() {
        std::lock_guard<std::mutex> guard(mutex_);
        return next_id_++;
    }

    /// Checks if a contact with the same email already exists (case-insensitive).
    bool HasEmail(const std::string& email) {
        std::lock_guard<std::mutex> guard(mutex_);
        const std::string target = ToLower(email);
        for (const auto& [id, contact] : contacts_) {
            if (ToLower(contact.email) == target) {
                return true;
            }
        }
        return false;
    }

    /// Adds a contact and returns the stored entity with its assigned ID.
    Contact AddContact(std::string name, std::string email, std::string phone, std::string category) {
        std::lock_guard<std::mutex> guard(mutex_);
        const int id = next_id_++;
        Contact contact{id, std::move(name), std::move(email), std::move(phone), std::move(category)};
        contacts_[id] = contact;
        return contact;
    }

    /// Returns a contact by ID if it exists.
    std::optional<Contact> GetContact(int id) {
        std::lock_guard<std::mutex> guard(mutex_);
        auto it = contacts_.find(id);
        if (it != contacts_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /// Returns contacts matching optional category and query, sorted by name ascending (and id ascending as tie-breaker).
    std::vector<Contact> GetContacts(const std::optional<std::string>& category = std::nullopt,
                                     const std::optional<std::string>& q = std::nullopt) {
        std::lock_guard<std::mutex> guard(mutex_);
        std::vector<Contact> result;
        const std::string lower_q = q ? ToLower(*q) : "";

        for (const auto& [id, contact] : contacts_) {
            if (category && contact.category != *category) {
                continue;
            }
            if (q && ToLower(contact.name).find(lower_q) == std::string::npos) {
                continue;
            }
            result.push_back(contact);
        }

        std::sort(result.begin(), result.end(), [](const Contact& a, const Contact& b) {
            if (a.name != b.name) {
                return a.name < b.name;
            }
            return a.id < b.id;
        });

        return result;
    }

    /// Returns a copy of all contacts.
    std::vector<Contact> GetAllContacts() {
        return GetContacts();
    }

private:
    std::mutex mutex_;
    int next_id_ = 1;
    std::map<int, Contact> contacts_;
};

/// Seconds since the process started.
inline long UptimeSeconds(std::chrono::steady_clock::time_point started_at) {
    const auto elapsed = std::chrono::steady_clock::now() - started_at;
    return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
}

inline nlohmann::json HealthBody(std::chrono::steady_clock::time_point started_at) {
    return {{"status", "ok"}, {"uptime_seconds", UptimeSeconds(started_at)}};
}

inline nlohmann::json VersionBody() {
    return {
        {"app", kAppName},
        {"version", kAppVersion},
        {"commit", Env("APP_COMMIT", "dev")},
        {"built_at", Env("APP_BUILT_AT", "unknown")},
    };
}

inline nlohmann::json ErrorBody(const std::string& code, const std::string& message) {
    return {{"error", code}, {"message", message}};
}

inline std::string HomePage() {
    return std::string(R"(<!DOCTYPE html>
<html lang="ro"><head><meta charset="utf-8"><title>)") + kAppName + R"(</title></head>
<body>
<h1>)" + kAppName + R"(</h1>
<p>Autor: Patrascu Ovidiu-Andrei, grupa 2.1</p>
<p>Tema: 1 - Agenda de contacte</p>
<p>Versiune: )" + kAppVersion + ", commit " + Env("APP_COMMIT", "dev") + R"(</p>
</body></html>)";
}

}  // namespace map_project
